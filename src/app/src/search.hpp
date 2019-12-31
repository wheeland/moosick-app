#pragma once

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

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

private:
    QString m_title;
    Type m_type;
    QString m_url;
    QString m_iconUrl;
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

public:
    Query(QObject *parent = nullptr);
    ~Query();

    void start(const QString &searchString);
    void seachMore();
    void abort();

    bool hasFinished() const;

signals:
    void finishedChanged(bool finished);

private:
    friend class QueryFilterModel;
    QVector<Result*> m_results;
};

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
