#include "qml_poi.h"

PoiObject::PoiObject(QObject *parent)
    : QObject(parent) {
}

PoiObject::PoiObject(
    const QString &name,
    const QString &type,
    const int distance,
    const QString &icon,
    struct pcoord &coords,
    QObject *parent)
    : QObject(parent), m_name(name), m_type(type), m_icon(icon), m_coords(coords) {
}

QString PoiObject::name() const {
    return m_name;
}

void PoiObject::setName(const QString &name) {
    if (name != m_name) {
        m_name = name;
        emit nameChanged();
    }
}

QString PoiObject::type() const {
    return m_type;
}

void PoiObject::setType(const QString &type) {
    if (type != m_type) {
        m_type = type;
        emit typeChanged();
    }
}

float PoiObject::distance() const {
    return m_distance/1000;
}

void PoiObject::setDistance(const int distance) {
    if (distance != m_distance) {
        m_distance = distance;
        emit distanceChanged();
    }
}

void PoiObject::setIcon(const QString &icon) {
    if (icon != m_icon) {
        m_icon = icon;
        emit iconChanged();
    }
}

QString PoiObject::icon() const {
    return m_icon;
}

struct pcoord PoiObject::coords() const {
    return m_coords;
}
