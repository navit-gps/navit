/**
 * @file navit_safety.c
 * @brief Navit Safety plugin: POI confidence, remote mode, resource planning for desert/arid travel.
 *
 * Handles POI confidence scoring, location-aware remote mode activation, and
 * resource planning (fuel/water) for desert driving and foot travel in arid or
 * sparsely serviced terrain. See docs/user/plugins/navit_safety.rst for full spec.
 *
 * Copyright (C) 2025-2026 Navit Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License version 2
 * as published by the Free Software Foundation.
 */

#include "navit_safety.h"
#include <string.h>

/**
 * @brief Set configuration to spec defaults.
 * @param config Configuration structure to fill (ignored if NULL)
 */
void navit_safety_config_default(struct navit_safety_config *config) {
    if (!config)
        return;
    memset(config, 0, sizeof(*config));
    config->remote_mode = NAVIT_SAFETY_REMOTE_AUTO;
    config->poi_density_threshold_km = 80;
    config->koppen_trigger = 1;
    config->fuel_buffer_standard_km = 25;
    config->fuel_buffer_remote_km = 85;
    config->fuel_buffer_arid_km = 140;
    config->water_buffer_standard_km = 5;
    config->water_buffer_remote_km = 20;
    config->water_buffer_arid_km = 30;
    config->desert_consumption_warning = 1;
    config->census_depopulation_layer = 1;
    config->chain_api_queries = 1;
    config->unconfirmed_poi_display = 1;
    config->foot_travel_mode = 0;
    config->body_weight_kg = 70;
    config->heat_stress_warnings = 1;
}

/* The following definitions are only needed when building Navit itself.
 * Unit tests for configuration defaults link only against navit_safety_config_default. */
#if NAVIT_SAFETY_WITH_OSD

#    include "attr.h"
#    include "callback.h"
#    include "color.h"
#    include "command.h"
#    include "coord.h"
#    include "debug.h"
#    include "graphics.h"
#    include "navit.h"
#    include "navit_nls.h"
#    include "navit_safety_route.h"
#    include "osd.h"
#    include "plugin.h"
#    include "point.h"
#    include "vehicle.h"
#    include "xmlconfig.h"
#    include <glib.h>
#    include <math.h>
#    include <stdio.h>
#    include <sys/time.h>
#    if NAVIT_SAFETY_WITH_SQLITE
#        include "navit_safety_cache.h"
#    endif

#    define NAVIT_SAFETY_DEFAULT_RANGE_KM 600
#    define NAVIT_SAFETY_DEFAULT_UPDATE_S 30
#    define NAVIT_SAFETY_DEFAULT_WBGT_C 28.0

/** @brief OSD private data; @p item must remain the first member. */
struct navit_safety_priv {
    struct osd_item item;
    struct navit *nav;
    struct navit_safety_config config;
    struct navit_safety_route_state route_state;
    int active;
    int full_range_km;
    int update_period;
    double wbgt_c;
    double last_refresh;
    int width;
    struct graphics_gc *warn_gc;
    char trip_id[64];
#    if NAVIT_SAFETY_WITH_SQLITE
    struct navit_safety_cache *cache;
#    endif
};

static GList *navit_safety_instances;
static int navit_safety_commands_registered;

#    if NAVIT_SAFETY_WITH_SQLITE
static struct navit_safety_cache *navit_safety_open_cache(void) {
    struct navit_safety_cache *cache;
    char *path;

    path = g_build_filename(g_get_home_dir(), ".navit", "navit_safety.db", NULL);
    cache = navit_safety_cache_open(path);
    if (!cache)
        dbg(lvl_warning, "Navit Safety: could not open confirmation cache at %s", path);
    g_free(path);
    return cache;
}
#    endif

static double navit_safety_now(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

static int navit_safety_vehicle_geo(struct vehicle *v, double *lat, double *lon) {
    struct attr pos;

    if (!v || !lat || !lon)
        return 0;
    if (!vehicle_get_attr(v, attr_position_coord_geo, &pos, NULL))
        return 0;
    *lat = pos.u.coord_geo->lat;
    *lon = pos.u.coord_geo->lng;
    return 1;
}

static void navit_safety_refresh(struct navit_safety_priv *priv, struct vehicle *v, int force) {
    double lat = NAN;
    double lon = NAN;
    double now;

    if (!priv || !priv->active)
        return;
    now = navit_safety_now();
    if (!force && priv->last_refresh > 0.0 && now - priv->last_refresh < priv->update_period)
        return;
    navit_safety_vehicle_geo(v, &lat, &lon);
    navit_safety_route_refresh(&priv->route_state, priv->nav, &priv->config,
#    if NAVIT_SAFETY_WITH_SQLITE
                               priv->cache,
#    else
                               NULL,
#    endif
                               priv->trip_id, priv->full_range_km, priv->wbgt_c, lat, lon);
    priv->last_refresh = now;
}

static void navit_safety_draw_text_lines(struct navit_safety_priv *priv, const char *text, struct graphics_gc *gc) {
    char *copy;
    char *line;
    char *save;
    struct point p;
    int y;

    if (!text || !priv->item.gr)
        return;
    copy = g_strdup(text);
    p.x = 4;
    y = 4;
    line = strtok_r(copy, "\n", &save);
    while (line) {
        p.y = y;
        graphics_draw_text(priv->item.gr, gc, NULL, priv->item.font, line, &p, 0x10000, 0);
        y += priv->item.font_size / 50 + 12;
        line = strtok_r(NULL, "\n", &save);
    }
    g_free(copy);
}

static void navit_safety_osd_draw(struct osd_priv *osd, struct navit *nav, struct vehicle *v) {
    struct navit_safety_priv *priv = (struct navit_safety_priv *)osd;
    struct graphics_gc *gc;

    if (!priv || !priv->item.configured)
        return;

    navit_safety_refresh(priv, v, 0);
    osd_fill_with_bgcolor(&priv->item);

    gc = priv->item.graphic_fg;
    if (priv->route_state.valid && priv->route_state.fuel_plan.gap.warning && priv->warn_gc)
        gc = priv->warn_gc;

    navit_safety_draw_text_lines(priv, priv->route_state.status, gc);
    graphics_draw_mode(priv->item.gr, draw_mode_end);
}

static void navit_safety_osd_init(struct osd_priv *osd, struct navit *nav) {
    struct navit_safety_priv *priv = (struct navit_safety_priv *)osd;
    struct color warn_color = {0xffff, 0x4000, 0x0000, 0xffff};

    osd_set_std_graphic(nav, &priv->item, (struct osd_priv *)priv);
    priv->item.graphic_fg = graphics_gc_new(priv->item.gr);
    graphics_gc_set_foreground(priv->item.graphic_fg, &priv->item.text_color);
    graphics_gc_set_linewidth(priv->item.graphic_fg, priv->width);
    priv->warn_gc = graphics_gc_new(priv->item.gr);
    graphics_gc_set_foreground(priv->warn_gc, &warn_color);
    graphics_gc_set_linewidth(priv->warn_gc, priv->width);

    navit_add_callback(nav, callback_new_attr_1(callback_cast(navit_safety_osd_draw), attr_destination, priv));
    navit_add_callback(nav, callback_new_attr_1(callback_cast(navit_safety_osd_draw), attr_position_coord_geo, priv));
    navit_safety_refresh(priv, NULL, 1);
    navit_safety_osd_draw((struct osd_priv *)priv, nav, NULL);
}

static void navit_safety_apply_attr(struct navit_safety_priv *priv, struct attr *attr) {
    if (!priv || !attr)
        return;
    if (attr->type == attr_update_period) {
        priv->update_period = attr->u.num > 0 ? attr->u.num : NAVIT_SAFETY_DEFAULT_UPDATE_S;
        return;
    }
    if (attr->type == attr_label && attr->u.str) {
        g_strlcpy(priv->trip_id, attr->u.str, sizeof(priv->trip_id));
        return;
    }
    if (attr->type == attr_data && attr->u.str) {
        if (!strncmp(attr->u.str, "wbgt=", 5))
            priv->wbgt_c = g_ascii_strtod(attr->u.str + 5, NULL);
        else if (!strncmp(attr->u.str, "range=", 6))
            priv->full_range_km = (int)g_ascii_strtod(attr->u.str + 6, NULL);
        return;
    }
}

static void navit_safety_parse_attrs(struct navit_safety_priv *priv, struct attr **attrs) {
    int i;

    osd_set_std_attr(attrs, &priv->item, ITEM_HAS_TEXT | TRANSPARENT_BG);
    priv->full_range_km = NAVIT_SAFETY_DEFAULT_RANGE_KM;
    priv->update_period = NAVIT_SAFETY_DEFAULT_UPDATE_S;
    priv->wbgt_c = NAVIT_SAFETY_DEFAULT_WBGT_C;
    g_strlcpy(priv->trip_id, "default", sizeof(priv->trip_id));
    priv->width = 2;

    if (attrs) {
        for (i = 0; attrs[i]; i++)
            navit_safety_apply_attr(priv, attrs[i]);
    }
}

static void navit_safety_osd_destroy(struct osd_priv *osd) {
    struct navit_safety_priv *priv = (struct navit_safety_priv *)osd;

    if (!priv)
        return;
    priv->active = 0;
    navit_safety_instances = g_list_remove(navit_safety_instances, priv);
    navit_safety_route_state_clear(&priv->route_state);
#    if NAVIT_SAFETY_WITH_SQLITE
    navit_safety_cache_close(priv->cache);
    priv->cache = NULL;
#    endif
    g_free(priv);
}

static int navit_safety_osd_set_attr(struct osd_priv *osd, struct attr *attr) {
    struct navit_safety_priv *priv = (struct navit_safety_priv *)osd;
    navit_safety_apply_attr(priv, attr);
    navit_safety_refresh(priv, NULL, 1);
    return 1;
}

static struct navit_safety_priv *navit_safety_from_navit(struct navit *nav) {
    GList *l;

    for (l = navit_safety_instances; l; l = l->next) {
        struct navit_safety_priv *priv = l->data;
        if (priv->nav == nav)
            return priv;
    }
    return NULL;
}

static int navit_safety_cmd_toggle_remote(void *data, char *cmd, struct attr **in, struct attr ***out) {
    struct navit_safety_priv *priv = navit_safety_from_navit((struct navit *)data);
    (void)cmd;
    (void)in;
    (void)out;
    if (!priv)
        return -1;
    priv->config.remote_mode = (priv->config.remote_mode + 1) % 3;
    navit_safety_refresh(priv, NULL, 1);
    navit_say(priv->nav, _("Remote mode updated"));
    return 0;
}

static int navit_safety_cmd_confirm_poi(void *data, char *cmd, struct attr **in, struct attr ***out) {
    struct navit_safety_priv *priv = navit_safety_from_navit((struct navit *)data);
    struct attr vehicle_attr;
    struct vehicle *v = NULL;
    double lat = NAN;
    double lon = NAN;
    (void)cmd;
    (void)in;
    (void)out;
    if (!priv)
        return -1;
    if (navit_get_attr(priv->nav, attr_vehicle, &vehicle_attr, NULL))
        v = vehicle_attr.u.vehicle;
    navit_safety_vehicle_geo(v, &lat, &lon);
    if (navit_safety_route_confirm_nearest(&priv->route_state, priv->nav, &priv->config,
#    if NAVIT_SAFETY_WITH_SQLITE
                                           priv->cache,
#    else
                                           NULL,
#    endif
                                           priv->trip_id, priv->full_range_km, priv->wbgt_c, lat, lon)
        != 0) {
        navit_say(priv->nav, _("No POI to confirm"));
        return -1;
    }
    navit_say(priv->nav, _("POI confirmed"));
    return 0;
}

static int navit_safety_cmd_deny_poi(void *data, char *cmd, struct attr **in, struct attr ***out) {
    struct navit_safety_priv *priv = navit_safety_from_navit((struct navit *)data);
    struct attr vehicle_attr;
    struct vehicle *v = NULL;
    double lat = NAN;
    double lon = NAN;
    (void)cmd;
    (void)in;
    (void)out;
    if (!priv)
        return -1;
    if (navit_get_attr(priv->nav, attr_vehicle, &vehicle_attr, NULL))
        v = vehicle_attr.u.vehicle;
    navit_safety_vehicle_geo(v, &lat, &lon);
    if (navit_safety_route_deny_nearest(&priv->route_state, priv->nav, &priv->config,
#    if NAVIT_SAFETY_WITH_SQLITE
                                        priv->cache,
#    else
                                        NULL,
#    endif
                                        priv->trip_id, priv->full_range_km, priv->wbgt_c, lat, lon)
        != 0) {
        navit_say(priv->nav, _("No POI to deny"));
        return -1;
    }
    navit_say(priv->nav, _("POI denied for this trip"));
    return 0;
}

static int navit_safety_cmd_show_plan(void *data, char *cmd, struct attr **in, struct attr ***out) {
    struct navit_safety_priv *priv = navit_safety_from_navit((struct navit *)data);
    (void)cmd;
    (void)in;
    (void)out;
    if (!priv)
        return -1;
    navit_safety_refresh(priv, NULL, 1);
    if (!priv->route_state.valid) {
        navit_say(priv->nav, _("No active route to plan"));
        return -1;
    }
    navit_say(priv->nav, priv->route_state.status);
    return 0;
}

static int navit_safety_cmd_toggle_foot(void *data, char *cmd, struct attr **in, struct attr ***out) {
    struct navit_safety_priv *priv = navit_safety_from_navit((struct navit *)data);
    (void)cmd;
    (void)in;
    (void)out;
    if (!priv)
        return -1;
    priv->config.foot_travel_mode = !priv->config.foot_travel_mode;
    navit_safety_refresh(priv, NULL, 1);
    navit_say(priv->nav, priv->config.foot_travel_mode ? _("Foot travel enabled") : _("Foot travel disabled"));
    return 0;
}

static struct command_table navit_safety_commands[] = {
    {"navit_safety_toggle_remote", command_cast(navit_safety_cmd_toggle_remote)},
    {"navit_safety_confirm_poi",   command_cast(navit_safety_cmd_confirm_poi)  },
    {"navit_safety_deny_poi",      command_cast(navit_safety_cmd_deny_poi)     },
    {"navit_safety_show_plan",     command_cast(navit_safety_cmd_show_plan)    },
    {"navit_safety_toggle_foot",   command_cast(navit_safety_cmd_toggle_foot)  },
};

static struct osd_methods navit_safety_osd_meth;

static struct osd_priv *navit_safety_osd_new(struct navit *nav, struct osd_methods *meth, struct attr **attrs) {
    struct navit_safety_priv *priv;

    if (!nav) {
        dbg(lvl_error, "Navit Safety: osd_new called with NULL nav");
        return NULL;
    }

    priv = g_new0(struct navit_safety_priv, 1);
    navit_safety_route_state_init(&priv->route_state);
    priv->item.navit = nav;
    priv->item.rel_x = 10;
    priv->item.rel_y = 10;
    priv->item.rel_w = 220;
    priv->item.rel_h = 120;
    priv->item.font_size = 180;
    priv->item.meth.draw = osd_draw_cast(navit_safety_osd_draw);
    priv->nav = nav;
    priv->active = 1;
    navit_safety_config_default(&priv->config);
    navit_safety_parse_attrs(priv, attrs);
#    if NAVIT_SAFETY_WITH_SQLITE
    priv->cache = navit_safety_open_cache();
#    endif

    navit_safety_osd_meth.osd_destroy = navit_safety_osd_destroy;
    navit_safety_osd_meth.set_attr = navit_safety_osd_set_attr;
    navit_safety_osd_meth.destroy = navit_safety_osd_destroy;
    navit_safety_osd_meth.get_attr = NULL;
    *meth = navit_safety_osd_meth;

    if (!navit_safety_commands_registered) {
        navit_command_add_table(nav, navit_safety_commands,
                                sizeof(navit_safety_commands) / sizeof(struct command_table));
        navit_safety_commands_registered = 1;
    }
    navit_safety_instances = g_list_append(navit_safety_instances, priv);
    navit_add_callback(nav, callback_new_attr_1(callback_cast(navit_safety_osd_init), attr_graphics_ready, priv));

    dbg(lvl_info, "Navit Safety plugin: OSD initialized (remote_mode=%d, fuel_buffer_remote=%d km)",
        priv->config.remote_mode, priv->config.fuel_buffer_remote_km);

    return (struct osd_priv *)priv;
}

void plugin_init(void) {
    dbg(lvl_debug, "Navit Safety plugin initializing");
    plugin_register_category_osd("navit_safety", navit_safety_osd_new);
}

#endif /* NAVIT_SAFETY_WITH_OSD */
