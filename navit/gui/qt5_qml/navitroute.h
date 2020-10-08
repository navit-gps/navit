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
    Q_PROPERTY(Status          status      READ   getStatus       NOTIFY statusChanged)
public:
    NavitRoute();
    void setNavit(NavitInstance * navit);
    static void routeCallbackHandler(NavitRoute * navitRoute);
    static void destinationCallbackHandler(NavitRoute * navitRoute);
    void routeUpdate();
    void destinationUpdate();
    void statusUpdate();

    enum Status {
        Invalid = status_invalid,
        NoRoute = status_no_route,
        Idle = status_no_destination,
        Calculating = status_calculating,
        Recalculating = status_recalculating,
        Navigating = status_routing
    };

    Q_ENUMS(Status)
signals:
    void propertiesChanged();
    void destinationAdded(QString destination);
    void destinationRemoved();
    void navigationFinished();
    void statusChanged();
public slots:
    void setDestination(QString label, int x, int y);
    void setPosition(int x, int y);
    void addStop(QString label, int x, int y, int position);
    void cancelNavigation();

private:
    NavitInstance *m_navitInstance = nullptr;
    QStringList m_directions;
    QString m_distance;
    QString m_timeLeft;
    QString m_arrivalTime;
    Status m_status;

    QString m_lastDestination;
    struct pcoord m_lastDestinationCoord;
    int m_destCount;

    QString getLastDestination(pcoord *pc);
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
    Status getStatus(){
        return m_status;
    }
};

#endif // NAVITROUTE_H
