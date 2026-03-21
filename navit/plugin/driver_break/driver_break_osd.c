/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
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

#include "driver_break_osd.h"
#include "attr.h"
#include "color.h"
#include "command.h"
#include "config.h"
#include "debug.h"
#include "driver_break_db.h"
#include "driver_break_finder.h"
#include "driver_break_poi.h"
#include "graphics.h"
#include "gui.h"
#include "item.h"
#include "map.h"
#include "mapset.h"
#include "navit.h"
#include "osd.h"
#include "plugin.h"
#include "point.h"
#include "popup.h"
#include "vehicleprofile.h"
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Internal GUI includes - needed for dialog creation */
#ifdef HAVE_API_ANDROID
/* Android doesn't use internal GUI */
#else
#    include "gui/internal/gui_internal.h"
#    include "gui/internal/gui_internal_menu.h"
#    include "gui/internal/gui_internal_priv.h"
#    include "gui/internal/gui_internal_widget.h"
#    include <dlfcn.h>

/* Runtime resolution of all gui_internal functions since gui_internal is a MODULE library */
/* Function pointers for all gui_internal functions we need */
static void (*gui_internal_back_ptr)(struct gui_priv *, struct widget *, void *) = NULL;
static void (*gui_internal_enter_ptr)(struct gui_priv *, int) = NULL;
static void (*gui_internal_leave_ptr)(struct gui_priv *) = NULL;
static void (*gui_internal_set_click_coord_ptr)(struct gui_priv *, struct point *) = NULL;
static void (*gui_internal_enter_setup_ptr)(struct gui_priv *) = NULL;
static struct widget *(*gui_internal_menu_ptr)(struct gui_priv *, const char *) = NULL;
static struct widget *(*gui_internal_box_new_ptr)(struct gui_priv *, enum flags) = NULL;
static struct widget *(*gui_internal_label_new_ptr)(struct gui_priv *, const char *) = NULL;
static struct widget *(*gui_internal_button_new_with_callback_ptr)(struct gui_priv *, const char *, struct widget *,
                                                                   enum flags,
                                                                   void (*)(struct gui_priv *, struct widget *, void *),
                                                                   void *) = NULL;
static void (*gui_internal_widget_append_ptr)(struct widget *, struct widget *) = NULL;
static void (*gui_internal_menu_render_ptr)(struct gui_priv *) = NULL;

static int gui_internal_functions_resolved = 0;

/* Resolve all gui_internal functions at runtime */
static void resolve_gui_internal_functions(void) {
    void *handle = NULL;
    const char *lib_paths[] = {"libgui_internal.so",
                               "libnavit_gui_internal.so",
                               "/usr/local/lib64/navit/gui/libgui_internal.so",
                               "/usr/local/lib/navit/gui/libgui_internal.so",
                               "/usr/lib64/navit/gui/libgui_internal.so",
                               "/usr/lib/navit/gui/libgui_internal.so",
                               "/usr/local/lib64/navit/libgui_internal.so",
                               "/usr/local/lib/navit/libgui_internal.so",
                               NULL};
    int i;
    char *error;

    if (gui_internal_functions_resolved)
        return;
    gui_internal_functions_resolved = 1;

    /* Clear any previous dlerror */
    dlerror();

    /* Try to open gui_internal library explicitly */
    for (i = 0; lib_paths[i] != NULL; i++) {
        handle = dlopen(lib_paths[i], RTLD_LAZY | RTLD_GLOBAL);
        if (handle) {
            dbg(lvl_debug, "Driver Break plugin: Opened gui_internal library: %s", lib_paths[i]);
            break;
        }
        error = dlerror();
        if (error) {
            dbg(lvl_debug, "Driver Break plugin: Failed to open %s: %s", lib_paths[i], error);
        }
    }

    /* If we couldn't open explicitly, try RTLD_DEFAULT */
    if (!handle) {
        dbg(lvl_debug, "Driver Break plugin: Could not open gui_internal library explicitly, trying RTLD_DEFAULT");
        handle = RTLD_DEFAULT;
    }

    /* Clear dlerror before resolving symbols */
    dlerror();

    gui_internal_back_ptr = (void (*)(struct gui_priv *, struct widget *, void *))dlsym(handle, "gui_internal_back");
    gui_internal_enter_ptr = (void (*)(struct gui_priv *, int))dlsym(handle, "gui_internal_enter");
    gui_internal_leave_ptr = (void (*)(struct gui_priv *))dlsym(handle, "gui_internal_leave");
    gui_internal_set_click_coord_ptr =
        (void (*)(struct gui_priv *, struct point *))dlsym(handle, "gui_internal_set_click_coord");
    gui_internal_enter_setup_ptr = (void (*)(struct gui_priv *))dlsym(handle, "gui_internal_enter_setup");
    gui_internal_menu_ptr = (struct widget * (*)(struct gui_priv *, const char *)) dlsym(handle, "gui_internal_menu");
    gui_internal_box_new_ptr = (struct widget * (*)(struct gui_priv *, enum flags))
        dlsym(handle, "gui_internal_box_new");
    gui_internal_label_new_ptr = (struct widget * (*)(struct gui_priv *, const char *))
        dlsym(handle, "gui_internal_label_new");
    gui_internal_button_new_with_callback_ptr = (struct widget
                                                 * (*)(struct gui_priv *, const char *, struct widget *, enum flags,
                                                       void (*)(struct gui_priv *, struct widget *, void *), void *))
        dlsym(handle, "gui_internal_button_new_with_callback");
    gui_internal_widget_append_ptr =
        (void (*)(struct widget *, struct widget *))dlsym(handle, "gui_internal_widget_append");
    gui_internal_menu_render_ptr = (void (*)(struct gui_priv *))dlsym(handle, "gui_internal_menu_render");

    error = dlerror();
    if (error) {
        dbg(lvl_debug, "Driver Break plugin: dlerror after symbol resolution: %s", error);
    }

    if (!gui_internal_menu_render_ptr) {
        dbg(lvl_error,
            "Driver Break plugin: Failed to resolve gui_internal functions - internal GUI may not be available");
        dbg(lvl_debug, "Driver Break plugin: Resolved: back=%p enter=%p menu=%p render=%p", gui_internal_back_ptr,
            gui_internal_enter_ptr, gui_internal_menu_ptr, gui_internal_menu_render_ptr);
    } else {
        dbg(lvl_debug, "Driver Break plugin: Successfully resolved all gui_internal functions");
    }
}

/* Wrapper macros to use resolved functions */
#    define gui_internal_back(p, w, d)                                                                                 \
        do {                                                                                                           \
            resolve_gui_internal_functions();                                                                          \
            if (gui_internal_back_ptr)                                                                                 \
                gui_internal_back_ptr(p, w, d);                                                                        \
        } while (0)
#    define gui_internal_enter(p, i)                                                                                   \
        do {                                                                                                           \
            resolve_gui_internal_functions();                                                                          \
            if (gui_internal_enter_ptr)                                                                                \
                gui_internal_enter_ptr(p, i);                                                                          \
        } while (0)
#    define gui_internal_leave(p)                                                                                      \
        do {                                                                                                           \
            resolve_gui_internal_functions();                                                                          \
            if (gui_internal_leave_ptr)                                                                                \
                gui_internal_leave_ptr(p);                                                                             \
        } while (0)
#    define gui_internal_set_click_coord(p, pt)                                                                        \
        do {                                                                                                           \
            resolve_gui_internal_functions();                                                                          \
            if (gui_internal_set_click_coord_ptr)                                                                      \
                gui_internal_set_click_coord_ptr(p, pt);                                                               \
        } while (0)
#    define gui_internal_enter_setup(p)                                                                                \
        do {                                                                                                           \
            resolve_gui_internal_functions();                                                                          \
            if (gui_internal_enter_setup_ptr)                                                                          \
                gui_internal_enter_setup_ptr(p);                                                                       \
        } while (0)
#    define gui_internal_menu(p, s)                                                                                    \
        (resolve_gui_internal_functions(), gui_internal_menu_ptr ? gui_internal_menu_ptr(p, s) : NULL)
#    define gui_internal_box_new(p, f)                                                                                 \
        (resolve_gui_internal_functions(), gui_internal_box_new_ptr ? gui_internal_box_new_ptr(p, f) : NULL)
#    define gui_internal_label_new(p, s)                                                                               \
        (resolve_gui_internal_functions(), gui_internal_label_new_ptr ? gui_internal_label_new_ptr(p, s) : NULL)
#    define gui_internal_button_new_with_callback(p, s, w, f, cb, d)                                                   \
        (resolve_gui_internal_functions(), gui_internal_button_new_with_callback_ptr                                   \
                                               ? gui_internal_button_new_with_callback_ptr(p, s, w, f, cb, d)          \
                                               : NULL)
#    define gui_internal_widget_append(w1, w2)                                                                         \
        do {                                                                                                           \
            resolve_gui_internal_functions();                                                                          \
            if (gui_internal_widget_append_ptr)                                                                        \
                gui_internal_widget_append_ptr(w1, w2);                                                                \
        } while (0)
#    define gui_internal_menu_render(p)                                                                                \
        do {                                                                                                           \
            resolve_gui_internal_functions();                                                                          \
            if (gui_internal_menu_render_ptr)                                                                          \
                gui_internal_menu_render_ptr(p);                                                                       \
        } while (0)
#endif

/* Command functions for menu integration */
int driver_break_cmd_suggest_stop(struct navit *nav, char *function, struct attr **in, struct attr ***out);
int driver_break_cmd_show_history(struct navit *nav, char *function, struct attr **in, struct attr ***out);
int driver_break_cmd_configure(struct navit *nav, char *function, struct attr **in, struct attr ***out);
int driver_break_cmd_start_break(struct navit *nav, char *function, struct attr **in, struct attr ***out);
int driver_break_cmd_end_break(struct navit *nav, char *function, struct attr **in, struct attr ***out);
int driver_break_cmd_configure_intervals(struct navit *nav, char *function, struct attr **in, struct attr ***out);
int driver_break_cmd_configure_overnight(struct navit *nav, char *function, struct attr **in, struct attr ***out);
int driver_break_cmd_set_fuel_level(struct navit *nav, char *function, struct attr **in, struct attr ***out);
int driver_break_cmd_log_fuel_stop(struct navit *nav, char *function, struct attr **in, struct attr ***out);
int driver_break_cmd_configure_fuel(struct navit *nav, char *function, struct attr **in, struct attr ***out);

/* Forward declaration for config save callback used by multiple dialogs. */
static void driver_break_save_config_callback(struct gui_priv *gui_priv, struct widget *widget, void *data);
static void driver_break_toggle_adaptive_learning_callback(struct gui_priv *gui_priv, struct widget *widget,
                                                           void *data);
static void driver_break_toggle_live_ecu_callback(struct gui_priv *gui_priv, struct widget *widget, void *data);
static void driver_break_show_motorcycle_intervals_dialog(struct gui_priv *gui_priv, struct driver_break_priv *priv);
static void driver_break_toggle_motorcycle_terrain_callback(struct gui_priv *gui_priv, struct widget *widget,
                                                            void *data);
static void driver_break_toggle_water_remote_arid_callback(struct gui_priv *gui_priv, struct widget *widget,
                                                           void *data);
static void driver_break_show_overnight_dialog(struct gui_priv *gui_priv, struct driver_break_priv *priv,
                                               const char *profile_name);

/* Profile name used when reopening overnight dialog from toggle (e.g. water remote/arid) */
static const char *s_overnight_profile_name;

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
    /* Backward compatibility aliases */
    {"rest_suggest_stop",                command_cast(driver_break_cmd_suggest_stop)       },
    {"rest_show_history",                command_cast(driver_break_cmd_show_history)       },
    {"rest_start_break",                 command_cast(driver_break_cmd_start_break)        },
    {"rest_end_break",                   command_cast(driver_break_cmd_end_break)          },
    {"rest_configure_intervals",         command_cast(driver_break_cmd_configure_intervals)},
    {"rest_configure_overnight",         command_cast(driver_break_cmd_configure_overnight)},
};

/* Register commands with Navit's command registry (same API as core OSDs).
 * Menu items must call these via navit.command_name() so the context is navit. */
void driver_break_register_commands(struct navit *nav) {
    if (!nav) {
        dbg(lvl_error, "Driver Break plugin: Cannot register commands - nav is NULL");
        return;
    }
    navit_command_add_table(nav, driver_break_commands, sizeof(driver_break_commands) / sizeof(struct command_table));
    dbg(lvl_info, "Driver Break plugin: Menu commands registered (count=%zu)",
        sizeof(driver_break_commands) / sizeof(struct command_table));
}

/* Static pointer to the driver_break_priv instance (there should only be one) */
/* This is set in driver_break_osd_new and accessed by command handlers */
struct driver_break_priv *driver_break_plugin_instance = NULL;

/* Get driver break plugin instance - returns the static instance */
static struct driver_break_priv *driver_break_get_plugin(struct navit *nav) {
    (void)nav; /* Unused, but kept for API consistency */
    if (!driver_break_plugin_instance) {
        dbg(lvl_warning, "Driver Break plugin: Plugin instance not found - OSD may not be instantiated");
    }
    return driver_break_plugin_instance;
}

/* Local struct matching gui.c's struct gui layout for accessing priv member */
struct gui_local {
    struct gui_methods meth;
    struct gui_priv *priv;
    struct attr **attrs;
    struct attr parent;
};

/* Get internal GUI priv from navit - returns NULL if not internal GUI */
struct gui_priv *driver_break_get_internal_gui_priv(struct navit *nav) {
    struct gui *gui;
    struct gui_local *gui_local;
    struct attr type_attr;

    gui = navit_get_gui(nav);
    if (!gui) {
        return NULL;
    }

    /* Check if this is the internal GUI */
    if (!gui_get_attr(gui, attr_type, &type_attr, NULL)) {
        return NULL;
    }

    if (g_strcmp0(type_attr.u.str, "internal") != 0) {
        return NULL;
    }

    /* Cast to local struct to access priv member */
    gui_local = (struct gui_local *)gui;
    return gui_local->priv;
}

/* Suggest rest stop command */
/* Show rest stop suggestions dialog */
static void driver_break_show_suggestions_dialog(struct gui_priv *gui_priv, struct driver_break_priv *priv,
                                                 GList *stops) {
    struct widget *menu, *box, *label;
    char buffer[512];
    GList *l;
    int count = 0;

    if (!priv) {
        dbg(lvl_error, "Driver Break plugin: driver_break_show_suggestions_dialog called with NULL priv");
        return;
    }

    graphics_draw_mode(gui_priv->gra, draw_mode_begin);
    gui_internal_enter(gui_priv, 1);
    gui_internal_set_click_coord(gui_priv, NULL);
    gui_internal_enter_setup(gui_priv);

    menu = gui_internal_menu(gui_priv, "Rest Stop Suggestions");
    box = gui_internal_box_new(gui_priv, gravity_left_top | orientation_vertical | flags_expand | flags_fill);
    gui_internal_widget_append(menu, box);

    if (!stops) {
        label = gui_internal_label_new(gui_priv, "No rest stops found along route");
        gui_internal_widget_append(box, label);
    } else {
        for (l = stops; l; l = g_list_next(l)) {
            struct driver_break_stop *stop = (struct driver_break_stop *)l->data;
            if (!stop)
                continue;

            count++;
            if (stop->name) {
                snprintf(buffer, sizeof(buffer), "%d. %s (%.1f km from route)", count, stop->name,
                         stop->distance_from_route / 1000.0);
            } else {
                snprintf(buffer, sizeof(buffer), "%d. Rest stop (%.1f km from route)", count,
                         stop->distance_from_route / 1000.0);
            }
            label = gui_internal_label_new(gui_priv, buffer);
            gui_internal_widget_append(box, label);
        }
    }

    gui_internal_menu_render(gui_priv);
    gui_internal_leave(gui_priv);
    graphics_draw_mode(gui_priv->gra, draw_mode_end);
}

int driver_break_cmd_suggest_stop(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    struct driver_break_priv *priv;
    struct route *route;
    struct attr route_attr;
    void *plugin;
    struct gui_priv *gui_priv;
    GList *stops = NULL;

    dbg(lvl_info, "Driver Break plugin: Suggest rest stop command");

    plugin = driver_break_get_plugin(nav);
    if (!plugin) {
        dbg(lvl_error, "Driver Break plugin: Plugin not found");
        navit_add_message(nav, "Driver Break plugin: Plugin not loaded");
        return 0;
    }

    priv = (struct driver_break_priv *)plugin;

    /* Use existing suggested stops if available, otherwise find new ones */
    if (priv->suggested_stops) {
        stops = priv->suggested_stops;
        dbg(lvl_info, "Driver Break plugin: Using existing suggested stops");
    } else {
        /* Get current route */
        if (!navit_get_attr(nav, attr_route, &route_attr, NULL)) {
            navit_add_message(nav, "Driver Break plugin: No active route");
            dbg(lvl_warning, "Driver Break plugin: No route found");
            return 0;
        }

        route = route_attr.u.route;
        if (!route) {
            navit_add_message(nav, "Driver Break plugin: No active route");
            return 0;
        }

        /* Find rest stops along route */
        stops = driver_break_finder_find_along_route(route, &priv->config, priv->session.mandatory_break_required);
    }

    /* Get internal GUI priv */
    gui_priv = driver_break_get_internal_gui_priv(nav);
    if (!gui_priv) {
        navit_add_message(nav, "Driver Break plugin: Suggestions dialog requires internal GUI");
        if (stops) {
            driver_break_finder_free_list(stops);
        }
        return 0;
    }

    /* Show suggestions dialog */
    driver_break_show_suggestions_dialog(gui_priv, priv, stops);

    /* Free stops list only if we created it (not if using existing suggested_stops) */
    if (stops && stops != priv->suggested_stops) {
        driver_break_finder_free_list(stops);
    }

    return 1;
}

/* Format a single history entry into buffer for display */
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
    } else {
        if (entry->name) {
            snprintf(buffer, buf_size, "%d. %s (%d min%s)", count, entry->name, entry->duration_minutes,
                     mandatory_suffix);
        } else {
            snprintf(buffer, buf_size, "%d. %.6f, %.6f (%d min%s)", count, entry->coord.lat, entry->coord.lng,
                     entry->duration_minutes, mandatory_suffix);
        }
    }
}

/* Show history dialog */
static void driver_break_show_history_dialog(struct gui_priv *gui_priv, struct driver_break_priv *priv,
                                             GList *history) {
    struct widget *menu, *box, *label;
    char buffer[512];
    GList *l;
    int count = 0;

    if (!priv) {
        dbg(lvl_error, "Driver Break plugin: driver_break_show_history_dialog called with NULL priv");
        return;
    }

    graphics_draw_mode(gui_priv->gra, draw_mode_begin);
    gui_internal_enter(gui_priv, 1);
    gui_internal_set_click_coord(gui_priv, NULL);
    gui_internal_enter_setup(gui_priv);

    menu = gui_internal_menu(gui_priv, "Rest Stop History");
    box = gui_internal_box_new(gui_priv, gravity_left_top | orientation_vertical | flags_expand | flags_fill);
    gui_internal_widget_append(menu, box);

    if (!history) {
        label = gui_internal_label_new(gui_priv, "No history entries found");
        gui_internal_widget_append(box, label);
    } else {
        for (l = history; l; l = g_list_next(l)) {
            struct driver_break_stop_history *entry = (struct driver_break_stop_history *)l->data;
            if (!entry) {
                continue;
            }
            count++;
            format_history_entry(entry, count, buffer, sizeof(buffer));
            label = gui_internal_label_new(gui_priv, buffer);
            gui_internal_widget_append(box, label);
        }
    }

    gui_internal_menu_render(gui_priv);
    gui_internal_leave(gui_priv);
    graphics_draw_mode(gui_priv->gra, draw_mode_end);
}

/* Show history command */
int driver_break_cmd_show_history(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    struct driver_break_priv *priv;
    void *plugin;
    struct gui_priv *gui_priv;
    GList *history = NULL;
    time_t since = 0; /* Get all history entries */

    dbg(lvl_info, "Driver Break plugin: Show history command");

    plugin = driver_break_get_plugin(nav);
    if (!plugin) {
        dbg(lvl_error, "Driver Break plugin: Plugin not found");
        navit_add_message(nav, "Driver Break plugin: Plugin not loaded");
        return 0;
    }

    priv = (struct driver_break_priv *)plugin;

    if (!priv->db) {
        navit_add_message(nav, "Driver Break plugin: Database not available");
        return 0;
    }

    /* Get history from database */
    history = driver_break_db_get_history(priv->db, since);

    /* Get internal GUI priv */
    gui_priv = driver_break_get_internal_gui_priv(nav);
    if (!gui_priv) {
        navit_add_message(nav, "Driver Break plugin: History dialog requires internal GUI");
        if (history) {
            g_list_free_full(history, (GDestroyNotify)driver_break_free_history_entry);
        }
        return 0;
    }

    /* Show history dialog */
    driver_break_show_history_dialog(gui_priv, priv, history);

    /* Free history list */
    if (history) {
        g_list_free_full(history, (GDestroyNotify)driver_break_free_history_entry);
    }

    return 1;
}

/* Configure command */
int driver_break_cmd_configure(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    dbg(lvl_info, "Driver Break plugin: Configure command");
    /* Placeholder for future global configuration menu (rest + fuel). */
    return 1;
}

/* Show fuel configuration summary dialog */
static void driver_break_show_fuel_config_dialog(struct gui_priv *gui_priv, struct driver_break_priv *priv) {
    struct widget *menu, *box, *label, *button;
    char buffer[256];
    int live_ecu_on;

    if (!priv) {
        dbg(lvl_error, "Driver Break plugin: driver_break_show_fuel_config_dialog called with NULL priv");
        return;
    }

    graphics_draw_mode(gui_priv->gra, draw_mode_begin);
    gui_internal_enter(gui_priv, 1);
    gui_internal_set_click_coord(gui_priv, NULL);
    gui_internal_enter_setup(gui_priv);

    menu = gui_internal_menu(gui_priv, "Fuel and Range Configuration");
    box = gui_internal_box_new(gui_priv, gravity_left_top | orientation_vertical | flags_expand | flags_fill);
    gui_internal_widget_append(menu, box);

    snprintf(buffer, sizeof(buffer), "Fuel type: %d", priv->config.fuel_type);
    label = gui_internal_label_new(gui_priv, buffer);
    gui_internal_widget_append(box, label);

    snprintf(buffer, sizeof(buffer), "Tank capacity: %d", priv->config.fuel_tank_capacity_l);
    label = gui_internal_label_new(gui_priv, buffer);
    gui_internal_widget_append(box, label);

    snprintf(buffer, sizeof(buffer), "Average consumption: %.1f L/100km", priv->config.fuel_avg_consumption_x10 / 10.0);
    label = gui_internal_label_new(gui_priv, buffer);
    gui_internal_widget_append(box, label);

    snprintf(buffer, sizeof(buffer), "Low fuel warning: %d km", priv->config.fuel_low_warning_km);
    label = gui_internal_label_new(gui_priv, buffer);
    gui_internal_widget_append(box, label);

    snprintf(buffer, sizeof(buffer), "Fuel search buffer: %d km", priv->config.fuel_search_buffer_km);
    label = gui_internal_label_new(gui_priv, buffer);
    gui_internal_widget_append(box, label);

    snprintf(buffer, sizeof(buffer), "High-load threshold: %d%%", priv->config.fuel_high_load_threshold);
    label = gui_internal_label_new(gui_priv, buffer);
    gui_internal_widget_append(box, label);

    live_ecu_on = priv->config.fuel_obd_available || priv->config.fuel_j1939_available
                  || priv->config.fuel_megasquirt_available;
    snprintf(buffer, sizeof(buffer), "Live ECU (OBD-II, J1939, MegaSquirt): %s", live_ecu_on ? "on" : "off");
    label = gui_internal_label_new(gui_priv, buffer);
    gui_internal_widget_append(box, label);
    button = gui_internal_button_new_with_callback(gui_priv, "Toggle live ECU", NULL, gravity_center | flags_fill,
                                                   driver_break_toggle_live_ecu_callback, priv);
    gui_internal_widget_append(box, button);

    snprintf(buffer, sizeof(buffer), "Adaptive fuel learning: %s",
             priv->config.fuel_adaptive_learning_enabled ? "on" : "off");
    label = gui_internal_label_new(gui_priv, buffer);
    gui_internal_widget_append(box, label);

    button = gui_internal_button_new_with_callback(gui_priv, "Toggle adaptive learning", NULL,
                                                   gravity_center | flags_fill,
                                                   driver_break_toggle_adaptive_learning_callback, priv);
    gui_internal_widget_append(box, button);

    if (priv->config.fuel_injector_flow_cc_min > 0) {
        snprintf(buffer, sizeof(buffer), "Injector flow: %d cc/min", priv->config.fuel_injector_flow_cc_min);
        label = gui_internal_label_new(gui_priv, buffer);
        gui_internal_widget_append(box, label);
    }

    label = gui_internal_label_new(
        gui_priv, "Note: Advanced editing of fuel parameters and adapters will be added in a later version.");
    gui_internal_widget_append(box, label);

    button = gui_internal_button_new_with_callback(gui_priv, "OK", NULL, gravity_center | flags_fill,
                                                   driver_break_save_config_callback, priv);
    gui_internal_widget_append(box, button);

    gui_internal_menu_render(gui_priv);
    gui_internal_leave(gui_priv);
    graphics_draw_mode(gui_priv->gra, draw_mode_end);
}

int driver_break_cmd_configure_fuel(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    struct driver_break_priv *priv;
    void *plugin;
    struct gui_priv *gui_priv;

    (void)function;
    (void)in;
    (void)out;

    dbg(lvl_info, "Driver Break plugin: Configure fuel command");

    plugin = driver_break_get_plugin(nav);
    if (!plugin) {
        dbg(lvl_error, "Driver Break plugin: Plugin not found");
        navit_add_message(nav, "Driver Break plugin: Plugin not loaded");
        return 0;
    }

    priv = (struct driver_break_priv *)plugin;

    gui_priv = driver_break_get_internal_gui_priv(nav);
    if (!gui_priv) {
        navit_add_message(nav, "Driver Break plugin: Fuel configuration dialog requires internal GUI");
        return 0;
    }

    driver_break_show_fuel_config_dialog(gui_priv, priv);
    return 1;
}

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

/* Set current fuel level command (manual entry, universal) */
int driver_break_cmd_set_fuel_level(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    struct driver_break_priv *priv;
    void *plugin;
    double value;

    (void)in;
    (void)out;

    dbg(lvl_info, "Driver Break plugin: Set fuel level command (function='%s')", function ? function : "");

    plugin = driver_break_get_plugin(nav);
    if (!plugin) {
        dbg(lvl_error, "Driver Break plugin: Plugin not found");
        navit_add_message(nav, "Driver Break plugin: Plugin not loaded");
        return 0;
    }

    priv = (struct driver_break_priv *)plugin;

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

/* Log a fuel stop (manual, all vehicles) */
int driver_break_cmd_log_fuel_stop(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    struct driver_break_priv *priv;
    void *plugin;
    struct driver_break_fuel_stop stop;
    struct attr position_attr;
    double added = 0.0;

    (void)in;
    (void)out;

    dbg(lvl_info, "Driver Break plugin: Log fuel stop command (function='%s')", function ? function : "");

    plugin = driver_break_get_plugin(nav);
    if (!plugin) {
        dbg(lvl_error, "Driver Break plugin: Plugin not found");
        navit_add_message(nav, "Driver Break plugin: Plugin not loaded");
        return 0;
    }

    priv = (struct driver_break_priv *)plugin;
    if (!priv->db) {
        navit_add_message(nav, "Driver Break plugin: Database not available");
        return 0;
    }

    if (function && *function) {
        if (!driver_break_parse_fuel_value(function, &added, "Driver Break plugin: Invalid fuel added value")) {
            return 0;
        }
    }

    /* Update current fuel level with added amount (if any) */
    priv->fuel_current += added;
    if (priv->config.fuel_tank_capacity_l > 0 && priv->fuel_current > priv->config.fuel_tank_capacity_l) {
        priv->fuel_current = priv->config.fuel_tank_capacity_l;
    }

    /* Fill stop structure */
    memset(&stop, 0, sizeof(stop));
    stop.timestamp = time(NULL);
    stop.fuel_added = added;
    stop.fuel_level_after = priv->fuel_current;
    stop.cost = 0.0;
    stop.currency = NULL;
    stop.ethanol_pct = priv->config.fuel_ethanol_manual_pct;

    /* Get current position */
    if (navit_get_attr(nav, attr_position_coord_geo, &position_attr, NULL) && position_attr.u.coord_geo) {
        stop.coord = *position_attr.u.coord_geo;
    } else {
        stop.coord.lat = 0.0;
        stop.coord.lng = 0.0;
    }

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

/* Start break command */
int driver_break_cmd_start_break(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    struct driver_break_priv *priv;
    void *plugin;
    struct attr position_attr;
    time_t now;

    dbg(lvl_info, "Driver Break plugin: Start break command");

    plugin = driver_break_get_plugin(nav);
    if (!plugin) {
        dbg(lvl_error, "Driver Break plugin: Plugin not found");
        navit_add_message(nav, "Driver Break plugin: Plugin not loaded");
        return 0;
    }

    priv = (struct driver_break_priv *)plugin;

    /* Check if break already in progress */
    if (priv->current_break_start_time != 0) {
        navit_add_message(nav, "Driver Break plugin: Break already in progress");
        dbg(lvl_warning, "Driver Break plugin: Attempted to start break while one is already active");
        return 0;
    }

    now = time(NULL);
    priv->current_break_start_time = now;

    /* Get current position */
    if (navit_get_attr(nav, attr_position_coord_geo, &position_attr, NULL)) {
        if (position_attr.u.coord_geo) {
            priv->current_break_location = *position_attr.u.coord_geo;
        }
    }

    dbg(lvl_info, "Driver Break plugin: Break started at %ld", (long)now);
    navit_add_message(nav, "Driver Break plugin: Break started");

    return 1;
}

/* End break command */
int driver_break_cmd_end_break(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    struct driver_break_priv *priv;
    void *plugin;
    time_t now;
    int duration_minutes;
    struct driver_break_stop_history *entry;

    dbg(lvl_info, "Driver Break plugin: End break command");

    plugin = driver_break_get_plugin(nav);
    if (!plugin) {
        dbg(lvl_error, "Driver Break plugin: Plugin not found");
        navit_add_message(nav, "Driver Break plugin: Plugin not loaded");
        return 0;
    }

    priv = (struct driver_break_priv *)plugin;

    /* Check if break is in progress */
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

    /* Create history entry */
    entry = g_new0(struct driver_break_stop_history, 1);
    if (!entry) {
        dbg(lvl_error, "Driver Break plugin: Out of memory");
        priv->current_break_start_time = 0;
        return 0;
    }

    entry->timestamp = priv->current_break_start_time;
    entry->coord = priv->current_break_location;
    entry->duration_minutes = duration_minutes;
    entry->was_mandatory = priv->session.mandatory_break_required;
    entry->name = NULL; /* Could be enhanced to get location name from map */

    /* Save to database */
    if (priv->db) {
        if (driver_break_db_add_history(priv->db, entry)) {
            dbg(lvl_info, "Driver Break plugin: Break saved to history: %d minutes", duration_minutes);
            navit_add_message(nav, "Driver Break plugin: Break ended and saved");
        } else {
            dbg(lvl_error, "Driver Break plugin: Failed to save break to history");
            navit_add_message(nav, "Driver Break plugin: Failed to save break");
        }
    }

    /* Update session - reset continuous driving time */
    priv->session.last_break_time = now;
    priv->session.continuous_driving_minutes = 0;
    priv->session.breaks_taken++;
    priv->session.mandatory_break_required = 0;

    /* Clear break start time */
    priv->current_break_start_time = 0;

    /* Free history entry */
    driver_break_free_history_entry(entry);

    dbg(lvl_info, "Driver Break plugin: Break ended after %d minutes", duration_minutes);

    return 1;
}

/* Callback: one toggle for live ECU (OBD-II, J1939, MegaSquirt) - all on or all off */
static void driver_break_toggle_live_ecu_callback(struct gui_priv *gui_priv, struct widget *widget, void *data) {
    struct driver_break_priv *priv = (struct driver_break_priv *)data;
    int any_on;
    if (!priv)
        return;
    any_on = priv->config.fuel_obd_available || priv->config.fuel_j1939_available
             || priv->config.fuel_megasquirt_available;
    priv->config.fuel_obd_available = any_on ? 0 : 1;
    priv->config.fuel_j1939_available = any_on ? 0 : 1;
    priv->config.fuel_megasquirt_available = any_on ? 0 : 1;
    gui_internal_back(gui_priv, NULL, NULL);
    driver_break_show_fuel_config_dialog(gui_priv, priv);
}

static void driver_break_toggle_adaptive_learning_callback(struct gui_priv *gui_priv, struct widget *widget,
                                                           void *data) {
    struct driver_break_priv *priv = (struct driver_break_priv *)data;
    if (!priv)
        return;
    priv->config.fuel_adaptive_learning_enabled = !priv->config.fuel_adaptive_learning_enabled;
    gui_internal_back(gui_priv, NULL, NULL);
    driver_break_show_fuel_config_dialog(gui_priv, priv);
}

static void driver_break_save_config_callback(struct gui_priv *gui_priv, struct widget *widget, void *data) {
    struct driver_break_priv *priv = (struct driver_break_priv *)data;
    if (priv && priv->db) {
        driver_break_db_save_config(priv->db, &priv->config);
        navit_add_message(priv->nav, "Driver Break plugin: Configuration saved");
    }
    gui_internal_back(gui_priv, NULL, NULL);
}

/* Show car interval configuration dialog */
static void driver_break_show_car_intervals_dialog(struct gui_priv *gui_priv, struct driver_break_priv *priv) {
    struct widget *menu, *box, *label, *button;
    char buffer[256];

    if (!priv) {
        dbg(lvl_error, "Driver Break plugin: driver_break_show_car_intervals_dialog called with NULL priv");
        return;
    }

    dbg(lvl_debug,
        "Driver Break plugin: Showing car intervals dialog - config values: soft_limit=%d, max=%d, break_interval=%d, "
        "break_duration=%d",
        priv->config.car_soft_limit_hours, priv->config.car_max_hours, priv->config.car_break_interval_hours,
        priv->config.car_break_duration_min);

    graphics_draw_mode(gui_priv->gra, draw_mode_begin);
    gui_internal_enter(gui_priv, 1);
    gui_internal_set_click_coord(gui_priv, NULL);
    gui_internal_enter_setup(gui_priv);

    menu = gui_internal_menu(gui_priv, "Car Rest Intervals");
    box = gui_internal_box_new(gui_priv, gravity_left_top | orientation_vertical | flags_expand | flags_fill);
    gui_internal_widget_append(menu, box);

    /* Soft limit hours - validate before displaying */
    {
        int val = priv->config.car_soft_limit_hours;
        if (val < 0 || val > 12) {
            dbg(lvl_error, "Driver Break plugin: Invalid car_soft_limit_hours value: %d, using default 7", val);
            val = 7;
            priv->config.car_soft_limit_hours = val;
        }
        snprintf(buffer, sizeof(buffer), "Soft limit: %d hours", val);
        label = gui_internal_label_new(gui_priv, buffer);
        gui_internal_widget_append(box, label);
    }

    /* Max hours - validate before displaying */
    {
        int val = priv->config.car_max_hours;
        if (val < 0 || val > 12) {
            dbg(lvl_error, "Driver Break plugin: Invalid car_max_hours value: %d, using default 10", val);
            val = 10;
            priv->config.car_max_hours = val;
        }
        snprintf(buffer, sizeof(buffer), "Max hours: %d hours", val);
        label = gui_internal_label_new(gui_priv, buffer);
        gui_internal_widget_append(box, label);
    }

    /* Break interval - validate before displaying */
    {
        int val = priv->config.car_break_interval_hours;
        if (val < 0 || val > 12) {
            dbg(lvl_error, "Driver Break plugin: Invalid car_break_interval_hours value: %d, using default 4", val);
            val = 4;
            priv->config.car_break_interval_hours = val;
        }
        snprintf(buffer, sizeof(buffer), "Break interval: %d hours", val);
        label = gui_internal_label_new(gui_priv, buffer);
        gui_internal_widget_append(box, label);
    }

    /* Break duration - validate before displaying */
    {
        int val = priv->config.car_break_duration_min;
        if (val < 0 || val > 120) {
            dbg(lvl_error, "Driver Break plugin: Invalid car_break_duration_min value: %d, using default 30", val);
            val = 30;
            priv->config.car_break_duration_min = val;
        }
        snprintf(buffer, sizeof(buffer), "Break duration: %d minutes", val);
        label = gui_internal_label_new(gui_priv, buffer);
        gui_internal_widget_append(box, label);
    }

    /* Note about editing */
    label = gui_internal_label_new(gui_priv, "Note: Advanced editing coming soon");
    gui_internal_widget_append(box, label);

    /* Save button */
    button = gui_internal_button_new_with_callback(gui_priv, "OK", NULL, gravity_center | flags_fill,
                                                   driver_break_save_config_callback, priv);
    gui_internal_widget_append(box, button);

    gui_internal_menu_render(gui_priv);
    gui_internal_leave(gui_priv);
    graphics_draw_mode(gui_priv->gra, draw_mode_end);
}

/* Show truck interval configuration dialog */
static void driver_break_show_truck_intervals_dialog(struct gui_priv *gui_priv, struct driver_break_priv *priv) {
    struct widget *menu, *box, *label, *button;
    char buffer[256];

    graphics_draw_mode(gui_priv->gra, draw_mode_begin);
    gui_internal_enter(gui_priv, 1);
    gui_internal_set_click_coord(gui_priv, NULL);
    gui_internal_enter_setup(gui_priv);

    menu = gui_internal_menu(gui_priv, "Truck Rest Intervals");
    box = gui_internal_box_new(gui_priv, gravity_left_top | orientation_vertical | flags_expand | flags_fill);
    gui_internal_widget_append(menu, box);

    /* Mandatory break after hours */
    snprintf(buffer, sizeof(buffer), "Mandatory break after: %.1f hours",
             (double)priv->config.truck_mandatory_break_after_hours);
    label = gui_internal_label_new(gui_priv, buffer);
    gui_internal_widget_append(box, label);

    /* Mandatory break duration */
    snprintf(buffer, sizeof(buffer), "Break duration: %d minutes", priv->config.truck_mandatory_break_duration_min);
    label = gui_internal_label_new(gui_priv, buffer);
    gui_internal_widget_append(box, label);

    /* Max daily hours */
    snprintf(buffer, sizeof(buffer), "Max daily hours: %d hours", priv->config.truck_max_daily_hours);
    label = gui_internal_label_new(gui_priv, buffer);
    gui_internal_widget_append(box, label);

    /* Note about editing */
    label = gui_internal_label_new(gui_priv, "Note: Advanced editing coming soon");
    gui_internal_widget_append(box, label);

    /* Save button */
    button = gui_internal_button_new_with_callback(gui_priv, "OK", NULL, gravity_center | flags_fill,
                                                   driver_break_save_config_callback, priv);
    gui_internal_widget_append(box, button);

    gui_internal_menu_render(gui_priv);
    gui_internal_leave(gui_priv);
    graphics_draw_mode(gui_priv->gra, draw_mode_end);
}

/* Show hiking interval configuration dialog */
static void driver_break_show_hiking_intervals_dialog(struct gui_priv *gui_priv, struct driver_break_priv *priv) {
    struct widget *menu, *box, *label, *button;
    char buffer[256];

    graphics_draw_mode(gui_priv->gra, draw_mode_begin);
    gui_internal_enter(gui_priv, 1);
    gui_internal_set_click_coord(gui_priv, NULL);
    gui_internal_enter_setup(gui_priv);

    menu = gui_internal_menu(gui_priv, "Hiking Rest Intervals");
    box = gui_internal_box_new(gui_priv, gravity_left_top | orientation_vertical | flags_expand | flags_fill);
    gui_internal_widget_append(menu, box);

    /* Main rest distance */
    snprintf(buffer, sizeof(buffer), "Main rest distance: %.2f km", priv->config.hiking_driver_break_distance_main);
    label = gui_internal_label_new(gui_priv, buffer);
    gui_internal_widget_append(box, label);

    /* Alternative rest distance */
    snprintf(buffer, sizeof(buffer), "Alternative rest: %.2f km", priv->config.hiking_driver_break_distance_alt);
    label = gui_internal_label_new(gui_priv, buffer);
    gui_internal_widget_append(box, label);

    /* Max daily distance */
    snprintf(buffer, sizeof(buffer), "Max daily distance: %.1f km", priv->config.hiking_max_daily_distance);
    label = gui_internal_label_new(gui_priv, buffer);
    gui_internal_widget_append(box, label);

    /* Note about editing */
    label = gui_internal_label_new(gui_priv, "Note: Advanced editing coming soon");
    gui_internal_widget_append(box, label);

    /* Save button */
    button = gui_internal_button_new_with_callback(gui_priv, "OK", NULL, gravity_center | flags_fill,
                                                   driver_break_save_config_callback, priv);
    gui_internal_widget_append(box, button);

    gui_internal_menu_render(gui_priv);
    gui_internal_leave(gui_priv);
    graphics_draw_mode(gui_priv->gra, draw_mode_end);
}

/* Show cycling interval configuration dialog */
static void driver_break_show_cycling_intervals_dialog(struct gui_priv *gui_priv, struct driver_break_priv *priv) {
    struct widget *menu, *box, *label, *button;
    char buffer[256];

    graphics_draw_mode(gui_priv->gra, draw_mode_begin);
    gui_internal_enter(gui_priv, 1);
    gui_internal_set_click_coord(gui_priv, NULL);
    gui_internal_enter_setup(gui_priv);

    menu = gui_internal_menu(gui_priv, "Cycling Rest Intervals");
    box = gui_internal_box_new(gui_priv, gravity_left_top | orientation_vertical | flags_expand | flags_fill);
    gui_internal_widget_append(menu, box);

    /* Main rest distance */
    snprintf(buffer, sizeof(buffer), "Main rest distance: %.2f km", priv->config.cycling_driver_break_distance_main);
    label = gui_internal_label_new(gui_priv, buffer);
    gui_internal_widget_append(box, label);

    /* Alternative rest distance */
    snprintf(buffer, sizeof(buffer), "Alternative rest: %.2f km", priv->config.cycling_driver_break_distance_alt);
    label = gui_internal_label_new(gui_priv, buffer);
    gui_internal_widget_append(box, label);

    /* Max daily distance */
    snprintf(buffer, sizeof(buffer), "Max daily distance: %.1f km", priv->config.cycling_max_daily_distance);
    label = gui_internal_label_new(gui_priv, buffer);
    gui_internal_widget_append(box, label);

    /* Note about editing */
    label = gui_internal_label_new(gui_priv, "Note: Advanced editing coming soon");
    gui_internal_widget_append(box, label);

    /* Save button */
    button = gui_internal_button_new_with_callback(gui_priv, "OK", NULL, gravity_center | flags_fill,
                                                   driver_break_save_config_callback, priv);
    gui_internal_widget_append(box, button);

    gui_internal_menu_render(gui_priv);
    gui_internal_leave(gui_priv);
    graphics_draw_mode(gui_priv->gra, draw_mode_end);
}

static void driver_break_toggle_motorcycle_terrain_callback(struct gui_priv *gui_priv, struct widget *widget,
                                                            void *data) {
    struct driver_break_priv *priv = (struct driver_break_priv *)data;
    if (!priv)
        return;
    priv->config.motorcycle_terrain_subtype = !priv->config.motorcycle_terrain_subtype;
    gui_internal_back(gui_priv, NULL, NULL);
    driver_break_show_motorcycle_intervals_dialog(gui_priv, priv);
}

static void driver_break_toggle_water_remote_arid_callback(struct gui_priv *gui_priv, struct widget *widget,
                                                           void *data) {
    struct driver_break_priv *priv = (struct driver_break_priv *)data;
    if (!priv)
        return;
    priv->config.enable_water_pois_remote_arid = !priv->config.enable_water_pois_remote_arid;
    gui_internal_back(gui_priv, NULL, NULL);
    driver_break_show_overnight_dialog(gui_priv, priv, s_overnight_profile_name ? s_overnight_profile_name : "car");
}

/* Show motorcycle interval configuration dialog */
static void driver_break_show_motorcycle_intervals_dialog(struct gui_priv *gui_priv, struct driver_break_priv *priv) {
    struct widget *menu, *box, *label, *button;
    char buffer[256];

    if (!priv) {
        dbg(lvl_error, "Driver Break plugin: driver_break_show_motorcycle_intervals_dialog called with NULL priv");
        return;
    }

    graphics_draw_mode(gui_priv->gra, draw_mode_begin);
    gui_internal_enter(gui_priv, 1);
    gui_internal_set_click_coord(gui_priv, NULL);
    gui_internal_enter_setup(gui_priv);

    menu = gui_internal_menu(gui_priv, "Motorcycle Rest Intervals");
    box = gui_internal_box_new(gui_priv, gravity_left_top | orientation_vertical | flags_expand | flags_fill);
    gui_internal_widget_append(menu, box);

    snprintf(buffer, sizeof(buffer), "Soft limit: %d min", priv->config.motorcycle_soft_limit_minutes);
    label = gui_internal_label_new(gui_priv, buffer);
    gui_internal_widget_append(box, label);

    snprintf(buffer, sizeof(buffer), "Mandatory break after: %d min",
             priv->config.motorcycle_mandatory_break_after_minutes);
    label = gui_internal_label_new(gui_priv, buffer);
    gui_internal_widget_append(box, label);

    snprintf(buffer, sizeof(buffer), "Break duration: %d min", priv->config.motorcycle_break_duration_min);
    label = gui_internal_label_new(gui_priv, buffer);
    gui_internal_widget_append(box, label);

    snprintf(buffer, sizeof(buffer), "Terrain: %s", priv->config.motorcycle_terrain_subtype ? "Adventure" : "Road");
    label = gui_internal_label_new(gui_priv, buffer);
    gui_internal_widget_append(box, label);

    button = gui_internal_button_new_with_callback(gui_priv, "Toggle terrain (Road/Adventure)", NULL,
                                                   gravity_center | flags_fill,
                                                   driver_break_toggle_motorcycle_terrain_callback, priv);
    gui_internal_widget_append(box, button);

    if (priv->config.motorcycle_terrain_subtype) {
        label = gui_internal_label_new(gui_priv,
                                       "Adventure mode: Off-road motor traffic on uncultivated land is prohibited by "
                                       "law in many countries (e.g. Norway, Sweden, Finland). Use only ways with "
                                       "explicit access. Permits may be required.");
        gui_internal_widget_append(box, label);
    }

    button = gui_internal_button_new_with_callback(gui_priv, "OK", NULL, gravity_center | flags_fill,
                                                   driver_break_save_config_callback, priv);
    gui_internal_widget_append(box, button);

    gui_internal_menu_render(gui_priv);
    gui_internal_leave(gui_priv);
    graphics_draw_mode(gui_priv->gra, draw_mode_end);
}

/* Configure rest stop intervals command */
int driver_break_cmd_configure_intervals(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    struct driver_break_priv *priv;
    struct vehicleprofile *profile;
    struct attr profile_attr;
    char *profile_name;
    void *plugin;
    struct gui_priv *gui_priv;

    dbg(lvl_info, "Driver Break plugin: driver_break_cmd_configure_intervals called");
    plugin = driver_break_get_plugin(nav);
    if (!plugin) {
        dbg(lvl_error, "Driver Break plugin: Plugin not found - OSD may not be instantiated. Check if <osd "
                       "type=\"rest\" enabled=\"yes\"/> is in config.");
        navit_add_message(nav, "Driver Break plugin: Plugin not loaded. Please check configuration.");
        return 0;
    }

    priv = (struct driver_break_priv *)plugin;

    /* Get current vehicle profile */
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
    dbg(lvl_info, "Driver Break plugin: Configure intervals for profile: %s", profile_name);

    /* Get internal GUI priv - if not available, show message */
    gui_priv = driver_break_get_internal_gui_priv(nav);
    if (!gui_priv) {
        navit_add_message(nav, "Driver Break plugin: Configuration dialog requires internal GUI");
        dbg(lvl_warning, "Driver Break plugin: Internal GUI not available");
        return 0;
    }

    /* Show configuration dialog based on profile */
    if (!g_ascii_strcasecmp(profile_name, "car")) {
        driver_break_show_car_intervals_dialog(gui_priv, priv);
    } else if (!g_ascii_strcasecmp(profile_name, "truck") || !g_ascii_strcasecmp(profile_name, "Truck")) {
        driver_break_show_truck_intervals_dialog(gui_priv, priv);
    } else if (!g_ascii_strcasecmp(profile_name, "hiking") || !g_ascii_strcasecmp(profile_name, "pedestrian")) {
        driver_break_show_hiking_intervals_dialog(gui_priv, priv);
    } else if (!g_ascii_strcasecmp(profile_name, "bike") || !g_ascii_strcasecmp(profile_name, "cycling")) {
        driver_break_show_cycling_intervals_dialog(gui_priv, priv);
    } else if (!g_ascii_strcasecmp(profile_name, "motorcycle")) {
        driver_break_show_motorcycle_intervals_dialog(gui_priv, priv);
    } else {
        dbg(lvl_warning, "Driver Break plugin: Unknown profile type: %s", profile_name);
        navit_add_message(nav, "Driver Break plugin: Configuration not available for this profile");
        return 0;
    }

    return 1;
}

/* Show overnight stops configuration dialog */
static void driver_break_show_overnight_dialog(struct gui_priv *gui_priv, struct driver_break_priv *priv,
                                               const char *profile_name) {
    struct widget *menu, *box, *label, *button;
    char buffer[256];
    char title[128];

    s_overnight_profile_name = profile_name;
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

    graphics_draw_mode(gui_priv->gra, draw_mode_begin);
    gui_internal_enter(gui_priv, 1);
    gui_internal_set_click_coord(gui_priv, NULL);
    gui_internal_enter_setup(gui_priv);

    menu = gui_internal_menu(gui_priv, title);
    box = gui_internal_box_new(gui_priv, gravity_left_top | orientation_vertical | flags_expand | flags_fill);
    gui_internal_widget_append(menu, box);

    /* Min distance from buildings - validate before displaying */
    {
        int val = priv->config.min_distance_from_buildings;
        if (val < 0 || val > 10000) {
            dbg(lvl_error, "Driver Break plugin: Invalid min_distance_from_buildings value: %d, using default 150",
                val);
            val = 150;
            priv->config.min_distance_from_buildings = val;
        }
        snprintf(buffer, sizeof(buffer), "Min distance from buildings: %d m", val);
        label = gui_internal_label_new(gui_priv, buffer);
        gui_internal_widget_append(box, label);
    }

    /* Min distance from glaciers (for hiking) */
    if (!g_ascii_strcasecmp(profile_name, "hiking") || !g_ascii_strcasecmp(profile_name, "pedestrian")) {
        int val = priv->config.min_distance_from_glaciers;
        if (val < 0 || val > 10000) {
            dbg(lvl_error, "Driver Break plugin: Invalid min_distance_from_glaciers value: %d, using default 300", val);
            val = 300;
            priv->config.min_distance_from_glaciers = val;
        }
        snprintf(buffer, sizeof(buffer), "Min distance from glaciers: %d m", val);
        label = gui_internal_label_new(gui_priv, buffer);
        gui_internal_widget_append(box, label);
    }

    /* POI search radius - validate before displaying */
    {
        int val = priv->config.poi_search_radius_km;
        if (val < 0 || val > 1000) {
            dbg(lvl_error, "Driver Break plugin: Invalid poi_search_radius_km value: %d, using default 15", val);
            val = 15;
            priv->config.poi_search_radius_km = val;
        }
        snprintf(buffer, sizeof(buffer), "POI search radius: %d km", val);
        label = gui_internal_label_new(gui_priv, buffer);
        gui_internal_widget_append(box, label);
    }

    /* Water search radius - validate before displaying */
    {
        int val = priv->config.poi_water_search_radius_km;
        if (val < 0 || val > 100) {
            dbg(lvl_error, "Driver Break plugin: Invalid poi_water_search_radius_km value: %d, using default 2", val);
            val = 2;
            priv->config.poi_water_search_radius_km = val;
        }
        snprintf(buffer, sizeof(buffer), "Water search radius: %d km", val);
        label = gui_internal_label_new(gui_priv, buffer);
        gui_internal_widget_append(box, label);
    }

    /* Cabin search radius - validate before displaying */
    {
        int val = priv->config.poi_cabin_search_radius_km;
        if (val < 0 || val > 100) {
            dbg(lvl_error, "Driver Break plugin: Invalid poi_cabin_search_radius_km value: %d, using default 5", val);
            val = 5;
            priv->config.poi_cabin_search_radius_km = val;
        }
        snprintf(buffer, sizeof(buffer), "Cabin search radius: %d km", val);
        label = gui_internal_label_new(gui_priv, buffer);
        gui_internal_widget_append(box, label);
    }

    /* Water POIs for car/truck/motorcycle (remote/arid/hot) */
    snprintf(buffer, sizeof(buffer), "Water POIs at rest stops (car/truck/motorcycle): %s",
             priv->config.enable_water_pois_remote_arid ? "on" : "off");
    label = gui_internal_label_new(gui_priv, buffer);
    gui_internal_widget_append(box, label);
    button = gui_internal_button_new_with_callback(gui_priv, "Toggle water POIs (remote/arid/hot)", NULL,
                                                   gravity_center | flags_fill,
                                                   driver_break_toggle_water_remote_arid_callback, priv);
    gui_internal_widget_append(box, button);

    /* Note about editing */
    label = gui_internal_label_new(gui_priv, "Note: Advanced editing coming soon");
    gui_internal_widget_append(box, label);

    /* Save button */
    button = gui_internal_button_new_with_callback(gui_priv, "OK", NULL, gravity_center | flags_fill,
                                                   driver_break_save_config_callback, priv);
    gui_internal_widget_append(box, button);

    gui_internal_menu_render(gui_priv);
    gui_internal_leave(gui_priv);
    graphics_draw_mode(gui_priv->gra, draw_mode_end);
}

/* Configure overnight stops command */
int driver_break_cmd_configure_overnight(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    struct driver_break_priv *priv;
    struct vehicleprofile *profile;
    struct attr profile_attr;
    char *profile_name;
    void *plugin;
    struct gui_priv *gui_priv;

    dbg(lvl_info, "Driver Break plugin: driver_break_cmd_configure_overnight called");
    plugin = driver_break_get_plugin(nav);
    if (!plugin) {
        dbg(lvl_error, "Driver Break plugin: Plugin not found - OSD may not be instantiated. Check if <osd "
                       "type=\"rest\" enabled=\"yes\"/> is in config.");
        navit_add_message(nav, "Driver Break plugin: Plugin not loaded. Please check configuration.");
        return 0;
    }

    priv = (struct driver_break_priv *)plugin;

    /* Get current vehicle profile */
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

    /* Get internal GUI priv - if not available, show message */
    gui_priv = driver_break_get_internal_gui_priv(nav);
    if (!gui_priv) {
        navit_add_message(nav, "Driver Break plugin: Configuration dialog requires internal GUI");
        dbg(lvl_warning, "Driver Break plugin: Internal GUI not available");
        return 0;
    }

    /* Show configuration dialog */
    driver_break_show_overnight_dialog(gui_priv, priv, profile_name);

    return 1;
}
