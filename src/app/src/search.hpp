#pragma once

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QNetworkReply>

class QNetworkAccessManager;

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
    Q_PROPERTY(QString url READ url CONSTANT)
    Q_PROPERTY(QString iconUrl READ iconUrl CONSTANT)
    Q_PROPERTY(bool querying READ isQuerying NOTIFY queryingChanged)

public:
    enum Type {
        BandcampArtist,
        BandcampAlbum,
        BandcampTrack,
        YoutubeVideo,
        YoutubePlaylist,
    };
    Q_ENUM(Type)

    Result(Type tp, const QString &title, const QString &url, const QString &icon, QObject *parent = nullptr);
    ~Result() override = default;

    QString title() const { return m_title; }
    Type resultType() const { return m_type; }
    QString url() const { return m_url; }
    QString iconUrl() const { return m_iconUrl; }
    bool isQuerying() const { return m_querying; }

    void setQuerying(bool querying);

signals:
    void queryingChanged(bool querying);

private:
    QString m_title;
    Type m_type;
    QString m_url;
    QString m_iconUrl;
    bool m_querying;
};

class BandcampArtistResult : public Result
{
    Q_OBJECT
    Q_PROPERTY(int albumCount READ albumCount NOTIFY albumCountChanged)

public:
    BandcampArtistResult(const QString &title, const QString &url, const QString &icon, QObject *parent = nullptr);
    ~BandcampArtistResult();

    int albumCount() const;
    void addAlbum(BandcampAlbumResult *album);

signals:
    void albumCountChanged(int albumCount);

private:
    QVector<BandcampAlbumResult*> m_albums;
};

class BandcampAlbumResult : public Result
{
    Q_OBJECT
    Q_PROPERTY(int trackCount READ trackCount NOTIFY trackCountChanged)

public:
    BandcampAlbumResult(const QString &title, const QString &url, const QString &icon, QObject *parent = nullptr);
    ~BandcampAlbumResult();

    int trackCount() const;
    void addTrack(BandcampTrackResult *track);

signals:
    void trackCountChanged(int trackCount);

private:
    QVector<BandcampTrackResult*> m_tracks;
};

class BandcampTrackResult : public Result
{
    Q_OBJECT
    Q_PROPERTY(int secs READ secs CONSTANT)

public:
    BandcampTrackResult(const QString &title, const QString &url, const QString &icon, QObject *parent = nullptr);
    ~BandcampTrackResult() = default;
    int secs() const { return m_secs; }

private:
    int m_secs;
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
class Query : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool finished READ hasFinished NOTIFY finishedChanged)
    Q_PROPERTY(bool hasErrors READ hasErrors NOTIFY hasErrorsChanged)

public:
    Query(const QString &host, quint16 port, const QString &searchString, QObject *parent = nullptr);
    ~Query();

    void seachMore();

    bool hasFinished() const;
    bool hasErrors() const;

    void retry();

signals:
    void finishedChanged(bool finished);
    void networkError(QNetworkReply::NetworkError error);
    void hasErrorsChanged(bool hasErrors);

private slots:
    void onNetworkReplyFinished(QNetworkReply *reply);
    void onNetworkError(QNetworkReply::NetworkError error);

private:
    QNetworkReply *request(const QString &path);
    QNetworkReply *requestRootSearch(int page);
    QNetworkReply *requestArtistSearch(const QString &url);
    QNetworkReply *requestAlbumSearch(const QString &url);
    bool populateRootResults(const QByteArray &json);

    // These search parameters won't change
    const QString m_searchString;
    const QString m_host;
    const quint16 m_port;
    QNetworkAccessManager *m_manager = nullptr;

    // keep track of all curently running queries
    QVector<QNetworkReply*> m_runningQueries;

    // associate each query with what they were querying
    int m_queriedPages = 0;
    QHash<QNetworkReply*, int> m_rootPageQueries;
    QHash<QNetworkReply*, BandcampArtistResult*> m_artistQueries;
    QHash<QNetworkReply*, BandcampAlbumResult*> m_albumQueries;

    // these queries have failed for some reason, and can be repeated later on
    QVector<QNetworkReply*> m_failedQueries;

    friend class QueryFilterModel;
    QVector<Result*> m_rootResults;
};

/**
 * Filters out one specific Result::Type from all results in one Query
 */
class QueryFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(Result::Type filterType READ filterType WRITE setFilterType NOTIFY filterTypeChanged)
    Q_PROPERTY(Query* source READ source WRITE setSource NOTIFY sourceChanged)

public:
    QueryFilterModel(QObject *parent = nullptr);
    ~QueryFilterModel() override = default;

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    Query* source() const { return m_source; }
    Result::Type filterType() const { return m_filterType; }
    void setFilterType(Result::Type type);

public slots:
    void setSource(Query* source);

signals:
    void filterTypeChanged(Result::Type type);
    void sourceChanged(Query* source);

private:
    Query *m_source;
    Result::Type m_filterType;
};


} // namespace Search
