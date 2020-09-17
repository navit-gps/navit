#ifndef NAVITROUTE_H
#define NAVITROUTE_H

#include <QObject>
#include <QDebug>
#include <QTime>
#include "navitinstance.h"

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
#include "bookmarks.h"

#include "callback.h"
#include "route.h"
#include "navigation.h"
#include "layout.h"

#include "proxy.h"
}

#include "navithelper.h"

class NavitRoute : public QObject
{
    Q_OBJECT
    Q_PROPERTY(NavitInstance * navit       MEMBER m_navitInstance WRITE setNavit)
    Q_PROPERTY(QStringList     directions  READ   getDirections   NOTIFY propertiesChanged)
    Q_PROPERTY(QString         distance    READ   getDistance     NOTIFY propertiesChanged)
    Q_PROPERTY(QString         timeLeft    READ   getTimeLeft     NOTIFY propertiesChanged)
    Q_PROPERTY(QString         arrivalTime READ   getArrivalTime  NOTIFY propertiesChanged)
public:
    NavitRoute();
    void setNavit(NavitInstance * navit);
    static void routeCallbackHandler(NavitRoute * navitRoute);
    static void destinationCallbackHandler(NavitRoute * navitRoute);
    void routeUpdate();
    void destinationUpdate();
    Q_INVOKABLE void setDestination(QString label, int x, int y);
    Q_INVOKABLE void setPosition(int x, int y);
    Q_INVOKABLE void addStop(QString label, int x, int y, int position);
    Q_INVOKABLE void cancelNavigation();
signals:
    void propertiesChanged();
    void destinationAdded();
    void destinationRemoved();
    void navigationFinished();
private:
    NavitInstance *m_navitInstance = nullptr;
    QStringList m_directions;
    QString m_distance;
    QString m_timeLeft;
    QString m_arrivalTime;
    int m_destCount;

    QStringList getDirections() {
        return m_directions;
    }
    QString getDistance() {
        return m_distance;
    }
    QString getTimeLeft() {
        return m_timeLeft;
    }
    QString getArrivalTime() {
        return m_arrivalTime;
    }
};

#endif // NAVITROUTE_H
