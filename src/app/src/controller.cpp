#include "controller.hpp"

#include <QDebug>

#ifdef Q_OS_ANDROID
#include <QRemoteObjectNode>
#endif // Q_OS_ANDROID

using Database::DbItem;
using Database::DbArtist;
using Database::DbAlbum;
using Database::DbSong;

Controller::Controller(QObject *parent)
    : QObject(parent)
{
    m_storage = new Storage();
    m_httpClient = new HttpClient(this);

#ifdef Q_OS_ANDROID
    m_playback = connectToPlaybackService();
#else // Q_OS_ANDROID
    m_playback = new Playback();
#endif // Q_OS_ANDROID

    m_playlist = new Playlist::Playlist(m_httpClient, m_playback, this);
    m_search = new Search::Query(m_httpClient, this);

    m_httpClient->setApiUrl(m_storage->host());
    m_httpClient->setPort(m_storage->port());
    m_httpClient->setUser(m_storage->userName());
    m_httpClient->setPass(m_storage->password());
    connect(m_httpClient, &HttpClient::apiUrlChanged, [=]() { m_storage->writeHost(m_httpClient->apiUrl()); });
    connect(m_httpClient, &HttpClient::portChanged, [=]() { m_storage->writePort(m_httpClient->port()); });
    connect(m_httpClient, &HttpClient::userChanged, [=]() { m_storage->writeUserName(m_httpClient->user()); });
    connect(m_httpClient, &HttpClient::passChanged, [=]() { m_storage->writePassword(m_httpClient->pass()); });

    // remember all SSL errors that the user chose to ignore
    m_httpClient->setIgnoredSslErrorData(m_storage->ignoredSslErrorData());
    connect(m_httpClient, &HttpClient::pendingSslErrorChanged, this, [=]() {
        m_storage->writeIgnoredSslErrorData(m_httpClient->ignoredSslErrorData());
    });

    Moosick::Library library;
    if (m_storage->readLibrary(library)) {
        m_database = new Database::DatabaseInterface(library, m_httpClient, this);
    }
    else {
        m_database = new Database::DatabaseInterface(m_httpClient, this);

        // when we receive the first library, store it to disk, but only once please
        connect(m_database, &Database::DatabaseInterface::hasLibraryChanged, [=]() {
            m_storage->writeLibrary(m_database->library());
            disconnect(m_database, &Database::DatabaseInterface::hasLibraryChanged, this, 0);
        });
    }
    m_database->sync();
}

Controller::~Controller()
{
    if (m_database->hasLibrary())
        m_storage->writeLibrary(m_database->library());

    // order please!
    delete m_search;
    // TODO: delete playback
    delete m_playlist;
    delete m_httpClient;
    delete m_storage;
}

void Controller::addSearchResultToPlaylist(Search::Result *result, bool append)
{
    if (!result)
        return;

    switch (result->resultType()) {
    case Search::Result::BandcampAlbum: {
        Search::BandcampAlbumResult *album = qobject_cast<Search::BandcampAlbumResult*>(result);
        if (!album) return;

        // if the album songs are already downloaded, add them now
        if (!queueBandcampAlbum(album, append)) {
            // otherwise, we'll need to schedule a download, and wait for the result
            album->queryInfo();
            connect(album, &Search::BandcampAlbumResult::statusChanged, this, [=](Search::Result::Status status) {
                if (status == Search::Result::Done)
                    queueBandcampAlbum(album, append);
            });
        }

        break;
    }
    case Search::Result::BandcampTrack: {
        Search::BandcampTrackResult *track = qobject_cast<Search::BandcampTrackResult*>(result);
        if (track)
            queueBandcampTrack(track, append);
        return;
    }
    case Search::Result::YoutubeVideo: {
        Search::YoutubeVideoResult *video = qobject_cast<Search::YoutubeVideoResult*>(result);
        if (!video) return;

        // if we don't know the audio URL yet, request it, and queue it afterwards
        if (video->audioUrl().isEmpty()) {
            video->queryInfo();
            connect(video, &Search::YoutubeVideoResult::hasDetails, this, [=]() {
                queueYoutubeVideo(video, append);
            });
        }
        else {
            queueYoutubeVideo(video, append);
        }
        return;
    }
    case Search::Result::BandcampArtist:
    default:
        break;
    }
}

void Controller::addLibraryItemToPlaylist(DbItem *item, bool append)
{
    QVector<DbSong*> songs;
    const auto addAlbum = [&](DbAlbum *album) { songs << album->songs(); };
    const auto addArtist = [&](DbArtist *artist) {
        for (DbAlbum *album : artist->albums())
            addAlbum(album);
    };

    if (DbArtist *artist = qobject_cast<DbArtist*>(item)) {
        m_database->fillArtistInfo(artist);
        addArtist(artist);
    }
    else if (DbAlbum *album = qobject_cast<DbAlbum*>(item))
        addAlbum(album);
    else if (DbSong *song = qobject_cast<DbSong*>(item))
        songs << song;

    for (DbSong *song : songs) {
        m_playlist->addFromLibrary(
            song->filePath(), song->artistName(), song->albumName(), song->name(), song->secs(), append
        );
    }
}

void Controller::download(Search::Result *result)
{
    MoosickMessage::DownloadRequest request;

    switch (result->resultType()) {
    case Search::Result::BandcampAlbum: {
        Search::BandcampAlbumResult *bc = qobject_cast<Search::BandcampAlbumResult*>(result);
        Q_ASSERT(bc);
        request.requestType = MoosickMessage::DownloadRequestType::BandcampAlbum;
        request.url = bc->url();
        request.artistName = bc->artist();
        request.albumName = bc->title();
        break;
    }
    case Search::Result::YoutubeVideo: {
        Search::YoutubeVideoResult *yt = qobject_cast<Search::YoutubeVideoResult*>(result);
        Q_ASSERT(yt);
        request.requestType = MoosickMessage::DownloadRequestType::YoutubeVideo;
        request.url = yt->url();
        request.albumName = yt->title();
        break;
    }
    default:
        qWarning() << "Not supported";
        return;
    }

    m_database->requestDownload(request, result);
}

QString Controller::formatTimeString(int msecs)
{
    return QString::asprintf("%d:%02d", msecs / 60000, (msecs % 60000) / 1000);
}

bool Controller::queueBandcampAlbum(Search::BandcampAlbumResult *album, bool append)
{
    const QVector<Search::BandcampTrackResult*> tracks = album->tracks();
    if (!tracks.isEmpty()) {
        if (append) {
            for (auto it = tracks.begin(); it != tracks.end(); ++it)
                queueBandcampTrack(*it, true);
        } else {
            for (auto it = tracks.rbegin(); it != tracks.rend(); ++it)
                queueBandcampTrack(*it, false);
        }

        return true;
    }
    return false;
}

void Controller::queueBandcampTrack(Search::BandcampTrackResult *track, bool append)
{
    m_playlist->addFromInternet(
        Playlist::Entry::Bandcamp, track->url(),
        track->album()->artist(), track->album()->title(), track->title(),
        track->secs(), track->iconUrl(), append
    );
}

void Controller::queueYoutubeVideo(Search::YoutubeVideoResult *video, bool append)
{
    m_playlist->addFromInternet(
        Playlist::Entry::Youtube, video->audioUrl(),
        "", "", video->title(), video->secs(), video->iconUrl(), append
    );
}
