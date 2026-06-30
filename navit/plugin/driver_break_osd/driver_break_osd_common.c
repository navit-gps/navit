/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 */

#include "attr.h"
#include "debug.h"
#include "driver_break_db.h"
#include "driver_break_osd_priv.h"
#include "graphics.h"
#include "gui.h"
#include "navit.h"
#include <glib.h>
#include <stdarg.h>
#include <stdio.h>

#ifndef HAVE_API_ANDROID
#    include "driver_break_gui_shim.h"
#    include "gui/internal/gui_internal.h"
#    include "gui/internal/gui_internal_priv.h"
#endif

struct driver_break_priv *driver_break_plugin_instance = NULL;

struct driver_break_priv *driver_break_osd_get_plugin(struct navit *nav) {
    (void)nav;
    if (!driver_break_plugin_instance) {
        dbg(lvl_warning, "Driver Break plugin: Plugin instance not found - OSD may not be instantiated");
    }
    return driver_break_plugin_instance;
}

struct gui_local {
    struct gui_methods meth;
    struct gui_priv *priv;
    struct attr **attrs;
    struct attr parent;
};

struct gui_priv *driver_break_osd_get_internal_gui_priv(struct navit *nav) {
    struct gui *gui;
    struct gui_local *gui_local;
    struct attr type_attr;

    if (!nav) {
        return NULL;
    }

    gui = navit_get_gui(nav);
    if (!gui) {
        return NULL;
    }

    if (!gui_get_attr(gui, attr_type, &type_attr, NULL)) {
        return NULL;
    }

    if (g_strcmp0(type_attr.u.str, "internal") != 0) {
        return NULL;
    }

    gui_local = (struct gui_local *)gui;
    return gui_local->priv;
}

int driver_break_osd_require_plugin(struct navit *nav, struct driver_break_priv **priv_out) {
    struct driver_break_priv *priv;

    priv = driver_break_osd_get_plugin(nav);
    if (!priv) {
        dbg(lvl_error, "Driver Break plugin: Plugin not found");
        navit_add_message(nav, "Driver Break plugin: Plugin not loaded");
        return 0;
    }

    *priv_out = priv;
    return 1;
}

int driver_break_osd_require_gui(struct navit *nav, struct gui_priv **gui_priv_out) {
    struct gui_priv *gui_priv;

    gui_priv = driver_break_osd_get_internal_gui_priv(nav);
    if (!gui_priv) {
        return 0;
    }

    *gui_priv_out = gui_priv;
    return 1;
}

#ifndef HAVE_API_ANDROID

void driver_break_osd_save_config_callback(struct gui_priv *gui_priv, struct widget *widget, void *data) {
    struct driver_break_priv *priv = (struct driver_break_priv *)data;

    (void)widget;

    if (priv && priv->db) {
        driver_break_db_save_config(priv->db, &priv->config);
        navit_add_message(priv->nav, "Driver Break plugin: Configuration saved");
    }

    driver_break_gui_shim_resolve();
    if (driver_break_gui_shim_back_ptr) {
        driver_break_gui_shim_back_ptr(gui_priv, NULL, NULL);
    }
}

void osd_dialog_begin(struct gui_priv *gui_priv, const char *title, struct osd_dialog_ctx *ctx) {
    ctx->gui_priv = gui_priv;
    driver_break_gui_shim_resolve();
    graphics_draw_mode(gui_priv->gra, draw_mode_begin);
    if (driver_break_gui_shim_enter_ptr) {
        driver_break_gui_shim_enter_ptr(gui_priv, 1);
    }
    if (driver_break_gui_shim_set_click_coord_ptr) {
        driver_break_gui_shim_set_click_coord_ptr(gui_priv, NULL);
    }
    if (driver_break_gui_shim_enter_setup_ptr) {
        driver_break_gui_shim_enter_setup_ptr(gui_priv);
    }
    ctx->menu = driver_break_gui_shim_menu_ptr ? driver_break_gui_shim_menu_ptr(gui_priv, title) : NULL;
    ctx->box = driver_break_gui_shim_box_new_ptr
                   ? driver_break_gui_shim_box_new_ptr(gui_priv, gravity_left_top | orientation_vertical | flags_expand
                                                                     | flags_fill)
                   : NULL;
    if (driver_break_gui_shim_widget_append_ptr && ctx->menu && ctx->box) {
        driver_break_gui_shim_widget_append_ptr(ctx->menu, ctx->box);
    }
}

void osd_dialog_end(struct osd_dialog_ctx *ctx) {
    if (driver_break_gui_shim_menu_render_ptr) {
        driver_break_gui_shim_menu_render_ptr(ctx->gui_priv);
    }
    if (driver_break_gui_shim_leave_ptr) {
        driver_break_gui_shim_leave_ptr(ctx->gui_priv);
    }
    graphics_draw_mode(ctx->gui_priv->gra, draw_mode_end);
}

void osd_dialog_end_with_save(struct osd_dialog_ctx *ctx, struct driver_break_priv *priv) {
    struct widget *button;

    if (driver_break_gui_shim_button_new_with_callback_ptr) {
        button = driver_break_gui_shim_button_new_with_callback_ptr(
            ctx->gui_priv, "OK", NULL, gravity_center | flags_fill, driver_break_osd_save_config_callback, priv);
        if (driver_break_gui_shim_widget_append_ptr && button) {
            driver_break_gui_shim_widget_append_ptr(ctx->box, button);
        }
    }
    osd_dialog_end(ctx);
}

void osd_append_label(struct osd_dialog_ctx *ctx, const char *text) {
    struct widget *label;

    if (!driver_break_gui_shim_label_new_ptr) {
        return;
    }

    label = driver_break_gui_shim_label_new_ptr(ctx->gui_priv, text);
    if (driver_break_gui_shim_widget_append_ptr && label) {
        driver_break_gui_shim_widget_append_ptr(ctx->box, label);
    }
}

void osd_append_label_printf(struct osd_dialog_ctx *ctx, const char *fmt, ...) {
    char buffer[256];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    osd_append_label(ctx, buffer);
}

void osd_append_validated_int_label(struct osd_dialog_ctx *ctx, const char *label_fmt, int *val, int min, int max,
                                    int default_val, const char *field_name) {
    if (*val < min || *val > max) {
        dbg(lvl_error, "Driver Break plugin: Invalid %s value: %d, using default %d", field_name, *val, default_val);
        *val = default_val;
    }
    osd_append_label_printf(ctx, label_fmt, *val);
}

void osd_dialog_back(struct gui_priv *gui_priv) {
    driver_break_gui_shim_resolve();
    if (driver_break_gui_shim_back_ptr) {
        driver_break_gui_shim_back_ptr(gui_priv, NULL, NULL);
    }
}

void osd_append_button_with_callback(struct osd_dialog_ctx *ctx, const char *label,
                                     void (*callback)(struct gui_priv *, struct widget *, void *), void *data) {
    struct widget *button;

    driver_break_gui_shim_resolve();
    if (!driver_break_gui_shim_button_new_with_callback_ptr) {
        return;
    }

    button = driver_break_gui_shim_button_new_with_callback_ptr(ctx->gui_priv, label, NULL, gravity_center | flags_fill,
                                                                callback, data);
    if (driver_break_gui_shim_widget_append_ptr && button) {
        driver_break_gui_shim_widget_append_ptr(ctx->box, button);
    }
}

#else

void driver_break_osd_save_config_callback(struct gui_priv *gui_priv, struct widget *widget, void *data) {
    struct driver_break_priv *priv = (struct driver_break_priv *)data;

    (void)gui_priv;
    (void)widget;

    if (priv && priv->db) {
        driver_break_db_save_config(priv->db, &priv->config);
        navit_add_message(priv->nav, "Driver Break plugin: Configuration saved");
    }
}

#endif /* !HAVE_API_ANDROID */
