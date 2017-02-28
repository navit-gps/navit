#ifndef __vehicle_qt5_h
#define __vehicle_qt5_h

#include <config.h>
#include <string.h>
#include <glib.h>
#include <math.h>
#include <time.h>
#include "debug.h"
#include "plugin.h"
#include "coord.h"
#include "item.h"
#include "vehicle.h"

#include <QObject>
#include <QStringList>
#include <QGeoPositionInfoSource>
#include <QGeoSatelliteInfoSource>

#include "callback.h"

#include <QObject>

class QNavitGeoReceiver;
struct vehicle_priv {
	struct callback_list *cbl;
	struct coord_geo geo;
	double speed;
	double direction;
	double height;
	double radius;
	int fix_type;
	time_t fix_time;
	char fixiso8601[128];
	int sats;
	int sats_used;
	int have_coords;
	struct attr ** attrs;
        
        QGeoPositionInfoSource *source;
        QGeoSatelliteInfoSource *satellites;
        QNavitGeoReceiver * receiver;
};

class QNavitGeoReceiver : public QObject
{
        Q_OBJECT
public:
        QNavitGeoReceiver (QObject * parent, struct vehicle_priv * c);
public slots:
        void positionUpdated(const QGeoPositionInfo &info);
        void satellitesInUseUpdated(const QList<QGeoSatelliteInfo> & satellites);
        void satellitesInViewUpdated(const QList<QGeoSatelliteInfo> & satellites);

private:
        struct vehicle_priv * priv;
};
#endif