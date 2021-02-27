#pragma once

#include "result.hpp"
#include "jsonconv.hpp"
#include "library_types.hpp"

#include <QHash>
#include <QDataStream>
#include <QJsonObject>
#include <QJsonArray>

namespace Moosick {

struct LibraryChangeRequest
{
    enum Type : quint32 {
        Invalid,
        SongAdd,
        SongRemove,
        SongSetName,
        SongSetPosition,
        SongSetLength,
        SongSetFileEnding,
        SongSetAlbum,
        SongAddTag,
        SongRemoveTag,

        AlbumAdd,
        AlbumRemove,
        AlbumSetName,
        AlbumSetArtist,
        AlbumAddTag,
        AlbumRemoveTag,

        ArtistAdd,
        ArtistAddOrGet,
        ArtistRemove,
        ArtistSetName,
        ArtistAddTag,
        ArtistRemoveTag,

        TagAdd,
        TagRemove,
        TagSetName,
        TagSetParent,
    };

    LibraryChangeRequest() = default;
    LibraryChangeRequest(const LibraryChangeRequest &other) = default;
    LibraryChangeRequest(Type tp, quint32 id, quint32 det = 0, const QString &nm = QString());

#define LIBRARY_CHANGE_REQUEST_DEFINE_STATIC_CTOR(TAG) \
    inline static LibraryChangeRequest Create ## TAG(quint32 id, quint32 det = 0, const QString &nm = QString()) { \
        return LibraryChangeRequest(TAG, id, det, nm); \
    }

    LIBRARY_CHANGE_REQUEST_DEFINE_STATIC_CTOR(Invalid)
    LIBRARY_CHANGE_REQUEST_DEFINE_STATIC_CTOR(SongAdd)
    LIBRARY_CHANGE_REQUEST_DEFINE_STATIC_CTOR(SongRemove)
    LIBRARY_CHANGE_REQUEST_DEFINE_STATIC_CTOR(SongSetName)
    LIBRARY_CHANGE_REQUEST_DEFINE_STATIC_CTOR(SongSetPosition)
    LIBRARY_CHANGE_REQUEST_DEFINE_STATIC_CTOR(SongSetLength)
    LIBRARY_CHANGE_REQUEST_DEFINE_STATIC_CTOR(SongSetFileEnding)
    LIBRARY_CHANGE_REQUEST_DEFINE_STATIC_CTOR(SongSetAlbum)
    LIBRARY_CHANGE_REQUEST_DEFINE_STATIC_CTOR(SongAddTag)
    LIBRARY_CHANGE_REQUEST_DEFINE_STATIC_CTOR(SongRemoveTag)

    LIBRARY_CHANGE_REQUEST_DEFINE_STATIC_CTOR(AlbumAdd)
    LIBRARY_CHANGE_REQUEST_DEFINE_STATIC_CTOR(AlbumRemove)
    LIBRARY_CHANGE_REQUEST_DEFINE_STATIC_CTOR(AlbumSetName)
    LIBRARY_CHANGE_REQUEST_DEFINE_STATIC_CTOR(AlbumSetArtist)
    LIBRARY_CHANGE_REQUEST_DEFINE_STATIC_CTOR(AlbumAddTag)
    LIBRARY_CHANGE_REQUEST_DEFINE_STATIC_CTOR(AlbumRemoveTag)

    LIBRARY_CHANGE_REQUEST_DEFINE_STATIC_CTOR(ArtistAdd)
    LIBRARY_CHANGE_REQUEST_DEFINE_STATIC_CTOR(ArtistAddOrGet)
    LIBRARY_CHANGE_REQUEST_DEFINE_STATIC_CTOR(ArtistRemove)
    LIBRARY_CHANGE_REQUEST_DEFINE_STATIC_CTOR(ArtistSetName)
    LIBRARY_CHANGE_REQUEST_DEFINE_STATIC_CTOR(ArtistAddTag)
    LIBRARY_CHANGE_REQUEST_DEFINE_STATIC_CTOR(ArtistRemoveTag)

    LIBRARY_CHANGE_REQUEST_DEFINE_STATIC_CTOR(TagAdd)
    LIBRARY_CHANGE_REQUEST_DEFINE_STATIC_CTOR(TagRemove)
    LIBRARY_CHANGE_REQUEST_DEFINE_STATIC_CTOR(TagSetName)
    LIBRARY_CHANGE_REQUEST_DEFINE_STATIC_CTOR(TagSetParent)

#undef LIBRARY_CHANGE_REQUEST_DEFINE_STATIC_CTOR

    Type changeType = Invalid;
    quint32 targetId = 0;
    quint32 detail = 0;
    QString name;

    friend QJsonValue enjson(const Moosick::LibraryChangeRequest &change);
    friend void dejson(const QJsonValue &json, Result<Moosick::LibraryChangeRequest, JsonifyError> &result);
};

struct CommittedLibraryChange
{
    LibraryChangeRequest changeRequest;
    quint32 committedRevision;
    quint32 createdId;

    friend QJsonValue enjson(const Moosick::CommittedLibraryChange &change);
    friend void dejson(const QJsonValue &json, Result<Moosick::CommittedLibraryChange, JsonifyError> &result);
};

class LibraryId
{
public:
    static constexpr int LENGTH = 8;

    LibraryId() = default;
    LibraryId(const LibraryId &other) = default;
    LibraryId &operator=(const LibraryId &other) = default;

    bool operator!=(const LibraryId &other) const { return !(*this == other); }
    bool operator==(const LibraryId &other) const;

    static LibraryId generate();

    QByteArray toString() const;
    bool fromString(const QByteArray &string);

private:
    std::array<quint8, LENGTH> m_bytes;
};

class Library
{
public:
    Library();
    ~Library();

    LibraryId id() const { return m_id; }
    quint32 revision() const { return m_revision; }

    QVector<TagId> rootTags() const;
    QVector<ArtistId> artistsByName() const;

    /**
     * Tries to commit the given change, returns an error string if something went wrong
     */
    Result<CommittedLibraryChange, QString> commit(const LibraryChangeRequest &change);

    /**
     * Applies those changes that are from the future (i.e. the DB server).
     */
    void commit(const QVector<CommittedLibraryChange> &changes);

    /**
     * Retrieves all changes that have been committed since the given revision.
     * (This must not necessarily contain all changes ever made, and can be empty)
     */
    QVector<CommittedLibraryChange> committedChangesSince(quint32 revision) const;

    /**
     * For debugging purposes, dump into human-readable listing
     */
    QStringList dumpToStringList() const;

    QJsonObject serializeToJson() const;

    /**
     * Tries to read the whole library from JSON data.
     * If committedChanges is not empty, try to read a history of committed changes from this array.
     */
    JsonifyError deserializeFromJson(const QJsonObject &libraryJson, const QJsonArray &committedChanges = QJsonArray());

private:
    void deserializeFromJsonInternal(const QJsonObject &libraryJson, const QJsonArray &committedChanges, Result<int, JsonifyError> &result);

    struct Song
    {
        QString name;
        AlbumId album;
        quint32 fileEnding = 0;
        quint32 position = 0;
        quint32 secs = 0;
        TagIdList tags;
    };

    struct Album
    {
        QString name;
        ArtistId artist;
        SongIdList songs;
        TagIdList tags;
    };

    struct Artist
    {
        QString name;
        AlbumIdList albums;
        TagIdList tags;
    };

    struct Tag
    {
        QString name;
        TagId parent;
        TagIdList children;
        SongIdList songs;
        AlbumIdList albums;
        ArtistIdList artists;
    };

    friend QJsonValue enjson(const Song &song);
    friend QJsonValue enjson(const Album &album);
    friend QJsonValue enjson(const Artist &artist);
    friend QJsonValue enjson(const Tag &tag);

    friend void dejson(const QJsonValue &json, Result<Song, JsonifyError> &result);
    friend void dejson(const QJsonValue &json, Result<Album, JsonifyError> &result);
    friend void dejson(const QJsonValue &json, Result<Artist, JsonifyError> &result);
    friend void dejson(const QJsonValue &json, Result<Tag, JsonifyError> &result);

    quint32 getOrCreateFileEndingId(const QString &ending);

    quint32 m_revision = 0;
    LibraryId m_id = LibraryId::generate();
    QVector<CommittedLibraryChange> m_committedChanges;

    ItemCollection<Song> m_songs;
    ItemCollection<Album> m_albums;
    ItemCollection<Artist> m_artists;
    ItemCollection<Tag> m_tags;
    ItemCollection<QString> m_fileEndings;

    QVector<TagId> m_rootTags;

    friend struct SongId;
    friend struct AlbumId;
    friend struct ArtistId;
    friend struct TagId;
};

} // namespace Moosick
