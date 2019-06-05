#ifndef MAPOBJECT_H
#define MAPOBJECT_H

#include <QObject>

class MapObject : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)

public:
    MapObject(QObject *parent=0);
    MapObject(const QString &name, const bool &active, QObject *parent=0);

    QString name() const;
    void setName(const QString &name);

    bool active() const;
    void setActive(const bool &active);

signals:
    void nameChanged();
    void activeChanged();

private:
    QString m_name;
    bool m_active;
};

#endif // MAPOBJECT_H
