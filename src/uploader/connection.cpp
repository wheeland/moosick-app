#include "connection.hpp"
#include "library_messages.hpp"

#include <QNetworkRequest>
#include <QTimer>

using namespace MoosickMessage;

Connection::Connection(QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
{
    connect(m_network, &QNetworkAccessManager::finished, this, &Connection::onNetworkReplyFinished);
}

Connection::~Connection()
{
}

void Connection::setUrl(const QString &url)
{
    m_url = url;
    QTimer::singleShot(0, this, &Connection::tryConnect);
}

void Connection::setPort(quint16 port)
{
    m_port = port;
    QTimer::singleShot(0, this, &Connection::tryConnect);
}

void Connection::setUser(const QString &user)
{
    m_user = user;
    QTimer::singleShot(0, this, &Connection::tryConnect);
}

void Connection::setPass(const QString &pass)
{
    m_pass = pass;
    QTimer::singleShot(0, this, &Connection::tryConnect);
}

QNetworkReply *Connection::post(const QByteArray &postData, const QByteArray &query)
{
    QUrl url(m_url);
    url.setPort(m_port);
    url.setUserName(m_user);
    url.setPassword(m_pass);
    if (!query.isEmpty())
        url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");

    return m_network->post(request, postData);
}

Result<Message, QString> Connection::tryParseReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError)
        return reply->errorString();

    auto response = MoosickMessage::Message::fromJson(reply->readAll());
    if (response.hasError())
        return response.getError().toString();

    return response.takeValue();
}

void Connection::tryConnect()
{
    if (m_currentRequest) {
        m_retry = true;
        return;
    }

    doConnect();
    emit connectingChanged();
}

void Connection::doConnect()
{
    const Message message(new LibraryRequest());
    const QByteArray postData = message.toJson();
    m_currentRequest = post(postData);
    m_retry = false;
}

Option<QString> Connection::tryParseConnectionReply(QNetworkReply *reply)
{
    Result<Message, QString> response = tryParseReply(reply);
    if (response.hasError() || !response.getValue().as<LibraryResponse>())
        return response.getError();


    const LibraryResponse lib = *response.getValue().as<LibraryResponse>();

    Moosick::SerializedLibrary serialized;
    serialized.libraryJson = lib.libraryJson;
    serialized.version = lib.version;
    EnjsonError error = m_library.deserializeFromJson(serialized);
    if (error.isError())
        return "Failed to parse LibraryResponse: " + error.toString();

    return {};
}

void Connection::onNetworkReplyFinished(QNetworkReply *reply)
{
    // if this doesn't belong to our initial LibraryRequest, forward to external users
    if (m_currentRequest != reply) {
        emit networkReplyFinished(reply);
        return;
    }

    m_error = reply->error();
    m_currentRequest = nullptr;
    reply->deleteLater();

    // try to parse LibraryRequest
    const Option<QString> result = tryParseConnectionReply(reply);

    if (result.hasValue()) {
        m_errorString = result.getValue();
        qWarning() << "Failed to connect:" << m_errorString;

        // lost connection? kinda weird but let's check against that
        if (m_connected) {
            m_connected = false;
            emit connectingChanged();
            emit connectedChanged();
            return;
        }

        if (m_retry) {
            doConnect();
            return;
        }

        emit connectingChanged();
    }
    else {
        m_errorString.clear();
        qWarning() << "Connected successfully";

        if (!m_connected) {
            m_connected = true;
            emit connectedChanged();
        }

        emit connectingChanged();
    }
}
