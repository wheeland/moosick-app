#include "audio.hpp"

#include <QMediaPlayer>

Audio::Audio(Playlist::Playlist *playlist, QObject *parent)
    : QObject(parent)
    , m_playlist(playlist)
    , m_player(new QMediaPlayer(this))
{
    // TODO: pre-read stuff from a HTTP connection to pass to setMedia()
    connect(playlist, &Playlist::Playlist::currentSongChanged, this, &Audio::onCurrentSongChanged);
    connect(m_player, &QMediaPlayer::positionChanged, this, &Audio::positionChanged);
    connect(m_player, static_cast<void (QMediaPlayer::*)(QMediaPlayer::Error)>(&QMediaPlayer::error), this, &Audio::updateStatus);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &Audio::updateStatus);
    connect(m_player, &QMediaPlayer::stateChanged, this, &Audio::updateStatus);
    connect(m_player, &QMediaPlayer::durationChanged, this, &Audio::durationStringChanged);
}

Audio::~Audio()
{
}

bool Audio::isPlaying() const
{
    return m_status == Playing || m_status == PlayingLoading;
}

bool Audio::isPaused() const
{
    return m_status == Paused || m_status == PausedLoading;
}

bool Audio::isLoading() const
{
    return m_status == PlayingLoading || m_status == PausedLoading;
}

bool Audio::hasSong() const
{
    return m_playlist->currentSong() != nullptr;
}

QString Audio::durationString() const
{
    return timeToDisplayString(1.0f);
}

float Audio::position() const
{
    if (!hasSong() || m_status == Error)
        return 0.0f;

    return (float) m_player->position() / (float) m_player->duration();
}

void Audio::play()
{
    m_player->play();
}

void Audio::seek(float time)
{
    m_player->setPosition(m_player->duration() * time);
}

void Audio::pause()
{
    m_player->pause();
}

QString Audio::timeToDisplayString(float time) const
{
    const qint64 duration = m_player->duration();
    if (duration == 0)
        return "0:00";

    const int msecs = m_player->duration() * time;
    return QString::asprintf("%d:%02d", msecs / 60000, (msecs % 60000) / 1000);
}

void Audio::onCurrentSongChanged()
{
    Playlist::Entry *song = m_playlist->currentSong();

    if (!song) {
        m_player->stop();
        m_status = Stopped;
    } else {
        m_player->setMedia(QUrl(song->url()));
        m_player->play();
        m_status = Playing;
    }

    updateStatus();
}

Audio::Status Audio::computeStatus()
{
    if (m_player->error() != QMediaPlayer::NoError) {
        qWarning() << "Error:" << m_player->errorString();
        return Error;
    }

    const bool playingState = m_player->state() == QMediaPlayer::PlayingState;
    const bool pausedState = m_player->state() == QMediaPlayer::PausedState;

    switch (m_player->mediaStatus()) {
    case QMediaPlayer::UnknownMediaStatus:
    case QMediaPlayer::InvalidMedia:
        return Error;
    case QMediaPlayer::NoMedia:
    case QMediaPlayer::EndOfMedia:
    case QMediaPlayer::LoadedMedia:
        return Stopped;
    case QMediaPlayer::StalledMedia:
        return playingState ? PlayingLoading : PausedLoading;
    case QMediaPlayer::BufferingMedia:
    case QMediaPlayer::BufferedMedia:
    case QMediaPlayer::LoadingMedia:
        return playingState ? Playing : (pausedState ? Paused : Stopped);
    default:
        return Error;
    }
}

void Audio::updateStatus()
{
    Status status = computeStatus();

    if (m_status != status) {
        m_status = status;
        emit statusChanged();
        emit positionChanged();
    }

    if (m_player->mediaStatus() == QMediaPlayer::EndOfMedia)
        m_playlist->next();
}
