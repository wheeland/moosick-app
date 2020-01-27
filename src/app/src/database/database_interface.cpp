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
    , m_editStringList(new StringModel(this))
{
    m_searchResults.addAccessor("artist", [&](const SearchResultArtist &artist) {
        return QVariant::fromValue(artist.artist);
    });

    connect(m_editStringList, &StringModel::selected, this, &DatabaseInterface::onStringSelected);

    connect(m_db, &Database::libraryChanged, this, &DatabaseInterface::onLibraryChanged);
    m_db->sync();
}

DatabaseInterface::~DatabaseInterface()
{
}

const Moosick::Library &DatabaseInterface::library()
{
    return m_db->library();
}

void DatabaseInterface::onLibraryChanged()
{
    repopulateTagsModel();
    repopulateSearchResults();
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

        if (!parentId.isValid())
            m_tagsModel->addTag(it.value());
    }

    return it.value();
}

void DatabaseInterface::removeTag(Moosick::TagId tagId)
{
    Q_ASSERT(tagId.isValid());

    const auto it = m_tags.find(tagId);
    if (it == m_tags.end())
        return;

    // remove from parent
    DbTag *parent = tagForTagId(tagId.parent(m_db->library()));
    if (parent)
        parent->removeChildTag(it.value());

    // remove children
    for (const Moosick::TagId &childId : tagId.children(m_db->library()))
        removeTag(childId);

    it.value()->deleteLater();
    m_tags.erase(it);
}

void DatabaseInterface::search(const QString &searchString)
{
    if (m_searchString == searchString)
        return;

    repopulateSearchResults();

    m_searchString = searchString;
    m_searchKeywords = searchString.split(" ");
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

void DatabaseInterface::editItem(DbItem *item)
{
    Q_ASSERT(m_editItemType == EditNone);
    Q_ASSERT(m_editItemSource == SourceNone);

    if (DbArtist *artist = qobject_cast<DbArtist*>(item)) {
        m_editItemType = EditArtist;
        m_editStringList->popup(artist->name());
        m_tagsModel->setSelectedTagIds(artist->tagIds());
    }
    else if (DbAlbum *album = qobject_cast<DbAlbum*>(item)) {
        m_editItemType = EditAlbum;
        m_editStringList->popup(album->name());
        m_tagsModel->setSelectedTagIds(album->tagIds());
    }
    else if (DbSong *song = qobject_cast<DbSong*>(item)) {
        m_editItemType = EditSong;
        m_editStringList->popup(song->name());
        m_tagsModel->setSelectedTagIds(song->tagIds());
    } else {
        m_editItemType = EditNone;
    }

    m_editItemSource = (m_editItemType != EditNone) ? SourceLibrary : SourceNone;
    m_editedItemId = (item && (m_editItemType != EditNone)) ? item->id() : 0;

    emit stateChanged();
}

void DatabaseInterface::requestDownload(const NetCommon::DownloadRequest &request, Search::Result *result)
{
    if (!m_requestedDownload.isNull() || (m_editItemType != EditNone) || (m_editItemSource != SourceNone))
        return;

    m_requestedDownload.reset(new DownloadRequest { request, Moosick::TagIdList(), result });

    m_editItemType = EditArtist;
    m_editItemSource = (request.tp == NetCommon::DownloadRequest::BandcampAlbum) ? SourceBandcamp : SourceYoutube;
    m_editedItemId = 0;

    m_editStringList->popup(request.artistName);
    m_tagsModel->setSelectedTagIds({});

    emit stateChanged();
}

void DatabaseInterface::onStringSelected(int id)
{
    if (id < 0)
        return;

    switch (m_editItemType) {
    case EditArtist:
        m_tagsModel->setSelectedTagIds(Moosick::ArtistId(id).tags(m_db->library()));
        break;
    case EditNone:
    default:
        break;
    }
}

void DatabaseInterface::editOkClicked()
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
                m_requestedDownload->albumTags = selectedTags;
            }
        }

        m_editItemType = EditNone;
        m_editItemSource = SourceNone;

        QNetworkReply *reply = m_db->download(m_requestedDownload->request, m_requestedDownload->albumTags);
        m_downloads[reply] = m_requestedDownload->searchResult;
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
            m_editStringList->popup("");
        }
        else if (m_editItemType == EditAlbum) {
            m_editItemType = EditNone;
            m_editItemSource = SourceNone;

            m_requestedDownload->request.albumName = m_editStringList->enteredString();
            QNetworkReply *reply = m_db->download(m_requestedDownload->request, m_requestedDownload->albumTags);
            m_downloads[reply] = m_requestedDownload->searchResult;
            m_requestedDownload.reset();
        }
    }

    if (m_editItemSource == SourceLibrary) {
        Q_ASSERT(m_editedItemId > 0);

        QNetworkReply *reply =
                (m_editItemType == EditArtist) ? m_db->setArtistDetails(m_editedItemId, enteredName, selectedTags) :
                (m_editItemType == EditAlbum) ? m_db->setAlbumDetails(m_editedItemId, enteredName, selectedTags) :
                (m_editItemType == EditSong) ? m_db->setSongDetails(m_editedItemId, enteredName, selectedTags) : nullptr;

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

void DatabaseInterface::repopulateSearchResults()
{
    QVector<Moosick::ArtistId> newSearchResults;

    // get all results for new search keywords
    if (m_searchString.isEmpty()) {
        newSearchResults = m_db->library().artistsByName();
    } else {
        for (const Moosick::ArtistId &artistId : m_db->library().artistsByName()) {
            const QString lowerName = artistId.name(m_db->library()).toLower();
            const bool matches = std::all_of(m_searchKeywords.cbegin(), m_searchKeywords.cend(), [&](const QString &keyword) {
                return lowerName.contains(keyword);
            });
            if (matches)
                newSearchResults << artistId;
        }
    }

    // TODO: we could do smarter than this
    for (const SearchResultArtist &artist : m_searchResults.data()) {
        if (artist.artist)
            artist.artist->deleteLater();
    }
    m_searchResults.clear();

    for (const Moosick::ArtistId &artistId : newSearchResults) {
        SearchResultArtist artist;
        artist.artist = new DbArtist(this, artistId);
        for (const Moosick::AlbumId &albumId : artistId.albums(m_db->library()))
            artist.albums << SearchResultAlbum { albumId, albumId.songs(m_db->library()) };
        m_searchResults.add(artist);
    }
}

void DatabaseInterface::repopulateEditStringList()
{
    m_editStringList->clear();
    for (const Moosick::ArtistId &artistId : m_db->library().artistsByName()) {
        m_editStringList->add((int) artistId, artistId.name(m_db->library()));
    }
}

void DatabaseInterface::repopulateTagsModel()
{
    for (const Moosick::TagId &tagId : m_db->library().rootTags()) {
        DbTag *rootTag = getOrCreateDbTag(tagId);
        m_tagsModel->addTag(rootTag);
    }
}

bool DatabaseInterface::editItemVisible() const
{
    return (m_editItemType != EditNone) && (m_editItemSource != SourceNone);
}

bool DatabaseInterface::editItemStringsChoiceActive() const
{
    return (m_editItemType == EditArtist) && (m_editItemSource != SourceNone);
}

} // namespace Database
