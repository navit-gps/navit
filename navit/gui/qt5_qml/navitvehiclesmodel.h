#ifndef NAVITVEHICLESMODEL_H
#define NAVITVEHICLESMODEL_H

#include <QAbstractItemModel>
#include <QDebug>
#include <QVariantMap>
#include "navitinstance.h"
#include "navithelper.h"

#include <glib.h>
extern "C" {
#include "config.h"
#include "item.h" /* needs to be first, as attr.h depends on it */
#include "navit.h"

#include "coord.h"
#include "attr.h"
#include "xmlconfig.h" // for NAVIT_OBJECT
#include "layout.h"
#include "map.h"
#include "transform.h"

#include "mapset.h"
#include "search.h"
#include "vehicleprofile.h"

#include "proxy.h"
}

class NavitVehiclesModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_PROPERTY(NavitInstance * navit MEMBER m_navitInstance WRITE setNavit)
public:
    NavitVehiclesModel(QObject *parent = 0);

    enum VehiclesModelRoles {
        NameRole = Qt::UserRole + 1,
        ActionRole,
        ImageUrlRole
    };

    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex & index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    void setNavit(NavitInstance * navit);
    Q_INVOKABLE void setVehicle(QString name);
    Q_INVOKABLE void update();
private:
    QList<QVariantMap> m_vehicles;
    NavitInstance *m_navitInstance = nullptr;
};

#endif // NAVITVEHICLESMODEL_H
