#include "database.hpp"
#include "database_items.hpp"
#include "jsonconv.hpp"
#include "search.hpp"

using NetCommon::DownloadRequest;

// let the Database be consistent and do ALL allocations
// the QObjects are just exposure to QML, all the data lies in the Database
// when tags change, all the classes need to get them anew from the DB

namespace Database {

DbItem::DbItem(Database *db, DbItem::Type tp, quint32 id)
    : QObject(db)
    , m_database(db)
    , m_type(tp)
    , m_id(id)
{
}

const Moosick::Library &DbItem::library() const
{
    return m_database->library();
}

DbTag::DbTag(Database *db, Moosick::TagId tag)
    : DbItem(db, DbItem::Tag, tag)
    , m_tag(tag)
{
    m_childTags.addValueAccessor("tag");
}

DbTaggedItem::DbTaggedItem(Database *db, DbItem::Type tp, quint32 id, const Moosick::TagIdList &tags)
    : DbItem(db, tp, id)
{
    m_tags.addValueAccessor("tag");
    for (Moosick::TagId tagId : tags) {
        DbTag *tag = db->tagForTagId(tagId);
        m_tags.add(tag);
        connect(tag, &QObject::destroyed, this, [=]() { removeTag(tag); });
    }
}

DbArtist::DbArtist(Database *db, Moosick::ArtistId artist)
    : DbTaggedItem(db, DbItem::Artist, artist, artist.tags(db->library()))
    , m_artist(artist)
{
    m_albums.addValueAccessor("album");
}

DbArtist::~DbArtist()
{
    qDeleteAll(m_albums.data());
}

DbAlbum::DbAlbum(Database *db, Moosick::AlbumId album)
    : DbTaggedItem(db, DbItem::Album, album, album.tags(db->library()))
    , m_album(album)
{
    m_songs.addValueAccessor("song");
}

DbAlbum::~DbAlbum()
{
    qDeleteAll(m_songs.data());
}

QString DbAlbum::durationString() const
{
    const QVector<DbSong*> &songs = m_songs.data();
    const int secs = std::accumulate(songs.begin(), songs.end(), 0, [&](int sum, DbSong *song) {
        return sum + song->secs();
    });
    return QString::asprintf("%d:%02d", secs/60, secs%60);
}

void DbAlbum::addSong(DbSong *song)
{
    m_songs.addExclusive(song);
    emit songsChanged();
}

void DbAlbum::removeSong(DbSong *song)
{
    m_songs.remove(song);
    emit songsChanged();
}

DbSong::DbSong(Database *db, Moosick::SongId song)
    : DbTaggedItem(db, DbItem::Song, song, song.tags(db->library()))
    , m_song(song)
{
}

QString DbSong::durationString() const
{
    const uint secs = m_song.secs(library());
    return QString::asprintf("%d:%02d", secs/60, secs%60);
}

Database::Database(HttpClient *httpClient, QObject *parent)
    : QObject(parent)
    , m_http(new HttpRequester(httpClient, this))
    , m_tagsModel(new SelectTagsModel(this))
    , m_editStringList(new StringModel(this))
{
    connect(m_http, &HttpRequester::receivedReply, this, &Database::onNetworkReplyFinished);

    m_searchResults.addAccessor("artist", [&](const SearchResultArtist &artist) {
        return QVariant::fromValue(artist.artist);
    });

    connect(m_editStringList, &StringModel::selected, this, &Database::onStringSelected);
}

Database::~Database()
{
}

void Database::sync()
{
    if (hasRunningRequestType(LibrarySync) || m_hasLibrary)
        return;

    QNetworkReply *reply = m_http->requestFromServer("/lib.do", "");
    m_requests[reply] = LibrarySync;
}

void Database::onNetworkReplyFinished(QNetworkReply *reply, QNetworkReply::NetworkError error)
{
    const RequestType requestType = m_requests.take(reply);
    const QByteArray data = reply->readAll();
    const bool hasError = (error != QNetworkReply::NoError);

    if (hasError) {
        qWarning() << reply->errorString();
    }

    switch (requestType) {
    case LibrarySync: {
        if (!hasError) {
            const QJsonObject obj = parseJsonObject(data, "Library");
            if (m_library.deserializeFromJson(obj))
                onNewLibrary();
        }
        break;
    }
    case BandcampDownload: {
        const auto downloadIt = std::find_if(m_runningDownloads.begin(), m_runningDownloads.end(), [&](const Download &download) {
            return download.networkReply == reply;
        });

        if (!hasError) {
            applyLibraryChanges(data);
        }

        if (downloadIt != m_runningDownloads.end()) {
            if (downloadIt->searchResult && !hasError)
                downloadIt->searchResult->setDownloadStatus(Search::Result::DownloadDone);
            // TODO: add downloadIt->artistTags to downloadIt->query.artistId
            m_runningDownloads.erase(downloadIt);
        }

        break;
    }
    default:
        break;
    }
}

void Database::onNewLibrary()
{
    repopulateTagsModel();
    repopulateSearchResults();
    repopulateEditStringList();

    m_hasLibrary = true;
    emit hasLibraryChanged();
}

bool Database::applyLibraryChanges(const QByteArray &changesJsonData)
{
    const QJsonArray changesJson = parseJsonArray(changesJsonData, "Library Changes");
    QVector<Moosick::CommittedLibraryChange> changes;
    if (!fromJson(changesJson, changes)) {
        qWarning() << "Unable to parse server library changes from JSON:";
        qWarning().noquote() << changesJsonData;
        return false;
    }

    bool hasChanged = false;

    // add into existing committed changes and apply as many as possible, and delete duplicates
    m_waitingChanges << changes;
    qSort(m_waitingChanges.begin(), m_waitingChanges.end(), [](const Moosick::CommittedLibraryChange &a, const Moosick::CommittedLibraryChange &b) {
        return a.revision <= b.revision;
    });
    for (auto it = m_waitingChanges.begin(); it != m_waitingChanges.end(); /*empty*/) {
        if (it->revision <= m_library.revision()) {
            it = m_waitingChanges.erase(it);
        }
        else if (it->revision == m_library.revision() + 1) {
            m_library.commit(it->change);
            hasChanged = true;
            ++it;
        } else {
            break;
        }
    }

    if (hasChanged) {
        repopulateTagsModel();
        repopulateSearchResults();
        repopulateEditStringList();
    }

    return true;
}

DbTag *Database::tagForTagId(Moosick::TagId tagId) const
{
    return m_tags.value(tagId, nullptr);
}

DbTag *Database::getOrCreateDbTag(Moosick::TagId tagId)
{
    Q_ASSERT(tagId.isValid());

    auto it = m_tags.find(tagId);

    if (it == m_tags.end()) {
        it = m_tags.insert(tagId, new DbTag(this, tagId));

        // link to parent
        const Moosick::TagId parentId = tagId.parent(m_library);
        if (parentId.isValid()) {
            DbTag *parent = getOrCreateDbTag(parentId);
            parent->addChildTag(it.value());
            it.value()->setParentTag(parent);
        }

        // link to children
        for (const Moosick::TagId &childId : tagId.children(m_library)) {
            DbTag *child = getOrCreateDbTag(childId);
            it.value()->addChildTag(child);
        }

        if (!parentId.isValid())
            m_tagsModel->addTag(it.value());
    }

    return it.value();
}

void Database::removeTag(Moosick::TagId tagId)
{
    Q_ASSERT(tagId.isValid());

    const auto it = m_tags.find(tagId);
    if (it == m_tags.end())
        return;

    // remove from parent
    DbTag *parent = tagForTagId(tagId.parent(m_library));
    if (parent)
        parent->removeChildTag(it.value());

    // remove children
    for (const Moosick::TagId &childId : tagId.children(m_library))
        removeTag(childId);

    it.value()->deleteLater();
    m_tags.erase(it);
}

void Database::search(QString searchString)
{
    if (m_searchString == searchString)
        return;

    repopulateSearchResults();

    m_searchString = searchString;
    emit searchStringChanged(m_searchString);
}

void Database::fillArtistInfo(DbArtist *artist)
{
    if (!artist->albums().isEmpty())
        return;

    const QVector<SearchResultArtist> artists = m_searchResults.data();
    auto it = std::find_if(artists.begin(), artists.end(), [=](const SearchResultArtist &sra) {
        return sra.artist == artist;
    });

    Q_ASSERT(it != artists.end());
    Q_ASSERT(it->artist == artist);

    // populate artist with album/songs objects
    for (const SearchResultAlbum &albumResult : it->albums) {
        DbAlbum *album = new DbAlbum(this, albumResult.albumId);

        QVector<DbSong*> songs;
        for (const Moosick::SongId &songId : albumResult.songs)
            songs << new DbSong(this, songId);
        qSort(songs.begin(), songs.end(), [=](DbSong *lhs, DbSong *rhs) {
            return lhs->position() < rhs->position();
        });
        for (DbSong *song : songs)
            album->addSong(song);

        artist->addAlbum(album);
    }
}

void Database::editItem(DbItem *item)
{
    Q_ASSERT(m_editItemType == EditNone);
    Q_ASSERT(m_editItemSource == SourceNone);

    DbArtist *artist = qobject_cast<DbArtist*>(item);
    DbAlbum *album = qobject_cast<DbAlbum*>(item);
    DbSong *song = qobject_cast<DbSong*>(item);

    m_editItemType = artist ? EditArtist : album ? EditAlbum : song ? EditSong : EditNone;
    m_editItemSource = (m_editItemType != EditNone) ? SourceLibrary : SourceNone;
    m_editedItemId = (item && (m_editItemType != EditNone)) ? item->id() : 0;
    emit stateChanged();
}

void Database::startDownload(const DownloadRequest &request, Search::Result *result)
{
    Q_ASSERT(m_requestedDownload.isNull());
    Q_ASSERT(m_editItemType == EditNone);
    Q_ASSERT(m_editItemSource == SourceNone);

    m_requestedDownload.reset(new Download { request, Moosick::TagIdList(), result, nullptr });
    m_requestedDownload->request.currentRevision = m_library.revision();

    m_editItemType = EditArtist;
    m_editItemSource = (request.tp == DownloadRequest::BandcampAlbum) ? SourceBandcamp : SourceYoutube;
    emit stateChanged();
}

void Database::onStringSelected(int id)
{
    if (id < 0)
        return;

    switch (m_editItemType) {
    case EditArtist:
        m_tagsModel->setSelectedTagIds(Moosick::ArtistId(id).tags(m_library));
        break;
    case EditNone:
    default:
        break;
    }
}

void Database::editOkClicked()
{
    const int selectedId = m_editStringList->selectedId();
    const QString enteredName = m_editStringList->enteredString();
    const Moosick::TagIdList selectedTags = m_tagsModel->selectedTagsIds();

    if (m_editItemSource == SourceBandcamp) {
        Q_ASSERT(!m_requestedDownload.isNull());

        if (m_editItemType == EditArtist) {
            if (selectedId > 0) {
                m_requestedDownload->request.artistId = selectedId;
            } else {
                m_requestedDownload->request.artistName = enteredName;
                m_requestedDownload->artistTags = selectedTags;
            }
        }

        m_editItemType = EditNone;
        m_editItemSource = SourceNone;

        startDownload();
    }

    emit stateChanged();
}

void Database::editCancelClicked()
{
    m_requestedDownload.reset();
    m_editItemType = EditNone;
    m_editItemSource = SourceNone;
    emit stateChanged();
}

void Database::startDownload()
{
    Q_ASSERT(!m_requestedDownload.isNull());

    if (m_requestedDownload->searchResult)
        m_requestedDownload->searchResult->setDownloadStatus(Search::Result::DownloadStarted);

    switch (m_requestedDownload->request.tp) {
    case NetCommon::DownloadRequest::BandcampAlbum: {
        const QString query = QString("v=") + m_requestedDownload->request.toBase64();
        QNetworkReply *reply = m_http->requestFromServer("/bandcamp-download.do", query);
        m_requests[reply] = BandcampDownload;
        m_requestedDownload->networkReply = reply;
        break;
    }
    default:
        qFatal("No Such download request supported");
    }

    m_runningDownloads << *m_requestedDownload;
    m_requestedDownload.reset();
}

void Database::clearSearchResults()
{
    for (const SearchResultArtist &artist : m_searchResults.data()) {
        if (artist.artist)
            artist.artist->deleteLater();
    }
    m_searchResults.clear();
}

void Database::repopulateSearchResults()
{
    clearSearchResults();

    QStringList keywords;
    for (const QString &searchWord : m_searchString.split(' '))
        keywords << searchWord.toLower();

    // see if we match the search keywords
    const auto matchesSearchString = [&](const QString &name) {
        if (m_searchString.isEmpty())
            return true;

        const QString lower = name.toLower();
        return std::all_of(keywords.cbegin(), keywords.cend(), [&](const QString &keyword) {
            return lower.contains(keyword);
        });
    };

    for (const Moosick::ArtistId &artistId : m_library.artistsByName()) {
        const bool matches = matchesSearchString(artistId.name(m_library));

        // build search result item
        if (matches) {
            SearchResultArtist artist;
            artist.artist = new DbArtist(this, artistId);
            for (const Moosick::AlbumId &albumId : artistId.albums(m_library))
                artist.albums << SearchResultAlbum { albumId, albumId.songs(m_library) };
            m_searchResults.add(artist);
        }
    }
}

void Database::repopulateEditStringList()
{
    m_editStringList->clear();
    for (const Moosick::ArtistId &artistId : m_library.artistsByName()) {
        m_editStringList->add((int) artistId, artistId.name(m_library));
    }
}

void Database::repopulateTagsModel()
{
    for (const Moosick::TagId &tagId : m_library.rootTags()) {
        DbTag *rootTag = getOrCreateDbTag(tagId);
        m_tagsModel->addTag(rootTag);
    }
}

bool Database::hasRunningRequestType(RequestType requestType) const
{
    return std::any_of(m_requests.begin(), m_requests.end(), [=](RequestType tp) {
        return tp == requestType;
    });
}

bool Database::editItemVisible() const
{
    return (m_editItemType != EditNone) && (m_editItemSource != SourceNone);
}

} // namespace Database
