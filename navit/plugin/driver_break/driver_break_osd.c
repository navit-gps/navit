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

#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
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
#include "gui.h"
#include "osd.h"
#include "popup.h"
#include "rest.h"
#include "driver_break_db.h"
#include "driver_break_finder.h"
#include "driver_break_poi.h"
#include "driver_break_osd.h"
#include "vehicleprofile.h"
#include "gui.h"
#include "graphics.h"

/* Internal GUI includes - needed for dialog creation */
#ifdef HAVE_API_ANDROID
/* Android doesn't use internal GUI */
#else
#include "gui/internal/gui_internal.h"
#include "gui/internal/gui_internal_widget.h"
#include "gui/internal/gui_internal_menu.h"
#include "gui/internal/gui_internal_priv.h"
#include <dlfcn.h>

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
static struct widget *(*gui_internal_button_new_with_callback_ptr)(struct gui_priv *, const char *, struct widget *, enum flags, void (*)(struct gui_priv *, struct widget *, void *), void *) = NULL;
static void (*gui_internal_widget_append_ptr)(struct widget *, struct widget *) = NULL;
static void (*gui_internal_menu_render_ptr)(struct gui_priv *) = NULL;

static int gui_internal_functions_resolved = 0;

/* Resolve all gui_internal functions at runtime */
static void resolve_gui_internal_functions(void) {
    void *handle = NULL;
    const char *lib_paths[] = {
        "libgui_internal.so",
        "libnavit_gui_internal.so",
        "/usr/local/lib64/navit/gui/libgui_internal.so",
        "/usr/local/lib/navit/gui/libgui_internal.so",
        "/usr/lib64/navit/gui/libgui_internal.so",
        "/usr/lib/navit/gui/libgui_internal.so",
        "/usr/local/lib64/navit/libgui_internal.so",
        "/usr/local/lib/navit/libgui_internal.so",
        NULL
    };
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
    gui_internal_set_click_coord_ptr = (void (*)(struct gui_priv *, struct point *))dlsym(handle, "gui_internal_set_click_coord");
    gui_internal_enter_setup_ptr = (void (*)(struct gui_priv *))dlsym(handle, "gui_internal_enter_setup");
    gui_internal_menu_ptr = (struct widget *(*)(struct gui_priv *, const char *))dlsym(handle, "gui_internal_menu");
    gui_internal_box_new_ptr = (struct widget *(*)(struct gui_priv *, enum flags))dlsym(handle, "gui_internal_box_new");
    gui_internal_label_new_ptr = (struct widget *(*)(struct gui_priv *, const char *))dlsym(handle, "gui_internal_label_new");
    gui_internal_button_new_with_callback_ptr = (struct widget *(*)(struct gui_priv *, const char *, struct widget *, enum flags, void (*)(struct gui_priv *, struct widget *, void *), void *))dlsym(handle, "gui_internal_button_new_with_callback");
    gui_internal_widget_append_ptr = (void (*)(struct widget *, struct widget *))dlsym(handle, "gui_internal_widget_append");
    gui_internal_menu_render_ptr = (void (*)(struct gui_priv *))dlsym(handle, "gui_internal_menu_render");
    
    error = dlerror();
    if (error) {
        dbg(lvl_debug, "Driver Break plugin: dlerror after symbol resolution: %s", error);
    }
    
    if (!gui_internal_menu_render_ptr) {
        dbg(lvl_error, "Driver Break plugin: Failed to resolve gui_internal functions - internal GUI may not be available");
        dbg(lvl_debug, "Driver Break plugin: Resolved: back=%p enter=%p menu=%p render=%p", 
            gui_internal_back_ptr, gui_internal_enter_ptr, gui_internal_menu_ptr, gui_internal_menu_render_ptr);
    } else {
        dbg(lvl_debug, "Driver Break plugin: Successfully resolved all gui_internal functions");
    }
}

/* Wrapper macros to use resolved functions */
#define gui_internal_back(p, w, d) do { resolve_gui_internal_functions(); if (gui_internal_back_ptr) gui_internal_back_ptr(p, w, d); } while(0)
#define gui_internal_enter(p, i) do { resolve_gui_internal_functions(); if (gui_internal_enter_ptr) gui_internal_enter_ptr(p, i); } while(0)
#define gui_internal_leave(p) do { resolve_gui_internal_functions(); if (gui_internal_leave_ptr) gui_internal_leave_ptr(p); } while(0)
#define gui_internal_set_click_coord(p, pt) do { resolve_gui_internal_functions(); if (gui_internal_set_click_coord_ptr) gui_internal_set_click_coord_ptr(p, pt); } while(0)
#define gui_internal_enter_setup(p) do { resolve_gui_internal_functions(); if (gui_internal_enter_setup_ptr) gui_internal_enter_setup_ptr(p); } while(0)
#define gui_internal_menu(p, s) (resolve_gui_internal_functions(), gui_internal_menu_ptr ? gui_internal_menu_ptr(p, s) : NULL)
#define gui_internal_box_new(p, f) (resolve_gui_internal_functions(), gui_internal_box_new_ptr ? gui_internal_box_new_ptr(p, f) : NULL)
#define gui_internal_label_new(p, s) (resolve_gui_internal_functions(), gui_internal_label_new_ptr ? gui_internal_label_new_ptr(p, s) : NULL)
#define gui_internal_button_new_with_callback(p, s, w, f, cb, d) (resolve_gui_internal_functions(), gui_internal_button_new_with_callback_ptr ? gui_internal_button_new_with_callback_ptr(p, s, w, f, cb, d) : NULL)
#define gui_internal_widget_append(w1, w2) do { resolve_gui_internal_functions(); if (gui_internal_widget_append_ptr) gui_internal_widget_append_ptr(w1, w2); } while(0)
#define gui_internal_menu_render(p) do { resolve_gui_internal_functions(); if (gui_internal_menu_render_ptr) gui_internal_menu_render_ptr(p); } while(0)
#endif

/* Command functions for menu integration */
int driver_break_cmd_suggest_stop(struct navit *nav, char *function, struct attr **in, struct attr ***out);
int driver_break_cmd_show_history(struct navit *nav, char *function, struct attr **in, struct attr ***out);
int driver_break_cmd_configure(struct navit *nav, char *function, struct attr **in, struct attr ***out);
int driver_break_cmd_start_break(struct navit *nav, char *function, struct attr **in, struct attr ***out);
int driver_break_cmd_end_break(struct navit *nav, char *function, struct attr **in, struct attr ***out);
int driver_break_cmd_configure_intervals(struct navit *nav, char *function, struct attr **in, struct attr ***out);
int driver_break_cmd_configure_overnight(struct navit *nav, char *function, struct attr **in, struct attr ***out);

static struct command_table driver_break_commands[] = {
    {"driver_break_suggest_stop", command_cast(driver_break_cmd_suggest_stop)},
    {"driver_break_show_history", command_cast(driver_break_cmd_show_history)},
    {"driver_break_configure", command_cast(driver_break_cmd_configure)},
    {"driver_break_start_break", command_cast(driver_break_cmd_start_break)},
    {"driver_break_end_break", command_cast(driver_break_cmd_end_break)},
    {"driver_break_configure_intervals", command_cast(driver_break_cmd_configure_intervals)},
    {"driver_break_configure_overnight", command_cast(driver_break_cmd_configure_overnight)},
};

/* Register commands in driver_break_osd_new */
void driver_break_register_commands(struct navit *nav) {
    struct attr gui_attr;
    struct attr cbl_attr;
    struct attr cmd_attr;
    
    if (!nav) {
        dbg(lvl_error, "Driver Break plugin: Cannot register commands - nav is NULL");
        return;
    }
    
    /* Get GUI from navit - commands are called from GUI context */
    if (!navit_get_attr(nav, attr_gui, &gui_attr, NULL)) {
        dbg(lvl_error, "Driver Break plugin: navit does not have a GUI");
        return;
    }
    
    /* Get GUI's callback list */
    if (!gui_get_attr(gui_attr.u.gui, attr_callback_list, &cbl_attr, NULL)) {
        dbg(lvl_error, "Driver Break plugin: GUI does not have attr_callback_list");
        return;
    }
    dbg(lvl_debug, "Driver Break plugin: Found GUI callback_list at %p", cbl_attr.u.callback_list);
    
    /* Create command table callback */
    command_add_table_attr(driver_break_commands, sizeof(driver_break_commands)/sizeof(struct command_table), 
                          nav, &cmd_attr);
    
    if (cmd_attr.type != attr_callback) {
        dbg(lvl_error, "Driver Break plugin: command_add_table_attr failed - attr type is %d, expected %d", 
            cmd_attr.type, attr_callback);
        return;
    }
    
    /* Add callback to GUI's callback list (not navit's) */
    dbg(lvl_debug, "Driver Break plugin: Adding callback %p to GUI callback list", cmd_attr.u.callback);
    callback_list_add(cbl_attr.u.callback_list, cmd_attr.u.callback);
    
    dbg(lvl_info, "Driver Break plugin: Menu commands registered (count=%zu)", 
        sizeof(driver_break_commands)/sizeof(struct command_table));
    dbg(lvl_debug, "Driver Break plugin: Registered commands: driver_break_configure_intervals, driver_break_configure_overnight");
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
static struct gui_priv *driver_break_get_internal_gui_priv(struct navit *nav) {
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
int driver_break_cmd_suggest_stop(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    dbg(lvl_info, "Driver Break plugin: Suggest rest stop command");
    /* This would show a dialog with rest stop suggestions */
    return 1;
}

/* Show history command */
int driver_break_cmd_show_history(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    dbg(lvl_info, "Driver Break plugin: Show history command");
    /* This would display rest stop history */
    return 1;
}

/* Configure command */
int driver_break_cmd_configure(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    dbg(lvl_info, "Driver Break plugin: Configure command");
    /* This would open configuration menu */
    return 1;
}

/* Start break command */
int driver_break_cmd_start_break(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    void *plugin = driver_break_get_plugin(nav);
    if (plugin) {
        /* Record break start time */
        dbg(lvl_info, "Driver Break plugin: Break started");
    }
    return 1;
}

/* End break command */
int driver_break_cmd_end_break(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    void *plugin = driver_break_get_plugin(nav);
    if (plugin) {
        /* Record break end time and save to history */
        dbg(lvl_info, "Driver Break plugin: Break ended");
    }
    return 1;
}

/* Callback to save configuration after dialog interaction */
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
    
    dbg(lvl_debug, "Driver Break plugin: Showing car intervals dialog - config values: soft_limit=%d, max=%d, break_interval=%d, break_duration=%d",
        priv->config.car_soft_limit_hours, priv->config.car_max_hours, 
        priv->config.car_break_interval_hours, priv->config.car_break_duration_min);
    
    graphics_draw_mode(gui_priv->gra, draw_mode_begin);
    gui_internal_enter(gui_priv, 1);
    gui_internal_set_click_coord(gui_priv, NULL);
    gui_internal_enter_setup(gui_priv);
    
    menu = gui_internal_menu(gui_priv, "Car Rest Intervals");
    box = gui_internal_box_new(gui_priv, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
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
    button = gui_internal_button_new_with_callback(gui_priv, "OK", NULL, 
        gravity_center|flags_fill, driver_break_save_config_callback, priv);
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
    box = gui_internal_box_new(gui_priv, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
    gui_internal_widget_append(menu, box);
    
    /* Mandatory break after hours */
    snprintf(buffer, sizeof(buffer), "Mandatory break after: %.1f hours", 
        (double)priv->config.truck_mandatory_break_after_hours);
    label = gui_internal_label_new(gui_priv, buffer);
    gui_internal_widget_append(box, label);
    
    /* Mandatory break duration */
    snprintf(buffer, sizeof(buffer), "Break duration: %d minutes", 
        priv->config.truck_mandatory_break_duration_min);
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
    button = gui_internal_button_new_with_callback(gui_priv, "OK", NULL, 
        gravity_center|flags_fill, driver_break_save_config_callback, priv);
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
    box = gui_internal_box_new(gui_priv, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
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
    button = gui_internal_button_new_with_callback(gui_priv, "OK", NULL, 
        gravity_center|flags_fill, driver_break_save_config_callback, priv);
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
    box = gui_internal_box_new(gui_priv, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
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
    button = gui_internal_button_new_with_callback(gui_priv, "OK", NULL, 
        gravity_center|flags_fill, driver_break_save_config_callback, priv);
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
        dbg(lvl_error, "Driver Break plugin: Plugin not found - OSD may not be instantiated. Check if <osd type=\"rest\" enabled=\"yes\"/> is in config.");
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
    } else {
        dbg(lvl_warning, "Driver Break plugin: Unknown profile type: %s", profile_name);
        navit_add_message(nav, "Driver Break plugin: Configuration not available for this profile");
        return 0;
    }

    return 1;
}

/* Show overnight stops configuration dialog */
static void driver_break_show_overnight_dialog(struct gui_priv *gui_priv, struct driver_break_priv *priv, const char *profile_name) {
    struct widget *menu, *box, *label, *button;
    char buffer[256];
    char title[128];
    
    if (!priv) {
        dbg(lvl_error, "Driver Break plugin: driver_break_show_overnight_dialog called with NULL priv");
        return;
    }
    
    dbg(lvl_debug, "Driver Break plugin: Showing overnight dialog - config values: min_buildings=%d, min_glaciers=%d, poi_radius=%d, water_radius=%d, cabin_radius=%d",
        priv->config.min_distance_from_buildings, priv->config.min_distance_from_glaciers,
        priv->config.poi_search_radius_km, priv->config.poi_water_search_radius_km, 
        priv->config.poi_cabin_search_radius_km);
    
    snprintf(title, sizeof(title), "Overnight Stops (%s)", profile_name);
    
    graphics_draw_mode(gui_priv->gra, draw_mode_begin);
    gui_internal_enter(gui_priv, 1);
    gui_internal_set_click_coord(gui_priv, NULL);
    gui_internal_enter_setup(gui_priv);
    
    menu = gui_internal_menu(gui_priv, title);
    box = gui_internal_box_new(gui_priv, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
    gui_internal_widget_append(menu, box);
    
    /* Min distance from buildings - validate before displaying */
    {
        int val = priv->config.min_distance_from_buildings;
        if (val < 0 || val > 10000) {
            dbg(lvl_error, "Driver Break plugin: Invalid min_distance_from_buildings value: %d, using default 150", val);
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
    
    /* Note about editing */
    label = gui_internal_label_new(gui_priv, "Note: Advanced editing coming soon");
    gui_internal_widget_append(box, label);
    
    /* Save button */
    button = gui_internal_button_new_with_callback(gui_priv, "OK", NULL, 
        gravity_center|flags_fill, driver_break_save_config_callback, priv);
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
        dbg(lvl_error, "Driver Break plugin: Plugin not found - OSD may not be instantiated. Check if <osd type=\"rest\" enabled=\"yes\"/> is in config.");
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
