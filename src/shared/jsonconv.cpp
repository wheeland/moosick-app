#include "jsonconv.hpp"

#include <QJsonDocument>

JsonifyError::JsonifyError()
    : m_type(NoError)
{
}

JsonifyError::JsonifyError(const JsonifyError &other)
    : m_type(NoError)
{
    *this = other;
}

JsonifyError::JsonifyError(JsonifyError &&other)
    : m_type(NoError)
{
    *this = std::move(other);
}

JsonifyError::~JsonifyError()
{
    clear();
}

void JsonifyError::clear()
{
    switch (m_type) {
    case NoError:
        break;
    case ParseError:
        m_parseError.~QJsonParseError();
        break;
    case TypeError:
        m_typeError.~TypeErrorDetails();
        break;
    case CustomError:
        m_customError.~CustomErrorDetails();
        break;
    case MissingMemberError:
        m_missingMemberError.~MissingMemberErrorDetails();
        break;
    case MemberError:
        m_memberError.~MemberErrorDetails();
        break;
    case ElementError:
        m_elementError.~ElementErrorDetails();
        break;
    default:
        qFatal("Invalid JsonifyError type");
    }
    m_type = NoError;
}

JsonifyError &JsonifyError::operator=(const JsonifyError &rhs)
{
    clear();

    m_type = rhs.m_type;
    switch (m_type) {
    case NoError:
        break;
    case ParseError:
        new (&m_parseError) QJsonParseError(rhs.m_parseError);
        break;
    case TypeError:
        new (&m_typeError) TypeErrorDetails(rhs.m_typeError);
        break;
    case CustomError:
        new (&m_customError) CustomErrorDetails(rhs.m_customError.message, rhs.m_customError.error ? new JsonifyError(*rhs.m_customError.error.get()) : nullptr);
        break;
    case MissingMemberError:
        new (&m_missingMemberError) MissingMemberErrorDetails(rhs.m_missingMemberError);
        break;
    case MemberError:
        new (&m_memberError) MemberErrorDetails(rhs.m_memberError.name, new JsonifyError(*rhs.m_memberError.error.get()));
        break;
    case ElementError:
        new (&m_elementError) ElementErrorDetails(rhs.m_elementError.element, new JsonifyError(*rhs.m_elementError.error.get()));
        break;
    default:
        qFatal("Invalid JsonifyError type");
    }

    return *this;
}

JsonifyError &JsonifyError::operator=(JsonifyError &&rhs)
{
    clear();

    m_type = rhs.m_type;
    switch (m_type) {
    case NoError:
        break;
    case ParseError:
        new (&m_parseError) QJsonParseError(std::move(rhs.m_parseError));
        break;
    case TypeError:
        new (&m_typeError) TypeErrorDetails(std::move(rhs.m_typeError));
        break;
    case CustomError:
        new (&m_customError) CustomErrorDetails(std::move(rhs.m_customError.message), rhs.m_customError.error ? rhs.m_customError.error.take() : nullptr);
        break;
    case MissingMemberError:
        new (&m_missingMemberError) MissingMemberErrorDetails(std::move(rhs.m_missingMemberError));
        break;
    case MemberError:
        new (&m_memberError) MemberErrorDetails(std::move(rhs.m_memberError.name), rhs.m_memberError.error.take());
        break;
    case ElementError:
        new (&m_elementError) ElementErrorDetails(rhs.m_elementError.element, rhs.m_elementError.error.take());
        break;
    default:
        qFatal("Invalid JsonifyError type");
    }

    rhs.clear();

    return *this;
}

static const char *jsonTypeName(QJsonValue::Type jsonType)
{
    switch (jsonType)
    {
    case QJsonValue::Null: return "Null";
    case QJsonValue::Bool: return "Bool";
    case QJsonValue::Double: return "Number";
    case QJsonValue::String: return "String";
    case QJsonValue::Array: return "Array";
    case QJsonValue::Object: return "Object";
    case QJsonValue::Undefined: return "Undefined";
    default: qFatal("QJsonValue::Type");
    }
}

QString JsonifyError::toString() const
{
    switch (m_type) {
    case NoError:
        return QString("No Error");
    case ParseError:
        return QString("Parse Error: ") + m_parseError.errorString();
    case TypeError:
        return QString("Type Error: expected ")
                + jsonTypeName(m_typeError.expected)
                + ", found "
                + jsonTypeName(m_typeError.found);
    case CustomError:
        return m_customError.error.isNull() ? m_customError.message
                                            : m_customError.message + ": " + m_customError.error->toString();
    case MissingMemberError:
        return QString("Missing Member: ") + m_missingMemberError.name;
    case MemberError:
        return m_memberError.name + ": " + m_memberError.error->toString();
    case ElementError:
        return QString("Element ") + QString::number(m_elementError.element) + ": " + m_elementError.error->toString();
    default:
        qFatal("Invalid JsonifyError type");
    }
}


JsonifyError JsonifyError::buildParseError(const QJsonParseError &parseError)
{
    JsonifyError err;
    err.m_type = ParseError;
    new (&err.m_parseError) QJsonParseError(parseError);
    return err;
}

JsonifyError JsonifyError::buildTypeError(QJsonValue::Type expected, QJsonValue::Type found)
{
    JsonifyError err;
    err.m_type = TypeError;
    new (&err.m_typeError) TypeErrorDetails{ expected, found };
    return err;
}

JsonifyError JsonifyError::buildCustomError(const QString &message)
{
    JsonifyError err;
    err.m_type = CustomError;
    new (&err.m_customError) CustomErrorDetails{ message, nullptr };
    return err;
}

JsonifyError JsonifyError::buildCustomError(const QString &message, const JsonifyError &jsonError)
{
    JsonifyError err;
    err.m_type = CustomError;
    new (&err.m_customError) CustomErrorDetails{ message, new JsonifyError(jsonError) };
    return err;
}

JsonifyError JsonifyError::buildCustomError(const QString &message, JsonifyError &&jsonError)
{
    JsonifyError err;
    err.m_type = CustomError;
    new (&err.m_customError) CustomErrorDetails{ message, new JsonifyError(std::move(jsonError)) };
    return err;
}

JsonifyError JsonifyError::buildMissingMemberError(const QString &name)
{
    JsonifyError err;
    err.m_type = MissingMemberError;
    new (&err.m_missingMemberError) MissingMemberErrorDetails{ name };
    return err;
}

JsonifyError JsonifyError::buildMemberError(const QString &name, const JsonifyError &memberError)
{
    JsonifyError err;
    err.m_type = MemberError;
    new (&err.m_memberError) MemberErrorDetails(name, new JsonifyError(memberError));
    return err;
}

JsonifyError JsonifyError::buildMemberError(const QString &name, JsonifyError &&memberError)
{
    JsonifyError err;
    err.m_type = MemberError;
    new (&err.m_memberError) MemberErrorDetails(name, new JsonifyError(std::move(memberError)));
    return err;
}

JsonifyError JsonifyError::buildElementError(int element, const JsonifyError &elementError)
{
    JsonifyError err;
    err.m_type = ElementError;
    new (&err.m_elementError) ElementErrorDetails(element, new JsonifyError(elementError));
    return err;
}

JsonifyError JsonifyError::buildElementError(int element, JsonifyError &&elementError)
{
    JsonifyError err;
    err.m_type = ElementError;
    new (&err.m_elementError) ElementErrorDetails(element, new JsonifyError(std::move(elementError)));
    return err;
}

void dejson(const QJsonValue &json, Result<quint64, JsonifyError> &result)
{
    JSONIFY_DEJSON_EXPECT_TYPE(json, result, Double);
    result = (quint64) json.toDouble();
}

void dejson(const QJsonValue &json, Result<qint64, JsonifyError> &result)
{
    JSONIFY_DEJSON_EXPECT_TYPE(json, result, Double);
    result = (qint64) json.toDouble();
}

void dejson(const QJsonValue &json, Result<quint32, JsonifyError> &result)
{
    JSONIFY_DEJSON_EXPECT_TYPE(json, result, Double);
    result = (quint32) json.toDouble();
}

void dejson(const QJsonValue &json, Result<qint32, JsonifyError> &result)
{
    JSONIFY_DEJSON_EXPECT_TYPE(json, result, Double);
    result = (qint32) json.toDouble();
}

void dejson(const QJsonValue &json, Result<float, JsonifyError> &result)
{
    JSONIFY_DEJSON_EXPECT_TYPE(json, result, Double);
    result = (float) json.toDouble();
}

void dejson(const QJsonValue &json, Result<double, JsonifyError> &result)
{
    JSONIFY_DEJSON_EXPECT_TYPE(json, result, Double);
    result = json.toDouble();
}

void dejson(const QJsonValue &json, Result<QString, JsonifyError> &result)
{
    JSONIFY_DEJSON_EXPECT_TYPE(json, result, String);
    result = json.toString();
}

void dejson(const QJsonValue &json, Result<bool, JsonifyError> &result)
{
    JSONIFY_DEJSON_EXPECT_TYPE(json, result, Bool);
    result = json.toBool();
}

QJsonValue enjson(qint32 value)
{
    return QJsonValue(value);
}

QJsonValue enjson(qint64 value)
{
    return QJsonValue(value);
}

QJsonValue enjson(quint32 value)
{
    return QJsonValue((qint64) value);
}

QJsonValue enjson(quint64 value)
{
    return QJsonValue((qint64) value);
}

QJsonValue enjson(float value)
{
    return QJsonValue((double) value);
}

QJsonValue enjson(double value)
{
    return QJsonValue(value);
}

QJsonValue enjson(const QString &value)
{
    return QJsonValue(value);
}

QJsonValue enjson(bool value)
{
    return QJsonValue(value);
}

QJsonValue enjson(const JsonObject &value)
{
    return value.serialize();
}

QJsonValue JsonObject::serialize() const
{
    QJsonObject ret;

    for (uintptr_t memberOffset : m_memberOffsets) {
        JsonMemberBase *member = getMember(memberOffset);
        ret.insert(member->name(), member->get());
    }

    return QJsonValue(ret);
}

JsonifyError JsonObject::deserialize(const QJsonValue &json)
{
    if (!json.isObject())
        return JsonifyError::buildTypeError(QJsonValue::Object, json.type());

    const QJsonObject obj = json.toObject();
    JsonifyError error;

    for (uintptr_t memberOffset : m_memberOffsets) {
        JsonMemberBase *member = getMember(memberOffset);
        const auto val = obj.find(member->name());
        if (val == obj.end()) {
            error = JsonifyError::buildMissingMemberError(member->name());
            break;
        }

        error = member->set(val.value());
        if (error.isError())
            break;
    }

    if (error.isError()) {
        for (uintptr_t memberOffset : m_memberOffsets) {
            JsonMemberBase *member = getMember(memberOffset);
            member->reset();
        }
    }

    return error;
}

QByteArray jsonSerializeObject(const QJsonObject &jsonObject, QJsonDocument::JsonFormat format)
{
    return QJsonDocument(jsonObject).toJson(format);
}

QByteArray jsonSerializeArray(const QJsonArray &jsonArray, QJsonDocument::JsonFormat format)
{
    return QJsonDocument(jsonArray).toJson(format);
}

Result<QJsonValue, JsonifyError> jsonDeserialize(const QByteArray &json)
{
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &error);

    if (error.error != QJsonParseError::NoError)
        return JsonifyError::buildParseError(error);

    if (doc.isEmpty() || doc.isNull())
        return JsonifyError::buildTypeError(QJsonValue::Object, QJsonValue::Null);

    if (doc.isArray()) {
        return QJsonValue(doc.array());
    }
    else {
        Q_ASSERT(doc.isObject());
        return QJsonValue(doc.object());
    }
}

Result<QJsonObject, JsonifyError> jsonDeserializeObject(const QByteArray &json)
{
    Result<QJsonValue, JsonifyError> value = jsonDeserialize(json);
    if (value.hasError())
        return value.takeError();
    if (value.getValue().isArray())
        return JsonifyError::buildTypeError(QJsonValue::Object, QJsonValue::Array);
    return value.takeValue().toObject();
}

Result<QJsonArray, JsonifyError> jsonDeserializeArray(const QByteArray &json)
{
    Result<QJsonValue, JsonifyError> value = jsonDeserialize(json);
    if (value.hasError())
        return value.takeError();
    if (value.getValue().isObject())
        return JsonifyError::buildTypeError(QJsonValue::Array, QJsonValue::Object);
    return value.takeValue().toArray();
}
