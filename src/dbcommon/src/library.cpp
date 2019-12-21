#include "library.hpp"
#include <QDebug>

namespace Moosick {

struct Library::Song
{
    QString name;
    AlbumId album;
    quint32 fileEnding;
    TagIdList tags;
};

struct Library::Album
{
    QString name;
    ArtistId artist;
    SongIdList songs;
    TagIdList tags;
};

struct Library::Artist
{
    QString name;
    AlbumIdList albums;
    TagIdList tags;
};

struct Library::Tag
{
    QString name;
    TagId parent;
    TagIdList children;
    SongIdList songs;
    AlbumIdList albums;
    ArtistIdList artists;
};

Library::Library()
{
}

Library::~Library()
{
}

bool Library::commit(const LibraryChange &change)
{
#define requireThat(condition, message) \
    do { if (!(condition)) { qWarning() << (message); return false; } } while (0)

#define fetchItem(Collection, Name, id) \
    auto *Name = Collection.findItem(id); requireThat(Name, #Name " not found");

    switch (change.changeType) {
    case Moosick::LibraryChange::SongAdd: {
        fetchItem(m_albums, album, change.subject);

        auto song = m_songs.create();
        song.second->name = change.name;
        song.second->album = change.detail;
        album->songs << song.first;
        break;
    }
    case Moosick::LibraryChange::SongRemove: {
        fetchItem(m_songs, song, change.subject);
        fetchItem(m_albums, album, song->album);
        Q_ASSERT(album->songs.contains(change.subject));

        album->songs.removeAll(change.subject);
        m_songs.remove(change.subject);
        break;
    }
    case Moosick::LibraryChange::SongSetName: {
        fetchItem(m_songs, song, change.subject);

        song->name = change.name;
        break;
    }
    case Moosick::LibraryChange::SongSetAlbum: {
        fetchItem(m_songs, song, change.subject);
        fetchItem(m_albums, oldAlbum, song->album);
        fetchItem(m_albums, newAlbum, change.detail);
        Q_ASSERT(oldAlbum->songs.contains(change.subject));

        song->album = change.detail;
        oldAlbum->songs.removeAll(change.subject);
        newAlbum->songs << change.subject;
        break;
    }
    case Moosick::LibraryChange::SongAddTag: {
        fetchItem(m_songs, song, change.subject);
        fetchItem(m_tags, tag, change.detail);
        requireThat(!song->tags.contains(change.detail), "Tag already on song");

        song->tags << change.detail;
        tag->songs << change.subject;
        break;
    }
    case Moosick::LibraryChange::SongRemoveTag: {
        fetchItem(m_songs, song, change.subject);
        fetchItem(m_tags, tag, change.detail);
        requireThat(song->tags.contains(change.detail), "Tag not on Song");
        Q_ASSERT(tag->songs.contains(change.subject));

        song->tags.removeAll(change.detail);
        tag->songs.removeAll(change.subject);
        break;
    }
    case Moosick::LibraryChange::AlbumAdd: {
        fetchItem(m_artists, artist, change.subject);

        auto album = m_albums.create();
        album.second->artist = change.subject;
        album.second->name = change.name;
        artist->albums << album.first;
        break;
    }
    case Moosick::LibraryChange::AlbumRemove: {
        fetchItem(m_albums, album, change.subject);
        fetchItem(m_artists, artist, album->artist);
        requireThat(album->songs.isEmpty(), "Album still contains songs");
        Q_ASSERT(artist->albums.contains(change.subject));

        artist->albums.removeAll(change.subject);
        m_songs.remove(change.subject);
        break;
    }
    case Moosick::LibraryChange::AlbumSetName: {
        fetchItem(m_albums, album, change.subject);

        album->name = change.name;
        break;
    }
    case Moosick::LibraryChange::AlbumSetArtist: {
        fetchItem(m_albums, album, change.subject);
        fetchItem(m_artists, oldArtist, album->artist);
        fetchItem(m_artists, newArtist, change.detail);
        Q_ASSERT(oldArtist->albums.contains(change.subject));

        album->artist = change.detail;
        oldArtist->albums.removeAll(change.subject);
        newArtist->albums << change.subject;
        break;
    }
    case Moosick::LibraryChange::AlbumAddTag: {
        fetchItem(m_albums, album, change.subject);
        fetchItem(m_tags, tag, change.detail);
        requireThat(!album->tags.contains(change.detail), "Tag already on album");

        album->tags << change.detail;
        tag->albums << change.subject;
        break;
    }
    case Moosick::LibraryChange::AlbumRemoveTag: {
        fetchItem(m_albums, album, change.subject);
        fetchItem(m_tags, tag, change.detail);
        requireThat(album->tags.contains(change.detail), "Tag not on Album");
        Q_ASSERT(tag->albums.contains(change.subject));

        album->tags.removeAll(change.detail);
        tag->albums.removeAll(change.subject);
        break;
    }
    case Moosick::LibraryChange::ArtistAdd: {
        auto artist = m_artists.create();
        artist.second->name = change.name;
        break;
    }
    case Moosick::LibraryChange::ArtistRemove: {
        fetchItem(m_artists, artist, change.subject);
        requireThat(artist->albums.isEmpty(), "Artist still has albums");

        m_artists.remove(change.subject);
        break;
    }
    case Moosick::LibraryChange::ArtistSetName: {
        fetchItem(m_artists, artist, change.subject);
        artist->name = change.name;
        break;
    }
    case Moosick::LibraryChange::ArtistAddTag: {
        fetchItem(m_artists, artist, change.subject);
        fetchItem(m_tags, tag, change.detail);
        requireThat(!artist->tags.contains(change.detail), "Tag already on artist");

        artist->tags << change.detail;
        tag->artists << change.subject;
        break;
    }
    case Moosick::LibraryChange::ArtistRemoveTag: {
        fetchItem(m_artists, artist, change.subject);
        fetchItem(m_tags, tag, change.detail);
        requireThat(artist->tags.contains(change.detail), "Tag not on Artist");
        Q_ASSERT(tag->artists.contains(change.subject));

        artist->tags.removeAll(change.detail);
        tag->artists.removeAll(change.subject);
        break;
    }
    case Moosick::LibraryChange::TagAdd: {
        auto parentTag = m_tags.findItem(change.subject);
        requireThat(parentTag || (change.subject == 0), "Parent tag not found");

        auto tag = m_tags.create();
        tag.second->name = change.name;
        tag.second->parent = change.subject;
        if (parentTag)
            parentTag->children << tag.first;
        break;
    }
    case Moosick::LibraryChange::TagRemove: {
        fetchItem(m_tags, tag, change.subject);
        requireThat(tag->children.isEmpty(), "Tag still contains children");
        requireThat(tag->songs.isEmpty(), "Tag still used for songs");
        requireThat(tag->albums.isEmpty(), "Tag still used for albums");
        requireThat(tag->artists.isEmpty(), "Tag still used for artists");

        auto parentTag = m_tags.findItem(tag->parent);
        if (parentTag) {
            Q_ASSERT(parentTag->children.contains(change.subject));
            parentTag->children.removeAll(change.subject);
        }

        m_tags.remove(change.subject);
        break;
    }
    case Moosick::LibraryChange::TagSetName: {
        fetchItem(m_tags, tag, change.subject);
        tag->name = change.name;
        break;
    }
    case Moosick::LibraryChange::TagSetParent: {
        fetchItem(m_tags, tag, change.subject);
        fetchItem(m_tags, newParent, change.detail);
        requireThat(change.detail != tag->parent, "Parent is the same");

        auto oldParent = m_tags.findItem(tag->parent);
        Q_ASSERT(oldParent || (tag->parent == 0));
        if (oldParent) {
            Q_ASSERT(oldParent->children.contains(change.subject));
            oldParent->children.removeAll(change.subject);
        }

        tag->parent = change.detail;
        newParent->children << change.subject;
        break;
    }
    }

#undef requireThat
#undef fetchItem

    m_revision += 1;

    return true;
}

#define FETCH(name, Collection, id) \
    auto name = library.Collection.findItem(id); \
    if (!name) return {};

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

QString SongId::filePath(const Library &library) const
{
    FETCH(song, m_songs, m_value);
    FETCH(ending, m_fileEndings, song->fileEnding);
    return song->name + ending;
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

QString TagId::name(const Library &library) const
{
    FETCH(tag, m_tags, m_value);
    return tag->name;
}

#undef FETCH

QDataStream &operator<<(QDataStream &stream, const LibraryChange &lch)
{
    stream << static_cast<quint32>(lch.changeType);
    stream << lch.subject;
    stream << lch.detail;
    stream << lch.name;
    return stream;
}

QDataStream &operator>>(QDataStream &stream, LibraryChange &lch)
{
    quint32 tp;
    stream >> tp;
    lch.changeType = static_cast<LibraryChange::Type>(tp);
    stream >> lch.subject;
    stream >> lch.detail;
    stream >> lch.name;
    return stream;
}

template <class T, class IntType>
void dumpCollection(QDataStream &stream, const ItemCollection<T, IntType> &col, const std::function<void(const T&)> &dumper)
{
    stream << col.size();
    for (auto it = col.begin(), end = col.end(); it != end; ++it) {
        stream << it.key();
        dumper(it.value());
    }
}

QDataStream &operator<<(QDataStream &stream, const Library &lib)
{
    stream << lib.m_revision;

    dumpCollection<Library::Tag>(stream, lib.m_tags, [&](const Library::Tag &tag) {
        stream << tag.name;
        stream << tag.parent;
    });

    dumpCollection<Library::Artist>(stream, lib.m_artists, [&](const Library::Artist &artist) {
        stream << artist.name;
        stream << artist.tags;
    });

    dumpCollection<Library::Album>(stream, lib.m_albums, [&](const Library::Album &album) {
        stream << album.name;
        stream << album.artist;
        stream << album.tags;
    });

    dumpCollection<Library::Song>(stream, lib.m_songs, [&](const Library::Song &song) {
        stream << song.name;
        stream << song.album;
        stream << song.fileEnding;
        stream << song.tags;
    });

    return stream;
}

template <class T, class IntType>
void readCollection(QDataStream &stream, ItemCollection<T, IntType> &col, const std::function<T(IntType)> &reader)
{
    quint32 sz;
    stream >> sz;

    for (quint32 i = 0; i < sz; ++i) {
        IntType id;
        stream >> id;

        const T newItem = reader(id);
        col.add(id, newItem);
    }
}

template <class IntType>
QDataStream &operator>>(QDataStream &stream, detail::FromInt<IntType> &dst) {
    quint32 into;
    stream >> into;
    dst = into;
    return stream;
}

QDataStream &operator>>(QDataStream &stream, Library &lib)
{
    stream >> lib.m_revision;

    readCollection<Library::Tag, quint32>(stream, lib.m_tags, [&](quint32) {
        Library::Tag newTag;
        stream >> newTag.name;
        stream >> newTag.parent;
        return newTag;
    });

#define TAG_PUSH_ID(tagId, member, id) \
    do { \
        Library::Tag *tag = lib.m_tags.findItem(tagId); \
        if (tag) tag->member << (id); \
    } while (0)

    // fix up tag child lists
    for (auto it = lib.m_tags.begin(); it != lib.m_tags.end(); ++it)
        TAG_PUSH_ID(it.value().parent, children, it.key());

    // read artists
    readCollection<Library::Artist, quint32>(stream, lib.m_artists, [&](quint32 id) {
        Library::Artist newArtist;
        stream >> newArtist.name;
        stream >> newArtist.tags;
        for (quint32 tagId : newArtist.tags)
            TAG_PUSH_ID(tagId, artists, id);
        return newArtist;
    });

    // read albums
    readCollection<Library::Album, quint32>(stream, lib.m_albums, [&](quint32 id) {
        Library::Album newAlbum;
        stream >> newAlbum.name;
        stream >> newAlbum.artist;
        stream >> newAlbum.tags;
        for (quint32 tagId : newAlbum.tags)
            TAG_PUSH_ID(tagId, albums, id);
        if (Library::Artist *artist = lib.m_artists.findItem(newAlbum.artist))
            artist->albums << id;
        return newAlbum;
    });

    // read songs
    readCollection<Library::Song, quint32>(stream, lib.m_songs, [&](quint32 id) {
        Library::Song newSong;
        stream >> newSong.name;
        stream >> newSong.album;
        stream >> newSong.fileEnding;
        stream >> newSong.tags;
        for (quint32 tagId : newSong.tags)
            TAG_PUSH_ID(tagId, songs, id);
        if (Library::Album *album = lib.m_albums.findItem(newSong.album))
            album->songs << id;
        return newSong;
    });

#undef TAG_PUSH_ID

    return stream;
}

} // namespace Moosick
