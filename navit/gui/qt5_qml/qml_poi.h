#ifndef POIOBJECT_H
#define POIOBJECT_H

#include <QObject>
#include "coord.h"

class PoiObject : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString type READ type WRITE setType NOTIFY typeChanged)
    Q_PROPERTY(int distance READ distance WRITE setDistance NOTIFY distanceChanged)
    Q_PROPERTY(QString icon READ icon WRITE setIcon NOTIFY iconChanged)
    Q_PROPERTY(struct pcoord coords NOTIFY coordsChanged)

public:
    PoiObject(QObject *parent=0);
    PoiObject(const QString &name, const QString &type, const int distance, const QString &icon, struct pcoord &coords, QObject *parent=0);
    QString name() const;
    void setName(const QString &name);

    QString type() const;
    void setType(const QString &type);

    float distance() const;
    void setDistance(const int distance);

    QString icon() const;
    void setIcon(const QString &icon);

    struct pcoord coords() const;
//    void setcoords(const struct pcoord &c);

signals:
    void nameChanged();
    void typeChanged();
    void distanceChanged();
    void iconChanged();
//    void coordsChanged();

private:
    struct pcoord m_coords;
    QString m_name;
    QString m_type;
    int m_distance;
    QString m_icon;
};

#endif // POIOBJECT_H

