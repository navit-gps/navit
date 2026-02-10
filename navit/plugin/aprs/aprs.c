/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "aprs_db.h"
#include "aprs_decode.h"
#include "aprs_nmea.h"
#include "aprs_plugin_interface.h"
#include "aprs_symbols.h"
#include "attr.h"
#include "callback.h"
#include "config.h"
#include "coord.h"
#include "debug.h"
#include "event.h"
#include "item.h"
#include "map.h"
#include "mapset.h"
#include "menu.h"
#include "navit.h"
#include "plugin.h"
#include "popup.h"
#include "projection.h"
#include "transform.h"
#include "vehicle.h"
#include <glib.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

extern int aprs_cmd_freq_144_39(struct navit *nav, char *function, struct attr **in, struct attr ***out);
extern int aprs_cmd_freq_144_8(struct navit *nav, char *function, struct attr **in, struct attr ***out);
extern int aprs_cmd_freq_145_175(struct navit *nav, char *function, struct attr **in, struct attr ***out);
extern int aprs_cmd_freq_144_575(struct navit *nav, char *function, struct attr **in, struct attr ***out);
extern int aprs_cmd_freq_144_64(struct navit *nav, char *function, struct attr **in, struct attr ***out);
extern int aprs_cmd_freq_144_93(struct navit *nav, char *function, struct attr **in, struct attr ***out);
extern int aprs_cmd_freq_145_57(struct navit *nav, char *function, struct attr **in, struct attr ***out);
extern int aprs_cmd_timeout_30min(struct navit *nav, char *function, struct attr **in, struct attr ***out);
extern int aprs_cmd_timeout_60min(struct navit *nav, char *function, struct attr **in, struct attr ***out);
extern int aprs_cmd_timeout_90min(struct navit *nav, char *function, struct attr **in, struct attr ***out);
extern int aprs_cmd_timeout_120min(struct navit *nav, char *function, struct attr **in, struct attr ***out);
extern int aprs_cmd_timeout_180min(struct navit *nav, char *function, struct attr **in, struct attr ***out);
extern int aprs_cmd_timeout_clear_expired(struct navit *nav, char *function, struct attr **in, struct attr ***out);
extern int aprs_cmd_device_rtlsdr_blog_v3(struct navit *nav, char *function, struct attr **in, struct attr ***out);
extern int aprs_cmd_device_rtlsdr_v4_r828d(struct navit *nav, char *function, struct attr **in, struct attr ***out);
extern int aprs_cmd_device_rtlsdr_nooelec(struct navit *nav, char *function, struct attr **in, struct attr ***out);
extern int aprs_cmd_device_rtlsdr_generic(struct navit *nav, char *function, struct attr **in, struct attr ***out);
extern int aprs_cmd_device_nmea(struct navit *nav, char *function, struct attr **in, struct attr ***out);
extern int aprs_cmd_nmea_baud_4800(struct navit *nav, char *function, struct attr **in, struct attr ***out);
extern int aprs_cmd_nmea_baud_9600(struct navit *nav, char *function, struct attr **in, struct attr ***out);
extern int aprs_cmd_nmea_baud_19200(struct navit *nav, char *function, struct attr **in, struct attr ***out);
extern int aprs_cmd_nmea_baud_38400(struct navit *nav, char *function, struct attr **in, struct attr ***out);
extern int aprs_cmd_nmea_parity_none(struct navit *nav, char *function, struct attr **in, struct attr ***out);
extern int aprs_cmd_nmea_parity_even(struct navit *nav, char *function, struct attr **in, struct attr ***out);
extern int aprs_cmd_nmea_parity_odd(struct navit *nav, char *function, struct attr **in, struct attr ***out);

/* Wrapper functions for popup callbacks */
static void aprs_cmd_freq_wrapper_144_39(void *data) {
    struct navit *nav = (struct navit *)data;
    aprs_cmd_freq_144_39(nav, NULL, NULL, NULL);
}

static void aprs_cmd_freq_wrapper_144_8(void *data) {
    struct navit *nav = (struct navit *)data;
    aprs_cmd_freq_144_8(nav, NULL, NULL, NULL);
}

static void aprs_cmd_freq_wrapper_145_175(void *data) {
    struct navit *nav = (struct navit *)data;
    aprs_cmd_freq_145_175(nav, NULL, NULL, NULL);
}

static void aprs_cmd_freq_wrapper_144_575(void *data) {
    struct navit *nav = (struct navit *)data;
    aprs_cmd_freq_144_575(nav, NULL, NULL, NULL);
}

static void aprs_cmd_freq_wrapper_144_64(void *data) {
    struct navit *nav = (struct navit *)data;
    aprs_cmd_freq_144_64(nav, NULL, NULL, NULL);
}

static void aprs_cmd_freq_wrapper_144_93(void *data) {
    struct navit *nav = (struct navit *)data;
    aprs_cmd_freq_144_93(nav, NULL, NULL, NULL);
}

static void aprs_cmd_freq_wrapper_145_57(void *data) {
    struct navit *nav = (struct navit *)data;
    aprs_cmd_freq_145_57(nav, NULL, NULL, NULL);
}

static void aprs_cmd_timeout_wrapper_30min(void *data) {
    struct navit *nav = (struct navit *)data;
    aprs_cmd_timeout_30min(nav, NULL, NULL, NULL);
}

static void aprs_cmd_timeout_wrapper_60min(void *data) {
    struct navit *nav = (struct navit *)data;
    aprs_cmd_timeout_60min(nav, NULL, NULL, NULL);
}

static void aprs_cmd_timeout_wrapper_90min(void *data) {
    struct navit *nav = (struct navit *)data;
    aprs_cmd_timeout_90min(nav, NULL, NULL, NULL);
}

static void aprs_cmd_timeout_wrapper_120min(void *data) {
    struct navit *nav = (struct navit *)data;
    aprs_cmd_timeout_120min(nav, NULL, NULL, NULL);
}

static void aprs_cmd_timeout_wrapper_180min(void *data) {
    struct navit *nav = (struct navit *)data;
    aprs_cmd_timeout_180min(nav, NULL, NULL, NULL);
}

static void aprs_cmd_timeout_wrapper_clear(void *data) {
    struct navit *nav = (struct navit *)data;
    aprs_cmd_timeout_clear_expired(nav, NULL, NULL, NULL);
}

static void aprs_cmd_device_wrapper_rtlsdr_blog_v3(void *data) {
    struct navit *nav = (struct navit *)data;
    aprs_cmd_device_rtlsdr_blog_v3(nav, NULL, NULL, NULL);
}

static void aprs_cmd_device_wrapper_rtlsdr_v4_r828d(void *data) {
    struct navit *nav = (struct navit *)data;
    aprs_cmd_device_rtlsdr_v4_r828d(nav, NULL, NULL, NULL);
}

static void aprs_cmd_device_wrapper_rtlsdr_nooelec(void *data) {
    struct navit *nav = (struct navit *)data;
    aprs_cmd_device_rtlsdr_nooelec(nav, NULL, NULL, NULL);
}

static void aprs_cmd_device_wrapper_rtlsdr_generic(void *data) {
    struct navit *nav = (struct navit *)data;
    aprs_cmd_device_rtlsdr_generic(nav, NULL, NULL, NULL);
}

static void aprs_cmd_device_wrapper_nmea(void *data) {
    struct navit *nav = (struct navit *)data;
    aprs_cmd_device_nmea(nav, NULL, NULL, NULL);
}

static void aprs_cmd_nmea_baud_wrapper_4800(void *data) {
    struct navit *nav = (struct navit *)data;
    aprs_cmd_nmea_baud_4800(nav, NULL, NULL, NULL);
}

static void aprs_cmd_nmea_baud_wrapper_9600(void *data) {
    struct navit *nav = (struct navit *)data;
    aprs_cmd_nmea_baud_9600(nav, NULL, NULL, NULL);
}

static void aprs_cmd_nmea_baud_wrapper_19200(void *data) {
    struct navit *nav = (struct navit *)data;
    aprs_cmd_nmea_baud_19200(nav, NULL, NULL, NULL);
}

static void aprs_cmd_nmea_baud_wrapper_38400(void *data) {
    struct navit *nav = (struct navit *)data;
    aprs_cmd_nmea_baud_38400(nav, NULL, NULL, NULL);
}

static void aprs_cmd_nmea_parity_wrapper_none(void *data) {
    struct navit *nav = (struct navit *)data;
    aprs_cmd_nmea_parity_none(nav, NULL, NULL, NULL);
}

static void aprs_cmd_nmea_parity_wrapper_even(void *data) {
    struct navit *nav = (struct navit *)data;
    aprs_cmd_nmea_parity_even(nav, NULL, NULL, NULL);
}

static void aprs_cmd_nmea_parity_wrapper_odd(void *data) {
    struct navit *nav = (struct navit *)data;
    aprs_cmd_nmea_parity_odd(nav, NULL, NULL, NULL);
}

struct aprs_item_priv {
    struct map_rect_priv *mr;
    struct aprs_station *station;
    struct coord coord;
    struct attr **attrs;
    int next_attr;
    int next_coord;
    int refcount;
};

struct map_rect_priv {
    struct map_priv *mpriv;
    GList *next_item;
    struct item *item;
};

enum aprs_input_type {
    APRS_INPUT_NONE = 0,
    APRS_INPUT_NMEA,
    APRS_INPUT_SDR_PLUGIN /* External SDR plugin */
};

struct map_priv {
    struct aprs_db *db;
    GList *items;
    struct callback_list *cbl;
    time_t expire_seconds;
    double range_km;
    struct coord_geo center;
    int has_center;
    GHashTable *station_hash;
    struct event_timeout *expire_timeout;
    struct event_timeout *update_timeout;
    struct navit *navit;
    struct callback *position_callback;
    enum aprs_input_type input_type;
    struct aprs_nmea *nmea;
    int nmea_enabled;

    /* Packet source callbacks (for SDR plugin) */
    GList *packet_sources;
};

/* Forward declarations */
static void aprs_update_items(struct map_priv *priv);
static int aprs_process_packet(struct map_priv *priv, const unsigned char *data, int length);

/* Global list of APRS map instances for inter-plugin communication */
static GList *aprs_map_instances = NULL;

/* NMEA callback function */
static void aprs_nmea_callback(void *data, struct aprs_station *station) {
    struct map_priv *priv = (struct map_priv *)data;
    if (!priv || !station)
        return;

    aprs_db_update_station(priv->db, station);
    aprs_update_items(priv);
}

static struct map_methods aprs_map_meth;

static void aprs_map_destroy(struct map_priv *priv);
static struct map_rect_priv *aprs_map_rect_new(struct map_priv *priv, struct map_selection *sel);
static void aprs_map_rect_destroy(struct map_rect_priv *mr);
static struct item *aprs_map_get_item(struct map_rect_priv *mr);
static struct item *aprs_map_get_item_byid(struct map_rect_priv *mr, int id_hi, int id_lo);
static int aprs_map_get_attr(struct map_priv *priv, enum attr_type type, struct attr *attr);

static void aprs_item_coord_rewind(void *priv_data);
static int aprs_item_coord_get(void *priv_data, struct coord *c, int count);
static void aprs_item_attr_rewind(void *priv_data);
static int aprs_item_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr);
static struct item *aprs_item_ref(struct item *item);
static struct item *aprs_item_unref(struct item *item);

static struct item_methods aprs_item_methods = {
    aprs_item_coord_rewind,
    aprs_item_coord_get,
    aprs_item_attr_rewind,
    aprs_item_attr_get,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

static void aprs_item_coord_rewind(void *priv_data) {
    struct aprs_item_priv *ip = (struct aprs_item_priv *)priv_data;
    ip->next_coord = 0;
}

static int aprs_item_coord_get(void *priv_data, struct coord *c, int count) {
    struct aprs_item_priv *ip = (struct aprs_item_priv *)priv_data;

    if (count < 1 || ip->next_coord >= 1) {
        return 0;
    }

    c[0] = ip->coord;
    ip->next_coord = 1;
    return 1;
}

static void aprs_item_attr_rewind(void *priv_data) {
    struct aprs_item_priv *ip = (struct aprs_item_priv *)priv_data;
    ip->next_attr = 0;
}

static int aprs_item_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr) {
    struct aprs_item_priv *ip = (struct aprs_item_priv *)priv_data;

    if (!ip->attrs)
        return 0;

    while (ip->attrs[ip->next_attr]) {
        if (ip->attrs[ip->next_attr]->type == attr_type) {
            *attr = *(ip->attrs[ip->next_attr]);
            ip->next_attr++;
            return 1;
        }
        ip->next_attr++;
    }

    return 0;
}

static struct item *aprs_item_ref(struct item *item) {
    struct aprs_item_priv *ip = (struct aprs_item_priv *)item->priv_data;
    ip->refcount++;
    return item;
}

static struct item *aprs_item_unref(struct item *item) {
    struct aprs_item_priv *ip = (struct aprs_item_priv *)item->priv_data;
    ip->refcount--;
    if (ip->refcount <= 0) {
        if (ip->attrs) {
            attr_list_free(ip->attrs);
        }
        if (ip->station) {
            aprs_station_free(ip->station);
        }
        g_free(ip);
    }
    return item;
}

static void aprs_item_destroy(struct item *item) {
    struct aprs_item_priv *ip = (struct aprs_item_priv *)item->priv_data;
    if (ip->mr) {
        struct map_priv *mpriv = ip->mr->mpriv;
        mpriv->items = g_list_remove(mpriv->items, item);
    }
    aprs_item_unref(item);
}

static void aprs_update_items(struct map_priv *priv) {
    GList *l;

    for (l = priv->items; l; l = g_list_next(l)) {
        struct item *item = (struct item *)l->data;
        aprs_item_destroy(item);
    }
    g_list_free(priv->items);
    priv->items = NULL;
    g_hash_table_remove_all(priv->station_hash);

    GList *stations = NULL;
    if (priv->has_center) {
        aprs_db_get_stations_in_range(priv->db, &priv->center, 150.0, &stations);
    } else {
        aprs_db_get_all_stations(priv->db, &stations);
    }

    int id_hi = 1;
    int id_lo = 0;

    for (l = stations; l; l = g_list_next(l)) {
        struct aprs_station *station = (struct aprs_station *)l->data;

        if (time(NULL) - station->timestamp > priv->expire_seconds) {
            continue;
        }

        struct item *new_item = g_new0(struct item, 1);
        struct aprs_item_priv *ip = g_new0(struct aprs_item_priv, 1);

        new_item->type = type_poi_custom0; /* Use custom POI type to support icon_src attribute */
        new_item->id_hi = id_hi;
        new_item->id_lo = id_lo++;
        new_item->meth = &aprs_item_methods;
        new_item->priv_data = ip;

        enum projection pro = projection_mg;
        transform_from_geo(pro, &station->position, &ip->coord);

        ip->station = station;
        ip->refcount = 1;

        struct attr *attrs[16];
        int attr_idx = 0;

        if (station->callsign) {
            attrs[attr_idx] = g_new0(struct attr, 1);
            attrs[attr_idx]->type = attr_label;
            attrs[attr_idx]->u.str = g_strdup(station->callsign);
            attr_idx++;
        }

        char timestamp_str[64];
        const struct tm *tm_info = gmtime(&station->timestamp);
        strftime(timestamp_str, sizeof(timestamp_str), "%Y-%m-%d %H:%M:%S UTC", tm_info);
        attrs[attr_idx] = g_new0(struct attr, 1);
        attrs[attr_idx]->type = attr_data;
        attrs[attr_idx]->u.str = g_strdup(timestamp_str);
        attr_idx++;

        if (station->speed >= 0) {
            attrs[attr_idx] = g_new0(struct attr, 1);
            attrs[attr_idx]->type = attr_speed;
            attrs[attr_idx]->u.num = (int)station->speed;
            attr_idx++;
        }

        if (station->course >= 0) {
            attrs[attr_idx] = g_new0(struct attr, 1);
            attrs[attr_idx]->type = attr_direction;
            attrs[attr_idx]->u.num = (int)station->course;
            attr_idx++;
        }

        /* Set APRS symbol icon if available */
        if (station->symbol_table && station->symbol_code) {
            char *icon_path = aprs_symbol_get_icon(station->symbol_table, station->symbol_code);
            if (icon_path) {
                attrs[attr_idx] = g_new0(struct attr, 1);
                attrs[attr_idx]->type = attr_icon_src;
                attrs[attr_idx]->u.str = icon_path;
                attr_idx++;
            }
        }

        attrs[attr_idx] = NULL;
        ip->attrs = attr_list_dup(attrs);
        attr_list_free(attrs);

        priv->items = g_list_append(priv->items, new_item);
        g_hash_table_insert(priv->station_hash, g_strdup(station->callsign), new_item);
    }

    g_list_free(stations);

    struct attr attr;
    attr.type = attr_active;
    attr.u.num = 1;
    callback_list_call_attr_2(priv->cbl, attr_active, NULL, &attr);
}

static void aprs_expire_callback(void *data) {
    struct map_priv *priv = (struct map_priv *)data;
    if (!priv || !priv->db)
        return;
    int deleted = aprs_db_delete_expired(priv->db, priv->expire_seconds);
    if (deleted > 0) {
        dbg(lvl_info, "Expired %d APRS stations (timeout: %ld minutes)", deleted, (long)(priv->expire_seconds / 60));
        aprs_update_items(priv);
    }
}

static void aprs_position_update_callback(void *data) {
    struct map_priv *priv = (struct map_priv *)data;
    struct attr vehicle_attr, position_attr;

    if (!priv || !priv->navit)
        return;

    if (navit_get_attr(priv->navit, attr_vehicle, &vehicle_attr, NULL) && vehicle_attr.u.vehicle) {
        if (vehicle_get_attr(vehicle_attr.u.vehicle, attr_position_coord_geo, &position_attr, NULL)) {
            priv->center = *position_attr.u.coord_geo;
            priv->has_center = 1;
            dbg(lvl_debug, "APRS center updated to device location: %.6f, %.6f", priv->center.lat, priv->center.lng);
            aprs_update_items(priv);
        }
    }
}

static void aprs_update_callback(void *data) {
    struct map_priv *priv = (struct map_priv *)data;
    aprs_position_update_callback(data);
    aprs_update_items(priv);
}

static void aprs_map_destroy(struct map_priv *priv) {
    aprs_symbol_cleanup();
    if (!priv)
        return;

    GList *l;
    for (l = priv->items; l; l = g_list_next(l)) {
        struct item *item = (struct item *)l->data;
        aprs_item_destroy(item);
    }
    g_list_free(priv->items);

    if (priv->expire_timeout) {
        event_remove_timeout(priv->expire_timeout);
    }
    if (priv->update_timeout) {
        event_remove_timeout(priv->update_timeout);
    }

    if (priv->position_callback && priv->navit) {
        navit_remove_callback(priv->navit, priv->position_callback);
        callback_destroy(priv->position_callback);
    }

    if (priv->nmea) {
        aprs_nmea_stop(priv->nmea);
        aprs_nmea_destroy(priv->nmea);
    }

    g_hash_table_destroy(priv->station_hash);
    aprs_db_destroy(priv->db);
    g_free(priv);
}

static struct map_rect_priv *aprs_map_rect_new(struct map_priv *priv, struct map_selection *sel) {
    struct map_rect_priv *mr = g_new0(struct map_rect_priv, 1);
    mr->mpriv = priv;
    mr->next_item = priv->items;
    return mr;
}

static void aprs_map_rect_destroy(struct map_rect_priv *mr) {
    g_free(mr);
}

static struct item *aprs_map_get_item(struct map_rect_priv *mr) {
    if (mr->item) {
        struct aprs_item_priv *ip = (struct aprs_item_priv *)mr->item->priv_data;
        ip->mr = NULL;
    }

    if (mr->next_item) {
        struct item *ret = (struct item *)mr->next_item->data;
        struct aprs_item_priv *ip = (struct aprs_item_priv *)ret->priv_data;
        ip->mr = mr;
        aprs_item_attr_rewind(ret->priv_data);
        aprs_item_coord_rewind(ret->priv_data);
        mr->next_item = g_list_next(mr->next_item);
        mr->item = ret;
        return ret;
    }

    mr->item = NULL;
    return NULL;
}

static struct item *aprs_map_get_item_byid(struct map_rect_priv *mr, int id_hi, int id_lo) {
    struct item *ret = NULL;
    do {
        ret = aprs_map_get_item(mr);
    } while (ret && (ret->id_lo != id_lo || ret->id_hi != id_hi));
    return ret;
}

static int aprs_map_get_attr(struct map_priv *priv, enum attr_type type, struct attr *attr) {
    if (!priv)
        return 0;

    switch (type) {
    case attr_active:
        attr->type = attr_active;
        attr->u.num = (priv->packet_sources != NULL || priv->nmea_enabled) ? 1 : 0;
        return 1;
    default:
        return 0;
    }
}

/* Create default NMEA config */
static void aprs_create_default_nmea_config(struct aprs_nmea_config *config, const char *device, int baud_rate) {
    memset(config, 0, sizeof(*config));
    config->device = g_strdup(device ? device : "/dev/ttyUSB0");
    config->baud_rate = baud_rate ? baud_rate : 4800;
    config->parity = 'N';
    config->data_bits = 8;
    config->stop_bits = 1;
}

/* Parse parity from device string */
static void aprs_parse_nmea_parity(const char *device_str, struct aprs_nmea_config *config) {
    const char *parity_str = strstr(device_str, "parity=");
    if (parity_str) {
        if (parity_str[7] == 'E' || parity_str[7] == 'e') {
            config->parity = 'E';
        } else if (parity_str[7] == 'O' || parity_str[7] == 'o') {
            config->parity = 'O';
        }
    }
}

/* Start NMEA input with config */
static int aprs_start_nmea_input(struct map_priv *priv, const struct aprs_nmea_config *config) {
    priv->nmea = aprs_nmea_new(config);
    if (!priv->nmea) {
        return 0;
    }

    aprs_nmea_set_callback(priv->nmea, aprs_nmea_callback, priv);
    if (aprs_nmea_start(priv->nmea)) {
        priv->nmea_enabled = 1;
        return 1;
    }

    aprs_nmea_destroy(priv->nmea);
    priv->nmea = NULL;
    return 0;
}

/* Stop and destroy NMEA input */
static void aprs_stop_nmea_input(struct map_priv *priv) {
    if (priv->nmea) {
        aprs_nmea_stop(priv->nmea);
        aprs_nmea_destroy(priv->nmea);
        priv->nmea = NULL;
        priv->nmea_enabled = 0;
    }
}

/* Handle frequency attribute */
static int aprs_handle_frequency_attr(struct map_priv *priv, struct attr *attr) {
    if (attr->u.numd) {
        dbg(lvl_debug, "Frequency change requested (handled by SDR plugin if available): %.3f MHz", *attr->u.numd);
    }
    return 1;
}

/* Handle distance attribute */
static int aprs_handle_distance_attr(struct map_priv *priv, struct attr *attr) {
    priv->range_km = 150.0;
    dbg(lvl_info, "APRS range hardcoded to 150 km (distance attribute ignored)");
    aprs_update_items(priv);
    return 1;
}

/* Handle timeout attribute */
static int aprs_handle_timeout_attr(struct map_priv *priv, struct attr *attr) {
    if (!attr->u.num) {
        return 0;
    }
    priv->expire_seconds = attr->u.num;
    dbg(lvl_info, "APRS timeout set to %ld seconds (%ld minutes)", (long)priv->expire_seconds,
        (long)(priv->expire_seconds / 60));
    aprs_update_items(priv);
    return 1;
}

/* Handle position attribute */
static int aprs_handle_position_attr(struct map_priv *priv, struct attr *attr) {
    if (!attr->u.coord_geo) {
        return 0;
    }
    priv->center = *attr->u.coord_geo;
    priv->has_center = 1;
    dbg(lvl_info, "APRS center set to %.6f, %.6f", priv->center.lat, priv->center.lng);
    aprs_update_items(priv);
    return 1;
}

/* Handle device attribute - NMEA */
static int aprs_handle_device_nmea(struct map_priv *priv, const char *device_str) {
    priv->input_type = APRS_INPUT_NMEA;
    if (priv->nmea) {
        return 1;
    }

    struct aprs_nmea_config nmea_config;
    aprs_create_default_nmea_config(&nmea_config, "/dev/ttyUSB0", 4800);
    aprs_parse_nmea_parity(device_str, &nmea_config);

    int result = aprs_start_nmea_input(priv, &nmea_config);
    if (result) {
        dbg(lvl_info, "NMEA input enabled");
    }
    g_free(nmea_config.device);
    return result;
}

/* Handle device attribute - SDR */
static int aprs_handle_device_sdr(struct map_priv *priv) {
    priv->input_type = APRS_INPUT_SDR_PLUGIN;
    aprs_stop_nmea_input(priv);
    dbg(lvl_info, "SDR input selected (requires aprs_sdr plugin)");
    return 1;
}

/* Handle device attribute */
static int aprs_handle_device_attr(struct map_priv *priv, struct attr *attr) {
    if (!attr->u.str) {
        return 0;
    }

    if (strstr(attr->u.str, "nmea")) {
        return aprs_handle_device_nmea(priv, attr->u.str);
    } else if (strstr(attr->u.str, "sdr") || strstr(attr->u.str, "rtlsdr")) {
        return aprs_handle_device_sdr(priv);
    }
    return 0;
}

/* Handle data attribute (NMEA device change) */
static int aprs_handle_data_attr(struct map_priv *priv, struct attr *attr) {
    if (!attr->u.str || !priv->nmea) {
        return 0;
    }

    aprs_stop_nmea_input(priv);

    struct aprs_nmea_config nmea_config;
    aprs_create_default_nmea_config(&nmea_config, attr->u.str, 4800);

    int result = aprs_start_nmea_input(priv, &nmea_config);
    if (result) {
        dbg(lvl_info, "NMEA device changed to %s", attr->u.str);
    }
    g_free(nmea_config.device);
    return result;
}

/* Handle baudrate attribute */
static int aprs_handle_baudrate_attr(struct map_priv *priv, struct attr *attr) {
    if (!attr->u.num || !priv->nmea) {
        return 0;
    }

    aprs_stop_nmea_input(priv);

    struct aprs_nmea_config nmea_config;
    aprs_create_default_nmea_config(&nmea_config, "/dev/ttyUSB0", attr->u.num);

    int result = aprs_start_nmea_input(priv, &nmea_config);
    if (result) {
        dbg(lvl_info, "NMEA baud rate changed to %ld", (long)attr->u.num);
    }
    g_free(nmea_config.device);
    return result;
}

static int aprs_map_set_attr(struct map_priv *priv, struct attr *attr) {
    if (!priv) {
        return 0;
    }

    switch (attr->type) {
    case attr_frequency:
        return aprs_handle_frequency_attr(priv, attr);
    case attr_distance:
        return aprs_handle_distance_attr(priv, attr);
    case attr_timeout:
        return aprs_handle_timeout_attr(priv, attr);
    case attr_position_coord_geo:
        return aprs_handle_position_attr(priv, attr);
    case attr_device:
        return aprs_handle_device_attr(priv, attr);
    case attr_data:
        return aprs_handle_data_attr(priv, attr);
    case attr_baudrate:
        return aprs_handle_baudrate_attr(priv, attr);
    default:
        return 0;
    }
}

static struct map_methods aprs_map_meth = {
    projection_mg,
    "utf-8",
    aprs_map_destroy,
    aprs_map_rect_new,
    aprs_map_rect_destroy,
    aprs_map_get_item,
    aprs_map_get_item_byid,
    NULL,
    NULL,
    NULL,
    aprs_map_get_attr,
    aprs_map_set_attr,
};

static int aprs_process_packet(struct map_priv *priv, const unsigned char *data, int length) {
    if (!priv || !data || length <= 0)
        return 0;
    struct aprs_packet packet;
    struct aprs_position pos;

    if (!aprs_decode_ax25(data, length, &packet)) {
        return 0;
    }

    if (!aprs_parse_position(packet.information_field, packet.information_length, &pos)) {
        aprs_packet_free(&packet);
        aprs_position_free(&pos);
        return 0;
    }

    if (!pos.has_position) {
        aprs_packet_free(&packet);
        aprs_position_free(&pos);
        return 0;
    }

    struct aprs_station *station = aprs_station_new();
    station->callsign = g_strdup(packet.source_callsign);
    station->position = pos.position;
    station->altitude = pos.altitude;
    station->course = pos.course;
    station->speed = pos.speed;
    station->symbol_table = pos.symbol_table;
    station->symbol_code = pos.symbol_code;
    station->comment = pos.comment ? g_strdup(pos.comment) : NULL;
    station->path = packet.path ? g_strdupv(packet.path) : NULL;
    station->path_count = packet.path_count;

    time_t packet_time = time(NULL);
    if (aprs_parse_timestamp(packet.information_field, packet.information_length, &packet_time)) {
        station->timestamp = packet_time;
    } else {
        station->timestamp = time(NULL);
    }

    aprs_db_update_station(priv->db, station);

    const struct item *existing_item = g_hash_table_lookup(priv->station_hash, station->callsign);
    if (existing_item) {
        aprs_update_items(priv);
    } else {
        aprs_update_items(priv);
    }

    aprs_packet_free(&packet);
    aprs_position_free(&pos);
    aprs_station_free(station);

    return 1;
}

static struct map_priv *aprs_map_new(struct map_methods *meth, struct attr **attrs, struct callback_list *cbl) {
    struct map_priv *ret = g_new0(struct map_priv, 1);
    struct attr *db_path_attr, *expire_attr, *range_attr, *center_attr;

    *meth = aprs_map_meth;
    ret->cbl = cbl;
    ret->expire_seconds = 5400;
    ret->range_km = 150.0;
    ret->has_center = 0;
    ret->input_type = APRS_INPUT_NONE;
    ret->nmea_enabled = 0;
    ret->nmea = NULL;
    ret->navit = NULL;
    ret->position_callback = NULL;
    ret->packet_sources = NULL;

    /* Add to global list of map instances */
    aprs_map_instances = g_list_append(aprs_map_instances, ret);

    db_path_attr = attr_search(attrs, attr_data);
    const char *db_path = db_path_attr ? db_path_attr->u.str : NULL;

    ret->db = aprs_db_new(db_path);
    if (!ret->db) {
        dbg(lvl_error, "Failed to open APRS database");
        g_free(ret);
        return NULL;
    }

    expire_attr = attr_search(attrs, attr_timeout);
    if (expire_attr) {
        ret->expire_seconds = expire_attr->u.num;
    } else {
        ret->expire_seconds = 5400;
    }

    range_attr = attr_search(attrs, attr_distance);
    if (range_attr && range_attr->u.numd) {
        double requested_km = *range_attr->u.numd / 1000.0;
        if (requested_km > 150.0) {
            ret->range_km = 150.0;
            dbg(lvl_info, "APRS range capped at 150 km (configured: %.1f km)", requested_km);
        } else {
            ret->range_km = requested_km;
        }
    }

    center_attr = attr_search(attrs, attr_position_coord_geo);
    if (center_attr && center_attr->u.coord_geo) {
        ret->center = *center_attr->u.coord_geo;
        ret->has_center = 1;
    }

    ret->station_hash = g_hash_table_new(g_str_hash, g_str_equal);

    ret->expire_timeout = event_add_timeout(60000, 0, callback_new_1(callback_cast(aprs_expire_callback), ret));
    ret->update_timeout = event_add_timeout(1000, 0, callback_new_1(callback_cast(aprs_update_callback), ret));

    /* Initialize APRS symbol lookup system */
    aprs_symbol_init(NULL, NULL);

    aprs_update_items(ret);

    return ret;
}

/* Wrapper functions are defined above, no duplicates needed */

static void aprs_popup(struct container *cont, struct map *map, struct popup *popup, struct popup_item **list) {
    /* APRS menu is now integrated into Settings via XML configuration */
    /* This popup function is kept for compatibility but does nothing */
    (void)cont;
    (void)map;
    (void)popup;
    (void)list;
}

/* Packet source registration structure */
struct packet_source_entry {
    aprs_packet_callback_t callback;
    void *user_data;
};

/* aprs_map_instances is already defined above */

/**
 * Register a packet source callback
 *
 * This function is called by external plugins (like aprs_sdr) to
 * register a callback for delivering decoded AX.25 frames.
 */
/* Export function for other plugins - use visibility attribute */
__attribute__((visibility("default"))) int aprs_register_packet_source(aprs_packet_callback_t callback,
                                                                       void *user_data) {
    struct map_priv *map_priv = NULL;

    if (!callback) {
        dbg(lvl_error, "aprs_register_packet_source: NULL callback");
        return 0;
    }

    /* Find the first APRS map instance */
    /* In a multi-map scenario, we'd need to handle this differently */
    if (aprs_map_instances) {
        map_priv = (struct map_priv *)aprs_map_instances->data;
    } else {
        /* Try to find via plugin system */
        map_priv = (struct map_priv *)plugin_get_category_map("aprs");
    }

    if (!map_priv) {
        dbg(lvl_warning, "aprs_register_packet_source: No APRS map instance found");
        return 0;
    }

    /* Create wrapper callback that delivers to this map instance */
    struct packet_source_entry *entry = g_new0(struct packet_source_entry, 1);
    entry->callback = callback;
    /* Store map_priv as user_data so callback can deliver to it */
    entry->user_data = map_priv;

    map_priv->packet_sources = g_list_append(map_priv->packet_sources, entry);

    /* Set up the callback to deliver to this map */
    /* The SDR plugin's callback will be called, which should call aprs_process_packet */
    dbg(lvl_info, "Packet source registered with APRS plugin (map instance: %p)", map_priv);
    return 1;
}

__attribute__((visibility("default"))) int aprs_unregister_packet_source(aprs_packet_callback_t callback) {
    GList *l, *map_list;

    if (!callback)
        return 0;

    /* Search all map instances */
    for (map_list = aprs_map_instances; map_list; map_list = g_list_next(map_list)) {
        struct map_priv *map_priv = (struct map_priv *)map_list->data;

        for (l = map_priv->packet_sources; l; l = g_list_next(l)) {
            struct packet_source_entry *entry = (struct packet_source_entry *)l->data;
            if (entry->callback == callback) {
                map_priv->packet_sources = g_list_remove(map_priv->packet_sources, entry);
                g_free(entry);
                dbg(lvl_info, "Packet source unregistered from APRS plugin");
                return 1;
            }
        }
    }

    return 0;
}

/**
 * Process packets from registered sources
 * Called by packet source callbacks
 */
static void aprs_deliver_packet_to_map(struct map_priv *priv, const unsigned char *data, int length) {
    if (!priv || !data || length <= 0)
        return;

    /* Process packet through normal pipeline */
    aprs_process_packet(priv, data, length);
}

void plugin_init(void) {
    dbg(lvl_debug, "APRS plugin initializing");
    plugin_register_category_map("aprs", aprs_map_new);
    plugin_register_popup(callback_cast(aprs_popup));
    extern void plugin_init_osd(void);
    plugin_init_osd();

    /* Export packet registration function for other plugins */
    /* Function is already defined above and will be visible via symbol export */
}
