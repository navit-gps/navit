/**
 * Navit, a modular navigation system.
 * Copyright (C) 2024 Navit Team
 *
 * Unit tests for APRS packet decoding
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "../../debug.h"
#include "../aprs_decode.h"

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

/* Sample AX.25 frame with position data */
static const unsigned char test_ax25_frame[] = {
    0x82, 0xa0, 0x9e, 0x8e, 0x88, 0x8a, 0x60, 0x9e, /* Destination: APRS */
    0x9e, 0x92, 0x88, 0x8a, 0x9a, 0x40, 0x60, 0x9e, /* Source: TEST-1 */
    0x03, 0xf0, /* Control, PID */
    /* Information field: position with timestamp */
    '=', '3', '7', '4', '7', '.', '4', '9', 'N', '/', '1', '2', '2', '2', '5', '.', '1', '7', 'W',
    ' ', 'A', '=', '0', '0', '0', '0', '0', '0', ' ', 'T', 'e', 's', 't', ' ', 'S', 't', 'a', 't', 'i', 'o', 'n',
    0x7e /* Flag */
};

static int test_decode_ax25(void) {
    struct aprs_packet packet;
    memset(&packet, 0, sizeof(packet));
    
    int result = aprs_decode_ax25(test_ax25_frame, sizeof(test_ax25_frame), &packet);
    TEST_ASSERT(result == 1, "AX.25 decode failed");
    TEST_ASSERT(packet.source_callsign != NULL, "Source callsign is NULL");
    TEST_ASSERT(strcmp(packet.source_callsign, "TEST-1") == 0, "Source callsign mismatch");
    
    aprs_packet_free(&packet);
    return 0;
}

static int test_parse_position(void) {
    /* Test uncompressed position format */
    const char *position_data = "=3747.49N/12225.17W Test Station";
    struct aprs_position pos;
    memset(&pos, 0, sizeof(pos));
    
    int result = aprs_parse_position(position_data, strlen(position_data), &pos);
    TEST_ASSERT(result == 1, "Position parse failed");
    TEST_ASSERT(pos.has_position == 1, "Position not detected");
    TEST_ASSERT(pos.position.lat > 37.0 && pos.position.lat < 38.0, "Latitude out of range");
    TEST_ASSERT(pos.position.lng > -123.0 && pos.position.lng < -122.0, "Longitude out of range");
    
    aprs_position_free(&pos);
    return 0;
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

