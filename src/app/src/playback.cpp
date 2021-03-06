#include "playback.hpp"

#ifdef Q_OS_ANDROID
#include <QtAndroid>
#include <QAndroidJniObject>
#include <QAndroidService>
#include <QRemoteObjectNode>
#include "rep_playback_source.h"
#include "rep_playback_replica.h"

class PlaybackSourceImpl : public PlaybackSource
{
public:
    PlaybackSourceImpl()
    {
        connect(&m_playback, &PlaybackImpl::hasSongChanged, this, &PlaybackSourceImpl::hasSongChanged);
        connect(&m_playback, &PlaybackImpl::currentSongChanged, this, &PlaybackSourceImpl::currentSongChanged);
        connect(&m_playback, &PlaybackImpl::durationChanged, this, &PlaybackSourceImpl::durationChanged);
        connect(&m_playback, &PlaybackImpl::positionChanged, this, &PlaybackSourceImpl::positionChanged);
        connect(&m_playback, &PlaybackImpl::playingChanged, this, &PlaybackSourceImpl::playingChanged);
    }

    bool playing() const override { return m_playback.playing(); }
    bool hasSong() const override { return m_playback.hasSong(); }
    int duration() const override { return m_playback.duration(); }
    float position() const override { return m_playback.position(); }

public slots:
    void setPlaylist(const QVector<QUrl> &urls) override
    {
        m_playback.setPlaylist(urls);
    }

    void addSong(const QUrl &url) override
    {
        m_playback.addSong(url);
    }

    void removeSong(int pos) override
    {
        m_playback.removeSong(pos);
    }

    void insertSong(int pos, const QUrl &url) override
    {
        m_playback.insertSong(pos, url);
    }

    void jump(int index) override
    {
        m_playback.jump(index);
    }

    void step(int delta) override
    {
        m_playback.step(delta);
    }

    void seek(float position) override
    {
        m_playback.seek(position);
    }

    void play() override
    {
        m_playback.play();
    }

    void pause() override
    {
        m_playback.pause();
    }

private:
    PlaybackImpl m_playback;
};

int runPlaybackService(int argc, char **argv)
{
    QAndroidService app(argc, argv);

    qWarning() << "PlaybackService: Starting";

    QRemoteObjectHost srcNode(QUrl(QStringLiteral("local:replica")));
    PlaybackSourceImpl playbackServer;
    srcNode.enableRemoting(&playbackServer);

    const int ret = app.exec();
    qWarning() << "PlaybackService: ended:" << ret;

    return ret;
}

PlaybackReplica *connectToPlaybackService()
{
    static QRemoteObjectNode repNode;

    repNode.connectToNode(QUrl(QStringLiteral("local:replica")));
    PlaybackReplica *rep = repNode.acquire<PlaybackReplica>();
    bool res = rep->waitForSource();
    qWarning() << "Connection to PlaybackService:" << res << rep->state();

    return rep;
}

void startPlaybackService()
{
    QAndroidJniObject::callStaticMethod<void>("de/wheeland/moosick/PlaybackService",
                                              "startMyService",
                                              "(Landroid/content/Context;)V",
                                              QtAndroid::androidActivity().object());
}

#endif

PlaybackImpl::PlaybackImpl(QObject *parent)
    : QObject(parent)
    , m_player(new QMediaPlayer(this))
    , m_playlist(new QMediaPlaylist(this))
{
    m_player->setPlaylist(m_playlist);
    m_playlist->setPlaybackMode(QMediaPlaylist::Sequential);

    connect(m_player, &QMediaPlayer::positionChanged, this, [=]() {
        emit positionChanged(position());
    });
    connect(m_player, &QMediaPlayer::durationChanged, this, &PlaybackImpl::durationChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &PlaybackImpl::onMediaStatusChanged);
    connect(m_player, &QMediaPlayer::stateChanged, this, &PlaybackImpl::onMediaStatusChanged);

    connect(m_playlist, &QMediaPlaylist::currentIndexChanged, this, &PlaybackImpl::onCurrentIndexChanged);
}

PlaybackImpl::~PlaybackImpl()
{
}

bool PlaybackImpl::playing() const
{
    return m_isPlaying;
}

bool PlaybackImpl::hasSong() const
{
    return !m_urls.isEmpty();
}

int PlaybackImpl::duration() const
{
    return m_player->duration();
}

float PlaybackImpl::position() const
{
    return (float) m_player->position() / (float) m_player->duration();
}

void PlaybackImpl::setPlaylist(const QVector<QUrl> &urls)
{
    QList<QMediaContent> media;
    for (const QUrl& url : urls) {
        media << QMediaContent(url);
    }

    m_urls = urls;
    m_playlist->clear();
    m_playlist->addMedia(media);
    emit hasSongChanged(hasSong());
}

void PlaybackImpl::addSong(const QUrl &url)
{
    m_urls << url;
    m_playlist->addMedia(QMediaContent(url));
    emit hasSongChanged(hasSong());
}

void PlaybackImpl::removeSong(int pos)
{
    m_urls.remove(pos);
    m_playlist->removeMedia(pos);
    emit hasSongChanged(hasSong());
}

void PlaybackImpl::insertSong(int pos, const QUrl &url)
{
    m_urls.insert(pos, url);
    m_playlist->insertMedia(pos, QMediaContent(url));
    emit hasSongChanged(hasSong());
}

void PlaybackImpl::jump(int index)
{
    m_playlist->setCurrentIndex(index);
}

void PlaybackImpl::step(int delta)
{
    if (delta > 0)
        m_playlist->setCurrentIndex(m_playlist->nextIndex(delta));
    else if (delta < 0)
        m_playlist->setCurrentIndex(m_playlist->previousIndex(-delta));
}

void PlaybackImpl::seek(float position)
{
    m_player->setPosition(position * m_player->duration());
}

void PlaybackImpl::play()
{
    m_player->play();
}

void PlaybackImpl::pause()
{
    m_player->pause();
}

void PlaybackImpl::onMediaStatusChanged()
{
    const bool isPlaying = (m_player->state() == QMediaPlayer::PlayingState);
    if (m_isPlaying != isPlaying) {
        m_isPlaying = isPlaying;
        emit playingChanged(isPlaying);
    }
}

void PlaybackImpl::onCurrentIndexChanged(int index)
{
    const QUrl url = m_urls.value(index, QUrl());
    emit currentSongChanged(index, url);
}
