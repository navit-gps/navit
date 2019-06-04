#include "qml_search.h"

SearchObject::SearchObject(QObject *parent)
    : QObject(parent) {
}

SearchObject::SearchObject(const QString &name, const QString &icon, struct pcoord *c, QObject *parent)
    : QObject(parent), m_name(name), m_icon(icon), m_c(c) {
}

QString SearchObject::name() const {
    return m_name;
}

void SearchObject::setName(const QString &name) {
    if (name != m_name) {
        m_name = name;
        emit nameChanged();
    }
}

QString SearchObject::icon() const {
    return m_icon;
}

void SearchObject::setIcon(const QString &icon) {
    if (icon != m_icon) {
        m_icon = icon;
        emit iconChanged();
    }
}

struct pcoord * SearchObject::getCoords() const {
    return m_c;
}
