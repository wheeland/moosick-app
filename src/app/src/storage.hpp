#pragma once

#include "library.hpp"

#include <QString>
#include <QSettings>

class Storage
{
public:
    Storage();
    ~Storage();

    bool hasValidLocalStorageDir() const;
    bool setLocalStorageDir(const QString &dir);

    bool readLibrary(Moosick::Library &library) const;
    void writeLibrary(const Moosick::Library &library);

    QString readHost() const;
    void writeHost(const QString &host);

    QStringList allLocalSongFiles() const;
    void addLocalSongFile(const QString &fileName, const QByteArray &data);
    void removeLocalSongFile(const QString &fileName);

private:
    QString storageDir() const;

    QString m_storageDirectory;
    QSettings m_settings;
};
