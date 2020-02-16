#pragma once

#include "library.hpp"

#include <QTcpSocket>

namespace ClientCommon {

enum MessageType : quint32
{
    Error,
    Ping,
    Pong,

    /** Retrieve current state of the library */
    LibraryRequest,
    LibraryResponse,

    /** Apply a set of changes to the library, return applied changes */
    ChangesRequest,
    ChangesResponse,

    /** Request a list of changes made since revision N */
    ChangeListRequest,
    ChangeListResponse,

    /** Download from YT/BC, return a list of running download IDs */
    DownloadRequest,
    DownloadResponse,
    DownloadQuery,
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

struct ServerConfig
{
    QString hostName;
    quint16 port;
    quint32 timeout;
};

bool sendRecv(const ServerConfig &server, const ClientCommon::Message &message, ClientCommon::Message &answer);

} //namespace ClientCommon
