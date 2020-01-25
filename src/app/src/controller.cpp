#include "controller.hpp"

Controller::Controller(QObject *parent)
    : QObject(parent)
    , m_httpClient(new HttpClient("localhost", 8080, this))
    , m_database(new Database::DatabaseInterface(m_httpClient, this))
    , m_playlist(new Playlist::Playlist(m_httpClient, this))
    , m_search(new Search::Query(m_httpClient, this))
    , m_audio(new Audio(m_playlist, this))
{
}

Controller::~Controller()
{
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
        if (video)
            queueYoutubeVideo(video, append);
        return;
    }
    case Search::Result::BandcampArtist:
    default:
        break;
    }
}

void Controller::addLibraryItemToPlaylist(Database::DbItem *item, bool append)
{

}

void Controller::download(Search::Result *result)
{
    NetCommon::DownloadRequest request;

    switch (result->resultType()) {
    case Search::Result::BandcampAlbum: {
        Search::BandcampAlbumResult *bc = qobject_cast<Search::BandcampAlbumResult*>(result);
        Q_ASSERT(bc);
        request = NetCommon::DownloadRequest {
            NetCommon::DownloadRequest::BandcampAlbum,
            bc->url(), 0, bc->artist(), bc->title(), 0
        };
        break;
    }
    default:
        qWarning() << "Not supported";
        return;
    }

    m_database->requestDownload(request, result);
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
    if (append) {
        m_playlist->append(Playlist::Entry::Bandcamp,
                           track->album()->artist(), track->album()->title(), track->title(),
                           track->url(), track->secs(), track->iconUrl());
    } else {
        m_playlist->prepend(Playlist::Entry::Bandcamp,
                           track->album()->artist(), track->album()->title(), track->title(),
                           track->url(), track->secs(), track->iconUrl());
    }
}

void Controller::queueYoutubeVideo(Search::YoutubeVideoResult *video, bool append)
{
    if (append)
        m_playlist->append(Playlist::Entry::Youtube, "", "", video->title(), video->url(), video->secs(), video->iconUrl());
    else
        m_playlist->prepend(Playlist::Entry::Youtube, "", "", video->title(), video->url(), video->secs(), video->iconUrl());
}
