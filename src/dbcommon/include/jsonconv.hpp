#pragma once

#include <QJsonValue>
#include <QJsonArray>
#include <QJsonObject>
#include <QVector>

QJsonArray parseJsonArray(const QByteArray &json, const QByteArray &name);
QJsonObject parseJsonObject(const QByteArray &json, const QByteArray &name);

QJsonValue toJson(qint32 value);
QJsonValue toJson(quint32 value);
QJsonValue toJson(qint64 value);
QJsonValue toJson(quint64 value);
QJsonValue toJson(const QString &value);
QJsonValue toJson(bool value);

bool fromJson(const QJsonValue &json, quint64 &value);
bool fromJson(const QJsonValue &json, qint64 &value);
bool fromJson(const QJsonValue &json, quint32 &value);
bool fromJson(const QJsonValue &json, qint32 &value);
bool fromJson(const QJsonValue &json, QString &value);
bool fromJson(const QJsonValue &json, bool &value);

template <class T> QJsonValue toJson(const QVector<T> &values)
{
    QJsonArray array;
    for (const T &value : values)
        array.append(toJson(value));
    return array;
}

template <class T> bool fromJson(const QJsonValue &json, QVector<T> &values)
{
    if (json.type() != QJsonValue::Array)
        return false;
    const QJsonArray array = json.toArray();
    QVector<T> ret;
    T tVal;
    for (const QJsonValue &value : array) {
        if (!fromJson(value, tVal))
            return false;
        ret.append(tVal);
    }
    values = ret;
    return true;
}
