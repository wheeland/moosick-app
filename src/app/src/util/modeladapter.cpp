#include "modeladapter.hpp"
#include <QDebug>

namespace ModelAdapter {

Model::Model(AdapterBase *adapter, QObject *parent)
    : QAbstractListModel(parent)
    , m_adapter(adapter)
{}

int Model::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_adapter->size();
}

QHash<int, QByteArray> Model::roleNames() const
{
    return m_adapter->roleNames();
}

QVariant Model::data(const QModelIndex &modelIndex, int role) const
{
    return m_adapter->getData(modelIndex.row(), role - Qt::UserRole);
}

int Model::roleIndex(const QString &name) const
{
    const int idx = m_adapter->m_roleNames.indexOf(name);
    return (idx < 0) ? idx : (idx + Qt::UserRole);
}

int Model::size() const
{
    return m_adapter->size();
}

AdapterBase::AdapterBase()
    : m_model(new Model(this))
{
}

AdapterBase::~AdapterBase()
{
}

void AdapterBase::beginAdd(int start, int end)
{
    m_model->beginInsertRows(QModelIndex(), start, end);
}

void AdapterBase::endAdd()
{
    m_model->endInsertRows();
    emit m_model->sizeChanged(size());
}

void AdapterBase::beginRemove(int start, int end)
{
    m_model->beginRemoveRows(QModelIndex(), start, end);
}

void AdapterBase::endRemove()
{
    m_model->endRemoveRows();
    emit m_model->sizeChanged(size());
}

void AdapterBase::beginMove(int from, int to)
{
    const bool success = m_model->beginMoveRows(QModelIndex(), from, from, QModelIndex(), to);
    Q_ASSERT(success);
}

void AdapterBase::endMove()
{
    m_model->endMoveRows();
}

QHash<int, QByteArray> AdapterBase::roleNames()
{
    QHash<int, QByteArray> ret;
    for (int i = 0; i < m_roleNames.size(); i++)
        ret.insert(Qt::UserRole + i, m_roleNames[i].toLocal8Bit());
    return ret;
}

bool AdapterBase::addRole(const QString &roleName)
{
    if (m_roleNames.contains(roleName))
        return false;
    m_roleNames << roleName;
    return true;
}

Model *AdapterBase::model() const
{
    return m_model.data();
}

void AdapterBase::dirty(int idx, const QString &role)
{
    Q_ASSERT(idx >= 0 && idx < size());
    const int roleIdx = m_roleNames.indexOf(role);

    if (!role.isEmpty()) {
        Q_ASSERT(roleIdx >= 0 && roleIdx < m_roleNames.size());
        Q_EMIT m_model->dataChanged(m_model->index(idx), m_model->index(idx), QVector<int>{ roleIdx });
    } else {
        Q_EMIT m_model->dataChanged(m_model->index(idx), m_model->index(idx), QVector<int>{});
    }
}

} // namespace ModelAdapter
