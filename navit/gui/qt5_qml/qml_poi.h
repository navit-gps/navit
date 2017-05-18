#ifndef POIOBJECT_H
#define POIOBJECT_H

#include <QObject>

class PoiObject : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(int distance READ distance WRITE setDistance NOTIFY distanceChanged)
    Q_PROPERTY(QString icon READ icon WRITE setIcon NOTIFY iconChanged)

public:
    PoiObject(QObject *parent=0);
    PoiObject(const QString &name, const int distance, const QString &icon, QObject *parent=0);

    QString name() const;
    void setName(const QString &name);

    float distance() const;
    void setDistance(const int distance);

    QString icon() const;
    void setIcon(const QString &icon);

signals:
    void nameChanged();
    void distanceChanged();
    void iconChanged();

private:
    QString m_name;
    int m_distance;
    QString m_icon;
};

#endif // POIOBJECT_H

