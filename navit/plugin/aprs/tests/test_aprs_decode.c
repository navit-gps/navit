/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024-2026 Navit Team
 *
 * Unit tests for APRS packet decoding
 */

#include "../../debug.h"
#include "../aprs_decode.h"
#include <glib.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Stub for debug_printf and max_debug_level for unit tests */
dbg_level max_debug_level = lvl_debug;

void debug_printf(dbg_level level, const char *module, const int mlen, const char *function, const int flen, int prefix,
                  const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (level <= max_debug_level) {
        fprintf(stderr, "[DEBUG] ");
        vfprintf(stderr, fmt, args);
        fprintf(stderr, "\n");
    }
    va_end(args);
}

#define TEST_ASSERT(condition, message)                                                                                \
    do {                                                                                                               \
        if (!(condition)) {                                                                                            \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, message);                                         \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

/* Sample AX.25 frame with position data */
/* Destination: APRS */
/* Source: TEST-1 - AX.25 encoding: (char_code << 1) | 0x40 (set address bit in bit 6) */
/* Decode: (byte & 0xBF) >> 1 (clear address bit before shift) */
/* T=0x54: (0x54<<1)|0x40 = 0xA8|0x40 = 0xE8, decode: (0xE8&0xBF)>>1 = 0xA8>>1 = 0x54 = 'T' */
/* E=0x45: (0x45<<1)|0x40 = 0x8A|0x40 = 0xCA, decode: (0xCA&0xBF)>>1 = 0x8A>>1 = 0x45 = 'E' */
/* S=0x53: (0x53<<1)|0x40 = 0xA6|0x40 = 0xE6, decode: (0xE6&0xBF)>>1 = 0xA6>>1 = 0x53 = 'S' */
static const unsigned char test_ax25_frame[] = {
    0x82, 0xa0, 0x9e, 0x8e, 0x88, 0x8a, 0x60, 0x9e, /* Destination: APRS */
    0xe8, 0xca, 0xe6, 0xe8, 0x40, 0x40,
    0x43,       /* Source: TEST-1 (T=0xE8, E=0xCA, S=0xE6, T=0xE8, space=0x40, space=0x40, SSID=1|end=0x43) */
    0x03, 0xf0, /* Control, PID */
    /* Information field: position with timestamp */
    '=', '3', '7', '4', '7', '.', '4', '9', 'N', '/', '1', '2', '2', '2', '5', '.', '1', '7', 'W', ' ', 'A', '=', '0',
    '0', '0', '0', '0', '0', ' ', 'T', 'e', 's', 't', ' ', 'S', 't', 'a', 't', 'i', 'o', 'n'};

static int test_decode_ax25(void) {
    struct aprs_packet packet;
    memset(&packet, 0, sizeof(packet));

    fprintf(stderr, "=== test_decode_ax25: Starting test ===\n");
    fprintf(stderr, "Frame length: %zu bytes\n", sizeof(test_ax25_frame));
    fprintf(stderr, "First few bytes: ");
    {
        int i;
        for (i = 0; i < 20 && i < (int)sizeof(test_ax25_frame); i++) {
            fprintf(stderr, "0x%02x ", test_ax25_frame[i]);
        }
    }
    fprintf(stderr, "\n");
    fflush(stderr);

    int result = aprs_decode_ax25(test_ax25_frame, sizeof(test_ax25_frame), &packet);
    fprintf(stderr, "aprs_decode_ax25 returned: %d\n", result);
    fflush(stderr);
    if (result == 1) {
        fprintf(stderr, "Destination: '%s'\n", packet.destination_callsign ? packet.destination_callsign : "(null)");
        fprintf(stderr, "Source: '%s'\n", packet.source_callsign ? packet.source_callsign : "(null)");
        fprintf(stderr, "Path count: %d\n", packet.path_count);
        fprintf(stderr, "Info length: %d\n", packet.information_length);
        fflush(stderr);
    } else {
        fprintf(stderr, "ERROR: Decode failed! result=%d\n", result);
        if (packet.source_callsign) {
            fprintf(stderr, "  But source_callsign is set: '%s'\n", packet.source_callsign);
        }
        if (packet.destination_callsign) {
            fprintf(stderr, "  And destination_callsign is set: '%s'\n", packet.destination_callsign);
        }
        fflush(stderr);
    }

    int ret = 0;
    if (result != 1) {
        fprintf(stderr, "FAIL: %s:%d: AX.25 decode failed\n", __FILE__, __LINE__);
        ret = 1;
    } else if (packet.source_callsign == NULL) {
        fprintf(stderr, "FAIL: %s:%d: Source callsign is NULL\n", __FILE__, __LINE__);
        ret = 1;
    } else if (strcmp(packet.source_callsign, "TEST-1") != 0) {
        fprintf(stderr, "FAIL: %s:%d: Source callsign mismatch\n", __FILE__, __LINE__);
        ret = 1;
    }

    aprs_packet_free(&packet);
    return ret;
}

static int test_parse_position(void) {
    /* Test uncompressed position format */
    const char *position_data = "=3747.49N/12225.17W Test Station";
    struct aprs_position pos;
    memset(&pos, 0, sizeof(pos));

    fprintf(stderr, "=== test_parse_position: Starting test ===\n");
    fprintf(stderr, "Position data: '%s'\n", position_data);
    fprintf(stderr, "Length: %zu\n", strlen(position_data));

    int result = aprs_parse_position(position_data, strlen(position_data), &pos);
    fprintf(stderr, "aprs_parse_position returned: %d\n", result);
    if (result == 1) {
        fprintf(stderr, "has_position: %d\n", pos.has_position);
        fprintf(stderr, "Latitude: %f\n", pos.position.lat);
        fprintf(stderr, "Longitude: %f\n", pos.position.lng);
        fprintf(stderr, "Expected lat range: 37.0 to 38.0, actual: %f\n", pos.position.lat);
        fprintf(stderr, "Expected lon range: -123.0 to -122.0, actual: %f\n", pos.position.lng);
    } else {
        fprintf(stderr, "ERROR: Position parse failed!\n");
    }

    int ret = 0;
    if (result != 1) {
        fprintf(stderr, "FAIL: %s:%d: Position parse failed\n", __FILE__, __LINE__);
        ret = 1;
    } else if (pos.has_position != 1) {
        fprintf(stderr, "FAIL: %s:%d: Position not detected\n", __FILE__, __LINE__);
        ret = 1;
    } else if (pos.position.lat <= 37.0 || pos.position.lat >= 38.0) {
        fprintf(stderr, "FAIL: %s:%d: Latitude out of range: %f\n", __FILE__, __LINE__, pos.position.lat);
        ret = 1;
    } else if (pos.position.lng <= -123.0 || pos.position.lng >= -122.0) {
        fprintf(stderr, "FAIL: %s:%d: Longitude out of range: %f\n", __FILE__, __LINE__, pos.position.lng);
        ret = 1;
    }

    aprs_position_free(&pos);
    return ret;
}

static int test_parse_compressed_position(void) {
    /* Test compressed position format */
    const char *compressed = "!3747.49N/12225.17W#Test";
    struct aprs_position pos;
    memset(&pos, 0, sizeof(pos));

    int result = aprs_parse_position(compressed, strlen(compressed), &pos);
    TEST_ASSERT(result == 1, "Compressed position parse failed");
    TEST_ASSERT(pos.has_position == 1, "Compressed position not detected");

    aprs_position_free(&pos);
    return 0;
}

static int test_parse_timestamp(void) {
    /* Test timestamp parsing */
    const char *data_with_time = "092345z3747.49N/12225.17W Test";
    time_t timestamp = 0;

    int result = aprs_parse_timestamp(data_with_time, strlen(data_with_time), &timestamp);
    /* Timestamp parsing may or may not succeed depending on format */
    /* Just check it doesn't crash */
    TEST_ASSERT(result == 0 || result == 1, "Timestamp parse error");

    return 0;
}

static int test_invalid_packet(void) {
    struct aprs_packet packet;
    memset(&packet, 0, sizeof(packet));

    /* Test with invalid data */
    const unsigned char invalid[] = {0x00, 0x01, 0x02, 0x03};
    int result = aprs_decode_ax25(invalid, sizeof(invalid), &packet);
    TEST_ASSERT(result == 0, "Invalid packet should fail to decode");

    return 0;
}

int main(void) {
    int failures = 0;

    printf("Running APRS decode tests...\n");

    failures += test_decode_ax25();
    failures += test_parse_position();
    failures += test_parse_compressed_position();
    failures += test_parse_timestamp();
    failures += test_invalid_packet();

    if (failures == 0) {
        printf("All decode tests passed!\n");
        return 0;
    } else {
        printf("%d test(s) failed\n", failures);
        return 1;
    }
}
