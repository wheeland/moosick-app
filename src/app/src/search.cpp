#include "search.hpp"
#include "jsonconv.hpp"
#include "util/qmlutil.hpp"
#include "library_messages.hpp"

#include <QtGlobal>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QBuffer>
#include <QImage>
#include <QDebug>

#include "musicscrape/musicscrape.hpp"

namespace Search {

Result::Result(Type tp, const QString &title, const QString &url, const QString &icon, QObject *parent)
    : QObject(parent)
    , m_title(title)
    , m_type(tp)
    , m_url(url)
    , m_iconUrl(icon)
{
}

void Result::setIconData(const QByteArray &data)
{
    m_iconData = QmlUtil::imageDataToDataUri(data, m_iconUrl);
    emit iconDataChanged();
}

void Result::setStatus(Result::Status status)
{
    if (m_status != status) {
        m_status = status;
        emit statusChanged(status);
    }
}

void Result::setDownloadStatus(Result::DownloadStatus downloadStatus)
{
    if (m_downloadStatus != downloadStatus) {
        m_downloadStatus = downloadStatus;
        downloadStatusChanged(downloadStatus);
    }
}

void Result::queryInfo()
{
    if (m_status == InfoOnly || m_status == Error)
        emit queryInfoRequested();
}

BandcampArtistResult::BandcampArtistResult(const QString &title, const QString &url, const QString &icon, QObject *parent)
    : Result(BandcampArtist, title, url, icon, parent)
{
    m_albums.addValueAccessor("result");
}

BandcampArtistResult::~BandcampArtistResult()
{
    for (BandcampAlbumResult *album : qAsConst(m_albums.data()))
        album->deleteLater();
}

void BandcampArtistResult::addAlbum(BandcampAlbumResult *album)
{
    if (album)
        m_albums.add(album);
}

BandcampAlbumResult::BandcampAlbumResult(const QString &artist, const QString &title, const QString &url, const QString &icon, QObject *parent)
    : Result(BandcampAlbum, title, url, icon, parent)
    , m_artist(artist)
{
    m_tracks.addValueAccessor("result");
}

BandcampAlbumResult::~BandcampAlbumResult()
{
    for (BandcampTrackResult *track : qAsConst(m_tracks.data()))
        track->deleteLater();
}

void BandcampAlbumResult::addTrack(BandcampTrackResult *track)
{
    if (track)
        m_tracks.add(track);
}

BandcampTrackResult::BandcampTrackResult(BandcampAlbumResult *album, const QString &title, const QString &url, const QString &icon, int secs, QObject *parent)
    : Result(BandcampTrack, title, url, icon, parent)
    , m_secs(secs)
    , m_album(album)
{
}

YoutubeVideoResult::YoutubeVideoResult(const QString &title, const QString &videoUrl, const QString &icon, QObject *parent)
    : Result(YoutubeVideo, title, videoUrl, icon, parent)
{
}

void YoutubeVideoResult::setDetails(const QString &audioUrl, int secs)
{
    m_audioUrl = audioUrl;
    m_secs = secs;
    emit hasDetails();
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

    const QVariant data = m_source->data(m_source->index(sourceRow, 0), m_source->roleIndex("result"));
    const Result *result = data.value<Result*>();
    return result && (result->resultType() == m_filterType);
}

void QueryFilterModel::setFilterType(Result::Type type)
{
    if (m_filterType == type)
        return;

    m_filterType = type;
    emit filterTypeChanged(m_filterType);
}

void QueryFilterModel::setSource(ModelAdapter::Model *source)
{
    if (m_source == source)
        return;

    m_source = source;
    emit sourceChanged(m_source);
}

Query::Query(HttpClient *http, QObject *parent)
    : QObject(parent)
    , m_http(new HttpRequester(http, this))
{
    m_rootResults.addValueAccessor("result");
    connect(m_http, &HttpRequester::receivedReply, this, &Query::onReply);
    connect(m_http, &HttpRequester::networkError, this, &Query::onNetworkError);
}

Query::~Query()
{
    clear();
}

QAbstractItemModel *Query::model() const
{
    return m_rootResults.model();
}

bool Query::hasFinished() const
{
    return (m_rootResults.size() > 0) && !m_bandcampRootSearch && !m_youtubeRootSearch;
}

void Query::clear()
{
    m_bandcampRootSearch = HTTP_NULL_REQUEST;
    m_youtubeRootSearch = HTTP_NULL_REQUEST;
    m_bandcampArtistQueries.clear();
    m_bandcampAlbumQueries.clear();
    m_youtubeVideoQueries.clear();
    m_iconQueries.clear();

    m_http->abortAll();

    for (Result *result : m_rootResults.data())
        result->deleteLater();
    m_rootResults.clear();
}

void Query::search(const QString &searchString)
{
    clear();
    m_searchString = searchString.trimmed();
    startBandcampRootSearch();
    startYoutubeRootSearch();
}

void Query::startBandcampRootSearch()
{
    if (!m_bandcampRootSearch) {
        const std::string url = ScrapeBandcamp::searchUrl(m_searchString.toStdString());
        m_bandcampRootSearch = m_http->request(QNetworkRequest(QUrl(QString::fromStdString(url))));
    }
}

void Query::startYoutubeRootSearch()
{
    if (!m_youtubeRootSearch) {
        const std::string url = ScrapeYoutube::searchUrl(m_searchString.toStdString());
        m_youtubeRootSearch = m_http->request(QNetworkRequest(QUrl(QString::fromStdString(url))));
    }
}

void Query::populateBandcampSearchResults(const QByteArray &html)
{
    const ScrapeBandcamp::ResultList results = ScrapeBandcamp::searchResult(html.toStdString());
    for (const ScrapeBandcamp::Result &result : results) {
        switch (result.resultType) {
        case ScrapeBandcamp::Result::Band:
            m_rootResults.add(createBandcampArtistResult(result));
            break;
        case ScrapeBandcamp::Result::Album:
            m_rootResults.add(createBandcampAlbumResult(result));
            break;
        case ScrapeBandcamp::Result::Track:
            // ignore fucking tracks
            break;
        default:
            qFatal("Query::populateBandcampResults: Invalid ScrapeBandcamp::Result");
        }
    }
}

static void addTracksToBandcampAlbum(BandcampAlbumResult *album, const ScrapeBandcamp::ResultList &results)
{
    for (const ScrapeBandcamp::Result &result : results) {
        Q_ASSERT(result.resultType == ScrapeBandcamp::Result::Track);
        const QString name = QString::fromStdString(result.trackName);
        const QString url = QString::fromStdString(result.mp3url);
        const QString icon = QString::fromStdString(result.artUrl);
        BandcampTrackResult *track = new BandcampTrackResult(album, name, url, icon, result.mp3duration, album);
        album->addTrack(track);
    }
}

void Query::populateBandcampArtist(BandcampArtistResult *artist, const QByteArray &html)
{
    bool singleRelease;
    const std::string bandUrl = artist->url().toStdString();
    const ScrapeBandcamp::ResultList results = ScrapeBandcamp::bandInfoResult(bandUrl, html.toStdString(), &singleRelease);

    if (results.empty()) {
        artist->setStatus(Result::Error);
        return;
    }

    if (!singleRelease) {
        for (const ScrapeBandcamp::Result &result : results) {
            if (result.resultType == ScrapeBandcamp::Result::Album)
                artist->addAlbum(createBandcampAlbumResult(result));
        }
    }
    else {
        // create a fake 'Album' Result and create an album from it
        ScrapeBandcamp::Result fakeResult = results.front();
        fakeResult.resultType = ScrapeBandcamp::Result::Album;
        fakeResult.url = bandUrl;

        BandcampAlbumResult *album = createBandcampAlbumResult(results.front());
        artist->addAlbum(album);
        addTracksToBandcampAlbum(album, results);
        album->setStatus(Result::Done);
    }

    artist->setStatus(Result::Done);
}

void Query::populateBandcampAlbum(BandcampAlbumResult *album, const QByteArray &html)
{
    const ScrapeBandcamp::ResultList results = ScrapeBandcamp::albumInfo(html.toStdString());

    if (!results.empty()) {
        addTracksToBandcampAlbum(album, results);
        album->setStatus(Result::Done);
    } else {
        album->setStatus(Result::Error);
    }
}

void Query::populateYoutubeSearchResults(const QByteArray &html)
{
    const ScrapeYoutube::ResultList results = ScrapeYoutube::searchResult(html.toStdString());
    for (const ScrapeYoutube::Result &result : results) {
        m_rootResults.add(
            createYoutubeVideoResult(result.title.data(), result.url.data(), result.thumbnailUrl.data())
        );
    }
}

void Query::populateYoutubeVideo(YoutubeVideoResult *video, const QByteArray &json)
{
    auto result = MoosickMessage::Message::fromJson(json);
    if (result.hasError()) {
        qWarning().noquote() << "Failed to parse youtube results:" << result.takeError().toString();
        video->setStatus(Result::Error);
        return;
    }

    const MoosickMessage::YoutubeUrlResponse *response = result->as<MoosickMessage::YoutubeUrlResponse>();
    if (!response) {
        qWarning().noquote() << "Failed to parse youtube results, wrong message type" << result->getTypeString();
        video->setStatus(Result::Error);
        return;
    }

    video->setDetails(response->url, response->duration);
    video->setStatus(Result::Done);
}

void Query::onReply(HttpRequestId requestId, const QByteArray &data)
{
    if (m_bandcampRootSearch == requestId) {
        m_bandcampRootSearch = 0;
        populateBandcampSearchResults(data);
        emit finishedChanged();
    }

    else if (m_youtubeRootSearch == requestId) {
        m_youtubeRootSearch = 0;
        populateYoutubeSearchResults(data);
        emit finishedChanged();
    }
    else if (BandcampArtistResult *artist = m_bandcampArtistQueries.take(requestId)) {
        populateBandcampArtist(artist, data);
    }
    else if (BandcampAlbumResult *album = m_bandcampAlbumQueries.take(requestId)) {
        populateBandcampAlbum(album, data);
    }
    else if (YoutubeVideoResult *video = m_youtubeVideoQueries.take(requestId)) {
        populateYoutubeVideo(video, data);
    }
    else if (Result *result = m_iconQueries.take(requestId)) {
        result->setIconData(data);
    }
}

void Query::onNetworkError(HttpRequestId requestId, QNetworkReply::NetworkError error)
{
    if (m_bandcampRootSearch == requestId) {
        m_bandcampRootSearch = 0;
        qWarning() << "Network error for bandcamp root search query:" << error;
    }
    else if (m_youtubeRootSearch == requestId) {
        m_youtubeRootSearch = 0;
        qWarning() << "Network error for youtube root search query:" << error;
    }
    else if (BandcampArtistResult *artist = m_bandcampArtistQueries.take(requestId)) {
        qWarning() << "Network error for artist query:" << artist->title() << error;
    }
    else if (BandcampAlbumResult *album = m_bandcampAlbumQueries.take(requestId)) {
        qWarning() << "Network error for album query:" << album->title() << error;
    }
    else if (YoutubeVideoResult *video = m_youtubeVideoQueries.take(requestId)) {
        qWarning() << "Network error for youtube video query:" << video->title() << error;
    }
    else if (Result *result = m_iconQueries.take(requestId)) {
        qWarning() << "Network error for icon query:" << result->title() << error;
    }
}

void Query::requestIcon(Result *result)
{
    if (!result->iconData().isEmpty() || result->iconUrl().isEmpty())
        return;

    HttpRequestId reply = m_http->request(QNetworkRequest(QUrl(result->iconUrl())));
    m_iconQueries[reply] = result;
}

void Query::requestBandcampAlbumInfo(BandcampAlbumResult *album)
{
    HttpRequestId reply = m_http->request(QNetworkRequest(QUrl(album->url())));
    m_bandcampAlbumQueries[reply] = album;
    album->setStatus(Result::Querying);
}

void Query::requestBandcampBandInfo(BandcampArtistResult *artist)
{
    const std::string url = ScrapeBandcamp::bandInfoUrl(artist->url().toStdString());
    HttpRequestId reply = m_http->request(QNetworkRequest(QUrl(QString::fromStdString(url))));
    m_bandcampArtistQueries[reply] = artist;
    artist->setStatus(Result::Querying);
}

void Query::requestYoutubeVideoInfo(YoutubeVideoResult *video)
{
    const QString beacon("watch?v=");
    const QStringList parts = video->url().split(beacon);
    if (parts.size() != 2) {
        qWarning() << "Couldn't find video ID in Youtube URL:" << video->url();
        return;
    }

    MoosickMessage::YoutubeUrlQuery query;
    query.videoId = parts[1];

    HttpRequestId reply = m_http->requestFromServer(MoosickMessage::Message(query).toJson());
    m_youtubeVideoQueries[reply] = video;
    video->setStatus(Result::Querying);
}

BandcampAlbumResult *Query::createBandcampAlbumResult(const ScrapeBandcamp::Result &result)
{
    const QString artist = QString::fromStdString(result.bandName);
    const QString name = QString::fromStdString(result.albumName);
    const QString url = QString::fromStdString(result.url);
    const QString icon = QString::fromStdString(result.artUrl);

    BandcampAlbumResult *album = new BandcampAlbumResult(artist, name, url, icon, this);
    connect(album, &Result::queryInfoRequested, this, [=]() { requestBandcampAlbumInfo(album); });
    requestIcon(album);

    return album;
}

BandcampArtistResult *Query::createBandcampArtistResult(const ScrapeBandcamp::Result &result)
{
    const QString name = QString::fromStdString(result.bandName);
    const QString url = QString::fromStdString(result.url);
    const QString icon = QString::fromStdString(result.artUrl);

    BandcampArtistResult *artist = new BandcampArtistResult(name, url, icon, this);
    connect(artist, &Result::queryInfoRequested, this, [=]() { requestBandcampBandInfo(artist); });
    requestIcon(artist);

    return artist;
}

YoutubeVideoResult *Query::createYoutubeVideoResult(const QString &title, const QString &url, const QString &icon)
{
    YoutubeVideoResult *video = new YoutubeVideoResult(title, url, icon, this);
    connect(video, &Result::queryInfoRequested, this, [=]() { requestYoutubeVideoInfo(video); });
    requestIcon(video);

    return video;
}

} // namespace Search
