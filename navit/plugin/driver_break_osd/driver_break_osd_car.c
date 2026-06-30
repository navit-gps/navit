/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 */

#include "debug.h"
#include "driver_break_osd_priv.h"

#ifndef HAVE_API_ANDROID

void driver_break_show_car_intervals_dialog(struct gui_priv *gui_priv, struct driver_break_priv *priv) {
    struct osd_dialog_ctx ctx;

    if (!priv) {
        dbg(lvl_error, "Driver Break plugin: driver_break_show_car_intervals_dialog called with NULL priv");
        return;
    }

    dbg(lvl_debug,
        "Driver Break plugin: Showing car intervals dialog - config values: soft_limit=%d, max=%d, break_interval=%d, "
        "break_duration=%d",
        priv->config.car_soft_limit_hours, priv->config.car_max_hours, priv->config.car_break_interval_hours,
        priv->config.car_break_duration_min);

    osd_dialog_begin(gui_priv, "Car Rest Intervals", &ctx);
    osd_append_validated_int_label(&ctx, "Soft limit: %d hours", &priv->config.car_soft_limit_hours, 0, 12, 7,
                                   "car_soft_limit_hours");
    osd_append_validated_int_label(&ctx, "Max hours: %d hours", &priv->config.car_max_hours, 0, 12, 10,
                                   "car_max_hours");
    osd_append_validated_int_label(&ctx, "Break interval: %d hours", &priv->config.car_break_interval_hours, 0, 12, 4,
                                   "car_break_interval_hours");
    osd_append_validated_int_label(&ctx, "Break duration: %d minutes", &priv->config.car_break_duration_min, 0, 120, 30,
                                   "car_break_duration_min");
    osd_append_label(&ctx, "Note: Advanced editing coming soon");
    osd_dialog_end_with_save(&ctx, priv);
}

#endif /* !HAVE_API_ANDROID */
