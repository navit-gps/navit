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

#include "aprs_decode.h"
#include "config.h"
#include "coord.h"
#include "debug.h"
#include <ctype.h>
#include <glib.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef MODULE
#    define MODULE "aprs_decode"
#endif

/* Decode one character at data[offset+i]; append to callsign if valid. Return 0 to stop loop, 1 to continue. */
static int ax25_decode_one_char(const unsigned char *data, int offset, int i, char *callsign, int max_len,
                                int *callsign_len) {
    unsigned char byte = data[offset + i];
    dbg(lvl_debug, "  Byte %d: 0x%02x", i, byte);
    if ((byte & 0x01) && i > 0) {
        dbg(lvl_debug, "  Bit 0 set at i=%d, breaking", i);
        return 0;
    }
    if (byte == 0x40) {
        byte = ' ';
        dbg(lvl_debug, "  Decoded as space");
    } else {
        byte = (byte & 0xBF) >> 1;
        dbg(lvl_debug, "  Decoded as: '%c' (0x%02x)", byte, byte);
    }
    if (byte == ' ') {
        dbg(lvl_debug, "  Found space, breaking");
        return 0;
    }
    if (*callsign_len < max_len - 1)
        callsign[(*callsign_len)++] = (char)byte;
    return 1;
}

/* Append SSID to callsign if present. */
static void ax25_append_ssid(const unsigned char *data, int length, int offset, char *callsign, int max_len,
                             int callsign_len) {
    if (offset < 0 || offset + 6 >= length)
        return;
    unsigned char ssid_byte = data[offset + 6];
    dbg(lvl_debug, "  SSID byte (offset+6): 0x%02x", ssid_byte & 0x0E);
    if ((ssid_byte & 0x0E) == 0)
        return;
    {
        int ssid = (ssid_byte >> 1) & 0x0F;
        dbg(lvl_debug, "  SSID value: %d", ssid);
        if (ssid > 0 && callsign_len < max_len - 4)
            snprintf(callsign + callsign_len, max_len - callsign_len, "-%d", ssid);
    }
}

static int ax25_decode_callsign(const unsigned char *data, int length, int offset, char *callsign, int max_len) {
    int i;
    int callsign_len = 0;
    dbg(lvl_debug, "ax25_decode_callsign: offset=%d", offset);
    if (offset < 0 || offset + 7 > length)
        return -1;
    for (i = 0; i < 6; i++) {
        if (!ax25_decode_one_char(data, offset, i, callsign, max_len, &callsign_len))
            break;
    }
    callsign[callsign_len] = '\0';
    dbg(lvl_debug, "  Callsign so far: '%s' (len=%d)", callsign, callsign_len);
    ax25_append_ssid(data, length, offset, callsign, max_len, callsign_len);
    return 7;
}

/* Decode destination and source addresses; return offset after source or -1. */
static int decode_ax25_dest_src(const unsigned char *data, int length, struct aprs_packet *packet, char *callsign,
                                size_t callsign_size) {
    int offset;

    offset = ax25_decode_callsign(data, length, 0, callsign, callsign_size);
    if (offset < 0)
        return -1;
    packet->destination_callsign = g_strdup(callsign);
    dbg(lvl_debug, "Decoded destination: '%s', offset=%d", callsign, offset);
    if (offset == 7 && offset + 1 < length && (data[offset + 1] & 0x40)) {
        offset++;
        dbg(lvl_debug, "Skipped 8th byte of destination, new offset=%d", offset);
    }
    {
        int consumed = ax25_decode_callsign(data, length, offset, callsign, callsign_size);
        if (consumed < 0)
            return -1;
        offset += consumed;
    }
    packet->source_callsign = g_strdup(callsign);
    dbg(lvl_debug, "Decoded source: '%s', offset=%d", callsign, offset);
    return offset;
}

/* Decode path addresses into path_list; return offset after path or -1. */
static int decode_ax25_path(const unsigned char *data, int length, int offset, GList **path_list, int *path_count,
                            char *callsign, size_t callsign_size) {
    *path_list = NULL;
    *path_count = 0;
    while (offset + 2 <= length) {
        if (data[offset] & 0x01)
            break;
        if (offset + 7 > length)
            break;
        {
            int consumed = ax25_decode_callsign(data, length, offset, callsign, callsign_size);
            if (consumed < 0) {
                g_list_free_full(*path_list, g_free);
                *path_list = NULL;
                *path_count = 0;
                return -1;
            }
            offset += consumed;
        }
        *path_list = g_list_append(*path_list, g_strdup(callsign));
        (*path_count)++;
        dbg(lvl_debug, "Decoded path: '%s', offset=%d", callsign, offset);
    }
    return offset;
}

/* Copy path_list into packet->path; caller frees path_list. */
static void ax25_copy_path_to_packet(GList *path_list, int path_count, struct aprs_packet *packet) {
    GList *l;
    int i = 0;
    if (!path_list)
        return;
    packet->path = (char **)g_malloc(sizeof(char *) * (path_count + 1));
    for (l = path_list; l; l = g_list_next(l))
        packet->path[i++] = (char *)l->data;
    packet->path[i] = NULL;
    g_list_free(path_list);
}

/* Validate control and PID at offset; return offset+2 on success, -1 on error. */
static int decode_ax25_control_pid(const unsigned char *data, int length, int offset) {
    if (offset >= length)
        return -1;
    if (data[offset] != 0x03) {
        dbg(lvl_debug, "ERROR: Invalid control field: 0x%02x", data[offset]);
        return -1;
    }
    offset++;
    if (offset >= length)
        return -1;
    if (data[offset] != 0xF0) {
        dbg(lvl_debug, "ERROR: Invalid PID field: 0x%02x", data[offset]);
        return -1;
    }
    return offset + 1;
}

int aprs_decode_ax25(const unsigned char *data, int length, struct aprs_packet *packet) {
    int offset;
    char callsign[16];
    GList *path_list;
    int info_start;

    dbg(lvl_debug, "aprs_decode_ax25: called with length=%d", length);
    if (!data || length < 14 || !packet) {
        dbg(lvl_debug, "aprs_decode_ax25: invalid input (data=%p, length=%d, packet=%p)", data, length, packet);
        return 0;
    }
    memset(packet, 0, sizeof(struct aprs_packet));
    offset = decode_ax25_dest_src(data, length, packet, callsign, sizeof(callsign));
    if (offset < 0)
        return 0;
    dbg(lvl_info, "APRS AX.25 frame from '%s' decoded to header and addresses", packet->source_callsign);
    offset = decode_ax25_path(data, length, offset, &path_list, &packet->path_count, callsign, sizeof(callsign));
    if (offset < 0) {
        aprs_packet_free(packet);
        return 0;
    }
    ax25_copy_path_to_packet(path_list, packet->path_count, packet);
    info_start = decode_ax25_control_pid(data, length, offset);
    if (info_start < 0) {
        aprs_packet_free(packet);
        return 0;
    }
    packet->information_length = length - info_start;
    packet->information_field = g_malloc(packet->information_length + 1);
    memcpy(packet->information_field, data + info_start, packet->information_length);
    packet->information_field[packet->information_length] = '\0';
    dbg(lvl_debug, "Decode successful, information_length=%d", packet->information_length);
    return 1;
}

static int parse_ddmm_ss(const char *str, double *degrees) {
    int dd, mm;
    double ss = 0.0;
    char dir;

    if (sscanf(str, "%2d%2d.%2lf%c", &dd, &mm, &ss, &dir) != 4) {
        if (sscanf(str, "%2d%2d%c", &dd, &mm, &dir) != 3) {
            return 0;
        }
    }

    *degrees = dd + mm / 60.0 + ss / 3600.0;
    if (dir == 'S' || dir == 's' || dir == 'W' || dir == 'w') {
        *degrees = -(*degrees);
    }

    return 1;
}

/* Parse 2-digit degrees (latitude): ddmm.mm or ddmm */
static int parse_ddmm_mm_2digit(const char *str, double *degrees) {
    int dd, mm, mmm = 0;
    char dir;
    if (sscanf(str, "%2d%2d.%2d%c", &dd, &mm, &mmm, &dir) == 4) {
        *degrees = dd + (mm + (double)mmm / 100.0) / 60.0;
        return 1;
    }
    if (sscanf(str, "%2d%2d%c", &dd, &mm, &dir) == 3) {
        *degrees = dd + mm / 60.0;
        return 1;
    }
    return 0;
}

/* Parse 3-digit degrees (longitude): dddmm.mm or dddmm */
static int parse_ddmm_mm_3digit(const char *str, double *degrees) {
    int dd, mm, mmm = 0;
    char dir;
    if (sscanf(str, "%3d%2d.%2d%c", &dd, &mm, &mmm, &dir) == 4) {
        *degrees = dd + (mm + (double)mmm / 100.0) / 60.0;
        return 1;
    }
    if (sscanf(str, "%3d%2d%c", &dd, &mm, &dir) == 3) {
        *degrees = dd + mm / 60.0;
        return 1;
    }
    return 0;
}

static void parse_ddmm_mm_apply_dir(char dir, double *degrees) {
    if (dir == 'S' || dir == 's' || dir == 'W' || dir == 'w')
        *degrees = -(*degrees);
}

static int parse_ddmm_mm_by_length(const char *str, int len, double *degrees, char *dir_out) {
    if (len == 8) {
        if (!parse_ddmm_mm_2digit(str, degrees))
            return 0;
        *dir_out = str[7];
        return 1;
    }
    if (len == 9) {
        if (!parse_ddmm_mm_3digit(str, degrees))
            return 0;
        *dir_out = str[8];
        return 1;
    }
    if (parse_ddmm_mm_2digit(str, degrees) || parse_ddmm_mm_3digit(str, degrees)) {
        *dir_out = str[len - 1];
        return 1;
    }
    return 0;
}

static int parse_ddmm_mm(const char *str, double *degrees) {
    int len;
    char dir;

    dbg(lvl_debug, "parse_ddmm_mm: parsing '%s'", str);
    len = strlen(str);
    if (len < 5) {
        dbg(lvl_debug, "parse_ddmm_mm: length %d < 5", len);
        return 0;
    }
    if (!parse_ddmm_mm_by_length(str, len, degrees, &dir)) {
        dbg(lvl_debug, "parse_ddmm_mm: format failed");
        return 0;
    }
    parse_ddmm_mm_apply_dir(dir, degrees);
    dbg(lvl_debug, "parse_ddmm_mm: applied direction, final: %f", *degrees);
    return 1;
}

static void parse_position_comment(struct aprs_position *pos, const char *comment_start, int comment_len) {
    const char *alt_ptr;
    const char *course_speed;
    double alt;
    int course, speed;

    pos->comment = g_strndup(comment_start, comment_len);
    alt_ptr = strstr(comment_start, "/A=");
    if (alt_ptr && sscanf(alt_ptr + 3, "%lf", &alt) == 1) {
        pos->altitude = alt * 0.3048;
        pos->has_altitude = 1;
    }
    course_speed = strstr(comment_start, "/");
    if (course_speed && course_speed[1] >= '0' && course_speed[1] <= '9'
        && sscanf(course_speed + 1, "%03d/%03d", &course, &speed) == 2) {
        pos->course = course;
        pos->speed = (int)(speed * 1.852);
        pos->has_course_speed = 1;
    }
}

static int aprs_parse_position_valid_header(const char *info_field, int length) {
    /* Minimum: header + lat/lon + symbol table/code. */
    if (!info_field || length < 21)
        return 0;
    if (info_field[0] != '!' && info_field[0] != '=' && info_field[0] != '/' && info_field[0] != '@')
        return 0;
    return 1;
}

static int aprs_parse_position_skip_timestamp(const char *info_field, int length, int *offset) {
    char time_str[8];
    if (info_field[0] == '@') {
        if (*offset + 7 <= length && sscanf(info_field + *offset, "%6s", time_str) == 1)
            *offset += 7;
    }
    /*
     * Lat/lon parsing reads 8 bytes (lat) + 1 separator + 9 bytes (lon) and
     * then reads 2 additional bytes for symbol table/code at offset+18/19.
     */
    return (*offset + 20 <= length);
}

static int aprs_parse_position_parse_lat_lon(const char *info_field, int offset, struct aprs_position *pos) {
    char lat_str[9], lon_str[10];

    memcpy(lat_str, info_field + offset, 8);
    lat_str[8] = '\0';
    memcpy(lon_str, info_field + offset + 9, 9);
    lon_str[9] = '\0';
    if (!parse_ddmm_mm(lat_str, &pos->position.lat) || !parse_ddmm_mm(lon_str, &pos->position.lng))
        return 0;
    pos->symbol_table = info_field[offset + 18];
    pos->symbol_code = info_field[offset + 19];
    pos->has_position = 1;
    return 1;
}

int aprs_parse_position(const char *info_field, int length, struct aprs_position *pos) {
    int offset = 1;

    dbg(lvl_debug, "aprs_parse_position: called with length=%d", length);
    if (!pos || !aprs_parse_position_valid_header(info_field, length)) {
        dbg(lvl_debug, "aprs_parse_position: invalid input");
        return 0;
    }
    memset(pos, 0, sizeof(struct aprs_position));
    if (!aprs_parse_position_skip_timestamp(info_field, length, &offset)) {
        dbg(lvl_debug, "aprs_parse_position: offset + 20 > length");
        return 0;
    }
    if (!aprs_parse_position_parse_lat_lon(info_field, offset, pos)) {
        dbg(lvl_debug, "aprs_parse_position: failed to parse lat/lon");
        aprs_position_free(pos);
        return 0;
    }
    offset += 20;
    if (offset < length)
        parse_position_comment(pos, info_field + offset, length - offset);
    return 1;
}

int aprs_parse_timestamp(const char *info_field, int length, time_t *timestamp) {
    if (!info_field || !timestamp || length < 7)
        return 0;

    if (info_field[0] != '@')
        return 0;

    int day, hour, min, sec = 0;
    if (sscanf(info_field + 1, "%2d%2d%2d%2d", &day, &hour, &min, &sec) < 3) {
        return 0;
    }

    time_t now = time(NULL);
    struct tm *tm_now = gmtime(&now);

    struct tm tm_aprs;
    memcpy(&tm_aprs, tm_now, sizeof(struct tm));
    tm_aprs.tm_mday = day;
    tm_aprs.tm_hour = hour;
    tm_aprs.tm_min = min;
    tm_aprs.tm_sec = sec;
    tm_aprs.tm_isdst = -1;

#ifdef HAVE_TIMEGM
    *timestamp = timegm(&tm_aprs);
#else
    {
        time_t local = mktime(&tm_aprs);
        struct tm *tm_local = localtime(&now);
        struct tm *tm_utc = gmtime(&now);
        long offset = (tm_local->tm_hour - tm_utc->tm_hour) * 3600;
        *timestamp = local - offset;
    }
#endif

    if (*timestamp > now) {
        tm_aprs.tm_year--;
#ifdef HAVE_TIMEGM
        *timestamp = timegm(&tm_aprs);
#else
        {
            time_t local = mktime(&tm_aprs);
            struct tm *tm_local = localtime(&now);
            struct tm *tm_utc = gmtime(&now);
            long offset = (tm_local->tm_hour - tm_utc->tm_hour) * 3600;
            *timestamp = local - offset;
        }
#endif
    }

    return 1;
}

void aprs_packet_free(struct aprs_packet *packet) {
    if (!packet)
        return;

    if (packet->source_callsign) {
        g_free(packet->source_callsign);
        packet->source_callsign = NULL;
    }
    if (packet->destination_callsign) {
        g_free(packet->destination_callsign);
        packet->destination_callsign = NULL;
    }
    if (packet->path) {
        int i;
        for (i = 0; i < packet->path_count; i++) {
            if (packet->path[i]) {
                g_free(packet->path[i]);
            }
        }
        g_free(packet->path);
        packet->path = NULL;
    }
    if (packet->information_field) {
        g_free(packet->information_field);
        packet->information_field = NULL;
    }
}

void aprs_position_free(struct aprs_position *pos) {
    if (!pos)
        return;
    g_free(pos->comment);
}
