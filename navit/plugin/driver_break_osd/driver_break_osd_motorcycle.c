/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 */

#include "driver_break_osd_priv.h"

#ifndef HAVE_API_ANDROID

static void driver_break_toggle_motorcycle_terrain_callback(struct gui_priv *gui_priv, struct widget *widget,
                                                            void *data) {
    struct driver_break_priv *priv = (struct driver_break_priv *)data;

    (void)widget;

    if (!priv) {
        return;
    }

    priv->config.motorcycle_terrain_subtype = !priv->config.motorcycle_terrain_subtype;
    osd_dialog_back(gui_priv);
    driver_break_show_motorcycle_intervals_dialog(gui_priv, priv);
}

void driver_break_show_motorcycle_intervals_dialog(struct gui_priv *gui_priv, struct driver_break_priv *priv) {
    struct osd_dialog_ctx ctx;

    if (!priv) {
        return;
    }

    osd_dialog_begin(gui_priv, "Motorcycle Rest Intervals", &ctx);
    osd_append_label_printf(&ctx, "Soft limit: %d min", priv->config.motorcycle_soft_limit_minutes);
    osd_append_label_printf(&ctx, "Mandatory break after: %d min",
                            priv->config.motorcycle_mandatory_break_after_minutes);
    osd_append_label_printf(&ctx, "Break duration: %d min", priv->config.motorcycle_break_duration_min);
    osd_append_label_printf(&ctx, "Terrain: %s", priv->config.motorcycle_terrain_subtype ? "Adventure" : "Road");
    osd_append_button_with_callback(&ctx, "Toggle terrain (Road/Adventure)",
                                    driver_break_toggle_motorcycle_terrain_callback, priv);

    if (priv->config.motorcycle_terrain_subtype) {
        osd_append_label(&ctx,
                         "Adventure mode: Off-road motor traffic on uncultivated land is prohibited by law in many "
                         "countries (e.g. Norway, Sweden, Finland). Use only ways with explicit access. Permits may "
                         "be required.");
    }

    osd_append_label(&ctx, "Note: Advanced editing coming soon");
    osd_dialog_end_with_save(&ctx, priv);
}

#endif /* !HAVE_API_ANDROID */
