/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 */

#include "driver_break_osd_priv.h"

#ifndef HAVE_API_ANDROID

void driver_break_show_hiking_intervals_dialog(struct gui_priv *gui_priv, struct driver_break_priv *priv) {
    struct osd_dialog_ctx ctx;

    osd_dialog_begin(gui_priv, "Hiking Rest Intervals", &ctx);
    osd_append_label_printf(&ctx, "Main rest distance: %.2f km", priv->config.hiking_driver_break_distance_main);
    osd_append_label_printf(&ctx, "Alternative rest: %.2f km", priv->config.hiking_driver_break_distance_alt);
    osd_append_label_printf(&ctx, "Max daily distance: %.1f km", priv->config.hiking_max_daily_distance);
    osd_append_label(&ctx, "Note: Advanced editing coming soon");
    osd_dialog_end_with_save(&ctx, priv);
}

#endif /* !HAVE_API_ANDROID */
