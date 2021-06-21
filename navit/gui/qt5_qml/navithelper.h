#ifndef NAVITHELPER_H
#define NAVITHELPER_H

#include <QString>
#include <QDebug>

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

#include "proxy.h"

#include "event.h"
#include "callback.h"
}

class NavitHelper
{
public:
    NavitHelper();

    static QString getAddress(NavitInstance * navitInstance, struct coord center, QString filter = "");
    static QVariantMap getPOI(NavitInstance *navitInstance, struct coord center, int distance = 2);
    static QString getClosest(QList<QVariantMap> items, int maxDistance = -1);
    static QString formatDist(int dist);
    static pcoord positionToPcoord (navit *navit, int x, int y);
    static coord positionToCoord (navit *navit, int x, int y);
    static pcoord coordToPcoord(navit *navit, int x, int y);
    static void setDestination(NavitInstance *navitInstance, QString label, int x, int y);
    static void setPosition(NavitInstance *navitInstance, int x, int y);
    static void addBookmark(NavitInstance *navitInstance, QString label, int x, int y);
    static void addStop(NavitInstance *navitInstance, int position, QString label, int x, int y);
};

#endif // NAVITHELPER_H
