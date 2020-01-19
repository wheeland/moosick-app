#pragma once

#include <QtGlobal>
#include <QVector>

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

    ArtistId artist(const Library &library) const;
    SongIdList songs(const Library &library) const;
    TagIdList tags(const Library &library) const;

    QString name(const Library &library) const;
};

struct ArtistId : public detail::FromU32
{
    using detail::FromU32::FromU32;

    AlbumIdList albums(const Library &library) const;
    TagIdList tags(const Library &library) const;

    QString name(const Library &library) const;
};

struct TagId : public detail::FromU32
{
    using detail::FromU32::FromU32;

    TagId parent(const Library &library) const;
    TagIdList children(const Library &library) const;

    QString name(const Library &library) const;
};

} // namespace Moosick
