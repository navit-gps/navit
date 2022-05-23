#include "qml_bookmark.h"

BookmarkObject::BookmarkObject(QObject *parent)
    : QObject(parent) {
}

BookmarkObject::BookmarkObject(
    const QString &name,
    struct pcoord &coords,
    QObject *parent)
    : QObject(parent), m_name(name), m_coords(coords) {
}

QString BookmarkObject::name() const {
    return m_name;
}

void BookmarkObject::setName(const QString &name) {
    if (name != m_name) {
        m_name = name;
        emit nameChanged();
    }
}

struct pcoord BookmarkObject::coords() const {
    return m_coords;
}
