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
class EnjsonError
{
public:
    EnjsonError();

    EnjsonError(const EnjsonError &other);
    EnjsonError(EnjsonError &&other);

    EnjsonError& operator=(const EnjsonError &rhs);
    EnjsonError& operator=(EnjsonError &&rhs);

    ~EnjsonError();

    bool isOk() const { return m_type == NoError; }
    bool isError() const { return m_type != NoError; }

    static EnjsonError buildParseError(const QJsonParseError &parseError);
    static EnjsonError buildTypeError(QJsonValue::Type expected, QJsonValue::Type found);
    static EnjsonError buildCustomError(const QString &message);
    static EnjsonError buildCustomError(const QString &message, const EnjsonError &jsonError);
    static EnjsonError buildCustomError(const QString &message, EnjsonError &&jsonError);
    static EnjsonError buildMissingMemberError(const QString &name);
    static EnjsonError buildMemberError(const QString &name, const EnjsonError &memberError);
    static EnjsonError buildMemberError(const QString &name, EnjsonError &&memberError);
    static EnjsonError buildElementError(int element, const EnjsonError &elementError);
    static EnjsonError buildElementError(int element, EnjsonError &&elementError);

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
        CustomErrorDetails(const QString &message, EnjsonError* err) : message(message), error(err) {}
        QString message;
        QScopedPointer<EnjsonError> error;
    };

    struct MissingMemberErrorDetails
    {
        QString name;
    };

    struct MemberErrorDetails
    {
        MemberErrorDetails(const QString &name, EnjsonError* err) : name(name), error(err) {}
        QString name;
        QScopedPointer<EnjsonError> error;
    };

    struct ElementErrorDetails
    {
        ElementErrorDetails(int elem, EnjsonError* err) : element(elem), error(err) {}
        int element;
        QScopedPointer<EnjsonError> error;
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

QJsonValue enjson(qint32 value);
QJsonValue enjson(quint32 value);
QJsonValue enjson(qint64 value);
QJsonValue enjson(quint64 value);
QJsonValue enjson(float value);
QJsonValue enjson(double value);
QJsonValue enjson(const QString &value);
QJsonValue enjson(bool value);
QJsonValue enjson(const QJsonValue &value);
template <class EnjsonObject> QJsonValue enjson(const EnjsonObject &enjsonObject, typename EnjsonObject::enjson_class_traits _dummy = {});
template <class T> QJsonValue enjson(const QVector<T> &values);
template <class A, class B> QJsonValue enjson(const QPair<A, B> &pair);

void dejson(const QJsonValue &json, Result<quint64, EnjsonError> &result);
void dejson(const QJsonValue &json, Result<qint64, EnjsonError> &result);
void dejson(const QJsonValue &json, Result<quint32, EnjsonError> &result);
void dejson(const QJsonValue &json, Result<qint32, EnjsonError> &result);
void dejson(const QJsonValue &json, Result<float, EnjsonError> &result);
void dejson(const QJsonValue &json, Result<double, EnjsonError> &result);
void dejson(const QJsonValue &json, Result<QString, EnjsonError> &result);
void dejson(const QJsonValue &json, Result<bool, EnjsonError> &result);
void dejson(const QJsonValue &json, Result<QJsonArray, EnjsonError> &result);
void dejson(const QJsonValue &json, Result<QJsonObject, EnjsonError> &result);
template <class T> void dejson(const QJsonValue &json, Result<QVector<T>, EnjsonError> &result);
template <class A, class B> void dejson(const QJsonValue &json, Result<QPair<A, B>, EnjsonError> &result);
template <class EnjsonObject> void dejson(const QJsonValue &json, Result<EnjsonObject, EnjsonError> &result, typename EnjsonObject::enjson_class_traits _dummy = {});

template <class T>
Result<T, EnjsonError> dejson(const QJsonValue &json)
{
    Result<T, EnjsonError> ret;
    dejson(json, ret);
    return ret;
}

QByteArray jsonSerializeObject(const QJsonObject &jsonObject, QJsonDocument::JsonFormat format = QJsonDocument::Indented);
QByteArray jsonSerializeArray(const QJsonArray &jsonArray, QJsonDocument::JsonFormat format = QJsonDocument::Indented);
Result<QJsonValue, EnjsonError> jsonDeserialize(const QByteArray &json);
Result<QJsonObject, EnjsonError> jsonDeserializeObject(const QByteArray &json);
Result<QJsonArray, EnjsonError> jsonDeserializeArray(const QByteArray &json);


template <class T>
QByteArray enjsonToString(const T &value, QJsonDocument::JsonFormat format = QJsonDocument::Indented)
{
    const QJsonValue json = enjson(value);
    Q_ASSERT(json.isObject());
    return jsonSerializeObject(json.toObject(), format);
}

template <class T>
Result<T, EnjsonError> dejsonFromString(const QByteArray &jsonString)
{
    Result<QJsonValue, EnjsonError> json = jsonDeserialize(jsonString);
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

#define ENJSON_DECLARE_ALIAS(TYPE, PROXYTYPE) \
inline QJsonValue enjson(TYPE value) \
{ \
    return ::enjson((PROXYTYPE) value); \
} \
inline void dejson(const QJsonValue &json, Result<TYPE, EnjsonError> &result) { \
    Result<PROXYTYPE, EnjsonError> ret; \
    dejson(json, ret); \
    if (ret.hasError()) \
        result = ret.takeError(); \
    else \
        result = (TYPE) ret.getValue(); \
} \

#define DEJSON_EXPECT_TYPE(JSON, RESULT, TYPE) \
    do { if (JSON.type() != QJsonValue::TYPE) { \
        RESULT = EnjsonError::buildTypeError(QJsonValue::TYPE, JSON.type()); \
        return; \
    } } while (0)

#define DEJSON_GET_MEMBER(JSON, RESULT, CPPTYPE, IDENTIFIER, NAME) \
    const QJsonValue IDENTIFIER ## _jsonvalue = JSON[NAME]; \
    if (IDENTIFIER ## _jsonvalue.type() == QJsonValue::Undefined) { \
        RESULT = EnjsonError::buildMissingMemberError(NAME); \
        return; \
    } \
    Result<CPPTYPE, EnjsonError> IDENTIFIER ## _result; \
    dejson(IDENTIFIER ## _jsonvalue, IDENTIFIER ## _result); \
    if (IDENTIFIER ## _result.hasError()) { \
        RESULT = IDENTIFIER ## _result.takeError(); \
        return; \
    } \
    CPPTYPE IDENTIFIER = IDENTIFIER ## _result.takeValue();

template<class T>
void dejson(const QJsonValue &json, Result<QVector<T>, EnjsonError> &result)
{
    DEJSON_EXPECT_TYPE(json, result, Array);
    const QJsonArray array = json.toArray();

    QVector<T> ret;
    T tVal;
    for (int i = 0; i < array.size(); ++i) {
        const QJsonValue value = array[i];
        Result<T, EnjsonError> element;
        dejson(value, element);
        if (element.hasError()) {
            result = std::move(EnjsonError::buildElementError(i, std::move(element.takeError())));
            return;
        }
        ret.append(std::move(element.takeValue()));
    }

    result = std::move(ret);
}

template<class A, class B>
void dejson(const QJsonValue &json, Result<QPair<A, B>, EnjsonError> &result)
{
    DEJSON_EXPECT_TYPE(json, result, Object);
    const QJsonObject object = json.toObject();

    const auto aIt = object.find("first");
    const auto bIt = object.find("second");

    if (aIt == object.end()) {
        result = std::move(EnjsonError::buildMissingMemberError("first"));
        return;
    }
    if (bIt == object.end()) {
        result = std::move(EnjsonError::buildMissingMemberError("second"));
        return;
    }

    Result<A, EnjsonError> a;
    Result<B, EnjsonError> b;
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


namespace enjson_detail {

using MemberGetter = QJsonValue (*)(const void*);
using MemberSetter = EnjsonError (*)(void*, const QJsonValue&);
using MemberResetter = void (*)(void*);

struct MemberDescriptor
{
    MemberGetter m_getter;
    MemberSetter m_setter;
    MemberResetter m_resetter;
    const char* m_name;
};

template <class Class>
struct ClassDescriptor
{
    QVector<MemberDescriptor> m_members;
};

template <class Class>
ClassDescriptor<Class> &getStaticClassDescriptor()
{
    static ClassDescriptor<Class> s_classDescriptor;
    return s_classDescriptor;
}

template <class OwnerTraits, class MemberTraits>
struct MemberDescriptorInitializer
{
    MemberDescriptorInitializer()
    {
        MemberDescriptor memberDescriptor;
        memberDescriptor.m_getter = &MemberTraits::get;
        memberDescriptor.m_setter = &MemberTraits::set;
        memberDescriptor.m_resetter = &MemberTraits::reset;
        memberDescriptor.m_name = MemberTraits::name();

        using Owner = typename OwnerTraits::Type;
        ClassDescriptor<Owner> &descriptor = getStaticClassDescriptor<Owner>();
        assert(std::none_of(descriptor.m_members.cbegin(),
                            descriptor.m_members.cend(),
                            [&](const MemberDescriptor &desc) { return desc.m_name == memberDescriptor.m_name; }));
        descriptor.m_members << memberDescriptor;
    }
};

template <class OwnerTraits, class Traits>
class Member
{
private:
    using ValueType = typename Traits::ValueType;
    using OwnerType = typename OwnerTraits::Type;

    ValueType m_value;

    QJsonValue enjson() const { return ::enjson(m_value); }
    EnjsonError dejson(const QJsonValue &json)
    {
        static_assert(offsetof(Member, m_value) == 0, "Can't have non-negative member offset");
        Result<ValueType, EnjsonError> ret;
        dejson(json, ret);
        return ret.hasError() ? ret.takeError() : EnjsonError();
    }

    static void init()
    {
        static MemberDescriptorInitializer<OwnerTraits, Traits> init = {};
        Q_UNUSED(init)
    }

public:
    Member() { init(); }
    Member(const Member &other) : m_value(other.m_value) { init(); }
    Member(Member &&other) : m_value(qMove(other.m_value)) { init(); }
    Member &operator=(const Member &other) { m_value = other.m_value; return *this; }
    Member &operator=(Member &&other) { m_value = std::move(other.m_value); return *this; }
    Member &operator=(const ValueType &value) { m_value = value; return *this; }
    Member &operator=(ValueType &&value) { m_value = std::move(value); return *this; }
    operator const ValueType&() const { return m_value; }
    operator ValueType&() { return m_value; }
    ValueType& operator*() { return m_value; }
    const ValueType& operator*() const { return m_value; }
    ValueType* operator->() { return &m_value; }
    const ValueType* operator->() const { return &m_value; }
};

} // namespace detail

#define ENJSON_OBJECT(ClassName) \
    struct enjson_class_traits \
    {\
        using Type = ClassName; \
        static const char *name() { return #ClassName; } \
    }; \

#define ENJSON_MEMBER(MemberType, MemberName) \
    struct enjson_ ## MemberName ## _member_traits \
    {\
        using ValueType = MemberType; \
        using OwnerType = typename enjson_class_traits::Type; \
        \
        template <class Owner, class Traits> friend class Member; \
        \
        static QJsonValue get(const void *ownerPtr) \
        { \
            auto memberPtr = &OwnerType::MemberName; \
            return ::enjson(*((*((const OwnerType*) ownerPtr)).*memberPtr)); \
        } \
        \
        static EnjsonError set(void *ownerPtr, const QJsonValue &json) \
        { \
            auto memberPtr = &OwnerType::MemberName; \
            EnjsonError ret; \
            Result<ValueType, EnjsonError> result; \
            ::dejson(json, result); \
            if (result.hasValue()) \
                ((OwnerType*) ownerPtr)->*memberPtr = result.takeValue(); \
            else \
                ret = result.takeError(); \
            return ret; \
        } \
        \
        static void reset(void *ownerPtr) \
        { \
            auto memberPtr = &OwnerType::MemberName; \
            ((OwnerType*) ownerPtr)->*memberPtr = ValueType(); \
        } \
        \
        static const char *name() { return #MemberName; } \
    }; \
    \
    ::enjson_detail::Member<enjson_class_traits, enjson_ ## MemberName ## _member_traits> MemberName;


template <class EnjsonObject>
void dejson(const QJsonValue &json, Result<EnjsonObject, EnjsonError> &result, typename EnjsonObject::enjson_class_traits _dummy)
{
    Q_UNUSED(_dummy)

    if (!json.isObject()) {
        result = EnjsonError::buildTypeError(QJsonValue::Object, json.type());
        return;
    }

    const QJsonObject obj = json.toObject();
    EnjsonObject ret;

    const ::enjson_detail::ClassDescriptor<EnjsonObject> &descriptor = ::enjson_detail::getStaticClassDescriptor<EnjsonObject>();
    for (const ::enjson_detail::MemberDescriptor &member : descriptor.m_members) {
        const auto val = obj.find(member.m_name);
        if (val == obj.end()) {
            result = EnjsonError::buildMissingMemberError(member.m_name);
            return;
        }

        const EnjsonError error = member.m_setter((void*) &ret, val.value());
        if (error.isError()) {
            result = qMove(error);
            return;
        }
    }

    result = qMove(ret);
}

template <class EnjsonObject>
QJsonValue enjson(const EnjsonObject &obj, typename EnjsonObject::enjson_class_traits _dummy)
{
    Q_UNUSED(_dummy)
    QJsonObject ret;
    const ::enjson_detail::ClassDescriptor<EnjsonObject> &descriptor = ::enjson_detail::getStaticClassDescriptor<EnjsonObject>();
    for (const ::enjson_detail::MemberDescriptor &member : descriptor.m_members) {
        ret.insert(member.m_name, member.m_getter((const void*) &obj));
    }
    return ret;
}
