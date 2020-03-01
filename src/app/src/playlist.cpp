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
    connect(m_http, &HttpRequester::networkError, this, &Playlist::onNetworkError);
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

void Playlist::onSelectedChanged()
{
    const QVector<Entry*> &entries = m_entries.data();
    const auto isSelected = [](Entry *e) { return e->isSelected(); };
    const bool hasSelected = std::any_of(entries.cbegin(), entries.cend(), isSelected);

    if (m_hasSelectedSongs != hasSelected) {
        m_hasSelectedSongs = hasSelected;
        emit hasSelectedSongsChanged(hasSelected);
    }
}

void Playlist::advance(int delta)
{
    if (m_entries.size() == 0 || delta == 0)
        return;

    // advance index
    m_currentEntry += delta;
    if (m_repeat) {
        while (m_currentEntry < 0)
            m_currentEntry += m_entries.size();
        while (m_currentEntry >= m_entries.size())
            m_currentEntry -= m_entries.size();
    }
    else {
        if (m_currentEntry < 0 || m_currentEntry >= m_entries.size())
            m_currentEntry = -1;
    }

    currentSongMaybeChanged();
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

void Playlist::currentSongMaybeChanged()
{
    Entry *newSong = nullptr;
    if (m_currentEntry >= 0 && m_currentEntry < m_entries.size()) {
        const int idx = m_randomized ? m_randomizedOverlay[m_currentEntry] : m_currentEntry;
        newSong = m_entries[idx];
    }

    if (m_currentSong != newSong) {
        m_currentSong = newSong;
        emit currentSongChanged();
    }
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

void Playlist::onNetworkError(HttpRequestId requestId, QNetworkReply::NetworkError error)
{
    for (auto it = m_iconQueries.cbegin(); it != m_iconQueries.cend(); ++it) {
        if (it.value() == requestId) {
            qWarning() << "Network error for icon query" << it.key() << ": " << error;
        }
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
    const int entryIndex = m_entries.data().indexOf(entry);
    if (entryIndex >= 0) {
        if (m_randomized)
            m_currentEntry = m_randomizedOverlay.indexOf(entryIndex);
        else
            m_currentEntry = entryIndex;
        currentSongMaybeChanged();
    }
}

void Playlist::remove(Entry *entry)
{
    const int entryIndex = m_entries.data().indexOf(entry);
    if (entryIndex >= 0) {
        // need to shift songs one backwards?
        if (!m_randomized && entryIndex < m_currentEntry) {
            m_currentEntry -= 1;
        }
        m_randomizedOverlay.removeAll(entryIndex);

        m_entries.remove(entry);
        entry->deleteLater();
        purgeUnusedIcons();

        currentSongMaybeChanged();
    }
}

void Playlist::addFromInternet(
    Entry::Source source, const QString &url,
    const QString &artist, const QString &album, const QString &title,
    int duration, const QString &iconUrl, bool append)
{
    Entry *entry = createEntry(source, artist, album, title, url, duration, iconUrl);
    insertEntry(entry, append);
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
    insertEntry(entry, append);
}

void Playlist::setRepeat(bool repeat)
{
    if (m_repeat == repeat)
        return;

    m_repeat = repeat;
    emit repeatChanged(m_repeat);
}

void Playlist::setRandomized(bool randomized)
{
    if (m_randomized == randomized)
        return;

    m_randomized = randomized;
    emit randomizedChanged(m_randomized);

    if (m_randomized) {
        // create random permutation of playlist songs
        for (int i = 0; i < m_entries.size(); ++i)
            m_randomizedOverlay << i;
        std::random_shuffle(m_randomizedOverlay.begin(), m_randomizedOverlay.end());

        // if we are currently playing a song, put it in front
        if (m_randomizedOverlay.removeAll(m_currentEntry))
            m_randomizedOverlay.prepend(m_currentEntry);
        m_currentEntry = 0;
    } else {
        // restore current index
        m_currentEntry = m_randomizedOverlay.value(m_currentEntry, -1);
        m_randomizedOverlay.clear();
    }
}

void Playlist::insertEntry(Entry *newEntry, bool append)
{
    if (append)
        m_entries.add(newEntry);
    else
        m_entries.insert(0, newEntry);

    if (m_randomized) {
        // shift all indices, if entry was inserted at front
        if (!append) {
            for (int &index : m_randomizedOverlay)
                ++index;
        }
        // insert new entry randomly behind current song
        const int minIndex = m_currentEntry + 1;
        const int insertIndex = minIndex + rand() % (m_randomizedOverlay.size() + 1 - minIndex);
        m_randomizedOverlay.insert(insertIndex, append ? m_entries.size() - 1 : 0);
    }
    else {
        // shift current index, if entry was inserted at front
        if (!append)
            m_currentEntry++;
    }

    // first song? -> start playing
    if (m_entries.size() == 1) {
        m_currentEntry = 0;
        currentSongMaybeChanged();
    }
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
