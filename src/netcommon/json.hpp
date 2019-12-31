#pragma once

#include <QJsonObject>
#include <QVector>

namespace NetCommon {

struct BandcampSongInfo
{
    QString name;
    QString url;
    int secs;

    bool fromJson(const QJsonObject &json);
};

struct BandcampAlbumInfo
{
    QString name;
    QString url;
    QString icon;
    QVector<BandcampSongInfo> tracks;

    bool fromJson(const QJsonObject &json);
};

} //namespace NetCommon
