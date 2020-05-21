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

#include <glib.h>
#include <string.h>
#include "config.h"
#include "debug.h"
#include "coord.h"
#include "item.h"
#include "navit.h"
#include "map.h"
#include "route.h"
#include "callback.h"
#include "transform.h"
#include "plugin.h"
#include "vehicle.h"
#include "event.h"
#include "corelocation.h"

/**
 * @defgroup vehicle-iphone Vehicle iPhone
 * @ingroup vehicle-plugins
 * @brief The Vehicle to gain position data from iPhone.
 *
 * @{
 */

struct vehicle_priv {
    int interval;
    int position_set;
    struct callback_list *cbl;
    struct navit *navit;
    struct coord_geo geo;
    struct coord last;
    double config_speed;
    double speed;
    double direction;
    double radius;
    struct callback *timer_callback;
    struct event_timeout *timer;
    char str_time[200];
};

static void vehicle_iphone_destroy(struct vehicle_priv *priv) {
    corelocation_exit();
    g_free(priv);
}

static int vehicle_iphone_position_attr_get(struct vehicle_priv *priv,
        enum attr_type type, struct attr *attr) {
    switch (type) {
    case attr_position_speed:
        attr->u.numd = &priv->speed;
        break;
    case attr_position_direction:
        attr->u.numd = &priv->direction;
        break;
    case attr_position_coord_geo:
        attr->u.coord_geo = &priv->geo;
        break;
    case attr_position_time_iso8601:
        attr->u.str = priv->str_time;
        break;
    case attr_position_radius:
        attr->u.numd = &priv->radius;
        break;
    case attr_position_nmea:
        return 0;
    default:
        return 0;
    }
    attr->type = type;
    return 1;
}

static int vehicle_iphone_set_attr(struct vehicle_priv *priv, struct attr *attr) {
    if (attr->type == attr_navit) {
        priv->navit = attr->u.navit;
        return 1;
    }
    return 0;
}

struct vehicle_methods vehicle_iphone_methods = {
    vehicle_iphone_destroy,
    vehicle_iphone_position_attr_get,
    vehicle_iphone_set_attr,
};

void vehicle_iphone_update(void *arg,
                           double lat,
                           double lng,
                           double dir,
                           double spd,
                           char * str_time,
                           double radius
                          ) {
    struct vehicle_priv * priv = arg;
    priv->geo.lat = lat;
    priv->geo.lng = lng;
    if(dir > 0) priv->direction = dir;
    if(spd > 0) priv->speed = spd*3.6;
    strcpy(priv->str_time, str_time);
    priv->radius = radius;

    dbg(lvl_debug,"position_get lat:%f lng:%f (spd:%f dir:%f time:%s)", priv->geo.lat, priv->geo.lng, priv->speed,
        priv->direction, priv->str_time);
    callback_list_call_attr_0(priv->cbl, attr_position_coord_geo);
}



static struct vehicle_priv *vehicle_iphone_new(struct vehicle_methods
        *meth, struct callback_list
        *cbl, struct attr **attrs) {
    struct vehicle_priv *ret;
    struct attr *interval,*speed,*position_coord_geo;

    dbg(lvl_debug, "enter");
    ret = g_new0(struct vehicle_priv, 1);
    ret->cbl = cbl;
    ret->interval=1000;
    ret->config_speed=40;
    if ((speed=attr_search(attrs, attr_speed))) {
        ret->config_speed=speed->u.num;
    }
    if ((interval=attr_search(attrs, attr_interval)))
        ret->interval=interval->u.num;
    if ((position_coord_geo=attr_search(attrs, attr_position_coord_geo))) {
        ret->geo=*(position_coord_geo->u.coord_geo);
        ret->position_set=1;
        dbg(lvl_debug,"position_set %f %f", ret->geo.lat, ret->geo.lng);
    }
    *meth = vehicle_iphone_methods;
    ret->str_time[0] = '\0';

    /** Initialize corelocation */
    corelocation_init(ret, vehicle_iphone_update);

    return ret;
}

void plugin_init(void) {
    dbg(lvl_debug, "enter");
    plugin_register_category_vehicle("iphone", vehicle_iphone_new);
}
