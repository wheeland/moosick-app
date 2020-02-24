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
    Q_PROPERTY(bool hostValid READ hostValid NOTIFY hostValidChanged)
    Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(quint16 port READ port WRITE setPort NOTIFY portChanged)

public:
    HttpClient(QObject *parent = nullptr);
    ~HttpClient() override;

    void setHost(const QString &name);
    void setPort(quint16 port);

    QString host() const { return m_host; }
    quint16 port() const { return m_port; }
    bool hostValid() const { return m_hostValid; }

signals:
    void hostValidChanged(bool hostValid);
    void portChanged(quint16 port);
    void hostChanged(QString host);

private slots:
    void onNetworkReplyFinished(QNetworkReply *reply);
    void maybeRelaunchRequests();

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

    void launchRequest(RunningRequest &request);

    void abortAll(HttpRequester *requester);

    friend class HttpRequester;

    QString m_host;
    quint16 m_port;
    QNetworkAccessManager *m_manager = nullptr;

    HttpRequestId m_nextRequestId = 1;
    QVector<RunningRequest> m_runningRequests;
    bool m_hostValid = false;
};
