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
#include "debug.h"
#include "command.h"
#include "navit.h"
#include "driver_break_srtm_osd.h"
#include "driver_break_srtm.h"

/* Command: Show SRTM download menu */
static int driver_break_cmd_srtm_download_menu(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    dbg(lvl_info, "Driver Break plugin: SRTM download menu");

    /* Get list of available regions */
    GList *regions = srtm_get_regions();
    if (regions) {
        GList *l = regions;
        dbg(lvl_info, "Driver Break plugin: Available SRTM regions:");
        while (l) {
            struct srtm_region *region = (struct srtm_region *)l->data;
            if (region) {
                dbg(lvl_info, "  - %s: %d tiles, %.1f MB, %d%% complete",
                    region->name, region->tile_count,
                    region->size_bytes / (1024.0 * 1024.0),
                    region->progress_percent);
            }
            l = g_list_next(l);
        }
        srtm_free_regions(regions);
    }

    return 1;
}

/* Command: Download SRTM region */
static int driver_break_cmd_srtm_download_region(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    struct attr *region_attr = attr_search(in, attr_name);

    if (!region_attr || !region_attr->u.str) {
        dbg(lvl_error, "Driver Break plugin: SRTM download region requires region name");
        return 0;
    }

    dbg(lvl_info, "Driver Break plugin: Downloading SRTM region: %s", region_attr->u.str);

    /* Initialize SRTM if not already done */
    if (!srtm_get_data_directory()) {
        srtm_init(NULL);
    }

    /* Get region */
    struct srtm_region *region = srtm_get_region(region_attr->u.str);
    if (!region) {
        dbg(lvl_error, "Driver Break plugin: Unknown SRTM region: %s", region_attr->u.str);
        return 0;
    }

    /* Start download */
    struct srtm_download_context *ctx = srtm_download_region(region, NULL, NULL);
    if (!ctx) {
        dbg(lvl_error, "Driver Break plugin: Failed to start SRTM download");
        g_free(region->name);
        g_free(region);
        return 0;
    }

    /* Store context in navit for pause/resume/cancel commands */
    /* Context is stored in the download context itself and can be retrieved */
    dbg(lvl_info, "Driver Break plugin: SRTM download started for %s (%d tiles, %.1f MB)",
        region->name, region->tile_count, region->size_bytes / (1024.0 * 1024.0));

    g_free(region->name);
    g_free(region);
    return 1;
}

/* Command: Get SRTM elevation at coordinate */
static int driver_break_cmd_srtm_get_elevation(struct navit *nav, char *function, struct attr **in, struct attr ***out) {
    struct attr *coord_attr = attr_search(in, attr_position_coord_geo);

    if (!coord_attr || !coord_attr->u.coord_geo) {
        dbg(lvl_error, "Driver Break plugin: SRTM get elevation requires coordinate");
        return 0;
    }

    int elevation = srtm_get_elevation(coord_attr->u.coord_geo);

    if (elevation == -32768) {
        dbg(lvl_info, "Driver Break plugin: Elevation not available for coordinate");
    } else {
        dbg(lvl_info, "Driver Break plugin: Elevation at coordinate: %d meters", elevation);
    }

    /* Return elevation in output attributes if output is provided */
    if (out && *out) {
        struct attr *elev_attr = g_new0(struct attr, 1);
        elev_attr->type = attr_height;
        elev_attr->u.numd = g_new(double, 1);
        *elev_attr->u.numd = (double)elevation;
        *out = g_new(struct attr *, 2);
        (*out)[0] = elev_attr;
        (*out)[1] = NULL;
    }

    return 1;
}

/* Command table */
static struct command_table driver_break_srtm_commands[] = {
    {"srtm_download_menu", command_cast(driver_break_cmd_srtm_download_menu)},
    {"srtm_download_region", command_cast(driver_break_cmd_srtm_download_region)},
    {"srtm_get_elevation", command_cast(driver_break_cmd_srtm_get_elevation)},
    {NULL, NULL}
};

/* Register commands */
void driver_break_srtm_register_commands(struct navit *nav) {
    navit_command_add_table(nav, driver_break_srtm_commands,
                           sizeof(driver_break_srtm_commands) / sizeof(driver_break_srtm_commands[0]) - 1);
    dbg(lvl_info, "Driver Break plugin: Registered SRTM commands");
}
