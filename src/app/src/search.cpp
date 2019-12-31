#include "search.hpp"


namespace Search {

Result::Result(Type tp, const QString &title, const QString &url, const QString &icon, QObject *parent)
    : QObject(parent)
    , m_title(title)
    , m_type(tp)
    , m_url(url)
    , m_iconUrl(icon)
{
}

BandcampArtistResult::BandcampArtistResult(const QString &title, const QString &url, const QString &icon, QObject *parent)
    : Result(BandcampArtist, title, url, icon, parent)
{
}

BandcampArtistResult::~BandcampArtistResult()
{
    for (BandcampAlbumResult *album : qAsConst(m_albums))
        album->deleteLater();
}

int BandcampArtistResult::albumCount() const
{
    return m_albums.size();
}

void BandcampArtistResult::addAlbum(BandcampAlbumResult *album)
{
    if (album) {
        m_albums << album;
        emit albumCountChanged(m_albums.size());
    }
}

BandcampAlbumResult::BandcampAlbumResult(const QString &title, const QString &url, const QString &icon, QObject *parent)
    : Result(BandcampAlbum, title, url, icon, parent)
{
}

BandcampAlbumResult::~BandcampAlbumResult()
{
    for (BandcampTrackResult *track : qAsConst(m_tracks))
        track->deleteLater();
}

int BandcampAlbumResult::trackCount() const
{
    return m_tracks.size();
}

void BandcampAlbumResult::addTrack(BandcampTrackResult *track)
{
    if (track) {
        m_tracks << track;
        emit trackCountChanged(m_tracks.size());
    }
}

BandcampTrackResult::BandcampTrackResult(const QString &title, const QString &url, const QString &icon, QObject *parent)
    : Result(BandcampTrack, title, url, icon, parent)
{
}

QueryFilterModel::QueryFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

bool QueryFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    Q_UNUSED(sourceParent)

    if (!m_source)
        return true;

    const Result *result = m_source->m_results.value(sourceRow);
    return result && (result->resultType() == m_filterType);
}

void QueryFilterModel::setFilterType(Result::Type type)
{
    if (m_filterType == type)
        return;

    m_filterType = type;
    emit filterTypeChanged(m_filterType);
}

void QueryFilterModel::setSource(Query *source)
{
    if (m_source == source)
        return;

    m_source = source;
    emit sourceChanged(m_source);
}

} // namespace Search
