#include "jsonconv.hpp"

#include <QJsonDocument>
#include <QDebug>

#define REQUIRE_TYPE(JSON, TYPE) if (JSON.type() != QJsonValue::TYPE) return false;

static QJsonDocument parseJsonDocument(const QByteArray &json, const QByteArray &name)
{
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &error);
    if (doc.isNull()) {
        qWarning().noquote() << "Failed to parse" << name;
        qWarning().noquote() << json;
        qWarning() << "Error:";
        qWarning().noquote() << error.errorString();
    }
    return doc;
}

QJsonArray parseJsonArray(const QByteArray &json, const QByteArray &name)
{
    const QJsonDocument doc = parseJsonDocument(json, name);
    if (doc.isNull())
        return QJsonArray();

    if (!doc.isArray()) {
        qWarning().noquote() << name << "is not a JSON array";
        return QJsonArray();
    }

    return doc.array();
}

QJsonObject parseJsonObject(const QByteArray &json, const QByteArray &name)
{
    const QJsonDocument doc = parseJsonDocument(json, name);
    if (doc.isNull())
        return QJsonObject();

    if (!doc.isObject()) {
        qWarning().noquote() << name << "is not a JSON object";
        return QJsonObject();
    }

    return doc.object();
}

bool fromJson(const QJsonValue &json, quint64 &value)
{
    REQUIRE_TYPE(json, Double)
    value = (quint64) json.toDouble();
    return true;
}

bool fromJson(const QJsonValue &json, qint64 &value)
{
    REQUIRE_TYPE(json, Double)
    value = (qint64) json.toDouble();
    return true;
}

bool fromJson(const QJsonValue &json, quint32 &value)
{
    REQUIRE_TYPE(json, Double)
    value = (quint32) json.toDouble();
    return true;
}

bool fromJson(const QJsonValue &json, qint32 &value)
{
    REQUIRE_TYPE(json, Double)
    value = (qint32) json.toDouble();
    return true;
}

bool fromJson(const QJsonValue &json, QString &value)
{
    REQUIRE_TYPE(json, String)
    value = json.toString();
    return true;
}

bool fromJson(const QJsonValue &json, bool &value)
{
    REQUIRE_TYPE(json, Bool)
    value = json.toBool();
    return true;
}

QJsonValue toJson(qint32 value)
{
    return QJsonValue(value);
}

QJsonValue toJson(qint64 value)
{
    return QJsonValue(value);
}

QJsonValue toJson(quint32 value)
{
    return QJsonValue((qint64) value);
}

QJsonValue toJson(quint64 value)
{
    return QJsonValue((qint64) value);
}

QJsonValue toJson(const QString &value)
{
    return QJsonValue(value);
}

QJsonValue toJson(bool value)
{
    return QJsonValue(value);
}
