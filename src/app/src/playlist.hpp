#pragma once

#include "util/modeladapter.hpp"
#include "result.hpp"
#include "httpclient.hpp"
#include "playback.hpp"

namespace Playlist {

class Entry : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Source source READ source CONSTANT)
    Q_PROPERTY(QString artist READ artist CONSTANT)
    Q_PROPERTY(QString album READ album CONSTANT)
    Q_PROPERTY(QString title READ title CONSTANT)
    Q_PROPERTY(QUrl url READ url CONSTANT)
    Q_PROPERTY(QString durationString READ durationString CONSTANT)
    Q_PROPERTY(QString iconData READ iconData NOTIFY iconDataChanged)
    Q_PROPERTY(bool selected READ isSelected WRITE setSelected NOTIFY selectedChanged)

public:
    enum Source {
        Bandcamp,
        Youtube,
        Library,
    };

    Entry(Source source, const QString &artist, const QString &album, const QString &title,
          const QUrl &url, const QString &iconUrl, int duration, QObject *parent = nullptr);
    ~Entry() override;

    Source source() const { return m_source; }
    QString title() const { return m_title; }
    QString artist() const { return m_artist; }
    QString album() const { return m_album; }
    QString iconData() const { return m_iconData; }
    QUrl url() const { return m_url; }
    QString iconUrl() const { return m_iconUrl; }
    bool isSelected() const { return m_selected; }

    QString durationString() const;

    void setSelected(bool selected);
    void setIconData(const QString &data);

signals:
    void iconDataChanged();
    void selectedChanged(bool selected);

private:
    Source m_source;
    QString m_title;
    QString m_artist;
    QString m_album;
    QString m_iconData;
    QUrl m_url;
    QString m_iconUrl;
    int m_duration = 0;
    bool m_selected = false;
};

class Playlist : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Entry *currentSong READ currentSong NOTIFY currentSongChanged)
    Q_PROPERTY(ModelAdapter::Model *entries READ entries CONSTANT)
    Q_PROPERTY(bool hasSelectedSongs READ hasSelectedSongs NOTIFY hasSelectedSongsChanged)
    Q_PROPERTY(bool repeat READ repeat WRITE setRepeat NOTIFY repeatChanged)

public:
    Playlist(HttpClient *httpClient, Playback *playback, QObject *parent = nullptr);
    ~Playlist();

    ModelAdapter::Model *entries() const;
    Entry *currentSong() const { return m_currentSong; }
    bool hasSelectedSongs() const { return m_hasSelectedSongs; }
    bool repeat() const { return m_repeat; }

    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void play(Entry *entry);
    Q_INVOKABLE void remove(Entry *entry);

    void addFromInternet(
        Entry::Source source, const QUrl &url,
        const QString &artist, const QString &album, const QString &title,
        int duration, const QString &iconUrl, bool append
    );

    void addFromLibrary(
        const QString &fileName, const QString &artist, const QString &album, const QString &title,
        int duration, bool append
    );


public slots:
    void setRepeat(bool repeat);

signals:
    void currentSongChanged();
    void hasSelectedSongsChanged(bool hasSelectedSongs);
    void repeatChanged(bool repeat);
    void randomizedChanged(bool randomized);

private slots:
    void onSelectedChanged();
    void onNetworkReplyFinished(HttpRequestId requestId, const QByteArray &data);
    void onNetworkError(HttpRequestId requestId, QNetworkReply::NetworkError error);

private:
    Entry *createEntry(Entry::Source source,
                       const QString &artist, const QString &album, const QString &title,
                       const QUrl &url, int duration, const QString &iconUrl);
    void insertEntry(Entry *newEntry, bool append);
    void advance(int delta);

    void purgeUnusedIcons();
    void requestIcon(Entry *entry);

    ModelAdapter::Adapter<Entry*> m_entries;
    QHash<QString, QString> m_iconUrlToDataString;
    QHash<QString, HttpRequestId> m_iconQueries;

    HttpRequestId m_mediaBaseUrlRequest;
    QString m_mediaBaseUrl;

    // -1 if we are at the end of the playlist
    int m_currentEntry = -1;
    bool m_hasSelectedSongs = false;
    bool m_repeat = false;

    Entry *m_currentSong = nullptr;
//    void currentSongMaybeChanged();

    // if the playlist is randomized, the current entry index doesn't refer to the
    // actual m_entries list, but rather goes through the indirection of the
    // random index overlay
//    bool m_randomized = false;
//    QVector<int> m_randomizedOverlay;

    HttpRequester *m_http;
    Playback *m_playback;
};

} // namespace Playlist

Q_DECLARE_METATYPE(Playlist::Entry*)
