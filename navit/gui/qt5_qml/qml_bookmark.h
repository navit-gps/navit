#ifndef BOOKMARKOBJECT_H
#define BOOKMARKOBJECT_H

#include <QObject>
#include "coord.h"

class BookmarkObject : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(struct pcoord coords NOTIFY coordsChanged)

public:
    BookmarkObject(QObject *parent=0);
    BookmarkObject(const QString &name, struct pcoord &coords, QObject *parent=0);
    QString name() const;
    void setName(const QString &name);

    struct pcoord coords() const;

signals:
    void nameChanged();

private:
    struct pcoord m_coords;
    QString m_name;
};

#endif // BOOKMARKOBJECT_H

