#include "controller.hpp"

Controller::Controller(QObject *parent)
    : QObject(parent)
    , m_playlist(new Playlist::Playlist(this))
    , m_search(new Search::Query("localhost", 8080, this))
{
}

Controller::~Controller()
{
}

void Controller::addToPlaylist(Search::Result *result, bool append)
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
    case Search::Result::BandcampArtist:
    default:
        break;
    }
}

void Controller::download(Search::Result *result)
{

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
