#pragma once

#include <QImage>

namespace QmlUtil {

QString imageToDataUri(const QImage &image);
QString imageDataToDataUri(const QByteArray &data, const QString &formatHint);

} // QmlUtil
