#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "option.hpp"
#include "library.hpp"
#include "library_messages.hpp"

class Connection : public QObject
{
    Q_OBJECT

public:
    Connection(QObject *parent = nullptr);
    ~Connection();

    void setUrl(const QString &url);
    void setPort(quint16 port);
    void setUser(const QString &user);
    void setPass(const QString &pass);

    bool isConnected() const { return m_connected; }
    bool isConnecting() const { return m_currentRequest; }

    QNetworkReply::NetworkError error() const { return m_error; }
    QString errorString() const { return m_errorString; }

    QNetworkReply *post(const QByteArray &postData, const QByteArray &query = QByteArray());
    Result<MoosickMessage::Message, QString> tryParseReply(QNetworkReply *reply);

    const Moosick::Library &library() const { return m_library; }

signals:
    void connectedChanged();
    void connectingChanged();
    void networkReplyFinished(QNetworkReply *reply);

private slots:
    void onNetworkReplyFinished(QNetworkReply *reply);

private:
    Moosick::Library m_library;

    QNetworkAccessManager *m_network;

    QNetworkReply *m_currentRequest = nullptr;
    bool m_retry = false;

    /** Tries to read the database from a LibraryResponse, otherwise returns an error string */
    Option<QString> tryParseConnectionReply(QNetworkReply *reply);

    void tryConnect();
    void doConnect();

    bool m_connected = false;

    QNetworkReply::NetworkError m_error = QNetworkReply::NoError;
    QString m_errorString;

    QString m_url;
    quint16 m_port = 0;
    QString m_user;
    QString m_pass;
};
