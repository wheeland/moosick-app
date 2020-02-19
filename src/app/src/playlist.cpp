#include "playlist.hpp"
#include "util/qmlutil.hpp"

namespace Playlist {

Entry::Entry(Entry::Source source,
             const QString &artist, const QString &album, const QString &title,
             const QString &url, const QString &iconUrl, int duration, QObject *parent)
    : QObject(parent)
    , m_source(source)
    , m_title(title)
    , m_artist(artist)
    , m_album(album)
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

Playlist::Playlist(HttpClient *httpClient, QObject *parent)
    : QObject(parent)
    , m_http(new HttpRequester(httpClient))
{
    m_entries.addValueAccessor("entry");
    connect(m_http, &HttpRequester::receivedReply, this, &Playlist::onNetworkReplyFinished);
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

void Playlist::requestIcon(Entry *entry)
{
    if (entry->iconUrl().isEmpty())
        return;

    const auto it = m_iconUrlToDataString.find(entry->iconUrl());
    if (it != m_iconUrlToDataString.end()) {
        entry->setIconData(it.value());
        return;
    }

    if (m_iconQueries.contains(entry->iconUrl()))
        return;

    HttpRequestId reply = m_http->request(QNetworkRequest(QUrl(entry->iconUrl())));
    m_iconQueries[entry->iconUrl()] = reply;
}

void Playlist::onNetworkReplyFinished(HttpRequestId requestId, const QByteArray &data)
{
    // get URL for this query
    QString url;
    for (auto it = m_iconQueries.cbegin(); it != m_iconQueries.cend(); ++it) {
        if (it.value() == requestId) {
            url = it.key();
            m_iconQueries.erase(it);
            break;
        }
    }

    if (url.isEmpty())
        return;

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

void Playlist::play(Entry *entry)
{
    const int rootSongIndex = m_entries.data().indexOf(entry);
    if (rootSongIndex >= 0) {
        m_currentEntry = rootSongIndex;
        emit currentSongChanged();
    }
}

void Playlist::remove(Entry *entry)
{
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

void Playlist::addFromInternet(
    Entry::Source source, const QString &url,
    const QString &artist, const QString &album, const QString &title,
    int duration, const QString &iconUrl, bool append)
{
    Entry *entry = createEntry(source, artist, album, title, url, duration, iconUrl);

    if (append)
        m_entries.add(entry);
    else
        m_entries.insert(0, entry);

    if (m_entries.size() == 1)
        emit currentSongChanged();
}

void Playlist::addFromLibrary(const QString &fileName, const QString &artist, const QString &album, const QString &title, int duration, bool append)
{
    QString url = "http://";
    url += m_http->host();
    url += ":";
    url += QString::number(m_http->port());
    url += "/";
    url += fileName;

    Entry *entry = createEntry(Entry::Library, artist, album, title, url, duration, "");

    if (append)
        m_entries.add(entry);
    else
        m_entries.insert(0, entry);

    if (m_entries.size() == 1)
        emit currentSongChanged();
}

Entry *Playlist::createEntry(Entry::Source source,
                             const QString &artist, const QString &album, const QString &title,
                             const QString &url, int duration, const QString &iconUrl)
{
    Entry *entry = new Entry(source, artist, album, title, url, iconUrl, duration, this);
    connect(entry, &Entry::selectedChanged, this, &Playlist::onSelectedChanged);
    requestIcon(entry);
    return entry;

}

} // namespace Playlist
