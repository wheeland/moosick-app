#include "library.hpp"
#include "jsonconv.hpp"

#include <QDebug>
#include <QRandomGenerator>

namespace Moosick {

QJsonValue enjson(const Moosick::LibraryChangeRequest &change)
{
    QJsonObject ret;
    ret["type"] = ::enjson((int) change.changeType);
    ret["targetId"] = ::enjson((int) change.targetId);
    ret["detail"] = ::enjson((int) change.detail);
    ret["name"] = ::enjson(change.name);
    return ret;
}

void dejson(const QJsonValue &json, Result<Moosick::LibraryChangeRequest, JsonifyError> &result)
{
    JSONIFY_DEJSON_EXPECT_TYPE(json, result, Object);
    JSONIFY_DEJSON_GET_MEMBER(json, result, int, tp, "type");
    JSONIFY_DEJSON_GET_MEMBER(json, result, int, targetId, "targetId");
    JSONIFY_DEJSON_GET_MEMBER(json, result, int, detail, "detail");
    JSONIFY_DEJSON_GET_MEMBER(json, result, QString, name, "name");

    Moosick::LibraryChangeRequest change;
    change.changeType = static_cast<Moosick::LibraryChangeRequest::Type>(tp);
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

void dejson(const QJsonValue &json, Result<Moosick::CommittedLibraryChange, JsonifyError> &result)
{
    JSONIFY_DEJSON_EXPECT_TYPE(json, result, Object);

    Result<Moosick::LibraryChangeRequest, JsonifyError> change = dejson<Moosick::LibraryChangeRequest>(json);
    if (change.hasError())
    {
        result = change.takeError();
        return;
    }

    JSONIFY_DEJSON_GET_MEMBER(json, result, int, committedRevision, "committedRevision");
    JSONIFY_DEJSON_GET_MEMBER(json, result, int, createdId, "createdId");

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

LibraryId LibraryId::generate()
{
    LibraryId ret;
    for (int i = 0; i < LENGTH; ++i)
        ret.m_bytes[i] = QRandomGenerator::global()->generate();
    return ret;
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

QByteArray LibraryId::toString() const
{
    QByteArray ret;
    for (quint8 byte : m_bytes) {
        ret += toDigit(byte / 16);
        ret += toDigit(byte % 16);
    }
    return ret;
}

bool LibraryId::fromString(const QByteArray &string)
{
    if (string.size() != 2 * LENGTH)
        return false;

    for (int i = 0; i < LENGTH; ++i) {
        quint8 high, low;
        if (!fromDigit(string[2*i], high) || !fromDigit(string[2*i+1], low))
            return false;
        m_bytes[i] = 16 * high + low;
    }
    return true;
}

bool LibraryId::operator==(const LibraryId &other) const
{
    for (int i = 0; i < LENGTH; ++i) {
        if (m_bytes[i] != other.m_bytes[i])
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

void dejson(const QJsonValue &json, Result<Library::Song, JsonifyError> &result)
{
    JSONIFY_DEJSON_GET_MEMBER(json, result, QString, name, "name");
    JSONIFY_DEJSON_GET_MEMBER(json, result, qint64, album, "album");
    JSONIFY_DEJSON_GET_MEMBER(json, result, qint64, fileEnding, "fileEnding");
    JSONIFY_DEJSON_GET_MEMBER(json, result, int, position, "position");
    JSONIFY_DEJSON_GET_MEMBER(json, result, int, secs, "secs");
    JSONIFY_DEJSON_GET_MEMBER(json, result, TagIdList, tags, "tags");
    Library::Song song;
    song.name = name;
    song.album = album;
    song.fileEnding = fileEnding;
    song.position = position;
    song.secs = secs;
    song.tags = tags;
    result = song;
}

void dejson(const QJsonValue &json, Result<Library::Album, JsonifyError> &result)
{
    JSONIFY_DEJSON_GET_MEMBER(json, result, QString, name, "name");
    JSONIFY_DEJSON_GET_MEMBER(json, result, qint64, artist, "artist");
    JSONIFY_DEJSON_GET_MEMBER(json, result, TagIdList, tags, "tags");
    Library::Album album;
    album.name = name;
    album.artist = artist;
    album.tags = tags;
    result = album;
}

void dejson(const QJsonValue &json, Result<Library::Artist, JsonifyError> &result)
{
    JSONIFY_DEJSON_GET_MEMBER(json, result, QString, name, "name");
    JSONIFY_DEJSON_GET_MEMBER(json, result, TagIdList, tags, "tags");
    Library::Artist artist;
    artist.name = name;
    artist.tags = tags;
    result = artist;
}

void dejson(const QJsonValue &json, Result<Library::Tag, JsonifyError> &result)
{
    JSONIFY_DEJSON_GET_MEMBER(json, result, QString, name, "name");
    JSONIFY_DEJSON_GET_MEMBER(json, result, int, parent, "parent");
    Library::Tag tag;
    tag.name = name;
    tag.parent = parent;
    result = tag;
}

QJsonObject Library::serializeToJson() const
{
    QJsonObject json;

    json["revision"] = ::enjson(m_revision);
    json["id"] = enjson(QString::fromUtf8(m_id.toString()));
    json["tags"] = enjson(m_tags);
    json["artists"] = enjson(m_artists);
    json["albums"] = enjson(m_albums);
    json["songs"] = enjson(m_songs);
    json["fileEndings"] = enjson(m_fileEndings);

    return json;
}

void Library::deserializeFromJsonInternal(const QJsonObject &libraryJson, const QJsonArray &committedChanges, Result<int, JsonifyError> &result)
{
    JSONIFY_DEJSON_GET_MEMBER(libraryJson, result, quint32, revision, "revision");
    JSONIFY_DEJSON_GET_MEMBER(libraryJson, result, QString, id, "id");
    JSONIFY_DEJSON_GET_MEMBER(libraryJson, result, ItemCollection<Tag>, tags, "tags");
    JSONIFY_DEJSON_GET_MEMBER(libraryJson, result, ItemCollection<Artist>, artists, "artists");
    JSONIFY_DEJSON_GET_MEMBER(libraryJson, result, ItemCollection<Album>, albums, "albums");
    JSONIFY_DEJSON_GET_MEMBER(libraryJson, result, ItemCollection<Song>, songs, "songs");
    JSONIFY_DEJSON_GET_MEMBER(libraryJson, result, ItemCollection<QString>, fileEndings, "fileEndings");

    auto changes = dejson<QVector<CommittedLibraryChange>>(committedChanges);
    if (changes.hasError()) {
        result = changes.takeError();
        return;
    }

    if (!m_id.fromString(id.toUtf8())) {
        result = JsonifyError::buildCustomError("'id' doesn't contain a valid ID");
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

JsonifyError Library::deserializeFromJson(const QJsonObject &libraryJson, const QJsonArray &committedChanges)
{
    Result<int, JsonifyError> result;
    deserializeFromJsonInternal(libraryJson, committedChanges, result);
    return result.hasError() ? result.getError() : JsonifyError();
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
                const QString filePath = song.filePath(*this);
                const quint32 secs = song.secs(*this);
                const QString fileInfo = QString::asprintf(" (%s - %d:%02d)", qPrintable(filePath), secs/60, secs%60);
                ret << QString("     |    |-- ") + pos + song.name(*this) + fileInfo + infoStr(song, song.tags(*this));
            }
        }
    }

    return ret;
}

} // namespace Moosick
