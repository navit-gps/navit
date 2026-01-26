/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024 Navit Team
 *
 * Unit tests for Bell 202 demodulation (DSP)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include "../../debug.h"
#include "../../aprs_sdr/aprs_sdr_dsp.h"

/* Stub for debug_printf and max_debug_level for unit tests */
dbg_level max_debug_level = lvl_error;

void debug_printf(dbg_level level, const char *module, const int mlen, const char *function, const int flen, int prefix, const char *fmt, ...) {
    (void)level;
    (void)module;
    (void)mlen;
    (void)function;
    (void)flen;
    (void)prefix;
    (void)fmt;
    /* No-op for tests */
}

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, message); \
            return 1; \
        } \
    } while(0)

static int test_dsp_create_destroy(void) {
    struct aprs_sdr_dsp_config config = {
        .sample_rate = 48000,
        .mark_freq = 1200.0,
        .space_freq = 2200.0,
        .baud_rate = 1200.0
    };
    
    struct aprs_sdr_dsp *dsp = aprs_sdr_dsp_new(&config);
    TEST_ASSERT(dsp != NULL, "DSP creation failed");
    aprs_sdr_dsp_destroy(dsp);
    return 0;
}

static int test_dsp_config_defaults(void) {
    struct aprs_sdr_dsp_config config = {
        .sample_rate = 48000,
        .mark_freq = 0.0,  /* Should use default */
        .space_freq = 0.0, /* Should use default */
        .baud_rate = 0.0   /* Should use default */
    };
    
    struct aprs_sdr_dsp *dsp = aprs_sdr_dsp_new(&config);
    TEST_ASSERT(dsp != NULL, "DSP creation with defaults failed");
    aprs_sdr_dsp_destroy(dsp);
    return 0;
}

static int test_dsp_process_samples(void) {
    struct aprs_sdr_dsp_config config = {
        .sample_rate = 48000,
        .mark_freq = 1200.0,
        .space_freq = 2200.0,
        .baud_rate = 1200.0
    };
    
    struct aprs_sdr_dsp *dsp = aprs_sdr_dsp_new(&config);
    TEST_ASSERT(dsp != NULL, "DSP creation failed");
    
    /* Create dummy I/Q samples (interleaved, 8-bit) */
    unsigned char samples[1000];
    for (int i = 0; i < 1000; i++) {
        samples[i] = 128 + (i % 256); /* Simple pattern */
    }
    
    /* Process samples - should not crash */
    int frames = aprs_sdr_dsp_process_samples(dsp, samples, sizeof(samples));
    /* May return 0 if no valid frames decoded, which is OK for test data */
    TEST_ASSERT(frames >= 0, "DSP processing failed");
    
    aprs_sdr_dsp_destroy(dsp);
    return 0;
}

static int callback_called = 0;

static void test_callback(const unsigned char *frame, int length, void *user_data) {
    (void)frame;
    (void)length;
    (void)user_data;
    callback_called = 1;
}

static int test_dsp_callback(void) {
    struct aprs_sdr_dsp_config config = {
        .sample_rate = 48000,
        .mark_freq = 1200.0,
        .space_freq = 2200.0,
        .baud_rate = 1200.0
    };
    
    struct aprs_sdr_dsp *dsp = aprs_sdr_dsp_new(&config);
    TEST_ASSERT(dsp != NULL, "DSP creation failed");
    
    callback_called = 0;
    int result = aprs_sdr_dsp_set_frame_callback(dsp, test_callback, NULL);
    TEST_ASSERT(result == 1, "Callback registration failed");
    
    aprs_sdr_dsp_destroy(dsp);
    return 0;
}

int main(void) {
    int failures = 0;
    
    printf("Running APRS DSP tests...\n");
    
    failures += test_dsp_create_destroy();
    failures += test_dsp_config_defaults();
    failures += test_dsp_process_samples();
    failures += test_dsp_callback();
    
    if (failures == 0) {
        printf("All DSP tests passed!\n");
        return 0;
    } else {
        printf("%d test(s) failed\n", failures);
        return 1;
    }
}

