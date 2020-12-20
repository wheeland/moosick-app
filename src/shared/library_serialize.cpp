#include "library.hpp"
#include "jsonconv.hpp"

#include <QDebug>
#include <QRandomGenerator>

#define JSON_REQUIRE_VALUE(NAME, OBJ, TAG, TYPE) \
    const QJsonValue NAME##_jsonvalue = OBJ.value(TAG); \
    if (NAME##_jsonvalue.isUndefined()) { qWarning() << "Not found in JSON:" << TAG; return false; } \
    if (NAME##_jsonvalue.type() != TYPE) { qWarning() << "Invalid type in JSON:" << TAG; return false; } \

#define JSON_REQUIRE_INT(NAME, OBJ, TAG) \
    JSON_REQUIRE_VALUE(NAME, OBJ, TAG, QJsonValue::Double); \
    const int NAME = (int) NAME##_jsonvalue.toDouble();

#define JSON_REQUIRE_STRING(NAME, OBJ, TAG) \
    JSON_REQUIRE_VALUE(NAME, OBJ, TAG, QJsonValue::String); \
    const QString NAME = NAME##_jsonvalue.toString();

QJsonValue toJson(const Moosick::LibraryChange &change)
{
    QJsonObject ret;
    ret["type"] = toJson((int) change.changeType);
    ret["subject"] = toJson((int) change.subject);
    ret["detail"] = toJson((int) change.detail);
    ret["name"] = toJson(change.name);
    return ret;
}

bool fromJson(const QJsonValue &json, Moosick::LibraryChange &change)
{
    if (json.type() != QJsonValue::Object)
        return false;

    const QJsonObject obj = json.toObject();

    JSON_REQUIRE_INT(jsTp, obj, "type");
    JSON_REQUIRE_INT(jsSubject, obj, "subject");
    JSON_REQUIRE_INT(jsDetail, obj, "detail");
    JSON_REQUIRE_STRING(jsName, obj, "name");

    change.changeType = static_cast<Moosick::LibraryChange::Type>(jsTp);
    change.subject = jsSubject;
    change.detail = jsDetail;
    change.name = jsName;
    return true;
}

QJsonValue toJson(const Moosick::CommittedLibraryChange &change)
{
    QJsonObject obj = toJson(change.change).toObject();
    obj["revision"] = (int) change.revision;
    return obj;
}

bool fromJson(const QJsonValue &json, Moosick::CommittedLibraryChange &change)
{
    if (json.type() != QJsonValue::Object)
        return false;

    Moosick::LibraryChange ch;
    if (!fromJson(json, ch))
        return false;

    const QJsonObject obj = json.toObject();
    JSON_REQUIRE_INT(rev, obj, "revision");

    change.change = ch;
    change.revision = rev;
    return true;
}

bool fromJson(const QJsonValue &json, Moosick::TagId &tag)
{
    int id;
    if (!fromJson(json, id)) return false;
    tag = id;
    return true;
}

QJsonValue toJson(Moosick::TagId tag)
{
    return toJson((int) tag);
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

template <class T, class IntType>
static QJsonArray collectionToJsonArray(const ItemCollection<T, IntType> &col, const std::function<void(QJsonObject&, const T &)> &dumper)
{
    QJsonArray array;
    for (auto it = col.begin(), end = col.end(); it != end; ++it) {
        QJsonObject newObj;
        newObj["id"] = QJsonValue((qint64) it.key());
        dumper(newObj, it.value());
        array << newObj;
    }
    return array;
}

QJsonObject Library::serializeToJson() const
{
    QJsonObject json;

    json["revision"] = QJsonValue((int) m_revision);
    json["id"] = QJsonValue(QString(m_id.toString()));

    json["tags"] = collectionToJsonArray<Library::Tag>(m_tags, [&](QJsonObject &json, const Library::Tag &tag) {
        json["name"] = QJsonValue(tag.name);
        json["parent"] = QJsonValue((int) tag.parent);
    });

    json["fileEndings"] = collectionToJsonArray<QString>(m_fileEndings, [&](QJsonObject &json, const QString &fileEnding) {
        json["ending"] = QJsonValue(fileEnding);
    });

    json["artists"] = collectionToJsonArray<Library::Artist>(m_artists, [&](QJsonObject &json, const Library::Artist &artist) {
        json["name"] = QJsonValue(artist.name);
        json["tags"] = toJson(artist.tags);
    });

    json["albums"] = collectionToJsonArray<Library::Album>(m_albums, [&](QJsonObject &json, const Library::Album &album) {
        json["name"] = QJsonValue(album.name);
        json["artist"] = QJsonValue((qint64) album.artist);
        json["tags"] = toJson(album.tags);
    });

    json["songs"] = collectionToJsonArray<Library::Song>(m_songs, [&](QJsonObject &json, const Library::Song &song) {
        json["name"] = QJsonValue(song.name);
        json["album"] = QJsonValue((qint64) song.album);
        json["fileEnding"] = QJsonValue((qint64) song.fileEnding);
        json["position"] = QJsonValue((int) song.position);
        json["secs"] = QJsonValue((int) song.secs);
        json["tags"] = toJson(song.tags);
    });

    return json;
}

template <class T, class IntType>
static bool readJsonCollection(const QJsonValue &val, ItemCollection<T, IntType> &col, const std::function<bool(const QJsonObject &, IntType id, T &)> &reader)
{
    if (val.type() != QJsonValue::Array)
        return false;

    ItemCollection<T, IntType> ret;
    const QJsonArray array = val.toArray();

    for (const QJsonValue &value : array) {
        if (value.type() != QJsonValue::Object)
            return false;

        const QJsonObject obj = value.toObject();
        JSON_REQUIRE_INT(id, obj, "id");

        T newItem;
        if (!reader(obj, id, newItem))
            return false;

        ret.add(id, newItem);
    }

    col = ret;
    return true;
}

bool Library::deserializeFromJson(const QJsonObject &libraryJson, const QJsonArray &logJson)
{
    JSON_REQUIRE_INT(revision, libraryJson, "revision");
    JSON_REQUIRE_STRING(id, libraryJson, "id");

    if (!m_id.fromString(id.toLocal8Bit())) {
        qWarning() << "Failed to deserialize Library: invalid ID";
        return false;
    }

    ItemCollection<Song> songs;
    ItemCollection<Album> albums;
    ItemCollection<Artist> artists;
    ItemCollection<Tag> tags;
    ItemCollection<QString> fileEndings;
    QVector<TagId> rootTags;
    bool success = true;

    success &= readJsonCollection<Library::Tag, quint32>(libraryJson["tags"], tags, [&](const QJsonObject &obj, quint32, Tag &tag) {
        JSON_REQUIRE_STRING(name, obj, "name");
        JSON_REQUIRE_INT(parent, obj, "parent");
        tag.name = name;
        tag.parent = parent;
        return true;
    });

#define TAG_PUSH_ID(tagId, member, id) \
    do { \
        Library::Tag *tag = tags.findItem(tagId); \
        if (tag) tag->member << (id); \
    } while (0)

#define TAG_PUSH_IDS(NEWTAGS, MEMBER, ID) \
    do { \
        for (quint32 tagId : NEWTAGS)  \
            TAG_PUSH_ID(tagId, MEMBER, ID); \
    } while (0)

#define JSON_REQUIRE_TAGS(NAME, OBJ) \
    TagIdList NAME; \
    if (!fromJson(OBJ["tags"], NAME)) return false; \

    // fix up tag child lists and fill root parent list
    for (auto it = tags.begin(); it != tags.end(); ++it) {
        TAG_PUSH_ID(it.value().parent, children, it.key());
        if (it.value().parent == 0)
            rootTags << it.key();
    }

    success &= readJsonCollection<QString, quint32>(libraryJson["fileEndings"], fileEndings,
            [&](const QJsonObject &obj, quint32, QString &ending) {
        JSON_REQUIRE_STRING(name, obj, "ending");
        ending = name;
        return true;
    });

    // read artists
    success &= readJsonCollection<Library::Artist, quint32>(libraryJson["artists"], artists,
            [&](const QJsonObject &obj, quint32 id, Library::Artist &artist) {
        JSON_REQUIRE_STRING(name, obj, "name");
        JSON_REQUIRE_TAGS(aTags, obj);
        artist.name = name;
        artist.tags = aTags;
        TAG_PUSH_IDS(artist.tags, artists, id);
        return true;
    });

    // read albums
    success &= readJsonCollection<Library::Album, quint32>(libraryJson["albums"], albums,
            [&](const QJsonObject &obj, quint32 id, Library::Album &album) {
        JSON_REQUIRE_STRING(name, obj, "name");
        JSON_REQUIRE_INT(artist, obj, "artist");
        JSON_REQUIRE_TAGS(aTags, obj);
        album.name = name;
        album.artist = artist;
        album.tags = aTags;
        TAG_PUSH_IDS(album.tags, albums, id);
        if (Library::Artist *artist = artists.findItem(album.artist))
            artist->albums << id;
        return true;
    });

    // read songs
    success &= readJsonCollection<Library::Song, quint32>(libraryJson["songs"], songs,
            [&](const QJsonObject &obj, quint32 id, Library::Song &song) {
        JSON_REQUIRE_STRING(name, obj, "name");
        JSON_REQUIRE_INT(album, obj, "album");
        JSON_REQUIRE_INT(ending, obj, "fileEnding");
        JSON_REQUIRE_INT(pos, obj, "position");
        JSON_REQUIRE_INT(secs, obj, "secs");
        JSON_REQUIRE_TAGS(aTags, obj);
        song.name = name;
        song.album = album;
        song.fileEnding = ending;
        song.position = pos;
        song.secs = secs;
        song.tags = aTags;
        TAG_PUSH_IDS(song.tags, songs, id);
        if (Library::Album *album = albums.findItem(song.album))
            album->songs << id;
        return true;
    });

#undef TAG_PUSH_ID
#undef TAG_PUSH_IDS

    // Read history of committed changes
    QVector<CommittedLibraryChange> committedChanges;
    for (int i = 0; i < logJson.size(); ++i) {
        CommittedLibraryChange change;
        if (!fromJson(logJson[i], change)) {
            qWarning() << "Failed to deserialize Library: invalid log entry";
            success = false;
            break;
        }
        committedChanges << change;
    }

    if (!success)
        return false;

    m_revision = revision;
    m_tags = tags;
    m_songs = songs;
    m_albums = albums;
    m_artists = artists;
    m_fileEndings = fileEndings;
    m_rootTags = rootTags;
    m_committedChanges = committedChanges;

    qDebug() << "Deserialized Library:";
    qDebug() << ".." << m_artists.size() << "artists";
    qDebug() << ".." << m_albums.size() << "albums";
    qDebug() << ".." << m_songs.size() << "songs";
    qDebug() << ".." << m_tags.size() << "tags";
    qDebug() << ".." << m_committedChanges.size() << "committed change log entries";

    return true;
}

static QString readUtf8(QDataStream &stream)
{
    QByteArray bytes;
    stream >> bytes;
    return QString::fromUtf8(bytes);
}

QDataStream &operator<<(QDataStream &stream, const LibraryChange &lch)
{
    stream << static_cast<quint32>(lch.changeType);
    stream << lch.subject;
    stream << lch.detail;
    stream << lch.name.toUtf8();
    return stream;
}

QDataStream &operator>>(QDataStream &stream, LibraryChange &lch)
{
    quint32 tp;
    stream >> tp;
    lch.changeType = static_cast<LibraryChange::Type>(tp);
    stream >> lch.subject;
    stream >> lch.detail;
    lch.name = readUtf8(stream);
    return stream;
}

template <class T, class IntType>
static void dumpCollection(QDataStream &stream, const ItemCollection<T, IntType> &col, const std::function<void(const T&)> &dumper)
{
    stream << col.size();
    for (auto it = col.begin(), end = col.end(); it != end; ++it) {
        stream << it.key();
        dumper(it.value());
    }
}

QDataStream &operator<<(QDataStream &stream, const Library &lib)
{
    stream << lib.m_revision;
    stream << lib.m_id.toString();

    dumpCollection<Library::Tag>(stream, lib.m_tags, [&](const Library::Tag &tag) {
        stream << tag.name.toUtf8();
        stream << tag.parent;
    });

    dumpCollection<QString>(stream, lib.m_fileEndings, [&](const QString &fileEnding) {
        stream << fileEnding.toUtf8();
    });

    dumpCollection<Library::Artist>(stream, lib.m_artists, [&](const Library::Artist &artist) {
        stream << artist.name.toUtf8();
        stream << artist.tags;
    });

    dumpCollection<Library::Album>(stream, lib.m_albums, [&](const Library::Album &album) {
        stream << album.name.toUtf8();
        stream << album.artist;
        stream << album.tags;
    });

    dumpCollection<Library::Song>(stream, lib.m_songs, [&](const Library::Song &song) {
        stream << song.name.toUtf8();
        stream << song.album;
        stream << song.fileEnding;
        stream << song.position;
        stream << song.secs;
        stream << song.tags;
    });

    return stream;
}

template <class T, class IntType>
static void readCollection(QDataStream &stream, ItemCollection<T, IntType> &col, const std::function<T(IntType)> &reader)
{
    quint32 sz;
    stream >> sz;

    for (quint32 i = 0; i < sz; ++i) {
        IntType id;
        stream >> id;

        const T newItem = reader(id);
        col.add(id, newItem);
    }
}

template <class IntType>
QDataStream &operator>>(QDataStream &stream, detail::FromInt<IntType> &dst) {
    quint32 into;
    stream >> into;
    dst = into;
    return stream;
}

QDataStream &operator>>(QDataStream &stream, Library &lib)
{
    lib.m_tags.clear();
    lib.m_songs.clear();
    lib.m_albums.clear();
    lib.m_artists.clear();
    lib.m_fileEndings.clear();

    stream >> lib.m_revision;

    QByteArray id;
    stream >> id;
    const bool success = lib.m_id.fromString(id);
    Q_ASSERT(success);

    readCollection<Library::Tag, quint32>(stream, lib.m_tags, [&](quint32) {
        Library::Tag newTag;
        newTag.name = readUtf8(stream);
        stream >> newTag.parent;
        return newTag;
    });

#define TAG_PUSH_ID(tagId, member, id) \
    do { \
        Library::Tag *tag = lib.m_tags.findItem(tagId); \
        if (tag) tag->member << (id); \
    } while (0)

    // fix up tag child lists and fill root parent list
    for (auto it = lib.m_tags.begin(); it != lib.m_tags.end(); ++it) {
        TAG_PUSH_ID(it.value().parent, children, it.key());
        if (it.value().parent == 0)
            lib.m_rootTags << it.key();
    }

    readCollection<QString, quint32>(stream, lib.m_fileEndings, [&](quint32) {
        return readUtf8(stream);
    });

    // read artists
    readCollection<Library::Artist, quint32>(stream, lib.m_artists, [&](quint32 id) {
        Library::Artist newArtist;
        newArtist.name = readUtf8(stream);
        stream >> newArtist.tags;
        for (quint32 tagId : newArtist.tags)
            TAG_PUSH_ID(tagId, artists, id);
        return newArtist;
    });

    // read albums
    readCollection<Library::Album, quint32>(stream, lib.m_albums, [&](quint32 id) {
        Library::Album newAlbum;
        newAlbum.name = readUtf8(stream);
        stream >> newAlbum.artist;
        stream >> newAlbum.tags;
        for (quint32 tagId : newAlbum.tags)
            TAG_PUSH_ID(tagId, albums, id);
        if (Library::Artist *artist = lib.m_artists.findItem(newAlbum.artist))
            artist->albums << id;
        return newAlbum;
    });

    // read songs
    readCollection<Library::Song, quint32>(stream, lib.m_songs, [&](quint32 id) {
        Library::Song newSong;
        newSong.name = readUtf8(stream);
        stream >> newSong.album;
        stream >> newSong.fileEnding;
        stream >> newSong.position;
        stream >> newSong.secs;
        stream >> newSong.tags;
        for (quint32 tagId : newSong.tags)
            TAG_PUSH_ID(tagId, songs, id);
        if (Library::Album *album = lib.m_albums.findItem(newSong.album))
            album->songs << id;
        return newSong;
    });

#undef TAG_PUSH_ID

    return stream;
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
