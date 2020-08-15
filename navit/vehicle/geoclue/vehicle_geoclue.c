/*
 * Navit, a modular navigation system.
 * Copyright 2020 Navit Team
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

#include <stdlib.h>
#include <locale.h>
#include <glib/gi18n.h>
#include <geoclue.h>
#include "debug.h"
#include "callback.h"
#include "coord.h"
#include "item.h"
#include "vehicle.h"
#include "plugin.h"

/**
 * @defgroup vehicle-gypsy Vehicle GeoClue
 * @ingroup vehicle-plugins
 * @brief The Vehicle to gain position data from GeoClue.
 * @Author jkoan
 * @date 2020
 *
 * @{
 */


struct vehicle_priv {
    GClueLocation *location;
    char *bla;
    struct callback_list *cbl;
    double speed;
    double direction;
    double height;
    struct coord_geo geo;
    int accuracy;
    char* time_str;
    GClueSimple *simple;
};
GClueClient *client = NULL;

/**
 * @brief Free the geoclue_vehicle
 *
 * @param priv
 * @returns nothing
 */
static void vehicle_geoclue_destroy(struct vehicle_priv *priv) {
    g_clear_object(&client);
    g_clear_object(&priv->simple);
}

static void print_location(GClueSimple *simple,
                           GParamSpec *pspec,
                           gpointer    user_data) {
    GClueLocation *location;
    gdouble altitude;
    gdouble speed;
    gdouble direction;
    GVariant *timestamp;
    struct vehicle_priv *priv = user_data;

    location = gclue_simple_get_location(simple);

    priv->geo.lat = gclue_location_get_latitude(location);
    priv->geo.lng = gclue_location_get_longitude(location);
    priv->accuracy = gclue_location_get_accuracy(location);
    altitude = gclue_location_get_altitude(location);
    if(altitude != -G_MAXDOUBLE) {
        priv->height=altitude;
    }
    speed = gclue_location_get_speed(location);
    if(speed != -G_MAXDOUBLE) {
        priv->speed=speed;
    }
    direction = gclue_location_get_heading(location);
    if(direction != -G_MAXDOUBLE) {
        priv->direction=direction;
    }

    timestamp= gclue_location_get_timestamp(location);
    if(timestamp) {
        GDateTime *date_time;
        glong second_since_epoch;


        g_variant_get (timestamp, "(tt)", &second_since_epoch, NULL);

        date_time = g_date_time_new_from_unix_local (second_since_epoch);

        priv->time_str = g_date_time_format_iso8601(g_date_time_to_utc(date_time));

    }
    callback_list_call_attr_0(priv->cbl, attr_position_coord_geo);
}

static void on_client_active_notify(GClueClient *client,
                                    GParamSpec *pspec,
                                    gpointer    user_data) {
    if(gclue_client_get_active(client))
        return;

    g_print("Geolocation disabled. Quitting..\n");
    //vehicle_geoclue_destroy(&user_data);
}

static void on_simple_ready(GObject *source_object,
                            GAsyncResult *res,
                            gpointer user_data) {
    GError *error = NULL;

    struct vehicle_priv* priv = user_data;

    priv->simple = gclue_simple_new_finish(res, &error);
    if(error != NULL) {
        dbg(lvl_error,"Failed to connect to GeoClue2 service: %s", error->message);

        exit(-1);
    }
    client = gclue_simple_get_client(priv->simple);
    if(client) {
        g_object_ref(client);
        dbg(lvl_debug,"Client object: %s\n",
            g_dbus_proxy_get_object_path(G_DBUS_PROXY(client)));

        g_signal_connect(client,
                         "notify::active",
                         G_CALLBACK(on_client_active_notify),
                         NULL);
    }
    print_location(priv->simple,NULL,priv);

    g_signal_connect(priv->simple,
                     "notify::location",
                     G_CALLBACK(print_location),
                     priv);
}

/**
 * @brief Provide the outside with information
 *
 * @param priv
 * @param type TODO: What can this be?
 * @param attr
 * @returns true/false
 */
static int vehicle_geoclue_position_attr_get(struct vehicle_priv *priv,
        enum attr_type type, struct attr *attr) {
    switch(type) {
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
        attr->u.num = priv->accuracy;
        break;
    case attr_position_coord_geo:
        attr->u.coord_geo = &priv->geo;
        break;
    case attr_position_time_iso8601:
        if(!priv->time_str) {
            return 0;
        }
        attr->u.str=priv->time_str;
        break;
    case attr_active:
        return 1;
    default:
        return 0;
    }
    attr->type = type;
    return 1;
}

struct vehicle_methods vehicle_geoclue_methods = {
    .destroy           = vehicle_geoclue_destroy,
    .position_attr_get = vehicle_geoclue_position_attr_get,
};

/**
 * @brief Create geoclue_vehicle
 *
 * @param meth[out] Methodes supported by geoclue Plugin
 * @param cbl[in] Callback List reference
 * @param attrs Configuration attributes
 * @returns vehicle_priv The newly created Vehicle priv
 */
static struct vehicle_priv *vehicle_geoclue_new(struct vehicle_methods *meth,
        struct callback_list *cbl,
        struct attr **attrs) {
    struct vehicle_priv *ret;
    dbg(lvl_debug, "enter");

    *meth = vehicle_geoclue_methods;

    ret = (struct vehicle_priv*)g_new0(struct vehicle_priv, 1);
    ret->cbl = cbl;
    gclue_simple_new("navit",
                     GCLUE_ACCURACY_LEVEL_EXACT,
                     NULL,
                     on_simple_ready,
                     ret
                    );
    return ret;
};

/**
 * @brief register vehicle_geoclue
 *
 * @returns nothing
 */
void plugin_init(void) {
    dbg(lvl_error, "enter");
    plugin_register_category_vehicle("geoclue", vehicle_geoclue_new);
};

