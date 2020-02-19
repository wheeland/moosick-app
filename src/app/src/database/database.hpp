#pragma once

#include <QPointer>
#include <QTimer>

#include "library.hpp"
#include "requests.hpp"
#include "../httpclient.hpp"

class HttpClient;
class HttpRequester;

namespace Database {

/**
 * A frontend for the library on the server
 */
class Database : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hasLibrary READ hasLibrary NOTIFY libraryChanged)
    Q_PROPERTY(bool isSyncing READ isSyncing NOTIFY isSyncingChanged)
    Q_PROPERTY(bool downloadsPending READ downloadsPending NOTIFY downloadsPendingChanged)
    Q_PROPERTY(bool changesPending READ changesPending NOTIFY changesPendingChanged)

public:
    Database(HttpClient *httpClient, QObject *parent = nullptr);
    ~Database() override;

    bool hasLibrary() const { return m_hasLibrary; }
    bool isSyncing() const { return hasRunningRequestType(LibraryGet) || hasRunningRequestType(LibraryUpdate); }
    bool downloadsPending() const { return m_downloadsPending; }
    bool changesPending() const { return hasRunningRequestType(LibraryChanges); }
    const Moosick::Library &library() const { return m_library; }

    HttpRequestId sync();
    HttpRequestId download(NetCommon::DownloadRequest request, const Moosick::TagIdList &albumTags);

    HttpRequestId setArtistDetails(Moosick::ArtistId id, const QString &name, const Moosick::TagIdList &tags);
    HttpRequestId setAlbumDetails(Moosick::AlbumId id, const QString &name, const Moosick::TagIdList &tags);
    HttpRequestId setSongDetails(Moosick::SongId id, const QString &name, const Moosick::TagIdList &tags);

    HttpRequestId setTagName(Moosick::TagId id, const QString &name);

    HttpRequestId setAlbumArtist(Moosick::AlbumId album, Moosick::ArtistId artist);

    HttpRequestId removeArtist(Moosick::ArtistId artistId);
    HttpRequestId removeAlbum(Moosick::AlbumId albumId);
    HttpRequestId removeSong(Moosick::SongId songId);

private slots:
    void onNetworkReplyFinished(HttpRequestId reply, const QByteArray &data);
    void onDownloadQueryTimer();

signals:
    void libraryChanged();
    void downloadsPendingChanged(bool downloadsPending);
    void changesPendingChanged(bool changesPending);
    void isSyncingChanged();

private:
    void onNewLibrary(const QJsonObject &json);
    bool applyLibraryChanges(const QByteArray &changesJsonData);

    HttpRequestId sendChangeRequests(const QVector<Moosick::LibraryChange> &changes);

    HttpRequestId setItemDetails(quint32 id,
                                 const QString &oldName, const Moosick::TagIdList &oldTags,
                                 const QString &newName, const Moosick::TagIdList &newTags,
                                 Moosick::LibraryChange::Type setName,
                                 Moosick::LibraryChange::Type addTag,
                                 Moosick::LibraryChange::Type removeTag);

    void addRemoveArtist(QVector<Moosick::LibraryChange> &changes, Moosick::ArtistId id);
    void addRemoveAlbum(QVector<Moosick::LibraryChange> &changes, Moosick::AlbumId id);
    void addRemoveSong(QVector<Moosick::LibraryChange> &changes, Moosick::SongId id);

    void onDownloadQueryResult(const QByteArray &data);

    enum RequestType {
        None,
        LibraryGet,
        LibraryUpdate,
        LibraryChanges,
        BandcampDownload,
        YoutubeDownload,
        DownloadQuery,
    };

    bool m_hasLibrary = false;
    bool m_downloadsPending = false;
    bool m_changesPending = false;
    Moosick::Library m_library;

    HttpRequester *m_http;
    QHash<HttpRequestId, RequestType> m_requests;

    bool hasRunningRequestType(RequestType requestType) const;

    /**
     * If we are not ready to commit() some result transitions yet,
     * we will save them for later application
     */
    QVector<Moosick::CommittedLibraryChange> m_waitingChanges;
    /**
     * TODO:
     * changes that have been requested at the server, but where the response hasn't yet
     * been received, plus the waiting changes that we know have finished, but just didn't get
     * the prior CommitedLibraryChanges, we should all group in a second Library.
     * This m_overlayLibrary will be based on the synced m_library, but with all pending
     * patches applied to it.
     */

    struct Download {
        NetCommon::DownloadRequest request;
        Moosick::TagIdList albumTags;
        HttpRequestId networkReply;
        quint32 id;
    };
    QVector<Download> m_runningDownloads;

    /** regularly query the state of the downloads */
    HttpRequestId m_downloadQuery = 0;
    QTimer m_downloadQueryTimer;
    bool m_isSyncing;
};

} // namespace Database
