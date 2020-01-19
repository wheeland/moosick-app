#pragma once

#include <QNetworkAccessManager>

#include "httpclient.hpp"
#include "library.hpp"
#include "util/modeladapter.hpp"

namespace Database {

class Database;
class DbItem;
class DbTag;
class DbArtist;
class DbAlbum;
class DbSong;

class DbItem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Type type READ getType CONSTANT)

public:
    enum Type {
        Tag,
        Artist,
        Album,
        Song
    };

    DbItem(Database *db, Type tp);
    ~DbItem() override = default;

    Type getType() const { return m_type; }

protected:
    Database *database() const { return m_database; }
    const Moosick::Library &library() const;

private:
    Database *m_database;
    const Type m_type;
};

class DbTag : public DbItem
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(ModelAdapter::Model *childTags READ childTagsModel CONSTANT)

public:
    DbTag(Database *db, Moosick::TagId tag);
    ~DbTag() override = default;

    void addChildTag(DbTag *child) { m_childTags.addExclusive(child); }
    void removeChildTag(DbTag *child) { m_childTags.remove(child); }

    QString name() const { return m_tag.name(library()); }
    QVector<DbTag*> childTags() const { return m_childTags.data(); }
    ModelAdapter::Model *childTagsModel() const { return m_childTags.model(); }

private:
    Moosick::TagId m_tag;
    ModelAdapter::Adapter<DbTag*> m_childTags;
};

class DbTaggedItem : public DbItem
{
    Q_OBJECT
    Q_PROPERTY(ModelAdapter::Model *tags READ tagsModel CONSTANT)

public:
    DbTaggedItem(Database *db, DbItem::Type tp, const Moosick::TagIdList &tags);
    ~DbTaggedItem() = default;

    void addTag(DbTag *tag) { m_tags.addExclusive(tag); }
    void removeTag(DbTag *tag) { m_tags.remove(tag); }

    QVector<DbTag*> tags() const { return m_tags.data(); }
    ModelAdapter::Model *tagsModel() const { return m_tags.model(); }

private:
    ModelAdapter::Adapter<DbTag*> m_tags;
};

class DbArtist : public DbTaggedItem
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(ModelAdapter::Model *albums READ albumsModel NOTIFY albumsChanged)

public:
    DbArtist(Database *db, Moosick::ArtistId artist);
    ~DbArtist();

    QString name() const { return m_artist.name(library()); }

    void addAlbum(DbAlbum *album) { m_albums.addExclusive(album); }
    void removeAlbum(DbAlbum *album) { m_albums.remove(album); }

    QVector<DbAlbum*> albums() const { return m_albums.data(); }
    ModelAdapter::Model *albumsModel() const { return m_albums.model(); }

signals:
    void albumsChanged(ModelAdapter::Model * albums);

private:
    Moosick::ArtistId m_artist;
    ModelAdapter::Adapter<DbAlbum*> m_albums;
};

class DbAlbum : public DbTaggedItem
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(ModelAdapter::Model *songs READ songsModel NOTIFY songsChanged)

public:
    DbAlbum(Database *db, Moosick::AlbumId album);
    ~DbAlbum();

    QString name() const { return m_album.name(library()); }

    void addSong(DbSong *song) { m_songs.addExclusive(song); }
    void removeSong(DbSong *song) { m_songs.remove(song); }

    QVector<DbSong*> songs() const { return m_songs.data(); }
    ModelAdapter::Model *songsModel() const { return m_songs.model(); }

signals:
    void songsChanged(ModelAdapter::Model * songs);

private:
    Moosick::AlbumId m_album;
    ModelAdapter::Adapter<DbSong*> m_songs;
};

class DbSong : public DbTaggedItem
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(int secs READ secs CONSTANT)
    Q_PROPERTY(int position READ position CONSTANT)

public:
    DbSong(Database *db, Moosick::SongId song);
    ~DbSong() = default;

    QString name() const { return m_song.name(library()); }
    int secs() const { return m_song.secs(library()); }
    int position() const { return m_song.position(library()); }

private:
    Moosick::SongId m_song;
};


/*

  what are the tasks of the database?

  - sync with remote DB
  - request changes (downloads, renames, etc.)
  - expose to QML:
    - search for stuff (appears in searches!)
    - browse by tags, names
    - browse tags


*/

class Database : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hasLibrary READ hasLibrary NOTIFY hasLibraryChanged)
    Q_PROPERTY(ModelAdapter::Model *rootTags READ rootTagsModel CONSTANT)
    Q_PROPERTY(ModelAdapter::Model *searchResults READ searchResults CONSTANT)
    Q_PROPERTY(QString searchString READ searchString NOTIFY searchStringChanged)

public:
    Database(HttpClient *httpClient, QObject *parent = nullptr);
    ~Database() override;

    const Moosick::Library &library() const { return m_library; }
    ModelAdapter::Model *rootTagsModel() const { return m_rootTags.model(); }
    ModelAdapter::Model *searchResults() const { return m_searchResults.model(); }

    bool hasLibrary() const { return m_hasLibrary; }
    QString searchString() const { return m_searchString; }

    Q_INVOKABLE void sync();
    Q_INVOKABLE void search(QString searchString);
    Q_INVOKABLE void clickOnArtist(DbArtist *artist);

private slots:
    void onNetworkReplyFinished(QNetworkReply *reply, QNetworkReply::NetworkError error);

signals:
    void hasLibraryChanged();
    void searchStringChanged(QString searchString);

private:
    friend class DbTaggedItem;

    enum RequestType {
        None,
        LibrarySync,
    };

    void onNewLibrary();

    DbTag *tagForTagId(Moosick::TagId tagId) const;
    DbTag *addTag(Moosick::TagId tagId);
    void removeTag(Moosick::TagId tagId);

    void clearSearchResults();
    void repopulateSearchResults();

    bool hasRunningRequestType(RequestType requestType) const;

    bool m_hasLibrary = false;
    Moosick::Library m_library;

    HttpRequester *m_http;
    QHash<QNetworkReply*, RequestType> m_requests;

    QHash<Moosick::TagId::IntType, DbTag*> m_tags;  // instantiations for all tag IDs
    ModelAdapter::Adapter<DbTag*> m_rootTags;       // view on root tags only

    struct SearchResultAlbum {
        Moosick::AlbumId albumId;
        Moosick::SongIdList songs;
    };

    struct SearchResultArtist {
        DbArtist *artist;
        QVector<SearchResultAlbum> albums;
    };

    QString m_searchString;
    ModelAdapter::Adapter<SearchResultArtist> m_searchResults;
};

} // namespace Database
