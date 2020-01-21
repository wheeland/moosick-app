#pragma once

#include <QNetworkAccessManager>

#include "httpclient.hpp"
#include "library.hpp"
#include "util/modeladapter.hpp"
#include "stringmodel.hpp"
#include "selecttagsmodel.hpp"
#include "database_items.hpp"

namespace Search {
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

    Q_INVOKABLE void sync();
    Q_INVOKABLE void search(QString searchString);
    Q_INVOKABLE void fillArtistInfo(DbArtist *artist);

    Q_INVOKABLE void editOkClicked();
    Q_INVOKABLE void editCancelClicked();
    Q_INVOKABLE void startBandcampDownload(Search::BandcampAlbumResult *bc);
    Q_INVOKABLE void startYoutubeDownload(Search::YoutubeVideoResult *yt);

private slots:
    void onNetworkReplyFinished(QNetworkReply *reply, QNetworkReply::NetworkError error);

signals:
    void hasLibraryChanged();
    void searchStringChanged(QString searchString);
    void stateChanged();

private:
    friend class DbTaggedItem;

    enum RequestType {
        None,
        LibrarySync,
    };

    void onNewLibrary();

    DbTag *tagForTagId(Moosick::TagId tagId) const;
    DbTag *addTag(Moosick::TagId tagId);
    void removeTag(Moosick::TagId tagId);

    void clearSearchResults();
    void repopulateSearchResults();

    bool hasRunningRequestType(RequestType requestType) const;

    bool m_hasLibrary = false;
    Moosick::Library m_library;

    HttpRequester *m_http;
    QHash<QNetworkReply*, RequestType> m_requests;

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
    ModelAdapter::Adapter<SearchResultArtist> m_searchResults;

    EditItemType m_editItemType = EditNone;
    EditItemSource m_editItemSource = SourceNone;
    StringModel *m_editStringList;
};

} // namespace Database
