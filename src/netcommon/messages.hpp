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
};

bool send(QTcpSocket *socket, const Message &msg);
bool receive(QTcpSocket *socket, Message &msg, int timeout);

} //namespace NetCommon
