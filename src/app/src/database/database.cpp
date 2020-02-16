#include "database.hpp"
#include "database_items.hpp"
#include "database_interface.hpp"
#include "jsonconv.hpp"
#include "../httpclient.hpp"

using Moosick::LibraryChange;
using Moosick::CommittedLibraryChange;

namespace Database {

Database::Database(HttpClient *httpClient, QObject *parent)
    : QObject(parent)
    , m_http(new HttpRequester(httpClient, this))
{
    connect(m_http, &HttpRequester::receivedReply, this, &Database::onNetworkReplyFinished);
}

Database::~Database()
{
}

QNetworkReply *Database::sync()
{
    if (hasRunningRequestType(LibrarySync) || m_hasLibrary)
        return nullptr;

    QNetworkReply *reply = m_http->requestFromServer("/lib.do", "");
    m_requests[reply] = LibrarySync;
    return reply;
}

QNetworkReply *Database::download(NetCommon::DownloadRequest request, const Moosick::TagIdList &albumTags)
{
    QNetworkReply *reply = nullptr;

    request.currentRevision = m_library.revision();

    switch (request.tp) {
    case NetCommon::DownloadRequest::BandcampAlbum: {
        const QString query = QString("v=") + request.toBase64();
        reply = m_http->requestFromServer("/download.do", query);
        m_requests[reply] = BandcampDownload;
        break;
    }
    case NetCommon::DownloadRequest::YoutubeVideo: {
        const QString query = QString("v=") + request.toBase64();
        reply = m_http->requestFromServer("/download.do", query);
        m_requests[reply] = YoutubeDownload;
        break;
    }
    default:
        qFatal("no such download");
        return nullptr;
    }

    m_runningDownloads << Download { request, albumTags, reply };
    return reply;
}

QNetworkReply *Database::setArtistDetails(Moosick::ArtistId id, const QString &name, const Moosick::TagIdList &tags)
{
    return setItemDetails(
        id, id.name(m_library), id.tags(m_library), name, tags,
        LibraryChange::ArtistSetName, LibraryChange::ArtistAddTag, LibraryChange::ArtistRemoveTag
    );
}

QNetworkReply *Database::setAlbumDetails(Moosick::AlbumId id, const QString &name, const Moosick::TagIdList &tags)
{
    return setItemDetails(
        id, id.name(m_library), id.tags(m_library), name, tags,
        LibraryChange::AlbumSetName, LibraryChange::AlbumAddTag, LibraryChange::AlbumRemoveTag
    );
}

QNetworkReply *Database::setSongDetails(Moosick::SongId id, const QString &name, const Moosick::TagIdList &tags)
{
    return setItemDetails(
        id, id.name(m_library), id.tags(m_library), name, tags,
        LibraryChange::SongSetName, LibraryChange::SongAddTag, LibraryChange::SongRemoveTag
    );
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

QNetworkReply *Database::removeArtist(Moosick::ArtistId artistId)
{
    QVector<LibraryChange> changes;
    addRemoveArtist(changes, artistId);
    return sendChangeRequests(changes);
}

QNetworkReply *Database::removeAlbum(Moosick::AlbumId albumId)
{
    QVector<LibraryChange> changes;
    addRemoveAlbum(changes, albumId);
    return sendChangeRequests(changes);
}

QNetworkReply *Database::removeSong(Moosick::SongId songId)
{
    QVector<LibraryChange> changes;
    addRemoveSong(changes, songId);
    return sendChangeRequests(changes);
}

QNetworkReply *Database::sendChangeRequests(const QVector<LibraryChange> &changes)
{
    if (changes.isEmpty())
        return nullptr;

    QByteArray data;
    QDataStream writer(&data, QIODevice::WriteOnly);
    writer << changes;

    const QString query = QString("v=") + data.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    QNetworkReply *reply = m_http->requestFromServer("/request-changes.do", query);
    m_requests[reply] = LibraryChanges;

    return reply;
}

QNetworkReply *Database::setItemDetails(
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

void Database::onNetworkReplyFinished(QNetworkReply *reply, QNetworkReply::NetworkError error)
{
    const RequestType requestType = m_requests.take(reply);
    const QByteArray data = reply->readAll();
    const bool hasError = (error != QNetworkReply::NoError);

    if (hasError) {
        qWarning() << reply->errorString();
    }

    switch (requestType) {
    case LibrarySync: {
        if (!hasError)
            onNewLibrary(parseJsonObject(data, "Library"));
        break;
    }
    case BandcampDownload:
    case YoutubeDownload: {
        const auto downloadIt = std::find_if(m_runningDownloads.begin(), m_runningDownloads.end(), [&](const Download &download) {
            return download.networkReply == reply;
        });

        if (!hasError) {
            applyLibraryChanges(data);
        }

        if (downloadIt != m_runningDownloads.end()) {
            // TODO: add downloadIt->artistTags to downloadIt->query.artistId
            m_runningDownloads.erase(downloadIt);
        }

        break;
    }
    case LibraryChanges: {
        if (!hasError) {
            applyLibraryChanges(data);
        }
        break;
    }
    default:
        break;
    }
}

void Database::onNewLibrary(const QJsonObject &json)
{
    if (m_library.deserializeFromJson(json)) {
        m_hasLibrary = true;
        emit libraryChanged();
    }
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
    qSort(m_waitingChanges.begin(), m_waitingChanges.end(), [](const Moosick::CommittedLibraryChange &a, const Moosick::CommittedLibraryChange &b) {
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
