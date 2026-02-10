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

#include "aprs_sdr_dsp.h"
#include "aprs_sdr_hw.h"
#include "attr.h"
#include "config.h"
#include "debug.h"
#include "map.h"
#include "mapset.h"
#include "plugin.h"
#include <glib.h>
#include <gmodule.h>
#include <stdlib.h>
#include <string.h>

/* Include the APRS plugin interface */
#include "../aprs/aprs_plugin_interface.h"

/* Forward declaration - aprs_process_packet is in aprs.h but we can't include it due to circular deps */
struct map_priv;
extern int aprs_process_packet(struct map_priv *priv, const unsigned char *data, int length);

/* Forward declaration for frame delivery */
static void aprs_sdr_frame_delivery_callback(const unsigned char *frame, int frame_length, void *user_data);

struct aprs_sdr_priv {
    struct aprs_sdr_hw *hw;
    struct aprs_sdr_dsp *dsp;
    void *aprs_user_data;
    int initialized;
    int running;
};

/* Forward declarations */
static void aprs_sdr_hw_data_callback(const unsigned char *samples, int length, void *user_data);
static void aprs_sdr_map_destroy(void *priv);
static struct map_rect_priv *aprs_sdr_map_rect_new(void *priv, struct map_selection *sel);
static void aprs_sdr_map_rect_destroy(struct map_rect_priv *mr);
static struct item *aprs_sdr_map_get_item(struct map_rect_priv *mr);
static int aprs_sdr_map_get_attr(void *priv, enum attr_type type, struct attr *attr);
static int aprs_sdr_map_set_attr(void *priv, struct attr *attr);

static struct map_methods aprs_sdr_map_meth;

/**
 * Discover APRS plugin and register packet delivery callback
 *
 * This function attempts to find the APRS plugin and register
 * a callback to deliver decoded AX.25 frames.
 */
static int aprs_sdr_discover_aprs_plugin(struct aprs_sdr_priv *priv) {
    int (*register_func)(aprs_packet_callback_t, void *) = NULL;
    void *map_priv_ptr = NULL;

    dbg(lvl_debug, "Attempting to discover APRS plugin...");

    /* Method 1: Look for APRS map via plugin category */
    map_priv_ptr = plugin_get_category_map("aprs");
    if (map_priv_ptr) {
        dbg(lvl_info, "Found APRS map via plugin category");
        priv->aprs_user_data = map_priv_ptr;

        /* Try to get the registration function via symbol lookup */
        /* The function should be exported from the APRS plugin */
        GModule *aprs_module = NULL;
        const char *plugin_names[] = {"libmap_aprs.so", "libaprs.so", "aprs.so", NULL};

        int i;
        for (i = 0; plugin_names[i]; i++) {
            aprs_module = g_module_open(plugin_names[i], G_MODULE_BIND_LAZY);
            if (aprs_module) {
                dbg(lvl_info, "Opened APRS module: %s", plugin_names[i]);
                break;
            }
        }

        if (aprs_module) {
            if (g_module_symbol(aprs_module, "aprs_register_packet_source", (gpointer *)&register_func)) {
                dbg(lvl_info, "Found aprs_register_packet_source function");
            } else {
                dbg(lvl_warning, "aprs_register_packet_source not found, will use direct call");
            }
        }

        /* Register callback - the registration function will store the map_priv */
        if (register_func) {
            aprs_packet_callback_t callback = aprs_sdr_frame_delivery_callback;
            if (register_func(callback, NULL)) {
                priv->aprs_user_data = map_priv_ptr;
                dbg(lvl_info, "Successfully registered with APRS plugin via function");
                return 1;
            }
        } else {
            /* Fallback: store map pointer for direct calls */
            priv->aprs_user_data = map_priv_ptr;
            dbg(lvl_info, "Will deliver frames directly to APRS map (registration function not found)");
            return 1;
        }
    } else {
        dbg(lvl_warning, "APRS map not found - SDR plugin will run but frames won't be delivered");
        dbg(lvl_info, "Load APRS plugin first, or frames will be discarded");
    }

    return 0;
}

/**
 * Hardware data callback - receives I/Q samples from RTL-SDR
 */
static void aprs_sdr_hw_data_callback(const unsigned char *samples, int length, void *user_data) {
    struct aprs_sdr_priv *priv = (struct aprs_sdr_priv *)user_data;

    if (!priv || !priv->dsp)
        return;

    /* Process samples through Bell 202 demodulator */
    aprs_sdr_dsp_process_samples(priv->dsp, samples, length);
}

/**
 * DSP frame callback - delivers decoded AX.25 frames to APRS plugin
 */
static void aprs_sdr_frame_delivery_callback(const unsigned char *frame, int frame_length, void *user_data) {
    struct aprs_sdr_priv *priv = (struct aprs_sdr_priv *)user_data;

    if (!priv || !frame || frame_length <= 0)
        return;

    /* Deliver frame to APRS plugin via direct function call */
    if (priv->aprs_user_data) {
        aprs_process_packet((struct map_priv *)priv->aprs_user_data, frame, frame_length);
    } else {
        dbg(lvl_warning, "Cannot deliver frame - no APRS map instance");
    }
}

static enum aprs_sdr_device_type device_str_to_type(const char *str) {
    if (!str)
        return APRS_SDR_GENERIC;
    if (strstr(str, "blog_v3"))
        return APRS_SDR_BLOG_V3;
    if (strstr(str, "v4_r828d"))
        return APRS_SDR_V4_R828D;
    if (strstr(str, "nooelec"))
        return APRS_SDR_NOOELEC;
    return APRS_SDR_GENERIC;
}

static void aprs_sdr_fill_config_from_attrs(struct attr **attrs, struct aprs_sdr_hw_config *hw_config,
                                            struct aprs_sdr_dsp_config *dsp_config) {
    struct attr *a;

    memset(hw_config, 0, sizeof(*hw_config));
    hw_config->frequency_mhz = 144.390;
    hw_config->sample_rate = 48000;
    hw_config->gain = 49;
    hw_config->ppm_correction = 0;
    hw_config->device_type = APRS_SDR_UNKNOWN;
    hw_config->device_index = 0;
    a = attr_search(attrs, attr_frequency);
    if (a && a->u.numd)
        hw_config->frequency_mhz = *a->u.numd;
    a = attr_search(attrs, attr_gain);
    if (a)
        hw_config->gain = a->u.num;
    a = attr_search(attrs, attr_ppm);
    if (a)
        hw_config->ppm_correction = a->u.num;
    a = attr_search(attrs, attr_device);
    if (a && a->u.str)
        hw_config->device_type = device_str_to_type(a->u.str);

    memset(dsp_config, 0, sizeof(*dsp_config));
    dsp_config->sample_rate = 48000;
    dsp_config->mark_freq = 1200.0;
    dsp_config->space_freq = 2200.0;
    dsp_config->baud_rate = 1200.0;
}

/**
 * Initialize SDR plugin from map attributes
 */
static void *aprs_sdr_map_new(struct map_methods *meth, struct attr **attrs, struct callback_list *cbl) {
    (void)cbl;
    struct aprs_sdr_priv *priv;
    struct aprs_sdr_hw_config hw_config;
    struct aprs_sdr_dsp_config dsp_config;

    dbg(lvl_debug, "APRS SDR plugin: map_new called");
    *meth = aprs_sdr_map_meth;
    priv = g_new0(struct aprs_sdr_priv, 1);
    if (!priv)
        return NULL;

    aprs_sdr_fill_config_from_attrs(attrs, &hw_config, &dsp_config);
    priv->hw = aprs_sdr_hw_new(&hw_config);
    if (!priv->hw) {
        dbg(lvl_error, "Failed to initialize RTL-SDR hardware");
        g_free(priv);
        return NULL;
    }
    priv->dsp = aprs_sdr_dsp_new(&dsp_config);
    if (!priv->dsp) {
        dbg(lvl_error, "Failed to initialize Bell 202 DSP");
        aprs_sdr_hw_destroy(priv->hw);
        g_free(priv);
        return NULL;
    }
    aprs_sdr_hw_set_callback(priv->hw, aprs_sdr_hw_data_callback, priv);
    aprs_sdr_dsp_set_frame_callback(priv->dsp, aprs_sdr_frame_delivery_callback, priv);
    if (!aprs_sdr_discover_aprs_plugin(priv)) {
        dbg(lvl_warning, "APRS plugin not found - SDR will run but frames won't be delivered");
    }
    if (!aprs_sdr_hw_start(priv->hw)) {
        dbg(lvl_error, "Failed to start RTL-SDR hardware");
        aprs_sdr_dsp_destroy(priv->dsp);
        aprs_sdr_hw_destroy(priv->hw);
        g_free(priv);
        return NULL;
    }
    priv->running = 1;
    priv->initialized = 1;
    dbg(lvl_info, "APRS SDR plugin started: %.3f MHz", hw_config.frequency_mhz);
    return priv;
}

static void aprs_sdr_map_destroy(void *priv) {
    struct aprs_sdr_priv *sdr_priv = (struct aprs_sdr_priv *)priv;

    if (!sdr_priv)
        return;

    if (sdr_priv->running) {
        aprs_sdr_hw_stop(sdr_priv->hw);
    }

    aprs_sdr_dsp_destroy(sdr_priv->dsp);
    aprs_sdr_hw_destroy(sdr_priv->hw);
    g_free(sdr_priv);
}

static struct map_rect_priv *aprs_sdr_map_rect_new(void *priv, struct map_selection *sel) {
    (void)priv; /* Unused */
    (void)sel;  /* Unused */
    /* SDR plugin doesn't render map items - that's APRS plugin's job */
    return NULL;
}

static void aprs_sdr_map_rect_destroy(struct map_rect_priv *mr) { /* Nothing to do */
}

static struct item *aprs_sdr_map_get_item(struct map_rect_priv *mr) {
    return NULL;
}

static int aprs_sdr_map_get_attr(void *priv, enum attr_type type, struct attr *attr) {
    struct aprs_sdr_priv *sdr_priv = (struct aprs_sdr_priv *)priv;

    if (!sdr_priv)
        return 0;

    switch (type) {
    case attr_active:
        attr->type = attr_active;
        attr->u.num = sdr_priv->running ? 1 : 0;
        return 1;
    default:
        return 0;
    }
}

static int aprs_sdr_set_attr_frequency(struct aprs_sdr_priv *sdr_priv, const struct attr *attr) {
    double freq_mhz;
    if (!attr->u.numd || !sdr_priv->hw)
        return 0;
    freq_mhz = *attr->u.numd;
    if (!aprs_sdr_hw_set_frequency(sdr_priv->hw, freq_mhz))
        return 0;
    dbg(lvl_info, "SDR frequency set to %.3f MHz", freq_mhz);
    return 1;
}

static int aprs_sdr_set_attr_gain(struct aprs_sdr_priv *sdr_priv, const struct attr *attr) {
    if (!sdr_priv->hw)
        return 0;
    if (!aprs_sdr_hw_set_gain(sdr_priv->hw, attr->u.num))
        return 0;
    dbg(lvl_info, "SDR gain set to %d", attr->u.num);
    return 1;
}

static int aprs_sdr_set_attr_ppm(struct aprs_sdr_priv *sdr_priv, const struct attr *attr) {
    if (!sdr_priv->hw)
        return 0;
    if (!aprs_sdr_hw_set_ppm(sdr_priv->hw, attr->u.num))
        return 0;
    dbg(lvl_info, "SDR PPM correction set to %d", attr->u.num);
    return 1;
}

static int aprs_sdr_map_set_attr(void *priv, struct attr *attr) {
    struct aprs_sdr_priv *sdr_priv = (struct aprs_sdr_priv *)priv;

    if (!sdr_priv)
        return 0;
    switch (attr->type) {
    case attr_frequency:
        return aprs_sdr_set_attr_frequency(sdr_priv, attr);
    case attr_gain:
        return aprs_sdr_set_attr_gain(sdr_priv, attr);
    case attr_ppm:
        return aprs_sdr_set_attr_ppm(sdr_priv, attr);
    default:
        return 0;
    }
}

void plugin_init(void) {
    aprs_sdr_map_meth.map_destroy = aprs_sdr_map_destroy;
    aprs_sdr_map_meth.map_rect_new = aprs_sdr_map_rect_new;
    aprs_sdr_map_meth.map_rect_destroy = aprs_sdr_map_rect_destroy;
    aprs_sdr_map_meth.map_rect_get_item = aprs_sdr_map_get_item;
    aprs_sdr_map_meth.map_get_attr = aprs_sdr_map_get_attr;
    aprs_sdr_map_meth.map_set_attr = aprs_sdr_map_set_attr;

    dbg(lvl_debug, "APRS SDR plugin initializing");
    plugin_register_category_map("aprs_sdr", aprs_sdr_map_new);
    dbg(lvl_info, "APRS SDR plugin registered");
}
