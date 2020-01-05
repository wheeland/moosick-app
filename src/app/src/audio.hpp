#pragma once

#include "search.hpp"
#include "playlist.hpp"

class QMediaPlayer;

class Audio : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool playing READ isPlaying NOTIFY statusChanged)
    Q_PROPERTY(bool paused READ isPaused NOTIFY statusChanged)
    Q_PROPERTY(bool hasSong READ hasSong NOTIFY statusChanged)
    Q_PROPERTY(bool loading READ isLoading NOTIFY statusChanged)

    Q_PROPERTY(QString durationString READ durationString NOTIFY durationStringChanged)
    Q_PROPERTY(float position READ position NOTIFY positionChanged)

public:
    enum Status {
        Stopped,
        Error,
        Paused,
        PausedLoading,
        Playing,
        PlayingLoading,
    };
    Q_ENUM(Status)

    Audio(Playlist::Playlist *playlist, QObject *parent = nullptr);
    ~Audio() override;

    Status status() const { return m_status; }
    bool isPlaying() const;
    bool isPaused() const;
    bool isLoading() const;
    bool hasSong() const;

    QString durationString() const;
    float position() const;

    Q_INVOKABLE void play();
    Q_INVOKABLE void seek(float time);
    Q_INVOKABLE void pause();

    Q_INVOKABLE QString timeToDisplayString(float time) const;

signals:
    void positionChanged();
    void statusChanged();
    void durationStringChanged();

private slots:
    void onCurrentSongChanged();
    void updateStatus();

private:
    Status computeStatus();

    Playlist::Playlist *m_playlist;
    QMediaPlayer *m_player;
    Status m_status = Stopped;
};


