#include "storage.hpp"
#include "jsonconv.hpp"

#include <QFile>
#include <QDir>
#include <QDebug>
#include <QJsonDocument>

static const char *LIBRARY_FILE = "lib.json.gz";

static const char *SETTINGS_HOST = "host";
static const char *SETTINGS_PORT = "port";
static const char *SETTINGS_USER = "user";
static const char *SETTINGS_PASS = "pass";
static const char *SETTINGS_STORAGE = "storage";
static const char *SETTINGS_LOCALSONGS = "localsongs";
static const char *SETTINGS_IGNORED_SSL_ERRORS = "ignoredSslErrors";

Storage::Storage()
{
    m_storageDirectory = m_settings.value(SETTINGS_STORAGE).toString();
}

Storage::~Storage()
{
}

bool Storage::hasValidLocalStorageDir() const
{
    const QDir dir(m_storageDirectory);
    return !m_storageDirectory.isEmpty() && dir.exists();
}

bool Storage::setLocalStorageDir(const QString &dir)
{
    m_storageDirectory = dir;
    if (!m_storageDirectory.endsWith(QDir::separator()))
        m_storageDirectory += QDir::separator();

    m_settings.setValue(SETTINGS_STORAGE, m_storageDirectory);
    return hasValidLocalStorageDir();
}

bool Storage::readLibrary(Moosick::Library &library) const
{
    QFile file(m_storageDirectory + LIBRARY_FILE);
    if (!file.exists())
        return false;

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open" << file.fileName() << "for reading";
        return false;
    }

    const QByteArray data = file.readAll();
    const QByteArray json = qUncompress(data);
    auto result = jsonDeserializeObject(json);
    if (result.hasError()) {
        qWarning().noquote().nospace() << "Failed to parse JSON from " << file.fileName() << ": " << result.takeError().toString();
        return false;
    }

    const QJsonObject jsonObj = result.takeValue();
    JsonifyError error = library.deserializeFromJson(jsonObj);
    if (error.isError()) {
        qWarning().noquote().nospace() << "Failed to parse Library from " << file.fileName() << ": " << error.toString();
        return false;
    }
    return true;
}

void Storage::writeLibrary(const Moosick::Library &library)
{
    QFile file(m_storageDirectory + LIBRARY_FILE);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open" << file.fileName() << "for writing";
        return;
    }

    const QJsonObject jsonObj = library.serializeToJson();
    const QByteArray json = QJsonDocument(jsonObj).toJson(QJsonDocument::Compact);
    const QByteArray data = qCompress(json);
    file.write(data);
}

QString Storage::host() const
{
    return m_settings.value(SETTINGS_HOST).toString();
}

quint16 Storage::port() const
{
    return m_settings.value(SETTINGS_PORT).toUInt();
}

QString Storage::userName() const
{
    return m_settings.value(SETTINGS_USER).toString();
}

QString Storage::password() const
{
    return m_settings.value(SETTINGS_PASS).toString();
}

void Storage::writeHost(const QString &host)
{
    m_settings.setValue(SETTINGS_HOST, host);
}

void Storage::writePort(quint16 port)
{
    m_settings.setValue(SETTINGS_PORT, port);
}

void Storage::writeUserName(const QString &userName)
{
    m_settings.setValue(SETTINGS_USER, userName);
}

void Storage::writePassword(const QString &password)
{
    m_settings.setValue(SETTINGS_PASS, password);
}

QStringList Storage::allLocalSongFiles() const
{
    return m_settings.value(SETTINGS_LOCALSONGS).value<QStringList>();
}

void Storage::addLocalSongFile(const QString &fileName, const QByteArray &data)
{
    QStringList files = allLocalSongFiles();

    QFile file(m_storageDirectory + fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open" << file.fileName() << "for writing";
        return;
    }

    if (file.write(data) != data.size()) {
        qWarning() << "Failed to write data to" << file.fileName();
        return;
    }

    files << fileName;
    m_settings.setValue(SETTINGS_LOCALSONGS, files);
}

void Storage::removeLocalSongFile(const QString &fileName)
{
    if (QFile::remove(m_storageDirectory + fileName)) {
        QStringList files = m_settings.value(SETTINGS_LOCALSONGS).value<QStringList>();
        files.removeAll(fileName);
        m_settings.setValue(SETTINGS_LOCALSONGS, files);
    }
}

QByteArray Storage::ignoredSslErrorData() const
{
    return m_settings.value(SETTINGS_IGNORED_SSL_ERRORS).toByteArray();
}

void Storage::writeIgnoredSslErrorData(const QByteArray &data)
{
    m_settings.setValue(SETTINGS_IGNORED_SSL_ERRORS, data);
}
