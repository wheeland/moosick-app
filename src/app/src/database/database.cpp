#include "database.hpp"
#include "database_items.hpp"
#include "database_interface.hpp"
#include "jsonconv.hpp"
#include "../httpclient.hpp"

#include <QTimer>
#include <QJsonDocument>
#include <QDebug>

using Moosick::LibraryChangeRequest;
using Moosick::CommittedLibraryChange;
using namespace MoosickMessage;

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
    return hasRunningRequestType(LibraryGet) || hasRunningRequestType(LibraryId) || hasRunningRequestType(LibraryPartialSync);
}

HttpRequestId Database::sync()
{
    if (isSyncing())
        return 0;

    HttpRequestId reply;

    if (!m_hasLibrary) {
        reply = m_http->requestFromServer(Message(LibraryRequest()).toJson());
        m_requests[reply] = LibraryGet;
    }
    // if we don't yet know the remote library ID, make sure to get that one first
    else if (!m_hasRemoteLibraryId) {
        reply = m_http->requestFromServer(Message(IdRequest()).toJson());
        m_requests[reply] = LibraryId;
    }
    // do a partial update if we already have a library
    else {
        ChangeListRequest request;
        request.revision = m_library.revision();
        reply = m_http->requestFromServer(Message(request).toJson());
        m_requests[reply] = LibraryPartialSync;
    }

    emit isSyncingChanged();
    return reply;
}

HttpRequestId Database::download(MoosickMessage::DownloadRequest request, const Moosick::TagIdList &albumTags)
{
    HttpRequestId reply = 0;

    request.currentRevision = m_library.revision();

    switch (*request.requestType) {
    case MoosickMessage::DownloadRequestType::BandcampAlbum: {
        reply = m_http->requestFromServer(Message(request).toJson());
        m_requests[reply] = BandcampDownload;
        break;
    }
    case MoosickMessage::DownloadRequestType::YoutubeVideo: {
        reply = m_http->requestFromServer(Message(request).toJson());
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
        LibraryChangeRequest::ArtistSetName, LibraryChangeRequest::ArtistAddTag, LibraryChangeRequest::ArtistRemoveTag
    );
}

HttpRequestId Database::setAlbumDetails(Moosick::AlbumId id, const QString &name, const Moosick::TagIdList &tags)
{
    return setItemDetails(
        id, id.name(m_library), id.tags(m_library), name, tags,
        LibraryChangeRequest::AlbumSetName, LibraryChangeRequest::AlbumAddTag, LibraryChangeRequest::AlbumRemoveTag
    );
}

HttpRequestId Database::setSongDetails(Moosick::SongId id, const QString &name, const Moosick::TagIdList &tags)
{
    return setItemDetails(
        id, id.name(m_library), id.tags(m_library), name, tags,
        LibraryChangeRequest::SongSetName, LibraryChangeRequest::SongAddTag, LibraryChangeRequest::SongRemoveTag
    );
}

HttpRequestId Database::addTag(const QString &name, Moosick::TagId parent)
{
    return sendChangeRequests({ LibraryChangeRequest{ LibraryChangeRequest::TagAdd, parent, 0, name } });
}

HttpRequestId Database::removeTag(Moosick::TagId id, bool force)
{
    Q_ASSERT(id.children(m_library).size() == 0);

    QVector<LibraryChangeRequest> changes;
    if (force) {
        for (Moosick::SongId song : id.songs(m_library))
            changes << LibraryChangeRequest{ LibraryChangeRequest::SongRemoveTag, song, id, "" };
        for (Moosick::AlbumId album : id.albums(m_library))
            changes << LibraryChangeRequest{ LibraryChangeRequest::AlbumRemoveTag, album, id, "" };
        for (Moosick::ArtistId artist : id.artists(m_library))
            changes << LibraryChangeRequest{ LibraryChangeRequest::ArtistRemoveTag, artist, id, "" };
    }
    changes << LibraryChangeRequest{ LibraryChangeRequest::TagRemove, id, 0, "" };

    return sendChangeRequests(changes);
}

HttpRequestId Database::setTagDetails(Moosick::TagId id, const QString &name, Moosick::TagId parent)
{
    QVector<LibraryChangeRequest> changes;
    if (id.name(m_library) != name)
        changes << LibraryChangeRequest { LibraryChangeRequest::TagSetName, id, 0, name };
    if (id.parent(m_library) != parent)
        changes << LibraryChangeRequest { LibraryChangeRequest::TagSetParent, id, parent, "" };
    return sendChangeRequests(changes);
}

HttpRequestId Database::setAlbumArtist(Moosick::AlbumId album, Moosick::ArtistId artist)
{
    return sendChangeRequests({ LibraryChangeRequest { LibraryChangeRequest::AlbumSetArtist, album, artist, "" } });
}

void Database::addRemoveArtist(QVector<LibraryChangeRequest> &changes, Moosick::ArtistId id)
{
    for (Moosick::AlbumId albumId : id.albums(m_library))
        addRemoveAlbum(changes, albumId);
    for (Moosick::TagId tagId : id.tags(m_library))
        changes << LibraryChangeRequest{ LibraryChangeRequest::ArtistRemoveTag, id, tagId, QString() };
    changes << LibraryChangeRequest{ LibraryChangeRequest::ArtistRemove, id, 0, QString() };
}

void Database::addRemoveAlbum(QVector<LibraryChangeRequest> &changes, Moosick::AlbumId id)
{
    for (Moosick::SongId songId : id.songs(m_library))
        addRemoveSong(changes, songId);
    for (Moosick::TagId tagId : id.tags(m_library))
        changes << LibraryChangeRequest{ LibraryChangeRequest::AlbumRemoveTag, id, tagId, QString() };
    changes << LibraryChangeRequest{ LibraryChangeRequest::AlbumRemove, id, 0, QString() };
}

void Database::addRemoveSong(QVector<LibraryChangeRequest> &changes, Moosick::SongId id)
{
    for (Moosick::TagId tagId : id.tags(m_library))
        changes << LibraryChangeRequest{ LibraryChangeRequest::SongRemoveTag, id, tagId, QString() };
    changes << LibraryChangeRequest{ LibraryChangeRequest::SongRemove, id, 0, QString() };
}

HttpRequestId Database::removeArtist(Moosick::ArtistId artistId)
{
    QVector<LibraryChangeRequest> changes;
    addRemoveArtist(changes, artistId);
    return sendChangeRequests(changes);
}

HttpRequestId Database::removeAlbum(Moosick::AlbumId albumId)
{
    QVector<LibraryChangeRequest> changes;
    addRemoveAlbum(changes, albumId);
    return sendChangeRequests(changes);
}

HttpRequestId Database::removeSong(Moosick::SongId songId)
{
    QVector<LibraryChangeRequest> changes;
    addRemoveSong(changes, songId);
    return sendChangeRequests(changes);
}

HttpRequestId Database::sendChangeRequests(const QVector<LibraryChangeRequest> &changes)
{
    if (changes.isEmpty())
        return 0;

    ChangesRequest request;
    request.changes = changes;
    HttpRequestId reply = m_http->requestFromServer(Message(request).toJson());
    m_requests[reply] = LibraryChanges;

    m_changesPending = true;
    emit changesPendingChanged(m_changesPending);

    return reply;
}

HttpRequestId Database::setItemDetails(
        quint32 id,
        const QString &oldName, const Moosick::TagIdList &oldTags,
        const QString &newName, const Moosick::TagIdList &newTags,
        LibraryChangeRequest::Type setName, LibraryChangeRequest::Type addTag, LibraryChangeRequest::Type removeTag)
{
    QVector<LibraryChangeRequest> changes;

    if (oldName != newName)
        changes << LibraryChangeRequest{ setName, id, 0, newName };

    for (Moosick::TagId oldTag : oldTags) {
        if (!newTags.contains(oldTag))
            changes << LibraryChangeRequest{ removeTag, id, oldTag, QString() };
    }

    for (Moosick::TagId newTag : newTags) {
        if (!oldTags.contains(newTag))
            changes << LibraryChangeRequest{ addTag, id, newTag, QString() };
    }

    return sendChangeRequests(changes);
}

void Database::onNetworkReplyFinished(HttpRequestId reply, const QByteArray &data)
{
    const RequestType requestType = m_requests.take(reply);
    Result<Message, JsonifyError> parsedMsg = Message::fromJson(data);
    if (parsedMsg.hasError()) {
        qWarning().noquote() << "Received invalid message:" << parsedMsg.takeError().toString();
        qWarning().noquote() << data;
        return;
    }
    Message message = parsedMsg.takeValue();

    Option<QString> error = processServerResponse(reply, requestType, message);
    if (error)
        qWarning().noquote() << "Failed processing server response:" << error.takeValue();

    m_changesPending = hasRunningRequestType(LibraryChanges);
    emit changesPendingChanged(m_changesPending);
    emit isSyncingChanged();
}

Option<QString> Database::processServerResponse(HttpRequestId reply, Database::RequestType requestType, const Message &msg)
{
    #define EXPECT_MESSAGE_TYPE(TYPE, NAME) \
    const TYPE *NAME = msg.as<TYPE>(); \
    if (!NAME) \
        return QString("Expected " #TYPE " message, instead got ") + msg->getMessageTypeString();

    switch (requestType) {
    case LibraryGet: {
        EXPECT_MESSAGE_TYPE(LibraryResponse, library);
        return onNewLibrary(library);
    }
    case LibraryId: {
        EXPECT_MESSAGE_TYPE(IdResponse, id);
        return onNewRemoteId(id);
    }
    case LibraryPartialSync: {
        EXPECT_MESSAGE_TYPE(ChangeListResponse, changes);
        return applyLibraryChanges(changes->changes);
    }
    case BandcampDownload:
    case YoutubeDownload: {
        EXPECT_MESSAGE_TYPE(DownloadResponse, download);
        return onDownloadResponse(reply, download);
    }
    case DownloadQuery: {
        EXPECT_MESSAGE_TYPE(DownloadQueryResponse, download);
        return onDownloadQueryResponse(download);
        break;
    }
    case LibraryChanges: {
        EXPECT_MESSAGE_TYPE(ChangesResponse, changes);
        return applyLibraryChanges(changes->changes);
    }
    default:
        break;
    }

    return QString("Unhandled message type: ") + msg->getMessageTypeString();

    #undef EXPECT_MESSAGE_TYPE
}

void Database::onNetworkError(HttpRequestId requestId, QNetworkReply::NetworkError error)
{
    const RequestType requestType = m_requests.take(requestId);
    qWarning() << "Network error for" << requestType << ": " << error;
}

Option<QString> Database::onNewLibrary(const LibraryResponse *message)
{
    JsonifyError error = m_library.deserializeFromJson(message->libraryJson.data());
    if (error.isError())
        return "Failed to parse LibraryResponse: " + error.toString();
    m_hasLibrary = true;
    m_hasRemoteLibraryId = true;
    m_remoteId = m_library.id();
    emit libraryChanged();
    emit newLibrary();
    return {};
}

Option<QString> Database::onNewRemoteId(const IdResponse *message)
{
    if (!m_remoteId.fromString(message->id->toUtf8()))
        return "Failed to parse ID: " + *message->id;

    m_hasRemoteLibraryId = true;

    // wrong library ID? -> request fresh one
    if (m_hasLibrary && m_library.id() != m_remoteId) {
        m_hasLibrary = false;
        m_library = Moosick::Library();
    }

    sync(); // continue syncing, either full or partial download
    return {};
}

void Database::onDownloadQueryTimer()
{
    Q_ASSERT(!m_downloadQuery);

    // start a new query for the currently running downloads at the server
    m_downloadQuery = m_http->requestFromServer(Message(MoosickMessage::DownloadQuery()).toJson());
    m_requests[m_downloadQuery] = DownloadQuery;
}

Option<QString> Database::onDownloadQueryResponse(const DownloadQueryResponse *message)
{
    if (!m_downloadQuery)
        return QString("No download query in progress, something went wrong");

    m_downloadQuery = 0;

    // TODO: populate some QML-exportable list of current downloads, to be viewed after program start

    // get download IDs from JSON data
    QVector<int> runningDownloadIds;
    for (const DownloadQueryResponse::ActiveDownload &download : *message->activeRequests)
        runningDownloadIds << download.first;

    // check if any of our running downloads has finished running on the server
    for (auto it = m_runningDownloads.begin(); it != m_runningDownloads.end(); /* empty */) {
        if (!it->networkReply && !runningDownloadIds.contains(it->id)) {
            // download has finished, get library changes for finished downloads
            // TODO: add downloadIt->artistTags to downloadIt->query.artistId
            sync();
            it = m_runningDownloads.erase(it);
        } else {
            ++it;
        }
    }

    m_downloadsPending = !runningDownloadIds.isEmpty();
    emit downloadsPendingChanged(m_downloadsPending);

    // if there are downloads still running, we'll need to check back later for the results
    if (m_downloadsPending)
        m_downloadQueryTimer.start(5000);

    return {};
}

Option<QString> Database::applyLibraryChanges(const QVector<Moosick::CommittedLibraryChange> &changes)
{
    bool hasChanged = false;

    // add into existing committed changes and apply as many as possible, and delete duplicates
    m_waitingChanges << changes;
    std::sort(m_waitingChanges.begin(), m_waitingChanges.end(), [](const Moosick::CommittedLibraryChange &a, const Moosick::CommittedLibraryChange &b) {
        return a.committedRevision <= b.committedRevision;
    });
    for (auto it = m_waitingChanges.begin(); it != m_waitingChanges.end(); /*empty*/) {
        if (it->committedRevision <= m_library.revision()) {
            it = m_waitingChanges.erase(it);
        }
        else if (it->committedRevision == m_library.revision() + 1) {
            m_library.commit(it->changeRequest);
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

    return {};
}

Option<QString> Database::onDownloadResponse(HttpRequestId reply, const DownloadResponse *message)
{
    const auto downloadIt = std::find_if(m_runningDownloads.begin(), m_runningDownloads.end(), [&](const Download &download) {
        return download.networkReply == reply;
    });

    if (downloadIt != m_runningDownloads.end()) {
        downloadIt->id = message->downloadId;
        downloadIt->networkReply = 0;
    }

    if (!m_downloadQueryTimer.isActive() && !m_downloadQuery)
        m_downloadQueryTimer.start(10000);

    return {};
}

bool Database::hasRunningRequestType(RequestType requestType) const
{
    return std::any_of(m_requests.begin(), m_requests.end(), [=](RequestType tp) {
        return tp == requestType;
    });
}

} // namespace Database
