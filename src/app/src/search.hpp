#pragma once

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QNetworkReply>

class QNetworkAccessManager;

namespace NetCommon {
class BandcampAlbumInfo;
}

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
    QString url() const { return m_url; }
    QString iconUrl() const { return m_iconUrl; }
    Status status() const { return m_status; }
    QString iconData() const { return m_iconData; }

    void setIconData(const QString &url, const QByteArray &data);
    void setStatus(Status status);

    Q_INVOKABLE void queryInfo();

signals:
    void statusChanged(Status status);
    void queryInfoRequested();
    void iconDataChanged();

private:
    QString m_title;
    Type m_type;
    QString m_url;
    QString m_iconUrl;
    QString m_iconData;
    Status m_status = InfoOnly;
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

    Q_INVOKABLE Search::BandcampAlbumResult *getAlbum(int index) const;

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

    Q_INVOKABLE Search::BandcampTrackResult *getTrack(int index) const;

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
    BandcampTrackResult(const QString &title, const QString &url, const QString &icon, int secs, QObject *parent = nullptr);
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

    enum Role {
        ResultRole = Qt::UserRole + 1
    };

public:
    Query(const QString &host, quint16 port, QObject *parent = nullptr);
    ~Query() override;

    bool hasFinished() const;
    bool hasErrors() const;

    Q_INVOKABLE void abort();
    Q_INVOKABLE void clear();
    Q_INVOKABLE void search(const QString &searchString);

    Q_INVOKABLE void retry();

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:
    void finishedChanged(bool finished);
    void networkError(QNetworkReply::NetworkError error);
    void hasErrorsChanged(bool hasErrors);

private:
    QNetworkReply *request(const QString &path, const QString &query);
    QNetworkReply *requestRootSearch();
    QNetworkReply *requestArtistSearch(const QString &url);
    QNetworkReply *requestAlbumSearch(const QString &url);
    void requestIcon(Result *result);

    void onNetworkReplyFinished(QNetworkReply *reply, QNetworkReply::NetworkError error);

    BandcampAlbumResult *createAlbumResult(const QString &name, const QString &url, const QString &icon);
    BandcampArtistResult *createArtistResult(const QString &name, const QString &url, const QString &icon);

    bool populateRootResults(const QByteArray &json);
    void populateAlbum(BandcampAlbumResult *album, const NetCommon::BandcampAlbumInfo &albumInfo);
    bool populateArtist(BandcampArtistResult *artist, const QByteArray &artistInfo);

    // These parameters won't change
    const QString m_host;
    const quint16 m_port;
    QNetworkAccessManager *m_manager = nullptr;

    QString m_searchString;

    // keep track of all curently running queries
    // some queries have failed for some reason, and can be repeated later on.
    // a query is in either of these two lists
    QVector<QNetworkReply*> m_runningQueries;

    // associate each query with what they were querying
    QNetworkReply *m_activeRootPageQuery = nullptr;
    QHash<QNetworkReply*, BandcampArtistResult*> m_artistQueries;
    QHash<QNetworkReply*, BandcampAlbumResult*> m_albumQueries;
    QHash<QNetworkReply*, Result*> m_iconQueries;

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

Q_DECLARE_METATYPE(Search::BandcampArtistResult*)
Q_DECLARE_METATYPE(Search::BandcampAlbumResult*)
Q_DECLARE_METATYPE(Search::BandcampTrackResult*)
