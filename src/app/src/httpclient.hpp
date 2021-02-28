#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>

class HttpClient;

using HttpRequestId = quint64;
constexpr static HttpRequestId HTTP_NULL_REQUEST = 0;

// todo: subclass HttpRequester to handle specifically musikator-related stuff
// (library shizzles, download, songs, etc.)

class HttpRequester : public QObject
{
    Q_OBJECT

public:
    HttpRequester(HttpClient *client, QObject *parent = nullptr);
    ~HttpRequester();

    QString host() const;
    quint16 port() const;
    QString user() const;
    QString pass() const;

    HttpRequestId request(const QNetworkRequest &request);
    HttpRequestId requestFromServer(const QByteArray &postData);

    void abortAll();

signals:
    void receivedReply(HttpRequestId requestId, const QByteArray &data);
    void networkError(HttpRequestId requestId, QNetworkReply::NetworkError error);
    void runningRequestsChanged(int runningRequests);

private:
    friend class HttpClient;
    HttpClient *m_httpClient;
};

class HttpClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hostValid READ hostValid NOTIFY hostValidChanged)
    Q_PROPERTY(QString apiUrl READ apiUrl WRITE setApiUrl NOTIFY apiUrlChanged)
    Q_PROPERTY(quint16 port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(QString user READ user WRITE setUser NOTIFY userChanged)
    Q_PROPERTY(QString pass READ pass WRITE setPass NOTIFY passChanged)

    Q_PROPERTY(bool hasPendingSslError READ hasPendingSslError NOTIFY pendingSslErrorChanged)
    Q_PROPERTY(QString pendingSslError READ pendingSslError NOTIFY pendingSslErrorChanged)

public:
    HttpClient(QObject *parent = nullptr);
    ~HttpClient() override;

    void setApiUrl(const QString &apiUrl);
    void setPort(quint16 port);
    void setPass(const QString &pass);
    void setUser(const QString &user);

    QString apiUrl() const { return m_apiUrl; }
    quint16 port() const { return m_port; }
    QString user() const { return m_user; }
    QString pass() const { return m_pass; }
    bool hostValid() const { return m_hostValid; }

    QByteArray ignoredSslErrorData() const;
    void setIgnoredSslErrorData(const QByteArray &data);

    bool hasPendingSslError() const;
    QString pendingSslError() const;
    Q_INVOKABLE void ignorePendingSslError(bool ignore);

signals:
    void hostValidChanged(bool hostValid);
    void portChanged(quint16 port);
    void passChanged(QString pass);
    void userChanged(QString user);
    void apiUrlChanged(QString apiUrl);
    void pendingSslErrorChanged();

private slots:
    void onNetworkReplyFinished(QNetworkReply *reply);
    void maybeRelaunchRequests();
    void onSslErrors(const QList<QSslError> &errors);

private:
    struct RunningRequest
    {
        HttpRequestId id;
        QPointer<QNetworkReply> currentReply;
        bool isGlobalRequest;
        bool hasPendingSslError = false;
        QNetworkRequest globalRequest;
        QByteArray postData;
        QPointer<HttpRequester> requester;
    };

    HttpRequestId request(HttpRequester *requester, const QNetworkRequest &request);
    HttpRequestId requestFromServer(HttpRequester *requester, const QByteArray &postData);

    QList<QSslError> ignoredSslErrors() const;

    void launchRequest(RunningRequest &request);

    void abortAll(HttpRequester *requester);

    friend class HttpRequester;

    QString m_apiUrl;
    QString m_host;
    QString m_hostPath;
    quint16 m_port;
    QString m_pass;
    QString m_user;
    QNetworkAccessManager *m_manager = nullptr;

    // all SSL errors that we encounter will be presented to the user
    // so that he can choose to ignore them in the future
    enum SslErrorReaction {
        SslErrorPending,
        SslErrorFail,
        SslErrorIgnore
    };
    QHash<QSslError, SslErrorReaction> m_sslErrors;
    QScopedPointer<QSslError> m_pendingSslError;

    HttpRequestId m_nextRequestId = 1;
    QVector<RunningRequest> m_runningRequests;
    bool m_hostValid = false;
};
