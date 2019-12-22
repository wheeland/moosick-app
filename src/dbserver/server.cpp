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
    // parse message
    NetCommon::Message message;
    if (!NetCommon::receive(socket, message, 1000)) {
        qWarning() << "Didn't receive message after 1000 msecs of waiting";
        socket->close();
        socket->deleteLater();
        return;
    }

    switch (message.tp) {
    case NetCommon::Ping: {
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
                if (it->isCreatingNewId())
                    it->detail = newId;

                ++it;
            } else {
                it = changes.erase(it);
            }
        }

        // send back all successful changes
        NetCommon::Message response;
        response.tp = NetCommon::ChangesResponse;
        QDataStream out(&response.data, QIODevice::WriteOnly);
        out << changes;

        NetCommon::send(socket, response);
        socket->deleteLater();

        // append library log
        QFile logFile(m_logPath);
        if (logFile.open(QIODevice::Append)) {
            QDataStream out(&logFile);
            for (const auto &ch : qAsConst(changes))
                out << ch;
        }

        // save log file
        QFile libFile(m_libraryPath);
        if (libFile.open(QIODevice::WriteOnly)) {
            QDataStream out(&libFile);
            out << m_library;
        }
    }
    default: {
        NetCommon::send(socket, {NetCommon::Error, {}});
        break;
    }
    }

}
