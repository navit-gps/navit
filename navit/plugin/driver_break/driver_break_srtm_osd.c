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

#include "driver_break_srtm_osd.h"
#include "attr.h"
#include "command.h"
#include "config.h"
#include "debug.h"
#include "driver_break_osd.h"
#include "driver_break_srtm.h"
#include "graphics.h"
#include "gui.h"
#include "navit.h"
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef HAVE_API_ANDROID
#    include "gui/internal/gui_internal.h"
#    include "gui/internal/gui_internal_menu.h"
#    include "gui/internal/gui_internal_priv.h"
#    include "gui/internal/gui_internal_widget.h"
#    include <dlfcn.h>

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

static int srtm_gui_internal_resolved;

static void srtm_resolve_gui_internal(void) {
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

    if (srtm_gui_internal_resolved)
        return;
    srtm_gui_internal_resolved = 1;

    dlerror();
    for (i = 0; lib_paths[i] != NULL; i++) {
        handle = dlopen(lib_paths[i], RTLD_LAZY | RTLD_GLOBAL);
        if (handle)
            break;
        error = dlerror();
        if (error)
            dbg(lvl_debug, "Driver Break SRTM: dlopen %s: %s", lib_paths[i], error);
    }
    if (!handle)
        handle = RTLD_DEFAULT;

    dlerror();
    gui_internal_back_ptr = (void (*)(struct gui_priv *, struct widget *, void *))dlsym(handle, "gui_internal_back");
    gui_internal_enter_ptr = (void (*)(struct gui_priv *, int))dlsym(handle, "gui_internal_enter");
    gui_internal_leave_ptr = (void (*)(struct gui_priv *))dlsym(handle, "gui_internal_leave");
    gui_internal_set_click_coord_ptr =
        (void (*)(struct gui_priv *, struct point *))dlsym(handle, "gui_internal_set_click_coord");
    gui_internal_enter_setup_ptr = (void (*)(struct gui_priv *))dlsym(handle, "gui_internal_enter_setup");
    gui_internal_menu_ptr = (struct widget * (*)(struct gui_priv *, const char *))dlsym(handle, "gui_internal_menu");
    gui_internal_box_new_ptr =
        (struct widget * (*)(struct gui_priv *, enum flags))dlsym(handle, "gui_internal_box_new");
    gui_internal_label_new_ptr =
        (struct widget * (*)(struct gui_priv *, const char *))dlsym(handle, "gui_internal_label_new");
    gui_internal_button_new_with_callback_ptr =
        (struct widget * (*)(struct gui_priv *, const char *, struct widget *, enum flags,
                             void (*)(struct gui_priv *, struct widget *, void *), void *))
            dlsym(handle, "gui_internal_button_new_with_callback");
    gui_internal_widget_append_ptr =
        (void (*)(struct widget *, struct widget *))dlsym(handle, "gui_internal_widget_append");
    gui_internal_menu_render_ptr = (void (*)(struct gui_priv *))dlsym(handle, "gui_internal_menu_render");

    error = dlerror();
    if (error)
        dbg(lvl_debug, "Driver Break SRTM: dlerror after dlsym: %s", error);
}

#    define gui_internal_back(p, w, d)                                                                                 \
        do {                                                                                                           \
            srtm_resolve_gui_internal();                                                                               \
            if (gui_internal_back_ptr)                                                                                 \
                gui_internal_back_ptr(p, w, d);                                                                        \
        } while (0)
#    define gui_internal_enter(p, i)                                                                                   \
        do {                                                                                                           \
            srtm_resolve_gui_internal();                                                                               \
            if (gui_internal_enter_ptr)                                                                                \
                gui_internal_enter_ptr(p, i);                                                                          \
        } while (0)
#    define gui_internal_leave(p)                                                                                      \
        do {                                                                                                           \
            srtm_resolve_gui_internal();                                                                               \
            if (gui_internal_leave_ptr)                                                                                \
                gui_internal_leave_ptr(p);                                                                             \
        } while (0)
#    define gui_internal_set_click_coord(p, pt)                                                                        \
        do {                                                                                                           \
            srtm_resolve_gui_internal();                                                                               \
            if (gui_internal_set_click_coord_ptr)                                                                      \
                gui_internal_set_click_coord_ptr(p, pt);                                                               \
        } while (0)
#    define gui_internal_enter_setup(p)                                                                                \
        do {                                                                                                           \
            srtm_resolve_gui_internal();                                                                               \
            if (gui_internal_enter_setup_ptr)                                                                          \
                gui_internal_enter_setup_ptr(p);                                                                       \
        } while (0)
#    define gui_internal_menu(p, s)                                                                                    \
        (srtm_resolve_gui_internal(), gui_internal_menu_ptr ? gui_internal_menu_ptr(p, s) : NULL)
#    define gui_internal_box_new(p, f)                                                                                 \
        (srtm_resolve_gui_internal(), gui_internal_box_new_ptr ? gui_internal_box_new_ptr(p, f) : NULL)
#    define gui_internal_label_new(p, s)                                                                               \
        (srtm_resolve_gui_internal(), gui_internal_label_new_ptr ? gui_internal_label_new_ptr(p, s) : NULL)
#    define gui_internal_button_new_with_callback(p, s, w, f, cb, d)                                                   \
        (srtm_resolve_gui_internal(), gui_internal_button_new_with_callback_ptr                                       \
                                           ? gui_internal_button_new_with_callback_ptr(p, s, w, f, cb, d)              \
                                           : NULL)
#    define gui_internal_widget_append(w1, w2)                                                                         \
        do {                                                                                                           \
            srtm_resolve_gui_internal();                                                                               \
            if (gui_internal_widget_append_ptr)                                                                        \
                gui_internal_widget_append_ptr(w1, w2);                                                              \
        } while (0)
#    define gui_internal_menu_render(p)                                                                                \
        do {                                                                                                           \
            srtm_resolve_gui_internal();                                                                               \
            if (gui_internal_menu_render_ptr)                                                                          \
                gui_internal_menu_render_ptr(p);                                                                       \
        } while (0)

static int srtm_gui_internal_ready(void) {
    srtm_resolve_gui_internal();
    return gui_internal_menu_render_ptr != NULL;
}

static void srtm_cb_dialog_back(struct gui_priv *gui_priv, struct widget *widget, void *data) {
    (void)data;
    gui_internal_back(gui_priv, widget, NULL);
}

#endif /* !HAVE_API_ANDROID */

static void srtm_ensure_init(void) {
    if (!srtm_get_data_directory())
        srtm_init(NULL);
}

struct srtm_region_dl_data {
    struct navit *nav;
    char *region_name;
};

/**
 * Start async download for a named region. The returned srtm_download_context keeps ownership of the
 * srtm_region; do not free the region after this call.
 */
static int srtm_start_download_named(struct navit *nav, const char *name) {
    struct srtm_region *region;
    struct srtm_download_context *ctx;

    if (!nav || !name || !*name)
        return 0;

    srtm_ensure_init();

    region = srtm_get_region(name);
    if (!region) {
        char msg[256];
        snprintf(msg, sizeof(msg), "SRTM: unknown region \"%s\"", name);
        navit_add_message(nav, msg);
        return 0;
    }

    ctx = srtm_download_region(region, NULL, NULL);
    if (!ctx) {
        navit_add_message(nav, "SRTM: download could not start (libcurl disabled or error)");
        g_free(region->name);
        g_free(region);
        return 0;
    }

    {
        char msg[320];
        snprintf(msg, sizeof(msg), "SRTM: download started for %s (%d tiles)", region->name, region->tile_count);
        navit_add_message(nav, msg);
    }
    return 1;
}

#ifndef HAVE_API_ANDROID
static void srtm_cb_region_download(struct gui_priv *gui_priv, struct widget *widget, void *data) {
    struct srtm_region_dl_data *d = (struct srtm_region_dl_data *)data;
    (void)gui_priv;
    (void)widget;
    if (d) {
        if (d->nav && d->region_name)
            srtm_start_download_named(d->nav, d->region_name);
        g_free(d->region_name);
        g_free(d);
    }
}

static void srtm_show_regions_dialog(struct navit *nav, struct gui_priv *gui_priv, GList *regions) {
    struct widget *menu, *box, *label, *button;
    char buf[384];
    GList *l;

    if (!gui_priv || !srtm_gui_internal_ready()) {
        GString *gs = g_string_new(NULL);
        g_string_append(gs, "SRTM regions: ");
        if (!regions) {
            g_string_append(gs, "none.");
        } else {
            for (l = regions; l; l = l->next) {
                struct srtm_region *r = (struct srtm_region *)l->data;
                if (!r)
                    continue;
                g_string_append_printf(gs, "%s; ", r->name);
            }
        }
        navit_add_message(nav, gs->str);
        g_string_free(gs, TRUE);
        return;
    }

    graphics_draw_mode(gui_priv->gra, draw_mode_begin);
    gui_internal_enter(gui_priv, 1);
    gui_internal_set_click_coord(gui_priv, NULL);
    gui_internal_enter_setup(gui_priv);

    menu = gui_internal_menu(gui_priv, "SRTM elevation data");
    box = gui_internal_box_new(gui_priv, gravity_left_top | orientation_vertical | flags_expand | flags_fill);
    gui_internal_widget_append(menu, box);

    if (!regions) {
        label = gui_internal_label_new(gui_priv, "No predefined regions are available.");
        gui_internal_widget_append(box, label);
    } else {
        label = gui_internal_label_new(gui_priv, "Select a region to download tiles (async, see log for progress).");
        gui_internal_widget_append(box, label);
        for (l = regions; l; l = l->next) {
            struct srtm_region *r = (struct srtm_region *)l->data;
            struct srtm_region_dl_data *d;

            if (!r || !r->name)
                continue;
            snprintf(buf, sizeof(buf), "%s: %d tiles, ~%.1f MB, %d%% complete", r->name, r->tile_count,
                     r->size_bytes / 1048576.0, r->progress_percent);
            label = gui_internal_label_new(gui_priv, buf);
            gui_internal_widget_append(box, label);
            d = g_new0(struct srtm_region_dl_data, 1);
            d->nav = nav;
            d->region_name = g_strdup(r->name);
            button = gui_internal_button_new_with_callback(gui_priv, "Download this region", NULL,
                                                           gravity_center | flags_fill, srtm_cb_region_download, d);
            gui_internal_widget_append(box, button);
        }
    }

    button = gui_internal_button_new_with_callback(gui_priv, "Back", NULL, gravity_center | flags_fill,
                                                   srtm_cb_dialog_back, NULL);
    gui_internal_widget_append(box, button);

    gui_internal_menu_render(gui_priv);
    gui_internal_leave(gui_priv);
    graphics_draw_mode(gui_priv->gra, draw_mode_end);
}

static void srtm_show_elevation_dialog(struct navit *nav, struct gui_priv *gui_priv, struct coord_geo *cg, int elev) {
    char buf[512];
    struct widget *menu, *box, *label, *button;

    if (elev == -32768) {
        snprintf(buf, sizeof(buf),
                 "Elevation data not available for:\nlatitude %.6f\nlongitude %.6f\n"
                 "(missing HGT/GeoTIFF tiles or outside coverage).",
                 cg->lat, cg->lng);
    } else {
        snprintf(buf, sizeof(buf), "Elevation at this position:\nlatitude %.6f\nlongitude %.6f\n\n%d m above sea level",
                 cg->lat, cg->lng, elev);
    }

    if (!gui_priv || !srtm_gui_internal_ready()) {
        navit_add_message(nav, buf);
        return;
    }

    graphics_draw_mode(gui_priv->gra, draw_mode_begin);
    gui_internal_enter(gui_priv, 1);
    gui_internal_set_click_coord(gui_priv, NULL);
    gui_internal_enter_setup(gui_priv);

    menu = gui_internal_menu(gui_priv, "SRTM elevation");
    box = gui_internal_box_new(gui_priv, gravity_left_top | orientation_vertical | flags_expand | flags_fill);
    gui_internal_widget_append(menu, box);
    label = gui_internal_label_new(gui_priv, buf);
    gui_internal_widget_append(box, label);
    button = gui_internal_button_new_with_callback(gui_priv, "Back", NULL, gravity_center | flags_fill,
                                                   srtm_cb_dialog_back, NULL);
    gui_internal_widget_append(box, button);

    gui_internal_menu_render(gui_priv);
    gui_internal_leave(gui_priv);
    graphics_draw_mode(gui_priv->gra, draw_mode_end);
}
#endif /* !HAVE_API_ANDROID */

static void srtm_regions_fallback_message(struct navit *nav, GList *regions) {
    GString *gs = g_string_new("SRTM regions: ");
    GList *l;
    if (!regions) {
        g_string_append(gs, "none listed.");
    } else {
        for (l = regions; l; l = l->next) {
            struct srtm_region *r = (struct srtm_region *)l->data;
            if (r && r->name)
                g_string_append_printf(gs, "%s ", r->name);
        }
    }
    navit_add_message(nav, gs->str);
    g_string_free(gs, TRUE);
}

/* Command: Show SRTM download manager (region list with download actions) */
static int driver_break_cmd_srtm_download_menu(struct navit *nav, char *function, struct attr **in,
                                               struct attr ***out) {
    GList *regions;
    struct gui_priv *gui_priv;

    (void)function;
    (void)in;
    (void)out;

    dbg(lvl_info, "Driver Break plugin: SRTM download menu");
    srtm_ensure_init();

    regions = srtm_get_regions();
    gui_priv = driver_break_get_internal_gui_priv(nav);

#ifndef HAVE_API_ANDROID
    if (gui_priv && srtm_gui_internal_ready()) {
        srtm_show_regions_dialog(nav, gui_priv, regions);
        srtm_free_regions(regions);
        return 1;
    }
#endif
    srtm_regions_fallback_message(nav, regions);
    srtm_free_regions(regions);
    return 1;
}

/* Command: Download a region by name, or open the region picker when no name is passed */
static int driver_break_cmd_srtm_download_region(struct navit *nav, char *function, struct attr **in,
                                                 struct attr ***out) {
    struct attr *region_attr = attr_search(in, attr_name);
    GList *regions;
    struct gui_priv *gui_priv;

    (void)function;
    (void)out;

    if (region_attr && region_attr->u.str && region_attr->u.str[0]) {
        dbg(lvl_info, "Driver Break plugin: SRTM download region %s", region_attr->u.str);
        return srtm_start_download_named(nav, region_attr->u.str) ? 1 : 0;
    }

    dbg(lvl_info, "Driver Break plugin: SRTM download region (picker)");
    srtm_ensure_init();
    regions = srtm_get_regions();
    gui_priv = driver_break_get_internal_gui_priv(nav);

#ifndef HAVE_API_ANDROID
    if (gui_priv && srtm_gui_internal_ready()) {
        srtm_show_regions_dialog(nav, gui_priv, regions);
        srtm_free_regions(regions);
        return 1;
    }
#endif
    srtm_regions_fallback_message(nav, regions);
    srtm_free_regions(regions);
    return 1;
}

/* Command: Query SRTM elevation at coordinate from attributes or current vehicle position */
static int driver_break_cmd_srtm_get_elevation(struct navit *nav, char *function, struct attr **in,
                                               struct attr ***out) {
    struct attr *coord_attr = attr_search(in, attr_position_coord_geo);
    struct attr pos_attr;
    struct coord_geo *cg = NULL;
    struct coord_geo stack_geo;
    int elevation;
    struct gui_priv *gui_priv;

    (void)function;

    srtm_ensure_init();

    if (coord_attr && coord_attr->u.coord_geo) {
        cg = coord_attr->u.coord_geo;
    } else if (navit_get_attr(nav, attr_position_coord_geo, &pos_attr, NULL) && pos_attr.u.coord_geo) {
        stack_geo = *pos_attr.u.coord_geo;
        cg = &stack_geo;
    }

    if (!cg) {
        navit_add_message(nav, "SRTM: no position (vehicle position or coordinate argument required)");
        dbg(lvl_warning, "Driver Break plugin: srtm_get_elevation without position");
        return 0;
    }

    elevation = srtm_get_elevation(cg);
    dbg(lvl_info, "Driver Break plugin: SRTM elevation at %.6f,%.6f -> %d", cg->lat, cg->lng, elevation);

    if (out && *out) {
        struct attr *elev_attr = g_new0(struct attr, 1);
        elev_attr->type = attr_height;
        elev_attr->u.numd = g_new(double, 1);
        *elev_attr->u.numd = (double)elevation;
        *out = g_new(struct attr *, 2);
        (*out)[0] = elev_attr;
        (*out)[1] = NULL;
    }

    gui_priv = driver_break_get_internal_gui_priv(nav);
#ifndef HAVE_API_ANDROID
    if (gui_priv && srtm_gui_internal_ready()) {
        srtm_show_elevation_dialog(nav, gui_priv, cg, elevation);
        return 1;
    }
#endif
    {
        char msg[256];
        if (elevation == -32768)
            snprintf(msg, sizeof(msg), "SRTM: no elevation data at %.5f, %.5f", cg->lat, cg->lng);
        else
            snprintf(msg, sizeof(msg), "SRTM: elevation %d m at %.5f, %.5f", elevation, cg->lat, cg->lng);
        navit_add_message(nav, msg);
    }
    return 1;
}

static struct command_table driver_break_srtm_commands[] = {
    {"srtm_download_menu",   command_cast(driver_break_cmd_srtm_download_menu)  },
    {"srtm_download_region", command_cast(driver_break_cmd_srtm_download_region)},
    {"srtm_get_elevation",   command_cast(driver_break_cmd_srtm_get_elevation)  },
    {NULL,                   NULL                                               }
};

void driver_break_srtm_register_commands(struct navit *nav) {
    navit_command_add_table(nav, driver_break_srtm_commands,
                            sizeof(driver_break_srtm_commands) / sizeof(driver_break_srtm_commands[0]) - 1);
    dbg(lvl_info, "Driver Break plugin: Registered SRTM commands");
}
