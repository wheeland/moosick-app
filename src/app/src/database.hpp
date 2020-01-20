#pragma once

#include <QNetworkAccessManager>

#include "httpclient.hpp"
#include "library.hpp"
#include "util/modeladapter.hpp"
#include "stringmodel.hpp"
#include "database_items.hpp"

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
    Q_PROPERTY(ModelAdapter::Model *rootTags READ rootTagsModel CONSTANT)
    Q_PROPERTY(ModelAdapter::Model *searchResults READ searchResults CONSTANT)
    Q_PROPERTY(QString searchString READ searchString NOTIFY searchStringChanged)
    Q_PROPERTY(StringModel *artistNames READ artistNames CONSTANT)

public:
    Database(HttpClient *httpClient, QObject *parent = nullptr);
    ~Database() override;

    const Moosick::Library &library() const { return m_library; }
    ModelAdapter::Model *rootTagsModel() const { return m_rootTags.model(); }
    ModelAdapter::Model *searchResults() const { return m_searchResults.model(); }

    bool hasLibrary() const { return m_hasLibrary; }
    QString searchString() const { return m_searchString; }
    StringModel *artistNames() const { return m_artistNames; }

    Q_INVOKABLE void sync();
    Q_INVOKABLE void search(QString searchString);
    Q_INVOKABLE void fillArtistInfo(DbArtist *artist);

private slots:
    void onNetworkReplyFinished(QNetworkReply *reply, QNetworkReply::NetworkError error);

signals:
    void hasLibraryChanged();
    void searchStringChanged(QString searchString);

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
    ModelAdapter::Adapter<DbTag*> m_rootTags;       // view on root tags only

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
    StringModel *m_artistNames;
};

} // namespace Database
