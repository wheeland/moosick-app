#pragma once

#include <QThread>

#include "tcpclientserver.hpp"
#include "library_messages.hpp"
#include "download.hpp"
#include "serversettings.hpp"

class Server;

class DownloaderThread : public QThread
{
    Q_OBJECT

public:
    DownloaderThread(const MoosickMessage::DownloadRequest &request, Server *server);
    ~DownloaderThread();

    void run() override;

signals:
    void done(DownloaderThread *thread);

private:
    MoosickMessage::DownloadRequest m_request;
    Server *m_server;
};

class Server : public TcpServer
{
    Q_OBJECT

public:
    Server(const ServerSettings &settings);
    ~Server();

protected:
    QByteArray handleMessage(const QByteArray &data) override;

private:
    quint32 startDownload(const MoosickMessage::DownloadRequest &request);

private slots:
    void onDownloadFinished(DownloaderThread *thread);

private:
    const ServerSettings m_settings;

    struct RunningDownload {
        MoosickMessage::DownloadRequest request;
        DownloaderThread *thread;
    };

    QHash<quint32, RunningDownload> m_downloads;
    quint32 m_nextDownloadId = 1;

    friend class DownloaderThread;
};
