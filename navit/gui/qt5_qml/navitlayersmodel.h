#ifndef NAVITLAYERS_H
#define NAVITLAYERS_H

#include <QAbstractItemModel>
#include <QDebug>
#include <QVariantMap>
#include "navitinstance.h"
#include "navithelper.h"
#include "navitlayoutsmodel.h"


class NavitLayersModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_PROPERTY(NavitLayoutsModel * layout MEMBER m_navitLayoutsInstance WRITE setNavitLayers)
public:
    NavitLayersModel(QObject *parent = 0);

    enum LayersModelRoles {
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

    void setNavitLayers(NavitLayoutsModel * navitLayouts);
    Q_INVOKABLE void toggleLayer(QString name);

private:
    QList<QVariantMap> m_layers;
    NavitLayoutsModel *m_navitLayoutsInstance = nullptr;
    void update();
};

#endif // NAVITLAYERS_H
