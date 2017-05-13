#include <QDebug>
#include "qml_poi.h"

PoiObject::PoiObject(QObject *parent)
    : QObject(parent)
{
}

PoiObject::PoiObject(
    const QString &name,
    const bool &active,
    const int idist,
    QObject *parent)
    : QObject(parent), m_name(name), m_active(active)
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

bool PoiObject::active() const
{
    return m_active;
}

void PoiObject::setActive(const bool &active)
{
    if (active != m_active) {
        m_active = active;
        emit activeChanged();
    }
}

int PoiObject::distance() const
{
    return m_distance;
}

void PoiObject::setDistance(const int distance)
{
    if (distance != m_distance) {
        m_distance = distance;
        emit distanceChanged();
    }
}

