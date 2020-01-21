#pragma once

#include <QQuickItem>

class MultiChoiceController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList options READ options NOTIFY popupChanged)
    Q_PROPERTY(qreal centerX READ centerX NOTIFY popupChanged)
    Q_PROPERTY(qreal centerY READ centerY NOTIFY popupChanged)
    Q_PROPERTY(qreal mouseX READ mouseX NOTIFY mouseChanged)
    Q_PROPERTY(qreal mouseY READ mouseY NOTIFY mouseChanged)
    Q_PROPERTY(bool visible READ visible NOTIFY popupChanged)
    Q_PROPERTY(int selected READ selected NOTIFY selectedIndexChanged)

public:
    using QObject::QObject;
    ~MultiChoiceController() override = default;

    QStringList options() const { return m_options; }
    qreal centerX() const { return m_centerX; }
    qreal centerY() const { return m_centerY; }
    qreal mouseX() const { return m_mouseX; }
    qreal mouseY() const { return m_mouseY; }
    bool visible() const { return m_visible; }
    int selected() const { return m_selectedIndex; }

    Q_INVOKABLE void show(QQuickItem *parent, const QStringList &options, qreal x, qreal y);
    Q_INVOKABLE void move(qreal x, qreal y);
    Q_INVOKABLE void hide();
    Q_INVOKABLE void setSelected(int index, bool selected);

signals:
    void popupChanged();
    void mouseChanged();
    void selectedIndexChanged(int selected);

private:
    QStringList m_options;
    QVector<bool> m_selected;
    qreal m_centerX = 0.0;
    qreal m_centerY = 0.0;
    qreal m_mouseX = 0.0;
    qreal m_mouseY = 0.0;
    qreal m_originX = 0.0;
    qreal m_originY = 0.0;
    bool m_visible = false;
    int m_selectedIndex;
};
