#include "library.hpp"
#include "jsonconv.hpp"

#include <QDebug>
#include <QRandomGenerator>

namespace Moosick {

QString LibraryChangeRequest::typeToStr(LibraryChangeRequest::Type tp)
{
#define HANDLE_CASE(TYPE) case TYPE: return #TYPE;
    switch (tp) {
    HANDLE_CASE(Invalid)
    HANDLE_CASE(SongAdd)
    HANDLE_CASE(SongRemove)
    HANDLE_CASE(SongSetName)
    HANDLE_CASE(SongSetPosition)
    HANDLE_CASE(SongSetLength)
    HANDLE_CASE(SongSetFileEnding)
    HANDLE_CASE(SongSetHandle)
    HANDLE_CASE(SongSetAlbum)
    HANDLE_CASE(SongAddTag)
    HANDLE_CASE(SongRemoveTag)
    HANDLE_CASE(AlbumAdd)
    HANDLE_CASE(AlbumRemove)
    HANDLE_CASE(AlbumSetName)
    HANDLE_CASE(AlbumSetArtist)
    HANDLE_CASE(AlbumAddTag)
    HANDLE_CASE(AlbumRemoveTag)
    HANDLE_CASE(ArtistAdd)
    HANDLE_CASE(ArtistAddOrGet)
    HANDLE_CASE(ArtistRemove)
    HANDLE_CASE(ArtistSetName)
    HANDLE_CASE(ArtistAddTag)
    HANDLE_CASE(ArtistRemoveTag)
    HANDLE_CASE(TagAdd)
    HANDLE_CASE(TagRemove)
    HANDLE_CASE(TagSetName)
    HANDLE_CASE(TagSetParent)
    }
    qCritical() << "Invalid LibraryChangeRequest type:" << tp;
    return "";
#undef HANDLE_CASE
}

bool LibraryChangeRequest::typeFromStr(const QString &str, LibraryChangeRequest::Type &tp)
{
#define HANDLE_CASE(TYPE) if (str == #TYPE) { tp = TYPE; return true; }
    HANDLE_CASE(SongAdd)
    HANDLE_CASE(SongRemove)
    HANDLE_CASE(SongSetName)
    HANDLE_CASE(SongSetPosition)
    HANDLE_CASE(SongSetLength)
    HANDLE_CASE(SongSetFileEnding)
    HANDLE_CASE(SongSetHandle)
    HANDLE_CASE(SongSetAlbum)
    HANDLE_CASE(SongAddTag)
    HANDLE_CASE(SongRemoveTag)
    HANDLE_CASE(AlbumAdd)
    HANDLE_CASE(AlbumRemove)
    HANDLE_CASE(AlbumSetName)
    HANDLE_CASE(AlbumSetArtist)
    HANDLE_CASE(AlbumAddTag)
    HANDLE_CASE(AlbumRemoveTag)
    HANDLE_CASE(ArtistAdd)
    HANDLE_CASE(ArtistAddOrGet)
    HANDLE_CASE(ArtistRemove)
    HANDLE_CASE(ArtistSetName)
    HANDLE_CASE(ArtistAddTag)
    HANDLE_CASE(ArtistRemoveTag)
    HANDLE_CASE(TagAdd)
    HANDLE_CASE(TagRemove)
    HANDLE_CASE(TagSetName)
    HANDLE_CASE(TagSetParent)
#undef HANDLE_CASE
    return false;
}

QJsonValue enjson(const Moosick::LibraryChangeRequest &change)
{
    QJsonObject ret;
    ret["type"] = ::enjson(Moosick::LibraryChangeRequest::typeToStr(change.changeType));
    ret["targetId"] = ::enjson((int) change.targetId);
    ret["detail"] = ::enjson((int) change.detail);
    ret["name"] = ::enjson(change.name);
    return ret;
}

void dejson(const QJsonValue &json, Result<Moosick::LibraryChangeRequest, EnjsonError> &result)
{
    DEJSON_EXPECT_TYPE(json, result, Object);
    DEJSON_GET_MEMBER(json, result, QString, tp, "type");
    DEJSON_GET_MEMBER(json, result, int, targetId, "targetId");
    DEJSON_GET_MEMBER(json, result, int, detail, "detail");
    DEJSON_GET_MEMBER(json, result, QString, name, "name");

    Moosick::LibraryChangeRequest change;
    if (!Moosick::LibraryChangeRequest::typeFromStr(tp, change.changeType)) {
        result = EnjsonError::buildCustomError("Invalid type value: " + tp);
        return;
    }
    change.targetId = targetId;
    change.detail = detail;
    change.name = name;
    result = change;
}

QJsonValue enjson(const Moosick::CommittedLibraryChange &change)
{
    QJsonObject obj = enjson(change.changeRequest).toObject();
    obj["committedRevision"] = (int) change.committedRevision;
    obj["createdId"] = (int) change.createdId;
    return obj;
}

void dejson(const QJsonValue &json, Result<Moosick::CommittedLibraryChange, EnjsonError> &result)
{
    DEJSON_EXPECT_TYPE(json, result, Object);

    Result<Moosick::LibraryChangeRequest, EnjsonError> change = dejson<Moosick::LibraryChangeRequest>(json);
    if (change.hasError())
    {
        result = change.takeError();
        return;
    }

    DEJSON_GET_MEMBER(json, result, int, committedRevision, "committedRevision");
    DEJSON_GET_MEMBER(json, result, int, createdId, "createdId");

    Moosick::CommittedLibraryChange ret;
    ret.changeRequest = change.takeValue();
    ret.committedRevision = committedRevision;
    ret.createdId = createdId;
    result = ret;
}

}

QJsonValue enjson(Moosick::TagId tag)
{
    return enjson((int) tag);
}

namespace Moosick {

void UniqueIdBase::generate(quint8 *dst, quint32 length)
{
    for (quint32 i = 0; i < length; ++i)
        dst[i] = QRandomGenerator::global()->generate();
}

static char toDigit(const quint8 n)
{
    Q_ASSERT(n < 16);
    return (n < 10) ? ('0' + n) : ('a' + (n-10));
}

static bool fromDigit(const char c, quint8 &num)
{
    if (c >= '0' && c <= '9')
        num = c - '0';
    else if (c >= 'a' && c <= 'f')
        num = 10 + (c - 'a');
    else if (c >= 'A' && c <= 'F')
        num = 10 + (c - 'A');
    else
        return false;
    return true;
}

QByteArray UniqueIdBase::toString(const quint8 *bytes, quint32 length)
{
    QByteArray ret;
    for (quint32 i = 0; i < length; ++i) {
        ret += toDigit(bytes[i] / 16);
        ret += toDigit(bytes[i] % 16);
    }
    return ret;
}

bool UniqueIdBase::fromString(const QByteArray &src, quint8 *dst, quint32 length)
{
    if ((quint32) src.size() != 2 * length)
        return false;

    for (quint32 i = 0; i < length; ++i) {
        quint8 high, low;
        if (!fromDigit(src[2*i], high) || !fromDigit(src[2*i+1], low))
            return false;
        dst[i] = 16 * high + low;
    }
    return true;
}

bool UniqueIdBase::compare(const quint8 *a, const quint8 *b, quint32 length)
{
    for (quint32 i = 0; i < length; ++i) {
        if (a[i] != b[i])
            return false;
    }
    return true;
}

QJsonValue enjson(const Library::Song &song)
{
    QJsonObject json;
    json["name"] = QJsonValue(song.name);
    json["album"] = QJsonValue((qint64) song.album);
    json["fileEnding"] = QJsonValue((qint64) song.fileEnding);
    json["position"] = QJsonValue((int) song.position);
    json["secs"] = QJsonValue((int) song.secs);
    json["tags"] = enjson(song.tags);
    json["handle"] = QJsonValue(QString::fromUtf8(song.handle.toString()));
    return json;
}

QJsonValue enjson(const Library::Album &album)
{
    QJsonObject json;
    json["name"] = QJsonValue(album.name);
    json["artist"] = QJsonValue((qint64) album.artist);
    json["tags"] = enjson(album.tags);
    return json;
}

QJsonValue enjson(const Library::Artist &artist)
{
    QJsonObject json;
    json["name"] = QJsonValue(artist.name);
    json["tags"] = enjson(artist.tags);
    return json;
}

QJsonValue enjson(const Library::Tag &tag)
{
    QJsonObject json;
    json["name"] = QJsonValue(tag.name);
    json["parent"] = QJsonValue((int) tag.parent);
    return json;
}

void dejson(const QJsonValue &json, Result<Library::Song, EnjsonError> &result)
{
    DEJSON_GET_MEMBER(json, result, QString, name, "name");
    DEJSON_GET_MEMBER(json, result, qint64, album, "album");
    DEJSON_GET_MEMBER(json, result, qint64, fileEnding, "fileEnding");
    DEJSON_GET_MEMBER(json, result, int, position, "position");
    DEJSON_GET_MEMBER(json, result, int, secs, "secs");
    DEJSON_GET_MEMBER(json, result, TagIdList, tags, "tags");
    DEJSON_GET_MEMBER(json, result, QString, handleString, "handle");
    Library::Song song;
    song.name = name;
    song.album = album;
    song.fileEnding = fileEnding;
    song.position = position;
    song.secs = secs;
    song.tags = tags;
    if (!song.handle.fromString(handleString.toUtf8())) {
        result = EnjsonError::buildCustomError("invalid song handle");
        return;
    }
    result = song;
}

void dejson(const QJsonValue &json, Result<Library::Album, EnjsonError> &result)
{
    DEJSON_GET_MEMBER(json, result, QString, name, "name");
    DEJSON_GET_MEMBER(json, result, qint64, artist, "artist");
    DEJSON_GET_MEMBER(json, result, TagIdList, tags, "tags");
    Library::Album album;
    album.name = name;
    album.artist = artist;
    album.tags = tags;
    result = album;
}

void dejson(const QJsonValue &json, Result<Library::Artist, EnjsonError> &result)
{
    DEJSON_GET_MEMBER(json, result, QString, name, "name");
    DEJSON_GET_MEMBER(json, result, TagIdList, tags, "tags");
    Library::Artist artist;
    artist.name = name;
    artist.tags = tags;
    result = artist;
}

void dejson(const QJsonValue &json, Result<Library::Tag, EnjsonError> &result)
{
    DEJSON_GET_MEMBER(json, result, QString, name, "name");
    DEJSON_GET_MEMBER(json, result, int, parent, "parent");
    Library::Tag tag;
    tag.name = name;
    tag.parent = parent;
    result = tag;
}

SerializedLibrary Library::serializeToJson() const
{
    QJsonObject json;

    json["revision"] = ::enjson(m_revision);
    json["id"] = enjson(QString::fromUtf8(m_id.toString()));
    json["tags"] = enjson(m_tags);
    json["artists"] = enjson(m_artists);
    json["albums"] = enjson(m_albums);
    json["songs"] = enjson(m_songs);
    json["fileEndings"] = enjson(m_fileEndings);

    SerializedLibrary ret;
    ret.libraryJson = json;
    ret.version = 1;

    return ret;
}

void Library::deserializeFromJsonInternal(const QJsonObject &libraryJson, const QJsonArray &committedChanges, Result<int, EnjsonError> &result)
{
    DEJSON_GET_MEMBER(libraryJson, result, quint32, revision, "revision");
    DEJSON_GET_MEMBER(libraryJson, result, QString, id, "id");
    DEJSON_GET_MEMBER(libraryJson, result, ItemCollection<Tag>, tags, "tags");
    DEJSON_GET_MEMBER(libraryJson, result, ItemCollection<Artist>, artists, "artists");
    DEJSON_GET_MEMBER(libraryJson, result, ItemCollection<Album>, albums, "albums");
    DEJSON_GET_MEMBER(libraryJson, result, ItemCollection<Song>, songs, "songs");
    DEJSON_GET_MEMBER(libraryJson, result, ItemCollection<QString>, fileEndings, "fileEndings");

    auto changes = dejson<QVector<CommittedLibraryChange>>(committedChanges);
    if (changes.hasError()) {
        result = changes.takeError();
        return;
    }

    if (!m_id.fromString(id.toUtf8())) {
        result = EnjsonError::buildCustomError("'id' doesn't contain a valid ID");
        return;
    }

    m_revision = revision;
    m_tags = tags;
    m_artists = artists;
    m_albums = albums;
    m_songs = songs;
    m_fileEndings = fileEndings;
    m_committedChanges = changes.takeValue();

    #define TAG_PUSH_ID(TAG, MEMBER, ID) do { \
        Library::Tag *tag = tags.findItem(TAG); \
        if (tag) tag->MEMBER << (ID); \
    } while (0)

    #define TAG_PUSH_IDS(TAGS, MEMBER, ID) do { \
        for (quint32 tagId : TAGS)  \
            TAG_PUSH_ID(tagId, MEMBER, ID); \
    } while (0)

    // Fill tag parent/child relationships
    for (auto it = m_tags.begin(); it != m_tags.end(); ++it) {
        TAG_PUSH_ID(it.value().parent, children, it.key());
        if (it.value().parent == 0)
            m_rootTags << it.key();
    }

    // Fill relationships for artists/albums/songs
    for (auto it = m_artists.begin(); it != m_artists.end(); ++it) {
        TAG_PUSH_IDS(it->tags, artists, it.key());
    }
    for (auto it = m_albums.begin(); it != m_albums.end(); ++it) {
        TAG_PUSH_IDS(it->tags, albums, it.key());
        if (Artist *artist = m_artists.findItem(it->artist))
            artist->albums << it.key();
    }
    for (auto it = m_songs.begin(); it != m_songs.end(); ++it) {
        TAG_PUSH_IDS(it->tags, songs, it.key());
        if (Album *album = m_albums.findItem(it->album))
            album->songs << it.key();
    }

    #undef TAG_PUSH_IDS
    #undef TAG_PUSH_ID

    result = 0;
}

EnjsonError Library::deserializeFromJson(const SerializedLibrary &libraryJson, const QJsonArray &committedChanges)
{
    Result<int, EnjsonError> result;
    deserializeFromJsonInternal(libraryJson.libraryJson, committedChanges, result);
    return result.hasError() ? result.getError() : EnjsonError();
}

QStringList Library::dumpToStringList() const
{
    QStringList ret;

    const auto infoStr = [&](quint32 id, const TagIdList &tags) {
        QString ret(" (id=");
        ret += QString::number(id);
        ret += ", tags=[";
        for (int i = 0; i < tags.size(); ++i) {
            if (i > 0)
                ret += ", ";
            ret += tags[i].name(*this);
        }
        ret += "])";
        return ret;
    };

    // dump tags
    ret << "Tags:";
    const std::function<void(TagId, const QString&)> dumpTag = [&](TagId tag, const QString &indent) {
        ret << (indent + tag.name(*this) + "(id=" + QString::number(tag) + ")");
        QString childIndent = indent + " |-- ";
        for (TagId child : tag.children(*this))
            dumpTag(child, childIndent);
    };

    for (TagId tagId : m_rootTags) {
        dumpTag(tagId, "    ");
    }

    // dump artists/albums/songs
    ret << "Artists:";
    for (ArtistId artist : artistsByName()) {
        ret << QString("    ") + artist.name(*this) + infoStr(artist, artist.tags(*this));

        for (AlbumId album : artist.albums(*this)) {
            ret << QString("     |-- ") + album.name(*this) + infoStr(album, album.tags(*this));

            for (SongId song : album.songs(*this)) {
                const QString pos = QString::asprintf("[%2d] ", song.position(*this));
                const QString filePath = song.fileName(*this);
                const quint32 secs = song.secs(*this);
                const QString fileInfo = QString::asprintf(" (%s - %d:%02d)", qPrintable(filePath), secs/60, secs%60);
                ret << QString("     |    |-- ") + pos + song.name(*this) + fileInfo + infoStr(song, song.tags(*this));
            }
        }
    }

    return ret;
}

} // namespace Moosick
