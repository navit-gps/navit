#ifndef NAVITROUTE_H
#define NAVITROUTE_H

#include <QObject>
#include <QDebug>
#include <QTime>
#include <QUrl>
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
#include "graphics.h"
#include "vehicle.h"
#include "vehicleprofile.h"
#include "roadprofile.h"
#include "track.h"

#include "proxy.h"
}

#include "navithelper.h"

class NavitRoute : public QObject
{
    Q_OBJECT
    Q_PROPERTY(NavitInstance * navit            MEMBER m_navitInstance     WRITE setNavit)
    Q_PROPERTY(QStringList     directions       READ   getDirections       NOTIFY propertiesChanged)
    Q_PROPERTY(QString         distance         READ   getDistance         NOTIFY propertiesChanged)
    Q_PROPERTY(QString         timeLeft         READ   getTimeLeft         NOTIFY propertiesChanged)
    Q_PROPERTY(QString         arrivalTime      READ   getArrivalTime      NOTIFY propertiesChanged)
    Q_PROPERTY(QString         currentStreet    READ   getCurrentStreet    NOTIFY propertiesChanged)
    Q_PROPERTY(int             speed            READ   getSpeed            NOTIFY propertiesChanged)
    Q_PROPERTY(int             speedLimit       READ   getSpeedLimit       NOTIFY propertiesChanged)
    Q_PROPERTY(Status          status           READ   getStatus           NOTIFY statusChanged)
    Q_PROPERTY(QUrl            nextTurnIcon     READ   getNextTurnIcon     NOTIFY nextTurnChanged)
    Q_PROPERTY(QString         nextTurn         READ   getNextTurn         NOTIFY nextTurnChanged)
    Q_PROPERTY(QString         nextTurnDistance READ   getNextTurnDistance NOTIFY nextTurnChanged)
public:
    NavitRoute();
    void setNavit(NavitInstance * navit);
    static void routeCallbackHandler(NavitRoute * navitRoute);
    static void destinationCallbackHandler(NavitRoute * navitRoute);
    static void statusCallbackHandler(NavitRoute * navitRoute, int status);
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
    void nextTurnChanged();
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
    QString m_currentStreet;
    QString m_nextTurn;
    QString m_nextTurnDistance;
    int m_speed;
    int m_speedLimit;
    QUrl m_nextTurnIcon;
    Status m_status;

    QString m_lastDestination;
    struct pcoord m_lastDestinationCoord;
    int m_destCount;

    QString getLastDestination(pcoord *pc);
    void updateNextTurn(struct map * map);
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
    QString getCurrentStreet() {
        return m_currentStreet;
    }
    QString getNextTurn() {
        return m_nextTurn;
    }
    QString getNextTurnDistance() {
        return m_nextTurnDistance;
    }
    int getSpeed() {
        return m_speed;
    }
    int getSpeedLimit() {
        return m_speedLimit;
    }
    Status getStatus(){
        return m_status;
    }
    QUrl getNextTurnIcon(){
        return m_nextTurnIcon;
    }
};

#endif // NAVITROUTE_H
