#pragma once

#include <QHash>

namespace Moosick {

template <class T, class IntType = quint32>
class ItemCollection : public QHash<IntType, T>
{
public:
    ItemCollection() {}
    ~ItemCollection() {}

    void add(IntType id, const T &value)
    {
        this->insert(id, value);
        m_nextId = qMax(m_nextId, id + 1);
    }

    T *findItem(IntType id)
    {
        const auto it = this->find(id);
        return (it != this->end()) ? (&it.value()) : nullptr;
    }

    const T *findItem(IntType id) const
    {
        const auto it = this->find(id);
        return (it != this->end()) ? (&it.value()) : nullptr;
    }

    QPair<IntType, T*> create()
    {
        const auto it = this->insert(m_nextId, T());
        m_nextId += 1;
        return qMakePair(m_nextId - 1, &it.value());
    }

private:
    IntType m_nextId = 1;
};

} // namespace Moosick
