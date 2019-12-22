#pragma once

#include "type_ids.hpp"
#include "itemcollection.hpp"
#include <QHash>
#include <QDataStream>

namespace Moosick {

struct LibraryChange
{
    enum Type : quint32 {
        Invalid,
        SongAdd,
        SongRemove,
        SongSetName,
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
        ArtistRemove,
        ArtistSetName,
        ArtistAddTag,
        ArtistRemoveTag,

        TagAdd,
        TagRemove,
        TagSetName,
        TagSetParent,
    };

    Type changeType = Invalid;
    quint32 subject = 0;
    quint32 detail = 0;
    QString name;

    bool isCreatingNewId() const;
};

class Library
{
public:
    Library();
    ~Library();

    QVector<TagId> rootTags() const;
    QVector<ArtistId> artistsByName() const;

    bool commit(const LibraryChange &change, quint32 *createdId = nullptr);

    QStringList dumpToStringList() const;

private:
    struct Song;
    struct Album;
    struct Artist;
    struct Tag;

    quint32 m_revision = 0;

    ItemCollection<Song> m_songs;
    ItemCollection<Album> m_albums;
    ItemCollection<Artist> m_artists;
    ItemCollection<Tag> m_tags;
    ItemCollection<QString> m_fileEndings;

    QVector<TagId> m_rootTags;

    friend SongId;
    friend AlbumId;
    friend ArtistId;
    friend TagId;

    friend QDataStream &operator<<(QDataStream &stream, const Library &lib);
    friend QDataStream &operator>>(QDataStream &stream, Library &lib);
};

QDataStream &operator<<(QDataStream &stream, const Library &lib);
QDataStream &operator>>(QDataStream &stream, Library &lib);

QDataStream &operator<<(QDataStream &stream, const LibraryChange &lch);
QDataStream &operator>>(QDataStream &stream, LibraryChange &lch);

} // namespace Moosick
