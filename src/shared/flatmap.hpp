#pragma once

#include <QVector>

template <class Key, class Value>
class FlatMap
{
private:
    using Entry = QPair<Key, Value>;
    friend class iterator;
    friend class const_iterator;

public:
    using KeyType = Key;
    using ValueType = Value;

    class iterator {
    private:
        friend class const_iterator;
        friend class FlatMap;
        Entry *entry;
    public:
        typedef std::bidirectional_iterator_tag iterator_category;
        typedef qptrdiff difference_type;
        typedef Value value_type;
        typedef Value *pointer;
        typedef Value &reference;

        inline iterator() : entry(nullptr) {}
        explicit inline iterator(Entry *node) : entry(node) {}

        inline Key &key() { return entry->first; }
        inline Value &value() { return entry->second; }
        inline Value &operator*() const { return entry->second; }
        inline Value *operator->() const { return &entry->second; }
        inline bool operator==(const iterator &o) const { return entry == o.entry; }
        inline bool operator!=(const iterator &o) const { return entry != o.entry; }

        inline iterator &operator++() { ++entry; return *this; }
        inline iterator operator++(int) { iterator r = *this; ++entry; return r; }
        inline iterator &operator--() { --entry; return *this; }
        inline iterator operator--(int) { iterator r = *this; --entry; return r; }

        inline iterator operator+(int j) const { return iterator(entry + j); }
        inline iterator operator-(int j) const { return iterator(entry - j); }
        inline iterator &operator+=(int j) { entry += j; return *this; }
        inline iterator &operator-=(int j) { entry -= j; return *this; }
        friend inline iterator operator+(int j, iterator k) { return k + j; }
    };

    class const_iterator {
    private:
        friend class iterator;
        friend class FlatMap;
        const Entry *entry;
    public:
        typedef std::bidirectional_iterator_tag iterator_category;
        typedef qptrdiff difference_type;
        typedef Value value_type;
        typedef const Value *pointer;
        typedef const Value &reference;

        inline const_iterator() : entry(nullptr) {}
        explicit inline const_iterator(const Entry *node) : entry(node) {}

        inline const Key &key() { return entry->first; }
        inline const Value &value() { return entry->second; }
        inline const Value &operator*() const { return entry->second; }
        inline const Value *operator->() const { return &entry->second; }
        inline bool operator==(const const_iterator &o) const { return entry == o.entry; }
        inline bool operator!=(const const_iterator &o) const { return entry != o.entry; }

        inline const_iterator &operator++() { ++entry; return *this; }
        inline const_iterator operator++(int) { const_iterator r = *this; ++entry; return r; }
        inline const_iterator &operator--() { --entry; return *this; }
        inline const_iterator operator--(int) { const_iterator r = *this; --entry; return r; }

        inline const_iterator operator+(int j) const { return const_iterator(entry + j); }
        inline const_iterator operator-(int j) const { return const_iterator(entry - j); }
        inline const_iterator &operator+=(int j) { entry += j; return *this; }
        inline const_iterator &operator-=(int j) { entry -= j; return *this; }
        friend inline const_iterator operator+(int j, const_iterator k) { return k + j; }
    };

    // STL compatibility
    typedef Value mapped_type;
    typedef Key key_type;
    typedef qptrdiff difference_type;
    typedef int size_type;

    inline FlatMap() {}
    inline FlatMap(const FlatMap<Key,Value> &other) : m_data(other.m_data) {}
    inline FlatMap(FlatMap<Key,Value> &&other) : m_data(qMove(other.m_data)) {}
    inline ~FlatMap() {}

    inline FlatMap<Key,Value> &operator=(const FlatMap<Key,Value> &other) { m_data = other.m_data; return *this; }
    inline FlatMap<Key,Value> &operator=(FlatMap<Key,Value> &&other) { m_data = qMove(other.m_data); return *this; }

    inline int size() const { return m_data.size(); }
    inline bool isEmpty() const { return m_data.isEmpty(); }

    inline void clear() { m_data.clear(); }
    int remove(const Key &key);
    Value take(const Key &key);

    inline iterator begin() { return iterator(m_data.begin()); }
    inline iterator end() { return iterator(m_data.end()); }
    inline const_iterator begin() const { return const_iterator(m_data.cbegin()); }
    inline const_iterator end() const { return const_iterator(m_data.cend()); }
    inline const_iterator cbegin() const { return const_iterator(m_data.cbegin()); }
    inline const_iterator cend() const { return const_iterator(m_data.cend()); }

    const Value value(const Key &key) const;
    const Value value(const Key &key, const Value &defaultValue) const;
    Value &operator[](const Key &key);
    const Value operator[](const Key &key) const { return value(key); }

private:
    inline typename QVector<Entry>::const_iterator cfind(const Key &key) const {
        typename QVector<Entry>::const_iterator ret = m_data.cbegin();
        while (ret != m_data.cend()) {
            if (ret->first == key)
                break;
            ++ret;
        }
        return ret;
    }

    inline typename QVector<Entry>::iterator find(const Key &key) {
        typename QVector<Entry>::iterator ret = m_data.begin();
        while (ret != m_data.end()) {
            if (ret->first == key)
                break;
            ++ret;
        }
        return ret;
    }

    QVector<Entry> m_data;
};

template<class Key, class Value>
int FlatMap<Key, Value>::remove(const Key &key)
{
    if (!isEmpty()) {
        auto it = find(key);
        if (it != m_data.end()) {
            m_data.erase(it);
            return 1;
        }
    }
    return 0;
}

template<class Key, class Value>
Value FlatMap<Key, Value>::take(const Key &key)
{
    auto it = find(key);
    Value ret{};
    if (it != m_data.cend()) {
        ret = it->second;
        m_data.erase(it);
    }
    return ret;
}

template<class Key, class Value>
const Value FlatMap<Key, Value>::value(const Key &key) const
{
    const auto it = cfind(key);
    return (it != m_data.cend()) ? it->second : Value();
}

template<class Key, class Value>
const Value FlatMap<Key, Value>::value(const Key &key, const Value &defaultValue) const
{
    const auto it = cfind(key);
    return (it != m_data.cend()) ? it->second : defaultValue;
}

template<class Key, class Value>
Value &FlatMap<Key, Value>::operator[](const Key &key)
{
    auto it = find(key);
    if (it == m_data.end())
        it = m_data.insert(it, {key, Value()});
    return it->second;
}
