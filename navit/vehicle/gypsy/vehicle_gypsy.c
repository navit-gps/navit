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
 *
 */

#include <config.h>
#include <gypsy/gypsy-device.h>
#include <gypsy/gypsy-control.h>
#include <gypsy/gypsy-course.h>
#include <gypsy/gypsy-position.h>
#include <gypsy/gypsy-satellite.h>
#ifdef USE_BINDING_DBUS
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#endif
#ifdef HAVE_UNISTD_H
#include <sys/types.h>
#include <unistd.h>
#endif
#include <string.h>
#include <glib.h>
#include <math.h>
#include "debug.h"
#include "callback.h"
#include "plugin.h"
#include "coord.h"
#include "item.h"
#include "vehicle.h"

/**
 * @defgroup vehicle-gypsy Vehicle gypsy
 * @ingroup vehicle-plugins
 * @brief The Vehicle to gain position data from gypsy. gypsy uses dbus signals
 * @Author Tim Niemeyer <reddog@mastersword.de>
 * @date 2008-2009
 *
 * @{
 */

static struct vehicle_priv {
    char *source;
    GypsyControl *control;
    GypsyPosition *position;
    GypsyDevice *device;
    GypsyCourse *course;
    GypsySatellite *satellite;
    char *path;
    struct callback_list *cbl;
    guint retry_interval;
    struct coord_geo geo;
    double speed;
    double direction;
    double height;
    int fix_type;
    time_t fix_time;
    char fixiso8601[128];
    int sats;
    int sats_used;
    guint retry_timer;
    struct attr ** attrs;
    int have_cords;
} *vehicle_last;

#define DEFAULT_RETRY_INTERVAL 10 // seconds
#define MIN_RETRY_INTERVAL 1 // seconds

/**
 * @brief When the fixstatus has changed, this function get called
 *
 * fixstatus can be one of this:
 * GYPSY_DEVICE_FIX_STATUS_INVALID = 0
 * GYPSY_DEVICE_FIX_STATUS_NONE = 1
 * GYPSY_DEVICE_FIX_STATUS_2D = 2
 * GYPSY_DEVICE_FIX_STATUS_3D = 3
 *
 * Anytime this functions get called, we have to call the global
 * callback.
 *
 * @param device The GypsyDevice
 * @param fixstatus The fisstatus 0, 1, 2 or 3
 * @param userdata
 * @returns nothing
 */
static void vehicle_gypsy_fixstatus_changed(GypsyDevice *device,
        gint fixstatus,
        gpointer userdata) {
    struct vehicle_priv *priv = vehicle_last;

    if (fixstatus==GYPSY_DEVICE_FIX_STATUS_3D)
        priv->fix_type = 3;
    else if (fixstatus==GYPSY_DEVICE_FIX_STATUS_2D)
        priv->fix_type = 1;
    else
        priv->fix_type = 0;

    callback_list_call_attr_0(priv->cbl, attr_position_coord_geo);
}

/**
 * @brief When the position has changed, this function get called
 *
 * The fields_set can hold:
 * GYPSY_POSITION_FIELDS_NONE = 1 << 0,
 * GYPSY_POSITION_FIELDS_LATITUDE = 1 << 1,
 * GYPSY_POSITION_FIELDS_LONGITUDE = 1 << 2,
 * GYPSY_POSITION_FIELDS_ALTITUDE = 1 << 3
 *
 * If we get any new information, we have to call the global
 * callback.
 *
 * @param position The GypsyPosition
 * @param fields_set Bitmask indicating what field was set
 * @param timestamp the time since Unix Epoch
 * @param latitude
 * @param longitude
 * @param altitude
 * @param userdata
 * @returns nothing
 */
static void vehicle_gypsy_position_changed(GypsyPosition *position,
        GypsyPositionFields fields_set, int timestamp,
        double latitude, double longitude, double altitude,
        gpointer userdata) {
    struct vehicle_priv *priv = vehicle_last;
    int cb = FALSE;

    if (timestamp > 0)
        priv->fix_time = timestamp;

    if (fields_set & GYPSY_POSITION_FIELDS_LATITUDE) {
        cb = TRUE;
        priv->geo.lat = latitude;
    }
    if (fields_set & GYPSY_POSITION_FIELDS_LONGITUDE) {
        cb = TRUE;
        priv->geo.lng = longitude;
    }
    if (fields_set & GYPSY_POSITION_FIELDS_ALTITUDE) {
        cb = TRUE;
        priv->height = altitude;
    }

    if (cb) {
        priv->have_cords = 1;
        callback_list_call_attr_0(priv->cbl, attr_position_coord_geo);
    }
}

/**
 * @brief Everytime any Sat-Details are changed, this function get called
 *
 * Going through all Sats, counting those wich are in use.
 *
 * Anytime this functions get called, we have to call the global
 * callback.
 *
 * @param satellite The GypsySatellite
 * @param satellites An GPtrArray wich hold information of all sats
 * @param userdata
 * @returns nothing
 */
static void vehicle_gypsy_satellite_changed(GypsySatellite *satellite,
        GPtrArray *satellites,
        gpointer userdata) {
    struct vehicle_priv *priv = vehicle_last;

    int i, sats, used=0;

    sats = satellites->len;
    for (i = 0; i < sats; i++) {
        GypsySatelliteDetails *details = satellites->pdata[i];
        if (details->in_use)
            used++;
    }

    priv->sats_used = used;
    priv->sats = sats;

    callback_list_call_attr_0(priv->cbl, attr_position_coord_geo);
}

/**
 * @brief When the course or speed has changed, this function get called
 *
 * Only speed and direction are used!
 *
 * If we get any new information, we have to call the global
 * callback.
 *
 * @param course The GypsyCourse
 * @param fields Bitmask indicating what field was set
 * @param timestamp the time since Unix Epoch
 * @param speed
 * @param direction
 * @param climb
 * @param userdata
 * @returns nothing
 */
static void vehicle_gypsy_course_changed (GypsyCourse *course,
        GypsyCourseFields fields,
        int timestamp,
        double speed,
        double direction,
        double climb,
        gpointer userdata) {
    struct vehicle_priv *priv = vehicle_last;
    int cb = FALSE;

    if (fields & GYPSY_COURSE_FIELDS_SPEED) {
        priv->speed = speed*3.6;
        cb = TRUE;
    }
    if (fields & GYPSY_COURSE_FIELDS_DIRECTION) {
        priv->direction = direction;
        cb = TRUE;
    }

    if (cb)
        callback_list_call_attr_0(priv->cbl, attr_position_coord_geo);
}

/**
 * @brief Attempt to open the gypsy device.
 *
 * Tells gypsy wich functions to call when anything occours.
 *
 * @param data
 * @returns TRUE to try again; FALSE if retry not required
 */
static gboolean vehicle_gypsy_try_open(gpointer *data) {
    struct vehicle_priv *priv = (struct vehicle_priv *)data;
    char *source = g_strdup(priv->source);

    GError *error = NULL;

    g_type_init();
    priv->control = gypsy_control_get_default();
    priv->path = gypsy_control_create(priv->control, source+8, &error);
    if (priv->path == NULL) {
        g_warning ("Error creating gypsy conrtol path for %s: %s", source+8, error->message);
        return TRUE;
    }

    priv->position = gypsy_position_new(priv->path);
    g_signal_connect(priv->position, "position-changed", G_CALLBACK (vehicle_gypsy_position_changed), NULL);

    priv->satellite = gypsy_satellite_new(priv->path);
    g_signal_connect(priv->satellite, "satellites-changed", G_CALLBACK (vehicle_gypsy_satellite_changed), NULL);

    priv->course = gypsy_course_new(priv->path);
    g_signal_connect(priv->course, "course-changed", G_CALLBACK (vehicle_gypsy_course_changed), NULL);

    priv->device = gypsy_device_new(priv->path);
    g_signal_connect(priv->device, "fix-status-changed", G_CALLBACK (vehicle_gypsy_fixstatus_changed), NULL);

    gypsy_device_start(priv->device, &error);
    if (error != NULL) {
        g_warning ("Error starting gypsy for %s: %s", source+8, error->message);
        return TRUE;
    }

    vehicle_last = priv;
    dbg(lvl_debug,"gypsy connected to %d", source+8);
    g_free(source);
    return FALSE;
}

/**
 * @brief Open a connection to gypsy. Will re-try the connection if it fails
 *
 * @param priv
 * @returns nothing
 */
static void vehicle_gypsy_open(struct vehicle_priv *priv) {
    priv->retry_timer=0;
    if (vehicle_gypsy_try_open((gpointer *)priv)) {
        priv->retry_timer = g_timeout_add(priv->retry_interval*1000, (GSourceFunc)vehicle_gypsy_try_open, (gpointer *)priv);
    }
}

/**
 * @brief Stop retry timer; Free alloced memory
 *
 * @param priv
 * @returns nothing
 */
static void vehicle_gypsy_close(struct vehicle_priv *priv) {
    if (priv->retry_timer) {
        g_source_remove(priv->retry_timer);
        priv->retry_timer=0;
    }
    if (priv->path)
        g_free(priv->path);

    if (priv->position)
        g_free(priv->position);

    if (priv->satellite)
        g_free(priv->satellite);

    if (priv->course)
        g_free(priv->course);

    if (priv->device)
        g_free(priv->device);

    if (priv->control)
        g_object_unref(G_OBJECT (priv->control));
}

/**
 * @brief Free the gypsy_vehicle
 *
 * @param priv
 * @returns nothing
 */
static void vehicle_gypsy_destroy(struct vehicle_priv *priv) {
    vehicle_gypsy_close(priv);
    if (priv->source)
        g_free(priv->source);
    g_free(priv);
}

/**
 * @brief Provide the outside with information
 *
 * @param priv
 * @param type TODO: What can this be?
 * @param attr
 * @returns true/false
 */
static int vehicle_gypsy_position_attr_get(struct vehicle_priv *priv,
        enum attr_type type, struct attr *attr) {
    struct attr * active=NULL;
    switch (type) {
    case attr_position_fix_type:
        attr->u.num = priv->fix_type;
        break;
    case attr_position_height:
        attr->u.numd = &priv->height;
        break;
    case attr_position_speed:
        attr->u.numd = &priv->speed;
        break;
    case attr_position_direction:
        attr->u.numd = &priv->direction;
        break;
    case attr_position_qual:
        attr->u.num = priv->sats;
        break;
    case attr_position_sats_used:
        attr->u.num = priv->sats_used;
        break;
    case attr_position_coord_geo:
        attr->u.coord_geo = &priv->geo;
        if (!priv->have_cords)
            return 0;
        break;
    case attr_position_time_iso8601: {
        struct tm tm;
        if (!priv->fix_time)
            return 0;
        if (gmtime_r(&priv->fix_time, &tm)) {
            strftime(priv->fixiso8601, sizeof(priv->fixiso8601),
                     "%Y-%m-%dT%TZ", &tm);
            attr->u.str=priv->fixiso8601;
        } else
            return 0;
    }
    case attr_active:
        active = attr_search(priv->attrs,attr_active);
        if(active != NULL && active->u.num == 1)
            return 1;
        else
            return 0;
        break;

    default:
        return 0;
    }
    attr->type = type;
    return 1;
}

struct vehicle_methods vehicle_gypsy_methods = {
    vehicle_gypsy_destroy,
    vehicle_gypsy_position_attr_get,
};

/**
 * @brief Create gypsy_vehicle
 *
 * @param meth
 * @param cbl
 * @param attrs
 * @returns vehicle_priv
 */
static struct vehicle_priv *vehicle_gypsy_new_gypsy(struct vehicle_methods *meth,
        struct callback_list *cbl,
        struct attr **attrs) {
    struct vehicle_priv *ret;
    struct attr *source, *retry_int;

#if defined(USE_BINDING_DBUS) && defined(HAVE_UNISTD_H)
    DBusConnection *conn;
    DBusMessage *message;
    dbus_uint32_t serial,pid=getpid();
    struct attr *destination,*path,*interface,*method;

    destination=attr_search(attrs, attr_dbus_destination);
    path=attr_search(attrs, attr_dbus_path);
    interface=attr_search(attrs, attr_dbus_interface);
    method=attr_search(attrs, attr_dbus_method);
    if (destination && path && interface && method) {
        conn=dbus_bus_get(DBUS_BUS_SESSION, NULL);
        if (conn) {
            message=dbus_message_new_method_call(destination->u.str,path->u.str,interface->u.str,method->u.str);
            dbus_message_append_args(message, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INVALID);
            dbus_connection_send(conn, message, &serial);
            dbus_message_unref(message);
            dbus_connection_unref(conn);
        } else {
            dbg(lvl_error,"failed to connect to session bus");
        }
    }
#endif
    dbg(lvl_debug, "enter");
    source = attr_search(attrs, attr_source);
    ret = g_new0(struct vehicle_priv, 1);
    ret->have_cords = 0;
    ret->source = g_strdup(source->u.str);
    ret->attrs = attrs;
    retry_int = attr_search(attrs, attr_retry_interval);
    if (retry_int) {
        ret->retry_interval = retry_int->u.num;
        if (ret->retry_interval < MIN_RETRY_INTERVAL) {
            dbg(lvl_error, "Retry interval %d too small, setting to %d", ret->retry_interval, MIN_RETRY_INTERVAL);
            ret->retry_interval = MIN_RETRY_INTERVAL;
        }
    } else {
        dbg(lvl_error, "Retry interval not defined, setting to %d", DEFAULT_RETRY_INTERVAL);
        ret->retry_interval = DEFAULT_RETRY_INTERVAL;
    }
    ret->cbl = cbl;
    *meth = vehicle_gypsy_methods;
    vehicle_gypsy_open(ret);
    return ret;
}

/**
 * @brief register vehicle_gypsy
 *
 * @returns nothing
 */
void plugin_init(void) {
    dbg(lvl_debug, "enter");
    plugin_register_category_vehicle("gypsy", vehicle_gypsy_new_gypsy);
}
