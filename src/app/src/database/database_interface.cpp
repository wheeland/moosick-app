#include "database.hpp"
#include "database_items.hpp"
#include "database_interface.hpp"
#include "jsonconv.hpp"
#include "../search.hpp"
#include "../selecttagsmodel.hpp"
#include "../stringmodel.hpp"

// let the Database be consistent and do ALL allocations
// the QObjects are just exposure to QML, all the data lies in the Database
// when tags change, all the classes need to get them anew from the DB

namespace Database {

DatabaseInterface::DatabaseInterface(HttpClient *httpClient, QObject *parent)
    : QObject(parent)
    , m_db(new Database(httpClient, parent))
    , m_tagsModel(new SelectTagsModel(this))
    , m_filterTagsModel(new SelectTagsModel(this))
    , m_editArtistStringList(new StringModel(this))
{
    m_searchResults.addAccessor("artist", [&](const SearchResultArtist &artist) {
        return QVariant::fromValue(artist.artist);
    });

    connect(m_editArtistStringList, &StringModel::selected, this, &DatabaseInterface::onArtistStringSelected);

    connect(m_db, &Database::libraryChanged, this, &DatabaseInterface::onLibraryChanged);
    connect(m_db, &Database::libraryChanged, this, &DatabaseInterface::hasLibraryChanged);
    connect(m_db, &Database::changesPendingChanged, this, &DatabaseInterface::changesPendingChanged);
    connect(m_db, &Database::downloadsPendingChanged, this, &DatabaseInterface::downloadsPendingChanged);
    connect(m_db, &Database::isSyncingChanged, this, &DatabaseInterface::isSyncingChanged);

    connect(m_filterTagsModel, &SelectTagsModel::selectionChanged, this, [=]() { updateSearchResults(); });
}

DatabaseInterface::DatabaseInterface(const Moosick::Library &library, HttpClient *httpClient, QObject *parent)
    : DatabaseInterface(httpClient, parent)
{
    m_db->setLibrary(library);
}

DatabaseInterface::~DatabaseInterface()
{
}

const Moosick::Library &DatabaseInterface::library() const
{
    return m_db->library();
}

void DatabaseInterface::sync()
{
    m_db->sync();
}

void DatabaseInterface::onLibraryChanged()
{
    repopulateTagsModel();
    updateSearchResults();
    repopulateEditStringList();
}

DbTag *DatabaseInterface::tagForTagId(Moosick::TagId tagId) const
{
    return m_tags.value(tagId, nullptr);
}

DbTag *DatabaseInterface::getOrCreateDbTag(Moosick::TagId tagId)
{
    Q_ASSERT(tagId.isValid());

    auto it = m_tags.find(tagId);

    if (it == m_tags.end()) {
        it = m_tags.insert(tagId, new DbTag(this, tagId));

        // link to parent
        const Moosick::TagId parentId = tagId.parent(m_db->library());
        if (parentId.isValid()) {
            DbTag *parent = getOrCreateDbTag(parentId);
            parent->addChildTag(it.value());
            it.value()->setParentTag(parent);
        }

        // link to children
        for (const Moosick::TagId &childId : tagId.children(m_db->library())) {
            DbTag *child = getOrCreateDbTag(childId);
            it.value()->addChildTag(child);
        }
    }

    return it.value();
}

void DatabaseInterface::search(const QString &searchString)
{
    if (m_searchString == searchString)
        return;

    m_searchString = searchString;
    m_searchKeywords = searchString.split(" ");

    updateSearchResults();

    emit searchStringChanged(m_searchString);
}

void DatabaseInterface::fillArtistInfo(DbArtist *artist)
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
        DbAlbum *album = new DbAlbum(this, albumResult.albumId, artist);
        album->setSongs(albumResult.songs);
        artist->addAlbum(album);
    }

    artist->dataSet();
}

void DatabaseInterface::changeAlbumArtist(DbAlbum *album)
{
    Q_ASSERT(m_editItemType == EditNone);
    Q_ASSERT(m_editItemSource == SourceNone);

    m_editItemType = ChangeAlbumArtist;
    m_editItemSource = SourceLibrary;
    m_editArtistStringList->popup(album->artist()->name());
    m_editedItemId = album->id();

    emit stateChanged();
}

void DatabaseInterface::editItem(DbItem *item)
{
    Q_ASSERT(m_editItemType == EditNone);
    Q_ASSERT(m_editItemSource == SourceNone);

    if (qobject_cast<DbArtist*>(item)) {
        m_editItemType = EditArtist;
    } else if (qobject_cast<DbAlbum*>(item)) {
        m_editItemType = EditAlbum;
    } else if (qobject_cast<DbSong*>(item)) {
        m_editItemType = EditSong;
    } else if (DbTag *tag = qobject_cast<DbTag*>(item)) {
        m_editItemType = EditTag;
        m_tagsModel->setSelectedTagIds({ tag->id().parent(library()) }, false);
    } else {
        m_editItemType = EditNone;
    }

    if (m_editItemType != EditNone) {
        m_editArtistStringList->popup(item->name());
        if (DbTaggedItem *taggedItem = qobject_cast<DbTaggedItem*>(item))
            m_tagsModel->setSelectedTagIds(taggedItem->tagIds(), true);
    }

    m_editItemSource = (m_editItemType != EditNone) ? SourceLibrary : SourceNone;
    m_editedItemId = (item && (m_editItemType != EditNone)) ? item->id() : 0;

    emit stateChanged();
}

void DatabaseInterface::requestDownload(const MoosickMessage::DownloadRequest &request, Search::Result *result)
{
    if (!m_requestedDownload.isNull() || (m_editItemType != EditNone) || (m_editItemSource != SourceNone))
        return;

    m_requestedDownload.reset(new DownloadRequest { request, Moosick::TagIdList(), result });

    m_editItemType = EditArtist;
    m_editItemSource = (request.requestType == MoosickMessage::DownloadRequestType::BandcampAlbum) ? SourceBandcamp : SourceYoutube;
    m_editedItemId = 0;

    m_editArtistStringList->popup(request.artistName);
    m_tagsModel->setSelectedTagIds({}, true);

    emit stateChanged();
}

void DatabaseInterface::removeItem(DbItem *item)
{
    if (!m_itemToBeRemoved.isNull() || editItemVisible())
        return;

    m_itemToBeRemoved = item;
    emit confirmationChanged();
}

void DatabaseInterface::addNewTag()
{
    Q_ASSERT(m_editItemType == EditNone);
    Q_ASSERT(m_editItemSource == SourceNone);

    m_editItemType = AddNewTag;
    m_editItemSource = SourceLibrary;
    m_editedItemId = 0;
    m_editArtistStringList->popup("");
    m_tagsModel->setSelectedTagIds({}, false);

    emit stateChanged();
}

QString DatabaseInterface::confirmationText() const
{
    if (m_itemToBeRemoved.isNull())
        return "";

    QString ret = QString("Really remove ") + m_itemToBeRemoved->name() + "?";

    // if we remove a tag that is still being used, show a warning
    if (DbTag *tag = qobject_cast<DbTag*>(m_itemToBeRemoved)) {
        QStringList stillUsing;
        const auto addStillUsing = [&](const char *name, int count) {
            if (count > 0)
                stillUsing << QString::number(count) + " " + name;
        };
        addStillUsing("Artists", tag->id().artists(library()).size());
        addStillUsing("Albums", tag->id().albums(library()).size());
        addStillUsing("Songs", tag->id().songs(library()).size());
        if (!stillUsing.isEmpty())
            ret += QString("\n\n") + stillUsing.join(", ") + " are still using this tag.";
    }

    return ret;
}

bool DatabaseInterface::confirmationVisible() const
{
    return !m_itemToBeRemoved.isNull();
}

void DatabaseInterface::confirm(bool ok)
{
    if (ok) {
        if (DbArtist *artist = qobject_cast<DbArtist*>(m_itemToBeRemoved))
            m_db->removeArtist(artist->id());
        else if (DbAlbum *album = qobject_cast<DbAlbum*>(m_itemToBeRemoved))
            m_db->removeAlbum(album->id());
        else if (DbSong *song = qobject_cast<DbSong*>(m_itemToBeRemoved))
            m_db->removeSong(song->id());
        else if (DbTag *tag = qobject_cast<DbTag*>(m_itemToBeRemoved))
            m_db->removeTag(tag->id(), true);
    }

    m_itemToBeRemoved.clear();
    emit confirmationChanged();
}

void DatabaseInterface::onArtistStringSelected(int id)
{
    if (id < 0)
        return;

    switch (m_editItemType) {
    case EditArtist:
        m_tagsModel->setSelectedTagIds(Moosick::ArtistId(id).tags(m_db->library()), true);
        break;
    case EditNone:
    default:
        break;
    }
}

void DatabaseInterface::editOkClicked()
{
    const int selectedId = m_editArtistStringList->selectedId();
    const QString enteredName = m_editArtistStringList->enteredString();
    const Moosick::TagIdList selectedTags = m_tagsModel->selectedTagsIds();

    if (m_editItemSource == SourceBandcamp) {
        Q_ASSERT(!m_requestedDownload.isNull());

        if (m_editItemType == EditArtist) {
            if (selectedId > 0) {
                m_requestedDownload->request.artistId = selectedId;
            } else {
                m_requestedDownload->request.artistName = enteredName;
                m_requestedDownload->albumTags = selectedTags;
            }
        }

        m_editItemType = EditNone;
        m_editItemSource = SourceNone;

        m_db->download(m_requestedDownload->request, m_requestedDownload->albumTags);
        m_requestedDownload.reset();
    }

    if (m_editItemSource == SourceYoutube) {
        Q_ASSERT(!m_requestedDownload.isNull());

        if (m_editItemType == EditArtist) {
            if (selectedId > 0) {
                m_requestedDownload->request.artistId = selectedId;
            } else {
                m_requestedDownload->request.artistName = enteredName;
                m_requestedDownload->albumTags = selectedTags;
            }
            m_editItemType = EditAlbum;
            m_editArtistStringList->popup("");
        }
        else if (m_editItemType == EditAlbum) {
            m_editItemType = EditNone;
            m_editItemSource = SourceNone;

            m_requestedDownload->request.albumName = m_editArtistStringList->enteredString();
            m_db->download(m_requestedDownload->request, m_requestedDownload->albumTags);
            m_requestedDownload.reset();
        }
    }

    if (m_editItemSource == SourceLibrary) {
        Q_ASSERT((m_editItemType == AddNewTag) || (m_editedItemId > 0));

        if ((m_editItemType == ChangeAlbumArtist) && (selectedId > 0)) {
            m_db->setAlbumArtist(m_editedItemId, selectedId);
        }
        else if (m_editItemType == AddNewTag && !enteredName.isEmpty()) {
            m_db->addTag(enteredName, selectedTags.value(0, 0));
        }
        else {
            const HttpRequestId reply =
                (m_editItemType == EditArtist) ? m_db->setArtistDetails(m_editedItemId, enteredName, selectedTags) :
                (m_editItemType == EditAlbum) ? m_db->setAlbumDetails(m_editedItemId, enteredName, selectedTags) :
                (m_editItemType == EditSong) ? m_db->setSongDetails(m_editedItemId, enteredName, selectedTags) :
                (m_editItemType == EditTag) ? m_db->setTagDetails(m_editedItemId, enteredName, selectedTags.value(0, 0)) : 0;
            Q_UNUSED(reply)
        }

        m_editItemType = EditNone;
        m_editItemSource = SourceNone;
    }

    emit stateChanged();
}

void DatabaseInterface::editCancelClicked()
{
    m_requestedDownload.reset();
    m_editedItemId = 0;
    m_editItemType = EditNone;
    m_editItemSource = SourceNone;
    emit stateChanged();
}

QVector<DatabaseInterface::SearchResultArtist> DatabaseInterface::computeSearchResults() const
{
    const Moosick::Library &lib = library();
    QVector<SearchResultArtist> ret;

    const Moosick::TagIdList filterTags = m_filterTagsModel->selectedTagsIds();
    const auto matchesTags = [=](const Moosick::TagIdList &tags) {
        if (filterTags.isEmpty())
            return true;
        return std::any_of(tags.begin(), tags.end(), [&](Moosick::TagId tag) { return filterTags.contains(tag); });
    };

    const auto matchesSearch = [=](const QString &name) {
        const QString lowerName = name.toLower();
        return std::all_of(m_searchKeywords.cbegin(), m_searchKeywords.cend(), [&](const QString &keyword) {
            return lowerName.contains(keyword);
        });
    };

    // get all results for new search keywords
    for (const Moosick::ArtistId artistId : m_db->library().artistsByName()) {
        const bool artistHasTag = matchesTags(artistId.tags(lib));
        const bool artistHasString = matchesSearch(artistId.name(lib));
        const bool includeArtist = artistHasTag && artistHasString;

        SearchResultArtist artist;
        artist.id = artistId;

        for (const Moosick::AlbumId albumId : artistId.albums(lib)) {
            const bool includeAlbum = includeArtist
                    || (matchesTags(albumId.tags(lib)) && matchesSearch(albumId.name(lib)));

            SearchResultAlbum album;
            album.albumId = albumId;

            for (const Moosick::SongId songId : albumId.songs(lib)) {
                const bool includeSong = includeAlbum
                        || (matchesTags(songId.tags(lib)) && matchesSearch(songId.name(lib)));
                if (includeSong)
                    album.songs << songId;
            }

            if (!album.songs.isEmpty())
                artist.albums << album;
        }

        if (includeArtist || !artist.albums.isEmpty())
            ret << artist;
    }

    return ret;
}

void DatabaseInterface::updateSearchResults()
{
    const QVector<SearchResultArtist> newSearchResults = computeSearchResults();

    const auto updateSearchResultArtist = [=](SearchResultArtist &dst, const SearchResultArtist &src) {
        Q_ASSERT(dst.artist);
        dst.albums = src.albums;

        emit dst.artist->libraryChanged();

        // if this artist wasn't yet filled with details for the overview, ignore it
        if (!dst.artist->hasData())
            return;

        // add new albums
        QVector<DbAlbum*> existingAlbums = dst.artist->albums();
        for (const SearchResultAlbum &newAlbum : src.albums) {
            const bool exists = (0 < std::count_if(existingAlbums.begin(), existingAlbums.end(), [=](const DbAlbum *album) {
                return album->id() == newAlbum.albumId;
            }));
            if (!exists)
                dst.artist->addAlbum(new DbAlbum(this, newAlbum.albumId, dst.artist));
        }

        // remove stale albums, and set song info for the valid ones
        existingAlbums = dst.artist->albums();
        for (DbAlbum *existingAlbum : qAsConst(existingAlbums)) {
            const auto dstAlbum = std::find_if(src.albums.begin(), src.albums.end(), [=](const SearchResultAlbum &album) {
                return album.albumId == existingAlbum->id();
            });
            if (dstAlbum == src.albums.end()) {
                dst.artist->removeAlbum(existingAlbum);
            } else {
                existingAlbum->setSongs(dstAlbum->songs);
            }
            emit existingAlbum->libraryChanged();
        }
    };

    const auto getArtistIndex = [=](Moosick::ArtistId artistId) {
        for (int i = 0; i < m_searchResults.size(); ++i) {
            if (m_searchResults[i].artist->id() == artistId)
                return i;
        }
        return -1;
    };

    // one-by-one move the existing results into place, or create new ones
    for (int i = 0; i < newSearchResults.size(); ++i) {
        const Moosick::ArtistId artistId = newSearchResults[i].id;

        if (i >= m_searchResults.size()) {
            m_searchResults.add(SearchResultArtist{ artistId, new DbArtist(this, artistId), {} });
        }
        else {
            const int existingIdx = getArtistIndex(newSearchResults[i].id);
            if (existingIdx != i) {
                if (existingIdx >= 0)
                    // if we already have that result, just move it into the correct place
                    m_searchResults.move(existingIdx, i);
                else
                    // we don't have that artist yet, so create it now
                    m_searchResults.insert(i, SearchResultArtist{ artistId, new DbArtist(this, artistId), {} });
            }
        }

        updateSearchResultArtist(m_searchResults[i], newSearchResults[i]);
    }

    // 2. remove all excess artists
    for (int i = m_searchResults.size() - 1; i >= newSearchResults.size(); --i) {
        m_searchResults[i].artist->deleteLater();
        m_searchResults.remove(i);
    }
}

void DatabaseInterface::repopulateEditStringList()
{
    m_editArtistStringList->clear();
    for (const Moosick::ArtistId &artistId : m_db->library().artistsByName()) {
        m_editArtistStringList->add((int) artistId, artistId.name(m_db->library()));
    }
}

void DatabaseInterface::repopulateTagsModel()
{
    // delete all tags and re-create them
    m_tagsModel->clear();
    m_filterTagsModel->clear();
    for (DbTag *tag : m_tags)
        tag->deleteLater();
    m_tags.clear();

    for (const Moosick::TagId &tagId : m_db->library().rootTags()) {
        DbTag *rootTag = getOrCreateDbTag(tagId);
        m_tagsModel->addTag(rootTag);
        m_filterTagsModel->addTag(rootTag);
    }
}

bool DatabaseInterface::editItemVisible() const
{
    return (m_editItemType != EditNone) && (m_editItemSource != SourceNone);
}

bool DatabaseInterface::editItemStringsChoiceActive() const
{
    return (m_editItemType == EditArtist || m_editItemType == ChangeAlbumArtist) && (m_editItemSource != SourceNone);
}

} // namespace Database
