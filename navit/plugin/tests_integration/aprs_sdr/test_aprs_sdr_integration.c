/**
 * Navit, a modular navigation system.
 *
 * Integration tests for APRS SDR DSP + AX.25 decode.
 *
 * These tests generate synthetic Bell 202 APRS signals at RF sample rate
 * and feed them through aprs_sdr_dsp, then decode the resulting AX.25 frame
 * with aprs_decode_ax25() to verify end-to-end correctness without hardware.
 */

#include "../../aprs/aprs_decode.h"
#include "../../aprs_sdr/aprs_sdr_dsp.h"
#include "../../debug.h"

#include <glib.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef M_PI
#    define M_PI 3.14159265358979323846
#endif

/* Stub for debug_printf and max_debug_level for unit tests */
dbg_level max_debug_level = lvl_error;

void debug_printf(dbg_level level, const char *module, const int mlen, const char *function, const int flen, int prefix,
                  const char *fmt, ...) {
    (void)level;
    (void)module;
    (void)mlen;
    (void)function;
    (void)flen;
    (void)prefix;
    (void)fmt;
    /* No-op in tests */
}

#define TEST_ASSERT(condition, message)                                                                                \
    do {                                                                                                               \
        if (!(condition)) {                                                                                            \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, (message));                                       \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

struct test_frame_accumulator {
    unsigned char *frame;
    int length;
    int count;
};

static void test_frame_callback(const unsigned char *frame, int length, void *user_data) {
    struct test_frame_accumulator *acc = (struct test_frame_accumulator *)user_data;
    if (!acc || !frame || length <= 0) {
        return;
    }
    if (!acc->frame) {
        acc->frame = g_memdup2(frame, length);
        acc->length = length;
    }
    acc->count++;
}

/**
 * Build a complete AX.25 UI frame for KG5XXX>APRS with CRC-CCITT FCS.
 *
 * Layout (without HDLC flags):
 *   - Destination: APRS
 *   - Source:      KG5XXX
 *   - Control:     0x03 (UI frame)
 *   - PID:         0xF0 (no layer 3)
 *   - Info:        "!5132.00N/00007.00W-Test"
 *   - FCS:         CRC-CCITT (0x1021, init 0xFFFF), appended little-endian
 */
static unsigned char *encode_ax25_frame(int *out_len) {
    const char *dest_callsign = "APRS";
    const char *src_callsign = "KG5XXX";
    const char *info_str = "!5132.00N/00007.00W-Test";

    unsigned char addr[14];
    unsigned char control = 0x03;
    unsigned char pid = 0xF0;
    size_t info_len = strlen(info_str);
    size_t payload_len = sizeof(addr) + 1 + 1 + info_len; /* addr + control + pid + info */
    size_t frame_len = payload_len + 2;                   /* + FCS */
    unsigned char *frame = g_malloc(frame_len);
    unsigned short crc = 0xFFFF;
    size_t i;

    memset(addr, ' ' << 1, sizeof(addr));

    /* Destination: APRS (6 chars, padded with spaces) */
    for (i = 0; i < 6 && dest_callsign[i]; i++) {
        addr[i] = (unsigned char)(dest_callsign[i] << 1);
    }
    addr[6] = 0x60; /* Destination SSID byte, no SSID, end-of-address bit clear */

    /* Source: KG5XXX (padded with spaces) */
    for (i = 0; i < 6 && src_callsign[i]; i++) {
        addr[7 + i] = (unsigned char)(src_callsign[i] << 1);
    }
    addr[13] = 0x61; /* Source SSID byte, no SSID, end-of-address bit set */

    memcpy(frame, addr, sizeof(addr));
    frame[14] = control;
    frame[15] = pid;
    memcpy(frame + 16, info_str, info_len);

    /* CRC-CCITT (0x1021, init 0xFFFF), non-reflected, over payload. */
    for (i = 0; i < payload_len; i++) {
        unsigned char b = frame[i];
        int bit;
        crc ^= (unsigned short)(b << 8);
        for (bit = 0; bit < 8; bit++) {
            if (crc & 0x8000) {
                crc = (unsigned short)((crc << 1) ^ 0x1021);
            } else {
                crc <<= 1;
            }
        }
    }

    /* Append FCS little-endian */
    frame[payload_len] = (unsigned char)(crc & 0xFF);
    frame[payload_len + 1] = (unsigned char)((crc >> 8) & 0xFF);

    *out_len = (int)frame_len;
    return frame;
}

/**
 * Append HDLC flags and apply bit-stuffing.
 *
 * - Prepends opening flag 0x7E (LSB-first).
 * - Appends closing flag 0x7E (LSB-first).
 * - Inserts a 0 after every run of five 1 bits between flags.
 */
static void encode_flag_bits(GArray *bitstream, int *stuff_count) {
    const guint8 flag = 0x7E;
    int i;

    /* Reset stuff counter before flag */
    *stuff_count = 0;
    /* Transmit 0x7E raw, LSB first, no stuffing check */
    for (i = 0; i < 8; i++) {
        guint8 bit = (guint8)((flag >> i) & 1);
        g_array_append_val(bitstream, bit);
    }
    /* Reset stuff counter after flag */
    *stuff_count = 0;
}

static void encode_data_byte_bits(GArray *bitstream, unsigned char byte, int *stuff_count) {
    int i;
    for (i = 0; i < 8; i++) {
        guint8 b = (guint8)((byte >> i) & 1);
        g_array_append_val(bitstream, b);
        if (b == 1) {
            (*stuff_count)++;
            if (*stuff_count == 5) {
                guint8 zero = 0;
                g_array_append_val(bitstream, zero);
                *stuff_count = 0;
            }
        } else {
            *stuff_count = 0;
        }
    }
}

static void append_flag_and_stuff_bits(const unsigned char *frame, int frame_len, GArray *bitstream) {
    int stuff_count = 0;
    int i;

    /* Preamble: 16 x 0x7E flags (LSB-first) */
    for (i = 0; i < 16; i++) {
        encode_flag_bits(bitstream, &stuff_count);
    }

    /* Opening flag: 0x7E (unstuffed) */
    encode_flag_bits(bitstream, &stuff_count);

    /* Frame bytes (payload + FCS) with bit stuffing */
    stuff_count = 0;
    for (i = 0; i < frame_len; i++) {
        encode_data_byte_bits(bitstream, frame[i], &stuff_count);
    }

    /* Closing flag (unstuffed) */
    encode_flag_bits(bitstream, &stuff_count);
}

/**
 * NRZI encode a bitstream.
 *
 * - Initial line state = 1.
 * - Bit 0 => transition (state ^= 1).
 * - Bit 1 => no transition.
 * - Output is the line state for each input bit.
 */
static GArray *nrzi_encode_bits(const GArray *bits) {
    GArray *encoded = g_array_sized_new(FALSE, FALSE, sizeof(unsigned char), bits->len);
    unsigned char state = 1;
    gint i;

    for (i = 0; i < (gint)bits->len; i++) {
        unsigned char b = g_array_index((GArray *)bits, unsigned char, i);
        if (b == 0) {
            state ^= 1;
        }
        g_array_append_val(encoded, state);
    }

    return encoded;
}

/* Simple Gaussian noise generator using Box–Muller transform. */
static double gaussian_noise(double stddev) {
    double u1 = (rand() + 1.0) / ((double)RAND_MAX + 2.0);
    double u2 = (rand() + 1.0) / ((double)RAND_MAX + 2.0);
    double mag = stddev * sqrt(-2.0 * log(u1));
    double z0 = mag * cos(2.0 * M_PI * u2);
    return z0;
}

/**
 * Emit one noisy IQ sample into the interleaved buffer and advance phase and index.
 */
static void write_noisy_iq_sample(unsigned char *iq, int *sample_index, double rf_freq, int rf_sample_rate,
                                  double noise_std, double *phase) {
    int idx = *sample_index;
    double phase_inc = 2.0 * M_PI * rf_freq / (double)rf_sample_rate;

    double noisy_i = cos(*phase) + gaussian_noise(noise_std);
    double noisy_q = sin(*phase) + gaussian_noise(noise_std);

    double scale = 80.0; /* keep well within [-128,127] */
    int si = (int)(noisy_i * scale);
    int sq = (int)(noisy_q * scale);
    if (si < -128)
        si = -128;
    if (si > 127)
        si = 127;
    if (sq < -128)
        sq = -128;
    if (sq > 127)
        sq = 127;

    iq[2 * idx] = (unsigned char)(si + 128);
    iq[2 * idx + 1] = (unsigned char)(sq + 128);
    (*sample_index)++;

    *phase += phase_inc;
    if (*phase > M_PI)
        *phase -= 2.0 * M_PI;
    else if (*phase < -M_PI)
        *phase += 2.0 * M_PI;
}

/**
 * Generate Bell 202 FSK IQ for the NRZI bitstream.
 *
 * - RF sample rate: 192000 Hz
 * - Baud rate:      1200 bps
 * - Mark:           1200 Hz
 * - Space:          2200 Hz
 * - IF offset:      if_offset_hz (e.g. 100000.0 or 0.0)
 * - NRZI line 1 -> mark, 0 -> space.
 * - Adds Gaussian noise at ~30 dB SNR.
 * - Pads RF samples so that the downsampled audio (48 kHz) has a length
 *   that is a multiple of 40 samples (one Goertzel block per bit).
 *
 * Signal model used by the test:
 * - For each NRZI line bit, emit exactly 160 RF samples of a unit phasor:
 *     I = cos(phase), Q = sin(phase)
 *   where phase advances by 2π * rf_freq / rf_sample_rate each sample.
 * - rf_freq is (if_offset_hz + tone_freq), where tone_freq is 1200 or 2200.
 * - Samples are scaled into unsigned bytes with 128 as the zero center:
 *     stored as interleaved I/Q: I0 Q0 I1 Q1 ...
 * - The test uses deterministic seeding so the result is reproducible.
 */
static unsigned char *generate_fsk_iq(const GArray *nrzi_bits, int *out_len, double if_offset_hz) {
    const int rf_sample_rate = 192000;
    const int baud = 1200;
    const double mark_freq = 1200.0;
    const double space_freq = 2200.0;
    const int samples_per_bit = rf_sample_rate / baud;             /* 160 RF samples per bit */
    const int decimation_factor = 4;                               /* 192k -> 48k */
    const int audio_per_bit = samples_per_bit / decimation_factor; /* 40 audio samples per bit */

    const int total_bits = (int)nrzi_bits->len;
    const int total_audio_samples = total_bits * audio_per_bit;
    int audio_remainder = total_audio_samples % 40;
    int extra_audio_samples = audio_remainder ? (40 - audio_remainder) : 0;
    int extra_rf_samples = extra_audio_samples * decimation_factor;
    const int total_samples = (samples_per_bit * total_bits) + extra_rf_samples;

    unsigned char *iq = g_malloc(total_samples * 2);
    double phase = 0.0;
    int sample_index = 0;
    gint i;

    /* Common noise scaling for all samples (bits and padding). */
    double noise_std = 0.0;

    /* Seed noise deterministically for reproducible tests. */
    srand(42);

    for (i = 0; i < (gint)nrzi_bits->len; i++) {
        unsigned char line = g_array_index((GArray *)nrzi_bits, unsigned char, i);
        /* NRZI line state to tones: line=1 -> mark (1200 Hz), line=0 -> space (2200 Hz). */
        double rf_freq = if_offset_hz + (line ? mark_freq : space_freq);
        for (int j = 0; j < samples_per_bit; j++) {
            write_noisy_iq_sample(iq, &sample_index, rf_freq, rf_sample_rate, noise_std, &phase);
        }
    }

    /* Pad with idle mark tone to complete any partial 40-sample audio block. */
    while (sample_index < total_samples) {
        double rf_freq_pad = if_offset_hz + mark_freq;
        write_noisy_iq_sample(iq, &sample_index, rf_freq_pad, rf_sample_rate, noise_std, &phase);
    }

    /* Pad to multiple of 4 bytes with trailing zero-IQ (128,128). */
    {
        int total_bytes = total_samples * 2;
        int padded_bytes = total_bytes;
        while ((padded_bytes % 4) != 0) {
            padded_bytes++;
        }
        if (padded_bytes == total_bytes) {
            *out_len = total_bytes;
            return iq;
        }

        unsigned char *iq_padded = g_malloc(padded_bytes);
        memcpy(iq_padded, iq, total_bytes);
        for (int k = total_bytes; k < padded_bytes; k++) {
            iq_padded[k] = 128;
        }
        g_free(iq);
        *out_len = padded_bytes;
        return iq_padded;
    }
}

static int test_verify_expect_success(int frames, struct test_frame_accumulator *acc, struct aprs_packet *packet) {
    if (frames < 1) {
        fprintf(stderr, "Expected at least one complete frame from DSP\n");
        return 1;
    }
    if (!(acc->count >= 1 && acc->frame != NULL)) {
        fprintf(stderr, "Expected frame callback to be invoked at least once\n");
        return 1;
    }
    if (aprs_decode_ax25(acc->frame, acc->length, packet) != 1) {
        fprintf(stderr, "Frame must decode successfully\n");
        return 1;
    }
    if (!packet->source_callsign || strcmp(packet->source_callsign, "KG5XXX") != 0) {
        fprintf(stderr, "Source callsign must match KG5XXX (got '%s')\n",
                packet->source_callsign ? packet->source_callsign : "(null)");
        return 1;
    }
    if (!packet->information_field || strcmp(packet->information_field, "!5132.00N/00007.00W-Test") != 0) {
        fprintf(stderr, "Information field must match synthetic payload (got '%s')\n",
                packet->information_field ? packet->information_field : "(null)");
        return 1;
    }
    return 0;
}

/* Standalone decode of synthetic frame before DSP (expect_success path only). */
static int run_dsp_precheck_ax25_decode(unsigned char *frame, int frame_len) {
    struct aprs_packet pkt_check;

    memset(&pkt_check, 0, sizeof(pkt_check));
    if (aprs_decode_ax25(frame, frame_len, &pkt_check) != 1) {
        fprintf(stderr, "Synthetic AX.25 frame failed to decode pre-DSP\n");
        aprs_packet_free(&pkt_check);
        return 1;
    }
    aprs_packet_free(&pkt_check);
    return 0;
}

static int run_dsp_postcheck_results(int expect_success, int frames, struct test_frame_accumulator *acc,
                                     struct aprs_packet *packet) {
    if (expect_success) {
        return test_verify_expect_success(frames, acc, packet);
    }
    if (frames < 0) {
        fprintf(stderr, "DSP reported negative frame count\n");
        return 1;
    }
    return 0;
}

static void run_dsp_integration_teardown(struct aprs_sdr_dsp *dsp, unsigned char *frame, GArray *bits,
                                         GArray *nrzi_bits, unsigned char *iq, struct aprs_packet *packet,
                                         struct test_frame_accumulator *acc) {
    if (acc->frame) {
        g_free(acc->frame);
    }
    if (frame) {
        g_free(frame);
    }
    if (bits) {
        g_array_free(bits, TRUE);
    }
    if (nrzi_bits) {
        g_array_free(nrzi_bits, TRUE);
    }
    if (iq) {
        g_free(iq);
    }
    aprs_packet_free(packet);
    if (dsp) {
        aprs_sdr_dsp_destroy(dsp);
    }
}

static int run_dsp_and_decode(double if_offset_hz, int expect_success) {
    struct aprs_sdr_dsp_config config = {
        .rf_sample_rate = 192000,
        .audio_sample_rate = 48000,
        .if_offset_hz = if_offset_hz,
        .mark_freq = 1200.0,
        .space_freq = 2200.0,
        .baud_rate = 1200.0,
    };
    struct aprs_sdr_dsp *dsp = aprs_sdr_dsp_new(&config);
    struct test_frame_accumulator acc = {0};
    int frame_len = 0;
    unsigned char *frame = encode_ax25_frame(&frame_len);
    GArray *bits = NULL;
    GArray *nrzi_bits = NULL;
    unsigned char *iq = NULL;
    int iq_len = 0;
    int frames = 0;
    int rc = 0;
    struct aprs_packet packet;

    memset(&packet, 0, sizeof(packet));

    if (!dsp || !frame) {
        fprintf(stderr, "%s\n", !dsp ? "DSP creation failed" : "Frame allocation failed");
        rc = 1;
        run_dsp_integration_teardown(dsp, frame, bits, nrzi_bits, iq, &packet, &acc);
        return rc;
    }

    if (expect_success) {
        rc = run_dsp_precheck_ax25_decode(frame, frame_len);
        if (rc != 0) {
            run_dsp_integration_teardown(dsp, frame, bits, nrzi_bits, iq, &packet, &acc);
            return rc;
        }
    }

    bits = g_array_new(FALSE, FALSE, sizeof(unsigned char));
    append_flag_and_stuff_bits(frame, frame_len, bits);

    nrzi_bits = nrzi_encode_bits(bits);
    iq = generate_fsk_iq(nrzi_bits, &iq_len, if_offset_hz);

    aprs_sdr_dsp_set_frame_callback(dsp, test_frame_callback, &acc);
    frames = aprs_sdr_dsp_process_samples(dsp, iq, iq_len);

    rc = run_dsp_postcheck_results(expect_success, frames, &acc, &packet);

    run_dsp_integration_teardown(dsp, frame, bits, nrzi_bits, iq, &packet, &acc);
    return rc;
}

static int test_aprs_sdr_pll_preamble_lock(void) {
    struct aprs_sdr_dsp_config config = {
        .rf_sample_rate = 192000,
        .audio_sample_rate = 48000,
        .if_offset_hz = 100000.0,
        .mark_freq = 1200.0,
        .space_freq = 2200.0,
        .baud_rate = 1200.0,
        .pll_alpha = 0.08,
    };
    struct aprs_sdr_dsp *dsp = NULL;
    GArray *bits = NULL;
    GArray *nrzi = NULL;
    unsigned char *iq = NULL;
    int iq_len = 0;
    int stuff_count = 0;
    int i;
    int rc = 0;

    dsp = aprs_sdr_dsp_new(&config);
    if (!dsp) {
        fprintf(stderr, "PLL test: DSP creation failed\n");
        return 1;
    }
    bits = g_array_new(FALSE, FALSE, sizeof(unsigned char));
    for (i = 0; i < 16; i++) {
        encode_flag_bits(bits, &stuff_count);
    }
    nrzi = nrzi_encode_bits(bits);
    iq = generate_fsk_iq(nrzi, &iq_len, 100000.0);
    if (!iq) {
        fprintf(stderr, "PLL test: IQ generation failed\n");
        rc = 1;
        goto cleanup;
    }
    aprs_sdr_dsp_process_samples(dsp, iq, iq_len);
    if (!aprs_sdr_dsp_pll_locked(dsp)) {
        fprintf(stderr, "PLL test: expected pll_locked after preamble\n");
        rc = 1;
        goto cleanup;
    }
    if (aprs_sdr_dsp_pll_transition_count(dsp) < 8) {
        fprintf(stderr, "PLL test: expected pll_transition_count >= 8\n");
        rc = 1;
        goto cleanup;
    }
    {
        double ph = aprs_sdr_dsp_pll_phase(dsp);
        if (ph < 0.0 || ph >= 1.0) {
            fprintf(stderr, "PLL test: pll_phase not in [0,1)\n");
            rc = 1;
        }
    }
cleanup:
    if (iq) {
        g_free(iq);
    }
    if (nrzi) {
        g_array_free(nrzi, TRUE);
    }
    if (bits) {
        g_array_free(bits, TRUE);
    }
    if (dsp) {
        aprs_sdr_dsp_destroy(dsp);
    }
    return rc;
}

static int test_aprs_sdr_if_offset(void) {
    return run_dsp_and_decode(100000.0, 1);
}

static int test_aprs_sdr_dc_centered(void) {
    return run_dsp_and_decode(0.0, 0);
}

int main(void) {
    int failures = 0;

    printf("Running APRS SDR integration tests...\n");

    failures += test_aprs_sdr_pll_preamble_lock();
    failures += test_aprs_sdr_if_offset();
    failures += test_aprs_sdr_dc_centered();

    if (failures == 0) {
        printf("All APRS SDR integration tests passed!\n");
        return 0;
    }

    printf("%d test(s) failed\n", failures);
    return 1;
}
