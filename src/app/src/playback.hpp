#pragma once

#include <QObject>
#include <QUrl>
#include <QMediaPlayer>
#include <QMediaPlaylist>

/**
 * Wrapper around QMediaPlayer/QMediaPlaylist, can be exposed via QtRemoteObjects on Android
 */
class PlaybackImpl : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(bool hasSong READ hasSong NOTIFY hasSongChanged)
    Q_PROPERTY(int duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(float position READ position NOTIFY positionChanged)

public:
    PlaybackImpl(QObject *parent = nullptr);
    ~PlaybackImpl();

    bool playing() const;
    bool hasSong() const;
    int duration() const;
    float position() const;

public slots:
    void setPlaylist(const QVector<QUrl>& urls);
    void addSong(const QUrl& url);
    void removeSong(int pos);
    void insertSong(int pos, const QUrl& url);
    void jump(int index);
    void step(int delta);
    void seek(float position);
    void play();
    void pause();

private slots:
    void onMediaStatusChanged();
    void onCurrentIndexChanged(int index);

signals:
    void currentSongChanged(int index, const QUrl& url);
    void durationChanged(int duration);
    void positionChanged(float position);
    void playingChanged(bool isPlaying);
    void hasSongChanged(bool isPlaying);

private:
    QMediaPlayer *m_player;
    QMediaPlaylist *m_playlist;

    QVector<QUrl> m_urls;

    bool m_isPlaying = false;
};

#ifdef Q_OS_ANDROID
#include "rep_playback_source.h"
#include "rep_playback_replica.h"

using Playback = PlaybackReplica;

int runPlaybackService(int argc, char **argv);
PlaybackReplica *connectToPlaybackService();
void startPlaybackService();
#else // Q_OS_ANDROID
using Playback = PlaybackImpl;
#endif

Q_DECLARE_METATYPE(Playback*);
