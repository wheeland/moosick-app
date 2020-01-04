#include "playlist.hpp"
#include "util/qmlutil.hpp"

#include <QNetworkReply>

namespace Playlist {

Entry::Entry(Entry::Source source, const QString &title, const QString &artist,
             const QString &url, const QString &iconUrl, int duration, QObject *parent)
    : QObject(parent)
    , m_source(source)
    , m_title(title)
    , m_artist(artist)
    , m_url(url)
    , m_iconUrl(iconUrl)
    , m_duration(duration)
{
}

Entry::~Entry()
{
}

QString Entry::durationString() const
{
    return QString::asprintf("%d:%02d", m_duration / 60, m_duration % 60);
}

void Entry::setIconData(const QString &iconData)
{
    if (m_iconData != iconData) {
        m_iconData = iconData;
        emit iconDataChanged();
    }
}

void Entry::setSelected(bool selected)
{
    if (m_selected == selected)
        return;

    m_selected = selected;
    emit selectedChanged(m_selected);
}

Playlist::Playlist(QObject *parent)
    : QObject(parent)
{
    m_manager = new QNetworkAccessManager(this);
    connect(m_manager, &QNetworkAccessManager::finished, this, &Playlist::onNetworkReplyFinished);

    m_entries.addValueAccessor("entry");
}

Playlist::~Playlist()
{
    for (Entry *entry : m_entries.data())
        entry->deleteLater();
    m_manager->deleteLater();
}

ModelAdapter::Model *Playlist::entries() const
{
    return m_entries.model();
}

Entry *Playlist::currentSong() const
{
    return m_entries.data().value(m_currentEntry);
}

void Playlist::onSelectedChanged()
{
    const QVector<Entry*> &entries = m_entries.data();
    const auto isSelected = [](Entry *e) { return e->isSelected(); };
    const bool hasSelected = std::any_of(entries.cbegin(), entries.cend(), isSelected);

    if (m_hasSelected != hasSelected) {
        m_hasSelected = hasSelected;
        emit hasSelectedSongsChanged(hasSelected);
    }
}

void Playlist::advance(int delta)
{
    if (m_entries.size() == 0 || delta == 0)
        return;

    // advance index
    m_currentEntry += delta;
    if (m_currentEntry >= m_entries.size())
        m_currentEntry = 0;
    else if (m_currentEntry < 0)
        m_currentEntry = m_entries.size() - 1;

    emit currentSongChanged();
}

void Playlist::requestIcon(Entry *entry)
{
    const auto it = m_iconUrlToDataString.find(entry->iconUrl());
    if (it != m_iconUrlToDataString.end()) {
        entry->setIconData(it.value());
        return;
    }

    if (m_iconQueries.contains(entry->iconUrl()))
        return;

    const QUrl url(entry->iconUrl());
    const QNetworkRequest request(url);
    QNetworkReply *reply = m_manager->get(request);

    m_iconQueries[entry->iconUrl()] = reply;
}

void Playlist::onNetworkReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    // get URL for this query
    QString url;
    for (auto it = m_iconQueries.cbegin(); it != m_iconQueries.cend(); ++it) {
        if (it.value() == reply) {
            url = it.key();
            m_iconQueries.erase(it);
            break;
        }
    }

    if (url.isEmpty())
        return;

    const QByteArray data = reply->readAll();
    const QString imgData = QmlUtil::imageDataToDataUri(data, url);

    // set icon for all entries that match
    for (Entry *entry : m_entries.data()) {
        if (entry->iconUrl() == url)
            entry->setIconData(imgData);
    }
}

void Playlist::purgeUnusedIcons()
{
    for (auto it = m_iconUrlToDataString.begin(); it != m_iconUrlToDataString.end(); /*empty*/) {
        // find entries with this item
        const QVector<Entry*> &entries = m_entries.data();
        const bool isUsed = std::any_of(entries.cbegin(), entries.cend(), [&](Entry *entry) {
                return entry->iconUrl() == it.value();
        });

        if (isUsed)
            ++it;
        else
            it = m_iconUrlToDataString.erase(it);
    }
}

void Playlist::next()
{
    advance(1);
}

void Playlist::previous()
{
    advance(-1);
}

void Playlist::remove(Entry *entry)
{
    // is it a song without a collection?
    const int rootSongIndex = m_entries.data().indexOf(entry);
    if (rootSongIndex >= 0) {
        // current song?
        if (rootSongIndex == m_currentEntry) {
            m_entries.remove(entry);
            emit currentSongChanged();
        }
        else if (rootSongIndex < m_currentEntry) {
            m_currentEntry -= 1;
        }

        entry->deleteLater();
        purgeUnusedIcons();
    }
}

void Playlist::append(Entry::Source source, const QString &title, const QString &artist,
                      const QString &url, int duration, const QString &iconUrl)
{
    m_entries.add(createEntry(source, title, artist, url, duration, iconUrl));
}

void Playlist::prepend(Entry::Source source, const QString &title, const QString &artist,
                       const QString &url, int duration, const QString &iconUrl)
{
    m_entries.insert(0, createEntry(source, title, artist, url, duration, iconUrl));
}

Entry *Playlist::createEntry(Entry::Source source, const QString &title, const QString &artist, const QString &url, int duration, const QString &iconUrl)
{
    Entry *entry = new Entry(source, title, artist, url, iconUrl, duration, this);
    connect(entry, &Entry::selectedChanged, this, &Playlist::onSelectedChanged);
    requestIcon(entry);
    return entry;

}

} // namespace Playlist
