#include "search.hpp"
#include "json.hpp"

#include <QtGlobal>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

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

BandcampAlbumResult *BandcampArtistResult::getAlbum(int index) const
{
    return m_albums.value(index);
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

BandcampTrackResult *BandcampAlbumResult::getTrack(int index) const
{
    return m_tracks.value(index);
}

BandcampTrackResult::BandcampTrackResult(const QString &title, const QString &url, const QString &icon, int secs, QObject *parent)
    : Result(BandcampTrack, title, url, icon, parent)
    , m_secs(secs)
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

    const Result *result = m_source->m_rootResults.value(sourceRow);
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

Query::Query(const QString &host, quint16 port, const QString &searchString, QObject *parent)
    : QAbstractListModel(parent)
    , m_searchString(searchString)
    , m_host(host)
    , m_port(port)
{
    m_manager = new QNetworkAccessManager(this);
    connect(m_manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply *reply) {
        onNetworkReplyFinished(reply, QNetworkReply::NoError);
    });

    // send inital search request
    m_activeRootPageQuery = requestRootSearch();
}

Query::~Query()
{
    for (QNetworkReply *reply : m_runningQueries)
        reply->deleteLater();
    for (Result *result : m_rootResults)
        result->deleteLater();
    m_manager->deleteLater();
}

bool Query::hasFinished() const
{
    return !m_rootResults.isEmpty();
}

bool Query::hasErrors() const
{
    return (m_activeRootPageQuery == nullptr) && (m_rootResults.isEmpty());
}

void Query::retry()
{
    if (!hasErrors())
        return;

    m_activeRootPageQuery = requestRootSearch();
    emit hasErrorsChanged(false);
}

int Query::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_rootResults.size();
}

QVariant Query::data(const QModelIndex &index, int role) const
{
    const int idx = index.row();
    switch (role) {
    case ResultRole: return QVariant::fromValue(m_rootResults[idx]);
    default: return QVariant();
    }
}

QHash<int, QByteArray> Query::roleNames() const
{
    return {{ResultRole, "result"}};
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
        const QString url = entry["url"].toString();
        const QString icon = entry["icon"].toString();

        if (type == "artist") {
            newResults << createArtistResult(name, url, icon);
        }
        else if (type == "album") {
            newResults << createAlbumResult(name, url, icon);
        }
        else if (type == "video") {
            qWarning() << "video not implemented";
        }
        else if (type == "playlist") {
            qWarning() << "playlist not implemented";
        }
    }

    // add to model
    beginInsertRows(QModelIndex(), m_rootResults.size(), m_rootResults.size() + newResults.size() - 1);
    m_rootResults << newResults;
    endInsertRows();

    return true;
}

bool Query::populateAlbum(BandcampAlbumResult *album, const NetCommon::BandcampAlbumInfo &albumInfo)
{
    for (const NetCommon::BandcampSongInfo &song : albumInfo.tracks) {
        BandcampTrackResult *track = new BandcampTrackResult(song.name, song.url, albumInfo.icon, song.secs, this);
        album->addTrack(track);
    }
}

bool Query::populateArtist(BandcampArtistResult *artist, const QByteArray &artistInfo)
{
    const QJsonArray root = parseJsonArray(artistInfo, "search.do");
    if (root.isEmpty())
        return false;

    for (const QJsonValue &entry : root) {
        NetCommon::BandcampAlbumInfo albumInfo;
        if (!albumInfo.fromJson(entry.toObject())) {
            qWarning() << "Failed to parse album info:" << entry;
            continue;
        }

        BandcampAlbumResult *newAlbum = new BandcampAlbumResult(albumInfo.name, albumInfo.url, albumInfo.icon, this);
        populateAlbum(newAlbum, albumInfo);

        artist->addAlbum(newAlbum);
    }

    return true;
}

void Query::onNetworkReplyFinished(QNetworkReply *reply, QNetworkReply::NetworkError error)
{
    if (!m_runningQueries.contains(reply))
        return;

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

    m_runningQueries.removeAll(reply);
    reply->deleteLater();
}

QNetworkReply *Query::request(const QString &path, const QString &query)
{
    QUrl url;
    url.setScheme("http");
    url.setHost(m_host);
    url.setPort(m_port);
    url.setPath(path);
    url.setQuery(query, QUrl::StrictMode);

    const QNetworkRequest request(url);
    QNetworkReply *reply = m_manager->get(request);
    connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
            this, [=](QNetworkReply::NetworkError error) {
        onNetworkReplyFinished(reply, error);
    });

    m_runningQueries << reply;
    if (m_runningQueries.size() == 1)
        emit finishedChanged(false);

    return reply;
}

QNetworkReply *Query::requestRootSearch()
{
    const QByteArray searchStringBase64 = m_searchString.toUtf8().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    return request("/search.do", QString("v=") + searchStringBase64);
}

QNetworkReply *Query::requestArtistSearch(const QString &url)
{
    return request("/bandcamp-artist-info.do", QString("v=") + url);
}

QNetworkReply *Query::requestAlbumSearch(const QString &url)
{
    return request("/bandcamp-album-info.do", QString("v=") + url);
}

BandcampAlbumResult *Query::createAlbumResult(const QString &name, const QString &url, const QString &icon)
{
    BandcampAlbumResult *album = new BandcampAlbumResult(name, url, icon, this);

    connect(album, &Result::queryInfoRequested, this, [=]() {
        if (album->status() != Result::Querying && album->status() != Result::Done) {
            QNetworkReply *reply = requestAlbumSearch(url);
            m_albumQueries[reply] = album;
            album->setStatus(Result::Querying);
        }
    });

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

    return artist;
}

} // namespace Search
