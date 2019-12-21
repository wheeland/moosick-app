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

    switch (change.changeType) {
    case Moosick::LibraryChange::SongAdd: {
        Album *album = m_albums.find(change.subject);
        requireThat(album, "Album not found");
        auto song = m_songs.create();
        song.second->name = change.name;
        song.second->album = change.detail;
        album->songs << song.first;
        break;
    }
    case Moosick::LibraryChange::SongRemove: {
        auto song = m_songs.find(change.subject);
        requireThat(song, "Song not found");
        Album *album = m_albums.find(change.subject);
        requireThat(album, "Album not found");
        album->songs.removeAll(change.subject);
        m_songs.remove(change.subject);
        break;
    }
    case Moosick::LibraryChange::SongSetName: {
        auto song = m_songs.find(change.subject);
        requireThat(song, "Song not found");
        song->name = change.name;
        break;
    }
    case Moosick::LibraryChange::SongSetAlbum: {
        auto song = m_songs.find(change.subject);
        requireThat(song, "Song not found");
        song->album = change.detail;
        break;
    }
    case Moosick::LibraryChange::SongAddTag: {
        auto song = m_songs.find(change.subject);
        requireThat(song, "Song not found");
        auto tag = m_tags.find(change.detail);
        requireThat(tag, "Tag not found");
        song->tags << change.detail;
        break;
    }
    case Moosick::LibraryChange::SongRemoveTag:
        break;
    case Moosick::LibraryChange::AlbumAdd:
        break;
    case Moosick::LibraryChange::AlbumRemove:
        break;
    case Moosick::LibraryChange::AlbumSetName:
        break;
    case Moosick::LibraryChange::AlbumSetArtist:
        break;
    case Moosick::LibraryChange::AlbumAddTag:
        break;
    case Moosick::LibraryChange::AlbumRemoveTag:
        break;
    case Moosick::LibraryChange::ArtistAdd:
        break;
    case Moosick::LibraryChange::ArtistRemove:
        break;
    case Moosick::LibraryChange::ArtistSetName:
        break;
    case Moosick::LibraryChange::ArtistAddTag:
        break;
    case Moosick::LibraryChange::ArtistRemoveTag:
        break;
    case Moosick::LibraryChange::TagAdd:
        break;
    case Moosick::LibraryChange::TagRemove:
        break;
    case Moosick::LibraryChange::TagSetName:
        break;
    case Moosick::LibraryChange::TagSetParent:
        break;

    }
}

} // namespace Moosick
