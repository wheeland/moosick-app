#include "server.hpp"
#include "messages.hpp"
#include "jsonconv.hpp"

#include <QFile>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QDate>
#include <QProcess>

Server::Server(const QString &libraryPath,
               const QString &logPath,
               const QString &dataPath,
               const QString &backupBasePath)
    : QObject ()
    , m_libraryPath(libraryPath)
    , m_logPath(logPath)
    , m_dataPath(dataPath)
    , m_backupBasePath(backupBasePath)
{
    // maybe load library file
    QFile libraryFile(libraryPath);
    if (libraryFile.open(QIODevice::ReadOnly)) {
        const QJsonObject jsonRoot = parseJsonObject(libraryFile.readAll(), "Library");
        if (m_library.deserializeFromJson(jsonRoot))
            qDebug() << "Parsed library from" << libraryPath;
        else
            qDebug() << "Failed parsing library from" << libraryPath;
    } else {
        qDebug() << "Could not read file" << libraryPath;
    }

    // check if we can open log file
    if (!QFile(logPath).open(QIODevice::Append)) {
        qWarning() << "Can't write to log file" << logPath;
    }

    QObject::connect(&m_tcpServer, &QTcpServer::newConnection, this, &Server::onNewConnection);
}

Server::~Server()
{
    saveLibrary();
}

bool Server::listen(quint16 port)
{
    if (!m_tcpServer.listen(QHostAddress::Any, port)) {
        qWarning() << "Failed to listen on port" << port;
        return false;
    }
    qDebug() << "Listening on port" << port;
    return true;
}

void Server::onNewConnection()
{
    while (true) {
        QTcpSocket *conn = m_tcpServer.nextPendingConnection();
        if (!conn)
            return;

        handleConnection(conn);
    }
}

void Server::handleConnection(QTcpSocket *socket)
{
    connect(socket, &QTcpSocket::readyRead, this, [=]() {
        Server::onNewDataReady(socket);
    });

    if (socket->bytesAvailable())
        onNewDataReady(socket);
}

void Server::onNewDataReady(QTcpSocket *socket)
{
    auto headerIt = m_connections.find(socket);
    const bool hasHeader = (headerIt != m_connections.end());

    // first receive message header
    if (!hasHeader) {
        ClientCommon::MessageHeader header;

        // wait until we can receive the header
        if (!ClientCommon::receiveHeader(socket, header))
            return;

        headerIt = m_connections.insert(socket, header);
    }

    // see if we can download the data already
    if (socket->bytesAvailable() < headerIt->dataSize)
        return;

    // download data
    const QByteArray data = socket->read(headerIt->dataSize);

    // parse message
    ClientCommon::Message message{ headerIt->tp, data };

    qDebug() << "Received" << message.format() << "from" << socket->peerAddress();

    switch (message.tp) {
    case ClientCommon::Ping: {
        qDebug() << "Sending Pong to" << socket->peerAddress();
        ClientCommon::send(socket, {ClientCommon::Pong, {}});
        break;
    }
    case ClientCommon::ChangesRequest: {
        // read changes from TCP stream
        QDataStream in(message.data);
        QVector<Moosick::LibraryChange> changes;
        in >> changes;

        QVector<Moosick::CommittedLibraryChange> appliedChanges;

        // apply changes
        for (const Moosick::LibraryChange &change : qAsConst(changes)) {
            quint32 newId, result = m_library.commit(change, &newId);
            Moosick::LibraryChange resultChange = change;

            if (result != 0) {
                // if this was an Add change, we'll want to communicate the newly created
                // ID back to the caller
                if (Moosick::LibraryChange::isCreatingNewId(change.changeType))
                    resultChange.detail = newId;

                appliedChanges << Moosick::CommittedLibraryChange{resultChange, result};
            }
        }

        qDebug() << "Applied" << appliedChanges.size() << "changes to Library. Sending ChangesResponse to" << socket->peerAddress();

        // send back all successful changes
        ClientCommon::Message response;
        response.tp = ClientCommon::ChangesResponse;
        response.data = QJsonDocument(toJson(appliedChanges).toArray()).toJson();

        ClientCommon::send(socket, response);

        // append library log
        QFile logFile(m_logPath);
        const qint64 sz = logFile.exists() ? logFile.size() : 0;
        if (logFile.open(QIODevice::Append)) {
            bool first = (sz <= 0);
            for (const auto &ch : qAsConst(appliedChanges)) {
                if (!first)
                    logFile.write(",\n");
                first = false;

                logFile.write(QJsonDocument(toJson(ch).toObject()).toJson().trimmed());
            }
        }

        saveLibrary();

        break;
    }
    case ClientCommon::LibraryRequest: {
        // send back whole library
        ClientCommon::Message response;
        response.tp = ClientCommon::LibraryReponse;
        response.data = QJsonDocument(m_library.serializeToJson()).toJson();

        qDebug() << "Sending LibraryResponse to" << socket->peerAddress();

        ClientCommon::send(socket, response);
        break;
    }
    default: {
        qDebug() << "Sending Error to" << socket->peerAddress();
        ClientCommon::send(socket, {ClientCommon::Error, {}});
        break;
    }
    }

    // clean up
    m_connections.erase(headerIt);
    socket->deleteLater();
}

void Server::saveLibrary() const
{
    const auto save = [&](const QString &path) {
        QFile libFile(path);
        if (libFile.open(QIODevice::WriteOnly)) {
            const QJsonDocument doc(m_library.serializeToJson());
            libFile.write(doc.toJson());
        }
    };

    save(m_libraryPath);

    // check if back-up for today already exists
    const QDate today = QDate::currentDate();
    const QString dateString = QString::asprintf("%d_%02d_%02d", today.year(), today.month(), today.day());
    const QString backupPath = m_backupBasePath + "." + dateString + ".json";
    if (!QFile::exists(backupPath) && !QFile::exists(backupPath + ".gz")) {
        save(backupPath);
        QProcess::execute("gzip", { backupPath });
    }
}
