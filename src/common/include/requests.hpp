#pragma once

#include "library.hpp"

namespace NetCommon {

struct DownloadRequest
{
    enum Type
    {
        BandcampAlbum,
        YoutubeVideo,
        YoutubePlaylist,
    };

    Type tp;
    QString url;
    Moosick::ArtistId artistId;
    QString artistName;
    QString albumName;
    quint32 currentRevision;

    QByteArray toBase64() const;
    bool fromBase64(const QByteArray &base64);
};

} //namespace NetCommon
