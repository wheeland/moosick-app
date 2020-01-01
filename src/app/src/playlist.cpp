#include "playlist.hpp"

#include "util/qmlutil.hpp"

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

void Entry::setIconData(const QString &url, const QByteArray &data)
{
    const QString icon = QmlUtil::imageDataToDataUri(data, url);
    if (m_iconData != icon) {
        m_iconData = icon;
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
    m_entries.addValueAccessor("entry");
}

Playlist::~Playlist()
{
    for (Entry *entry : m_entries.data())
        entry->deleteLater();
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

void Playlist::requestIcon(const QString &url)
{
    const auto it = m_icons.find(url);
    if (it != m_icons.end())
        return;

    m_icons[url] = "";

    // TODO: request
}

void Playlist::purgeUnusedIcons()
{
    for (auto it = m_icons.begin(); it != m_icons.end(); /*empty*/) {
        // find entries with this item
        const QVector<Entry*> &entries = m_entries.data();
        const bool isUsed = std::any_of(entries.cbegin(), entries.cend(), [&](Entry *entry) {
                return entry->iconUrl() == it.value();
        });

        if (isUsed)
            ++it;
        else
            it = m_icons.erase(it);
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
    requestIcon(iconUrl);
    return entry;

}

} // namespace Playlist
