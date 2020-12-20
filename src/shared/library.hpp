#pragma once

#include "library_types.hpp"

#include <QHash>
#include <QDataStream>
#include <QJsonObject>
#include <QJsonArray>

namespace Moosick {

struct LibraryChange
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

    LibraryChange() = default;
    LibraryChange(const LibraryChange &other) = default;
    LibraryChange(Type tp, quint32 subj, quint32 det, const QString &nm);

    Type changeType = Invalid;
    quint32 subject = 0;
    quint32 detail = 0;
    QString name;

    static bool isCreatingNewId(Type changeType);
    static bool hasStringArg(Type changeType);
};

struct CommittedLibraryChange
{
    LibraryChange change;
    quint32 revision;
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
     * Upon success, it returns the new revision of the database, otherwise 0
     */
    quint32 commit(const LibraryChange &change, quint32 *createdId = nullptr);

    /**
     * Applies those changes that are from the future (i.e. the DB server)
     */
    void commit(const QVector<CommittedLibraryChange> &changes);

    QVector<CommittedLibraryChange> committedChangesSince(quint32 revision) const;

    QStringList dumpToStringList() const;

    QJsonObject serializeToJson() const;
    bool deserializeFromJson(const QJsonObject &libraryJson, const QJsonArray &logJson = QJsonArray());

private:
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

    quint32 getFileEnding(const QString &ending);

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

    friend QDataStream &operator<<(QDataStream &stream, const Library &lib);
    friend QDataStream &operator>>(QDataStream &stream, Library &lib);
};

QDataStream &operator<<(QDataStream &stream, const Library &lib);
QDataStream &operator>>(QDataStream &stream, Library &lib);

QDataStream &operator<<(QDataStream &stream, const LibraryChange &lch);
QDataStream &operator>>(QDataStream &stream, LibraryChange &lch);

} // namespace Moosick

QJsonValue toJson(const Moosick::LibraryChange &change);
bool fromJson(const QJsonValue &json, Moosick::LibraryChange &change);

QJsonValue toJson(const Moosick::CommittedLibraryChange &change);
bool fromJson(const QJsonValue &json, Moosick::CommittedLibraryChange &change);
