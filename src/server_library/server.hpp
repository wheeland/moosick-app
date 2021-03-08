#pragma once

#include <QTcpServer>

#include "tcpclientserver.hpp"
#include "serversettings.hpp"
#include "library.hpp"
#include "library_messages.hpp"
#include "option.hpp"

class DownloaderThread;
class DownloadResult;

class Server : public TcpServer
{
public:
    Server();
    ~Server();

    EnjsonError init(const ServerSettings &settings);

protected:
    QByteArray handleMessage(const QByteArray &data) override;

private:
    quint32 startDownload(const MoosickMessage::DownloadRequest &request);
    void finishDownload(quint32 id, const DownloadResult &result);
    void onDownloaderThreadFinished(DownloaderThread *thread);
    QString createSongHandleFile(Moosick::SongId songId);

private:
    void saveLibrary() const;

    ServerSettings m_settings;

    Moosick::Library m_library;

    struct RunningDownload {
        MoosickMessage::DownloadRequest request;
        DownloaderThread *thread;
    };
    QHash<quint32, RunningDownload> m_downloads;
    quint32 m_nextDownloadId = 1;

    Moosick::ArtistId getOrCreateArtist(const QString &name);
    Moosick::AlbumId getOrCreateAlbum(Moosick::ArtistId artist, const QString &name);

    friend class DownloaderThread;
};
