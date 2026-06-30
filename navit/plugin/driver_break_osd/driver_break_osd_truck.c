/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 */

#include "driver_break_osd_priv.h"

#ifndef HAVE_API_ANDROID

void driver_break_show_truck_intervals_dialog(struct gui_priv *gui_priv, struct driver_break_priv *priv) {
    struct osd_dialog_ctx ctx;

    osd_dialog_begin(gui_priv, "Truck Rest Intervals", &ctx);
    osd_append_label_printf(&ctx, "Mandatory break after: %.1f hours",
                            (double)priv->config.truck_mandatory_break_after_hours);
    osd_append_label_printf(&ctx, "Break duration: %d minutes", priv->config.truck_mandatory_break_duration_min);
    osd_append_label_printf(&ctx, "Max daily hours: %d hours", priv->config.truck_max_daily_hours);
    osd_append_label(&ctx, "Note: Advanced editing coming soon");
    osd_dialog_end_with_save(&ctx, priv);
}

#endif /* !HAVE_API_ANDROID */
