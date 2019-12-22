#pragma once

#include "library.hpp"

#include <QTcpSocket>

namespace NetCommon {

enum MessageType : quint32
{
    Error,
    Ping,
    Pong,
    LibraryRequest,
    LibraryReponse,
    ChangesRequest,
    ChangesResponse,
};

struct Message
{
    MessageType tp;
    QByteArray data;

    QString format();
};

struct MessageHeader {
    MessageType tp;
    quint32 dataSize;
};

bool send(QTcpSocket *socket, const Message &msg);
bool receive(QTcpSocket *socket, Message &msg, int timeout);

bool receiveHeader(QTcpSocket *socket, MessageHeader &msg);

} //namespace NetCommon
