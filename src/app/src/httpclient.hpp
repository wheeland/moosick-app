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
    Q_PROPERTY(QString user READ user WRITE setUser NOTIFY userChanged)
    Q_PROPERTY(QString pass READ pass WRITE setPass NOTIFY passChanged)

public:
    HttpClient(QObject *parent = nullptr);
    ~HttpClient() override;

    void setHost(const QString &name);
    void setPort(quint16 port);
    void setPass(const QString &pass);
    void setUser(const QString &user);

    QString host() const { return m_host; }
    quint16 port() const { return m_port; }
    QString user() const { return m_user; }
    QString pass() const { return m_pass; }
    bool hostValid() const { return m_hostValid; }

signals:
    void hostValidChanged(bool hostValid);
    void hostChanged(QString host);
    void portChanged(quint16 port);
    void passChanged(QString pass);
    void userChanged(QString user);

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
    QString m_pass;
    QString m_user;
    QNetworkAccessManager *m_manager = nullptr;

    HttpRequestId m_nextRequestId = 1;
    QVector<RunningRequest> m_runningRequests;
    bool m_hostValid = false;
};
