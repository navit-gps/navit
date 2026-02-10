/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024 Navit Team
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

#include "aprs_rtlsdr.h"
#include "config.h"
#include "debug.h"
#include <glib.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_RTLSDR
#    include <errno.h>
#    include <pthread.h>
#    include <rtl-sdr.h>
#    include <unistd.h>

struct aprs_rtlsdr {
    rtlsdr_dev_t *dev;
    struct aprs_rtlsdr_config config;
    aprs_rtlsdr_callback callback;
    void *callback_user_data;
    pthread_t thread;
    int running;
    int stop_requested;
};

static void *aprs_rtlsdr_read_thread(void *arg) {
    struct aprs_rtlsdr *rtl = (struct aprs_rtlsdr *)arg;
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

    while (!rtl->stop_requested) {
        result = rtlsdr_read_sync(rtl->dev, buffer, buffer_size, &n_read);

        if (result < 0) {
            dbg(lvl_error, "RTL-SDR read error: %d", result);
            break;
        }

        if (n_read > 0 && rtl->callback) {
            rtl->callback(buffer, n_read, rtl->callback_user_data);
        }

        if (n_read < buffer_size) {
            usleep(1000);
        }
    }

    g_free(buffer);
    dbg(lvl_debug, "RTL-SDR read thread stopped");
    return NULL;
}

static enum aprs_rtlsdr_device_type aprs_rtlsdr_detect_type(rtlsdr_dev_t *dev) {
    char manufact[256], product[256], serial[256];

    if (rtlsdr_get_usb_strings(dev, manufact, product, serial) < 0) {
        return APRS_RTLSDR_GENERIC;
    }

    dbg(lvl_debug, "RTL-SDR device: %s %s %s", manufact, product, serial);

    if (strstr(manufact, "Nooelec") || strstr(product, "Nooelec") || strstr(manufact, "NESDR")) {
        return APRS_RTLSDR_NOOELEC;
    }

    if (strstr(product, "R828D") || strstr(serial, "R828D")) {
        return APRS_RTLSDR_V4_R828D;
    }

    if (strstr(product, "R820T2") || strstr(serial, "R820T2") || strstr(product, "Blog")) {
        return APRS_RTLSDR_BLOG_V3;
    }

    return APRS_RTLSDR_GENERIC;
}

/* Validate RTL-SDR device index */
static int aprs_rtlsdr_validate_device_index(int device_index) {
    int device_count = rtlsdr_get_device_count();
    if (device_count < 1) {
        dbg(lvl_error, "No RTL-SDR devices found");
        return 0;
    }

    dbg(lvl_info, "Found %d RTL-SDR device(s)", device_count);

    if (device_index >= device_count) {
        dbg(lvl_error, "Device index %d out of range (0-%d)", device_index, device_count - 1);
        return 0;
    }

    return 1;
}

/* Open RTL-SDR device */
static rtlsdr_dev_t *aprs_rtlsdr_open_device(int device_index) {
    rtlsdr_dev_t *dev = NULL;
    int result = rtlsdr_open(&dev, device_index);
    if (result < 0) {
        dbg(lvl_error, "Failed to open RTL-SDR device %d: %d", device_index, result);
        return NULL;
    }
    return dev;
}

/* Configure RTL-SDR sample rate and frequency */
static int aprs_rtlsdr_configure_rf(rtlsdr_dev_t *dev, double frequency_mhz, int sample_rate) {
    int result = rtlsdr_set_sample_rate(dev, sample_rate);
    if (result < 0) {
        dbg(lvl_error, "Failed to set sample rate: %d", result);
        return 0;
    }

    result = rtlsdr_set_center_freq(dev, (uint32_t)(frequency_mhz * 1000000));
    if (result < 0) {
        dbg(lvl_error, "Failed to set frequency: %d", result);
        return 0;
    }

    return 1;
}

/* Configure RTL-SDR gain settings */
static void aprs_rtlsdr_configure_gain(rtlsdr_dev_t *dev, int gain) {
    if (gain >= 0) {
        int result = rtlsdr_set_tuner_gain_mode(dev, 1);
        if (result < 0) {
            dbg(lvl_error, "Failed to set gain mode: %d", result);
        } else {
            result = rtlsdr_set_tuner_gain(dev, gain * 10);
            if (result < 0) {
                dbg(lvl_error, "Failed to set gain: %d", result);
            }
        }
    } else {
        rtlsdr_set_tuner_gain_mode(dev, 0);
    }
}

/* Configure RTL-SDR device settings */
static int aprs_rtlsdr_configure_device(struct aprs_rtlsdr *rtl) {
    if (!aprs_rtlsdr_configure_rf(rtl->dev, rtl->config.frequency_mhz, rtl->config.sample_rate)) {
        return 0;
    }

    if (rtl->config.ppm_correction != 0) {
        rtlsdr_set_freq_correction(rtl->dev, rtl->config.ppm_correction);
    }

    aprs_rtlsdr_configure_gain(rtl->dev, rtl->config.gain);

    int result = rtlsdr_set_agc_mode(rtl->dev, 0);
    if (result < 0) {
        dbg(lvl_warning, "Failed to set AGC mode: %d", result);
    }

    result = rtlsdr_reset_buffer(rtl->dev);
    if (result < 0) {
        dbg(lvl_error, "Failed to reset buffer: %d", result);
        return 0;
    }

    return 1;
}

struct aprs_rtlsdr *aprs_rtlsdr_new(const struct aprs_rtlsdr_config *config) {
    if (!config) {
        dbg(lvl_error, "RTL-SDR config is NULL");
        return NULL;
    }

    if (!aprs_rtlsdr_validate_device_index(config->device_index)) {
        return NULL;
    }

    rtlsdr_dev_t *dev = aprs_rtlsdr_open_device(config->device_index);
    if (!dev) {
        return NULL;
    }

    struct aprs_rtlsdr *rtl = g_new0(struct aprs_rtlsdr, 1);
    rtl->dev = dev;
    rtl->config = *config;

    enum aprs_rtlsdr_device_type detected_type = aprs_rtlsdr_detect_type(dev);
    if (rtl->config.device_type == APRS_RTLSDR_UNKNOWN) {
        rtl->config.device_type = detected_type;
    }

    dbg(lvl_info, "RTL-SDR device type: %d", rtl->config.device_type);

    if (!aprs_rtlsdr_configure_device(rtl)) {
        aprs_rtlsdr_destroy(rtl);
        return NULL;
    }

    dbg(lvl_info, "RTL-SDR initialized: %.3f MHz, %d Hz, gain=%d, ppm=%d", rtl->config.frequency_mhz,
        rtl->config.sample_rate, rtl->config.gain, rtl->config.ppm_correction);

    return rtl;
}

void aprs_rtlsdr_destroy(struct aprs_rtlsdr *rtl) {
    if (!rtl) return;

    if (rtl->running) {
        aprs_rtlsdr_stop(rtl);
    }

    if (rtl->dev) {
        rtlsdr_close(rtl->dev);
    }

    g_free(rtl);
}

int aprs_rtlsdr_start(struct aprs_rtlsdr *rtl) {
    int result;

    if (!rtl)
        return 0;
    if (rtl->running)
        return 1;

    result = rtlsdr_reset_buffer(rtl->dev);
    if (result < 0) {
        dbg(lvl_error, "Failed to reset buffer: %d", result);
        return 0;
    }

    rtl->stop_requested = 0;
    result = pthread_create(&rtl->thread, NULL, aprs_rtlsdr_read_thread, rtl);
    if (result != 0) {
        dbg(lvl_error, "Failed to create RTL-SDR thread: %s", strerror(result));
        return 0;
    }

    rtl->running = 1;
    dbg(lvl_info, "RTL-SDR started");
    return 1;
}

int aprs_rtlsdr_stop(struct aprs_rtlsdr *rtl) {
    if (!rtl || !rtl->running)
        return 1;

    rtl->stop_requested = 1;
    rtlsdr_cancel_async(rtl->dev);

    pthread_join(rtl->thread, NULL);
    rtl->running = 0;

    dbg(lvl_info, "RTL-SDR stopped");
    return 1;
}

int aprs_rtlsdr_is_running(struct aprs_rtlsdr *rtl) {
    return rtl ? rtl->running : 0;
}

int aprs_rtlsdr_detect_devices(int *count) {
    if (!count)
        return 0;

#    ifdef HAVE_RTLSDR
    *count = rtlsdr_get_device_count();
    return 1;
#else
    *count = 0;
    return 0;
#    endif
}

int aprs_rtlsdr_get_device_info(int index, char *name, size_t name_len, enum aprs_rtlsdr_device_type *type) {
#    ifdef HAVE_RTLSDR
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
        *type = aprs_rtlsdr_detect_type(dev);
    }

    rtlsdr_close(dev);
    return 1;
#    else
    return 0;
#    endif
}

int aprs_rtlsdr_set_callback(struct aprs_rtlsdr *rtl, aprs_rtlsdr_callback cb, void *user_data) {
    if (!rtl)
        return 0;
    rtl->callback = cb;
    rtl->callback_user_data = user_data;
    return 1;
}

int aprs_rtlsdr_set_frequency(struct aprs_rtlsdr *rtl, double frequency_mhz) {
    if (!rtl || !rtl->dev)
        return 0;

    int result = rtlsdr_set_center_freq(rtl->dev, (uint32_t)(frequency_mhz * 1000000));
    if (result >= 0) {
        rtl->config.frequency_mhz = frequency_mhz;
    }
    return result >= 0;
}

int aprs_rtlsdr_set_gain(struct aprs_rtlsdr *rtl, int gain) {
    if (!rtl || !rtl->dev)
        return 0;

    int result;
    if (gain >= 0) {
        result = rtlsdr_set_tuner_gain_mode(rtl->dev, 1);
        if (result >= 0) {
            result = rtlsdr_set_tuner_gain(rtl->dev, gain * 10);
        }
    } else {
        result = rtlsdr_set_tuner_gain_mode(rtl->dev, 0);
    }

    if (result >= 0) {
        rtl->config.gain = gain;
    }
    return result >= 0;
}

int aprs_rtlsdr_set_ppm(struct aprs_rtlsdr *rtl, int ppm) {
    if (!rtl || !rtl->dev)
        return 0;

    int result = rtlsdr_set_freq_correction(rtl->dev, ppm);
    if (result >= 0) {
        rtl->config.ppm_correction = ppm;
    }
    return result >= 0;
}

#else /* HAVE_RTLSDR */

struct aprs_rtlsdr {
    int dummy;
};

struct aprs_rtlsdr *aprs_rtlsdr_new(const struct aprs_rtlsdr_config *config) {
    dbg(lvl_error, "RTL-SDR support not compiled in");
    return NULL;
}

void aprs_rtlsdr_destroy(struct aprs_rtlsdr *rtl) {
}

int aprs_rtlsdr_start(struct aprs_rtlsdr *rtl) {
    return 0;
}
int aprs_rtlsdr_stop(struct aprs_rtlsdr *rtl) {
    return 0;
}
int aprs_rtlsdr_is_running(struct aprs_rtlsdr *rtl) {
    return 0;
}
int aprs_rtlsdr_detect_devices(int *count) {
    if (count)
        *count = 0;
    return 0;
}
int aprs_rtlsdr_get_device_info(int index, char *name, size_t name_len, enum aprs_rtlsdr_device_type *type) {
    return 0;
}
int aprs_rtlsdr_set_callback(struct aprs_rtlsdr *rtl, aprs_rtlsdr_callback cb, void *user_data) {
    return 0;
}
int aprs_rtlsdr_set_frequency(struct aprs_rtlsdr *rtl, double frequency_mhz) {
    return 0;
}
int aprs_rtlsdr_set_gain(struct aprs_rtlsdr *rtl, int gain) {
    return 0;
}
int aprs_rtlsdr_set_ppm(struct aprs_rtlsdr *rtl, int ppm) {
    return 0;
}

#endif /* HAVE_RTLSDR */

