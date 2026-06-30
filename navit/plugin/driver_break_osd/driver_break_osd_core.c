/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 */

#include "attr.h"
#include "command.h"
#include "debug.h"
#include "driver_break_db.h"
#include "driver_break_finder.h"
#include "driver_break_osd.h"
#include "driver_break_osd_priv.h"
#include "navit.h"
#include "vehicleprofile.h"
#include <glib.h>
#include <stdio.h>
#include <time.h>

static struct command_table driver_break_commands[] = {
    {"driver_break_suggest_stop",        command_cast(driver_break_cmd_suggest_stop)       },
    {"driver_break_show_history",        command_cast(driver_break_cmd_show_history)       },
    {"driver_break_configure",           command_cast(driver_break_cmd_configure)          },
    {"driver_break_start_break",         command_cast(driver_break_cmd_start_break)        },
    {"driver_break_end_break",           command_cast(driver_break_cmd_end_break)          },
    {"driver_break_configure_intervals", command_cast(driver_break_cmd_configure_intervals)},
    {"driver_break_configure_overnight", command_cast(driver_break_cmd_configure_overnight)},
    {"driver_break_set_fuel_level",      command_cast(driver_break_cmd_set_fuel_level)     },
    {"driver_break_log_fuel_stop",       command_cast(driver_break_cmd_log_fuel_stop)      },
    {"driver_break_configure_fuel",      command_cast(driver_break_cmd_configure_fuel)     },
    {"rest_suggest_stop",                command_cast(driver_break_cmd_suggest_stop)       },
    {"rest_show_history",                command_cast(driver_break_cmd_show_history)       },
    {"rest_start_break",                 command_cast(driver_break_cmd_start_break)        },
    {"rest_end_break",                   command_cast(driver_break_cmd_end_break)          },
    {"rest_configure_intervals",         command_cast(driver_break_cmd_configure_intervals)},
    {"rest_configure_overnight",         command_cast(driver_break_cmd_configure_overnight)},
};

void driver_break_register_commands(struct navit *nav) {
    if (!nav) {
        dbg(lvl_error, "Driver Break plugin: Cannot register commands - nav is NULL");
        return;
    }
    navit_command_add_table(nav, driver_break_commands, sizeof(driver_break_commands) / sizeof(struct command_table));
    dbg(lvl_info, "Driver Break plugin: Menu commands registered (count=%zu)",
        sizeof(driver_break_commands) / sizeof(struct command_table));
}

#ifndef HAVE_API_ANDROID

static void suggestions_append_stop(struct osd_dialog_ctx *ctx, struct driver_break_stop *stop, int count) {
    char buffer[512];

    if (stop->name) {
        snprintf(buffer, sizeof(buffer), "%d. %s (%.1f km from route)", count, stop->name,
                 stop->distance_from_route / 1000.0);
    } else {
        snprintf(buffer, sizeof(buffer), "%d. Rest stop (%.1f km from route)", count,
                 stop->distance_from_route / 1000.0);
    }
    osd_append_label(ctx, buffer);
}

static void suggestions_append_stops(struct osd_dialog_ctx *ctx, GList *stops) {
    GList *l;
    int count = 0;

    if (!stops) {
        osd_append_label(ctx, "No rest stops found along route");
        return;
    }

    for (l = stops; l; l = g_list_next(l)) {
        struct driver_break_stop *stop = (struct driver_break_stop *)l->data;
        if (!stop) {
            continue;
        }
        count++;
        suggestions_append_stop(ctx, stop, count);
    }
}

void driver_break_show_suggestions_dialog(struct gui_priv *gui_priv, struct driver_break_priv *priv, GList *stops) {
    struct osd_dialog_ctx ctx;

    if (!priv) {
        dbg(lvl_error, "Driver Break plugin: driver_break_show_suggestions_dialog called with NULL priv");
        return;
    }

    osd_dialog_begin(gui_priv, "Rest Stop Suggestions", &ctx);
    suggestions_append_stops(&ctx, stops);
    osd_dialog_end(&ctx);
}

static void format_history_entry(struct driver_break_stop_history *entry, int count, char *buffer, size_t buf_size) {
    const char *mandatory_suffix = entry->was_mandatory ? ", mandatory" : "";
    struct tm *tm_info = localtime(&entry->timestamp);

    if (tm_info) {
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", tm_info);
        if (entry->name) {
            snprintf(buffer, buf_size, "%d. %s - %s (%d min%s)", count, time_str, entry->name, entry->duration_minutes,
                     mandatory_suffix);
        } else {
            snprintf(buffer, buf_size, "%d. %s - %.6f, %.6f (%d min%s)", count, time_str, entry->coord.lat,
                     entry->coord.lng, entry->duration_minutes, mandatory_suffix);
        }
        return;
    }

    if (entry->name) {
        snprintf(buffer, buf_size, "%d. %s (%d min%s)", count, entry->name, entry->duration_minutes, mandatory_suffix);
    } else {
        snprintf(buffer, buf_size, "%d. %.6f, %.6f (%d min%s)", count, entry->coord.lat, entry->coord.lng,
                 entry->duration_minutes, mandatory_suffix);
    }
}

static void history_append_entries(struct osd_dialog_ctx *ctx, GList *history) {
    GList *l;
    int count = 0;
    char buffer[512];

    if (!history) {
        osd_append_label(ctx, "No history entries found");
        return;
    }

    for (l = history; l; l = g_list_next(l)) {
        struct driver_break_stop_history *entry = (struct driver_break_stop_history *)l->data;
        if (!entry) {
            continue;
        }
        count++;
        format_history_entry(entry, count, buffer, sizeof(buffer));
        osd_append_label(ctx, buffer);
    }
}

void driver_break_show_history_dialog(struct gui_priv *gui_priv, struct driver_break_priv *priv, GList *history) {
    struct osd_dialog_ctx ctx;

    if (!priv) {
        dbg(lvl_error, "Driver Break plugin: driver_break_show_history_dialog called with NULL priv");
        return;
    }

    osd_dialog_begin(gui_priv, "Rest Stop History", &ctx);
    history_append_entries(&ctx, history);
    osd_dialog_end(&ctx);
}

#endif /* !HAVE_API_ANDROID */

static GList *driver_break_suggest_stops_for_nav(struct navit *nav, struct driver_break_priv *priv) {
    struct route *route;
    struct attr route_attr;

    if (priv->suggested_stops) {
        dbg(lvl_info, "Driver Break plugin: Using existing suggested stops");
        return priv->suggested_stops;
    }

    if (!navit_get_attr(nav, attr_route, &route_attr, NULL)) {
        navit_add_message(nav, "Driver Break plugin: No active route");
        dbg(lvl_warning, "Driver Break plugin: No route found");
        return NULL;
    }

    route = route_attr.u.route;
    if (!route) {
        navit_add_message(nav, "Driver Break plugin: No active route");
        return NULL;
    }

    return driver_break_finder_find_along_route(route, &priv->config, priv->session.mandatory_break_required);
}

int driver_break_cmd_suggest_stop(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    struct driver_break_priv *priv;
    struct gui_priv *gui_priv;
    GList *stops;

    (void)function;
    (void)in;
    (void)out;

    dbg(lvl_info, "Driver Break plugin: Suggest rest stop command");

    if (!driver_break_osd_require_plugin(nav, &priv)) {
        return 0;
    }

    stops = driver_break_suggest_stops_for_nav(nav, priv);
    if (!stops && !priv->suggested_stops) {
        return 0;
    }

    if (!driver_break_osd_require_gui(nav, &gui_priv)) {
        navit_add_message(nav, "Driver Break plugin: Suggestions dialog requires internal GUI");
        if (stops && stops != priv->suggested_stops) {
            driver_break_finder_free_list(stops);
        }
        return 0;
    }

#ifndef HAVE_API_ANDROID
    driver_break_show_suggestions_dialog(gui_priv, priv, stops);
#endif

    if (stops && stops != priv->suggested_stops) {
        driver_break_finder_free_list(stops);
    }

    return 1;
}

int driver_break_cmd_show_history(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    struct driver_break_priv *priv;
    struct gui_priv *gui_priv;
    GList *history;

    (void)function;
    (void)in;
    (void)out;

    dbg(lvl_info, "Driver Break plugin: Show history command");

    if (!driver_break_osd_require_plugin(nav, &priv)) {
        return 0;
    }

    if (!priv->db) {
        navit_add_message(nav, "Driver Break plugin: Database not available");
        return 0;
    }

    history = driver_break_db_get_history(priv->db, 0);
    if (!driver_break_osd_require_gui(nav, &gui_priv)) {
        navit_add_message(nav, "Driver Break plugin: History dialog requires internal GUI");
        if (history) {
            g_list_free_full(history, (GDestroyNotify)driver_break_free_history_entry);
        }
        return 0;
    }

#ifndef HAVE_API_ANDROID
    driver_break_show_history_dialog(gui_priv, priv, history);
#endif

    if (history) {
        g_list_free_full(history, (GDestroyNotify)driver_break_free_history_entry);
    }

    return 1;
}

int driver_break_cmd_configure(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    (void)nav;
    (void)function;
    (void)in;
    (void)out;

    dbg(lvl_info, "Driver Break plugin: Configure command");
    return 1;
}

static struct driver_break_stop_history *driver_break_build_history_entry(struct driver_break_priv *priv,
                                                                          int duration_minutes) {
    struct driver_break_stop_history *entry;

    entry = g_new0(struct driver_break_stop_history, 1);
    if (!entry) {
        return NULL;
    }

    entry->timestamp = priv->current_break_start_time;
    entry->coord = priv->current_break_location;
    entry->duration_minutes = duration_minutes;
    entry->was_mandatory = priv->session.mandatory_break_required;
    entry->name = NULL;
    return entry;
}

static void driver_break_save_history_entry(struct driver_break_priv *priv, struct driver_break_stop_history *entry,
                                            int duration_minutes) {
    if (!priv->db) {
        return;
    }

    if (driver_break_db_add_history(priv->db, entry)) {
        dbg(lvl_info, "Driver Break plugin: Break saved to history: %d minutes", duration_minutes);
        navit_add_message(priv->nav, "Driver Break plugin: Break ended and saved");
    } else {
        dbg(lvl_error, "Driver Break plugin: Failed to save break to history");
        navit_add_message(priv->nav, "Driver Break plugin: Failed to save break");
    }
}

static void driver_break_reset_break_session(struct driver_break_priv *priv, time_t now) {
    priv->session.last_break_time = now;
    priv->session.continuous_driving_minutes = 0;
    priv->session.breaks_taken++;
    priv->session.mandatory_break_required = 0;
    priv->current_break_start_time = 0;
}

int driver_break_cmd_start_break(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    struct driver_break_priv *priv;
    struct attr position_attr;
    time_t now;

    (void)function;
    (void)in;
    (void)out;

    dbg(lvl_info, "Driver Break plugin: Start break command");

    if (!driver_break_osd_require_plugin(nav, &priv)) {
        return 0;
    }

    if (priv->current_break_start_time != 0) {
        navit_add_message(nav, "Driver Break plugin: Break already in progress");
        dbg(lvl_warning, "Driver Break plugin: Attempted to start break while one is already active");
        return 0;
    }

    now = time(NULL);
    priv->current_break_start_time = now;

    if (navit_get_attr(nav, attr_position_coord_geo, &position_attr, NULL) && position_attr.u.coord_geo) {
        priv->current_break_location = *position_attr.u.coord_geo;
    }

    dbg(lvl_info, "Driver Break plugin: Break started at %ld", (long)now);
    navit_add_message(nav, "Driver Break plugin: Break started");
    return 1;
}

int driver_break_cmd_end_break(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    struct driver_break_priv *priv;
    struct driver_break_stop_history *entry;
    time_t now;
    int duration_minutes;

    (void)function;
    (void)in;
    (void)out;

    dbg(lvl_info, "Driver Break plugin: End break command");

    if (!driver_break_osd_require_plugin(nav, &priv)) {
        return 0;
    }

    if (priv->current_break_start_time == 0) {
        navit_add_message(nav, "Driver Break plugin: No break in progress");
        dbg(lvl_warning, "Driver Break plugin: Attempted to end break when none is active");
        return 0;
    }

    now = time(NULL);
    duration_minutes = (int)((now - priv->current_break_start_time) / 60);
    if (duration_minutes < 0) {
        dbg(lvl_error, "Driver Break plugin: Invalid break duration: %d minutes", duration_minutes);
        priv->current_break_start_time = 0;
        return 0;
    }

    entry = driver_break_build_history_entry(priv, duration_minutes);
    if (!entry) {
        dbg(lvl_error, "Driver Break plugin: Out of memory");
        priv->current_break_start_time = 0;
        return 0;
    }

    driver_break_save_history_entry(priv, entry, duration_minutes);
    driver_break_reset_break_session(priv, now);
    driver_break_free_history_entry(entry);

    dbg(lvl_info, "Driver Break plugin: Break ended after %d minutes", duration_minutes);
    return 1;
}

static int driver_break_osd_profile_name(struct navit *nav, char **profile_name_out) {
    struct vehicleprofile *profile;
    struct attr profile_attr;

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

    *profile_name_out = profile_attr.u.str;
    return 1;
}

int driver_break_osd_show_intervals_for_profile(struct gui_priv *gui_priv, struct driver_break_priv *priv,
                                                const char *profile_name) {
    if (!g_ascii_strcasecmp(profile_name, "car")) {
        driver_break_show_car_intervals_dialog(gui_priv, priv);
        return 1;
    }
    if (!g_ascii_strcasecmp(profile_name, "truck") || !g_ascii_strcasecmp(profile_name, "Truck")) {
        driver_break_show_truck_intervals_dialog(gui_priv, priv);
        return 1;
    }
    if (!g_ascii_strcasecmp(profile_name, "hiking") || !g_ascii_strcasecmp(profile_name, "pedestrian")) {
        driver_break_show_hiking_intervals_dialog(gui_priv, priv);
        return 1;
    }
    if (!g_ascii_strcasecmp(profile_name, "bike") || !g_ascii_strcasecmp(profile_name, "cycling")) {
        driver_break_show_cycling_intervals_dialog(gui_priv, priv);
        return 1;
    }
    if (!g_ascii_strcasecmp(profile_name, "motorcycle")) {
        driver_break_show_motorcycle_intervals_dialog(gui_priv, priv);
        return 1;
    }

    dbg(lvl_warning, "Driver Break plugin: Unknown profile type: %s", profile_name);
    return 0;
}

int driver_break_cmd_configure_intervals(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    struct driver_break_priv *priv;
    struct gui_priv *gui_priv;
    char *profile_name;

    (void)function;
    (void)in;
    (void)out;

    dbg(lvl_info, "Driver Break plugin: driver_break_cmd_configure_intervals called");

    if (!driver_break_osd_require_plugin(nav, &priv)) {
        navit_add_message(nav, "Driver Break plugin: Plugin not loaded. Please check configuration.");
        return 0;
    }

    if (!driver_break_osd_profile_name(nav, &profile_name)) {
        return 0;
    }

    dbg(lvl_info, "Driver Break plugin: Configure intervals for profile: %s", profile_name);

    if (!driver_break_osd_require_gui(nav, &gui_priv)) {
        navit_add_message(nav, "Driver Break plugin: Configuration dialog requires internal GUI");
        dbg(lvl_warning, "Driver Break plugin: Internal GUI not available");
        return 0;
    }

#ifndef HAVE_API_ANDROID
    if (!driver_break_osd_show_intervals_for_profile(gui_priv, priv, profile_name)) {
        navit_add_message(nav, "Driver Break plugin: Configuration not available for this profile");
        return 0;
    }
#endif

    return 1;
}
