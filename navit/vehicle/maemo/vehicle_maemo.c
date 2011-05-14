/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */


/* 
	Plugin for new Maemo's  liblocation API.
 
	<vehicle source="maemo://any" retry_interval="1"/>
   source cound be on of "any","cwp","acwp","gnss","agnss"
   retry_interval could be one of "1","2","5","10","20","30","60","120" measured in seconds
*/

#include <config.h>
#include <string.h>
#include <glib.h>
#include <math.h>
#include <location/location-gps-device.h>
#include <location/location-gpsd-control.h>
#include "debug.h"
#include "callback.h"
#include "plugin.h"
#include "coord.h"
#include "item.h"
#include "vehicle.h"
#include "event.h"

static struct vehicle_priv {
	LocationGPSDControl *control;
	LocationGPSDevice *device;
	char *source;
	guint retry_interval;
	struct callback_list *cbl;
	struct attr ** attrs;
	int sats; // satellites_in_view
	int sats_used; //satellites_in_user
	int fix_type; //mode
	struct coord_geo geo; //lattigute&longittude
	double speed; //speed:)
	double direction; //track
	double height; //altitude
	double hdop; //eph
	time_t fix_time; //time
	char fixiso8601[128];
};

 
static void vehicle_maemo_callback(LocationGPSDevice *device, gpointer user_data) {
	struct vehicle_priv *priv=(struct vehicle_priv*)user_data;

	priv->sats=device->satellites_in_view;
	priv->sats_used=device->satellites_in_use;
	callback_list_call_attr_0(priv->cbl, attr_position_sats);

	dbg(1,"Got update with %u/%u satellites\n",priv->sats_used,priv->sats);

	if (device->fix) {
		switch(device->fix->mode) {
		case LOCATION_GPS_DEVICE_MODE_NOT_SEEN:
		case LOCATION_GPS_DEVICE_MODE_NO_FIX:
		    priv->fix_type=0;
		    break;
		case LOCATION_GPS_DEVICE_MODE_2D:
		case LOCATION_GPS_DEVICE_MODE_3D:
		    priv->fix_type=1;
		    break;
		}

		if (device->fix->fields & LOCATION_GPS_DEVICE_LATLONG_SET) {
			priv->geo.lat=device->fix->latitude;
			priv->geo.lng=device->fix->longitude;
			priv->hdop=device->fix->eph/100;
			callback_list_call_attr_0(priv->cbl, attr_position_coord_geo);
			dbg(1,"Position: %f %f with error %f meters\n",priv->geo.lat,priv->geo.lng,priv->hdop);
		}

		if (device->fix->fields & LOCATION_GPS_DEVICE_SPEED_SET) {
			priv->speed=device->fix->speed;
			callback_list_call_attr_0(priv->cbl, attr_position_speed);
			dbg(1,"Speed: %f\n ",priv->speed);
		}

		if (device->fix->fields & LOCATION_GPS_DEVICE_TRACK_SET) {
			priv->direction=device->fix->track;
			dbg(1,"Direction: %f\n",priv->direction);
		}

		if (device->fix->fields & LOCATION_GPS_DEVICE_TIME_SET) {
			priv->fix_time=device->fix->time;
			dbg(1,"Time: %f\n",priv->fix_time);
		}

		if (device->fix->fields & LOCATION_GPS_DEVICE_ALTITUDE_SET) {
			priv->height=device->fix->altitude;
			dbg(1,"Elevation: %f\n",priv->height);
		}

	}

	return;
}
 
static void vehicle_maemo_error(LocationGPSDControl *control, LocationGPSDControlError error, gpointer user_data)
{ 
  switch (error) {
  case LOCATION_ERROR_USER_REJECTED_DIALOG:
    dbg(0,"User didn't enable requested methods\n");
    break;
  case LOCATION_ERROR_USER_REJECTED_SETTINGS:
    dbg(0,"User changed settings, which disabled location\n");
    break;
  case LOCATION_ERROR_BT_GPS_NOT_AVAILABLE:
    dbg(0,"Problems with BT GPS\n");
    break;
  case LOCATION_ERROR_METHOD_NOT_ALLOWED_IN_OFFLINE_MODE:
    dbg(0,"Requested method is not allowed in offline mode\n");
    break;
  case LOCATION_ERROR_SYSTEM:
    dbg(0,"System error\n");
    break;
  }
}

/**
 * Instantiate liblocation objects
 */
static void
vehicle_maemo_open(struct vehicle_priv *priv)
{

	priv->control = location_gpsd_control_get_default();
	priv->device = g_object_new(LOCATION_TYPE_GPS_DEVICE, NULL);

	if (!strcasecmp(priv->source+8,"cwp")) {
		g_object_set(G_OBJECT(priv->control),	"preferred-method", LOCATION_METHOD_CWP, NULL);
		dbg(1,"Method set: CWP\n");
	} else if (!strcasecmp(priv->source+8,"acwp")) {
		g_object_set(G_OBJECT(priv->control),	"preferred-method", LOCATION_METHOD_ACWP, NULL);
		dbg(1,"Method set: ACWP\n");
	} else if (!strcasecmp(priv->source+8,"gnss")) {
		g_object_set(G_OBJECT(priv->control),	"preferred-method", LOCATION_METHOD_GNSS, NULL);
		dbg(1,"Method set: GNSS\n");
	} else if (!strcasecmp(priv->source+8,"agnss")) {
		g_object_set(G_OBJECT(priv->control),	"preferred-method", LOCATION_METHOD_AGNSS, NULL); 
		dbg(1,"Method set: AGNSS\n");
	} else {
		g_object_set(G_OBJECT(priv->control),	"preferred-method", LOCATION_METHOD_USER_SELECTED, NULL); 
		dbg(1,"Method set: ANY\n");
	}

	switch (priv->retry_interval) {
	case 2:
		g_object_set(G_OBJECT(priv->control),	"preferred-interval", LOCATION_INTERVAL_2S,	NULL);
		dbg(1,"Interval set: 2s\n");
		break;
	case 5:
		g_object_set(G_OBJECT(priv->control),	"preferred-interval", LOCATION_INTERVAL_5S,	NULL);
		dbg(1,"Interval set: 5s\n");
		break;
	case 10:
		g_object_set(G_OBJECT(priv->control),	"preferred-interval", LOCATION_INTERVAL_10S,	NULL);
		dbg(1,"Interval set: 10s\n");
		break;
	case 20:
		g_object_set(G_OBJECT(priv->control),	"preferred-interval", LOCATION_INTERVAL_20S,	NULL);
		dbg(1,"Interval set: 20s\n");
		break;
	case 30:
		g_object_set(G_OBJECT(priv->control),	"preferred-interval", LOCATION_INTERVAL_30S,	NULL);
		dbg(1,"Interval set: 30s\n");
		break;
	case 60:
		g_object_set(G_OBJECT(priv->control),	"preferred-interval", LOCATION_INTERVAL_60S,	NULL);
		dbg(1,"Interval set: 60s\n");
		break;
	case 120:
		g_object_set(G_OBJECT(priv->control),	"preferred-interval", LOCATION_INTERVAL_120S,	NULL);
		dbg(1,"Interval set: 120s\n");
		break;
	case 1:
	default:
		g_object_set(G_OBJECT(priv->control),	"preferred-interval", LOCATION_INTERVAL_1S,	NULL);
		dbg(1,"Interval set: 1s\n");
		break;
	}

	g_signal_connect(priv->device, "changed", G_CALLBACK(vehicle_maemo_callback), priv);
	g_signal_connect(priv->control, "error-verbose", G_CALLBACK(vehicle_maemo_error), priv);

	location_gpsd_control_start(priv->control);

	return;
}

static void
vehicle_maemo_destroy(struct vehicle_priv *priv)
{
	location_gpsd_control_stop(priv->control);

	g_object_unref(priv->device);
	g_object_unref(priv->control);

	return;
}

static int
vehicle_maemo_position_attr_get(struct vehicle_priv *priv,
			       enum attr_type type, struct attr *attr)
{
	struct attr * active=NULL;
	switch (type) {
	case attr_position_fix_type:
		dbg(1,"Attr requested: position_fix_type\n");
		attr->u.num = priv->fix_type;
		break;
	case attr_position_height:
		dbg(1,"Attr requested: position_height\n");
		attr->u.numd = &priv->height;
		break;
	case attr_position_speed:
		dbg(1,"Attr requested: position_speed\n");
		attr->u.numd = &priv->speed;
		break;
	case attr_position_direction:
		dbg(1,"Attr requested: position_direction\n");
		attr->u.numd = &priv->direction;
		break;
	case attr_position_hdop:
		dbg(1,"Attr requested: position_hdop\n");
		attr->u.numd = &priv->hdop;
		break;
	case attr_position_sats:
		dbg(1,"Attr requested: position_sats_signal\n");
		attr->u.num = priv->sats;
		break;
	case attr_position_sats_used:
		dbg(1,"Attr requested: position_sats_used\n");
		attr->u.num = priv->sats_used;
		break;
	case attr_position_coord_geo:
		dbg(1,"Attr requested: position_coord_geo\n");
		attr->u.coord_geo = &priv->geo;
		break;
	case attr_position_time_iso8601:
		{
		struct tm tm;
		dbg(1,"Attr requested: position_time_iso8601\n");
		if (!priv->fix_time)
			return 0;
		if (gmtime_r(&priv->fix_time, &tm)) {
			strftime(priv->fixiso8601, sizeof(priv->fixiso8601),
				"%Y-%m-%dT%TZ", &tm);
			attr->u.str=priv->fixiso8601;
		} else
			return 0;
		}
		break;
	case attr_active:
		dbg(1,"Attr requested: position_active\n");
		active = attr_search(priv->attrs,NULL,attr_active);
		if(active != NULL) {
			attr->u.num=active->u.num;
			return 1;
		} else
			return 0;
			break;
	default:
			return 0;
	}
	attr->type = type;
	return 1;
}

struct vehicle_methods vehicle_maemo_methods = {
	vehicle_maemo_destroy,
	vehicle_maemo_position_attr_get,
};

static struct vehicle_priv *
vehicle_maemo_new_maemo(struct vehicle_methods
		      *meth, struct callback_list
		      *cbl, struct attr **attrs)
{
	struct vehicle_priv *ret;
	struct attr *source, *retry_int;

	dbg(1, "enter\n");
	source = attr_search(attrs, NULL, attr_source);
	ret = g_new0(struct vehicle_priv, 1);
	ret->source = g_strdup(source->u.str);
	retry_int = attr_search(attrs, NULL, attr_retry_interval);
	if (retry_int) {
		ret->retry_interval = retry_int->u.num;
		if (ret->retry_interval !=1 && ret->retry_interval !=2 && ret->retry_interval !=5 && ret->retry_interval !=10 && ret->retry_interval !=20 && ret->retry_interval !=30 && ret->retry_interval !=60 && ret->retry_interval !=120 ) {
			dbg(0, "Retry interval %d invalid, setting to 1\n", ret->retry_interval,1);
			ret->retry_interval = 1;
		}
	} else {
		ret->retry_interval = 1;
	}
	dbg(1,"source: %s, interval: %u\n",ret->source,ret->retry_interval);
	ret->cbl = cbl;
	*meth = vehicle_maemo_methods;
	ret->attrs = attrs;
	vehicle_maemo_open(ret);
	return ret;
}

void
plugin_init(void)
{
	dbg(1, "enter\n");
	plugin_register_vehicle_type("maemo", vehicle_maemo_new_maemo);
}
