#pragma once

#include "search.hpp"
#include "playlist.hpp"
#include "httpclient.hpp"
#include "storage.hpp"
#include "playback.hpp"
#include "database/database.hpp"
#include "database/database_items.hpp"
#include "database/database_interface.hpp"

class Controller : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Search::Query *search READ search CONSTANT)
    Q_PROPERTY(Playlist::Playlist *playlist READ playlist CONSTANT)
    Q_PROPERTY(Database::DatabaseInterface *database READ database CONSTANT)
    Q_PROPERTY(Playback *playback READ playback CONSTANT)
    Q_PROPERTY(HttpClient *httpClient READ httpClient CONSTANT)

public:
    Controller(QObject *parent = nullptr);
    ~Controller() override;

    Playlist::Playlist *playlist() const { return m_playlist; }
    Search::Query *search() const { return m_search; }
    Database::DatabaseInterface *database() const { return m_database; }
    Playback *playback() const { return m_playback; }
    HttpClient *httpClient() const { return m_httpClient; }

    Q_INVOKABLE void addSearchResultToPlaylist(Search::Result *result, bool append);
    Q_INVOKABLE void addLibraryItemToPlaylist(Database::DbItem *item, bool append);
    Q_INVOKABLE void download(Search::Result *result);

    Q_INVOKABLE QString formatTimeString(int msecs);

private:
    bool queueBandcampAlbum(Search::BandcampAlbumResult *album, bool append);
    void queueBandcampTrack(Search::BandcampTrackResult *track, bool append);
    void queueYoutubeVideo(Search::YoutubeVideoResult *video, bool append);

    Storage *m_storage;
    HttpClient *m_httpClient;
    Database::DatabaseInterface *m_database;
    Playback *m_playback;
    Playlist::Playlist *m_playlist;
    Search::Query *m_search;
};
