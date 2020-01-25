#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>

#include "library.hpp"
#include "requests.hpp"

class HttpClient;
class HttpRequester;

namespace Database {

class Database : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hasLibrary READ hasLibrary NOTIFY libraryChanged)

public:
    Database(HttpClient *httpClient, QObject *parent = nullptr);
    ~Database() override;

    bool hasLibrary() const { return m_hasLibrary; }
    const Moosick::Library &library() const { return m_library; }

    QNetworkReply *sync();
    QNetworkReply *download(NetCommon::DownloadRequest request, const Moosick::TagIdList &albumTags);

private slots:
    void onNetworkReplyFinished(QNetworkReply *reply, QNetworkReply::NetworkError error);

signals:
    void libraryChanged();

private:
    void onNewLibrary(const QJsonObject &json);
    bool applyLibraryChanges(const QByteArray &changesJsonData);

    enum RequestType {
        None,
        LibrarySync,
        BandcampDownload,
    };

    bool m_hasLibrary = false;
    Moosick::Library m_library;

    HttpRequester *m_http;
    QHash<QNetworkReply*, RequestType> m_requests;

    bool hasRunningRequestType(RequestType requestType) const;

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

    struct Download {
        NetCommon::DownloadRequest request;
        Moosick::TagIdList albumTags;
        QNetworkReply *networkReply;
    };
    QVector<Download> m_runningDownloads;
};

} // namespace Database
