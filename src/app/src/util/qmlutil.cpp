#include "qmlutil.hpp"

#include <QBuffer>

namespace QmlUtil {

QString imageToDataUri(const QImage &image)
{
    QBuffer buffer;
    image.save(&buffer, "PNG");
    QByteArray imgData = buffer.data();
    QString imageDataUri = "data:image/png;base64,";
    imageDataUri += imgData.toBase64();
    return imageDataUri;
}

QString imageDataToDataUri(const QByteArray &data, const QString &formatHint)
{
    QString format;
    QByteArray imgData = data;

    if (formatHint.contains(".jpg"))
        format = "jpg";
    else if (formatHint.contains(".png"))
        format = "png";
    else {
        // if nothing matches, parse the image and save it as PNG
        const QImage image = QImage::fromData(data);
        QBuffer buffer;
        image.save(&buffer, "PNG");
        imgData = buffer.data();
        format = "png";
    }

    QString imageDataUri = "data:image/";
    imageDataUri += format += ";base64,";
    imageDataUri += imgData.toBase64();

    return imageDataUri;
}

} // QmlUtil
