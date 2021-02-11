#pragma once

#include <QTcpServer>

#include "result.hpp"

class TcpServer : public QObject
{
    Q_OBJECT

public:
    TcpServer(qint32 maxMessageSize = 64 * 1024 * 1024);
    ~TcpServer() override;

    bool listen(quint16 port);

protected:
    /**
     * The data returned will be sent back to the client
     */
    virtual QByteArray handleMessage(const QByteArray &data) = 0;

private slots:
    void onNewConnection();

private:
    void handleConnection(QTcpSocket *socket);
    void onNewDataReady(QTcpSocket *socket);

    struct IncomingMessage
    {
        qint32 size;
        qint32 alreadyAvailable;
        QByteArray data;
    };

    QTcpServer m_tcpServer;
    qint32 m_maxMessageSize;
    QHash<QTcpSocket*, IncomingMessage> m_incomingMessages;
};

class TcpClient
{
public:
    /**
     * Sends the given message and waits for a response for a maximum of timeout seconds.
     */
    static Result<QByteArray, QString> sendMessage(QTcpSocket &socket, const QByteArray &data, int timeout);
};
