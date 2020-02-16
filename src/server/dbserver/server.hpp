#pragma once

#include <QTcpServer>

#include "tcpserver.hpp"
#include "library.hpp"
#include "messages.hpp"

class Server : public NetCommon::TcpServer
{
public:
    Server(const QString &libraryPath,
           const QString &logPath,
           const QString &dataPath,
           const QString &m_backupBasePath);
    ~Server();

protected:
    bool handleMessage(const ClientCommon::Message &message, ClientCommon::Message &response) override;

private:
    void saveLibrary() const;

    const QString m_libraryPath;
    const QString m_logPath;
    const QString m_dataPath;
    const QString m_backupBasePath;

    Moosick::Library m_library;
};
