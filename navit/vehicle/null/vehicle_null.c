/*
 * @brief null uses dbus signals
 *
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
 * @Author Tim Niemeyer <reddog@mastersword.de>
 * @date 2008-2009
 */

#include <config.h>
#include <string.h>
#include <glib.h>
#include <math.h>
#include <time.h>
#include "debug.h"
#include "callback.h"
#include "plugin.h"
#include "coord.h"
#include "item.h"
#include "vehicle.h"

/**
 * @defgroup vehicle-null Vehicle Null
 * @ingroup vehicle-plugins
 * @brief A dummy Vehicle to have a null movement.
 *
 * @{
 */

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
};

/**
 * @brief Free the null_vehicle
 *
 * @param priv
 * @returns nothing
 */
static void vehicle_null_destroy(struct vehicle_priv *priv) {
    dbg(lvl_debug,"enter");
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
static int vehicle_null_position_attr_get(struct vehicle_priv *priv,
        enum attr_type type, struct attr *attr) {
    dbg(lvl_debug,"enter %s",attr_to_name(type));
    switch (type) {
    case attr_position_height:
        attr->u.numd = &priv->height;
        break;
    case attr_position_speed:
        attr->u.numd = &priv->speed;
        break;
    case attr_position_direction:
        attr->u.numd = &priv->direction;
        break;
    case attr_position_radius:
        attr->u.numd = &priv->radius;
        break;
    case attr_position_coord_geo:
        attr->u.coord_geo = &priv->geo;
        if (!priv->have_coords)
            return 0;
        break;
    case attr_position_time_iso8601:
        attr->u.str=priv->fixiso8601;
        break;
    default:
        return 0;
    }
    dbg(lvl_debug,"ok");
    attr->type = type;
    return 1;
}

static int vehicle_null_set_attr(struct vehicle_priv *priv, struct attr *attr) {
    switch (attr->type) {
    case attr_position_speed:
        priv->speed=*attr->u.numd;
        break;
    case attr_position_direction:
        priv->direction=*attr->u.numd;
        break;
    case attr_position_coord_geo:
        priv->geo=*attr->u.coord_geo;
        priv->have_coords=1;
        break;
    default:
        break;
    }
    callback_list_call_attr_0(priv->cbl, attr->type);
    return 1;
}


struct vehicle_methods vehicle_null_methods = {
    vehicle_null_destroy,
    vehicle_null_position_attr_get,
    vehicle_null_set_attr,
};

/**
 * @brief Create null_vehicle
 *
 * @param meth
 * @param cbl
 * @param attrs
 * @returns vehicle_priv
 */
static struct vehicle_priv *vehicle_null_new_null(struct vehicle_methods *meth,
        struct callback_list *cbl,
        struct attr **attrs) {
    struct vehicle_priv *ret;

    dbg(lvl_debug, "enter");
    ret = g_new0(struct vehicle_priv, 1);
    ret->cbl = cbl;
    *meth = vehicle_null_methods;
    dbg(lvl_debug, "return");
    return ret;
}

/**
 * @brief register vehicle_null
 *
 * @returns nothing
 */
void plugin_init(void) {
    dbg(lvl_debug, "enter");
    plugin_register_category_vehicle("null", vehicle_null_new_null);
}
