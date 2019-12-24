#include "server.hpp"
#include "messages.hpp"

#include <QFile>
#include <QTcpSocket>

Server::Server(const QString &libraryPath, const QString &logPath, const QString &dataPath)
    : QObject ()
    , m_libraryPath(libraryPath)
    , m_logPath(logPath)
    , m_dataPath(dataPath)
{
    // maybe load library file
    QFile libraryFile(libraryPath);
    if (libraryFile.open(QIODevice::ReadOnly)) {
        QDataStream stream(&libraryFile);
        stream >> m_library;
        qDebug() << "Parsed library from" << libraryPath;
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
        NetCommon::MessageHeader header;

        // wait until we can receive the header
        if (!NetCommon::receiveHeader(socket, header))
            return;

        headerIt = m_connections.insert(socket, header);
    }

    // see if we can download the data already
    if (socket->bytesAvailable() < headerIt->dataSize)
        return;

    // download data
    const QByteArray data = socket->read(headerIt->dataSize);

    // parse message
    NetCommon::Message message{ headerIt->tp, data };

    qDebug() << "Received" << message.format() << "from" << socket->peerAddress();

    switch (message.tp) {
    case NetCommon::Ping: {
        qDebug() << "Sending Pong to" << socket->peerAddress();
        NetCommon::send(socket, {NetCommon::Pong, {}});
        break;
    }
    case NetCommon::ChangesRequest: {
        // read changes from TCP stream
        QDataStream in(message.data);
        QVector<Moosick::LibraryChange> changes;
        in >> changes;

        // apply changes
        for (auto it = changes.begin(); it != changes.end(); /* empty */) {
            quint32 newId;
            if (m_library.commit(*it, &newId)) {
                // if this was an Add change, we'll want to communicate the newly created
                // ID back to the caller
                if (Moosick::LibraryChange::isCreatingNewId(it->changeType))
                    it->detail = newId;

                ++it;
            } else {
                it = changes.erase(it);
            }
        }

        qDebug() << "Applied" << changes.size() << "changes to Library. Sending ChangesResponse to" << socket->peerAddress();

        // send back all successful changes
        NetCommon::Message response;
        response.tp = NetCommon::ChangesResponse;
        QDataStream out(&response.data, QIODevice::WriteOnly);
        out << changes;

        NetCommon::send(socket, response);

        // append library log
        QFile logFile(m_logPath);
        if (logFile.open(QIODevice::Append)) {
            QDataStream out(&logFile);
            for (const auto &ch : qAsConst(changes))
                out << ch;
        }

        saveLibrary();

        break;
    }
    case NetCommon::LibraryRequest: {
        // send back whole library
        NetCommon::Message response;
        response.tp = NetCommon::LibraryReponse;
        QDataStream out(&response.data, QIODevice::WriteOnly);
        out << m_library;

        qDebug() << "Sending LibraryResponse to" << socket->peerAddress();

        NetCommon::send(socket, response);
        break;
    }
    default: {
        qDebug() << "Sending Error to" << socket->peerAddress();
        NetCommon::send(socket, {NetCommon::Error, {}});
        break;
    }
    }

    // clean up
    m_connections.erase(headerIt);
    socket->deleteLater();
}

void Server::saveLibrary() const
{
    QFile libFile(m_libraryPath);
    if (libFile.open(QIODevice::WriteOnly)) {
        QDataStream out(&libFile);
        out << m_library;
    }
}
