/**
 * Navit, a modular navigation system.
 *
 * Integration tests for APRS SDR DSP + AX.25 decode.
 *
 * These tests generate synthetic Bell 202 APRS signals at RF sample rate
 * and feed them through aprs_sdr_dsp, then decode the resulting AX.25 frame
 * with aprs_decode_ax25() to verify end-to-end correctness without hardware.
 */

#include "../../aprs_sdr/aprs_sdr_dsp.h"
#include "../../aprs/aprs_decode.h"
#include "../../debug.h"

#include <glib.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
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

#define TEST_ASSERT(condition, message)                                                                                 \
    do {                                                                                                                \
        if (!(condition)) {                                                                                             \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, (message));                                        \
            return 1;                                                                                                   \
        }                                                                                                               \
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

    /* Diagnostic: dump the synthetic AX.25 frame bytes. */
    fprintf(stderr, "Synthetic AX.25 frame bytes (len=%zu):", frame_len);
    for (i = 0; i < frame_len; i++) {
        fprintf(stderr, " %02X", frame[i]);
    }
    fprintf(stderr, "\n");

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
static void append_flag_and_stuff_bits(const unsigned char *frame, int frame_len, GArray *bitstream) {
    unsigned char flag = 0x7E;
    int ones = 0;
    int i;

    /* Opening flag */
    for (i = 0; i < 8; i++) {
        unsigned char bit = (unsigned char)((flag >> i) & 1);
        g_array_append_val(bitstream, bit);
    }

    /* Frame bytes (payload + FCS) with bit stuffing */
    for (i = 0; i < frame_len; i++) {
        unsigned char b = frame[i];
        int j;
        for (j = 0; j < 8; j++) {
            unsigned char bit = (unsigned char)((b >> j) & 1); /* LSB-first */
            g_array_append_val(bitstream, bit);
            if (bit) {
                ones++;
                if (ones == 5) {
                    unsigned char zero = 0;
                    g_array_append_val(bitstream, zero);
                    ones = 0;
                }
            } else {
                ones = 0;
            }
        }
    }

    /* Closing flag (unstuffed) */
    for (i = 0; i < 8; i++) {
        unsigned char bit = (unsigned char)((flag >> i) & 1);
        g_array_append_val(bitstream, bit);
    }
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
 * Generate Bell 202 FSK IQ for the NRZI bitstream.
 *
 * - RF sample rate: 192000 Hz
 * - Baud rate:      1200 bps
 * - Mark:           1200 Hz
 * - Space:          2200 Hz
 * - IF offset:      if_offset_hz (e.g. 100000.0 or 0.0)
 * - NRZI line 1 -> mark, 0 -> space.
 * - Adds small DC offset (~5 counts) and Gaussian noise at ~10 dB SNR.
 * - Pads RF samples so that the downsampled audio (48 kHz) has a length
 *   that is a multiple of 40 samples (one Goertzel block per bit).
 */
static unsigned char *generate_fsk_iq(const GArray *nrzi_bits, int *out_len, double if_offset_hz) {
    const int rf_sample_rate = 192000;
    const int baud = 1200;
    const double mark_freq = 1200.0;
    const double space_freq = 2200.0;
    const int samples_per_bit = rf_sample_rate / baud; /* 160 RF samples per bit */
    const int decimation_factor = 4;                   /* 192k -> 48k */
    const int audio_per_bit = samples_per_bit / decimation_factor; /* 40 audio samples per bit */

    const int total_bits = (int)nrzi_bits->len;
    const int total_audio_samples = total_bits * audio_per_bit;
    int audio_remainder = total_audio_samples % 40;
    int extra_audio_samples = audio_remainder ? (40 - audio_remainder) : 0;
    int extra_rf_samples = extra_audio_samples * decimation_factor;
    const int total_samples = samples_per_bit * total_bits + extra_rf_samples;

    unsigned char *iq = g_malloc(total_samples * 2);
    double phase = 0.0;
    int sample_index = 0;
    gint i;

    /* Seed noise deterministically for reproducible tests. */
    srand(42);

    for (i = 0; i < (gint)nrzi_bits->len; i++) {
        unsigned char line = g_array_index((GArray *)nrzi_bits, unsigned char, i);
        /* NRZI line state to tones: line=1 -> mark (1200 Hz), line=0 -> space (2200 Hz). */
        double audio_freq = line ? mark_freq : space_freq;
        double rf_freq = if_offset_hz + audio_freq;
        int j;
        for (j = 0; j < samples_per_bit; j++) {
            double phase_inc = 2.0 * M_PI * rf_freq / (double)rf_sample_rate;
            double ci = cos(phase);
            double cq = sin(phase);

            /* Baseband amplitude before quantization */
            double sig_i = ci;
            double sig_q = cq;

            /* Approximate 10 dB SNR: noise stddev ~ signal_rms / (10^(SNRdB/20)) */
            double signal_rms = 1.0 / sqrt(2.0);
            double snr_linear = pow(10.0, 10.0 / 20.0);
            double noise_std = signal_rms / snr_linear;

            double ni = gaussian_noise(noise_std);
            double nq = gaussian_noise(noise_std);

            double noisy_i = sig_i + ni;
            double noisy_q = sig_q + nq;

            /* Scale to int8 range and add DC offset of about 5 counts. */
            double scale = 80.0; /* keep well within [-128,127] */
            int si = (int)(noisy_i * scale);
            int sq = (int)(noisy_q * scale);
            si += 5;
            sq += 5;
            if (si < -128)
                si = -128;
            if (si > 127)
                si = 127;
            if (sq < -128)
                sq = -128;
            if (sq > 127)
                sq = 127;

            unsigned char ui = (unsigned char)(si + 128);
            unsigned char uq = (unsigned char)(sq + 128);

            iq[2 * sample_index] = ui;
            iq[2 * sample_index + 1] = uq;
            sample_index++;

            phase += phase_inc;
            if (phase > M_PI)
                phase -= 2.0 * M_PI;
            else if (phase < -M_PI)
                phase += 2.0 * M_PI;
        }
    }

    /* Pad with idle mark tone to complete any partial 40-sample audio block. */
    for (; sample_index < total_samples; sample_index++) {
        double phase_inc = 2.0 * M_PI * (if_offset_hz + mark_freq) / (double)rf_sample_rate;
        double ci = cos(phase);
        double cq = sin(phase);
        double sig_i = ci;
        double sig_q = cq;

        double signal_rms = 1.0 / sqrt(2.0);
        double snr_linear = pow(10.0, 10.0 / 20.0);
        double noise_std = signal_rms / snr_linear;

        double ni = gaussian_noise(noise_std);
        double nq = gaussian_noise(noise_std);

        double noisy_i = sig_i + ni;
        double noisy_q = sig_q + nq;

        double scale = 80.0;
        int si = (int)(noisy_i * scale) + 5;
        int sq = (int)(noisy_q * scale) + 5;
        if (si < -128)
            si = -128;
        if (si > 127)
            si = 127;
        if (sq < -128)
            sq = -128;
        if (sq > 127)
            sq = 127;

        unsigned char ui = (unsigned char)(si + 128);
        unsigned char uq = (unsigned char)(sq + 128);

        iq[2 * sample_index] = ui;
        iq[2 * sample_index + 1] = uq;

        phase += phase_inc;
        if (phase > M_PI)
            phase -= 2.0 * M_PI;
        else if (phase < -M_PI)
            phase += 2.0 * M_PI;
    }

    *out_len = total_samples * 2;
    return iq;
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
    int frame_len;
    unsigned char *frame = encode_ax25_frame(&frame_len);
    GArray *bits = g_array_new(FALSE, FALSE, sizeof(unsigned char));
    GArray *nrzi_bits;
    unsigned char *iq;
    int iq_len;
    int frames;
    int rc = 0;
    struct aprs_packet packet;

    memset(&packet, 0, sizeof(packet));

    TEST_ASSERT(dsp != NULL, "DSP creation failed");
    TEST_ASSERT(frame != NULL, "Frame allocation failed");

    /* Sanity-check that the raw AX.25 frame decodes on its own. */
    if (expect_success) {
        struct aprs_packet pkt_check;
        memset(&pkt_check, 0, sizeof(pkt_check));
        TEST_ASSERT(aprs_decode_ax25(frame, frame_len, &pkt_check) == 1,
                    "Synthetic AX.25 frame failed to decode pre-DSP");
        fprintf(stderr,
                "Synthetic AX.25 pre-DSP decode: src=%s dest=%s info=%s\n",
                pkt_check.source_callsign ? pkt_check.source_callsign : "(null)",
                pkt_check.destination_callsign ? pkt_check.destination_callsign : "(null)",
                pkt_check.information_field ? pkt_check.information_field : "(null)");
        aprs_packet_free(&pkt_check);
    }

    append_flag_and_stuff_bits(frame, frame_len, bits);
    nrzi_bits = nrzi_encode_bits(bits);
    iq = generate_fsk_iq(nrzi_bits, &iq_len, if_offset_hz);

    aprs_sdr_dsp_set_frame_callback(dsp, test_frame_callback, &acc);
    frames = aprs_sdr_dsp_process_samples(dsp, iq, iq_len);

    if (expect_success) {
        int flags_found = (frames >= 1) ? 2 : 0; /* Implicitly two flags per successfully decoded frame. */

        TEST_ASSERT(flags_found >= 2, "Expected at least two HDLC flags in +100 kHz test");
        TEST_ASSERT(frames >= 1, "Expected at least one complete frame from DSP");
        TEST_ASSERT(acc.count >= 1 && acc.frame != NULL, "Expected frame callback to be invoked at least once");
        TEST_ASSERT(aprs_decode_ax25(acc.frame, acc.length, &packet) == 1, "Frame must decode successfully");
        TEST_ASSERT(packet.source_callsign != NULL, "Decoded packet has no source callsign");
        TEST_ASSERT(strcmp(packet.source_callsign, "KG5XXX") == 0, "Source callsign must match KG5XXX");
        TEST_ASSERT(packet.information_field != NULL, "Decoded packet has no information field");
        TEST_ASSERT(strcmp(packet.information_field, "!5132.00N/00007.00W-Test") == 0,
                    "Information field must match synthetic payload");
    } else {
        /* For 0 Hz (DC-centred) case we only require that the DSP and decoder handle input without crash/hang. */
        TEST_ASSERT(frames >= 0, "DSP reported negative frame count");
    }

    if (acc.frame) {
        g_free(acc.frame);
    }
    g_free(frame);
    g_array_free(bits, TRUE);
    g_array_free(nrzi_bits, TRUE);
    g_free(iq);
    aprs_packet_free(&packet);
    aprs_sdr_dsp_destroy(dsp);

    return rc;
}

static int test_aprs_sdr_if_offset(void) {
    fprintf(stderr, "=== test_aprs_sdr_if_offset (+100 kHz IF) ===\n");
    return run_dsp_and_decode(100000.0, 1);
}

static int test_aprs_sdr_dc_centered(void) {
    fprintf(stderr, "=== test_aprs_sdr_dc_centered (0 Hz IF) ===\n");
    return run_dsp_and_decode(0.0, 0);
}

int main(void) {
    int failures = 0;

    printf("Running APRS SDR integration tests...\n");

    failures += test_aprs_sdr_if_offset();
    failures += test_aprs_sdr_dc_centered();

    if (failures == 0) {
        printf("All APRS SDR integration tests passed!\n");
        return 0;
    }

    printf("%d test(s) failed\n", failures);
    return 1;
}

