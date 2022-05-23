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
#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <errno.h>
#include "debug.h"
#include "callback.h"
#include "plugin.h"
#include "coord.h"
#include "item.h"
#include "vehicle.h"
#include "event.h"

/**
 * @defgroup vehicle-gpsd-dbus Vehicle Gpsd DBus
 * @ingroup vehicle-plugins
 * @brief The Vehicle to gain position data from Gpsd over DBus
 *
 * @{
 */

static char *vehicle_gpsd_dbus_prefix="gpsd_dbus:";

struct vehicle_priv {
    char *source;
    char *address;
    int flags;
    struct callback_list *cbl;
    DBusConnection *connection;
    double time, track, speed, altitude;
    time_t fix_time;
    struct coord_geo geo;
    struct attr ** attrs;
    char fixiso8601[128];
};

static void vehicle_gpsd_dbus_close(struct vehicle_priv *priv) {
    if (priv->connection)
        dbus_connection_unref(priv->connection);
    priv->connection=NULL;
}

static DBusHandlerResult vehicle_gpsd_dbus_filter(DBusConnection *connection, DBusMessage *message, void *user_data) {
    struct vehicle_priv *priv=user_data;
    double time,ept,latitude,longitude,eph,altitude,epv,track,epd,speed,eps,climb,epc;
    int mode;
    char *devname;

    if (dbus_message_is_signal(message, "org.gpsd","fix")) {
        dbus_message_get_args (message, NULL,
                               DBUS_TYPE_DOUBLE, &time,
                               DBUS_TYPE_INT32,  &mode,
                               DBUS_TYPE_DOUBLE, &ept,
                               DBUS_TYPE_DOUBLE, &latitude,
                               DBUS_TYPE_DOUBLE, &longitude,
                               DBUS_TYPE_DOUBLE, &eph,
                               DBUS_TYPE_DOUBLE, &altitude,
                               DBUS_TYPE_DOUBLE, &epv,
                               DBUS_TYPE_DOUBLE, &track,
                               DBUS_TYPE_DOUBLE, &epd,
                               DBUS_TYPE_DOUBLE, &speed,
                               DBUS_TYPE_DOUBLE, &eps,
                               DBUS_TYPE_DOUBLE, &climb,
                               DBUS_TYPE_DOUBLE, &epc,
                               DBUS_TYPE_STRING, &devname,
                               DBUS_TYPE_INVALID);
        if (!isnan(latitude) && !isnan(longitude)) {
            priv->geo.lat=latitude;
            priv->geo.lng=longitude;
        }
        if (!isnan(track))
            priv->track=track;
        if (!isnan(speed))
            priv->speed=speed;
        if (!isnan(altitude))
            priv->altitude=altitude;
        if (time != priv->time || (priv->flags & 1)) {
            priv->time=time;
            priv->fix_time=time;
            callback_list_call_attr_0(priv->cbl, attr_position_coord_geo);
        }
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static int vehicle_gpsd_dbus_open(struct vehicle_priv *priv) {
    DBusError error;

    dbus_error_init(&error);
    if (priv->address) {
        priv->connection = dbus_connection_open(priv->address, &error);
    } else {
        priv->connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
    }
    if (!priv->connection) {
        dbg(lvl_error,"Failed to open connection to %s message bus: %s", priv->address?priv->address:"session",error.message);
        dbus_error_free(&error);
        return 0;
    }
    dbus_connection_setup_with_g_main(priv->connection, NULL);
    dbus_bus_add_match(priv->connection,"type='signal',interface='org.gpsd'",&error);
    dbus_connection_flush(priv->connection);
    if (dbus_error_is_set(&error)) {
        dbg(lvl_error,"Failed to add match to connection: %s", error.message);
        vehicle_gpsd_dbus_close(priv);
        return 0;
    }
    if (!dbus_connection_add_filter(priv->connection, vehicle_gpsd_dbus_filter, priv, NULL)) {
        dbg(lvl_error,"Failed to add filter to connection");
        vehicle_gpsd_dbus_close(priv);
        return 0;
    }
    return 1;
}


static void vehicle_gpsd_dbus_destroy(struct vehicle_priv *priv) {
    vehicle_gpsd_dbus_close(priv);
    if (priv->source)
        g_free(priv->source);
    g_free(priv);
}

static int vehicle_gpsd_dbus_position_attr_get(struct vehicle_priv *priv,
        enum attr_type type, struct attr *attr) {
    switch (type) {
    case attr_position_height:
        attr->u.numd = &priv->altitude;
        break;
    case attr_position_speed:
        attr->u.numd = &priv->speed;
        break;
    case attr_position_direction:
        attr->u.numd = &priv->track;
        break;
    case attr_position_coord_geo:
        attr->u.coord_geo = &priv->geo;
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
    break;
    default:
        return 0;
    }
    attr->type = type;
    return 1;
}

static int vehicle_gpsd_dbus_set_attr_do(struct vehicle_priv *priv, struct attr *attr, int init) {
    switch (attr->type) {
    case attr_source:
        if (strncmp(vehicle_gpsd_dbus_prefix,attr->u.str,strlen(vehicle_gpsd_dbus_prefix))) {
            dbg(lvl_error,"source must start with '%s'", vehicle_gpsd_dbus_prefix);
            return 0;
        }
        g_free(priv->source);
        priv->source=g_strdup(attr->u.str);
        priv->address=priv->source+strlen(vehicle_gpsd_dbus_prefix);
        if (!priv->address[0])
            priv->address=NULL;
        if (!init) {
            vehicle_gpsd_dbus_close(priv);
            vehicle_gpsd_dbus_open(priv);
        }
        return 1;
    case attr_flags:
        priv->flags=attr->u.num;
        return 1;
    default:
        return 0;
    }
}

static int vehicle_gpsd_dbus_set_attr(struct vehicle_priv *priv, struct attr *attr) {
    return vehicle_gpsd_dbus_set_attr_do(priv, attr, 0);
}

static struct vehicle_methods vehicle_gpsd_methods = {
    vehicle_gpsd_dbus_destroy,
    vehicle_gpsd_dbus_position_attr_get,
    vehicle_gpsd_dbus_set_attr,
};

static struct vehicle_priv *vehicle_gpsd_dbus_new(struct vehicle_methods
        *meth, struct callback_list
        *cbl, struct attr **attrs) {
    struct vehicle_priv *ret;


    ret = g_new0(struct vehicle_priv, 1);
    ret->attrs = attrs;
    ret->cbl = cbl;
    *meth = vehicle_gpsd_methods;
    while (*attrs) {
        vehicle_gpsd_dbus_set_attr_do(ret, *attrs, 1);
        attrs++;
    }
    vehicle_gpsd_dbus_open(ret);
    return ret;
}

void plugin_init(void) {
    dbg(lvl_debug, "enter");
    plugin_register_category_vehicle("gpsd_dbus", vehicle_gpsd_dbus_new);
}
