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

#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <glib.h>
#include "debug.h"
#include "aprs_decode.h"
#include "coord.h"

#ifndef MODULE
#define MODULE "aprs_decode"
#endif

static int ax25_decode_callsign(const unsigned char *data, int offset, char *callsign, int max_len) {
    int i;
    int callsign_len = 0;
    dbg(lvl_debug, "ax25_decode_callsign: offset=%d", offset);
    for (i = 0; i < 6; i++) {
        unsigned char byte = data[offset + i];
        dbg(lvl_debug, "  Byte %d: 0x%02x", i, byte);
        if ((byte & 0x01) && i > 0) {
            dbg(lvl_debug, "  Bit 0 set at i=%d, breaking", i);
            break;
        }
        /* Handle space character (0x40 encodes space) */
        if (byte == 0x40) {
            byte = ' ';
            dbg(lvl_debug, "  Decoded as space");
        } else {
            /* Clear address bit (bit 6) before shifting to get correct character */
            byte = (byte & 0xBF) >> 1;
            dbg(lvl_debug, "  Decoded as: '%c' (0x%02x)", byte, byte);
        }
        if (byte == ' ') {
            dbg(lvl_debug, "  Found space, breaking");
            break;
        }
        if (callsign_len < max_len - 1) {
            callsign[callsign_len++] = (char)byte;
        }
    }
    callsign[callsign_len] = '\0';
    dbg(lvl_debug, "  Callsign so far: '%s' (len=%d)", callsign, callsign_len);
    
    /* Check for SSID in byte 6 (the SSID byte, always present in AX.25 address) */
    /* The address field is always 7 bytes, so offset + 6 should be valid */
    unsigned char ssid_byte = data[offset + 6];
    dbg(lvl_debug, "  SSID byte (offset+6): 0x%02x, check: 0x%02x", ssid_byte, ssid_byte & 0x0E);
    if ((ssid_byte & 0x0E)) {
        int ssid = (ssid_byte >> 1) & 0x0F;
        dbg(lvl_debug, "  SSID value: %d", ssid);
        if (ssid > 0 && callsign_len < max_len - 4) {
            snprintf(callsign + callsign_len, max_len - callsign_len, "-%d", ssid);
            dbg(lvl_debug, "  Final callsign: '%s'", callsign);
        }
    }
    
    /* AX.25 address field is always 7 bytes (6 callsign + 1 SSID/control) */
    dbg(lvl_debug, "  Returning 7 bytes consumed");
    return 7;
}

int aprs_decode_ax25(const unsigned char *data, int length, struct aprs_packet *packet) {
    dbg(lvl_debug, "aprs_decode_ax25: called with length=%d", length);
    if (!data || length < 14 || !packet) {
        dbg(lvl_debug, "aprs_decode_ax25: invalid input (data=%p, length=%d, packet=%p)", data, length, packet);
        return 0;
    }
    
    memset(packet, 0, sizeof(struct aprs_packet));
    
    int offset = 0;
    char callsign[16];
    
    /* Decode destination address (first address, 7 bytes in AX.25) */
    offset += ax25_decode_callsign(data, offset, callsign, sizeof(callsign));
    packet->destination_callsign = g_strdup(callsign);
    dbg(lvl_debug, "Decoded destination: '%s', offset=%d", callsign, offset);
    
    /* Some AX.25 frames have an 8th byte for destination */
    /* If we're at offset 7 and the next byte (offset 8) looks like start of source (bit 6 set), */
    /* skip the 8th byte (offset 7) */
    if (offset == 7 && offset + 1 < length && (data[offset + 1] & 0x40)) {
        /* Skip the 8th byte of destination address */
        offset++;
        dbg(lvl_debug, "Skipped 8th byte of destination, new offset=%d", offset);
    }
    
    /* Decode source address (second address, always 7 bytes in AX.25) */
    offset += ax25_decode_callsign(data, offset, callsign, sizeof(callsign));
    packet->source_callsign = g_strdup(callsign);
    dbg(lvl_debug, "Decoded source: '%s', offset=%d", callsign, offset);
    
    /* Check if there are repeater addresses (path) - these come before control field */
    /* Path addresses have bit 0 clear, last address has bit 0 set */
    /* The control field is at offset, so we need at least 2 more bytes (control + PID) */
    GList *path_list = NULL;
    dbg(lvl_debug, "Checking for path addresses, offset=%d, length=%d", offset, length);
    while (offset + 2 <= length) {
        /* Check if this is the control field (bit 0 set indicates end of addresses) */
        dbg(lvl_debug, "Checking byte at offset %d: 0x%02x, bit 0: %d", offset, data[offset], data[offset] & 0x01);
        if (data[offset] & 0x01) {
            dbg(lvl_debug, "Found end of addresses at offset %d", offset);
            break;
        }
        
        /* Decode path address (7 bytes) */
        if (offset + 7 > length) {
            dbg(lvl_debug, "Not enough bytes for path address at offset %d", offset);
            break;
        }
        offset += ax25_decode_callsign(data, offset, callsign, sizeof(callsign));
        path_list = g_list_append(path_list, g_strdup(callsign));
        packet->path_count++;
        dbg(lvl_debug, "Decoded path: '%s', offset=%d", callsign, offset);
    }
    
    if (path_list) {
        packet->path = (char **)g_malloc(sizeof(char *) * (packet->path_count + 1));
        GList *l = path_list;
        int i = 0;
        while (l) {
            packet->path[i++] = (char *)l->data;
            l = g_list_next(l);
        }
        packet->path[i] = NULL;
        g_list_free(path_list);
    }
    
    /* After addresses, we should have control field */
    dbg(lvl_debug, "After path loop, offset=%d, length=%d", offset, length);
    if (offset >= length) {
        dbg(lvl_debug, "ERROR: offset %d >= length %d", offset, length);
        aprs_packet_free(packet);
        return 0;
    }
    
    dbg(lvl_debug, "Checking control field at offset %d: 0x%02x (expected 0x03)", offset, data[offset]);
    if (data[offset] != 0x03) {
        dbg(lvl_debug, "ERROR: Invalid control field: 0x%02x", data[offset]);
        aprs_packet_free(packet);
        return 0;
    }
    offset++;
    
    if (offset >= length) {
        dbg(lvl_debug, "ERROR: offset %d >= length %d after control", offset, length);
        aprs_packet_free(packet);
        return 0;
    }
    
    dbg(lvl_debug, "Checking PID field at offset %d: 0x%02x (expected 0xf0)", offset, data[offset]);
    if (data[offset] != 0xF0) {
        dbg(lvl_debug, "ERROR: Invalid PID field: 0x%02x", data[offset]);
        aprs_packet_free(packet);
        return 0;
    }
    offset++;
    
    if (offset >= length) {
        dbg(lvl_debug, "ERROR: offset %d >= length %d after PID", offset, length);
        aprs_packet_free(packet);
        return 0;
    }
    
    packet->information_length = length - offset;
    packet->information_field = g_malloc(packet->information_length + 1);
    memcpy(packet->information_field, data + offset, packet->information_length);
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

static int parse_ddmm_mm(const char *str, double *degrees) {
    int dd, mm;
    int mmm = 0;
    char dir;
    int len;
    int parsed;
    
    dbg(lvl_debug, "parse_ddmm_mm: parsing '%s'", str);
    len = strlen(str);
    if (len < 5) {
        dbg(lvl_debug, "parse_ddmm_mm: length %d < 5", len);
        return 0;
    }
    
    /* Determine format based on string length and structure */
    /* Latitude format: ddmm.mmN (8 chars) - 2 digits degrees */
    /* Longitude format: dddmm.mmW (9 chars) - 3 digits degrees */
    /* Check if it's 8 characters (latitude) or 9 characters (longitude) */
    if (len == 8) {
        /* Latitude: 2-digit degrees */
        parsed = sscanf(str, "%2d%2d.%2d%c", &dd, &mm, &mmm, &dir);
        dbg(lvl_debug, "parse_ddmm_mm: sscanf(%%2d%%2d.%%2d%%c) returned %d, dd=%d, mm=%d, mmm=%d, dir='%c'", parsed, dd, mm, mmm, dir);
        if (parsed == 4) {
            /* 2-digit degrees (latitude) with decimal minutes */
            *degrees = dd + (mm + (double)mmm / 100.0) / 60.0;
            dbg(lvl_debug, "parse_ddmm_mm: 2-digit with decimal: %f", *degrees);
        } else {
            parsed = sscanf(str, "%2d%2d%c", &dd, &mm, &dir);
            dbg(lvl_debug, "parse_ddmm_mm: sscanf(%%2d%%2d%%c) returned %d", parsed);
            if (parsed == 3) {
                /* 2-digit degrees without decimal minutes */
                *degrees = dd + mm / 60.0;
                dbg(lvl_debug, "parse_ddmm_mm: 2-digit without decimal: %f", *degrees);
            } else {
                dbg(lvl_debug, "parse_ddmm_mm: latitude format failed");
                return 0;
            }
        }
    } else if (len == 9) {
        /* Longitude: 3-digit degrees */
        parsed = sscanf(str, "%3d%2d.%2d%c", &dd, &mm, &mmm, &dir);
        dbg(lvl_debug, "parse_ddmm_mm: sscanf(%%3d%%2d.%%2d%%c) returned %d, dd=%d, mm=%d, mmm=%d, dir='%c'", parsed, dd, mm, mmm, dir);
        if (parsed == 4) {
            /* 3-digit degrees (longitude) with decimal minutes */
            *degrees = dd + (mm + (double)mmm / 100.0) / 60.0;
            dbg(lvl_debug, "parse_ddmm_mm: 3-digit with decimal: %f", *degrees);
        } else {
            parsed = sscanf(str, "%3d%2d%c", &dd, &mm, &dir);
            dbg(lvl_debug, "parse_ddmm_mm: sscanf(%%3d%%2d%%c) returned %d", parsed);
            if (parsed == 3) {
                /* 3-digit degrees without decimal minutes */
                *degrees = dd + mm / 60.0;
                dbg(lvl_debug, "parse_ddmm_mm: 3-digit without decimal: %f", *degrees);
            } else {
                dbg(lvl_debug, "parse_ddmm_mm: longitude format failed");
                return 0;
            }
        }
    } else {
        /* Try both formats as fallback */
        parsed = sscanf(str, "%2d%2d.%2d%c", &dd, &mm, &mmm, &dir);
        dbg(lvl_debug, "parse_ddmm_mm: sscanf(%%2d%%2d.%%2d%%c) returned %d, dd=%d, mm=%d, mmm=%d, dir='%c'", parsed, dd, mm, mmm, dir);
        if (parsed == 4) {
            /* 2-digit degrees (latitude) with decimal minutes */
            *degrees = dd + (mm + (double)mmm / 100.0) / 60.0;
            dbg(lvl_debug, "parse_ddmm_mm: 2-digit with decimal: %f", *degrees);
        } else {
            parsed = sscanf(str, "%3d%2d.%2d%c", &dd, &mm, &mmm, &dir);
            dbg(lvl_debug, "parse_ddmm_mm: sscanf(%%3d%%2d.%%2d%%c) returned %d, dd=%d, mm=%d, mmm=%d, dir='%c'", parsed, dd, mm, mmm, dir);
            if (parsed == 4) {
                /* 3-digit degrees (longitude) with decimal minutes */
                *degrees = dd + (mm + (double)mmm / 100.0) / 60.0;
                dbg(lvl_debug, "parse_ddmm_mm: 3-digit with decimal: %f", *degrees);
            } else {
                parsed = sscanf(str, "%2d%2d%c", &dd, &mm, &dir);
                dbg(lvl_debug, "parse_ddmm_mm: sscanf(%%2d%%2d%%c) returned %d", parsed);
                if (parsed == 3) {
                    *degrees = dd + mm / 60.0;
                    dbg(lvl_debug, "parse_ddmm_mm: 2-digit without decimal: %f", *degrees);
                } else {
                    parsed = sscanf(str, "%3d%2d%c", &dd, &mm, &dir);
                    dbg(lvl_debug, "parse_ddmm_mm: sscanf(%%3d%%2d%%c) returned %d", parsed);
                    if (parsed == 3) {
                        *degrees = dd + mm / 60.0;
                        dbg(lvl_debug, "parse_ddmm_mm: 3-digit without decimal: %f", *degrees);
                    } else {
                        dbg(lvl_debug, "parse_ddmm_mm: all formats failed");
                        return 0;
                    }
                }
            }
        }
    }
    
    if (dir == 'S' || dir == 's' || dir == 'W' || dir == 'w') {
        *degrees = -(*degrees);
        dbg(lvl_debug, "parse_ddmm_mm: applied direction, final: %f", *degrees);
    }
    
    return 1;
}

int aprs_parse_position(const char *info_field, int length, struct aprs_position *pos) {
    dbg(lvl_debug, "aprs_parse_position: called with length=%d", length);
    if (!info_field || !pos) {
        dbg(lvl_debug, "aprs_parse_position: invalid input");
        return 0;
    }
    
    memset(pos, 0, sizeof(struct aprs_position));
    
    if (length < 18) {
        dbg(lvl_debug, "aprs_parse_position: length %d < 18", length);
        return 0;
    }
    
    dbg(lvl_debug, "aprs_parse_position: first char='%c'", info_field[0]);
    if (info_field[0] != '!' && info_field[0] != '=' && info_field[0] != '/' && info_field[0] != '@') {
        dbg(lvl_debug, "aprs_parse_position: invalid first character");
        return 0;
    }
    
    char lat_str[9], lon_str[10];
    char symbol_table, symbol_code;
    int offset = 1;
    
    if (info_field[0] == '@') {
        char time_str[8];
        if (sscanf(info_field + offset, "%6s", time_str) == 1) {
            offset += 7;
        }
    }
    
    if (offset + 18 > length) {
        dbg(lvl_debug, "aprs_parse_position: offset %d + 18 > length %d", offset, length);
        return 0;
    }
    
    memcpy(lat_str, info_field + offset, 8);
    lat_str[8] = '\0';
    offset += 9;  /* Skip latitude (8 chars) + '/' separator (1 char) */
    dbg(lvl_debug, "aprs_parse_position: lat_str='%s', offset=%d", lat_str, offset);
    
    memcpy(lon_str, info_field + offset, 9);
    lon_str[9] = '\0';
    offset += 9;  /* Skip longitude (9 chars) */
    dbg(lvl_debug, "aprs_parse_position: lon_str='%s', offset=%d", lon_str, offset);
    
    symbol_table = info_field[offset];
    symbol_code = info_field[offset + 1];
    offset += 2;
    
    dbg(lvl_debug, "aprs_parse_position: parsing latitude '%s'", lat_str);
    if (!parse_ddmm_mm(lat_str, &pos->position.lat)) {
        dbg(lvl_debug, "aprs_parse_position: failed to parse latitude");
        aprs_position_free(pos);
        return 0;
    }
    dbg(lvl_debug, "aprs_parse_position: latitude=%f", pos->position.lat);
    
    dbg(lvl_debug, "aprs_parse_position: parsing longitude '%s'", lon_str);
    if (!parse_ddmm_mm(lon_str, &pos->position.lng)) {
        dbg(lvl_debug, "aprs_parse_position: failed to parse longitude");
        aprs_position_free(pos);
        return 0;
    }
    dbg(lvl_debug, "aprs_parse_position: longitude=%f", pos->position.lng);
    
    pos->symbol_table = symbol_table;
    pos->symbol_code = symbol_code;
    pos->has_position = 1;
    
    if (offset < length) {
        char *comment_start = (char *)info_field + offset;
        int comment_len = length - offset;
        
        pos->comment = g_strndup(comment_start, comment_len);
        
        char *alt_ptr = strstr(comment_start, "/A=");
        if (alt_ptr) {
            double alt;
            if (sscanf(alt_ptr + 3, "%lf", &alt) == 1) {
                pos->altitude = alt * 0.3048;
                pos->has_altitude = 1;
            }
        }
        
        char *course_speed = strstr(comment_start, "/");
        if (course_speed && course_speed[1] >= '0' && course_speed[1] <= '9') {
            int course, speed;
            if (sscanf(course_speed + 1, "%03d/%03d", &course, &speed) == 2) {
                pos->course = course;
                pos->speed = (int)(speed * 1.852);
                pos->has_course_speed = 1;
            }
        }
    }
    
    return 1;
}

int aprs_parse_timestamp(const char *info_field, int length, time_t *timestamp) {
    if (!info_field || !timestamp || length < 7) return 0;
    
    if (info_field[0] != '@') return 0;
    
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
    if (!packet) return;
    
    if (packet->source_callsign) {
        g_free(packet->source_callsign);
        packet->source_callsign = NULL;
    }
    if (packet->destination_callsign) {
        g_free(packet->destination_callsign);
        packet->destination_callsign = NULL;
    }
    if (packet->path) {
        for (int i = 0; i < packet->path_count; i++) {
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
    if (!pos) return;
    g_free(pos->comment);
}

