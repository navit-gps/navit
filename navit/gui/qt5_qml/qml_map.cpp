#include "qml_map.h"

MapObject::MapObject(QObject *parent)
    : QObject(parent) {
}

MapObject::MapObject(const QString &name, const bool &active, QObject *parent)
    : QObject(parent), m_name(name), m_active(active) {
}

QString MapObject::name() const {
    return m_name;
}

void MapObject::setName(const QString &name) {
    if (name != m_name) {
        m_name = name;
        emit nameChanged();
    }
}

bool MapObject::active() const {
    return m_active;
}

void MapObject::setActive(const bool &active) {
    if (active != m_active) {
        m_active = active;
        emit activeChanged();
    }
}

