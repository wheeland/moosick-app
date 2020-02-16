#pragma once

#include <QJsonValue>
#include <QJsonArray>
#include <QJsonObject>
#include <QVector>
#include <QPair>

QJsonArray parseJsonArray(const QByteArray &json, const QByteArray &name = QByteArray());
QJsonObject parseJsonObject(const QByteArray &json, const QByteArray &name = QByteArray());

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

template <class A, class B> QJsonValue toJson(const QPair<A, B> &pair)
{
    QJsonObject obj;
    obj.insert("p1", toJson(pair.first));
    obj.insert("p2", toJson(pair.second));
    return obj;
}

template <class A, class B> bool fromJson(const QJsonValue &json, QPair<A, B> &pair)
{
    if (json.type() != QJsonValue::Object)
        return false;

    const QJsonObject obj = json.toObject();
    const auto p1 = obj.find("p1");
    const auto p2 = obj.find("p2");
    if (p1 == obj.end() || p2 == obj.end())
        return false;

    A a;
    B b;
    if (!fromJson(p1.value(), a) || !fromJson(p2.value(), b))
        return false;

    pair.first = a;
    pair.second = b;

    return true;
}
