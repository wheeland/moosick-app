#include "database.hpp"
#include "database_items.hpp"
#include "jsonconv.hpp"

// let the Database be consistent and do ALL allocations
// the QObjects are just exposure to QML, all the data lies in the Database
// when tags change, all the classes need to get them anew from the DB

namespace Database {

DbItem::DbItem(Database *db, DbItem::Type tp)
    : QObject(db)
    , m_database(db)
    , m_type(tp)
{
}

const Moosick::Library &DbItem::library() const
{
    return m_database->library();
}

DbTag::DbTag(Database *db, Moosick::TagId tag)
    : DbItem(db, DbItem::Tag)
    , m_tag(tag)
{
    m_childTags.addValueAccessor("tag");
}

DbTaggedItem::DbTaggedItem(Database *db, DbItem::Type tp, const Moosick::TagIdList &tags)
    : DbItem(db, tp)
{
    m_tags.addValueAccessor("tag");
    for (Moosick::TagId tagId : tags) {
        DbTag *tag = db->tagForTagId(tagId);
        m_tags.add(tag);
        connect(tag, &QObject::destroyed, this, [=]() { removeTag(tag); });
    }
}

DbArtist::DbArtist(Database *db, Moosick::ArtistId artist)
    : DbTaggedItem(db, DbItem::Artist, artist.tags(db->library()))
    , m_artist(artist)
{
    m_albums.addValueAccessor("album");
}

DbArtist::~DbArtist()
{
    qDeleteAll(m_albums.data());
}

DbAlbum::DbAlbum(Database *db, Moosick::AlbumId album)
    : DbTaggedItem(db, DbItem::Album, album.tags(db->library()))
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
    : DbTaggedItem(db, DbItem::Song, song.tags(db->library()))
    , m_song(song)
{
}

QString DbSong::durationString() const
{
    const int secs = m_song.secs(library());
    return QString::asprintf("%d:%02d", secs/60, secs%60);
}

Database::Database(HttpClient *httpClient, QObject *parent)
    : QObject(parent)
    , m_http(new HttpRequester(httpClient, this))
    , m_artistNames(new StringModel(this))
{
    connect(m_http, &HttpRequester::receivedReply, this, &Database::onNetworkReplyFinished);

    m_rootTags.addValueAccessor("tag");
    m_searchResults.addAccessor("artist", [&](const SearchResultArtist &artist) {
        return QVariant::fromValue(artist.artist);
    });
}

Database::~Database()
{
}

void Database::sync()
{
    if (hasRunningRequestType(LibrarySync) || m_hasLibrary)
        return;

    QNetworkReply *reply = m_http->requestFromServer("/lib.do", "");
    m_requests[reply] = LibrarySync;
}

void Database::onNetworkReplyFinished(QNetworkReply *reply, QNetworkReply::NetworkError error)
{
    const RequestType requestType = m_requests.take(reply);
    const QByteArray data = reply->readAll();

    switch (requestType) {
    case LibrarySync: {
        const QJsonObject obj = parseJsonObject(data, "Library");
        if (m_library.deserializeFromJson(obj))
            onNewLibrary();
        break;
    }
    default:
        break;
    }
}

void Database::onNewLibrary()
{
    // add tags
    for (const Moosick::TagId tagId : m_library.rootTags())
        addTag(tagId);

    repopulateSearchResults();

    for (const Moosick::ArtistId &artistId : m_library.artistsByName()) {
        m_artistNames->add((int) artistId, artistId.name(m_library));
    }

    m_hasLibrary = true;
    emit hasLibraryChanged();
}

DbTag *Database::tagForTagId(Moosick::TagId tagId) const
{
    return m_tags.value(tagId, nullptr);
}

DbTag *Database::addTag(Moosick::TagId tagId)
{
    Q_ASSERT(tagId.isValid());

    auto it = m_tags.find(tagId);

    if (it == m_tags.end()) {
        it = m_tags.insert(tagId, new DbTag(this, tagId));

        // link to parent
        const Moosick::TagId parentId = tagId.parent(m_library);
        if (parentId.isValid()) {
            DbTag *parent = addTag(parentId);
            parent->addChildTag(it.value());
        }

        // link to children
        for (const Moosick::TagId childId : tagId.children(m_library)) {
            DbTag *child = addTag(childId);
            it.value()->addChildTag(child);
        }

        if (!parentId.isValid())
            m_rootTags.addExclusive(it.value());
    }

    return it.value();
}

void Database::removeTag(Moosick::TagId tagId)
{
    Q_ASSERT(tagId.isValid());

    const auto it = m_tags.find(tagId);
    if (it == m_tags.end())
        return;

    // remove from parent
    DbTag *parent = tagForTagId(tagId.parent(m_library));
    if (parent)
        parent->removeChildTag(it.value());

    // remove children
    for (const Moosick::TagId childId : tagId.children(m_library))
        removeTag(childId);

    m_tags.erase(it);
}

void Database::search(QString searchString)
{
    if (m_searchString == searchString)
        return;

    repopulateSearchResults();

    m_searchString = searchString;
    emit searchStringChanged(m_searchString);
}

void Database::fillArtistInfo(DbArtist *artist)
{
    if (!artist->albums().isEmpty())
        return;

    const QVector<SearchResultArtist> artists = m_searchResults.data();
    auto it = std::find_if(artists.begin(), artists.end(), [=](const SearchResultArtist &sra) {
        return sra.artist == artist;
    });

    Q_ASSERT(it != artists.end());
    Q_ASSERT(it->artist == artist);

    // populate artist with album/songs objects
    for (const SearchResultAlbum &albumResult : it->albums) {
        DbAlbum *album = new DbAlbum(this, albumResult.albumId);

        QVector<DbSong*> songs;
        for (const Moosick::SongId songId : albumResult.songs)
            songs << new DbSong(this, songId);
        qSort(songs.begin(), songs.end(), [=](DbSong *lhs, DbSong *rhs) {
            return lhs->position() < rhs->position();
        });
        for (DbSong *song : songs)
            album->addSong(song);

        artist->addAlbum(album);
    }
}

void Database::clearSearchResults()
{
    for (const SearchResultArtist &artist : m_searchResults.data()) {
        if (artist.artist)
            artist.artist->deleteLater();
    }
    m_searchResults.clear();
}

void Database::repopulateSearchResults()
{
    clearSearchResults();

    QStringList keywords;
    for (const QString searchWord : m_searchString.split(' '))
        keywords << searchWord.toLower();

    // see if we match the search keywords
    const auto matchesSearchString = [&](const QString &name) {
        if (m_searchString.isEmpty())
            return true;

        const QString lower = name.toLower();
        return std::all_of(keywords.cbegin(), keywords.cend(), [&](const QString &keyword) {
            return lower.contains(keyword);
        });
    };

    for (const Moosick::ArtistId &artistId : m_library.artistsByName()) {
        const bool matches = matchesSearchString(artistId.name(m_library));

        // build search result item
        if (matches) {
            SearchResultArtist artist;
            artist.artist = new DbArtist(this, artistId);
            for (const Moosick::AlbumId albumId : artistId.albums(m_library))
                artist.albums << SearchResultAlbum { albumId, albumId.songs(m_library) };
            m_searchResults.add(artist);
        }
    }
}

bool Database::hasRunningRequestType(RequestType requestType) const
{
    return std::any_of(m_requests.begin(), m_requests.end(), [=](RequestType tp) {
        return tp == requestType;
    });
}

} // namespace Database
