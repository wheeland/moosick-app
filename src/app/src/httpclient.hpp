#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>

class HttpClient;

using HttpRequestId = quint64;

class HttpRequester : public QObject
{
    Q_OBJECT

public:
    HttpRequester(HttpClient *client, QObject *parent = nullptr);
    ~HttpRequester();

    QString host() const;
    quint16 port() const;

    HttpRequestId request(const QNetworkRequest &request);
    HttpRequestId requestFromServer(const QString &path, const QString &query);

    void abortAll();

signals:
    void receivedReply(HttpRequestId requestId, const QByteArray &data);
    void runningRequestsChanged(int runningRequests);

private:
    friend class HttpClient;
    HttpClient *m_httpClient;
};

class HttpClient : public QObject
{
    Q_OBJECT

public:
    HttpClient(const QString &host, quint16 port, QObject *parent = nullptr);
    ~HttpClient() override;

    void setHost(const QString &name);
    void setPort(quint16 port);

    QString host() const { return m_host; }
    quint16 port() const { return m_port; }

private slots:
    void onNetworkConnectivityChanged(QNetworkAccessManager::NetworkAccessibility accessible);

private:
    struct RunningRequest
    {
        HttpRequestId id;
        QPointer<QNetworkReply> currentReply;
        bool isGlobalRequest;
        QNetworkRequest globalRequest;
        QString serverPath;
        QString serverQuery;
        QPointer<HttpRequester> requester;
    };

    HttpRequestId request(HttpRequester *requester, const QNetworkRequest &request);
    HttpRequestId requestFromServer(HttpRequester *requester, const QString &path, const QString &query);
    void onNetworkReplyFinished(QNetworkReply *reply);

    void launchRequest(RunningRequest &request);

    void abortAll(HttpRequester *requester);

    friend class HttpRequester;

    QString m_host;
    quint16 m_port;
    QNetworkAccessManager *m_manager = nullptr;

    HttpRequestId m_nextRequestId = 1;
    QVector<RunningRequest> m_runningRequests;
};
