#pragma once

#include <QTcpServer>

#include "library.hpp"

class Server : public QObject
{
public:
    Server(const QString &libraryPath, const QString &logPath, const QString &dataPath);
    ~Server();

    bool listen(quint16 port);

private slots:
    void onNewConnection();

private:
    void handleConnection(QTcpSocket *socket);

    QTcpServer m_tcpServer;
    const QString m_libraryPath;
    const QString m_logPath;
    const QString m_dataPath;

    Moosick::Library m_library;
};
