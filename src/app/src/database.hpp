#pragma once

#include <QNetworkAccessManager>

#include "httpclient.hpp"
#include "library.hpp"

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

public:
    Database(HttpClient *httpClient, QObject *parent = nullptr);
    ~Database() override;

public slots:
    void sync();

signals:

private slots:
    void onNetworkReplyFinished(QNetworkReply *reply, QNetworkReply::NetworkError error);

private:
    enum RequestType {
        None,
        LibrarySync,
    };

    bool hasRunningRequestType(RequestType requestType) const;

    Moosick::Library m_libray;

    HttpRequester *m_http;
    QHash<QNetworkReply*, RequestType> m_requests;
};
