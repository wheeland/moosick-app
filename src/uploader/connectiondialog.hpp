#pragma once

#include <QWidget>
#include <QMessageBox>
#include <QLabel>
#include <QLineEdit>
#include <QSettings>

#include "library.hpp"
#include "connection.hpp"

class ConnectionDialog : public QWidget
{
    Q_OBJECT

public:
    explicit ConnectionDialog(Connection *connection, QWidget *parent = nullptr);
    ~ConnectionDialog();

    Moosick::Library *library() const;

signals:
    void connected();
    void cancelled();

private slots:
    void onConnectClicked();
    void onCancelClicked();
    void onConnectingChanged();

private:
    QSettings m_settings;

    Connection *m_connection;

    QLineEdit *m_urlEdit;
    QLineEdit *m_portEdit;
    QLineEdit *m_userEdit;
    QLineEdit *m_passEdit;

    QScopedPointer<QMessageBox> m_connectingDialog;

    QString m_url;
    quint16 m_port;
    QString m_user;
    QString m_pass;
};
