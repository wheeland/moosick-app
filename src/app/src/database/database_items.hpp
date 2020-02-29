#pragma once

#include "library.hpp"
#include "../util/modeladapter.hpp"

namespace Database {

class DatabaseInterface;
class DbItem;
class DbTag;
class DbArtist;
class DbAlbum;
class DbSong;

class DbItem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name NOTIFY libraryChanged)
    Q_PROPERTY(Type type READ getType CONSTANT)

public:
    enum Type {
        Tag,
        Artist,
        Album,
        Song
    };

    DbItem(DatabaseInterface *db, Type tp, quint32 id);
    ~DbItem() override = default;

    virtual QString name() const = 0;

    Type getType() const { return m_type; }
    quint32 id() const { return m_id; }

protected:
    DatabaseInterface *database() const { return m_database; }
    const Moosick::Library &library() const;

signals:
    void libraryChanged();

private:
    DatabaseInterface * const m_database;
    const Type m_type;
    const quint32 m_id;
};

class DbTag : public DbItem
{
    Q_OBJECT
    Q_PROPERTY(ModelAdapter::Model *childTags READ childTagsModel CONSTANT)
    Q_PROPERTY(DbTag *parentTag READ parentTag CONSTANT)

public:
    DbTag(DatabaseInterface *db, Moosick::TagId tag);
    ~DbTag() override = default;

    void addChildTag(DbTag *child) { m_childTags.addExclusive(child); }
    void removeChildTag(DbTag *child) { m_childTags.remove(child); }

    Moosick::TagId id() const { return m_tag; }
    QString name() const override { return m_tag.name(library()); }
    DbTag *parentTag() const { return m_parentTag; }
    QVector<DbTag*> childTags() const { return m_childTags.data(); }
    ModelAdapter::Model *childTagsModel() const { return m_childTags.model(); }

    void setParentTag(DbTag *parentTag) { m_parentTag = parentTag; }

private:
    Moosick::TagId m_tag;
    DbTag *m_parentTag = nullptr;
    ModelAdapter::Adapter<DbTag*> m_childTags;
};

class DbTaggedItem : public DbItem
{
    Q_OBJECT
    Q_PROPERTY(ModelAdapter::Model *tags READ tagsModel CONSTANT)

public:
    DbTaggedItem(DatabaseInterface *db, DbItem::Type tp, quint32 id, const Moosick::TagIdList &tags);
    ~DbTaggedItem() = default;

    QVector<DbTag*> tags() const { return m_tags.data(); }
    Moosick::TagIdList tagIds() const;
    ModelAdapter::Model *tagsModel() const { return m_tags.model(); }

protected slots:
    void updateTags(const Moosick::TagIdList &tags);

private:
    ModelAdapter::Adapter<DbTag*> m_tags;
};

class DbArtist : public DbTaggedItem
{
    Q_OBJECT
    Q_PROPERTY(ModelAdapter::Model *albums READ albumsModel NOTIFY albumsChanged)

public:
    DbArtist(DatabaseInterface *db, Moosick::ArtistId artist);
    ~DbArtist();

    QString name() const override { return m_artist.name(library()); }

    void addAlbum(DbAlbum *album) { m_albums.addExclusive(album); }
    void removeAlbum(DbAlbum *album);

    QVector<DbAlbum*> albums() const { return m_albums.data(); }
    ModelAdapter::Model *albumsModel() const { return m_albums.model(); }

    bool hasData() const { return m_hasData; }
    void dataSet() { m_hasData = true; }

signals:
    void albumsChanged(ModelAdapter::Model * albums);

private:
    Moosick::ArtistId m_artist;
    bool m_hasData = false;
    ModelAdapter::Adapter<DbAlbum*> m_albums;
};

class DbAlbum : public DbTaggedItem
{
    Q_OBJECT
    Q_PROPERTY(QString durationString READ durationString NOTIFY songsChanged)
    Q_PROPERTY(ModelAdapter::Model *songs READ songsModel NOTIFY songsChanged)

public:
    DbAlbum(DatabaseInterface *db, Moosick::AlbumId album, DbArtist *artist);
    ~DbAlbum();

    DbArtist *artist() const { return m_artist; }

    QString name() const override { return m_album.name(library()); }
    QString durationString() const;

    void setSongs(const Moosick::SongIdList &songs);
    void addSong(DbSong *song);
    void removeSong(DbSong *song);

    QVector<DbSong*> songs() const { return m_songs.data(); }
    ModelAdapter::Model *songsModel() const { return m_songs.model(); }

signals:
    void songsChanged();

private:
    Moosick::AlbumId m_album;
    ModelAdapter::Adapter<DbSong*> m_songs;
    DbArtist *m_artist;
};

class DbSong : public DbTaggedItem
{
    Q_OBJECT
    Q_PROPERTY(QString durationString READ durationString NOTIFY libraryChanged)
    Q_PROPERTY(int position READ position NOTIFY libraryChanged)

public:
    DbSong(DatabaseInterface *db, Moosick::SongId song, DbAlbum *album);
    ~DbSong() = default;

    DbAlbum *album() const { return m_album; }

    QString name() const override { return m_song.name(library()); }
    QString artistName() const;
    QString albumName() const;
    QString durationString() const;
    QString filePath() const { return m_song.filePath(library()); }
    int secs() const { return m_song.secs(library()); }
    int position() const { return m_song.position(library()); }

private:
    Moosick::SongId m_song;
    DbAlbum *m_album;
};

} // namespace Database
