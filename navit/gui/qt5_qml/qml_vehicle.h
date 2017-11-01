#ifndef VEHICLEOBJECT_H
#define VEHICLEOBJECT_H

#include <QObject>
#include "item.h"
#include "vehicle.h"

class VehicleObject : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    //Q_PROPERTY(struct vehicle * vehicle READ vehicle WRITE setVehicle NOTIFY vehicleChanged)

public:
    VehicleObject(QObject *parent=0);
    VehicleObject(const QString &name, const bool &active, struct vehicle *v, QObject *parent=0);

    QString name() const;
    void setName(const QString &name);

    bool active() const;
    void setActive(const bool &active);

    struct vehicle * vehicle() const;
    void setVehicle(struct vehicle * vehicle);

signals:
    void nameChanged();
    void activeChanged();
    void vehicleChanged();

private:
    QString m_name;
    bool m_active;
    struct vehicle *m_vehicle;
};

#endif // MAPOBJECT_H
