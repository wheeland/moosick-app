#pragma once

#include <QHash>

namespace Moosick {

template <class T, class IntType = quint32>
class ItemCollection
{
public:
    ItemCollection() {}
    ~ItemCollection() {}

    void add(IntType id, const T &value)
    {
        m_data.insert(id, value);
        m_nextId = qMax(m_nextId, id + 1);
    }

    void remove(IntType id)
    {
        m_data.remove(id);
    }

    T *find(IntType id)
    {
        const auto it = m_data.find(id);
        return (it != m_data.end()) ? (&it.value()) : nullptr;
    }

    const T *find(IntType id) const
    {
        const auto it = m_data.find(id);
        return (it != m_data.end()) ? (&it.value()) : nullptr;
    }

    QPair<IntType, T*> create()
    {
        const auto it = m_data.insert(m_nextId, T());
        m_nextId += 1;
        return qMakePair(m_nextId - 1, &it.value());
    }

private:
    QHash<IntType, T> m_data;
    IntType m_nextId = 1;
};

} // namespace Moosick
