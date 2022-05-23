#include "qml_vehicle.h"

VehicleObject::VehicleObject(QObject *parent)
    : QObject(parent) {
}

VehicleObject::VehicleObject(const QString &name, const bool &active, struct vehicle *v, QObject *parent)
    : QObject(parent), m_name(name), m_active(active), m_vehicle(v) {
}

QString VehicleObject::name() const {
    return m_name;
}

void VehicleObject::setName(const QString &name) {
    if (name != m_name) {
        m_name = name;
        emit nameChanged();
    }
}

bool VehicleObject::active() const {
    return m_active;
}

void VehicleObject::setActive(const bool &active) {
    if (active != m_active) {
        m_active = active;
        emit activeChanged();
    }
}

struct vehicle * VehicleObject::vehicle() const {
    return m_vehicle;
}

void VehicleObject::setVehicle(struct vehicle * vehicle) {
    if (vehicle != m_vehicle) {
        m_vehicle = vehicle;
        emit vehicleChanged();
    }
}


