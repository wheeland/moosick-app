#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>

class HttpClient;

class HttpRequester : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int runningRequests READ runningRequests NOTIFY runningRequestsChanged)

public:
    HttpRequester(HttpClient *client, QObject *parent = nullptr);
    ~HttpRequester();

    QNetworkReply *request(const QNetworkRequest &request);
    QNetworkReply *requestFromServer(const QString &path, const QString &query);

    void abortAll();

    int runningRequests() const { return m_runningQueries.size(); }

signals:
    void receivedReply(QNetworkReply *receivedReply, QNetworkReply::NetworkError error);
    void runningRequestsChanged(int runningRequests);

private:
    void requestDone(QNetworkReply *receivedReply, QNetworkReply::NetworkError error);

    friend class HttpClient;

    HttpClient *m_httpClient;
    QVector<QNetworkReply*> m_runningQueries;
};

class HttpClient : public QObject
{
    Q_OBJECT

public:
    HttpClient(QObject *parent = nullptr);
    ~HttpClient() override;

    void setHost(const QString &name);
    void setPort(quint16 port);

private:
    QNetworkReply *request(HttpRequester *requester, const QNetworkRequest &request);
    QNetworkReply *requestFromServer(HttpRequester *requester, const QString &path, const QString &query);
    void onNetworkReplyFinished(QNetworkReply *reply, QNetworkReply::NetworkError error);

    void abort(QNetworkReply *reply);

    friend class HttpRequester;

    QString m_host;
    quint16 m_port;
    QNetworkAccessManager *m_manager = nullptr;

    QHash<QNetworkReply*, QPointer<HttpRequester>> m_runningQueries;
};