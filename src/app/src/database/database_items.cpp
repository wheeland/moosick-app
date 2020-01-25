#include "database_items.hpp"
#include "database.hpp"

namespace Database {

DbItem::DbItem(Database *db, DbItem::Type tp, quint32 id)
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

DbTag::DbTag(Database *db, Moosick::TagId tag)
    : DbItem(db, DbItem::Tag, tag)
    , m_tag(tag)
{
    m_childTags.addValueAccessor("tag");
}

DbTaggedItem::DbTaggedItem(Database *db, DbItem::Type tp, quint32 id, const Moosick::TagIdList &tags)
    : DbItem(db, tp, id)
{
    m_tags.addValueAccessor("tag");
    for (Moosick::TagId tagId : tags) {
        DbTag *tag = db->tagForTagId(tagId);
        m_tags.add(tag);
        connect(tag, &QObject::destroyed, this, [=]() { removeTag(tag); });
    }
}

DbArtist::DbArtist(Database *db, Moosick::ArtistId artist)
    : DbTaggedItem(db, DbItem::Artist, artist, artist.tags(db->library()))
    , m_artist(artist)
{
    m_albums.addValueAccessor("album");
}

DbArtist::~DbArtist()
{
    qDeleteAll(m_albums.data());
}

DbAlbum::DbAlbum(Database *db, Moosick::AlbumId album)
    : DbTaggedItem(db, DbItem::Album, album, album.tags(db->library()))
    , m_album(album)
{
    m_songs.addValueAccessor("song");
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

DbSong::DbSong(Database *db, Moosick::SongId song)
    : DbTaggedItem(db, DbItem::Song, song, song.tags(db->library()))
    , m_song(song)
{
}

QString DbSong::durationString() const
{
    const uint secs = m_song.secs(library());
    return QString::asprintf("%d:%02d", secs/60, secs%60);
}

} //namespace Database
