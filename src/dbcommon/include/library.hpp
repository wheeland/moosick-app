#pragma once

#include "type_ids.hpp"
#include "itemcollection.hpp"
#include <QHash>

namespace Moosick {

struct LibraryChange
{
    enum Type {
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

    Type changeType;
    quint32 subject;
    quint32 detail;
    QString name;
};

class Library
{
public:
    Library();
    ~Library();

    bool commit(const LibraryChange &change);

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
};

} // namespace Moosick
