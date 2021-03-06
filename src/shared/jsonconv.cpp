#include "jsonconv.hpp"

#include <QJsonDocument>

EnjsonError::EnjsonError()
    : m_type(NoError)
{
}

EnjsonError::EnjsonError(const EnjsonError &other)
    : m_type(NoError)
{
    *this = other;
}

EnjsonError::EnjsonError(EnjsonError &&other)
    : m_type(NoError)
{
    *this = std::move(other);
}

EnjsonError::~EnjsonError()
{
    clear();
}

void EnjsonError::clear()
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
        qFatal("Invalid EnjsonError type");
    }
    m_type = NoError;
}

EnjsonError &EnjsonError::operator=(const EnjsonError &rhs)
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
        new (&m_customError) CustomErrorDetails(rhs.m_customError.message, rhs.m_customError.error ? new EnjsonError(*rhs.m_customError.error.get()) : nullptr);
        break;
    case MissingMemberError:
        new (&m_missingMemberError) MissingMemberErrorDetails(rhs.m_missingMemberError);
        break;
    case MemberError:
        new (&m_memberError) MemberErrorDetails(rhs.m_memberError.name, new EnjsonError(*rhs.m_memberError.error.get()));
        break;
    case ElementError:
        new (&m_elementError) ElementErrorDetails(rhs.m_elementError.element, new EnjsonError(*rhs.m_elementError.error.get()));
        break;
    default:
        qFatal("Invalid EnjsonError type");
    }

    return *this;
}

EnjsonError &EnjsonError::operator=(EnjsonError &&rhs)
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
        qFatal("Invalid EnjsonError type");
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

QString EnjsonError::toString() const
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
        qFatal("Invalid EnjsonError type");
    }
}


EnjsonError EnjsonError::buildParseError(const QJsonParseError &parseError)
{
    EnjsonError err;
    err.m_type = ParseError;
    new (&err.m_parseError) QJsonParseError(parseError);
    return err;
}

EnjsonError EnjsonError::buildTypeError(QJsonValue::Type expected, QJsonValue::Type found)
{
    EnjsonError err;
    err.m_type = TypeError;
    new (&err.m_typeError) TypeErrorDetails{ expected, found };
    return err;
}

EnjsonError EnjsonError::buildCustomError(const QString &message)
{
    EnjsonError err;
    err.m_type = CustomError;
    new (&err.m_customError) CustomErrorDetails{ message, nullptr };
    return err;
}

EnjsonError EnjsonError::buildCustomError(const QString &message, const EnjsonError &jsonError)
{
    EnjsonError err;
    err.m_type = CustomError;
    new (&err.m_customError) CustomErrorDetails{ message, new EnjsonError(jsonError) };
    return err;
}

EnjsonError EnjsonError::buildCustomError(const QString &message, EnjsonError &&jsonError)
{
    EnjsonError err;
    err.m_type = CustomError;
    new (&err.m_customError) CustomErrorDetails{ message, new EnjsonError(std::move(jsonError)) };
    return err;
}

EnjsonError EnjsonError::buildMissingMemberError(const QString &name)
{
    EnjsonError err;
    err.m_type = MissingMemberError;
    new (&err.m_missingMemberError) MissingMemberErrorDetails{ name };
    return err;
}

EnjsonError EnjsonError::buildMemberError(const QString &name, const EnjsonError &memberError)
{
    EnjsonError err;
    err.m_type = MemberError;
    new (&err.m_memberError) MemberErrorDetails(name, new EnjsonError(memberError));
    return err;
}

EnjsonError EnjsonError::buildMemberError(const QString &name, EnjsonError &&memberError)
{
    EnjsonError err;
    err.m_type = MemberError;
    new (&err.m_memberError) MemberErrorDetails(name, new EnjsonError(std::move(memberError)));
    return err;
}

EnjsonError EnjsonError::buildElementError(int element, const EnjsonError &elementError)
{
    EnjsonError err;
    err.m_type = ElementError;
    new (&err.m_elementError) ElementErrorDetails(element, new EnjsonError(elementError));
    return err;
}

EnjsonError EnjsonError::buildElementError(int element, EnjsonError &&elementError)
{
    EnjsonError err;
    err.m_type = ElementError;
    new (&err.m_elementError) ElementErrorDetails(element, new EnjsonError(std::move(elementError)));
    return err;
}

void dejson(const QJsonValue &json, Result<quint64, EnjsonError> &result)
{
    DEJSON_EXPECT_TYPE(json, result, Double);
    result = (quint64) json.toDouble();
}

void dejson(const QJsonValue &json, Result<qint64, EnjsonError> &result)
{
    DEJSON_EXPECT_TYPE(json, result, Double);
    result = (qint64) json.toDouble();
}

void dejson(const QJsonValue &json, Result<quint32, EnjsonError> &result)
{
    DEJSON_EXPECT_TYPE(json, result, Double);
    result = (quint32) json.toDouble();
}

void dejson(const QJsonValue &json, Result<qint32, EnjsonError> &result)
{
    DEJSON_EXPECT_TYPE(json, result, Double);
    result = (qint32) json.toDouble();
}

void dejson(const QJsonValue &json, Result<float, EnjsonError> &result)
{
    DEJSON_EXPECT_TYPE(json, result, Double);
    result = (float) json.toDouble();
}

void dejson(const QJsonValue &json, Result<double, EnjsonError> &result)
{
    DEJSON_EXPECT_TYPE(json, result, Double);
    result = json.toDouble();
}

void dejson(const QJsonValue &json, Result<QString, EnjsonError> &result)
{
    DEJSON_EXPECT_TYPE(json, result, String);
    result = json.toString();
}

void dejson(const QJsonValue &json, Result<bool, EnjsonError> &result)
{
    DEJSON_EXPECT_TYPE(json, result, Bool);
    result = json.toBool();
}

void dejson(const QJsonValue &json, Result<QJsonObject, EnjsonError> &result)
{
    DEJSON_EXPECT_TYPE(json, result, Object);
    result = json.toObject();
}

void dejson(const QJsonValue &json, Result<QJsonArray, EnjsonError> &result)
{
    DEJSON_EXPECT_TYPE(json, result, Array);
    result = json.toArray();
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

QJsonValue enjson(const QJsonValue &value)
{
    return value;
}

QByteArray jsonSerializeObject(const QJsonObject &jsonObject, QJsonDocument::JsonFormat format)
{
    return QJsonDocument(jsonObject).toJson(format);
}

QByteArray jsonSerializeArray(const QJsonArray &jsonArray, QJsonDocument::JsonFormat format)
{
    return QJsonDocument(jsonArray).toJson(format);
}

Result<QJsonValue, EnjsonError> jsonDeserialize(const QByteArray &json)
{
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &error);

    if (error.error != QJsonParseError::NoError)
        return EnjsonError::buildParseError(error);

    if (doc.isEmpty() || doc.isNull())
        return EnjsonError::buildTypeError(QJsonValue::Object, QJsonValue::Null);

    if (doc.isArray()) {
        return QJsonValue(doc.array());
    }
    else {
        Q_ASSERT(doc.isObject());
        return QJsonValue(doc.object());
    }
}

Result<QJsonObject, EnjsonError> jsonDeserializeObject(const QByteArray &json)
{
    Result<QJsonValue, EnjsonError> value = jsonDeserialize(json);
    if (value.hasError())
        return value.takeError();
    if (value.getValue().isArray())
        return EnjsonError::buildTypeError(QJsonValue::Object, QJsonValue::Array);
    return value.takeValue().toObject();
}

Result<QJsonArray, EnjsonError> jsonDeserializeArray(const QByteArray &json)
{
    Result<QJsonValue, EnjsonError> value = jsonDeserialize(json);
    if (value.hasError())
        return value.takeError();
    if (value.getValue().isObject())
        return EnjsonError::buildTypeError(QJsonValue::Array, QJsonValue::Object);
    return value.takeValue().toArray();
}
