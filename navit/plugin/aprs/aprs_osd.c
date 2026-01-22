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

#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include "debug.h"
#include "item.h"
#include "navit.h"
#include "color.h"
#include "point.h"
#include "osd.h"
#include "plugin.h"
#include "map.h"
#include "mapset.h"
#include "attr.h"
#include "command.h"
#include "popup.h"
#include "aprs.h"
#include "aprs_db.h"

static void aprs_osd_set_frequency(struct navit *nav, double frequency) {
    struct attr map_attr, freq_attr;
    struct mapset *ms;
    struct map *map;
    struct attr_iter *iter;
    
    if (!nav) return;
    
    if (!navit_get_attr(nav, attr_mapset, &map_attr, NULL)) {
        dbg(lvl_error, "No mapset found");
        return;
    }
    
    ms = map_attr.u.mapset;
    iter = mapset_attr_iter_new(NULL);
    
    while (mapset_get_attr(ms, attr_map, &map_attr, iter)) {
        map = map_attr.u.map;
        if (map_get_attr(map, attr_type, &map_attr, NULL)) {
            if (map_attr.u.str && !strcmp(map_attr.u.str, "aprs")) {
                freq_attr.type = attr_frequency;
                freq_attr.u.numd = &frequency;
                map_set_attr(map, &freq_attr);
                dbg(lvl_info, "APRS frequency set to %.3f MHz via menu", frequency);
                break;
            }
        }
    }
    mapset_attr_iter_destroy(iter);
}

int aprs_cmd_freq_144_39(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    aprs_osd_set_frequency(nav, 144.39);
    return 1;
}

int aprs_cmd_freq_144_8(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    aprs_osd_set_frequency(nav, 144.8);
    return 1;
}

int aprs_cmd_freq_145_175(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    aprs_osd_set_frequency(nav, 145.175);
    return 1;
}

int aprs_cmd_freq_144_575(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    aprs_osd_set_frequency(nav, 144.575);
    return 1;
}

int aprs_cmd_freq_144_64(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    aprs_osd_set_frequency(nav, 144.64);
    return 1;
}

int aprs_cmd_freq_144_66(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    aprs_osd_set_frequency(nav, 144.66);
    return 1;
}

int aprs_cmd_freq_144_93(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    aprs_osd_set_frequency(nav, 144.93);
    return 1;
}

int aprs_cmd_freq_145_57(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    aprs_osd_set_frequency(nav, 145.57);
    return 1;
}


static void aprs_osd_set_timeout(struct navit *nav, int timeout_seconds) {
    struct attr map_attr, timeout_attr;
    struct mapset *ms;
    struct map *map;
    struct attr_iter *iter;
    
    if (!nav) return;
    
    if (!navit_get_attr(nav, attr_mapset, &map_attr, NULL)) {
        dbg(lvl_error, "No mapset found");
        return;
    }
    
    ms = map_attr.u.mapset;
    iter = mapset_attr_iter_new(NULL);
    
    while (mapset_get_attr(ms, attr_map, &map_attr, iter)) {
        map = map_attr.u.map;
        if (map_get_attr(map, attr_type, &map_attr, NULL)) {
            if (map_attr.u.str && !strcmp(map_attr.u.str, "aprs")) {
                timeout_attr.type = attr_timeout;
                timeout_attr.u.num = timeout_seconds;
                map_set_attr(map, &timeout_attr);
                dbg(lvl_info, "APRS timeout set to %d seconds (%d minutes) via menu", 
                    timeout_seconds, timeout_seconds / 60);
                break;
            }
        }
    }
    mapset_attr_iter_destroy(iter);
}


int aprs_cmd_timeout_30min(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    aprs_osd_set_timeout(nav, 1800);
    return 1;
}

int aprs_cmd_timeout_60min(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    aprs_osd_set_timeout(nav, 3600);
    return 1;
}

int aprs_cmd_timeout_90min(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    aprs_osd_set_timeout(nav, 5400);
    return 1;
}

int aprs_cmd_timeout_120min(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    aprs_osd_set_timeout(nav, 7200);
    return 1;
}

int aprs_cmd_timeout_180min(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    aprs_osd_set_timeout(nav, 10800);
    return 1;
}

int aprs_cmd_timeout_clear_expired(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    struct attr map_attr;
    struct mapset *ms;
    struct attr_iter *iter;
    
    if (!nav) return 0;
    
    if (!navit_get_attr(nav, attr_mapset, &map_attr, NULL)) {
        return 0;
    }
    
    ms = map_attr.u.mapset;
    iter = mapset_attr_iter_new(NULL);
    
    while (mapset_get_attr(ms, attr_map, &map_attr, iter)) {
        struct map *m = map_attr.u.map;
        if (map_get_attr(m, attr_type, &map_attr, NULL)) {
            if (map_attr.u.str && !strcmp(map_attr.u.str, "aprs")) {
                /* Use a special attribute to trigger expiration - handled by map_set_attr */
                struct attr expire_attr;
                expire_attr.type = attr_timeout;
                expire_attr.u.num = 0; /* Special value to trigger immediate expiration */
                map_set_attr(m, &expire_attr);
                dbg(lvl_info, "Triggered APRS station expiration");
                break;
            }
        }
    }
    mapset_attr_iter_destroy(iter);
    return 1;
}

static void aprs_osd_set_device(struct navit *nav, const char *device_type) {
    struct attr map_attr, device_attr;
    struct mapset *ms;
    struct map *map;
    struct attr_iter *iter;
    
    if (!nav) return;
    
    if (!navit_get_attr(nav, attr_mapset, &map_attr, NULL)) {
        dbg(lvl_error, "No mapset found");
        return;
    }
    
    ms = map_attr.u.mapset;
    iter = mapset_attr_iter_new(NULL);
    
    while (mapset_get_attr(ms, attr_map, &map_attr, iter)) {
        map = map_attr.u.map;
        if (map_get_attr(map, attr_type, &map_attr, NULL)) {
            if (map_attr.u.str && !strcmp(map_attr.u.str, "aprs")) {
                device_attr.type = attr_device;
                device_attr.u.str = (char *)device_type;
                map_set_attr(map, &device_attr);
                dbg(lvl_info, "APRS device set to %s via menu", device_type);
                break;
            }
        }
    }
    mapset_attr_iter_destroy(iter);
}

/* NMEA device path is set via device attribute, not needed separately */

static void aprs_osd_set_nmea_baud(struct navit *nav, int baud_rate) {
    struct attr map_attr, baud_attr;
    struct mapset *ms;
    struct map *map;
    struct attr_iter *iter;
    
    if (!nav) return;
    
    if (!navit_get_attr(nav, attr_mapset, &map_attr, NULL)) {
        dbg(lvl_error, "No mapset found");
        return;
    }
    
    ms = map_attr.u.mapset;
    iter = mapset_attr_iter_new(NULL);
    
    while (mapset_get_attr(ms, attr_map, &map_attr, iter)) {
        map = map_attr.u.map;
        if (map_get_attr(map, attr_type, &map_attr, NULL)) {
            if (map_attr.u.str && !strcmp(map_attr.u.str, "aprs")) {
                baud_attr.type = attr_baudrate;
                baud_attr.u.num = baud_rate;
                map_set_attr(map, &baud_attr);
                dbg(lvl_info, "APRS NMEA baud rate set to %d via menu", baud_rate);
                break;
            }
        }
    }
    mapset_attr_iter_destroy(iter);
}

static void aprs_osd_set_nmea_parity(struct navit *nav, char parity) {
    struct attr map_attr, device_attr;
    struct mapset *ms;
    struct map *map;
    struct attr_iter *iter;
    char device_str[256];
    
    if (!nav) return;
    
    if (!navit_get_attr(nav, attr_mapset, &map_attr, NULL)) {
        dbg(lvl_error, "No mapset found");
        return;
    }
    
    ms = map_attr.u.mapset;
    iter = mapset_attr_iter_new(NULL);
    
    while (mapset_get_attr(ms, attr_map, &map_attr, iter)) {
        map = map_attr.u.map;
        if (map_get_attr(map, attr_type, &map_attr, NULL)) {
            if (map_attr.u.str && !strcmp(map_attr.u.str, "aprs")) {
                if (map_get_attr(map, attr_device, &device_attr, NULL)) {
                    snprintf(device_str, sizeof(device_str), "nmea,parity=%c", parity);
                } else {
                    snprintf(device_str, sizeof(device_str), "nmea,parity=%c", parity);
                }
                device_attr.type = attr_device;
                device_attr.u.str = device_str;
                map_set_attr(map, &device_attr);
                dbg(lvl_info, "APRS NMEA parity set to %c via menu", parity);
                break;
            }
        }
    }
    mapset_attr_iter_destroy(iter);
}

int aprs_cmd_device_rtlsdr_blog_v3(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    aprs_osd_set_device(nav, "rtlsdr,device_type=blog_v3");
    return 1;
}

int aprs_cmd_device_rtlsdr_v4_r828d(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    aprs_osd_set_device(nav, "rtlsdr,device_type=v4_r828d");
    return 1;
}

int aprs_cmd_device_rtlsdr_nooelec(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    aprs_osd_set_device(nav, "rtlsdr,device_type=nooelec");
    return 1;
}

int aprs_cmd_device_rtlsdr_generic(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    aprs_osd_set_device(nav, "rtlsdr,device_type=generic");
    return 1;
}

int aprs_cmd_device_nmea(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    aprs_osd_set_device(nav, "nmea");
    return 1;
}

int aprs_cmd_nmea_baud_4800(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    aprs_osd_set_nmea_baud(nav, 4800);
    return 1;
}

int aprs_cmd_nmea_baud_9600(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    aprs_osd_set_nmea_baud(nav, 9600);
    return 1;
}

int aprs_cmd_nmea_baud_19200(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    aprs_osd_set_nmea_baud(nav, 19200);
    return 1;
}

int aprs_cmd_nmea_baud_38400(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    aprs_osd_set_nmea_baud(nav, 38400);
    return 1;
}

int aprs_cmd_nmea_parity_none(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    aprs_osd_set_nmea_parity(nav, 'N');
    return 1;
}

int aprs_cmd_nmea_parity_even(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    aprs_osd_set_nmea_parity(nav, 'E');
    return 1;
}

int aprs_cmd_nmea_parity_odd(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    aprs_osd_set_nmea_parity(nav, 'O');
    return 1;
}

static struct command_table aprs_commands[] = {
    {"aprs_freq_144_39", command_cast(aprs_cmd_freq_144_39)},
    {"aprs_freq_144_8", command_cast(aprs_cmd_freq_144_8)},
    {"aprs_freq_145_175", command_cast(aprs_cmd_freq_145_175)},
    {"aprs_freq_144_575", command_cast(aprs_cmd_freq_144_575)},
    {"aprs_freq_144_64", command_cast(aprs_cmd_freq_144_64)},
    {"aprs_freq_144_66", command_cast(aprs_cmd_freq_144_66)},
    {"aprs_freq_144_93", command_cast(aprs_cmd_freq_144_93)},
    {"aprs_freq_145_57", command_cast(aprs_cmd_freq_145_57)},
    {"aprs_timeout_30min", command_cast(aprs_cmd_timeout_30min)},
    {"aprs_timeout_60min", command_cast(aprs_cmd_timeout_60min)},
    {"aprs_timeout_90min", command_cast(aprs_cmd_timeout_90min)},
    {"aprs_timeout_120min", command_cast(aprs_cmd_timeout_120min)},
    {"aprs_timeout_180min", command_cast(aprs_cmd_timeout_180min)},
    {"aprs_timeout_clear", command_cast(aprs_cmd_timeout_clear_expired)},
    {"aprs_device_rtlsdr_blog_v3", command_cast(aprs_cmd_device_rtlsdr_blog_v3)},
    {"aprs_device_rtlsdr_v4_r828d", command_cast(aprs_cmd_device_rtlsdr_v4_r828d)},
    {"aprs_device_rtlsdr_nooelec", command_cast(aprs_cmd_device_rtlsdr_nooelec)},
    {"aprs_device_rtlsdr_generic", command_cast(aprs_cmd_device_rtlsdr_generic)},
    {"aprs_device_nmea", command_cast(aprs_cmd_device_nmea)},
    {"aprs_nmea_baud_4800", command_cast(aprs_cmd_nmea_baud_4800)},
    {"aprs_nmea_baud_9600", command_cast(aprs_cmd_nmea_baud_9600)},
    {"aprs_nmea_baud_19200", command_cast(aprs_cmd_nmea_baud_19200)},
    {"aprs_nmea_baud_38400", command_cast(aprs_cmd_nmea_baud_38400)},
    {"aprs_nmea_parity_none", command_cast(aprs_cmd_nmea_parity_none)},
    {"aprs_nmea_parity_even", command_cast(aprs_cmd_nmea_parity_even)},
    {"aprs_nmea_parity_odd", command_cast(aprs_cmd_nmea_parity_odd)},
};

static void aprs_osd_destroy(struct osd_priv *osd) {
    g_free(osd);
}

static int aprs_osd_set_attr(struct osd_priv *osd, struct attr *attr) {
    return 0;
}

static struct osd_methods aprs_osd_meth = {
    aprs_osd_destroy,
    aprs_osd_set_attr,
    aprs_osd_destroy,
    NULL,
};

static struct osd_priv *aprs_osd_new(struct navit *nav, struct osd_methods *meth, struct attr **attrs) {
    struct osd_priv *priv;
    struct attr cmd_attr;
    
    priv = g_malloc0(sizeof(void *)); /* osd_priv is opaque, just allocate pointer-sized memory */
    *meth = aprs_osd_meth;
    
    command_add_table_attr(aprs_commands, sizeof(aprs_commands)/sizeof(struct command_table), nav, &cmd_attr);
    navit_add_attr(nav, &cmd_attr);
    
    dbg(lvl_info, "APRS OSD menu commands registered");
    
    return priv;
}

void plugin_init_osd(void) {
    dbg(lvl_debug, "APRS OSD plugin initializing");
    plugin_register_category_osd("aprs", aprs_osd_new);
}

