#pragma once

#include <utility>
#include <cassert>

template <class T>
class Option
{
public:
    Option();

    Option(const T &value);
    Option(T &&value);

    Option(const Option &other);
    Option(Option &&other);

    ~Option();

    Option& operator=(const Option &rhs);
    Option& operator=(Option &&rhs);

    Option& operator=(const T &value);
    Option& operator=(T &&value);

    bool hasValue() const { return m_hasValue; }
    operator bool() const { return hasValue(); }

    const T &getValue() const;
    T takeValue();

    T* operator->();
    const T* operator->() const;

    void clear();

private:

    union {
        T m_value;
    };

    char m_hasValue : 1;
};

template <class T>
Option<T>::Option()
    : m_hasValue(0)
{
}

template <class T>
Option<T>::Option(const T &value)
    : m_hasValue(1)
{
    new (&m_value) T (value);
}

template <class T>
Option<T>::Option(T &&value)
    : m_hasValue(1)
{
    new (&m_value) T (std::move(value));
}

template <class T>
Option<T>::Option(const Option &other)
    : m_hasValue(other.m_hasValue)
{
    if (m_hasValue)
        new (&m_value) T (other.m_value);
}

template <class T>
Option<T>::Option(Option &&other)
    : m_hasValue(other.m_hasValue)
{
    if (m_hasValue) {
        new (&m_value) T (std::move(other.m_value));
        other.clear();
    }
}

template <class T>
Option<T>::~Option()
{
    clear();
}

template <class T>
void Option<T>::clear()
{
    if (m_hasValue) {
        m_value.~T();
        m_hasValue = 0;
    }
}

template <class T>
Option<T> &Option<T>::operator=(const Option &rhs)
{
    if (m_hasValue) {
        if (rhs.m_hasValue)
            m_value = rhs.m_value;
        else
            clear();
    }
    else if (rhs.m_hasValue) {
        new (&m_value) T (rhs.m_value);
        m_hasValue = true;
    }
    return *this;
}

template <class T>
Option<T> &Option<T>::operator=(Option &&rhs)
{
    if (m_hasValue) {
        if (rhs.m_hasValue) {
            m_value = std::move(rhs.m_value);
            rhs.clear();
        }
        else
            clear();
    }
    else if (rhs.m_hasValue) {
        new (&m_value) T (std::move(rhs.m_value));
        m_hasValue = true;
        rhs.clear();
    }
    return *this;
}

template <class T>
Option<T> &Option<T>::operator=(const T &value)
{
    if (m_hasValue) {
        m_value = value;
    }
    else {
        new (&m_value) T(value);
        m_hasValue = true;
    }
    return *this;
}

template <class T>
Option<T> &Option<T>::operator=(T &&value)
{
    if (m_hasValue) {
        m_value = std::move(value);
    }
    else {
        new (&m_value) T(std::move(value));
        m_hasValue = true;
    }
    return *this;
}

template <class T>
const T &Option<T>::getValue() const
{
    assert(m_hasValue);
    return m_value;
}

template <class T>
T Option<T>::takeValue()
{
    assert(m_hasValue);
    T value = std::move(m_value);
    m_value.~T();
    m_hasValue = false;
    return std::move(value);
}

template<class T>
T *Option<T>::operator->()
{
    assert(m_hasValue);
    return &m_value;
}

template<class T>
const T *Option<T>::operator->() const
{
    assert(m_hasValue);
    return &m_value;
}
