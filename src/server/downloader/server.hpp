#pragma once

#include <QProcess>

#include "tcpserver.hpp"
#include "messages.hpp"
#include "download.hpp"

class Server : public NetCommon::TcpServer
{
    Q_OBJECT

public:
    Server(const QString &argv0, const QString &media, const QString &tool, const QString temp, quint16 port);
    ~Server();

protected:
    bool handleMessage(const ClientCommon::Message &message, ClientCommon::Message &response) override;

private:
    quint32 startDownload(const NetCommon::DownloadRequest &request);

private:
    const QString m_program;
    const QString m_media;
    const QString m_tool;
    const QString m_temp;

    QHash<quint32, QProcess*> m_downloads;
    quint32 m_nextDownloadId = 1;
};
