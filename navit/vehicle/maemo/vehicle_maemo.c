/*
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

/**
 * @defgroup vehicle-iphone Vehicle Maemo
 * @ingroup vehicle-plugins
 * @brief The Vehicle to gain position data from Maemo.
 *
 * Plugin for new Maemo's  liblocation API.
 * <vehicle source="maemo://any" retry_interval="1"/>
 *  source cound be on of "any","cwp","acwp","gnss","agnss"
 *  retry_interval could be one of "1","2","5","10","20","30","60","120" measured in seconds
 *
 * @{
 */

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

    dbg(lvl_debug,"Got update with %u/%u satellites",priv->sats_used,priv->sats);

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
            dbg(lvl_debug,"Position: %f %f with error %f meters",priv->geo.lat,priv->geo.lng,priv->hdop);
        }

        if (device->fix->fields & LOCATION_GPS_DEVICE_SPEED_SET) {
            priv->speed=device->fix->speed;
            callback_list_call_attr_0(priv->cbl, attr_position_speed);
            dbg(lvl_debug,"Speed: %f ",priv->speed);
        }

        if (device->fix->fields & LOCATION_GPS_DEVICE_TRACK_SET) {
            priv->direction=device->fix->track;
            dbg(lvl_debug,"Direction: %f",priv->direction);
        }

        if (device->fix->fields & LOCATION_GPS_DEVICE_TIME_SET) {
            priv->fix_time=device->fix->time;
            dbg(lvl_debug,"Time: %f",priv->fix_time);
        }

        if (device->fix->fields & LOCATION_GPS_DEVICE_ALTITUDE_SET) {
            priv->height=device->fix->altitude;
            dbg(lvl_debug,"Elevation: %f",priv->height);
        }

    }

    return;
}

static void vehicle_maemo_error(LocationGPSDControl *control, LocationGPSDControlError error, gpointer user_data) {
    switch (error) {
    case LOCATION_ERROR_USER_REJECTED_DIALOG:
        dbg(lvl_error,"User didn't enable requested methods");
        break;
    case LOCATION_ERROR_USER_REJECTED_SETTINGS:
        dbg(lvl_error,"User changed settings, which disabled location");
        break;
    case LOCATION_ERROR_BT_GPS_NOT_AVAILABLE:
        dbg(lvl_error,"Problems with BT GPS");
        break;
    case LOCATION_ERROR_METHOD_NOT_ALLOWED_IN_OFFLINE_MODE:
        dbg(lvl_error,"Requested method is not allowed in offline mode");
        break;
    case LOCATION_ERROR_SYSTEM:
        dbg(lvl_error,"System error");
        break;
    }
}

/**
 * Instantiate liblocation objects
 */
static void vehicle_maemo_open(struct vehicle_priv *priv) {

    priv->control = location_gpsd_control_get_default();
    priv->device = g_object_new(LOCATION_TYPE_GPS_DEVICE, NULL);

    if (!strcasecmp(priv->source+8,"cwp")) {
        g_object_set(G_OBJECT(priv->control),	"preferred-method", LOCATION_METHOD_CWP, NULL);
        dbg(lvl_debug,"Method set: CWP");
    } else if (!strcasecmp(priv->source+8,"acwp")) {
        g_object_set(G_OBJECT(priv->control),	"preferred-method", LOCATION_METHOD_ACWP, NULL);
        dbg(lvl_debug,"Method set: ACWP");
    } else if (!strcasecmp(priv->source+8,"gnss")) {
        g_object_set(G_OBJECT(priv->control),	"preferred-method", LOCATION_METHOD_GNSS, NULL);
        dbg(lvl_debug,"Method set: GNSS");
    } else if (!strcasecmp(priv->source+8,"agnss")) {
        g_object_set(G_OBJECT(priv->control),	"preferred-method", LOCATION_METHOD_AGNSS, NULL);
        dbg(lvl_debug,"Method set: AGNSS");
    } else {
        g_object_set(G_OBJECT(priv->control),	"preferred-method", LOCATION_METHOD_USER_SELECTED, NULL);
        dbg(lvl_debug,"Method set: ANY");
    }

    switch (priv->retry_interval) {
    case 2:
        g_object_set(G_OBJECT(priv->control),	"preferred-interval", LOCATION_INTERVAL_2S,	NULL);
        dbg(lvl_debug,"Interval set: 2s");
        break;
    case 5:
        g_object_set(G_OBJECT(priv->control),	"preferred-interval", LOCATION_INTERVAL_5S,	NULL);
        dbg(lvl_debug,"Interval set: 5s");
        break;
    case 10:
        g_object_set(G_OBJECT(priv->control),	"preferred-interval", LOCATION_INTERVAL_10S,	NULL);
        dbg(lvl_debug,"Interval set: 10s");
        break;
    case 20:
        g_object_set(G_OBJECT(priv->control),	"preferred-interval", LOCATION_INTERVAL_20S,	NULL);
        dbg(lvl_debug,"Interval set: 20s");
        break;
    case 30:
        g_object_set(G_OBJECT(priv->control),	"preferred-interval", LOCATION_INTERVAL_30S,	NULL);
        dbg(lvl_debug,"Interval set: 30s");
        break;
    case 60:
        g_object_set(G_OBJECT(priv->control),	"preferred-interval", LOCATION_INTERVAL_60S,	NULL);
        dbg(lvl_debug,"Interval set: 60s");
        break;
    case 120:
        g_object_set(G_OBJECT(priv->control),	"preferred-interval", LOCATION_INTERVAL_120S,	NULL);
        dbg(lvl_debug,"Interval set: 120s");
        break;
    case 1:
    default:
        g_object_set(G_OBJECT(priv->control),	"preferred-interval", LOCATION_INTERVAL_1S,	NULL);
        dbg(lvl_debug,"Interval set: 1s");
        break;
    }

    g_signal_connect(priv->device, "changed", G_CALLBACK(vehicle_maemo_callback), priv);
    g_signal_connect(priv->control, "error-verbose", G_CALLBACK(vehicle_maemo_error), priv);

    location_gpsd_control_start(priv->control);

    return;
}

static void vehicle_maemo_destroy(struct vehicle_priv *priv) {
    location_gpsd_control_stop(priv->control);

    g_object_unref(priv->device);
    g_object_unref(priv->control);

    return;
}

static int vehicle_maemo_position_attr_get(struct vehicle_priv *priv,
        enum attr_type type, struct attr *attr) {
    struct attr * active=NULL;
    switch (type) {
    case attr_position_fix_type:
        dbg(lvl_debug,"Attr requested: position_fix_type");
        attr->u.num = priv->fix_type;
        break;
    case attr_position_height:
        dbg(lvl_debug,"Attr requested: position_height");
        attr->u.numd = &priv->height;
        break;
    case attr_position_speed:
        dbg(lvl_debug,"Attr requested: position_speed");
        attr->u.numd = &priv->speed;
        break;
    case attr_position_direction:
        dbg(lvl_debug,"Attr requested: position_direction");
        attr->u.numd = &priv->direction;
        break;
    case attr_position_hdop:
        dbg(lvl_debug,"Attr requested: position_hdop");
        attr->u.numd = &priv->hdop;
        break;
    case attr_position_sats:
        dbg(lvl_debug,"Attr requested: position_sats");
        attr->u.num = priv->sats;
        break;
    case attr_position_sats_used:
        dbg(lvl_debug,"Attr requested: position_sats_used");
        attr->u.num = priv->sats_used;
        break;
    case attr_position_coord_geo:
        dbg(lvl_debug,"Attr requested: position_coord_geo");
        attr->u.coord_geo = &priv->geo;
        break;
    case attr_position_time_iso8601: {
        struct tm tm;
        dbg(lvl_debug,"Attr requested: position_time_iso8601");
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
        dbg(lvl_debug,"Attr requested: position_active");
        active = attr_search(priv->attrs,attr_active);
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

static struct vehicle_priv *vehicle_maemo_new_maemo(struct vehicle_methods
        *meth, struct callback_list
        *cbl, struct attr **attrs) {
    struct vehicle_priv *ret;
    struct attr *source, *retry_int;

    dbg(lvl_debug, "enter");
    source = attr_search(attrs, attr_source);
    ret = g_new0(struct vehicle_priv, 1);
    ret->source = g_strdup(source->u.str);
    retry_int = attr_search(attrs, attr_retry_interval);
    if (retry_int) {
        ret->retry_interval = retry_int->u.num;
        if (ret->retry_interval !=1 && ret->retry_interval !=2 && ret->retry_interval !=5 && ret->retry_interval !=10
                && ret->retry_interval !=20 && ret->retry_interval !=30 && ret->retry_interval !=60 && ret->retry_interval !=120 ) {
            dbg(lvl_error, "Retry interval %d invalid, setting to 1", ret->retry_interval,1);
            ret->retry_interval = 1;
        }
    } else {
        ret->retry_interval = 1;
    }
    dbg(lvl_debug,"source: %s, interval: %u",ret->source,ret->retry_interval);
    ret->cbl = cbl;
    *meth = vehicle_maemo_methods;
    ret->attrs = attrs;
    vehicle_maemo_open(ret);
    return ret;
}

void plugin_init(void) {
    dbg(lvl_debug, "enter");
    plugin_register_category_vehicle("maemo", vehicle_maemo_new_maemo);
}
