/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 */

#include "attr.h"
#include "debug.h"
#include "driver_break_db.h"
#include "driver_break_osd.h"
#include "driver_break_osd_priv.h"
#include "navit.h"
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifndef HAVE_API_ANDROID

static void fuel_dialog_append_profile(struct osd_dialog_ctx *ctx, struct driver_break_config *config) {
    osd_append_label_printf(ctx, "Fuel type: %d", config->fuel_type);
    osd_append_label_printf(ctx, "Tank capacity: %d", config->fuel_tank_capacity_l);
    osd_append_label_printf(ctx, "Average consumption: %.1f L/100km", config->fuel_avg_consumption_x10 / 10.0);
    osd_append_label_printf(ctx, "Low fuel warning: %d km", config->fuel_low_warning_km);
    osd_append_label_printf(ctx, "Fuel search buffer: %d km", config->fuel_search_buffer_km);
    osd_append_label_printf(ctx, "High-load threshold: %d%%", config->fuel_high_load_threshold);
}

static void fuel_dialog_append_adapters(struct osd_dialog_ctx *ctx, struct driver_break_config *config) {
    osd_append_label_printf(ctx, "OBD-II available: %s", config->fuel_obd_available ? "yes" : "no");
    osd_append_label_printf(ctx, "J1939 available: %s", config->fuel_j1939_available ? "yes" : "no");
    osd_append_label_printf(ctx, "MegaSquirt available: %s", config->fuel_megasquirt_available ? "yes" : "no");
    if (config->fuel_injector_flow_cc_min > 0) {
        osd_append_label_printf(ctx, "Injector flow: %d cc/min", config->fuel_injector_flow_cc_min);
    }
}

void driver_break_show_fuel_config_dialog(struct gui_priv *gui_priv, struct driver_break_priv *priv) {
    struct osd_dialog_ctx ctx;

    if (!priv) {
        dbg(lvl_error, "Driver Break plugin: driver_break_show_fuel_config_dialog called with NULL priv");
        return;
    }

    osd_dialog_begin(gui_priv, "Fuel and Range Configuration", &ctx);
    fuel_dialog_append_profile(&ctx, &priv->config);
    fuel_dialog_append_adapters(&ctx, &priv->config);
    osd_append_label(&ctx, "Note: Advanced editing of fuel parameters and adapters will be added in a later version.");
    osd_dialog_end_with_save(&ctx, priv);
}

#endif /* !HAVE_API_ANDROID */

static int driver_break_parse_fuel_value(const char *function, double *value_out, const char *what) {
    char *endptr = NULL;
    double value;

    if (!function || *function == '\0') {
        navit_add_message(NULL, "Driver Break plugin: Missing numeric argument for fuel operation");
        return 0;
    }

    value = g_ascii_strtod(function, &endptr);
    if (endptr == function || (!g_ascii_isspace(*endptr) && *endptr != '\0')) {
        navit_add_message(NULL, what);
        return 0;
    }

    if (value < 0.0) {
        navit_add_message(NULL, "Driver Break plugin: Fuel value cannot be negative");
        return 0;
    }

    *value_out = value;
    return 1;
}

static void driver_break_update_remaining_range(struct driver_break_priv *priv) {
    if (priv->config.fuel_avg_consumption_x10 > 0) {
        double avg_l_per_100 = priv->config.fuel_avg_consumption_x10 / 10.0;
        priv->fuel_remaining_range = (priv->fuel_current / avg_l_per_100) * 100.0;
    } else {
        priv->fuel_remaining_range = 0.0;
    }
}

int driver_break_cmd_configure_fuel(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    struct driver_break_priv *priv;
    struct gui_priv *gui_priv;

    (void)function;
    (void)in;
    (void)out;

    dbg(lvl_info, "Driver Break plugin: Configure fuel command");

    if (!driver_break_osd_require_plugin(nav, &priv)) {
        return 0;
    }

    if (!driver_break_osd_require_gui(nav, &gui_priv)) {
        navit_add_message(nav, "Driver Break plugin: Fuel configuration dialog requires internal GUI");
        return 0;
    }

#ifndef HAVE_API_ANDROID
    driver_break_show_fuel_config_dialog(gui_priv, priv);
#endif

    return 1;
}

int driver_break_cmd_set_fuel_level(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    struct driver_break_priv *priv;
    double value;

    (void)in;
    (void)out;

    dbg(lvl_info, "Driver Break plugin: Set fuel level command (function='%s')", function ? function : "");

    if (!driver_break_osd_require_plugin(nav, &priv)) {
        return 0;
    }

    if (!driver_break_parse_fuel_value(function, &value, "Driver Break plugin: Invalid fuel level value")) {
        return 0;
    }

    priv->fuel_current = value;
    driver_break_update_remaining_range(priv);

    dbg(lvl_info,
        "Driver Break plugin: Fuel level set to %.2f (tank capacity=%d L, avg=%.1f L/100km, remaining range=%.1f km)",
        priv->fuel_current, priv->config.fuel_tank_capacity_l, priv->config.fuel_avg_consumption_x10 / 10.0,
        priv->fuel_remaining_range);

    navit_add_message(nav, "Driver Break plugin: Fuel level updated");
    return 1;
}

static void driver_break_apply_fuel_added(struct driver_break_priv *priv, double added) {
    priv->fuel_current += added;
    if (priv->config.fuel_tank_capacity_l > 0 && priv->fuel_current > priv->config.fuel_tank_capacity_l) {
        priv->fuel_current = priv->config.fuel_tank_capacity_l;
    }
}

static void driver_break_fill_fuel_stop(struct navit *nav, struct driver_break_priv *priv,
                                        struct driver_break_fuel_stop *stop, double added) {
    struct attr position_attr;

    memset(stop, 0, sizeof(*stop));
    stop->timestamp = time(NULL);
    stop->fuel_added = added;
    stop->fuel_level_after = priv->fuel_current;
    stop->cost = 0.0;
    stop->currency = NULL;
    stop->ethanol_pct = priv->config.fuel_ethanol_manual_pct;

    if (navit_get_attr(nav, attr_position_coord_geo, &position_attr, NULL) && position_attr.u.coord_geo) {
        stop->coord = *position_attr.u.coord_geo;
    }
}

int driver_break_cmd_log_fuel_stop(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    struct driver_break_priv *priv;
    struct driver_break_fuel_stop stop;
    double added = 0.0;

    (void)in;
    (void)out;

    dbg(lvl_info, "Driver Break plugin: Log fuel stop command (function='%s')", function ? function : "");

    if (!driver_break_osd_require_plugin(nav, &priv)) {
        return 0;
    }

    if (!priv->db) {
        navit_add_message(nav, "Driver Break plugin: Database not available");
        return 0;
    }

    if (function && *function
        && !driver_break_parse_fuel_value(function, &added, "Driver Break plugin: Invalid fuel added value")) {
        return 0;
    }

    driver_break_apply_fuel_added(priv, added);
    driver_break_fill_fuel_stop(nav, priv, &stop, added);

    if (!driver_break_db_add_fuel_stop(priv->db, &stop)) {
        navit_add_message(nav, "Driver Break plugin: Failed to log fuel stop");
        return 0;
    }

    driver_break_update_remaining_range(priv);
    navit_add_message(nav, "Driver Break plugin: Fuel stop logged");
    dbg(lvl_info,
        "Driver Break plugin: Fuel stop logged (added=%.2f, level_after=%.2f, remaining_range=%.1f km, ethanol=%d%%)",
        added, priv->fuel_current, priv->fuel_remaining_range, priv->config.fuel_ethanol_manual_pct);

    return 1;
}
