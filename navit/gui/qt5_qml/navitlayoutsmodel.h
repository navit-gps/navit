#ifndef NAVITLAYOUTSMODEL_H
#define NAVITLAYOUTSMODEL_H

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

#include "proxy.h"
}

class NavitLayoutsModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_PROPERTY(NavitInstance * navit MEMBER m_navitInstance WRITE setNavit)
public:
    NavitLayoutsModel(QObject *parent = 0);

    enum LayoutsModelRoles {
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
    Q_INVOKABLE void setLayout(QString name);
signals:
    void layoutChanged();
private:
    QList<QVariantMap> m_layouts;
    NavitInstance *m_navitInstance = nullptr;
    void update();
    friend class NavitLayersModel;
};

#endif // NAVITLAYOUTSMODEL_H


