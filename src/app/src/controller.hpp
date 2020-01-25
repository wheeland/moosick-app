#pragma once

#include "search.hpp"
#include "playlist.hpp"
#include "audio.hpp"
#include "httpclient.hpp"
#include "database/database.hpp"
#include "database/database_items.hpp"
#include "database/database_interface.hpp"

class Controller : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Search::Query *search READ search CONSTANT)
    Q_PROPERTY(Playlist::Playlist *playlist READ playlist CONSTANT)
    Q_PROPERTY(Database::DatabaseInterface *database READ database CONSTANT)
    Q_PROPERTY(Audio *audio READ audio CONSTANT)

public:
    Controller(QObject *parent = nullptr);
    ~Controller() override;

    Playlist::Playlist *playlist() const { return m_playlist; }
    Search::Query *search() const { return m_search; }
    Database::DatabaseInterface *database() const { return m_database; }
    Audio *audio() const { return m_audio; }

    Q_INVOKABLE void addSearchResultToPlaylist(Search::Result *result, bool append);
    Q_INVOKABLE void addLibraryItemToPlaylist(Database::DbItem *item, bool append);
    Q_INVOKABLE void download(Search::Result *result);

private:
    bool queueBandcampAlbum(Search::BandcampAlbumResult *album, bool append);
    void queueBandcampTrack(Search::BandcampTrackResult *track, bool append);
    void queueYoutubeVideo(Search::YoutubeVideoResult *video, bool append);

    HttpClient *m_httpClient;
    Database::DatabaseInterface *m_database;
    Playlist::Playlist *m_playlist;
    Search::Query *m_search;
    Audio *m_audio;
};
