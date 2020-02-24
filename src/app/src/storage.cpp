#include "storage.hpp"
#include "jsonconv.hpp"

#include <QFile>
#include <QDir>
#include <QDebug>
#include <QJsonDocument>

static const char *LIBRARY_FILE = "lib.json.gz";

static const char *SETTINGS_HOST = "host";
static const char *SETTINGS_STORAGE = "storage";
static const char *SETTINGS_LOCALSONGS = "localsongs";

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
    const QJsonObject jsonObj = parseJsonObject(json, "Library");
    if (jsonObj.isEmpty()) {
        qWarning() << "Failed to parse JSON from" << file.fileName();
        return false;
    }
    if (!library.deserializeFromJson(jsonObj)) {
        qWarning() << "Failed to parse Library from" << file.fileName();
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

QString Storage::readHost() const
{
    return m_settings.value(SETTINGS_HOST).toString();
}

void Storage::writeHost(const QString &host)
{
    m_settings.setValue(SETTINGS_HOST, host);
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