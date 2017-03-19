#include "QNavitQuick.h"
#include <QPainter>

QNavitQuick::QNavitQuick(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
}

QString QNavitQuick::name() const
{
    return m_name;
}

void QNavitQuick::setName(const QString &name)
{
    m_name = name;
}

QColor QNavitQuick::color() const
{
    return m_color;
}

void QNavitQuick::setColor(const QColor &color)
{
    m_color = color;
}

void QNavitQuick::paint(QPainter *painter)
{
    QPen pen(m_color, 2);
    painter->setPen(pen);
    painter->setRenderHints(QPainter::Antialiasing, true);
    painter->drawPie(boundingRect().adjusted(1, 1, -1, -1), 90 * 16, 290 * 16);
}
