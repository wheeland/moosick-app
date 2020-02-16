#include "tcpserver.hpp"

#include <QTcpSocket>
#include <QJsonDocument>
#include <QDate>
#include <QProcess>

namespace NetCommon {

TcpServer::TcpServer()
    : QObject ()
{
    QObject::connect(&m_tcpServer, &QTcpServer::newConnection, this, &TcpServer::onNewConnection);
}

TcpServer::~TcpServer()
{
}

bool TcpServer::listen(quint16 port)
{
    if (!m_tcpServer.listen(QHostAddress::Any, port)) {
        qWarning() << "Failed to listen on port" << port;
        return false;
    }
    qDebug() << "Listening on port" << port;
    return true;
}

void TcpServer::onNewConnection()
{
    while (true) {
        QTcpSocket *conn = m_tcpServer.nextPendingConnection();
        if (!conn)
            return;

        handleConnection(conn);
    }
}

void TcpServer::handleConnection(QTcpSocket *socket)
{
    connect(socket, &QTcpSocket::readyRead, this, [=]() {
        TcpServer::onNewDataReady(socket);
    });

    if (socket->bytesAvailable())
        onNewDataReady(socket);
}

void TcpServer::onNewDataReady(QTcpSocket *socket)
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
    ClientCommon::Message answer;
    ClientCommon::Message message{ headerIt->tp, data };

    qDebug() << "Received" << message.format() << "from" << socket->peerAddress();

    const bool doAnswer = handleMessage(message, answer);
    if (doAnswer)
        ClientCommon::send(socket, answer);

    // clean up
    m_connections.erase(headerIt);
    socket->deleteLater();
}

} // namespace NetCommon
