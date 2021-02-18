#include "library.hpp"
#include "library_messages.hpp"

#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>

namespace Moosick {

LibraryChangeRequest::LibraryChangeRequest(LibraryChangeRequest::Type tp, quint32 id, quint32 det, const QString &nm)
    : changeType(tp)
    , targetId(id)
    , detail(det)
    , name(nm)
{
}

Library::Library()
{
}

Library::~Library()
{
}

QVector<TagId> Library::rootTags() const
{
    return m_rootTags;
}

QVector<ArtistId> Library::artistsByName() const
{
    QVector<ArtistId> ret = m_artists.ids<ArtistId>();
    std::sort(ret.begin(), ret.end(), [&](ArtistId a, ArtistId b) {
        const int cmp = a.name(*this).localeAwareCompare(b.name(*this));
        return cmp <= 0;
    });
    return ret;
}

quint32 Library::getOrCreateFileEndingId(const QString &ending)
{
    for (auto it = m_fileEndings.begin(); it != m_fileEndings.end(); ++it) {
        if (it.value() == ending)
            return it.key();
    }
    auto newEntry = m_fileEndings.create();
    *newEntry.second = ending;
    return newEntry.first;
}

Result<CommittedLibraryChange, QString> Library::commit(const LibraryChangeRequest &change)
{
#define requireThat(condition, message) \
    do { if (!(condition)) return QString(message); } while (0)

#define fetchItem(Collection, Name, id) \
    auto *Name = Collection.findItem(id); requireThat(Name, #Name " not found");

    CommittedLibraryChange commit{change, 0, 0};

    switch (change.changeType) {
    case Moosick::LibraryChangeRequest::SongAdd: {
        fetchItem(m_albums, album, change.targetId);

        auto song = m_songs.create();
        song.second->name = change.name;
        song.second->album = change.targetId;
        album->songs << song.first;
        commit.createdId = song.first;
        break;
    }
    case Moosick::LibraryChangeRequest::SongRemove: {
        fetchItem(m_songs, song, change.targetId);
        fetchItem(m_albums, album, song->album);
        requireThat(song->tags.isEmpty(), "Song still has tags");
        Q_ASSERT(album->songs.contains(change.targetId));

        album->songs.removeAll(change.targetId);
        m_songs.remove(change.targetId);
        break;
    }
    case Moosick::LibraryChangeRequest::SongSetName: {
        fetchItem(m_songs, song, change.targetId);

        song->name = change.name;
        break;
    }
    case Moosick::LibraryChangeRequest::SongSetPosition: {
        fetchItem(m_songs, song, change.targetId);

        song->position = change.detail;
        break;
    }
    case Moosick::LibraryChangeRequest::SongSetLength: {
        fetchItem(m_songs, song, change.targetId);

        song->secs = change.detail;
        break;
    }
    case Moosick::LibraryChangeRequest::SongSetFileEnding: {
        fetchItem(m_songs, song, change.targetId);

        song->fileEnding = getOrCreateFileEndingId(change.name);
        break;
    }
    case Moosick::LibraryChangeRequest::SongSetAlbum: {
        fetchItem(m_songs, song, change.targetId);
        fetchItem(m_albums, oldAlbum, song->album);
        fetchItem(m_albums, newAlbum, change.detail);
        Q_ASSERT(oldAlbum->songs.contains(change.targetId));

        song->album = change.detail;
        oldAlbum->songs.removeAll(change.targetId);
        newAlbum->songs << change.targetId;
        break;
    }
    case Moosick::LibraryChangeRequest::SongAddTag: {
        fetchItem(m_songs, song, change.targetId);
        fetchItem(m_tags, tag, change.detail);
        requireThat(!song->tags.contains(change.detail), "Tag already on song");

        song->tags << change.detail;
        tag->songs << change.targetId;
        break;
    }
    case Moosick::LibraryChangeRequest::SongRemoveTag: {
        fetchItem(m_songs, song, change.targetId);
        fetchItem(m_tags, tag, change.detail);
        requireThat(song->tags.contains(change.detail), "Tag not on Song");
        Q_ASSERT(tag->songs.contains(change.targetId));

        song->tags.removeAll(change.detail);
        tag->songs.removeAll(change.targetId);
        break;
    }
    case Moosick::LibraryChangeRequest::AlbumAdd: {
        fetchItem(m_artists, artist, change.targetId);

        auto album = m_albums.create();
        album.second->artist = change.targetId;
        album.second->name = change.name;
        artist->albums << album.first;
        commit.createdId = album.first;
        break;
    }
    case Moosick::LibraryChangeRequest::AlbumRemove: {
        fetchItem(m_albums, album, change.targetId);
        fetchItem(m_artists, artist, album->artist);
        requireThat(album->songs.isEmpty(), "Album still contains songs");
        requireThat(album->tags.isEmpty(), "Album still has tags");
        Q_ASSERT(artist->albums.contains(change.targetId));

        artist->albums.removeAll(change.targetId);
        m_albums.remove(change.targetId);
        break;
    }
    case Moosick::LibraryChangeRequest::AlbumSetName: {
        fetchItem(m_albums, album, change.targetId);

        album->name = change.name;
        break;
    }
    case Moosick::LibraryChangeRequest::AlbumSetArtist: {
        fetchItem(m_albums, album, change.targetId);
        fetchItem(m_artists, oldArtist, album->artist);
        fetchItem(m_artists, newArtist, change.detail);
        Q_ASSERT(oldArtist->albums.contains(change.targetId));

        album->artist = change.detail;
        oldArtist->albums.removeAll(change.targetId);
        newArtist->albums << change.targetId;
        break;
    }
    case Moosick::LibraryChangeRequest::AlbumAddTag: {
        fetchItem(m_albums, album, change.targetId);
        fetchItem(m_tags, tag, change.detail);
        requireThat(!album->tags.contains(change.detail), "Tag already on album");

        album->tags << change.detail;
        tag->albums << change.targetId;
        break;
    }
    case Moosick::LibraryChangeRequest::AlbumRemoveTag: {
        fetchItem(m_albums, album, change.targetId);
        fetchItem(m_tags, tag, change.detail);
        requireThat(album->tags.contains(change.detail), "Tag not on Album");
        Q_ASSERT(tag->albums.contains(change.targetId));

        album->tags.removeAll(change.detail);
        tag->albums.removeAll(change.targetId);
        break;
    }
    case Moosick::LibraryChangeRequest::ArtistAddOrGet: {
        // See if the artist with that name already exists
        for (ArtistId artistId : m_artists.ids<ArtistId>()) {
            if (artistId.name(*this) == change.name) {
                commit.createdId = artistId;
                break;
            }
        }
        // fall through
    }
    case Moosick::LibraryChangeRequest::ArtistAdd: {
        auto artist = m_artists.create();
        artist.second->name = change.name;
        commit.createdId = artist.first;

        break;
    }
    case Moosick::LibraryChangeRequest::ArtistRemove: {
        fetchItem(m_artists, artist, change.targetId);
        requireThat(artist->albums.isEmpty(), "Artist still has albums");
        requireThat(artist->tags.isEmpty(), "Artist still has tags");

        m_artists.remove(change.targetId);
        break;
    }
    case Moosick::LibraryChangeRequest::ArtistSetName: {
        fetchItem(m_artists, artist, change.targetId);
        artist->name = change.name;
        break;
    }
    case Moosick::LibraryChangeRequest::ArtistAddTag: {
        fetchItem(m_artists, artist, change.targetId);
        fetchItem(m_tags, tag, change.detail);
        requireThat(!artist->tags.contains(change.detail), "Tag already on artist");

        artist->tags << change.detail;
        tag->artists << change.targetId;
        break;
    }
    case Moosick::LibraryChangeRequest::ArtistRemoveTag: {
        fetchItem(m_artists, artist, change.targetId);
        fetchItem(m_tags, tag, change.detail);
        requireThat(artist->tags.contains(change.detail), "Tag not on Artist");
        Q_ASSERT(tag->artists.contains(change.targetId));

        artist->tags.removeAll(change.detail);
        tag->artists.removeAll(change.targetId);
        break;
    }
    case Moosick::LibraryChangeRequest::TagAdd: {
        auto parentTag = m_tags.findItem(change.targetId);
        requireThat(parentTag || (change.targetId == 0), "Parent tag not found");

        auto tag = m_tags.create();
        tag.second->name = change.name;
        tag.second->parent = change.targetId;
        if (parentTag)
            parentTag->children << tag.first;
        else
            m_rootTags << tag.first;

        commit.createdId = tag.first;

        break;
    }
    case Moosick::LibraryChangeRequest::TagRemove: {
        fetchItem(m_tags, tag, change.targetId);
        requireThat(tag->children.isEmpty(), "Tag still contains children");
        requireThat(tag->songs.isEmpty(), "Tag still used for songs");
        requireThat(tag->albums.isEmpty(), "Tag still used for albums");
        requireThat(tag->artists.isEmpty(), "Tag still used for artists");

        auto parentTag = m_tags.findItem(tag->parent);
        if (parentTag) {
            Q_ASSERT(parentTag->children.contains(change.targetId));
            parentTag->children.removeAll(change.targetId);
        } else {
            Q_ASSERT(m_rootTags.contains(change.targetId));
            m_rootTags.removeAll(change.targetId);
        }

        m_tags.remove(change.targetId);
        break;
    }
    case Moosick::LibraryChangeRequest::TagSetName: {
        fetchItem(m_tags, tag, change.targetId);
        tag->name = change.name;
        break;
    }
    case Moosick::LibraryChangeRequest::TagSetParent: {
        fetchItem(m_tags, tag, change.targetId);
        requireThat(change.detail != tag->parent, "Parent is the same");
        requireThat(change.targetId != change.detail, "Can't be your own parent");

        Tag *oldParent = m_tags.findItem(tag->parent);
        Tag *newParent = m_tags.findItem(change.detail);

        Q_ASSERT(oldParent || (tag->parent == 0));
        Q_ASSERT(newParent || (change.detail == 0));

        // make sure we don't introduce circularity
        TagId parentsId = change.detail;
        while (parentsId) {
            if (parentsId == change.targetId) {
                return QString("Detected circular tag parenting");
            }
            Tag *parent = m_tags.findItem(parentsId);
            parentsId = parent ? parent->parent : TagId();
        }

        if (tag->parent == 0) {
            Q_ASSERT(!oldParent);
            Q_ASSERT(m_rootTags.contains(change.targetId));
            m_rootTags.removeAll(change.targetId);
        }
        else {
            Q_ASSERT(oldParent);
            Q_ASSERT(oldParent->children.contains(change.targetId));
            oldParent->children.removeAll(change.targetId);
        }

        if (change.detail == 0) {
            Q_ASSERT(!newParent);
            m_rootTags << change.targetId;
        } else {
            Q_ASSERT(newParent);
            newParent->children << change.targetId;
        }

        tag->parent = change.detail;

        break;
    }
    case Moosick::LibraryChangeRequest::Invalid:
    default: {
        return QString("No such LibraryChangeRequest");
    }
    }

#undef requireThat
#undef fetchItem

    m_revision += 1;
    commit.committedRevision = m_revision;
    m_committedChanges << commit;

    return commit;
}

void Library::commit(const QVector<CommittedLibraryChange> &changes)
{
    for (const CommittedLibraryChange &change : changes) {
        if (change.committedRevision == m_revision + 1) {
            Result<CommittedLibraryChange, QString> committed = commit(change.changeRequest);
            Q_ASSERT(committed.hasValue());
            Q_ASSERT(committed.getValue().committedRevision == change.committedRevision);
        }
    }
}

QVector<CommittedLibraryChange> Library::committedChangesSince(quint32 revision) const
{
    QVector<CommittedLibraryChange> ret;
    ret.reserve(m_committedChanges.size());

    for (const CommittedLibraryChange &committed : m_committedChanges) {
        if (committed.committedRevision >= revision)
            ret << committed;
    }

    return ret;
}

#define FETCH(name, Collection, id) \
    auto name = library.Collection.findItem(id); \
    if (!name) return {};

bool SongId::exists(const Library &library) const
{
    return library.m_songs.contains(m_value);
}

AlbumId SongId::album(const Library &library) const
{
    FETCH(song, m_songs, m_value);
    return song->album;
}

ArtistId SongId::artist(const Library &library) const
{
    FETCH(song, m_songs, m_value);
    FETCH(album, m_albums, song->album);
    return album->artist;
}

TagIdList SongId::tags(const Library &library) const
{
    FETCH(song, m_songs, m_value);
    return song->tags;
}

QString SongId::name(const Library &library) const
{
    FETCH(song, m_songs, m_value);
    return song->name;
}

quint32 SongId::position(const Library &library) const
{
    FETCH(song, m_songs, m_value);
    return song->position;
}

quint32 SongId::secs(const Library &library) const
{
    FETCH(song, m_songs, m_value);
    return song->secs;
}

QString SongId::filePath(const Library &library) const
{
    FETCH(song, m_songs, m_value);
    FETCH(ending, m_fileEndings, song->fileEnding);
    return QString::number(m_value) + ending;
}

bool AlbumId::exists(const Library &library) const
{
    return library.m_albums.contains(m_value);
}

ArtistId AlbumId::artist(const Library &library) const
{
    FETCH(album, m_albums, m_value);
    return album->artist;
}

SongIdList AlbumId::songs(const Library &library) const
{
    FETCH(album, m_albums, m_value);
    return album->songs;
}

TagIdList AlbumId::tags(const Library &library) const
{
    FETCH(album, m_albums, m_value);
    return album->tags;
}

QString AlbumId::name(const Library &library) const
{
    FETCH(album, m_albums, m_value);
    return album->name;
}

bool ArtistId::exists(const Library &library) const
{
    return library.m_artists.contains(m_value);
}

AlbumIdList ArtistId::albums(const Library &library) const
{
    FETCH(artist, m_artists, m_value);
    return artist->albums;
}

TagIdList ArtistId::tags(const Library &library) const
{
    FETCH(artist, m_artists, m_value);
    return artist->tags;
}

QString ArtistId::name(const Library &library) const
{
    FETCH(artist, m_artists, m_value);
    return artist->name;
}

bool TagId::exists(const Library &library) const
{
    return library.m_tags.contains(m_value);
}

TagId TagId::parent(const Library &library) const
{
    FETCH(tag, m_tags, m_value);
    return tag->parent;
}

TagIdList TagId::children(const Library &library) const
{
    FETCH(tag, m_tags, m_value);
    return tag->children;
}

ArtistIdList TagId::artists(const Library &library) const
{
    FETCH(tag, m_tags, m_value);
    return tag->artists;
}

AlbumIdList TagId::albums(const Library &library) const
{
    FETCH(tag, m_tags, m_value);
    return tag->albums;
}

SongIdList TagId::songs(const Library &library) const
{
    FETCH(tag, m_tags, m_value);
    return tag->songs;
}

QString TagId::name(const Library &library) const
{
    FETCH(tag, m_tags, m_value);
    return tag->name;
}

#undef FETCH

} // namespace Moosick

namespace MoosickMessage {

QByteArray messageToJson(const MessageBase &message)
{
    QJsonObject obj;
    obj["id"] = message.getMessageTypeString();
    obj["data"] = enjson(message);
    return jsonSerializeObject(obj);
}

QByteArray Message::toJson() const
{
    return m_msg ? messageToJson(*m_msg) : QByteArray();
}

QString typeString(Type messageType)
{
    switch (messageType) {
    case Type::Error: return "Error";
    case Type::Ping: return "Ping";
    case Type::Pong: return "Pong";
    case Type::LibraryRequest: return "LibraryRequest";
    case Type::LibraryResponse: return "LibraryResponse";
    case Type::IdRequest: return "IdRequest";
    case Type::IdResponse: return "IdResponse";
    case Type::ChangesRequest: return "ChangesRequest";
    case Type::ChangesResponse: return "ChangesResponse";
    case Type::ChangeListRequest: return "ChangeListRequest";
    case Type::ChangeListResponse: return "ChangeListResponse";
    case Type::DownloadRequest: return "DownloadRequest";
    case Type::DownloadResponse: return "DownloadResponse";
    case Type::DownloadQuery: return "DownloadQuery";
    case Type::DownloadQueryResponse: return "DownloadQueryResponse";
    case Type::YoutubeUrlQuery: return "YoutubeUrlQuery";
    case Type::YoutubeUrlResponse: return "YoutubeUrlResponse";
    default:
        qFatal("No such Message Type");
    }
}

Result<Message, JsonifyError> Message::fromJson(const QByteArray &message)
{
    Result<QJsonObject, JsonifyError> json = jsonDeserializeObject(message);
    if (!json)
        return json.takeError();

    // Read ID
    const QJsonObject jsonObj = json.takeValue();
    const auto idIt = jsonObj.find("id");
    if (idIt == jsonObj.end())
        return JsonifyError::buildMissingMemberError("id");
    if (idIt->type() != QJsonValue::String)
        return JsonifyError::buildTypeError(QJsonValue::String, idIt->type());
    const QString id = idIt->toString();

    // Read Data Object
    const auto dataIt = jsonObj.find("data");
    if (dataIt == jsonObj.end())
        return JsonifyError::buildMissingMemberError("data");
    if (dataIt->type() != QJsonValue::Object)
        return JsonifyError::buildTypeError(QJsonValue::Object, dataIt->type());
    const QJsonObject data = dataIt->toObject();

    // See if we find a matching message
    #define CHECK_MESSAGE_TYPE(TYPE) \
    if (id == typeString(TYPE::MESSAGE_TYPE)) { \
        Result<TYPE, JsonifyError> result = ::dejson<TYPE>(data); \
        if (!result) \
            return result.takeError(); \
        return Message(new TYPE(result.takeValue())); \
    }

    CHECK_MESSAGE_TYPE(Error)
    CHECK_MESSAGE_TYPE(Ping)
    CHECK_MESSAGE_TYPE(Pong)

    CHECK_MESSAGE_TYPE(LibraryRequest)
    CHECK_MESSAGE_TYPE(LibraryResponse)

    CHECK_MESSAGE_TYPE(IdRequest)
    CHECK_MESSAGE_TYPE(IdResponse)

    CHECK_MESSAGE_TYPE(ChangesRequest)
    CHECK_MESSAGE_TYPE(ChangesResponse)

    CHECK_MESSAGE_TYPE(ChangeListRequest)
    CHECK_MESSAGE_TYPE(ChangeListResponse)

    CHECK_MESSAGE_TYPE(DownloadRequest)
    CHECK_MESSAGE_TYPE(DownloadResponse)
    CHECK_MESSAGE_TYPE(DownloadQuery)
    CHECK_MESSAGE_TYPE(DownloadQueryResponse)

    CHECK_MESSAGE_TYPE(YoutubeUrlQuery)
    CHECK_MESSAGE_TYPE(YoutubeUrlResponse)

    return JsonifyError::buildCustomError(QString("No such message ID: ") + id);
}

}
