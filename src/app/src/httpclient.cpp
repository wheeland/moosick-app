#include "httpclient.hpp"

#include <QTimer>

HttpRequester::HttpRequester(HttpClient *client, QObject *parent)
    : QObject(parent)
    , m_httpClient(client)
{
}

HttpRequester::~HttpRequester()
{
}

QString HttpRequester::host() const
{
    return m_httpClient->m_host;
}

quint16 HttpRequester::port() const
{
    return m_httpClient->m_port;
}

HttpRequestId HttpRequester::request(const QNetworkRequest &request)
{
    return m_httpClient->request(this, request);
}

HttpRequestId HttpRequester::requestFromServer(const QString &path, const QString &query)
{
    const QString p = path.startsWith("/") ? path : QString("/") + path;
    return m_httpClient->requestFromServer(this, p, query);
}

void HttpRequester::abortAll()
{
    m_httpClient->abortAll(this);
}

HttpClient::HttpClient(QObject *parent)
    : QObject(parent)
    , m_manager(new QNetworkAccessManager(this))
{
    connect(m_manager, &QNetworkAccessManager::finished, this, &HttpClient::onNetworkReplyFinished);
    connect(m_manager, &QNetworkAccessManager::networkAccessibleChanged, this, &HttpClient::maybeRelaunchRequests);
}

HttpClient::~HttpClient()
{
}

void HttpClient::setHost(const QString &name)
{
    m_host = name;
    m_hostValid = true;
    QTimer::singleShot(0, this, &HttpClient::maybeRelaunchRequests);
    emit hostValidChanged(true);
    emit hostChanged(name);
}

void HttpClient::setPort(quint16 port)
{
    m_port = port;
    m_hostValid = true;
    QTimer::singleShot(0, this, &HttpClient::maybeRelaunchRequests);
    emit hostValidChanged(true);
    emit portChanged(port);
}

void HttpClient::abortAll(HttpRequester *requester)
{
    Q_UNUSED(requester)
    Q_ASSERT(false); // TODO
}

HttpRequestId HttpClient::request(HttpRequester *requester, const QNetworkRequest &request)
{
    Q_ASSERT(requester);

    RunningRequest newRequest;
    newRequest.id = m_nextRequestId++;
    newRequest.requester = requester;
    newRequest.globalRequest = request;
    newRequest.isGlobalRequest = true;

    launchRequest(newRequest);
    m_runningRequests << newRequest;

    return newRequest.id;
}

HttpRequestId HttpClient::requestFromServer(HttpRequester *requester, const QString &path, const QString &query)
{
    Q_ASSERT(requester);

    RunningRequest newRequest;
    newRequest.id = m_nextRequestId++;
    newRequest.requester = requester;
    newRequest.serverPath = path;
    newRequest.serverQuery = query;
    newRequest.isGlobalRequest = false;

    launchRequest(newRequest);
    m_runningRequests << newRequest;

    return newRequest.id;
}

void HttpClient::launchRequest(HttpClient::RunningRequest &request)
{
    Q_ASSERT(request.currentReply.isNull());

    if (!m_hostValid || (m_manager->networkAccessible() != QNetworkAccessManager::Accessible)) {
        qWarning() << "POSTPONE" << request.globalRequest.url() << request.serverPath << request.serverQuery;
        return;
    }

    if (request.isGlobalRequest) {
        request.currentReply = m_manager->get(request.globalRequest);
    }
    else {
        QUrl url;
        url.setScheme("http");
        url.setHost(m_host);
        url.setPort(m_port);
        url.setPath(request.serverPath);
        url.setQuery(request.serverQuery, QUrl::StrictMode);
        request.currentReply = m_manager->get(QNetworkRequest(url));
    }

    qWarning() << "LAUNCH" << request.currentReply->url();
}

static bool isHostOffline(QNetworkReply::NetworkError error)
{
    switch (error) {
    // network layer errors [relating to the destination server] (1-99):
    case QNetworkReply::ConnectionRefusedError:
    case QNetworkReply::RemoteHostClosedError:
    case QNetworkReply::HostNotFoundError:
    case QNetworkReply::TooManyRedirectsError:
    case QNetworkReply::ContentAccessDenied:
    case QNetworkReply::ContentOperationNotPermittedError:
    case QNetworkReply::ContentNotFoundError:
    case QNetworkReply::AuthenticationRequiredError:
    case QNetworkReply::ContentReSendError:
    case QNetworkReply::ContentConflictError:
    case QNetworkReply::ContentGoneError:
    case QNetworkReply::OperationNotImplementedError:
        return true;
    default:
        return false;
    }
}

void HttpClient::onNetworkReplyFinished(QNetworkReply *reply)
{
    const QNetworkReply::NetworkError error = reply->error();
    reply->deleteLater();

    // find RunningRequest
    int requestIdx = -1;
    for (int i = 0; i < m_runningRequests.size(); ++i) {
        if (m_runningRequests[i].currentReply == reply) {
            requestIdx = i;
            break;
        }
    }

    if (requestIdx < 0) {
        qWarning() << "HttpClient: no RunningRequest found for QNetworkReply" << reply;
        return;
    }

    RunningRequest &runningRequest = m_runningRequests[requestIdx];
    runningRequest.currentReply.clear();

    if (runningRequest.requester.isNull()) {
        qWarning() << "HttpClient: HttpRequester no longer exists for QNetworkReply";
        m_runningRequests.remove(requestIdx);
        return;
    }

    if (error == QNetworkReply::NoError) {
        const QByteArray data = reply->readAll();
        runningRequest.requester->receivedReply(runningRequest.id, data);
        m_runningRequests.remove(requestIdx);
        return;
    }

    // if the host can't be reached, prompt the user again for the host name
    if (isHostOffline(error)) {
        m_hostValid = false;
        emit hostValidChanged(false);
        return;
    }

    // if there was an error, we want to re-schedule the request, either now, or when
    // network connectivity is restored
    if (m_manager->networkAccessible() == QNetworkAccessManager::Accessible) {
        launchRequest(runningRequest);
    }
}

void HttpClient::maybeRelaunchRequests()
{
    if (m_hostValid && (m_manager->networkAccessible() == QNetworkAccessManager::Accessible)) {
        // re-start pending requests
        for (RunningRequest &runningRequest : m_runningRequests) {
            if (runningRequest.currentReply.isNull())
                launchRequest(runningRequest);
        }
    }
}
