#include "library.hpp"
#include "library_priv.hpp"

#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>

namespace Moosick {

LibraryChange::LibraryChange(LibraryChange::Type tp, quint32 subj, quint32 det, const QString &nm)
    : changeType(tp)
    , subject(subj)
    , detail(det)
    , name(nm)
{
}

bool LibraryChange::isCreatingNewId(Type changeType)
{
    switch (changeType) {
    case TagAdd:
    case ArtistAdd:
    case AlbumAdd:
    case SongAdd:
        return true;
    default:
        return false;
    }
}

bool LibraryChange::hasStringArg(Type changeType)
{
    switch (changeType) {
    case TagAdd:
    case ArtistAdd:
    case AlbumAdd:
    case SongAdd:
    case SongSetFileEnding:
    case TagSetName:
    case ArtistSetName:
    case AlbumSetName:
    case SongSetName:
        return true;
    default:
        return false;
    }
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
        return a.name(*this).localeAwareCompare(b.name(*this));
    });
    return ret;
}

QByteArray Library::serialize() const
{
    QByteArray ret;
    QDataStream out(&ret, QIODevice::WriteOnly);
    out << *this;
    return ret;
}

void Library::deserialize(const QByteArray &bytes)
{
    QDataStream in(bytes);
    in >> *this;
}

quint32 Library::getFileEnding(const QString &ending)
{
    for (auto it = m_fileEndings.begin(); it != m_fileEndings.end(); ++it) {
        if (it.value() == ending)
            return it.key();
    }
    auto newEntry = m_fileEndings.create();
    *newEntry.second = ending;
    return newEntry.first;
}

quint32 Library::commit(const LibraryChange &change, quint32 *createdId)
{
    CommittedLibraryChange commit{change, 0};

#define requireThat(condition, message) \
    do { if (!(condition)) { qWarning() << (message); return 0; } } while (0)

#define fetchItem(Collection, Name, id) \
    auto *Name = Collection.findItem(id); requireThat(Name, #Name " not found");

    switch (change.changeType) {
    case Moosick::LibraryChange::SongAdd: {
        fetchItem(m_albums, album, change.subject);

        auto song = m_songs.create();
        song.second->name = change.name;
        song.second->album = change.subject;
        album->songs << song.first;
        if (createdId)
            *createdId = song.first;
        commit.change.detail = song.first;
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
    case Moosick::LibraryChange::SongSetPosition: {
        fetchItem(m_songs, song, change.subject);

        song->position = change.detail;
        break;
    }
    case Moosick::LibraryChange::SongSetLength: {
        fetchItem(m_songs, song, change.subject);

        song->secs = change.detail;
        break;
    }
    case Moosick::LibraryChange::SongSetFileEnding: {
        fetchItem(m_songs, song, change.subject);

        song->fileEnding = getFileEnding(change.name);
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
        if (createdId)
            *createdId = album.first;
        commit.change.detail = album.first;
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
        if (createdId)
            *createdId = artist.first;
        commit.change.detail = artist.first;

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
        else
            m_rootTags << tag.first;

        if (createdId)
            *createdId = tag.first;
        commit.change.detail = tag.first;

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
        } else {
            Q_ASSERT(m_rootTags.contains(change.subject));
            m_rootTags.removeAll(change.subject);
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
        requireThat(change.detail != tag->parent, "Parent is the same");
        requireThat(change.subject != change.detail, "Can't be your own parent");

        auto oldParent = m_tags.findItem(tag->parent);
        auto newParent = m_tags.findItem(change.detail);

        Q_ASSERT(oldParent || (tag->parent == 0));
        Q_ASSERT(newParent || (change.detail == 0));

        if (tag->parent == 0) {
            Q_ASSERT(!oldParent);
            Q_ASSERT(m_rootTags.contains(change.subject));
            m_rootTags.removeAll(change.subject);
        }
        else {
            Q_ASSERT(oldParent);
            Q_ASSERT(oldParent->children.contains(change.subject));
            oldParent->children.removeAll(change.subject);
        }

        if (change.detail == 0) {
            Q_ASSERT(!newParent);
            m_rootTags << change.subject;
        } else {
            Q_ASSERT(newParent);
            newParent->children << change.subject;
        }

        tag->parent = change.detail;

        break;
    }
    case Moosick::LibraryChange::Invalid:
    default: {
        return 0;
    }
    }

#undef requireThat
#undef fetchItem

    m_revision += 1;
    commit.revision = m_revision;
    m_committedChanges << commit;

    return m_revision;
}

void Library::commit(const QVector<CommittedLibraryChange> &changes)
{
    for (const CommittedLibraryChange &change : changes) {
        if (change.revision > m_revision) {
            const quint32 expectedRevision = commit(change.change);
            Q_ASSERT(expectedRevision == change.revision);
        }
    }
}

QVector<CommittedLibraryChange> Library::committedChangesSince(quint32 revision) const
{
    QVector<CommittedLibraryChange> ret;
    ret.reserve(m_committedChanges.size());

    for (const CommittedLibraryChange &committed : m_committedChanges) {
        if (committed.revision >= revision)
            ret << committed;
    }

    return ret;
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

} // namespace Moosick
