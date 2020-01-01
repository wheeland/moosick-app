#pragma once

#include <QAbstractListModel>
#include <QScopedPointer>
#include <functional>

namespace ModelAdapter {

class AdapterBase;

class Model : public QAbstractListModel
{
    Q_OBJECT

public:
    Model(AdapterBase *adapter, QObject* parent = nullptr);
    ~Model() override = default;

    int rowCount(const QModelIndex &parent) const override;
    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &modelIndex, int role) const override;

    int roleIndex(const QString &name) const;

private:
    AdapterBase *m_adapter;
    friend class AdapterBase;
};

// base class used by QALM
class AdapterBase
{
public:
    Model *model() const;

    virtual int size() const = 0;
    void dirty(int idx, const QString &role = QString());

protected:
    AdapterBase();
    ~AdapterBase();

    void beginAdd(int start, int end);
    void endAdd();

    void beginRemove(int start, int end);
    void endRemove();

    QHash<int, QByteArray> roleNames();

    virtual QVariant getData(int index, int role) const = 0;

    bool addRole(const QString &roleName);

private:
    const QScopedPointer<Model> m_model;
    QVector<QString> m_roleNames;

    friend Model;
};

template <class T>
class Adapter : public AdapterBase
{
public:
    Adapter() {}
    ~Adapter() { clear(); }

    typedef std::function<QVariant(const T &)> AccessorFunction;

    void addAccessor(const QString &roleName, const AccessorFunction &accessor)
    {
        if (!addRole(roleName))
            qFatal("[ModelAdapter] role already exists");
        m_accessors << accessor;
    }

    void addValueAccessor(const QString &roleName)
    {
        if (!addRole(roleName))
            qFatal("[ModelAdapter] role already exists");
        m_accessors << [](const T &val) { return QVariant::fromValue(val); };
    }

    void set(int idx, const T &val)
    {
        Q_ASSERT(idx >= 0 && idx < m_data.size());
        m_data[idx] = val;
        dirty(idx);
    }

    void add(const T &val)
    {
        beginAdd(size(), size());
        m_data << val;
        endAdd();
    }

    void insert(int index, const T& val)
    {
        if (index < 0)
            index = 0;
        if (index >= m_data.size())
            add(val);
        beginAdd(index, index);
        m_data.insert(index, val);
        endAdd();
    }

    void remove(const T &val)
    {
        const int idx = m_data.indexOf(val);
        if (idx >= 0 && idx < m_data.size())
            remove(idx);
    }

    void remove(int idx) {
        Q_ASSERT(idx >= 0 && idx < m_data.size());
        beginRemove(idx, idx);
        m_data.remove(idx);
        endRemove();
    }

    void clear()
    {
        if (m_data.size() > 0) {
            beginRemove(0, m_data.size() - 1);
            m_data.clear();
            endRemove();
        }
    }

    virtual int size() const { return m_data.size(); }
    const QVector<T>& data() const { return m_data; }
    const T& operator[](int idx) const { return m_data[idx]; }
    T& operator[](int idx) { return m_data[idx]; }

protected:
    virtual QVariant getData(int index, int role) const
    {
        if (role < 0 || role >= m_accessors.size())
            return QVariant();
        if (index < 0 || index >= m_data.size())
            return QVariant();
        return m_accessors[role](m_data[index]);
    }

private:
    QVector<T> m_data;
    QVector<AccessorFunction> m_accessors;
};

} // namespace ModelAdapter
