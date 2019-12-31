#include "requests.hpp"
#include <QDataStream>

namespace NetCommon {

QByteArray DownloadRequest::toBase64() const
{
    QByteArray ret;
    QDataStream out(&ret, QIODevice::WriteOnly);
    out << static_cast<quint32>(tp);
    out << url;
    out << artistId;
    out << artistName;
    out << albumName;
    return ret.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}

bool DownloadRequest::fromBase64(const QByteArray &base64)
{
    QByteArray data = QByteArray::fromBase64(base64, QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    QDataStream in(&data, QIODevice::ReadOnly);

    quint32 tp32, id32;
    QString u, artist, album;
    in >> tp32;
    in >> u;
    in >> id32;
    in >> artist;
    in >> album;

    if (in.status() == QDataStream::Ok) {
        tp = static_cast<Type>(tp32);
        url = u;
        artistId = id32;
        artistName = artist;
        albumName = album;
        return true;
    }

    return false;
}

} //namespace NetCommon
