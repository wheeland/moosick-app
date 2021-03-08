#include "connectiondialog.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QIntValidator>
#include <QMetaEnum>

static const char* SETTINGS_URL = "URL";
static const char* SETTINGS_PORT = "PORT";
static const char* SETTINGS_USER = "USER";
static const char* SETTINGS_PASS = "PASS";

template <class T = QString>
static QLineEdit *setupLabel(QVBoxLayout *vbox, const QString &labelString, T *value) {
    QWidget *container = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout();
    container->setLayout(layout);
    vbox->addWidget(container);

    QLabel *label = new QLabel(labelString);
    layout->addWidget(label);

    QLineEdit *edit = new QLineEdit(QVariant(*value).toString());
    layout->addWidget(edit);

    QObject::connect(edit, &QLineEdit::textEdited, [=]() {
        *value = QVariant(edit->text()).value<T>();
    });

    return edit;
};

ConnectionDialog::ConnectionDialog(Connection *connection, QWidget *parent)
    : QWidget(parent)
    , m_connection(connection)
{
    connect(m_connection, &Connection::connectingChanged, this, &ConnectionDialog::onConnectingChanged);

    if (!m_settings.contains(SETTINGS_URL))
        m_settings.setValue(SETTINGS_URL, "musikator.de/moosick.do");
    if (!m_settings.contains(SETTINGS_PORT))
        m_settings.setValue(SETTINGS_PORT, 443);
    if (!m_settings.contains(SETTINGS_USER))
        m_settings.setValue(SETTINGS_USER, "musikator");
    if (!m_settings.contains(SETTINGS_PASS))
        m_settings.setValue(SETTINGS_PASS, "musikator");

    m_url = m_settings.value(SETTINGS_URL).toString();
    m_port = m_settings.value(SETTINGS_PORT).toInt();
    m_user = m_settings.value(SETTINGS_USER).toString();
    m_pass = m_settings.value(SETTINGS_PASS).toString();

    resize(400, 10);
    QVBoxLayout *vbox = new QVBoxLayout(this);

    m_urlEdit = setupLabel(vbox, "URL", &m_url);
    m_portEdit = setupLabel(vbox, "Port", &m_port);
    m_portEdit->setValidator(new QIntValidator(0, 65535, this));
    m_userEdit = setupLabel(vbox, "User", &m_user);
    m_passEdit = setupLabel(vbox, "Pass", &m_pass);
    m_passEdit->setEchoMode(QLineEdit::PasswordEchoOnEdit);

    QWidget *buttons = new QWidget();
    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    buttons->setLayout(buttonsLayout);
    vbox->addWidget(buttons);

    buttonsLayout->addStretch();

    QPushButton *connectButton = new QPushButton("Connect");
    connect(connectButton, &QPushButton::clicked, this, &ConnectionDialog::onConnectClicked);
    buttonsLayout->addWidget(connectButton);
    buttonsLayout->addStretch();

    QPushButton *cancelButton = new QPushButton("Cancel");
    connect(cancelButton, &QPushButton::clicked, this, &ConnectionDialog::onCancelClicked);
    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addStretch();

}

ConnectionDialog::~ConnectionDialog()
{
}

void ConnectionDialog::onConnectClicked()
{
    m_settings.setValue(SETTINGS_URL, m_url);
    m_settings.setValue(SETTINGS_PORT, m_port);
    m_settings.setValue(SETTINGS_USER, m_user);
    m_settings.setValue(SETTINGS_PASS, m_pass);

    m_connection->setUrl(m_url);
    m_connection->setPort(m_port);
    m_connection->setUser(m_user);
    m_connection->setPass(m_pass);
}

void ConnectionDialog::onCancelClicked()
{
    emit cancelled();
}

void ConnectionDialog::onConnectingChanged()
{
    if (m_connection->isConnecting()) {
        m_connectingDialog.reset(new QMessageBox(
                    QMessageBox::Information,
                    "Connecting",
                    "Connecting...",
                    QMessageBox::Cancel,
                    this));
        m_connectingDialog->setWindowModality(Qt::WindowModal);
        m_connectingDialog->show();
    }
    else {
        m_connectingDialog.reset();
        if (m_connection->isConnected()) {
            emit connected();
        }
        else {
            const QMetaEnum metaEnum = QMetaEnum::fromType<QNetworkReply::NetworkError>();
            const QString error = metaEnum.valueToKey(m_connection->error());

            QString message = "Failed to connect to " + m_url + "\n\n";
            message += error + ":\n";
            message += m_connection->errorString();
            QMessageBox::information(this, "Failed to connect", message);
        }
    }
}
