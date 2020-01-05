#include "httpclient.hpp"

HttpRequester::HttpRequester(HttpClient *client, QObject *parent)
    : QObject(parent)
    , m_httpClient(client)
{
}

HttpRequester::~HttpRequester()
{
}

QNetworkReply *HttpRequester::request(const QNetworkRequest &request)
{
    m_runningQueries << m_httpClient->request(this, request);
    emit runningRequestsChanged(runningRequests());
    return m_runningQueries.last();
}

QNetworkReply *HttpRequester::requestFromServer(const QString &path, const QString &query)
{
    m_runningQueries << m_httpClient->requestFromServer(this, path, query);
    emit runningRequestsChanged(runningRequests());
    return m_runningQueries.last();
}

void HttpRequester::abortAll()
{
    for (QNetworkReply *reply : qAsConst(m_runningQueries))
        m_httpClient->abort(reply);
    m_runningQueries.clear();
    emit runningRequestsChanged(runningRequests());
}

void HttpRequester::requestDone(QNetworkReply *reply, QNetworkReply::NetworkError error)
{
    if (!m_runningQueries.contains(reply))
        return;

    m_runningQueries.removeAll(reply);
    receivedReply(reply, error);
    emit runningRequestsChanged(runningRequests());
}

HttpClient::HttpClient(QObject *parent)
    : QObject(parent)
    , m_manager(new QNetworkAccessManager(this))
{
    connect(m_manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply *reply) {
        onNetworkReplyFinished(reply, QNetworkReply::NoError);
    });
}

HttpClient::~HttpClient()
{
}

void HttpClient::setHost(const QString &name)
{
    m_host = name;
}

void HttpClient::setPort(quint16 port)
{
    m_port = port;
}

void HttpClient::abort(QNetworkReply *reply)
{
    const auto it = m_runningQueries.find(reply);
    if (it == m_runningQueries.end())
        return;

    reply->abort();
    reply->deleteLater();
    m_runningQueries.erase(it);
}

QNetworkReply *HttpClient::request(HttpRequester *requester, const QNetworkRequest &request)
{
    Q_ASSERT(requester);

    QNetworkReply *reply = m_manager->get(request);
    connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
            this, [=](QNetworkReply::NetworkError error) {
        onNetworkReplyFinished(reply, error);
    });

    m_runningQueries[reply] = requester;
    return reply;
}

QNetworkReply *HttpClient::requestFromServer(HttpRequester *requester, const QString &path, const QString &query)
{
    QUrl url;
    url.setScheme("http");
    url.setHost(m_host);
    url.setPort(m_port);
    url.setPath(path);
    url.setQuery(query, QUrl::StrictMode);
    return request(requester, QNetworkRequest(url));
}

void HttpClient::onNetworkReplyFinished(QNetworkReply *reply, QNetworkReply::NetworkError error)
{
    auto it = m_runningQueries.find(reply);

    if (it != m_runningQueries.end()) {
        if (it.value())
            it.value()->receivedReply(reply, error);
        m_runningQueries.erase(it);
    }

    reply->deleteLater();
}
