#include "database.hpp"

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

DbAlbum::DbAlbum(Database *db, Moosick::AlbumId album)
    : DbTaggedItem(db, DbItem::Album, album.tags(db->library()))
    , m_album(album)
{
    m_songs.addValueAccessor("song");
}

DbSong::DbSong(Database *db, Moosick::SongId song)
    : DbTaggedItem(db, DbItem::Song, song.tags(db->library()))
{
}

Database::Database(HttpClient *httpClient, QObject *parent)
    : QObject(parent)
    , m_http(new HttpRequester(httpClient, this))
{
    connect(m_http, &HttpRequester::receivedReply, this, &Database::onNetworkReplyFinished);
}

Database::~Database()
{
}

void Database::sync()
{
    if (hasRunningRequestType(LibrarySync))
        return;

    QNetworkReply *reply = m_http->requestFromServer("/lib.do", "");
    m_requests[reply] = LibrarySync;
}

void Database::onNetworkReplyFinished(QNetworkReply *reply, QNetworkReply::NetworkError error)
{
    const RequestType requestType = m_requests.take(reply);
    const QByteArray data = reply->readAll();

    switch (requestType) {
    case LibrarySync:
        m_library.deserialize(QByteArray::fromBase64(data));
        qWarning().noquote() << m_library.dumpToStringList();
        break;
    default:
        break;
    }
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

bool Database::hasRunningRequestType(RequestType requestType) const
{
    return std::any_of(m_requests.begin(), m_requests.end(), [=](RequestType tp) {
        return tp == requestType;
    });
}

} // namespace Database
