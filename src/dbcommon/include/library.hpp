#pragma once

#include "type_ids.hpp"
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

template <class T, class IntType = quint32>
class Collection
{
public:
    Collection() {}
    ~Collection() {}

    void add(IntType id, const T &value)
    {
        m_data.insert(id, value);
        m_nextId = qMax(m_nextId, id + 1);
    }

    void remove(IntType id)
    {
        m_data.remove(id);
    }

    T *find(IntType id)
    {
        const auto it = m_data.find(id);
        return (it != m_data.end()) ? (&it.value()) : nullptr;
    }

    QPair<IntType, T*> create()
    {
        const auto it = m_data.insert(m_nextId, T());
        m_nextId += 1;
        return qMakePair(m_nextId - 1, &it.value());
    }

private:
    QHash<IntType, T> m_data;
    IntType m_nextId = 1;
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
    Collection<Song> m_songs;
    Collection<Album> m_albums;
    Collection<Artist> m_artists;
    Collection<Tag> m_tags;
};

} // namespace Moosick
