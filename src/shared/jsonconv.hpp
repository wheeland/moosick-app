#pragma once

#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QScopedPointer>
#include <QVector>
#include <QString>

#include "result.hpp"

/**
 * Holds details about what went wrong when deserializing a JSON value
 */
class JsonifyError
{
public:
    JsonifyError();

    JsonifyError(const JsonifyError &other);
    JsonifyError(JsonifyError &&other);

    JsonifyError& operator=(const JsonifyError &rhs);
    JsonifyError& operator=(JsonifyError &&rhs);

    ~JsonifyError();

    bool isOk() const { return m_type == NoError; }
    bool isError() const { return m_type != NoError; }

    static JsonifyError buildParseError(const QJsonParseError &parseError);
    static JsonifyError buildTypeError(QJsonValue::Type expected, QJsonValue::Type found);
    static JsonifyError buildCustomError(const QString &message);
    static JsonifyError buildCustomError(const QString &message, const JsonifyError &jsonError);
    static JsonifyError buildCustomError(const QString &message, JsonifyError &&jsonError);
    static JsonifyError buildMissingMemberError(const QString &name);
    static JsonifyError buildMemberError(const QString &name, const JsonifyError &memberError);
    static JsonifyError buildMemberError(const QString &name, JsonifyError &&memberError);
    static JsonifyError buildElementError(int element, const JsonifyError &elementError);
    static JsonifyError buildElementError(int element, JsonifyError &&elementError);

    QString toString() const;

private:
    void clear();

    enum Type
    {
        NoError,
        ParseError,
        TypeError,
        CustomError,
        MissingMemberError,
        MemberError,
        ElementError,
    };

    struct TypeErrorDetails
    {
        QJsonValue::Type expected;
        QJsonValue::Type found;
    };

    struct CustomErrorDetails
    {
        CustomErrorDetails(const QString &message, JsonifyError* err) : message(message), error(err) {}
        QString message;
        QScopedPointer<JsonifyError> error;
    };

    struct MissingMemberErrorDetails
    {
        QString name;
    };

    struct MemberErrorDetails
    {
        MemberErrorDetails(const QString &name, JsonifyError* err) : name(name), error(err) {}
        QString name;
        QScopedPointer<JsonifyError> error;
    };

    struct ElementErrorDetails
    {
        ElementErrorDetails(int elem, JsonifyError* err) : element(elem), error(err) {}
        int element;
        QScopedPointer<JsonifyError> error;
    };

    union
    {
        QJsonParseError m_parseError;
        TypeErrorDetails m_typeError;
        CustomErrorDetails m_customError;
        MissingMemberErrorDetails m_missingMemberError;
        MemberErrorDetails m_memberError;
        ElementErrorDetails m_elementError;
    };

    Type m_type;
};

/**
 * Base object for user-defined JSON aggregate types
 */
class JsonObject
{
public:
    QJsonValue serialize() const;
    JsonifyError deserialize(const QJsonValue &json);

    struct Dummy {};

protected:
    JsonObject() = default;
    ~JsonObject() = default;
    JsonObject(const JsonObject&) = default;
    JsonObject(JsonObject&&) = default;
    JsonObject &operator=(const JsonObject&) = default;
    JsonObject &operator=(JsonObject&&) = default;

    class JsonMemberBase
    {
    protected:
        virtual QJsonValue get() const = 0;
        virtual JsonifyError set(const QJsonValue &json) = 0;
        virtual void reset() = 0;
        virtual const char *name() const = 0;
        friend class JsonObject;
    };

    template <class T, class NameTrait>
    class JsonMember : public JsonMemberBase
    {
    public:
        JsonMember(JsonObject *jsonObject);
        JsonMember(const JsonMember &other) : m_value(other.m_value) {}
        JsonMember(JsonMember &&other) : m_value(std::move(other.m_value)) {}
        JsonMember &operator=(const JsonMember &other) { m_value = other.m_value; return *this; }
        JsonMember &operator=(JsonMember &&other) { m_value = std::move(other.m_value); return *this; }
        JsonMember &operator=(const T &value) { m_value = value; return *this; }
        JsonMember &operator=(T &&value) { m_value = std::move(value); return *this; }
        operator const T&() const { return m_value; }
        operator T&() { return m_value; }
        T& operator*() { return m_value; }
        const T& operator*() const { return m_value; }
        T* operator->() { return &m_value; }
        const T* operator->() const { return &m_value; }
        const T& data() const { return m_value; }
        T& data() { return m_value; }

    protected:
        QJsonValue get() const override;
        JsonifyError set(const QJsonValue &json) override;
        void reset() override;
        const char* name() const override;

    private:
        T m_value;
    };

private:
    inline JsonMemberBase *getMember(uintptr_t offset) const
    {
        uintptr_t memberPtr = (uintptr_t) this + offset;
        return (JsonMemberBase*) memberPtr;
    }

    // TODO: make an inplace-array out of this
    QVector<uintptr_t> m_memberOffsets;

    template <class T, class NameTrait>
    friend class JsonMember;
};

#define JSONIFY_MEMBER(TYPE, NAME) \
    struct NAME ## _Name { static constexpr const char* name = #NAME; }; \
    JsonMember<TYPE, NAME ## _Name> NAME = this;

QJsonValue enjson(qint32 value);
QJsonValue enjson(quint32 value);
QJsonValue enjson(qint64 value);
QJsonValue enjson(quint64 value);
QJsonValue enjson(float value);
QJsonValue enjson(double value);
QJsonValue enjson(const QString &value);
QJsonValue enjson(bool value);
QJsonValue enjson(const JsonObject &jsonify);
template <class T> QJsonValue enjson(const QVector<T> &values);
template <class A, class B> QJsonValue enjson(const QPair<A, B> &pair);

void dejson(const QJsonValue &json, Result<quint64, JsonifyError> &result);
void dejson(const QJsonValue &json, Result<qint64, JsonifyError> &result);
void dejson(const QJsonValue &json, Result<quint32, JsonifyError> &result);
void dejson(const QJsonValue &json, Result<qint32, JsonifyError> &result);
void dejson(const QJsonValue &json, Result<float, JsonifyError> &result);
void dejson(const QJsonValue &json, Result<double, JsonifyError> &result);
void dejson(const QJsonValue &json, Result<QString, JsonifyError> &result);
void dejson(const QJsonValue &json, Result<bool, JsonifyError> &result);
template <class T> void dejson(const QJsonValue &json, Result<QVector<T>, JsonifyError> &result);
template <class A, class B> void dejson(const QJsonValue &json, Result<QPair<A, B>, JsonifyError> &result);
template <class JsonObjectType> void dejson(const QJsonValue &json, Result<JsonObjectType, JsonifyError> &result, typename JsonObjectType::Dummy _dummy = typename JsonObjectType::Dummy());

template <class T>
Result<T, JsonifyError> dejson(const QJsonValue &json)
{
    Result<T, JsonifyError> ret;
    dejson(json, ret);
    return ret;
}

QByteArray jsonSerializeObject(const QJsonObject &jsonObject);
QByteArray jsonSerializeArray(const QJsonArray &jsonArray);
Result<QJsonValue, JsonifyError> jsonDeserialize(const QByteArray &json);
Result<QJsonObject, JsonifyError> jsonDeserializeObject(const QByteArray &json);
Result<QJsonArray, JsonifyError> jsonDeserializeArray(const QByteArray &json);


template <class T>
QByteArray enjsonToString(const T &value)
{
    const QJsonValue json = enjson(value);
    Q_ASSERT(json.isObject());
    return jsonSerializeObject(json.toObject());
}

template <class T>
Result<T, JsonifyError> dejsonFromString(const QByteArray &jsonString)
{
    Result<QJsonValue, JsonifyError> json = jsonDeserialize(jsonString);
    if (json.hasError())
        return json.takeError();

    QJsonValue jsonValue = json.takeValue();
    return dejson<T>(jsonValue);
}

template <class T> QJsonValue enjson(const QVector<T> &values)
{
    QJsonArray array;
    for (const T &value : values)
        array.append(enjson(value));
    return array;
}

template <class A, class B> QJsonValue enjson(const QPair<A, B> &pair)
{
    QJsonObject obj;
    obj.insert("first", enjson(pair.first));
    obj.insert("second", enjson(pair.second));
    return obj;
}

#define JSONIFY_DECLARE_PROXY_ENJSON(TYPE, PROXYTYPE) \
inline QJsonValue enjson(TYPE value) \
{ \
    return ::enjson((PROXYTYPE) value); \
} \
inline void dejson(const QJsonValue &json, Result<TYPE, JsonifyError> &result) { \
    Result<PROXYTYPE, JsonifyError> ret; \
    dejson(json, ret); \
    if (ret.hasError()) \
        result = ret.takeError(); \
    else \
        result = (TYPE) ret.getValue(); \
} \

#define JSONIFY_DEJSON_EXPECT_TYPE(JSON, RESULT, TYPE) \
    do { if (JSON.type() != QJsonValue::TYPE) { \
        RESULT = JsonifyError::buildTypeError(QJsonValue::TYPE, JSON.type()); \
        return; \
    } } while (0)

#define JSONIFY_DEJSON_GET_MEMBER(JSON, RESULT, CPPTYPE, IDENTIFIER, NAME) \
    const QJsonValue IDENTIFIER ## _jsonvalue = JSON[NAME]; \
    if (IDENTIFIER ## _jsonvalue.type() == QJsonValue::Undefined) { \
        RESULT = JsonifyError::buildMissingMemberError(NAME); \
        return; \
    } \
    Result<CPPTYPE, JsonifyError> IDENTIFIER ## _result; \
    dejson(IDENTIFIER ## _jsonvalue, IDENTIFIER ## _result); \
    if (IDENTIFIER ## _result.hasError()) { \
        RESULT = IDENTIFIER ## _result.takeError(); \
        return; \
    } \
    CPPTYPE IDENTIFIER = IDENTIFIER ## _result.takeValue();

template<class T>
void dejson(const QJsonValue &json, Result<QVector<T>, JsonifyError> &result)
{
    JSONIFY_DEJSON_EXPECT_TYPE(json, result, Array);
    const QJsonArray array = json.toArray();

    QVector<T> ret;
    T tVal;
    for (int i = 0; i < array.size(); ++i) {
        const QJsonValue value = array[i];
        Result<T, JsonifyError> element;
        dejson(value, element);
        if (element.hasError()) {
            result = std::move(JsonifyError::buildElementError(i, std::move(element.takeError())));
            return;
        }
        ret.append(std::move(element.takeValue()));
    }

    result = std::move(ret);
}

template<class A, class B>
void dejson(const QJsonValue &json, Result<QPair<A, B>, JsonifyError> &result)
{
    JSONIFY_DEJSON_EXPECT_TYPE(json, result, Object);
    const QJsonObject object = json.toObject();

    const auto aIt = object.find("first");
    const auto bIt = object.find("second");

    if (aIt == object.end()) {
        result = std::move(JsonifyError::buildMissingMemberError("first"));
        return;
    }
    if (bIt == object.end()) {
        result = std::move(JsonifyError::buildMissingMemberError("second"));
        return;
    }

    Result<A, JsonifyError> a;
    Result<B, JsonifyError> b;
    dejson(aIt.value(), a);
    dejson(bIt.value(), b);

    if (a.hasError()) {
        result = std::move(a.takeError());
        return;
    }

    if (b.hasError()) {
        result = std::move(b.takeError());
        return;
    }

    result = std::move(QPair<A, B>(a.takeValue(), b.takeValue()));
}

template <class JsonObjectType> void dejson(const QJsonValue &json, Result<JsonObjectType, JsonifyError> &result, typename JsonObjectType::Dummy _dummy)
{
    Q_UNUSED(_dummy)
    JsonObjectType jsonObject;
    JsonifyError ret = jsonObject.deserialize(json);
    if (ret.isError())
        result = std::move(ret);
    else
        result = std::move(jsonObject);
}

namespace detail {

template <class T>
struct MemberTraits
{
    static QJsonValue get(const T &value) {
        return enjson(value);
    }

    static JsonifyError set(const QJsonValue &json, T &value) {
        JsonifyError ret;
        Result<T, JsonifyError> result;
        dejson(json, result);
        if (result)
            value = result.takeValue();
        else
            ret = result.takeError();
        return ret;
    }
};

template <>
struct MemberTraits<QJsonValue>
{
    static QJsonValue get(const QJsonValue &value) { return value; }

    static JsonifyError set(const QJsonValue &json, QJsonValue &value) {
        value = json;
        return JsonifyError();
    }
};

template <>
struct MemberTraits<QJsonObject>
{
    static QJsonValue get(const QJsonObject &value) { return value; }

    static JsonifyError set(const QJsonValue &json, QJsonObject &value) {
        if (!json.isObject())
            return JsonifyError::buildTypeError(QJsonValue::Object, json.type());
        value = json.toObject();
        return JsonifyError();
    }
};

template <>
struct MemberTraits<QJsonArray>
{
    static QJsonValue get(const QJsonArray &value) { return value; }

    static JsonifyError set(const QJsonValue &json, QJsonArray &value) {
        if (!json.isArray())
            return JsonifyError::buildTypeError(QJsonValue::Array, json.type());
        value = json.toArray();
        return JsonifyError();
    }
};

} // namespace detail

template<class T, class NameTrait>
JsonObject::JsonMember<T, NameTrait>::JsonMember(JsonObject *jsonObject)
    : m_value()
{
    JsonMemberBase *base = this;
    const uintptr_t parentPtr = (uintptr_t) jsonObject;
    const uintptr_t basePtr = (uintptr_t) base;
    Q_ASSERT(basePtr >= parentPtr);
    if (!jsonObject->m_memberOffsets.contains(basePtr - parentPtr))
        jsonObject->m_memberOffsets << (basePtr - parentPtr);
}

template<class T, class NameTrait>
QJsonValue JsonObject::JsonMember<T, NameTrait>::get() const
{
    return detail::MemberTraits<T>::get(m_value);
}

template<class T, class NameTrait>
JsonifyError JsonObject::JsonMember<T, NameTrait>::set(const QJsonValue &json)
{
    return detail::MemberTraits<T>::set(json, m_value);
}

template<class T, class NameTrait>
void JsonObject::JsonMember<T, NameTrait>::reset()
{
    m_value = T();
}

template<class T, class NameTrait>
const char *JsonObject::JsonMember<T, NameTrait>::name() const
{
    return NameTrait::name;
}
