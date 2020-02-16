#pragma once

#include <QTcpServer>

#include "library.hpp"
#include "messages.hpp"

class Server : public QObject
{
public:
    Server(const QString &libraryPath,
           const QString &logPath,
           const QString &dataPath,
           const QString &m_backupBasePath);
    ~Server();

    bool listen(quint16 port);

private slots:
    void onNewConnection();

private:
    void handleConnection(QTcpSocket *socket);
    void onNewDataReady(QTcpSocket *socket);
    void saveLibrary() const;

    QTcpServer m_tcpServer;
    const QString m_libraryPath;
    const QString m_logPath;
    const QString m_dataPath;
    const QString m_backupBasePath;

    Moosick::Library m_library;

    QHash<QTcpSocket *, ClientCommon::MessageHeader> m_connections;
};
