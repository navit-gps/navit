/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 */

#include "attr.h"
#include "debug.h"
#include "driver_break_osd.h"
#include "driver_break_osd_priv.h"
#include "navit.h"
#include "vehicleprofile.h"
#include <glib.h>
#include <stdio.h>

#ifndef HAVE_API_ANDROID

static void overnight_append_building_distance(struct osd_dialog_ctx *ctx, struct driver_break_priv *priv) {
    osd_append_validated_int_label(ctx, "Min distance from buildings: %d m", &priv->config.min_distance_from_buildings,
                                   0, 10000, 150, "min_distance_from_buildings");
}

static void overnight_append_glacier_distance(struct osd_dialog_ctx *ctx, struct driver_break_priv *priv) {
    osd_append_validated_int_label(ctx, "Min distance from glaciers: %d m", &priv->config.min_distance_from_glaciers, 0,
                                   10000, 300, "min_distance_from_glaciers");
}

static void overnight_append_poi_radii(struct osd_dialog_ctx *ctx, struct driver_break_priv *priv) {
    osd_append_validated_int_label(ctx, "POI search radius: %d km", &priv->config.poi_search_radius_km, 0, 1000, 15,
                                   "poi_search_radius_km");
    osd_append_validated_int_label(ctx, "Water search radius: %d km", &priv->config.poi_water_search_radius_km, 0, 100,
                                   2, "poi_water_search_radius_km");
    osd_append_validated_int_label(ctx, "Cabin search radius: %d km", &priv->config.poi_cabin_search_radius_km, 0, 100,
                                   5, "poi_cabin_search_radius_km");
}

void driver_break_show_overnight_dialog(struct gui_priv *gui_priv, struct driver_break_priv *priv,
                                        const char *profile_name) {
    struct osd_dialog_ctx ctx;
    char title[128];

    if (!priv) {
        dbg(lvl_error, "Driver Break plugin: driver_break_show_overnight_dialog called with NULL priv");
        return;
    }

    dbg(lvl_debug,
        "Driver Break plugin: Showing overnight dialog - config values: min_buildings=%d, min_glaciers=%d, "
        "poi_radius=%d, water_radius=%d, cabin_radius=%d",
        priv->config.min_distance_from_buildings, priv->config.min_distance_from_glaciers,
        priv->config.poi_search_radius_km, priv->config.poi_water_search_radius_km,
        priv->config.poi_cabin_search_radius_km);

    snprintf(title, sizeof(title), "Overnight Stops (%s)", profile_name);
    osd_dialog_begin(gui_priv, title, &ctx);
    overnight_append_building_distance(&ctx, priv);

    if (!g_ascii_strcasecmp(profile_name, "hiking") || !g_ascii_strcasecmp(profile_name, "pedestrian")) {
        overnight_append_glacier_distance(&ctx, priv);
    }

    overnight_append_poi_radii(&ctx, priv);
    osd_append_label(&ctx, "Note: Advanced editing coming soon");
    osd_dialog_end_with_save(&ctx, priv);
}

#endif /* !HAVE_API_ANDROID */

int driver_break_cmd_configure_overnight(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    struct driver_break_priv *priv;
    struct vehicleprofile *profile;
    struct attr profile_attr;
    struct gui_priv *gui_priv;
    char *profile_name;

    (void)function;
    (void)in;
    (void)out;

    dbg(lvl_info, "Driver Break plugin: driver_break_cmd_configure_overnight called");

    if (!driver_break_osd_require_plugin(nav, &priv)) {
        navit_add_message(nav, "Driver Break plugin: Plugin not loaded. Please check configuration.");
        return 0;
    }

    profile = navit_get_vehicleprofile(nav);
    if (!profile) {
        dbg(lvl_warning, "Driver Break plugin: No vehicle profile found");
        navit_add_message(nav, "Driver Break plugin: No vehicle profile selected");
        return 0;
    }

    if (!vehicleprofile_get_attr(profile, attr_name, &profile_attr, NULL)) {
        dbg(lvl_warning, "Driver Break plugin: Could not get profile name");
        navit_add_message(nav, "Driver Break plugin: Could not get profile name");
        return 0;
    }

    profile_name = profile_attr.u.str;
    dbg(lvl_info, "Driver Break plugin: Configure overnight stops for profile: %s", profile_name);

    if (!driver_break_osd_require_gui(nav, &gui_priv)) {
        navit_add_message(nav, "Driver Break plugin: Configuration dialog requires internal GUI");
        dbg(lvl_warning, "Driver Break plugin: Internal GUI not available");
        return 0;
    }

#ifndef HAVE_API_ANDROID
    driver_break_show_overnight_dialog(gui_priv, priv, profile_name);
#endif

    return 1;
}
