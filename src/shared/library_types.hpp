#pragma once

#include <QtGlobal>
#include <QVector>
#include <QHash>

#include "jsonconv.hpp"

namespace Moosick {

class Library;

namespace detail {

template <class Int>
class FromInt
{
public:
    FromInt() {}
    FromInt(Int value) : m_value(value) {}
    FromInt(const FromInt &) = default;

    operator Int() const { return m_value; }

    bool isValid() const { return m_value > 0; }

    using IntType = Int;

protected:
    IntType m_value;
};

using FromU8 = FromInt<quint8>;
using FromU16 = FromInt<quint16>;
using FromU32 = FromInt<quint32>;

} // namespace detail

struct SongId;
struct AlbumId;
struct ArtistId;
struct TagId;

using SongIdList = QVector<SongId>;
using AlbumIdList = QVector<AlbumId>;
using TagIdList = QVector<TagId>;
using ArtistIdList = QVector<ArtistId>;

struct SongId : public detail::FromU32
{
    using detail::FromU32::FromU32;

    bool exists(const Library &library) const;
    AlbumId album(const Library &library) const;
    ArtistId artist(const Library &library) const;
    TagIdList tags(const Library &library) const;

    QString name(const Library &library) const;
    quint32 position(const Library &library) const;
    quint32 secs(const Library &library) const;
    QString filePath(const Library &library) const;
};

struct AlbumId : public detail::FromU32
{
    using detail::FromU32::FromU32;

    bool exists(const Library &library) const;
    ArtistId artist(const Library &library) const;
    SongIdList songs(const Library &library) const;
    TagIdList tags(const Library &library) const;

    QString name(const Library &library) const;
};

struct ArtistId : public detail::FromU32
{
    using detail::FromU32::FromU32;

    bool exists(const Library &library) const;
    AlbumIdList albums(const Library &library) const;
    TagIdList tags(const Library &library) const;

    QString name(const Library &library) const;
};

struct TagId : public detail::FromU32
{
    using detail::FromU32::FromU32;

    bool exists(const Library &library) const;
    TagId parent(const Library &library) const;
    TagIdList children(const Library &library) const;

    ArtistIdList artists(const Library &library) const;
    AlbumIdList albums(const Library &library) const;
    SongIdList songs(const Library &library) const;

    QString name(const Library &library) const;
};

ENJSON_DECLARE_ALIAS(SongId, quint32)
ENJSON_DECLARE_ALIAS(ArtistId, quint32)
ENJSON_DECLARE_ALIAS(AlbumId, quint32)
ENJSON_DECLARE_ALIAS(TagId, quint32)

template <class T, class IntType = quint32>
class ItemCollection : public QHash<IntType, T>
{
public:
    ItemCollection() {}
    ~ItemCollection() {}

    void add(IntType id, const T &value)
    {
        this->insert(id, value);
        m_nextId = qMax(m_nextId, id + 1);
    }

    T *findItem(IntType id)
    {
        const auto it = this->find(id);
        return (it != this->end()) ? (&it.value()) : nullptr;
    }

    const T *findItem(IntType id) const
    {
        const auto it = this->find(id);
        return (it != this->end()) ? (&it.value()) : nullptr;
    }

    QPair<IntType, T*> create()
    {
        const auto it = this->insert(m_nextId, T());
        m_nextId += 1;
        return qMakePair(m_nextId - 1, &it.value());
    }

    template <class IntLike>
    QVector<IntLike> ids() const
    {
        QVector<IntLike> ret;
        for (auto it = this->begin(); it != this->end(); ++it) {
            ret << it.key();
        }
        return ret;
    }

private:
    template <class TT, class II>
    friend QJsonValue enjson(const ItemCollection<TT, II> &collection);

    template <class TT, class II>
    friend void dejson(const QJsonValue &json, Result<ItemCollection<TT, II>, EnjsonError> &result);

    IntType m_nextId = 1;
};

template <class T, class IntType>
QJsonValue enjson(const ItemCollection<T, IntType> &collection)
{
    QJsonArray entries;
    for (auto it = collection.begin(); it != collection.end(); ++it)
        entries.append(enjson(qMakePair(it.key(), it.value())));

    QJsonObject json;
    json["nextId"] = ::enjson(collection.m_nextId);
    json["entries"] = entries;
    return json;
}

template <class T, class IntType>
void dejson(const QJsonValue &json, Result<ItemCollection<T, IntType>, EnjsonError> &result)
{
    using KeyValuePair = QPair<IntType, T>;
    DEJSON_GET_MEMBER(json, result, IntType, nextId, "nextId");
    DEJSON_GET_MEMBER(json, result, QVector<KeyValuePair>, entries, "entries");

    ItemCollection<T, IntType> ret;
    for (const KeyValuePair &kv : entries)
        ret[kv.first] = kv.second;
    ret.m_nextId = nextId;

    result = ret;
}

} // namespace Moosick
