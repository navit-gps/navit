#ifndef NAVITPOIMODEL_H
#define NAVITPOIMODEL_H

#include <QAbstractItemModel>
#include <QDebug>
#include <QThread>
#include <QThreadPool>
#include <QRunnable>
#include <QMutex>
#include <QMap>
#include <QVariant>
#include <QJsonDocument>
#include <QJsonObject>

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


class POISearchWorker : public QThread
{
    Q_OBJECT
public:
    POISearchWorker (NavitInstance * navit, QString filter, int screenX, int screenY, int distance);
    ~POISearchWorker ();
    void run() override;
signals:
    void gotSearchResult(QVariantMap poi);
private:
    NavitInstance * m_navitInstance;
    QString m_filter;
    int m_screenX;
    int m_screenY;
    int m_distance;
};

class NavitPOIModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_PROPERTY(NavitInstance * navit MEMBER m_navitInstance WRITE setNavit)
public:
    enum POIModelRoles {
        NameRole = Qt::UserRole + 1,
        TypeRole,
        DistanceRole,
        IconRole,
        CoordinatesRole,
        AddressRole,
        LabelRole
    };

    NavitPOIModel(QObject *parent = 0);
    ~NavitPOIModel() override;
    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex & index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
public slots:
    void search(QString filter, int screenX = -1, int screenY = -1, int distance = 15000);
    void setAsDestination(int index);
    void setAsPosition(int index);
    void addAsBookmark(int index);
    void addStop(int index,  int position);
    void setNavit(NavitInstance * navit);
    void reset();
private slots:
    void receiveSearchResult(QVariantMap poi);
private:
    QList<QVariantMap> m_pois;
    NavitInstance *m_navitInstance = nullptr;
    QStringList m_poiTypes;
    QString getAddressString(struct item *item, int prependPostal);
    void stopWorker(bool clearModel = false);
    POISearchWorker *m_poiWorker = nullptr;
    QMutex modelMutex;
};

#endif // NAVITPOIMODEL_H
