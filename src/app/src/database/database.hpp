#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>

#include "library.hpp"
#include "../util/modeladapter.hpp"
#include "requests.hpp"

class HttpClient;
class HttpRequester;
class SelectTagsModel;
class StringModel;

namespace Search {
class Result;
class BandcampAlbumResult;
class YoutubeVideoResult;
}

namespace Database {

class DbItem;
class DbTag;
class DbArtist;
class DbAlbum;
class DbSong;

/*

  what are the tasks of the database?

  - sync with remote DB
  - request changes (downloads, renames, etc.)
  - expose to QML:
    - search for stuff (appears in searches!)
    - browse by tags, names
    - browse tags


*/

class Database : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hasLibrary READ hasLibrary NOTIFY hasLibraryChanged)

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

    Database(HttpClient *httpClient, QObject *parent = nullptr);
    ~Database() override;

    const Moosick::Library &library() const { return m_library; }
    SelectTagsModel *tagsModel() const { return m_tagsModel; }
    ModelAdapter::Model *searchResults() const { return m_searchResults.model(); }
    bool hasLibrary() const { return m_hasLibrary; }
    QString searchString() const { return m_searchString; }
    StringModel *editStringList() const { return m_editStringList; }
    EditItemType editItemType() const { return m_editItemType; }
    EditItemSource editItemSource() const { return m_editItemSource; }
    bool editItemVisible() const;

    Q_INVOKABLE void sync();
    Q_INVOKABLE void search(QString searchString);
    Q_INVOKABLE void fillArtistInfo(DbArtist *artist);

    Q_INVOKABLE void editItem(DbItem *item);
    Q_INVOKABLE void editOkClicked();
    Q_INVOKABLE void editCancelClicked();
    Q_INVOKABLE void requestDownload(const NetCommon::DownloadRequest &request, Search::Result *result);

private slots:
    void onNetworkReplyFinished(QNetworkReply *reply, QNetworkReply::NetworkError error);
    void onStringSelected(int id);

signals:
    void hasLibraryChanged();
    void searchStringChanged(QString searchString);
    void stateChanged();

private:
    friend class DbTaggedItem;

    enum RequestType {
        None,
        LibrarySync,
        BandcampDownload,
    };

    void onNewLibrary();
    bool applyLibraryChanges(const QByteArray &changesJsonData);

    DbTag *tagForTagId(Moosick::TagId tagId) const;
    DbTag *getOrCreateDbTag(Moosick::TagId tagId);
    void removeTag(Moosick::TagId tagId);

    void repopulateSearchResults();
    void repopulateEditStringList();
    void repopulateTagsModel();

    bool hasRunningRequestType(RequestType requestType) const;

    bool m_hasLibrary = false;
    Moosick::Library m_library;

    HttpRequester *m_http;
    QHash<QNetworkReply*, RequestType> m_requests;

    QHash<Moosick::TagId::IntType, DbTag*> m_tags;  // instantiations for all tag IDs
    SelectTagsModel *m_tagsModel;

    /**
     * If we are not ready to commit() some result transitions yet,
     * we will save them for later application
     */
    QVector<Moosick::CommittedLibraryChange> m_waitingChanges;
    /**
     * TODO:
     * changes that have been requested at the server, but where the response hasn't yet
     * been received, plus the waiting changes that we know have finished, but just didn't get
     * the prior CommitedLibraryChanges, we should all group in a second Library.
     * This m_overlayLibrary will be based on the synced m_library, but with all pending
     * patches applied to it.
     */

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
    struct Download {
        NetCommon::DownloadRequest request;
        Moosick::TagIdList artistTags;
        QPointer<Search::Result> searchResult;
        QNetworkReply *networkReply;
    };
    void startDownload();
    QScopedPointer<Download> m_requestedDownload;
    QVector<Download> m_runningDownloads;
};

} // namespace Database
