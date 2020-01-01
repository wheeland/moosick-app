#pragma once

#include "util/modeladapter.hpp"

namespace Playlist {

class Entry : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Source source READ source CONSTANT)
    Q_PROPERTY(QString artist READ artist CONSTANT)
    Q_PROPERTY(QString title READ title CONSTANT)
    Q_PROPERTY(QString url READ url CONSTANT)
    Q_PROPERTY(QString durationString READ durationString CONSTANT)
    Q_PROPERTY(QString iconData READ iconData NOTIFY iconDataChanged)
    Q_PROPERTY(bool selected READ isSelected WRITE setSelected NOTIFY selectedChanged)

public:
    enum Source {
        Bandcamp,
        Youtube,
        Library,
    };

    Entry(Source source, const QString &title, const QString &artist,
          const QString &url, const QString &iconUrl, int duration, QObject *parent = nullptr);
    ~Entry() override;

    Source source() const { return m_source; }
    QString title() const { return m_title; }
    QString artist() const { return m_artist; }
    QString iconData() const { return m_iconData; }
    QString url() const { return m_url; }
    QString iconUrl() const { return m_iconUrl; }
    bool isSelected() const { return m_selected; }

    QString durationString() const;

    void setSelected(bool selected);
    void setIconData(const QString &url, const QByteArray &data);

signals:
    void iconDataChanged();
    void selectedChanged(bool selected);

private:
    Source m_source;
    QString m_title;
    QString m_artist;
    QString m_iconData;
    QString m_url;
    QString m_iconUrl;
    int m_duration = 0;
    bool m_selected = false;
};

class Playlist : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Entry *currentSong READ currentSong NOTIFY currentSongChanged)
    Q_PROPERTY(ModelAdapter::Model *entries READ entries CONSTANT)
    Q_PROPERTY(bool hasSelectedSongs READ hasSelectedSongs NOTIFY hasSelectedSongsChanged)

public:
    Playlist(QObject *parent = nullptr);
    ~Playlist();

    ModelAdapter::Model *entries() const;
    Entry *currentSong() const;
    bool hasSelectedSongs() const { return m_hasSelected; }

    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void remove(Entry *entry);

    void append(Entry::Source source, const QString &title, const QString &artist,
                const QString &url, int duration, const QString &iconUrl);

    void prepend(Entry::Source source, const QString &title, const QString &artist,
                 const QString &url, int duration, const QString &iconUrl);

signals:
    void currentSongChanged();
    void hasSelectedSongsChanged(bool hasSelectedSongs);

private slots:
    void onSelectedChanged();

private:
    Entry *createEntry(Entry::Source source, const QString &title, const QString &artist,
                       const QString &url, int duration, const QString &iconUrl);
    void advance(int delta);

    void purgeUnusedIcons();
    void requestIcon(const QString &url);

    ModelAdapter::Adapter<Entry*> m_entries;
    QHash<QString, QString> m_icons;
    int m_currentEntry = 0;
    bool m_hasSelected = false;
};

} // namespace Playlist

Q_DECLARE_METATYPE(Playlist::Entry*)