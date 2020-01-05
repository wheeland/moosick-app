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
        const QVector<Search::BandcampTrackResult*> tracks = album->tracks();
        if (!tracks.isEmpty()) {
            if (append) {
                for (auto it = tracks.begin(); it != tracks.end(); ++it)
                    queueBandcampTrack(*it, true);
            } else {
                for (auto it = tracks.rbegin(); it != tracks.rend(); ++it)
                    queueBandcampTrack(*it, false);
            }
        }
        // otherwise, we'll need to schedule a download, and wait for the result
        else {

        }

        break;
    }
    case Search::Result::BandcampTrack: {
//        queueBandcampTrack(result, append);
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

void Controller::queueBandcampTrack(Search::BandcampTrackResult *track, bool append)
{
//    m_playlist->append(Playlist::Entry::Bandcamp, track->);
}
