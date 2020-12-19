#pragma once

#include <QTcpServer>

#include "messages.hpp"

namespace NetCommon {

class TcpServer : public QObject
{
    Q_OBJECT

public:
    TcpServer();
    ~TcpServer() override;

    bool listen(quint16 port);

protected:
    virtual bool handleMessage(const ClientCommon::Message &message, ClientCommon::Message &response) = 0;

private slots:
    void onNewConnection();

private:
    void handleConnection(QTcpSocket *socket);
    void onNewDataReady(QTcpSocket *socket);

    QTcpServer m_tcpServer;
    QHash<QTcpSocket *, ClientCommon::MessageHeader> m_connections;
};

} // namespace NetCommon
