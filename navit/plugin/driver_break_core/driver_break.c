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

#include "driver_break.h"
#include "attr.h"
#include "callback.h"
#include "config.h"
#include "debug.h"
#include "driver_break_cycling.h"
#include "driver_break_finder.h"
#include "driver_break_fuel_core.h"
#include "driver_break_hiking.h"
#include "driver_break_osd_lifecycle.h"
#include "driver_break_route_stops.h"
#include "event.h"
#include "navit.h"
#include "plugin.h"
#include "route.h"
#include <glib.h>
#include <stdlib.h>
#include <time.h>

static void driver_break_check_driving_time(struct driver_break_priv *priv) {
    time_t now = time(NULL);
    struct driver_break_config *config = &priv->config;
    int mandatory_break = 0;

    if (priv->session.start_time == 0) {
        priv->session.start_time = now;
        priv->session.last_break_time = now;
        return;
    }

    priv->session.continuous_driving_minutes = (int)((now - priv->session.last_break_time) / 60);

    if (config->vehicle_type == DRIVER_BREAK_VEHICLE_TRUCK) {
        if (priv->session.continuous_driving_minutes >= (config->truck_mandatory_break_after_hours * 60)) {
            mandatory_break = 1;
        }
    } else if (config->vehicle_type == DRIVER_BREAK_VEHICLE_CAR) {
        if (priv->session.continuous_driving_minutes >= (config->car_break_interval_hours * 60)) {
            mandatory_break = 0;
        }
    }

    priv->session.mandatory_break_required = mandatory_break;

    if (mandatory_break) {
        dbg(lvl_info, "Driver Break plugin: Mandatory break required after %d minutes of driving",
            priv->session.continuous_driving_minutes);
    }
}

void driver_break_vehicle_callback_wrapper(void *priv_data) {
    struct driver_break_priv *priv = (struct driver_break_priv *)priv_data;
    struct attr attr;

    if (!priv) {
        return;
    }

    driver_break_ecu_sync_to_priv(priv);

    if (navit_get_attr(priv->nav, attr_position_coord_geo, &attr, NULL) && attr.u.coord_geo) {
        driver_break_check_driving_time(priv);
        driver_break_update_fuel_learning(priv, attr.u.coord_geo, time(NULL));
    }
}

static void driver_break_handle_hiking_route(struct driver_break_priv *priv) {
    struct attr dist_attr;
    double total_distance = 0.0;

    if (!priv->current_route) {
        return;
    }

    if (route_get_attr(priv->current_route, attr_destination_length, &dist_attr, NULL)) {
        total_distance = (double)dist_attr.u.num;
    } else {
        dbg(lvl_error, "Driver Break plugin: Hiking route handler failed to get attr_destination_length");
    }

    if (total_distance > 0) {
        double max_daily = priv->config.hiking_max_daily_distance;
        GList *hiking_stops;

        if (priv->config.enable_dnt_priority) {
            max_daily = 46400.0;
        }

        dbg(lvl_info, "Driver Break plugin: Hiking route length=%.0f m max_daily=%.0f m (DNT priority=%d)",
            total_distance, max_daily, priv->config.enable_dnt_priority);

        hiking_stops = hiking_calculate_driver_break_stops_with_max(total_distance, 0, max_daily);
        if (hiking_stops) {
            driver_break_process_hiking_stops(priv, hiking_stops);
            hiking_free_driver_break_stops(hiking_stops);
        } else {
            dbg(lvl_error, "Driver Break plugin: Hiking route handler did not place any rest stops");
        }
    }
}

static void driver_break_handle_cycling_route(struct driver_break_priv *priv) {
    struct attr dist_attr;
    double total_distance = 0.0;

    if (!priv->current_route) {
        return;
    }

    if (route_get_attr(priv->current_route, attr_destination_length, &dist_attr, NULL)) {
        total_distance = (double)dist_attr.u.num;
    } else {
        dbg(lvl_error, "Driver Break plugin: Cycling route handler failed to get attr_destination_length");
    }

    if (total_distance > 0) {
        GList *cycling_stops;

        dbg(lvl_info, "Driver Break plugin: Cycling route length=%.0f m", total_distance);
        cycling_stops = cycling_calculate_driver_break_stops(total_distance, 0);
        if (cycling_stops) {
            driver_break_process_cycling_stops(priv, cycling_stops);
            cycling_free_driver_break_stops(cycling_stops);
        } else {
            dbg(lvl_error, "Driver Break plugin: Cycling route handler did not place any rest stops");
        }
    }
}

static void driver_break_handle_motor_route(struct driver_break_priv *priv) {
    if (!priv->current_route || !priv->session.mandatory_break_required) {
        return;
    }

    priv->suggested_stops = driver_break_finder_find_along_route(priv->current_route, &priv->config,
                                                                 priv->session.mandatory_break_required);
}

void driver_break_route_callback_wrapper(void *priv_data) {
    struct driver_break_priv *priv = (struct driver_break_priv *)priv_data;
    struct attr attr;

    if (!priv) {
        return;
    }

    if (!navit_get_attr(priv->nav, attr_route, &attr, NULL)) {
        return;
    }

    priv->current_route = attr.u.route;
    if (!priv->current_route) {
        return;
    }

    dbg(lvl_info, "Driver Break plugin: Finding rest stops along route (vehicle_type=%d)", priv->config.vehicle_type);

    switch (priv->config.vehicle_type) {
    case DRIVER_BREAK_VEHICLE_HIKING:
        driver_break_handle_hiking_route(priv);
        break;
    case DRIVER_BREAK_VEHICLE_CYCLING:
        driver_break_handle_cycling_route(priv);
        break;
    case DRIVER_BREAK_VEHICLE_CAR:
    case DRIVER_BREAK_VEHICLE_TRUCK:
        driver_break_handle_motor_route(priv);
        break;
    default:
        break;
    }
}

void driver_break_check_timeout(struct event_timeout *ev, void *data) {
    struct driver_break_priv *priv = (struct driver_break_priv *)data;

    driver_break_ecu_sync_to_priv(priv);
    driver_break_check_driving_time(priv);

    if (priv->session.mandatory_break_required && priv->current_route) {
        dbg(lvl_debug, "Driver Break plugin: Checking for rest stops");
    }

    if (priv->config.vehicle_type == DRIVER_BREAK_VEHICLE_CAR
        || priv->config.vehicle_type == DRIVER_BREAK_VEHICLE_TRUCK) {
        driver_break_check_fuel_low_range(priv);
    }
}

void plugin_init(void) {
    dbg(lvl_debug, "Driver Break plugin initializing");
    plugin_register_category_osd("driver_break", driver_break_osd_new);
}
