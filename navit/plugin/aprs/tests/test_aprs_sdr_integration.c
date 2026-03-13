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

    for (i = 0; i < 6; i++) {
        unsigned char ch = (i < strlen(src_callsign)) ? (unsigned char)src_callsign[i] : ' ';
        fprintf(stderr, "src[%zu] = '%c' (0x%02X) encoded=0x%02X\n", i, ch, ch, addr[7 + i]);
    }

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
    int ones = 0;
    int i;
    const guint8 flag_bits[8] = {0, 1, 1, 1, 1, 1, 1, 0};

    /* Preamble: 10 x 0x7E flags (LSB-first) */
    for (i = 0; i < 10 * 8; i++) {
        guint8 bit = flag_bits[i % 8];
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
        guint8 bit = flag_bits[i];
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
        unsigned char state_before = state;
        if (b == 0) {
            state ^= 1;
        }
        if (i < 16) {
            fprintf(stderr,
                    "NRZI encode: i=%d data=%d state_before=%d state_after=%d\n",
                    i, b, state_before, state);
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
static void write_noisy_iq_sample(unsigned char *iq,
                                  int *sample_index,
                                  double rf_freq,
                                  int rf_sample_rate,
                                  double noise_std,
                                  double *phase,
                                  int log_window_start,
                                  int log_window_end) {
    int idx = *sample_index;
    double phase_inc = 2.0 * M_PI * rf_freq / (double)rf_sample_rate;
    double ci = cos(*phase);
    double cq = sin(*phase);

    double sig_i = ci;
    double sig_q = cq;

    double ni = gaussian_noise(noise_std);
    double nq = gaussian_noise(noise_std);

    double noisy_i = sig_i + ni;
    double noisy_q = sig_q + nq;

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

    unsigned char ui = (unsigned char)(si + 128);
    unsigned char uq = (unsigned char)(sq + 128);

    if (idx >= log_window_start && idx <= log_window_end) {
        fprintf(stderr, "IQ[%d]: I=%u Q=%u\n", idx, ui, uq);
    }

    iq[2 * idx] = ui;
    iq[2 * idx + 1] = uq;
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
    fprintf(stderr,
            "generate_fsk_iq: nrzi_bits->len=%d total_bits=%d mark_rf=%.0f space_rf=%.0f if_offset=%.0f rf_rate=%d\n",
            (int)nrzi_bits->len, total_bits, if_offset_hz + mark_freq, if_offset_hz + space_freq, if_offset_hz,
            rf_sample_rate);
    if (total_bits > 7) {
        unsigned char b7 = g_array_index((GArray *)nrzi_bits, unsigned char, 7);
        fprintf(stderr, "Encoder NRZI bit 7 = %d, starts at RF sample %d\n",
                b7, 7 * samples_per_bit + 2 /* +2 for dummy prepend samples */);
    }
    const int total_audio_samples = total_bits * audio_per_bit;
    int audio_remainder = total_audio_samples % 40;
    int extra_audio_samples = audio_remainder ? (40 - audio_remainder) : 0;
    int extra_rf_samples = extra_audio_samples * decimation_factor;
    const int total_samples = samples_per_bit * total_bits + extra_rf_samples;

    unsigned char *iq = g_malloc(total_samples * 2);
    double phase = 0.0;
    int sample_index = 0;
    gint i;

    /* Common noise scaling for all samples (bits and padding). */
    double signal_rms = 1.0 / sqrt(2.0);
    double snr_linear = pow(10.0, 30.0 / 10.0);
    double noise_std = signal_rms / sqrt(snr_linear);

    /* Report where the first few bits start in RF sample indices. */
    for (int b = 0; b < 4; b++) {
        fprintf(stderr, "Encoder bit[%d] starts at RF sample %d\n",
                b, b * samples_per_bit);
    }

    /* Seed noise deterministically for reproducible tests. */
    srand(42);

    for (i = 0; i < (gint)nrzi_bits->len; i++) {
        unsigned char line = g_array_index((GArray *)nrzi_bits, unsigned char, i);
        /* NRZI line state to tones: line=1 -> mark (1200 Hz), line=0 -> space (2200 Hz). */
        double audio_freq = line ? mark_freq : space_freq;
        double rf_freq = if_offset_hz + audio_freq;
        if (i >= 6 && i <= 8) {
            fprintf(stderr,
                    "Encoder bit[%d]: nrzi=%d audio_freq=%.0f rf_freq=%.0f\n",
                    (int)i, line, audio_freq, rf_freq);
        }
        for (int j = 0; j < samples_per_bit; j++) {
            write_noisy_iq_sample(iq,
                                  &sample_index,
                                  rf_freq,
                                  rf_sample_rate,
                                  noise_std,
                                  &phase,
                                  155,
                                  165);
        }
    }

    /* Pad with idle mark tone to complete any partial 40-sample audio block. */
    while (sample_index < total_samples) {
        double rf_freq_pad = if_offset_hz + mark_freq;
        write_noisy_iq_sample(iq,
                              &sample_index,
                              rf_freq_pad,
                              rf_sample_rate,
                              noise_std,
                              &phase,
                              /* logging disabled for padding region */
                              -1,
                              -1);
    }

    /* Prepend 2 dummy RF samples (128,128) to compensate for 2-sample
     * RF-domain offset between encoder symbol boundaries and DSP
     * decimation/Goertzel windows. */
    {
        int total_bytes = total_samples * 2;
        unsigned char *iq_padded = g_malloc(total_bytes + 4);
        iq_padded[0] = 128;
        iq_padded[1] = 128;
        iq_padded[2] = 128;
        iq_padded[3] = 128;
        memcpy(iq_padded + 4, iq, total_bytes);
        g_free(iq);
        *out_len = total_bytes + 4;
        return iq_padded;
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

    if (!dsp) {
        fprintf(stderr, "DSP creation failed\n");
        rc = 1;
        goto cleanup;
    }
    if (!frame) {
        fprintf(stderr, "Frame allocation failed\n");
        rc = 1;
        goto cleanup;
    }

    /* Sanity-check that the raw AX.25 frame decodes on its own. */
    if (expect_success) {
        struct aprs_packet pkt_check;
        memset(&pkt_check, 0, sizeof(pkt_check));
        if (aprs_decode_ax25(frame, frame_len, &pkt_check) != 1) {
            fprintf(stderr, "Synthetic AX.25 frame failed to decode pre-DSP\n");
            rc = 1;
            aprs_packet_free(&pkt_check);
            goto cleanup;
        }
        fprintf(stderr,
                "Synthetic AX.25 pre-DSP decode: src=%s dest=%s info=%s\n",
                pkt_check.source_callsign ? pkt_check.source_callsign : "(null)",
                pkt_check.destination_callsign ? pkt_check.destination_callsign : "(null)",
                pkt_check.information_field ? pkt_check.information_field : "(null)");
        aprs_packet_free(&pkt_check);
    }

    bits = g_array_new(FALSE, FALSE, sizeof(unsigned char));
    append_flag_and_stuff_bits(frame, frame_len, bits);
    fprintf(stderr, "Total encoder bits (with preamble and flags) = %d\n", (int)bits->len);
    /* Verify flag_bits pattern is correct (0 1 1 1 1 1 1 0). */
    {
        const guint8 flag_bits_check[8] = {0, 1, 1, 1, 1, 1, 1, 0};
        fprintf(stderr, "Flag bits array: ");
        for (int b = 0; b < 8; b++) {
            fprintf(stderr, "%d", flag_bits_check[b]);
        }
        fprintf(stderr, "\n");
    }
    /* Encoder DATA bits before NRZI (including preamble flags). */
    {
        int dump = bits->len < 32 ? bits->len : 32;
        fprintf(stderr, "Encoder DATA bits first %d: ", dump);
        for (int i = 0; i < dump; i++) {
            fprintf(stderr, "%d", g_array_index(bits, unsigned char, i));
        }
        fprintf(stderr, "\n");
        fprintf(stderr, "Encoder DATA bits 0-10 (detailed): ");
        for (int i = 0; i <= 10 && i < dump; i++) {
            fprintf(stderr, "%d", g_array_index(bits, unsigned char, i));
        }
        fprintf(stderr, "\n");
    }
    nrzi_bits = nrzi_encode_bits(bits);
    iq = generate_fsk_iq(nrzi_bits, &iq_len, if_offset_hz);

    /* Dump raw IQ bytes around the RF index where bit 7 starts. */
    fprintf(stderr, "IQ raw bytes around bit 7 boundary:\n");
    for (int n = 1118; n <= 1132; n++) {
        if (n * 2 + 1 < iq_len) {
            fprintf(stderr, "RF[%d]: I=%d Q=%d\n",
                    n, (int)iq[n * 2], (int)iq[n * 2 + 1]);
        }
    }

    /* Dump first 32 NRZI encoder bits for comparison with DSP. */
    {
        int dump = nrzi_bits->len < 32 ? nrzi_bits->len : 32;
        fprintf(stderr, "Encoder first %d NRZI: ", dump);
        for (int i = 0; i < dump; i++) {
            fprintf(stderr, "%d", g_array_index(nrzi_bits, unsigned char, i));
        }
        fprintf(stderr, "\n");
    }

    {
        int total_bits = (int)nrzi_bits->len;
        int expected_iq_len = total_bits * 160 * 2; /* 160 RF samples/bit, I+Q bytes */
        fprintf(stderr,
                "Calling aprs_sdr_dsp_process_samples with iq_len=%d (expected %d)\n",
                iq_len, expected_iq_len);
    }

    aprs_sdr_dsp_set_frame_callback(dsp, test_frame_callback, &acc);
    frames = aprs_sdr_dsp_process_samples(dsp, iq, iq_len);

    if (expect_success) {
        /* With diagnostics available in the DSP, we rely on frames/flags; no implicit flag count here. */
        if (frames < 1) {
            fprintf(stderr, "Expected at least one complete frame from DSP\n");
            rc = 1;
            goto cleanup;
        }
        if (!(acc.count >= 1 && acc.frame != NULL)) {
            fprintf(stderr, "Expected frame callback to be invoked at least once\n");
            rc = 1;
            goto cleanup;
        }
        if (aprs_decode_ax25(acc.frame, acc.length, &packet) != 1) {
            fprintf(stderr, "Frame must decode successfully\n");
            rc = 1;
            goto cleanup;
        }
        if (!packet.source_callsign || strcmp(packet.source_callsign, "KG5XXX") != 0) {
            fprintf(stderr, "Source callsign must match KG5XXX (got '%s')\n",
                    packet.source_callsign ? packet.source_callsign : "(null)");
            rc = 1;
            goto cleanup;
        }
        if (!packet.information_field
            || strcmp(packet.information_field, "!5132.00N/00007.00W-Test") != 0) {
            fprintf(stderr, "Information field must match synthetic payload (got '%s')\n",
                    packet.information_field ? packet.information_field : "(null)");
            rc = 1;
            goto cleanup;
        }
    } else {
        /* For 0 Hz (DC-centred) case we only require that the DSP and decoder handle input without crash/hang. */
        if (frames < 0) {
            fprintf(stderr, "DSP reported negative frame count\n");
            rc = 1;
            goto cleanup;
        }
    }

cleanup:
    if (acc.frame) {
        g_free(acc.frame);
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
    aprs_packet_free(&packet);
    if (dsp) {
        aprs_sdr_dsp_destroy(dsp);
    }

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

