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

static int ax25_decode_callsign(const unsigned char *data, int offset, char *callsign, int max_len) {
    int i;
    for (i = 0; i < 6; i++) {
        unsigned char byte = data[offset + i];
        if ((byte & 0x01) && i > 0) break;
        byte >>= 1;
        if (byte == ' ') break;
        if (i < max_len - 1) {
            callsign[i] = (char)byte;
        }
    }
    callsign[i] = '\0';
    
    if (i < 6 && (data[offset + i] & 0x0E)) {
        int ssid = (data[offset + i] >> 1) & 0x0F;
        if (ssid > 0 && i < max_len - 4) {
            snprintf(callsign + i, max_len - i, "-%d", ssid);
        }
    }
    
    return i + 1;
}

int aprs_decode_ax25(const unsigned char *data, int length, struct aprs_packet *packet) {
    if (!data || length < 14 || !packet) return 0;
    
    memset(packet, 0, sizeof(struct aprs_packet));
    
    int offset = 0;
    char callsign[16];
    
    offset += ax25_decode_callsign(data, offset, callsign, sizeof(callsign));
    packet->source_callsign = g_strdup(callsign);
    
    offset += ax25_decode_callsign(data, offset, callsign, sizeof(callsign));
    packet->destination_callsign = g_strdup(callsign);
    
    GList *path_list = NULL;
    while (offset < length - 2) {
        if (data[offset] & 0x01) break;
        
        offset += ax25_decode_callsign(data, offset, callsign, sizeof(callsign));
        path_list = g_list_append(path_list, g_strdup(callsign));
        packet->path_count++;
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
    
    if (offset >= length) return 0;
    
    if (data[offset] != 0x03) {
        dbg(lvl_debug, "Invalid control field: 0x%02x", data[offset]);
        return 0;
    }
    offset++;
    
    if (offset >= length) return 0;
    
    if (data[offset] != 0xF0) {
        dbg(lvl_debug, "Invalid PID field: 0x%02x", data[offset]);
        return 0;
    }
    offset++;
    
    if (offset >= length) return 0;
    
    packet->information_length = length - offset;
    packet->information_field = g_malloc(packet->information_length + 1);
    memcpy(packet->information_field, data + offset, packet->information_length);
    packet->information_field[packet->information_length] = '\0';
    
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
    double mmm = 0.0;
    char dir;
    
    if (sscanf(str, "%2d%2d.%3lf%c", &dd, &mm, &mmm, &dir) != 4) {
        if (sscanf(str, "%2d%2d%c", &dd, &mm, &dir) != 3) {
            return 0;
        }
    }
    
    *degrees = dd + (mm + mmm / 100.0) / 60.0;
    if (dir == 'S' || dir == 's' || dir == 'W' || dir == 'w') {
        *degrees = -(*degrees);
    }
    
    return 1;
}

int aprs_parse_position(const char *info_field, int length, struct aprs_position *pos) {
    if (!info_field || !pos) return 0;
    
    memset(pos, 0, sizeof(struct aprs_position));
    
    if (length < 18) return 0;
    
    if (info_field[0] != '!' && info_field[0] != '=' && info_field[0] != '/' && info_field[0] != '@') {
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
    
    if (offset + 18 > length) return 0;
    
    memcpy(lat_str, info_field + offset, 8);
    lat_str[8] = '\0';
    offset += 9;
    
    memcpy(lon_str, info_field + offset, 9);
    lon_str[9] = '\0';
    offset += 9;
    
    symbol_table = info_field[offset];
    symbol_code = info_field[offset + 1];
    offset += 2;
    
    if (!parse_ddmm_mm(lat_str, &pos->position.lat)) {
        return 0;
    }
    
    if (!parse_ddmm_mm(lon_str, &pos->position.lng)) {
        return 0;
    }
    
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
    
    g_free(packet->source_callsign);
    g_free(packet->destination_callsign);
    if (packet->path) {
        for (int i = 0; i < packet->path_count; i++) {
            g_free(packet->path[i]);
        }
        g_free(packet->path);
    }
    g_free(packet->information_field);
}

void aprs_position_free(struct aprs_position *pos) {
    if (!pos) return;
    g_free(pos->comment);
}

