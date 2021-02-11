#include "tcpclientserver.hpp"

#include <QTcpSocket>
#include <QJsonDocument>
#include <QDataStream>
#include <QDate>
#include <QElapsedTimer>
#include <QProcess>

static bool sendData(QTcpSocket &socket, const QByteArray &data)
{
    QDataStream out(&socket);
    out << (qint32) data.size();

    int written = 0;
    while (written < data.size()) {
        int sent = out.writeRawData(data.constData() + written, data.size() - written);
        if (sent < 0)
            return false;
        written += sent;
    }

    return true;
}

TcpServer::TcpServer(qint32 maxMessageSize)
    : QObject()
    , m_maxMessageSize(maxMessageSize)
{
    QObject::connect(&m_tcpServer, &QTcpServer::newConnection, this, &TcpServer::onNewConnection);
}

TcpServer::~TcpServer()
{
}

bool TcpServer::listen(quint16 port)
{
    if (!m_tcpServer.listen(QHostAddress::Any, port)) {
        qWarning() << "TcpServer: Failed to listen on port" << port;
        return false;
    }
    qDebug() << "TcpServer: Listening on port" << port;
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
    auto msg = m_incomingMessages.find(socket);
    const bool hasMessage = (msg != m_incomingMessages.end());

    // first receive message header
    if (!hasMessage) {
        // return early if not enough data is available
        if (!socket->isValid() || !socket->isReadable()) {
            qWarning() << "TcpServer: socket not valid while preparing to read";
            return;
        }

        if (socket->bytesAvailable() < 4)
            return;

        qint32 size;
        QDataStream in(socket);
        in >> size;

        // honor maximum message size
        if (size > m_maxMessageSize) {
            qWarning() << "TcpServer: message size exceeds maximum:" << size;
            socket->close();
            socket->deleteLater();
            return;
        }

        msg = m_incomingMessages.insert(socket, IncomingMessage{ size, 0, QByteArray(size, 0) });
    }

    // download data
    const qint32 bytesLeft = msg->size - msg->alreadyAvailable;
    const qint32 bytesRead = socket->read(msg->data.data() + msg->alreadyAvailable, bytesLeft);
    msg->alreadyAvailable += bytesRead;

    if (msg->alreadyAvailable < msg->size)
        return;

    Q_ASSERT(msg->alreadyAvailable == msg->size);

    const QByteArray response = handleMessage(msg->data);
    Q_ASSERT(response.size() <= m_maxMessageSize);
    sendData(*socket, response);

    // clean up
    m_incomingMessages.erase(msg);
}

Result<QByteArray, QString> TcpClient::sendMessage(QTcpSocket &socket, const QByteArray &data, int timeout)
{
    if (!socket.waitForConnected(timeout)) {
        return QString("TcpClient: Unable to connect to ") + socket.peerName() + ", port=" + QString::number(socket.peerPort());
    }

    // build message
    if (!sendData(socket, data))
        return QString("Error while sending message");

    QElapsedTimer timer;
    timer.start();

    socket.waitForReadyRead(timeout);
    if (socket.bytesAvailable() < 4)
        return QString("Timeout while waiting for message header");

    qint32 sz;
    QDataStream in(&socket);
    in >> sz;

    while (socket.bytesAvailable() < sz) {
        const int msLeft = timeout - timer.elapsed();
        if (msLeft <= 0)
            return QString("Timeout while waiting for message");
        socket.waitForReadyRead(msLeft);
    }

    const QByteArray bytes = socket.read(sz);
    if (bytes.size() != sz)
        return QString("Message size doesn't match");

    return bytes;
}
