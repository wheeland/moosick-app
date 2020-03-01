#pragma once

#include <QPointer>

#include "library.hpp"
#include "database.hpp"
#include "database_items.hpp"
#include "../util/modeladapter.hpp"

class SelectTagsModel;
class StringModel;

namespace Search {
class Result;
class BandcampAlbumResult;
class YoutubeVideoResult;
}

namespace Database {

/*

  what are the tasks of the database?

  - sync with remote DB
  - request changes (downloads, renames, etc.)
  - expose to QML:
    - search for stuff (appears in searches!)
    - browse by tags, names
    - browse tags


*/

class DatabaseInterface : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool hasLibrary READ hasLibrary NOTIFY hasLibraryChanged)
    Q_PROPERTY(bool isSyncing READ isSyncing NOTIFY isSyncingChanged)
    Q_PROPERTY(bool downloadsPending READ downloadsPending NOTIFY downloadsPendingChanged)
    Q_PROPERTY(bool changesPending READ changesPending NOTIFY changesPendingChanged)

    /** list of all available tags in the library */
    Q_PROPERTY(SelectTagsModel *tagsModel READ tagsModel CONSTANT)
    Q_PROPERTY(SelectTagsModel *filterTagsModel READ filterTagsModel CONSTANT)

    /** list of artists that match the current search criteria */
    Q_PROPERTY(ModelAdapter::Model *searchResults READ searchResults CONSTANT)

    /** current search string */
    Q_PROPERTY(QString searchString READ searchString NOTIFY searchStringChanged)

    /** item currently being edited */
    Q_PROPERTY(EditItemType editItemType READ editItemType NOTIFY stateChanged)
    Q_PROPERTY(EditItemSource editItemSource READ editItemSource NOTIFY stateChanged)
    Q_PROPERTY(StringModel *editStringList READ editStringList CONSTANT)
    Q_PROPERTY(bool editItemVisible READ editItemVisible NOTIFY stateChanged)
    Q_PROPERTY(bool editItemStringsChoiceActive READ editItemStringsChoiceActive NOTIFY stateChanged)

    /** OK/cancel popup */
    Q_PROPERTY(bool confirmationVisible READ confirmationVisible NOTIFY confirmationChanged)
    Q_PROPERTY(QString confirmationText READ confirmationText NOTIFY confirmationChanged)

public:

    /** Is an edit dialog being displayed, and for what library item type? */
    enum EditItemType {
        EditNone,
        EditArtist,
        EditAlbum,
        EditSong,
        EditTag,
        ChangeAlbumArtist,
        AddNewTag,
    };
    Q_ENUM(EditItemType)

    /** Is a download dialog being displayed, and for what source? */
    enum EditItemSource {
        SourceNone,
        SourceLibrary,
        SourceBandcamp,
        SourceYoutube,
    };
    Q_ENUM(EditItemSource)

    DatabaseInterface(HttpClient *httpClient, QObject *parent = nullptr);
    DatabaseInterface(const Moosick::Library &library, HttpClient *httpClient, QObject *parent = nullptr);
    ~DatabaseInterface() override;

    const Moosick::Library &library() const;
    Q_INVOKABLE void sync();

    bool hasLibrary() const { return m_db->hasLibrary(); }
    bool isSyncing() const { return m_db->isSyncing(); }
    bool downloadsPending() const { return m_db->downloadsPending(); }
    bool changesPending() const { return m_db->changesPending(); }

    SelectTagsModel *tagsModel() const { return m_tagsModel; }
    SelectTagsModel *filterTagsModel() const { return m_filterTagsModel; }
    ModelAdapter::Model *searchResults() const { return m_searchResults.model(); }
    QString searchString() const { return m_searchString; }
    StringModel *editStringList() const { return m_editArtistStringList; }
    EditItemType editItemType() const { return m_editItemType; }
    EditItemSource editItemSource() const { return m_editItemSource; }
    bool editItemVisible() const;
    bool editItemStringsChoiceActive() const;

    Q_INVOKABLE void search(const QString &searchString);
    Q_INVOKABLE void fillArtistInfo(DbArtist *artist);

    Q_INVOKABLE void changeAlbumArtist(DbAlbum *album);

    Q_INVOKABLE void addNewTag();
    Q_INVOKABLE void editItem(DbItem *item);
    Q_INVOKABLE void editOkClicked();
    Q_INVOKABLE void editCancelClicked();
    Q_INVOKABLE void requestDownload(const NetCommon::DownloadRequest &request, Search::Result *result);

    Q_INVOKABLE void removeItem(DbItem *item);

    QString confirmationText() const;
    bool confirmationVisible() const;
    Q_INVOKABLE void confirm(bool ok);

private slots:
    void onArtistStringSelected(int id);
    void onLibraryChanged();

signals:
    void searchStringChanged(QString searchString);
    void stateChanged();
    void confirmationChanged();
    void hasLibraryChanged();
    void isSyncingChanged();
    void downloadsPendingChanged();
    void changesPendingChanged();

private:
    friend class DbTaggedItem;

    Database *m_db;

    enum RequestType {
        None,
        LibrarySync,
        BandcampDownload,
    };

    DbTag *tagForTagId(Moosick::TagId tagId) const;
    DbTag *getOrCreateDbTag(Moosick::TagId tagId);

    QHash<Moosick::TagId::IntType, DbTag*> m_tags;  // instantiations for all tag IDs
    SelectTagsModel *m_tagsModel;
    SelectTagsModel *m_filterTagsModel;

    /**
     * Data structures for all currently active search results:
     */
    struct SearchResultAlbum {
        Moosick::AlbumId albumId;
        Moosick::SongIdList songs;
    };
    struct SearchResultArtist {
        Moosick::ArtistId id;
        DbArtist *artist;
        QVector<SearchResultAlbum> albums;
    };
    QString m_searchString;
    QStringList m_searchKeywords;
    ModelAdapter::Adapter<SearchResultArtist> m_searchResults;

    QVector<SearchResultArtist> computeSearchResults() const;

    void updateSearchResults();
    void repopulateEditStringList();
    void repopulateTagsModel();

    /**
     * Currently edited thing
     */
    quint32 m_editedItemId;
    EditItemType m_editItemType = EditNone;
    EditItemSource m_editItemSource = SourceNone;
    StringModel *m_editArtistStringList;

    /**
     * Confirmation dialog
     */
    QPointer<DbItem> m_itemToBeRemoved;

    /**
     * Runnning Downloads
     */
    struct DownloadRequest {
        NetCommon::DownloadRequest request;
        Moosick::TagIdList albumTags;
        QPointer<Search::Result> searchResult;
    };
    QScopedPointer<DownloadRequest> m_requestedDownload;
};

} // namespace Database
