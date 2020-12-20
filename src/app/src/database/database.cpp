#include "database.hpp"
#include "database_items.hpp"
#include "database_interface.hpp"
#include "jsonconv.hpp"
#include "../httpclient.hpp"

#include <QTimer>
#include <QJsonDocument>
#include <QDebug>

using Moosick::LibraryChange;
using Moosick::CommittedLibraryChange;

namespace Database {

Database::Database(HttpClient *httpClient, QObject *parent)
    : QObject(parent)
    , m_http(new HttpRequester(httpClient, this))
{
    connect(m_http, &HttpRequester::receivedReply, this, &Database::onNetworkReplyFinished);
    connect(m_http, &HttpRequester::networkError, this, &Database::onNetworkError);

    connect(&m_downloadQueryTimer, &QTimer::timeout, this, &Database::onDownloadQueryTimer);
    m_downloadQueryTimer.setSingleShot(true);
    m_downloadQueryTimer.start(5000);
}

Database::~Database()
{
}

void Database::setLibrary(const Moosick::Library &library)
{
    m_library = library;
    m_hasLibrary = true;
    emit libraryChanged();
}

bool Database::isSyncing() const
{
    return hasRunningRequestType(LibraryGet) || hasRunningRequestType(LibraryId) || hasRunningRequestType(LibraryUpdate);
}

HttpRequestId Database::sync()
{
    if (isSyncing())
        return 0;

    HttpRequestId reply;

    if (!m_hasLibrary) {
        reply = m_http->requestFromServer("/music/lib.do", "");
        m_requests[reply] = LibraryGet;
    }
    // if we don't yet know the remote library ID, make sure to get that one first
    else if (!m_hasRemoteLibraryId) {
        reply = m_http->requestFromServer("/music/id.do", "");
        m_requests[reply] = LibraryId;
    }
    // do a partial update if we already have a library
    else {
        const int lastRev = m_library.revision();
        reply = m_http->requestFromServer("/music/get-change-list.do", QString::asprintf("v=%d", lastRev));
        m_requests[reply] = LibraryUpdate;
    }

    emit isSyncingChanged();
    return reply;
}

void Database::onNewLibrary(const QJsonObject &json)
{
    if (!m_library.deserializeFromJson(json)) {
        emit libraryError("Failed to parse Library JSON");
        return;
    }

    m_hasLibrary = true;
    m_hasRemoteLibraryId = true;
    m_remoteId = m_library.id();
    emit libraryChanged();
    emit newLibrary();
}

void Database::onNewRemoteId(const QByteArray &data)
{
    if (m_remoteId.fromString(data.trimmed())) {
        m_hasRemoteLibraryId = true;

        // wrong library ID? -> request fresh one
        if (m_hasLibrary && m_library.id() != m_remoteId) {
            m_hasLibrary = false;
            m_library = Moosick::Library();
        }

        sync(); // continue syncing, either full or partial download
    } else {
        emit libraryError("Failed to parse library ID");
    }
}

HttpRequestId Database::download(NetCommon::DownloadRequest request, const Moosick::TagIdList &albumTags)
{
    HttpRequestId reply = 0;

    request.currentRevision = m_library.revision();

    switch (request.tp) {
    case NetCommon::DownloadRequest::BandcampAlbum: {
        const QString query = QString("v=") + request.toBase64();
        reply = m_http->requestFromServer("/music/download.do", query);
        m_requests[reply] = BandcampDownload;
        break;
    }
    case NetCommon::DownloadRequest::YoutubeVideo: {
        const QString query = QString("v=") + request.toBase64();
        reply = m_http->requestFromServer("/music/download.do", query);
        m_requests[reply] = YoutubeDownload;
        break;
    }
    default:
        qFatal("no such download");
        return 0;
    }

    m_runningDownloads << Download { request, albumTags, reply };

    m_downloadsPending = true;
    emit downloadsPendingChanged(m_downloadsPending);

    return reply;
}

HttpRequestId Database::setArtistDetails(Moosick::ArtistId id, const QString &name, const Moosick::TagIdList &tags)
{
    return setItemDetails(
        id, id.name(m_library), id.tags(m_library), name, tags,
        LibraryChange::ArtistSetName, LibraryChange::ArtistAddTag, LibraryChange::ArtistRemoveTag
    );
}

HttpRequestId Database::setAlbumDetails(Moosick::AlbumId id, const QString &name, const Moosick::TagIdList &tags)
{
    return setItemDetails(
        id, id.name(m_library), id.tags(m_library), name, tags,
        LibraryChange::AlbumSetName, LibraryChange::AlbumAddTag, LibraryChange::AlbumRemoveTag
    );
}

HttpRequestId Database::setSongDetails(Moosick::SongId id, const QString &name, const Moosick::TagIdList &tags)
{
    return setItemDetails(
        id, id.name(m_library), id.tags(m_library), name, tags,
        LibraryChange::SongSetName, LibraryChange::SongAddTag, LibraryChange::SongRemoveTag
    );
}

HttpRequestId Database::addTag(const QString &name, Moosick::TagId parent)
{
    return sendChangeRequests({ LibraryChange{ LibraryChange::TagAdd, parent, 0, name } });
}

HttpRequestId Database::removeTag(Moosick::TagId id, bool force)
{
    Q_ASSERT(id.children(m_library).size() == 0);

    QVector<LibraryChange> changes;
    if (force) {
        for (Moosick::SongId song : id.songs(m_library))
            changes << LibraryChange{ LibraryChange::SongRemoveTag, song, id, "" };
        for (Moosick::AlbumId album : id.albums(m_library))
            changes << LibraryChange{ LibraryChange::AlbumRemoveTag, album, id, "" };
        for (Moosick::ArtistId artist : id.artists(m_library))
            changes << LibraryChange{ LibraryChange::ArtistRemoveTag, artist, id, "" };
    }
    changes << LibraryChange{ LibraryChange::TagRemove, id, 0, "" };

    return sendChangeRequests(changes);
}

HttpRequestId Database::setTagDetails(Moosick::TagId id, const QString &name, Moosick::TagId parent)
{
    QVector<LibraryChange> changes;
    if (id.name(m_library) != name)
        changes << LibraryChange { LibraryChange::TagSetName, id, 0, name };
    if (id.parent(m_library) != parent)
        changes << LibraryChange { LibraryChange::TagSetParent, id, parent, "" };
    return sendChangeRequests(changes);
}

HttpRequestId Database::setAlbumArtist(Moosick::AlbumId album, Moosick::ArtistId artist)
{
    return sendChangeRequests({ LibraryChange { LibraryChange::AlbumSetArtist, album, artist, "" } });
}

void Database::addRemoveArtist(QVector<LibraryChange> &changes, Moosick::ArtistId id)
{
    for (Moosick::AlbumId albumId : id.albums(m_library))
        addRemoveAlbum(changes, albumId);
    for (Moosick::TagId tagId : id.tags(m_library))
        changes << LibraryChange{ LibraryChange::ArtistRemoveTag, id, tagId, QString() };
    changes << LibraryChange{ LibraryChange::ArtistRemove, id, 0, QString() };
}

void Database::addRemoveAlbum(QVector<LibraryChange> &changes, Moosick::AlbumId id)
{
    for (Moosick::SongId songId : id.songs(m_library))
        addRemoveSong(changes, songId);
    for (Moosick::TagId tagId : id.tags(m_library))
        changes << LibraryChange{ LibraryChange::AlbumRemoveTag, id, tagId, QString() };
    changes << LibraryChange{ LibraryChange::AlbumRemove, id, 0, QString() };
}

void Database::addRemoveSong(QVector<LibraryChange> &changes, Moosick::SongId id)
{
    for (Moosick::TagId tagId : id.tags(m_library))
        changes << LibraryChange{ LibraryChange::SongRemoveTag, id, tagId, QString() };
    changes << LibraryChange{ LibraryChange::SongRemove, id, 0, QString() };
}

HttpRequestId Database::removeArtist(Moosick::ArtistId artistId)
{
    QVector<LibraryChange> changes;
    addRemoveArtist(changes, artistId);
    return sendChangeRequests(changes);
}

HttpRequestId Database::removeAlbum(Moosick::AlbumId albumId)
{
    QVector<LibraryChange> changes;
    addRemoveAlbum(changes, albumId);
    return sendChangeRequests(changes);
}

HttpRequestId Database::removeSong(Moosick::SongId songId)
{
    QVector<LibraryChange> changes;
    addRemoveSong(changes, songId);
    return sendChangeRequests(changes);
}

HttpRequestId Database::sendChangeRequests(const QVector<LibraryChange> &changes)
{
    if (changes.isEmpty())
        return 0;

    QByteArray data;
    QDataStream writer(&data, QIODevice::WriteOnly);
    writer << changes;

    const QString query = QString("v=") + data.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    HttpRequestId reply = m_http->requestFromServer("/music/request-changes.do", query);
    m_requests[reply] = LibraryChanges;

    m_changesPending = true;
    emit changesPendingChanged(m_changesPending);

    return reply;
}

HttpRequestId Database::setItemDetails(
        quint32 id,
        const QString &oldName, const Moosick::TagIdList &oldTags,
        const QString &newName, const Moosick::TagIdList &newTags,
        LibraryChange::Type setName, LibraryChange::Type addTag, LibraryChange::Type removeTag)
{
    QVector<LibraryChange> changes;

    if (oldName != newName)
        changes << LibraryChange{ setName, id, 0, newName };

    for (Moosick::TagId oldTag : oldTags) {
        if (!newTags.contains(oldTag))
            changes << LibraryChange{ removeTag, id, oldTag, QString() };
    }

    for (Moosick::TagId newTag : newTags) {
        if (!oldTags.contains(newTag))
            changes << LibraryChange{ addTag, id, newTag, QString() };
    }

    return sendChangeRequests(changes);
}

void Database::onNetworkReplyFinished(HttpRequestId reply, const QByteArray &data)
{
    const RequestType requestType = m_requests.take(reply);

    switch (requestType) {
    case LibraryGet: {
        onNewLibrary(parseJsonObject(data, "Library"));
        break;
    }
    case LibraryId: {
        onNewRemoteId(data);
        break;
    }
    case LibraryUpdate: {
        applyLibraryChanges(data);
        break;
    }
    case BandcampDownload:
    case YoutubeDownload: {
        const auto downloadIt = std::find_if(m_runningDownloads.begin(), m_runningDownloads.end(), [&](const Download &download) {
            return download.networkReply == reply;
        });

        if (downloadIt != m_runningDownloads.end()) {
            downloadIt->id = data.toInt();
            downloadIt->networkReply = 0;
        }

        if (!m_downloadQueryTimer.isActive() && !m_downloadQuery)
            m_downloadQueryTimer.start(10000);

        break;
    }
    case DownloadQuery: {
        onDownloadQueryResult(data);
        break;
    }
    case LibraryChanges: {
        applyLibraryChanges(data);
        break;
    }
    default:
        break;
    }

    m_changesPending = hasRunningRequestType(LibraryChanges);
    emit changesPendingChanged(m_changesPending);
    emit isSyncingChanged();
}

void Database::onNetworkError(HttpRequestId requestId, QNetworkReply::NetworkError error)
{
    const RequestType requestType = m_requests.take(requestId);
    qWarning() << "Network error for" << requestType << ": " << error;
}

void Database::onDownloadQueryTimer()
{
    Q_ASSERT(!m_downloadQuery);

    // start a new query for the currently running downloads at the server
    m_downloadQuery = m_http->requestFromServer("/music/running-downloads.do", "");
    m_requests[m_downloadQuery] = DownloadQuery;
}

void Database::onDownloadQueryResult(const QByteArray &data)
{
    Q_ASSERT(m_downloadQuery);
    m_downloadQuery = 0;

    const QJsonArray jsonArray = QJsonDocument::fromJson(data).array();

    // get download IDs from JSON data
    QVector<int> runningDownloadIds;
    for (const QJsonValue &entry : jsonArray) {
        const QJsonObject obj = entry.toObject();
        runningDownloadIds << obj.value("id").toInt(-1);
        // TODO: populate some QML-exportable list of current downloads, to be viewed after program start
    }

    // check if any of our running downloads has finished running on the server
    bool needsSync = false;
    for (auto it = m_runningDownloads.begin(); it != m_runningDownloads.end(); /* empty */) {
        if (!it->networkReply && !runningDownloadIds.contains(it->id)) {
            // download has finished
            // TODO: add downloadIt->artistTags to downloadIt->query.artistId
            needsSync = true;
            it = m_runningDownloads.erase(it);
        } else {
            ++it;
        }
    }

    // get library changes for finished downloads
    if (needsSync)
        sync();

    m_downloadsPending = !jsonArray.isEmpty();
    emit downloadsPendingChanged(m_downloadsPending);

    // if there are downloads still running, we'll need to check back later for the results
    if (m_downloadsPending)
        m_downloadQueryTimer.start(5000);
}

bool Database::applyLibraryChanges(const QByteArray &changesJsonData)
{
    const QJsonArray changesJson = parseJsonArray(changesJsonData, "Library Changes");
    QVector<Moosick::CommittedLibraryChange> changes;
    if (!fromJson(changesJson, changes)) {
        qWarning() << "Unable to parse server library changes from JSON:";
        qWarning().noquote() << changesJsonData;
        return false;
    }

    bool hasChanged = false;

    // add into existing committed changes and apply as many as possible, and delete duplicates
    m_waitingChanges << changes;
    std::sort(m_waitingChanges.begin(), m_waitingChanges.end(), [](const Moosick::CommittedLibraryChange &a, const Moosick::CommittedLibraryChange &b) {
        return a.revision <= b.revision;
    });
    for (auto it = m_waitingChanges.begin(); it != m_waitingChanges.end(); /*empty*/) {
        if (it->revision <= m_library.revision()) {
            it = m_waitingChanges.erase(it);
        }
        else if (it->revision == m_library.revision() + 1) {
            m_library.commit(it->change);
            hasChanged = true;
            ++it;
        } else {
            qWarning() << "Database invalid, need to do a full sync";
            m_hasLibrary = false;
            m_library = Moosick::Library();
            sync();
            break;
        }
    }

    if (hasChanged)
        emit libraryChanged();

    return true;
}

bool Database::hasRunningRequestType(RequestType requestType) const
{
    return std::any_of(m_requests.begin(), m_requests.end(), [=](RequestType tp) {
        return tp == requestType;
    });
}

} // namespace Database
