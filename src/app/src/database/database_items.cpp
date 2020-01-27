#include "database_items.hpp"
#include "database_interface.hpp"
#include "database.hpp"

namespace Database {

DbItem::DbItem(DatabaseInterface *db, DbItem::Type tp, quint32 id)
    : QObject(db)
    , m_database(db)
    , m_type(tp)
    , m_id(id)
{
}

const Moosick::Library &DbItem::library() const
{
    return m_database->library();
}

DbTag::DbTag(DatabaseInterface *db, Moosick::TagId tag)
    : DbItem(db, DbItem::Tag, tag)
    , m_tag(tag)
{
    m_childTags.addValueAccessor("tag");
}

DbTaggedItem::DbTaggedItem(DatabaseInterface *db, DbItem::Type tp, quint32 id, const Moosick::TagIdList &tags)
    : DbItem(db, tp, id)
{
    m_tags.addValueAccessor("tag");
    updateTags(tags);
}

void DbTaggedItem::updateTags(const Moosick::TagIdList &tags)
{
    m_tags.clear();
    for (Moosick::TagId tagId : tags) {
        DbTag *tag = database()->tagForTagId(tagId);
        m_tags.add(tag);
        connect(tag, &QObject::destroyed, this, [=]() { m_tags.remove(tag); });
    }
}

Moosick::TagIdList DbTaggedItem::tagIds() const
{
    Moosick::TagIdList ret;
    for (DbTag *tag : m_tags.data())
        ret << tag->id();
    return ret;
}

DbArtist::DbArtist(DatabaseInterface *db, Moosick::ArtistId artist)
    : DbTaggedItem(db, DbItem::Artist, artist, artist.tags(db->library()))
    , m_artist(artist)
{
    m_albums.addValueAccessor("album");
    connect(this, &DbTaggedItem::libraryChanged, this, [=]() {
        updateTags(m_artist.tags(library()));
    });
}

DbArtist::~DbArtist()
{
    qDeleteAll(m_albums.data());
}

void DbArtist::removeAlbum(DbAlbum *album)
{
    m_albums.remove(album);
    album->deleteLater();
}

DbAlbum::DbAlbum(DatabaseInterface *db, Moosick::AlbumId album)
    : DbTaggedItem(db, DbItem::Album, album, album.tags(db->library()))
    , m_album(album)
{
    m_songs.addValueAccessor("song");
    connect(this, &DbTaggedItem::libraryChanged, this, [=]() {
        updateTags(m_album.tags(library()));
        emit songsChanged();
    });
}

DbAlbum::~DbAlbum()
{
    qDeleteAll(m_songs.data());
}

QString DbAlbum::durationString() const
{
    const QVector<DbSong*> &songs = m_songs.data();
    const int secs = std::accumulate(songs.begin(), songs.end(), 0, [&](int sum, DbSong *song) {
        return sum + song->secs();
    });
    return QString::asprintf("%d:%02d", secs/60, secs%60);
}

void DbAlbum::setSongs(const Moosick::SongIdList &songs)
{
    for (DbSong *song : m_songs.data())
        song->deleteLater();
    m_songs.clear();

    QVector<DbSong*> newSongs;
    for (const Moosick::SongId &songId : songs)
        newSongs << new DbSong(database(), songId);
    qSort(newSongs.begin(), newSongs.end(), [=](DbSong *lhs, DbSong *rhs) {
        return lhs->position() < rhs->position();
    });
    for (DbSong *song : newSongs)
        m_songs.addExclusive(song);

    emit songsChanged();
}

void DbAlbum::addSong(DbSong *song)
{
    m_songs.addExclusive(song);
    emit songsChanged();
}

void DbAlbum::removeSong(DbSong *song)
{
    m_songs.remove(song);
    emit songsChanged();
}

DbSong::DbSong(DatabaseInterface *db, Moosick::SongId song)
    : DbTaggedItem(db, DbItem::Song, song, song.tags(db->library()))
    , m_song(song)
{
    connect(this, &DbTaggedItem::libraryChanged, this, [=]() {
        updateTags(m_song.tags(library()));
    });
}

QString DbSong::artistName() const
{
    const Moosick::Library &lib = library();
    return m_song.album(lib).artist(lib).name(lib);
}

QString DbSong::albumName() const
{
    const Moosick::Library &lib = library();
    return m_song.album(lib).name(lib);
}

QString DbSong::durationString() const
{
    const uint secs = m_song.secs(library());
    return QString::asprintf("%d:%02d", secs/60, secs%60);
}

} //namespace Database
