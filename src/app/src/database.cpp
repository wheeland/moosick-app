#include "database.hpp"

Database::Database(HttpClient *httpClient, QObject *parent)
    : QObject(parent)
    , m_http(new HttpRequester(httpClient, this))
{
    connect(m_http, &HttpRequester::receivedReply, this, &Database::onNetworkReplyFinished);
}

Database::~Database()
{
}

void Database::sync()
{
    if (hasRunningRequestType(LibrarySync))
        return;

    QNetworkReply *reply = m_http->requestFromServer("/lib.do", "");
    m_requests[reply] = LibrarySync;
}

void Database::onNetworkReplyFinished(QNetworkReply *reply, QNetworkReply::NetworkError error)
{
    const RequestType requestType = m_requests.take(reply);
    const QByteArray data = reply->readAll();

    switch (requestType) {
    case LibrarySync:
        m_libray.deserialize(QByteArray::fromBase64(data));
        qWarning().noquote() << m_libray.dumpToStringList();
        break;
    default:
        break;
    }
}

bool Database::hasRunningRequestType(Database::RequestType requestType) const
{
    for (auto it = m_requests.begin(); it != m_requests.end(); ++it) {
        if (it.value() == requestType)
            return true;
    }
    return false;
}
