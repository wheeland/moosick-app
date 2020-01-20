#include "messages.hpp"

#include <QElapsedTimer>

namespace ClientCommon {

bool send(QTcpSocket *socket, const Message &msg)
{
    if (!socket->isValid() || !socket->isWritable())
        return false;

    {
        QDataStream out(socket);
        out << (quint32) msg.tp;
        out << (quint32) msg.data.size();
        out.writeRawData(msg.data.data(), msg.data.size());
    }

    socket->flush();

    return socket->isValid();
}

bool receive(QTcpSocket *socket, Message &msg, int timeout)
{
    if (!socket->isValid() || !socket->isReadable())
        return false;

    QElapsedTimer timer;
    timer.start();

    socket->waitForReadyRead(timeout);
    if (socket->bytesAvailable() < 8)
        return false;

    quint32 tp, sz;
    QDataStream in(socket);
    in >> tp;
    in >> sz;

    while (socket->bytesAvailable() < sz) {
        const int msLeft = timeout - timer.elapsed();
        if (msLeft <= 0)
            return false;
        socket->waitForReadyRead(msLeft);
    }

    const QByteArray bytes = socket->read(sz);
    if (bytes.size() != sz)
        return false;

    msg.tp = static_cast<MessageType>(tp);
    msg.data = bytes;
    return true;
}

bool receiveHeader(QTcpSocket *socket, MessageHeader &msg)
{
    if (!socket->isValid() || !socket->isReadable())
        return false;

    if (socket->bytesAvailable() < 8)
        return false;

    quint32 tp, sz;
    QDataStream in(socket);
    in >> tp;
    in >> sz;

    msg.tp = static_cast<MessageType>(tp);
    msg.dataSize = sz;

    return true;
}

QString Message::format()
{
    static const QStringList msgNames = {
        "Error",
        "Ping",
        "Pong",
        "LibraryRequest",
        "LibraryReponse",
        "ChangesRequest",
        "ChangesResponse",
        "ChangeListRequest",
        "ChangeListReponse",
    };

    const quint32 idx = (quint32) tp;
    if (idx >= msgNames.size()) {
        return QString::asprintf("Invalid(%d)", idx);
    }

    return QString::asprintf("%s(%d)", qPrintable(msgNames[idx]), data.size());
}

bool sendRecv(const ServerConfig &server, const Message &message, Message &answer)
{
    // try to connect to TCP server
    QTcpSocket sock;
    sock.connectToHost(server.hostName, server.port);
    if (!sock.waitForConnected(server.timeout)) {
        qWarning() << "Unable to connect to" << server.hostName << ", port =" << server.port;
        return false;
    }

    ClientCommon::send(&sock, message);
    ClientCommon::receive(&sock, answer, server.timeout);

    return true;
}

} //namespace ClientCommon
