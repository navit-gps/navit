/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * Internal declarations shared between Driver Break OSD translation units.
 */

#ifndef NAVIT_PLUGIN_DRIVER_BREAK_OSD_PRIV_H
#define NAVIT_PLUGIN_DRIVER_BREAK_OSD_PRIV_H

#include "driver_break.h"
#include "navit.h"

struct gui_priv;
struct widget;

extern struct driver_break_priv *driver_break_plugin_instance;

struct driver_break_priv *driver_break_osd_get_plugin(struct navit *nav);
struct gui_priv *driver_break_osd_get_internal_gui_priv(struct navit *nav);
int driver_break_osd_require_plugin(struct navit *nav, struct driver_break_priv **priv_out);
int driver_break_osd_require_gui(struct navit *nav, struct gui_priv **gui_priv_out);

struct osd_dialog_ctx {
    struct gui_priv *gui_priv;
    struct widget *menu;
    struct widget *box;
};

void driver_break_osd_save_config_callback(struct gui_priv *gui_priv, struct widget *widget, void *data);

#ifndef HAVE_API_ANDROID
void osd_dialog_begin(struct gui_priv *gui_priv, const char *title, struct osd_dialog_ctx *ctx);
void osd_dialog_end_with_save(struct osd_dialog_ctx *ctx, struct driver_break_priv *priv);
void osd_dialog_end(struct osd_dialog_ctx *ctx);
void osd_append_label(struct osd_dialog_ctx *ctx, const char *text);
void osd_append_label_printf(struct osd_dialog_ctx *ctx, const char *fmt, ...);
void osd_append_validated_int_label(struct osd_dialog_ctx *ctx, const char *label_fmt, int *val, int min, int max,
                                    int default_val, const char *field_name);
void osd_dialog_back(struct gui_priv *gui_priv);
void osd_append_button_with_callback(struct osd_dialog_ctx *ctx, const char *label,
                                     void (*callback)(struct gui_priv *, struct widget *, void *), void *data);
#endif

void driver_break_show_suggestions_dialog(struct gui_priv *gui_priv, struct driver_break_priv *priv, GList *stops);
void driver_break_show_history_dialog(struct gui_priv *gui_priv, struct driver_break_priv *priv, GList *history);
void driver_break_show_fuel_config_dialog(struct gui_priv *gui_priv, struct driver_break_priv *priv);
void driver_break_show_car_intervals_dialog(struct gui_priv *gui_priv, struct driver_break_priv *priv);
void driver_break_show_truck_intervals_dialog(struct gui_priv *gui_priv, struct driver_break_priv *priv);
void driver_break_show_hiking_intervals_dialog(struct gui_priv *gui_priv, struct driver_break_priv *priv);
void driver_break_show_cycling_intervals_dialog(struct gui_priv *gui_priv, struct driver_break_priv *priv);
void driver_break_show_motorcycle_intervals_dialog(struct gui_priv *gui_priv, struct driver_break_priv *priv);
void driver_break_show_overnight_dialog(struct gui_priv *gui_priv, struct driver_break_priv *priv,
                                        const char *profile_name);

int driver_break_osd_show_intervals_for_profile(struct gui_priv *gui_priv, struct driver_break_priv *priv,
                                                const char *profile_name);

#endif /* NAVIT_PLUGIN_DRIVER_BREAK_OSD_PRIV_H */
