#pragma once

#include <utility>
#include <cassert>

template <class ValueType, class ErrorType>
class Result
{
public:
    Result();

    Result(const ValueType &value);
    Result(ValueType &&value);

    Result(const ErrorType &error);
    Result(ErrorType &&error);

    Result(const Result &other);
    Result(Result &&other);

    static Result value(const ValueType &value);
    static Result value(ValueType &&value);

    static Result error(const ErrorType &error);
    static Result error(ErrorType &&error);

    ~Result();

    Result& operator=(const Result &rhs);
    Result& operator=(Result &&rhs);

    void setValue(const ValueType &value);
    void setValue(ValueType &&value);

    Result& operator=(const ValueType &value) { setValue(value); return *this; }
    Result& operator=(ValueType &&value) { setValue(std::move(value)); return *this; }

    void setError(const ErrorType &error);
    void setError(ErrorType &&error);

    Result& operator=(const ErrorType &error) { setError(error); return *this; }
    Result& operator=(ErrorType &&error) { setError(std::move(error)); return *this; }

    bool isNull() const { return m_state == Null; }
    bool hasValue() const { return m_state == Value; }
    bool hasError() const { return m_state == Error; }

    const ValueType &getValue() const;
    ValueType takeValue();

    const ValueType &operator*() const { return getValue(); }
    const ValueType *operator->() const { return &getValue(); }

    const ErrorType &getError() const;
    ErrorType takeError();

private:
    void clear();

    enum State : char
    {
        Null,
        Value,
        Error
    };

    union {
        ValueType m_value;
        ErrorType m_error;
    };

    State m_state;
};

template <class ValueType, class ErrorType>
Result<ValueType, ErrorType>::Result()
    : m_state(Null)
{
}

template <class ValueType, class ErrorType>
Result<ValueType, ErrorType>::Result(const ValueType &value)
    : m_state(Value)
{
    new (&m_value)ValueType(value);
}

template <class ValueType, class ErrorType>
Result<ValueType, ErrorType>::Result(ValueType &&value)
    : m_state(Value)
{
    new (&m_value)ValueType(std::move(value));
}

template <class ValueType, class ErrorType>
Result<ValueType, ErrorType>::Result(const ErrorType &error)
    : m_state(Error)
{
    new (&m_error)ErrorType(error);
}

template <class ValueType, class ErrorType>
Result<ValueType, ErrorType>::Result(ErrorType &&error)
    : m_state(Error)
{
    new (&m_error)ErrorType(std::move(error));
}

template <class ValueType, class ErrorType>
Result<ValueType, ErrorType>::Result(const Result &other)
    : m_state(Null)
{
    *this = other;
}

template <class ValueType, class ErrorType>
Result<ValueType, ErrorType>::Result(Result &&other)
    : m_state(Null)
{
    *this = std::move(other);
}

template <class ValueType, class ErrorType>
Result<ValueType, ErrorType> Result<ValueType, ErrorType>::value(const ValueType &value)
{
    Result<ValueType, ErrorType> ret;
    ret.setValue(value);
    return ret;
}

template <class ValueType, class ErrorType>
Result<ValueType, ErrorType> Result<ValueType, ErrorType>::value(ValueType &&value)
{
    Result<ValueType, ErrorType> ret;
    ret.setValue(std::move(value));
    return ret;
}

template <class ValueType, class ErrorType>
Result<ValueType, ErrorType> Result<ValueType, ErrorType>::error(const ErrorType &error)
{
    Result<ValueType, ErrorType> ret;
    ret.setError(error);
    return ret;
}

template <class ValueType, class ErrorType>
Result<ValueType, ErrorType> Result<ValueType, ErrorType>::error(ErrorType &&error)
{
    Result<ValueType, ErrorType> ret;
    ret.setError(std::move(error));
    return ret;
}

template <class ValueType, class ErrorType>
Result<ValueType, ErrorType>::~Result()
{
    clear();
}

template <class ValueType, class ErrorType>
void Result<ValueType, ErrorType>::clear()
{
    switch (m_state) {
    case Null:
        break;
    case Value:
        m_value.~ValueType();
        break;
    case Error:
        m_error.~ErrorType();
        break;
    default:
        assert(false);
    }
    m_state = Null;
}

template <class ValueType, class ErrorType>
Result<ValueType, ErrorType> &Result<ValueType, ErrorType>::operator=(const Result &rhs)
{
    if (m_state == rhs.m_state) {
        switch (m_state) {
        case Null:
            break;
        case Value:
            m_value = rhs.m_value;
            break;
        case Error:
            m_error = rhs.error;
            break;
        default:
            assert(false);
        }
    }
    else {
        clear();
        m_state = rhs.m_state;

        switch (m_state) {
        case Null:
            break;
        case Value:
            new (&m_value)ValueType(rhs.m_value);
            break;
        case Error:
            new (&m_error)ErrorType(rhs.error);
            break;
        default:
            assert(false);
        }
    }

    return *this;
}

template <class ValueType, class ErrorType>
Result<ValueType, ErrorType> &Result<ValueType, ErrorType>::operator=(Result &&rhs)
{
    if (m_state == rhs.m_state) {
        switch (m_state) {
        case Null:
            break;
        case Value:
            m_value = std::move(rhs.m_value);
            break;
        case Error:
            m_error = std::move(rhs.m_error);
            break;
        default:
            assert(false);
        }
    }
    else {
        clear();
        m_state = rhs.m_state;

        switch (m_state) {
        case Null:
            break;
        case Value:
            new (&m_value)ValueType(std::move(rhs.m_value));
            break;
        case Error:
            new (&m_error)ErrorType(std::move(rhs.m_error));
            break;
        default:
            assert(false);
        }
    }

    return *this;
}

template <class ValueType, class ErrorType>
void Result<ValueType, ErrorType>::setValue(const ValueType &value)
{
    switch (m_state) {
    case Null:
        new (&m_value) ValueType(value);
        break;
    case Value:
        m_value = value;
        break;
    case Error:
        m_error.~ErrorType();
        new (&m_value) ValueType(value);
        break;
    default:
        assert(false);
    }
    m_state = Value;
}

template <class ValueType, class ErrorType>
void Result<ValueType, ErrorType>::setValue(ValueType &&value)
{
    switch (m_state) {
    case Null:
        new (&m_value) ValueType(std::move(value));
        break;
    case Value:
        m_value = std::move(value);
        break;
    case Error:
        m_error.~ErrorType();
        new (&m_value) ValueType(std::move(value));
        break;
    default:
        assert(false);
    }
    m_state = Value;
}

template <class ValueType, class ErrorType>
void Result<ValueType, ErrorType>::setError(const ErrorType &error)
{
    switch (m_state) {
    case Null:
        new (&m_error) ErrorType(error);
        break;
    case Value:
        m_value.~ValueType();
        new (&m_error) ErrorType(error);
        break;
    case Error:
        m_error = error;
        break;
    default:
        assert(false);
    }
    m_state = Error;

}

template <class ValueType, class ErrorType>
void Result<ValueType, ErrorType>::setError(ErrorType &&error)
{
    switch (m_state) {
    case Null:
        new (&m_error) ErrorType(std::move(error));
        break;
    case Value:
        m_value.~ValueType();
        new (&m_error) ErrorType(std::move(error));
        break;
    case Error:
        m_error = std::move(error);
        break;
    default:
        assert(false);
    }
    m_state = Error;

}

template <class ValueType, class ErrorType>
const ValueType &Result<ValueType, ErrorType>::getValue() const
{
    assert(m_state == Value);
    return m_value;
}

template <class ValueType, class ErrorType>
ValueType Result<ValueType, ErrorType>::takeValue()
{
    assert(m_state == Value);
    ValueType ret = std::move(m_value);
    m_value.~ValueType();
    m_state = Null;
    return ret;
}

template <class ValueType, class ErrorType>
const ErrorType &Result<ValueType, ErrorType>::getError() const
{
    assert(m_state == Error);
    return m_error;
}

template <class ValueType, class ErrorType>
ErrorType Result<ValueType, ErrorType>::takeError()
{
    assert(m_state == Error);
    ErrorType ret = std::move(m_error);
    m_error.~ErrorType();
    m_state = Null;
    return ret;
}
