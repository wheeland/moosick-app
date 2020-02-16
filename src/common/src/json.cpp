#include "json.hpp"

#include <QStringList>
#include <QJsonArray>

namespace NetCommon {

bool BandcampSongInfo::fromJson(const QJsonObject &json)
{
    const auto nameIt = json.find("name");
    const auto urlIt = json.find("url");
    const auto durIt = json.find("duration");

    if ((nameIt == json.end()) || (urlIt == json.end()) || (durIt == json.end()))
        return false;

    const QStringList durParts = durIt->toString().split(":");
    if (durParts.size() != 2)
        return false;

    bool ok1, ok2;
    const int durMins = durParts[0].toInt(&ok1);
    const int durSecs = durParts[1].toInt(&ok2);
    if (!ok1 || !ok2)
        return false;

    name = nameIt->toString();
    url = urlIt->toString();
    secs = durMins * 60 + durSecs;

    return true;
}

bool BandcampAlbumInfo::fromJson(const QJsonObject &json)
{
    const auto nameIt = json.find("name");
    const auto urlIt = json.find("url");
    const auto iconIt = json.find("icon");
    const auto tracksIt = json.find("tracks");

    if ((nameIt == json.end()) || (iconIt == json.end()) || (tracksIt == json.end()) || (urlIt == json.end()))
        return false;

    const QJsonArray trackArray = tracksIt->toArray();
    if (trackArray.isEmpty())
        return false;

    QVector<BandcampSongInfo> songInfo;
    for (const QJsonValue &track : trackArray) {
        BandcampSongInfo song;
        if (!song.fromJson(track.toObject()))
            return false;
        songInfo << song;
    }

    name = nameIt->toString();
    url = urlIt->toString();
    icon = iconIt->toString();
    tracks = songInfo;

    return true;
}

} //namespace NetCommon
