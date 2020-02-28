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

    QString host() const;
    quint16 port() const;
    QString userName() const;
    QString password() const;

    void writeHost(const QString &host);
    void writePort(quint16 port);
    void writeUserName(const QString &userName);
    void writePassword(const QString &password);

    QStringList allLocalSongFiles() const;
    void addLocalSongFile(const QString &fileName, const QByteArray &data);
    void removeLocalSongFile(const QString &fileName);

private:
    QString m_storageDirectory;
    QSettings m_settings;
};
