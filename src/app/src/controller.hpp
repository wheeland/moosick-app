#pragma once

#include "search.hpp"
#include "playlist.hpp"

class Controller : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Search::Query *search READ search CONSTANT)
    Q_PROPERTY(Playlist::Playlist *playlist READ playlist CONSTANT)

public:
    Controller(QObject *parent = nullptr);
    ~Controller() override;

    Playlist::Playlist *playlist() const { return m_playlist; }
    Search::Query *search() const { return m_search; }

    Q_INVOKABLE void addToPlaylist(Search::Result *result, bool append);
    Q_INVOKABLE void download(Search::Result *result);

private:
    bool queueBandcampAlbum(Search::BandcampAlbumResult *album, bool append);
    void queueBandcampTrack(Search::BandcampTrackResult *track, bool append);

    Playlist::Playlist *m_playlist;
    Search::Query *m_search;
};
