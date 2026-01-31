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

#include "aprs_sdr_hw.h"
#include "config.h"
#include "debug.h"
#include <glib.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_RTLSDR
#include <errno.h>
#include <pthread.h>
#include <rtl-sdr.h>
#include <unistd.h>

struct aprs_sdr_hw {
    rtlsdr_dev_t *dev;
    struct aprs_sdr_hw_config config;
    aprs_sdr_hw_callback callback;
    void *callback_user_data;
    pthread_t thread;
    int running;
    int stop_requested;
};

static void *aprs_sdr_hw_read_thread(void *arg) {
    struct aprs_sdr_hw *hw = (struct aprs_sdr_hw *)arg;
    unsigned char *buffer;
    int buffer_size = 16384;
    int n_read;
    int result;

    buffer = g_malloc(buffer_size);
    if (!buffer) {
        dbg(lvl_error, "Failed to allocate RTL-SDR buffer");
        return NULL;
    }

    dbg(lvl_debug, "RTL-SDR read thread started");

    while (!hw->stop_requested) {
        result = rtlsdr_read_sync(hw->dev, buffer, buffer_size, &n_read);

        if (result < 0) {
            dbg(lvl_error, "RTL-SDR read error: %d", result);
            break;
        }

        if (n_read > 0 && hw->callback) {
            hw->callback(buffer, n_read, hw->callback_user_data);
        }

        if (n_read < buffer_size) {
            usleep(1000);
        }
    }

    g_free(buffer);
    dbg(lvl_debug, "RTL-SDR read thread stopped");
    return NULL;
}

static enum aprs_sdr_device_type aprs_sdr_hw_detect_type(rtlsdr_dev_t *dev) {
    char manufact[256], product[256], serial[256];

    if (rtlsdr_get_usb_strings(dev, manufact, product, serial) < 0) {
        return APRS_SDR_GENERIC;
    }

    dbg(lvl_debug, "RTL-SDR device: %s %s %s", manufact, product, serial);

    if (strstr(manufact, "Nooelec") || strstr(product, "Nooelec") || strstr(manufact, "NESDR")) {
        return APRS_SDR_NOOELEC;
    }

    if (strstr(product, "R828D") || strstr(serial, "R828D")) {
        return APRS_SDR_V4_R828D;
    }

    if (strstr(product, "R820T2") || strstr(serial, "R820T2") || strstr(product, "Blog")) {
        return APRS_SDR_BLOG_V3;
    }

    return APRS_SDR_GENERIC;
}

static int rtlsdr_configure_gain(rtlsdr_dev_t *dev, int gain) {
    int result;
    if (gain >= 0) {
        result = rtlsdr_set_tuner_gain_mode(dev, 1);
        if (result < 0) {
            dbg(lvl_error, "Failed to set gain mode: %d", result);
            return -1;
        }
        result = rtlsdr_set_tuner_gain(dev, gain * 10);
        if (result < 0) {
            dbg(lvl_error, "Failed to set gain: %d", result);
        }
    } else {
        rtlsdr_set_tuner_gain_mode(dev, 0);
    }
    return 0;
}

static int rtlsdr_configure_after_open(struct aprs_sdr_hw *hw) {
    rtlsdr_dev_t *dev = hw->dev;
    int result;

    result = rtlsdr_set_sample_rate(dev, hw->config.sample_rate);
    if (result < 0) {
        dbg(lvl_error, "Failed to set sample rate: %d", result);
        return -1;
    }
    result = rtlsdr_set_center_freq(dev, (uint32_t)(hw->config.frequency_mhz * 1000000));
    if (result < 0) {
        dbg(lvl_error, "Failed to set frequency: %d", result);
        return -1;
    }
    if (hw->config.ppm_correction != 0) {
        rtlsdr_set_freq_correction(dev, hw->config.ppm_correction);
    }
    rtlsdr_configure_gain(dev, hw->config.gain);
    result = rtlsdr_set_agc_mode(dev, 0);
    if (result < 0) {
        dbg(lvl_warning, "Failed to set AGC mode: %d", result);
    }
    result = rtlsdr_reset_buffer(dev);
    if (result < 0) {
        dbg(lvl_error, "Failed to reset buffer: %d", result);
        return -1;
    }
    return 0;
}

struct aprs_sdr_hw *aprs_sdr_hw_new(const struct aprs_sdr_hw_config *config) {
    struct aprs_sdr_hw *hw;
    int device_count;
    int result;
    rtlsdr_dev_t *dev = NULL;

    if (!config) {
        dbg(lvl_error, "RTL-SDR config is NULL");
        return NULL;
    }
    device_count = rtlsdr_get_device_count();
    if (device_count < 1) {
        dbg(lvl_error, "No RTL-SDR devices found");
        return NULL;
    }
    dbg(lvl_info, "Found %d RTL-SDR device(s)", device_count);
    if (config->device_index >= device_count) {
        dbg(lvl_error, "Device index %d out of range (0-%d)", config->device_index, device_count - 1);
        return NULL;
    }
    result = rtlsdr_open(&dev, config->device_index);
    if (result < 0) {
        dbg(lvl_error, "Failed to open RTL-SDR device %d: %d", config->device_index, result);
        return NULL;
    }
    hw = g_new0(struct aprs_sdr_hw, 1);
    hw->dev = dev;
    hw->config = *config;
    if (hw->config.device_type == APRS_SDR_UNKNOWN) {
        hw->config.device_type = aprs_sdr_hw_detect_type(dev);
    }
    dbg(lvl_info, "RTL-SDR device type: %d", hw->config.device_type);
    if (rtlsdr_configure_after_open(hw) < 0) {
        aprs_sdr_hw_destroy(hw);
        return NULL;
    }
    dbg(lvl_info, "RTL-SDR initialized: %.3f MHz, %d Hz, gain=%d, ppm=%d", hw->config.frequency_mhz,
        hw->config.sample_rate, hw->config.gain, hw->config.ppm_correction);
    return hw;
}

void aprs_sdr_hw_destroy(struct aprs_sdr_hw *hw) {
    if (!hw)
        return;

    if (hw->running) {
        aprs_sdr_hw_stop(hw);
    }

    if (hw->dev) {
        rtlsdr_close(hw->dev);
    }

    g_free(hw);
}

int aprs_sdr_hw_start(struct aprs_sdr_hw *hw) {
    int result;

    if (!hw)
        return 0;
    if (hw->running)
        return 1;

    result = rtlsdr_reset_buffer(hw->dev);
    if (result < 0) {
        dbg(lvl_error, "Failed to reset buffer: %d", result);
        return 0;
    }

    hw->stop_requested = 0;
    memset(&hw->thread, 0, sizeof(hw->thread));
    result = pthread_create(&hw->thread, NULL, aprs_sdr_hw_read_thread, hw);
    if (result != 0) {
        dbg(lvl_error, "Failed to create RTL-SDR thread: %s", strerror(result));
        return 0;
    }

    hw->running = 1;
    dbg(lvl_info, "RTL-SDR started");
    return 1;
}

int aprs_sdr_hw_stop(struct aprs_sdr_hw *hw) {
    if (!hw || !hw->running)
        return 1;

    hw->stop_requested = 1;
    rtlsdr_cancel_async(hw->dev);

    pthread_join(hw->thread, NULL);
    hw->running = 0;

    dbg(lvl_info, "RTL-SDR stopped");
    return 1;
}

int aprs_sdr_hw_is_running(struct aprs_sdr_hw *hw) { return hw ? hw->running : 0; }

int aprs_sdr_hw_detect_devices(int *count) {
    if (!count)
        return 0;

    *count = rtlsdr_get_device_count();
    return 1;
}

int aprs_sdr_hw_get_device_info(int index, char *name, size_t name_len, enum aprs_sdr_device_type *type) {
    rtlsdr_dev_t *dev;
    char manufact[256], product[256], serial[256];
    int result;

    if (index >= rtlsdr_get_device_count()) {
        return 0;
    }

    result = rtlsdr_open(&dev, index);
    if (result < 0) {
        return 0;
    }

    result = rtlsdr_get_usb_strings(dev, manufact, product, serial);
    if (result >= 0 && name && name_len > 0) {
        snprintf(name, name_len, "%s %s", manufact, product);
    }

    if (type) {
        *type = aprs_sdr_hw_detect_type(dev);
    }

    rtlsdr_close(dev);
    return 1;
}

int aprs_sdr_hw_set_callback(struct aprs_sdr_hw *hw, aprs_sdr_hw_callback cb, void *user_data) {
    if (!hw)
        return 0;
    hw->callback = cb;
    hw->callback_user_data = user_data;
    return 1;
}

int aprs_sdr_hw_set_frequency(struct aprs_sdr_hw *hw, double frequency_mhz) {
    if (!hw || !hw->dev)
        return 0;

    int result = rtlsdr_set_center_freq(hw->dev, (uint32_t)(frequency_mhz * 1000000));
    if (result >= 0) {
        hw->config.frequency_mhz = frequency_mhz;
    }
    return result >= 0;
}

int aprs_sdr_hw_set_gain(struct aprs_sdr_hw *hw, int gain) {
    if (!hw || !hw->dev)
        return 0;

    int result;
    if (gain >= 0) {
        result = rtlsdr_set_tuner_gain_mode(hw->dev, 1);
        if (result >= 0) {
            result = rtlsdr_set_tuner_gain(hw->dev, gain * 10);
        }
    } else {
        result = rtlsdr_set_tuner_gain_mode(hw->dev, 0);
    }

    if (result >= 0) {
        hw->config.gain = gain;
    }
    return result >= 0;
}

int aprs_sdr_hw_set_ppm(struct aprs_sdr_hw *hw, int ppm) {
    if (!hw || !hw->dev)
        return 0;

    int result = rtlsdr_set_freq_correction(hw->dev, ppm);
    if (result >= 0) {
        hw->config.ppm_correction = ppm;
    }
    return result >= 0;
}

#else /* HAVE_RTLSDR */

struct aprs_sdr_hw {
    int dummy;
};

struct aprs_sdr_hw *aprs_sdr_hw_new(const struct aprs_sdr_hw_config *config) {
    dbg(lvl_error, "RTL-SDR support not compiled in");
    return NULL;
}

void aprs_sdr_hw_destroy(struct aprs_sdr_hw *hw) {}

int aprs_sdr_hw_start(struct aprs_sdr_hw *hw) { return 0; }
int aprs_sdr_hw_stop(struct aprs_sdr_hw *hw) { return 0; }
int aprs_sdr_hw_is_running(struct aprs_sdr_hw *hw) { return 0; }
int aprs_sdr_hw_detect_devices(int *count) {
    if (count)
        *count = 0;
    return 0;
}
int aprs_sdr_hw_get_device_info(int index, char *name, size_t name_len, enum aprs_sdr_device_type *type) { return 0; }
int aprs_sdr_hw_set_callback(struct aprs_sdr_hw *hw, aprs_sdr_hw_callback cb, void *user_data) { return 0; }
int aprs_sdr_hw_set_frequency(struct aprs_sdr_hw *hw, double frequency_mhz) { return 0; }
int aprs_sdr_hw_set_gain(struct aprs_sdr_hw *hw, int gain) { return 0; }
int aprs_sdr_hw_set_ppm(struct aprs_sdr_hw *hw, int ppm) { return 0; }

#endif /* HAVE_RTLSDR */
