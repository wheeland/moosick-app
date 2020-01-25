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

    /** list of all available tags in the library */
    Q_PROPERTY(SelectTagsModel *tagsModel READ tagsModel CONSTANT)

    /** list of artists that match the current search criteria */
    Q_PROPERTY(ModelAdapter::Model *searchResults READ searchResults CONSTANT)

    /** current search string */
    Q_PROPERTY(QString searchString READ searchString NOTIFY searchStringChanged)

    /** item currently being edited */
    Q_PROPERTY(EditItemType editItemType READ editItemType NOTIFY stateChanged)
    Q_PROPERTY(EditItemSource editItemSource READ editItemSource NOTIFY stateChanged)
    Q_PROPERTY(StringModel *editStringList READ editStringList CONSTANT)
    Q_PROPERTY(bool editItemVisible READ editItemVisible NOTIFY stateChanged)

public:

    /** Is an edit dialog being displayed, and for what library item type? */
    enum EditItemType {
        EditNone,
        EditArtist,
        EditAlbum,
        EditSong,
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
    ~DatabaseInterface() override;

    const Moosick::Library &library();

    SelectTagsModel *tagsModel() const { return m_tagsModel; }
    ModelAdapter::Model *searchResults() const { return m_searchResults.model(); }
    QString searchString() const { return m_searchString; }
    StringModel *editStringList() const { return m_editStringList; }
    EditItemType editItemType() const { return m_editItemType; }
    EditItemSource editItemSource() const { return m_editItemSource; }
    bool editItemVisible() const;

    Q_INVOKABLE void search(const QString &searchString);
    Q_INVOKABLE void fillArtistInfo(DbArtist *artist);

    Q_INVOKABLE void editItem(DbItem *item);
    Q_INVOKABLE void editOkClicked();
    Q_INVOKABLE void editCancelClicked();
    Q_INVOKABLE void requestDownload(const NetCommon::DownloadRequest &request, Search::Result *result);

private slots:
    void onStringSelected(int id);
    void onLibraryChanged();

signals:
    void searchStringChanged(QString searchString);
    void stateChanged();

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
    void removeTag(Moosick::TagId tagId);

    void repopulateSearchResults();
    void repopulateEditStringList();
    void repopulateTagsModel();

    QHash<Moosick::TagId::IntType, DbTag*> m_tags;  // instantiations for all tag IDs
    SelectTagsModel *m_tagsModel;

    /**
     * Data structures for all currently active search results:
     */
    struct SearchResultAlbum {
        Moosick::AlbumId albumId;
        Moosick::SongIdList songs;
    };
    struct SearchResultArtist {
        DbArtist *artist;
        QVector<SearchResultAlbum> albums;
    };
    QString m_searchString;
    QStringList m_searchKeywords;
    ModelAdapter::Adapter<SearchResultArtist> m_searchResults;

    /**
     * Currently edited thing
     */
    quint32 m_editedItemId;
    EditItemType m_editItemType = EditNone;
    EditItemSource m_editItemSource = SourceNone;
    StringModel *m_editStringList;

    /**
     * Runnning Downloads
     */
    struct DownloadRequest {
        NetCommon::DownloadRequest request;
        Moosick::TagIdList albumTags;
        QPointer<Search::Result> searchResult;
    };
    QScopedPointer<DownloadRequest> m_requestedDownload;
    QHash<QNetworkReply*, QPointer<Search::Result>> m_downloads;
};

} // namespace Database
