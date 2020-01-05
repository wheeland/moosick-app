#include "search.hpp"
#include "json.hpp"
#include "util/qmlutil.hpp"

#include <QtGlobal>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QBuffer>
#include <QImage>

static QJsonDocument parseJson(const QByteArray &json, const QByteArray &name)
{
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &error);
    if (doc.isNull()) {
        qWarning().noquote() << "Failed to parse" << name;
        qWarning().noquote() << json;
        qWarning() << "Error:";
        qWarning().noquote() << error.errorString();
    }
    return doc;
}

static QJsonArray parseJsonArray(const QByteArray &json, const QByteArray &name)
{
    const QJsonDocument doc = parseJson(json, name);
    if (doc.isNull())
        return QJsonArray();

    if (!doc.isArray()) {
        qWarning().noquote() << name << "is not a JSON array";
        return QJsonArray();
    }

    return doc.array();
}

static QJsonObject parseJsonObject(const QByteArray &json, const QByteArray &name)
{
    const QJsonDocument doc = parseJson(json, name);
    if (doc.isNull())
        return QJsonObject();

    if (!doc.isObject()) {
        qWarning().noquote() << name << "is not a JSON object";
        return QJsonObject();
    }

    return doc.object();
}

namespace Search {

Result::Result(Type tp, const QString &title, const QString &url, const QString &icon, QObject *parent)
    : QObject(parent)
    , m_title(title)
    , m_type(tp)
    , m_url(url)
    , m_iconUrl(icon)
{
}

void Result::setIconData(const QString &url, const QByteArray &data)
{
    m_iconData = QmlUtil::imageDataToDataUri(data, url);
    emit iconDataChanged();
}

void Result::setStatus(Result::Status status)
{
    if (m_status != status) {
        m_status = status;
        emit statusChanged(status);
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
}

Query::~Query()
{
    abort();
    clear();
}

QAbstractItemModel *Query::model() const
{
    return m_rootResults.model();
}

bool Query::hasFinished() const
{
    return m_rootResults.size() > 0;
}

bool Query::hasErrors() const
{
    return (m_activeRootPageQuery == nullptr) && hasFinished();
}

void Query::clear()
{
    for (Result *result : m_rootResults.data())
        result->deleteLater();
    m_rootResults.clear();
}

void Query::search(const QString &searchString)
{
    clear();
    m_searchString = searchString.trimmed();
    m_activeRootPageQuery = requestRootSearch();
}

void Query::retry()
{
    if (!hasErrors())
        return;

    m_activeRootPageQuery = requestRootSearch();
    emit hasErrorsChanged(false);
}

bool Query::populateRootResults(const QByteArray &json)
{
    const QJsonArray root = parseJsonArray(json, "search.do");
    if (root.isEmpty())
        return false;

    QVector<Result*> newResults;

    for (const QJsonValue &entry : root) {
        const QString type = entry["type"].toString();
        const QString name = entry["name"].toString();
        const QString artist = entry["artist"].toString();
        const QString url = entry["url"].toString();
        const QString icon = entry["icon"].toString();

        if (type == "artist") {
            newResults << createArtistResult(name, url, icon);
        }
        else if (type == "album") {
            newResults << createAlbumResult(artist, name, url, icon);
        }
        else if (type == "video") {
            qWarning() << "video not implemented";
        }
        else if (type == "playlist") {
            qWarning() << "playlist not implemented";
        }
    }

    // add to model
    for (Result *newResult : qAsConst(newResults))
        m_rootResults.add(newResult);

    emit finishedChanged(true);

    return true;
}

void Query::populateAlbum(BandcampAlbumResult *album, const NetCommon::BandcampAlbumInfo &albumInfo)
{
    for (const NetCommon::BandcampSongInfo &song : albumInfo.tracks) {
        BandcampTrackResult *track = new BandcampTrackResult(album, song.name, song.url, albumInfo.icon, song.secs, this);
        album->addTrack(track);
    }
}

bool Query::populateArtist(BandcampArtistResult *artist, const QByteArray &artistInfo)
{
    const QJsonArray root = parseJsonArray(artistInfo, "bandcamp-artist-info");
    if (root.isEmpty())
        return false;

    for (const QJsonValue &entry : root) {
        const QString type = entry["type"].toString();
        const QString name = entry["name"].toString();
        const QString url = entry["url"].toString();
        const QString icon = entry["icon"].toString();

        BandcampAlbumResult *newAlbum = createAlbumResult(artist->title(), name, url, icon);
        artist->addAlbum(newAlbum);
    }

    return true;
}

void Query::onReply(QNetworkReply *reply, QNetworkReply::NetworkError error)
{
    const QByteArray data = reply->readAll();
    const bool hasError = (error != QNetworkReply::NoError);

    // is this the first root query?
    if (m_activeRootPageQuery == reply) {
        m_activeRootPageQuery = nullptr;

        if (!hasError) {
            populateRootResults(data);
            emit finishedChanged(true);
        }

        emit hasErrorsChanged(hasErrors());
    }

    else if (m_artistQueries.contains(reply)) {
        BandcampArtistResult *artist = m_artistQueries.take(reply);

        if (!hasError)
            populateArtist(artist, data);

        artist->setStatus(hasError ? Result::Error : Result::Done);
    }

    else if (m_albumQueries.contains(reply)) {
        BandcampAlbumResult *album = m_albumQueries.take(reply);

        if (!hasError) {
            const QJsonObject json = parseJsonObject(data, "bandcamp-album-info");
            NetCommon::BandcampAlbumInfo albumInfo;
            if (!albumInfo.fromJson(json))
                qWarning() << "Failed to parse album info:" << json;
            else
                populateAlbum(album, albumInfo);
        }

        album->setStatus(hasError ? Result::Error : Result::Done);
    }

    else if (m_iconQueries.contains(reply)) {
        Result *result = m_iconQueries.take(reply);

        if (!hasError) {
            const QString url = reply->url().toString();
            result->setIconData(url, data);
        }
    }
}

QNetworkReply *Query::requestRootSearch()
{
    const QByteArray searchStringBase64 = m_searchString.toUtf8().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    return m_http->requestFromServer("/search.do", QString("v=") + searchStringBase64);
}

QNetworkReply *Query::requestArtistSearch(const QString &url)
{
    return m_http->requestFromServer("/bandcamp-artist-info.do", QString("v=") + url);
}

QNetworkReply *Query::requestAlbumSearch(const QString &url)
{
    return m_http->requestFromServer("/bandcamp-album-info.do", QString("v=") + url);
}

void Query::requestIcon(Result *result)
{
    if (!result->iconData().isEmpty() || result->iconUrl().isEmpty())
        return;

    QNetworkReply *reply = m_http->request(QNetworkRequest(QUrl(result->iconUrl())));
    m_iconQueries[reply] = result;
}

BandcampAlbumResult *Query::createAlbumResult(const QString &artist, const QString &name, const QString &url, const QString &icon)
{
    BandcampAlbumResult *album = new BandcampAlbumResult(artist, name, url, icon, this);

    connect(album, &Result::queryInfoRequested, this, [=]() {
        if (album->status() != Result::Querying && album->status() != Result::Done) {
            QNetworkReply *reply = requestAlbumSearch(url);
            m_albumQueries[reply] = album;
            album->setStatus(Result::Querying);
        }
    });

    requestIcon(album);

    return album;
}

BandcampArtistResult *Query::createArtistResult(const QString &name, const QString &url, const QString &icon)
{
    BandcampArtistResult *artist = new BandcampArtistResult(name, url, icon, this);

    connect(artist, &Result::queryInfoRequested, this, [=]() {
        if (artist->status() != Result::Querying && artist->status() != Result::Done) {
            QNetworkReply *reply = requestArtistSearch(url);
            m_artistQueries[reply] = artist;
            artist->setStatus(Result::Querying);
        }
    });

    requestIcon(artist);

    return artist;
}

} // namespace Search
