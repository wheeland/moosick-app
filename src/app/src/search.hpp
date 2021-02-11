#pragma once

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include "util/modeladapter.hpp"
#include "httpclient.hpp"

namespace ScrapeBandcamp { struct Result; }
namespace ScrapeYoutube { struct Result; }

namespace Search {

class BandcampArtistResult;
class BandcampAlbumResult;
class BandcampTrackResult;

/**
 * One single result which may be either top-level or nested
 */
class Result : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title CONSTANT)
    Q_PROPERTY(Type type READ resultType CONSTANT)
    Q_PROPERTY(DownloadStatus downloadStatus READ downloadStatus NOTIFY downloadStatusChanged)
    Q_PROPERTY(QString url READ url CONSTANT)
    Q_PROPERTY(QString iconUrl READ iconUrl CONSTANT)
    Q_PROPERTY(QString iconData READ iconData NOTIFY iconDataChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)

public:
    enum Type {
        BandcampArtist,
        BandcampAlbum,
        BandcampTrack,
        YoutubeVideo,
        YoutubePlaylist,
    };
    Q_ENUM(Type)

    enum DownloadStatus {
        DownloadNotStarted,
        DownloadStarted,
        DownloadDone,
    };
    Q_ENUM(DownloadStatus)

    enum Status {
        InfoOnly,
        Querying,
        Error,
        Done,
    };
    Q_ENUM(Status)

    Result(Type tp, const QString &title, const QString &url, const QString &icon, QObject *parent = nullptr);
    ~Result() override = default;

    QString title() const { return m_title; }
    Type resultType() const { return m_type; }
    DownloadStatus downloadStatus() const { return m_downloadStatus; }
    QString url() const { return m_url; }
    QString iconUrl() const { return m_iconUrl; }
    Status status() const { return m_status; }
    QString iconData() const { return m_iconData; }

    void setIconData(const QByteArray &data);
    void setStatus(Status status);
    void setDownloadStatus(DownloadStatus downloadStatus);

    Q_INVOKABLE void queryInfo();

signals:
    void statusChanged(Status status);
    void queryInfoRequested();
    void iconDataChanged();
    void downloadStatusChanged(DownloadStatus downloadStatus);

private:
    QString m_title;
    Type m_type;
    QString m_url;
    QString m_iconUrl;
    QString m_iconData;
    Status m_status = InfoOnly;
    DownloadStatus m_downloadStatus = DownloadNotStarted;
};

class BandcampArtistResult : public Result
{
    Q_OBJECT
    Q_PROPERTY(ModelAdapter::Model *albums READ albumsModel CONSTANT)

public:
    BandcampArtistResult(const QString &title, const QString &url, const QString &icon, QObject *parent = nullptr);
    ~BandcampArtistResult();

    void addAlbum(BandcampAlbumResult *album);
    ModelAdapter::Model *albumsModel() const { return m_albums.model(); }
    QVector<BandcampAlbumResult*> albums() const { return m_albums.data(); }

private:
    ModelAdapter::Adapter<BandcampAlbumResult*> m_albums;
};

class BandcampAlbumResult : public Result
{
    Q_OBJECT
    Q_PROPERTY(ModelAdapter::Model *tracks READ tracksModel CONSTANT)
    Q_PROPERTY(QString artist READ artist CONSTANT)

public:
    BandcampAlbumResult(const QString &artist, const QString &title, const QString &url, const QString &icon, QObject *parent = nullptr);
    ~BandcampAlbumResult();

    void addTrack(BandcampTrackResult *track);
    ModelAdapter::Model *tracksModel() const { return m_tracks.model(); }
    QVector<BandcampTrackResult*> tracks() const { return m_tracks.data(); }

    QString artist() const { return m_artist; }

private:
    ModelAdapter::Adapter<BandcampTrackResult*> m_tracks;
    QString m_artist;
};

class BandcampTrackResult : public Result
{
    Q_OBJECT
    Q_PROPERTY(int secs READ secs CONSTANT)
    Q_PROPERTY(BandcampAlbumResult *album READ album CONSTANT)

public:
    BandcampTrackResult(BandcampAlbumResult *album, const QString &title, const QString &url, const QString &icon, int secs, QObject *parent = nullptr);
    ~BandcampTrackResult() = default;
    int secs() const { return m_secs; }

    BandcampAlbumResult *album() const { return m_album; }

private:
    int m_secs;
    BandcampAlbumResult *m_album;
};

class YoutubeVideoResult : public Result
{
    Q_OBJECT

public:
    YoutubeVideoResult(const QString &title, const QString &videoUrl, const QString &icon, QObject *parent = nullptr);
    ~YoutubeVideoResult() = default;

    int secs() const { return m_secs; }
    QString audioUrl() { return m_audioUrl; }

    void setDetails(const QString &audioUrl, int secs);

signals:
    void hasDetails();

private:
    int m_secs;
    QString m_audioUrl;
};

/**
 * Encapsulates all results from one specific search
 *
 * This will include:
 * - one or more pages of bc/yt/sc root entries
 * - artists that have albums that have songs
 * - playlists that have videos
 * - data will be filled as it comes in
 */
class Query : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool finished READ hasFinished NOTIFY finishedChanged)
    Q_PROPERTY(QAbstractItemModel *model READ model CONSTANT)

public:
    Query(HttpClient *http, QObject *parent = nullptr);
    ~Query() override;

    QAbstractItemModel *model() const;

    bool hasFinished() const;

    Q_INVOKABLE void clear();
    Q_INVOKABLE void search(const QString &searchString);

signals:
    void finishedChanged();
    void hasErrorsChanged(bool hasErrors);

private slots:
    void onReply(HttpRequestId requestId, const QByteArray &data);
    void onNetworkError(HttpRequestId requestId, QNetworkReply::NetworkError error);

    void startBandcampRootSearch();
    void startYoutubeRootSearch();

private:
    void requestIcon(Result *result);

    BandcampArtistResult *createBandcampArtistResult(const ScrapeBandcamp::Result &result);
    BandcampAlbumResult *createBandcampAlbumResult(const ScrapeBandcamp::Result &result);
    YoutubeVideoResult *createYoutubeVideoResult(const QString &title, const QString &url, const QString &icon);

    void requestBandcampBandInfo(BandcampArtistResult *artist);
    void requestBandcampAlbumInfo(BandcampAlbumResult *album);
    void requestYoutubeVideoInfo(YoutubeVideoResult *video);

    void populateBandcampSearchResults(const QByteArray &html);
    void populateBandcampArtist(BandcampArtistResult *artist, const QByteArray &html);
    void populateBandcampAlbum(BandcampAlbumResult *album,const QByteArray &html);
    void populateYoutubeSearchResults(const QByteArray &html);
    void populateYoutubeVideo(YoutubeVideoResult *video, const QByteArray &json);

    HttpRequester * const m_http;
    QString m_searchString;

    // associate each query with what they were querying
    HttpRequestId m_bandcampRootSearch = HTTP_NULL_REQUEST;
    HttpRequestId m_youtubeRootSearch = HTTP_NULL_REQUEST;
    QHash<HttpRequestId, BandcampArtistResult*> m_bandcampArtistQueries;
    QHash<HttpRequestId, BandcampAlbumResult*> m_bandcampAlbumQueries;
    QHash<HttpRequestId, YoutubeVideoResult*> m_youtubeVideoQueries;
    QHash<HttpRequestId, Result*> m_iconQueries;

    ModelAdapter::Adapter<Result*> m_rootResults;
};

/**
 * Filters out one specific Result::Type from all results in one Query
 */
class QueryFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(Result::Type filterType READ filterType WRITE setFilterType NOTIFY filterTypeChanged)
    Q_PROPERTY(ModelAdapter::Model* source READ source WRITE setSource NOTIFY sourceChanged)

public:
    QueryFilterModel(QObject *parent = nullptr);
    ~QueryFilterModel() override = default;

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    ModelAdapter::Model* source() const { return m_source; }
    Result::Type filterType() const { return m_filterType; }
    void setFilterType(Result::Type type);

public slots:
    void setSource(ModelAdapter::Model* source);

signals:
    void filterTypeChanged(Result::Type type);
    void sourceChanged(ModelAdapter::Model* source);

private:
    ModelAdapter::Model *m_source;
    Result::Type m_filterType;
};


} // namespace Search

Q_DECLARE_METATYPE(Search::Result*)
Q_DECLARE_METATYPE(Search::BandcampArtistResult*)
Q_DECLARE_METATYPE(Search::BandcampAlbumResult*)
Q_DECLARE_METATYPE(Search::BandcampTrackResult*)
