#include "multichoicecontroller.hpp"

void MultiChoiceController::show(QQuickItem *parent, const QStringList &options, qreal x, qreal y)
{
    Q_ASSERT(parent);

    const QPointF origin = parent->mapToScene(QPointF(0, 0));
    m_originX = origin.x();
    m_originY = origin.y();
    m_centerX = m_originX + x;
    m_centerY = m_originY + y;
    m_mouseX = m_centerX;
    m_mouseY = m_centerY;
    m_visible = true;
    m_options = options;
    m_selected = QVector<bool>(options.size(), false);
    m_selectedIndex = -1;

    emit popupChanged();
}

void MultiChoiceController::move(qreal x, qreal y)
{
    m_mouseX = m_originX + x;
    m_mouseY = m_originY + y;
    emit mouseChanged();
}

void MultiChoiceController::hide()
{
    m_visible = false;
    m_options.clear();
    emit popupChanged();
}

void MultiChoiceController::setSelected(int index, bool selected)
{
    if (index < 0 || index >= m_selected.size())
        return;

    m_selected[index] = selected;

    int selectedIndex = -1;
    for (int i = 0; i < m_selected.size(); ++i) {
        if (m_selected[i])
            selectedIndex = i;
    }

    if (m_selectedIndex != selectedIndex) {
        m_selectedIndex = selectedIndex;
        emit selectedIndexChanged(selectedIndex);
    }
}
