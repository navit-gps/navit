#include <QDebug>
#include "qml_poi.h"

PoiObject::PoiObject(QObject *parent)
    : QObject(parent)
{
}

PoiObject::PoiObject(
    const QString &name,
    const int distance,
    const QString &icon,
    QObject *parent)
    : QObject(parent), m_name(name), m_icon(icon)
{
}

QString PoiObject::name() const
{
    return m_name;
}

void PoiObject::setName(const QString &name)
{
    if (name != m_name) {
        m_name = name;
        emit nameChanged();
    }
}

float PoiObject::distance() const
{
    return m_distance/1000;
}

void PoiObject::setDistance(const int distance)
{
    if (distance != m_distance) {
        m_distance = distance;
        emit distanceChanged();
    }
}

void PoiObject::setIcon(const QString &icon)
{
    qDebug() << icon;
    if (icon != m_icon) {
        m_icon = icon;
        emit iconChanged();
    }
}

QString PoiObject::icon() const
{
    return m_icon;
}

