#include "server.hpp"
#include "download.hpp"
#include "jsonconv.hpp"
#include "library_messages.hpp"

#include <QJsonDocument>
#include <QProcess>
#include <QDebug>
#include <QtGlobal>

using namespace Moosick;
using namespace MoosickMessage;

DownloaderThread::DownloaderThread(const DownloadRequest &request, Server *server)
    : m_request(request)
    , m_server(server)
{
}

DownloaderThread::~DownloaderThread()
{
}

void DownloaderThread::run()
{
    const QString toolDir = m_server->m_settings.toolsDir();
    const QString tmpDir = m_server->m_settings.tempDir();
    const QString mediaDir = m_server->m_settings.mediaBaseDir();

    QTcpSocket tcpSocket;
    tcpSocket.connectToHost("localhost", m_server->m_settings.dbserverPort());
    if (!tcpSocket.waitForConnected(1000)) {
        qCritical() << "Unable to connect to" << tcpSocket.peerName() << ", port =" << tcpSocket.peerPort();
        return;
    }

    auto result = download(tcpSocket, m_request, mediaDir, toolDir, tmpDir);
    if (!result.hasValue()) {
        qWarning().noquote() << "Error while downloading:" << result.takeError();
        return ;
    }

    QVector<Moosick::CommittedLibraryChange> changes = result.takeValue();
    qCritical().noquote() << "Download finished, results:" << changes.size();

    emit done(this);
}

Server::Server(const ServerSettings &settings)
    : TcpServer()
    , m_settings(settings)
{
    listen(settings.downloaderPort());
}

Server::~Server()
{
}

QByteArray Server::handleMessage(const QByteArray &data)
{
    Result<Message, JsonifyError> messageParsingResult = Message::fromJson(data);
    if (messageParsingResult.hasError()) {
        qWarning().noquote() << "Error parsing message:" << messageParsingResult.takeError().toString();
        return QByteArray();
    }

    Message message = messageParsingResult.takeValue();

    switch (message.getType()) {
    case Type::DownloadRequest: {
        const DownloadRequest *downloadRequest = message.as<DownloadRequest>();
        const quint32 id = startDownload(*downloadRequest);
        DownloadResponse response;
        response.downloadId = id;
        return messageToJson(response);
    }
    case Type::DownloadQuery: {
        DownloadQueryResponse response;
        for (auto it = m_downloads.begin(); it != m_downloads.end(); ++it)
            response.activeRequests->append(DownloadQueryResponse::ActiveDownload{ it.key(), it.value().request });
        return messageToJson(response);
    }
    default: {
        return messageToJson(Error());
    }
    }
}

quint32 Server::startDownload(const MoosickMessage::DownloadRequest &request)
{
    const quint32 id = m_nextDownloadId++;

    qDebug() << "Starting download" << id << ":" << (int) *request.requestType << *request.url << *request.albumName << *request.artistId << *request.artistName;

    DownloaderThread *thread = new DownloaderThread(request, this);
    m_downloads[id] = RunningDownload { request, thread };
    connect(thread, &DownloaderThread::done, this, &Server::onDownloadFinished);

    thread->start();

    return id;
}

void Server::onDownloadFinished(DownloaderThread *thread)
{
    for (auto it = m_downloads.begin(); it != m_downloads.end(); ++it) {
        if (it->thread == thread) {
            m_downloads.erase(it);
            break;
        }
    }
    thread->wait();
    thread->deleteLater();
}
