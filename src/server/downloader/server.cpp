#include "server.hpp"
#include "download.hpp"
#include "jsonconv.hpp"

#include <QJsonDocument>
#include <QtGlobal>

Server::Server(const QString &argv0, const QString &media, const QString &tool, const QString temp, quint16 port)
    : NetCommon::TcpServer()
    , m_program(argv0)
    , m_media(media)
    , m_tool(tool)
    , m_temp(temp)
{
    listen(port);
}

Server::~Server()
{
}

bool Server::handleMessage(const ClientCommon::Message &message, ClientCommon::Message &response)
{
    switch (message.tp) {
    case ClientCommon::DownloadRequest: {
        NetCommon::DownloadRequest request;
        if (!request.fromBase64(message.data)) {
            qWarning() << "Failed parsing the base64 download request";
            response = { ClientCommon::DownloadResponse, "" };
        }
        else {
            const quint32 id = startDownload(request);
            response = { ClientCommon::DownloadResponse, QByteArray::number(id) };
        }
        return true;
    }
    case ClientCommon::DownloadQuery: {
        response = { ClientCommon::DownloadResponse, QJsonDocument(getRunningDownloadsInfo()).toJson() };
        return true;
    }
    default: {
        response = {ClientCommon::Error, {}};
        return true;
    }
    }
}

quint32 Server::startDownload(const NetCommon::DownloadRequest &request)
{
    const quint32 id = m_nextDownloadId++;

    qDebug() << "Starting download" << id << ":" << request.tp << request.url << request.albumName << request.artistId << request.artistName;

    QProcess *proc = new QProcess(this);
    proc->setProgram(m_program);
    proc->setArguments({ "--download", request.toBase64() });
    proc->setProcessEnvironment(QProcessEnvironment::systemEnvironment());
    proc->start();
    m_downloads[id] = RunningDownload { request, proc };

    const auto handler = [=](int exitCode, QProcess::ExitStatus status) {
        if (exitCode != 0 || status == QProcess::CrashExit) {
            qWarning() << "Download" << id << "Failed:";
            qWarning().noquote() << proc->readAllStandardError();
        }
        else {
            qDebug() << "Finished download" << id;
        }
        m_downloads.remove(id);
        proc->deleteLater();
    };

    connect(proc, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished), this, handler);

    return id;
}

QJsonArray Server::getRunningDownloadsInfo() const
{
    QJsonArray ret;
    for (auto it = m_downloads.begin(); it != m_downloads.end(); ++it) {
        QJsonObject obj;
        obj["id"] = QJsonValue((int) it.key());
        obj["request"] = QJsonValue(QString(it.value().request.toBase64()));
        ret.append(obj);
    }
    return ret;
}
