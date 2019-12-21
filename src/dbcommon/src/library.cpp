#include "library.hpp"
#include <QDebug>

namespace Moosick {

struct Library::Song
{
    QString name;
    AlbumId album;
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
    auto *Name = Collection.find(id); requireThat(Name, #Name " not found");

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
        auto parentTag = m_tags.find(change.subject);
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

        auto parentTag = m_tags.find(tag->parent);
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

        auto oldParent = m_tags.find(tag->parent);
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

    return true;
}

} // namespace Moosick
