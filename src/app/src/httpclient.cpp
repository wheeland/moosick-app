#include "httpclient.hpp"

#include <QTimer>
#include <QDataStream>

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

QString HttpRequester::user() const
{
    return m_httpClient->m_user;
}

QString HttpRequester::pass() const
{
    return m_httpClient->m_pass;
}

HttpRequestId HttpRequester::request(const QNetworkRequest &request)
{
    return m_httpClient->request(this, request);
}

HttpRequestId HttpRequester::requestFromServer(const QByteArray &postData)
{
    return m_httpClient->requestFromServer(this, postData);
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

void HttpClient::setApiUrl(const QString &apiUrl)
{
    QUrl url("http://" + apiUrl);
    m_apiUrl = apiUrl;
    m_host = url.host();
    m_hostPath = url.path();
    m_hostValid = true;
    QTimer::singleShot(0, this, &HttpClient::maybeRelaunchRequests);
    emit hostValidChanged(true);
    emit apiUrlChanged(apiUrl);
}

void HttpClient::setPort(quint16 port)
{
    m_port = port;
    m_hostValid = true;
    QTimer::singleShot(0, this, &HttpClient::maybeRelaunchRequests);
    emit hostValidChanged(true);
    emit portChanged(port);
}

void HttpClient::setUser(const QString &user)
{
    m_user = user;
    m_hostValid = true;
    QTimer::singleShot(0, this, &HttpClient::maybeRelaunchRequests);
    emit hostValidChanged(true);
    emit userChanged(user);
}

void HttpClient::setPass(const QString &pass)
{
    m_pass = pass;
    m_hostValid = true;
    QTimer::singleShot(0, this, &HttpClient::maybeRelaunchRequests);
    emit hostValidChanged(true);
    emit passChanged(pass);
}

void HttpClient::abortAll(HttpRequester *requester)
{
    for (auto it = m_runningRequests.begin(); it != m_runningRequests.end(); /*empty*/) {
        if (it->requester == requester) {
            QNetworkReply *reply = it->currentReply;
            it = m_runningRequests.erase(it);
            if (reply) {
                reply->abort();
                reply->deleteLater();
            }
        } else {
            ++it;
        }
    }
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

HttpRequestId HttpClient::requestFromServer(HttpRequester *requester, const QByteArray &postData)
{
    Q_ASSERT(requester);

    RunningRequest newRequest;
    newRequest.id = m_nextRequestId++;
    newRequest.requester = requester;
    newRequest.postData = postData;
    newRequest.isGlobalRequest = false;

    launchRequest(newRequest);
    m_runningRequests << newRequest;

    return newRequest.id;
}

void HttpClient::onSslErrors(const QList<QSslError> &errors)
{
    // add to encountered SSL errors, if it's a new one
    for (const QSslError &error : errors) {
        auto it = m_sslErrors.find(error);

        if (it == m_sslErrors.end()) {
            it = m_sslErrors.insert(error, SslErrorPending);

            if (m_pendingSslError.isNull()) {
                m_pendingSslError.reset(new QSslError(error));
                emit pendingSslErrorChanged();
            }
        }
    }
}

bool HttpClient::hasPendingSslError() const
{
    return !m_pendingSslError.isNull();
}

QString HttpClient::pendingSslError() const
{
    if (m_pendingSslError.isNull())
        return QString();

    QString ret = m_pendingSslError->errorString();
    ret += ":\n\n";

    const QSslCertificate cert = m_pendingSslError->certificate();
    ret += cert.toText();

    return ret;
}

void HttpClient::ignorePendingSslError(bool ignore)
{
    Q_ASSERT(!m_pendingSslError.isNull());
    Q_ASSERT(m_sslErrors.contains(*m_pendingSslError));

    m_sslErrors[*m_pendingSslError] = ignore ? SslErrorIgnore : SslErrorFail;

    for (RunningRequest &runningRequest : m_runningRequests)
        runningRequest.hasPendingSslError = false;
    maybeRelaunchRequests();

    // ask user to confirm next pending SSL error, if there is another one
    m_pendingSslError.reset();
    for (auto it = m_sslErrors.begin(); it != m_sslErrors.end(); ++it) {
        if (it.value() == SslErrorPending)
            m_pendingSslError.reset(new QSslError(it.key()));
    }
    pendingSslErrorChanged();
}

QList<QSslError> HttpClient::ignoredSslErrors() const
{
    QList<QSslError> ret;
    for (auto it = m_sslErrors.begin(); it != m_sslErrors.end(); ++it) {
        if (it.value() == SslErrorIgnore)
            ret << it.key();
    }
    return ret;
}

QByteArray HttpClient::ignoredSslErrorData() const
{
    QByteArray ret;
    QDataStream out(&ret, QIODevice::WriteOnly);

    const QList<QSslError> ignoredErrors = ignoredSslErrors();
    out << (qint32) ignoredErrors.size();

    for (const QSslError &error : ignoredErrors) {
        out << (qint32) error.error();
        out << error.certificate().toPem();
    }

    return ret;
}

void HttpClient::setIgnoredSslErrorData(const QByteArray &data)
{
    QDataStream in(data);

    qint32 count, error;
    QByteArray cert;
    QVector<QSslError> errors;

    in >> count;
    for (int i = 0; i < count; ++i) {
        in >> error;
        in >> cert;
        errors << QSslError((QSslError::SslError) error, QSslCertificate(cert, QSsl::Pem));
    }

    if (in.status() == QDataStream::Ok) {
        for (const QSslError &newError : errors)
            m_sslErrors[newError] = SslErrorIgnore;
        qWarning() << "Imported" << count << "ignored SSL errors";
    }
}

void HttpClient::launchRequest(HttpClient::RunningRequest &request)
{
    Q_ASSERT(request.currentReply.isNull());

    if (!m_hostValid || (m_manager->networkAccessible() != QNetworkAccessManager::Accessible)) {
        return;
    }

    if (request.isGlobalRequest) {
        request.currentReply = m_manager->get(request.globalRequest);
    }
    else {
        QUrl url;
        url.setScheme("https");
        url.setHost(m_host);
        url.setPort(m_port);
        url.setUserName(m_user);
        url.setPassword(m_pass);
        url.setPath(m_hostPath);
        if (request.postData.isEmpty()) {
            request.currentReply = m_manager->get(QNetworkRequest(url));
        } else {
            QNetworkRequest req(url);
            req.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
            request.currentReply = m_manager->post(req, request.postData);
        }

        request.currentReply->ignoreSslErrors(ignoredSslErrors());
        connect(request.currentReply, &QNetworkReply::sslErrors, this, &HttpClient::onSslErrors);
    }
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

static bool isTransientError(QNetworkReply::NetworkError error)
{
    switch (error) {
    case QNetworkReply::ServiceUnavailableError:
    case QNetworkReply::TimeoutError:
    case QNetworkReply::TemporaryNetworkFailureError:
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
        emit runningRequest.requester->receivedReply(runningRequest.id, data);
        m_runningRequests.remove(requestIdx);
        return;
    }

    if (!runningRequest.isGlobalRequest) {
        // if there were SSL errors, we will try again once one of the pending errors
        // got dealt with by the user
        if (error == QNetworkReply::SslHandshakeFailedError) {
            runningRequest.hasPendingSslError = true;
            return;
        }

        // if this is a server request and the host can't be reached,
        // prompt the user again for the host name
        if (isHostOffline(error)) {
            qWarning() << "Failed to get" << m_hostPath
                       << "with" << runningRequest.postData.size() << "bytes of POST data:" << error;
            m_hostValid = false;
            emit hostValidChanged(false);
            return;
        }
    }

    if (runningRequest.isGlobalRequest && !isTransientError(error)) {
        emit runningRequest.requester->networkError(runningRequest.id, error);
        m_runningRequests.remove(requestIdx);
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
            if (runningRequest.currentReply.isNull() && !runningRequest.hasPendingSslError)
                launchRequest(runningRequest);
        }
    }
}
