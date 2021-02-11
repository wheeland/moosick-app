#pragma once

#include <QPointer>
#include <QTimer>

#include "option.hpp"
#include "library.hpp"
#include "library_messages.hpp"
#include "flatmap.hpp"
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

    void setLibrary(const Moosick::Library &library);

    bool hasLibrary() const { return m_hasLibrary; }
    bool isSyncing() const;
    bool downloadsPending() const { return m_downloadsPending; }
    bool changesPending() const { return hasRunningRequestType(LibraryChanges); }
    const Moosick::Library &library() const { return m_library; }

    HttpRequestId sync();
    HttpRequestId download(MoosickMessage::DownloadRequest request, const Moosick::TagIdList &albumTags);

    HttpRequestId setArtistDetails(Moosick::ArtistId id, const QString &name, const Moosick::TagIdList &tags);
    HttpRequestId setAlbumDetails(Moosick::AlbumId id, const QString &name, const Moosick::TagIdList &tags);
    HttpRequestId setSongDetails(Moosick::SongId id, const QString &name, const Moosick::TagIdList &tags);

    HttpRequestId addTag(const QString &name, Moosick::TagId parent);
    HttpRequestId removeTag(Moosick::TagId id, bool force);
    HttpRequestId setTagDetails(Moosick::TagId id, const QString &name, Moosick::TagId parent);

    HttpRequestId setAlbumArtist(Moosick::AlbumId album, Moosick::ArtistId artist);

    HttpRequestId removeArtist(Moosick::ArtistId artistId);
    HttpRequestId removeAlbum(Moosick::AlbumId albumId);
    HttpRequestId removeSong(Moosick::SongId songId);

private slots:
    void onNetworkReplyFinished(HttpRequestId reply, const QByteArray &data);
    void onNetworkError(HttpRequestId requestId, QNetworkReply::NetworkError error);
    void onDownloadQueryTimer();

signals:
    void libraryChanged();
    void newLibrary();
    void downloadsPendingChanged(bool downloadsPending);
    void changesPendingChanged(bool changesPending);
    void isSyncingChanged();

private:
    // Methods to react on server response messages
    Option<QString> onNewLibrary(const MoosickMessage::LibraryResponse *message);
    Option<QString> onNewRemoteId(const MoosickMessage::IdResponse *message);
    Option<QString> applyLibraryChanges(const QVector<Moosick::CommittedLibraryChange> &changes);
    Option<QString> onDownloadResponse(HttpRequestId reply, const MoosickMessage::DownloadResponse *message);
    Option<QString> onDownloadQueryResponse(const MoosickMessage::DownloadQueryResponse *message);

    HttpRequestId sendChangeRequests(const QVector<Moosick::LibraryChangeRequest> &changes);

    HttpRequestId setItemDetails(quint32 id,
                                 const QString &oldName, const Moosick::TagIdList &oldTags,
                                 const QString &newName, const Moosick::TagIdList &newTags,
                                 Moosick::LibraryChangeRequest::Type setName,
                                 Moosick::LibraryChangeRequest::Type addTag,
                                 Moosick::LibraryChangeRequest::Type removeTag);

    void addRemoveArtist(QVector<Moosick::LibraryChangeRequest> &changes, Moosick::ArtistId id);
    void addRemoveAlbum(QVector<Moosick::LibraryChangeRequest> &changes, Moosick::AlbumId id);
    void addRemoveSong(QVector<Moosick::LibraryChangeRequest> &changes, Moosick::SongId id);

    enum RequestType {
        None,
        LibraryId,
        LibraryGet,
        LibraryPartialSync,
        LibraryChanges,
        BandcampDownload,
        YoutubeDownload,
        DownloadQuery,
    };

    Option<QString> processServerResponse(HttpRequestId reply, RequestType requestType, const MoosickMessage::Message &msg);

    bool m_hasRemoteLibraryId = false;
    bool m_hasLibrary = false;
    bool m_downloadsPending = false;
    bool m_changesPending = false;
    Moosick::Library m_library;
    Moosick::LibraryId m_remoteId;

    HttpRequester *m_http;
    FlatMap<HttpRequestId, RequestType> m_requests;

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
        MoosickMessage::DownloadRequest request;
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
