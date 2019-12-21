#include "server.hpp"

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
    // read changes from TCP stream
    QDataStream stream(socket);
    QVector<Moosick::LibraryChange> changes;
    stream >> changes;

    // apply changes
    for (auto it = changes.begin(); it != changes.end(); /* empty */) {
        if (m_library.commit(*it))
            ++it;
        else {
            it = changes.erase(it);
        }
    }

    // send back all successful changes
    stream << changes;
    socket->close();
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
