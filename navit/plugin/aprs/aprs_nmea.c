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

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <glib.h>
#include <math.h>
#include <time.h>
#include "debug.h"
#include "aprs_nmea.h"
#include "aprs_db.h"

#include <pthread.h>

struct aprs_nmea {
    struct aprs_nmea_config config;
    int fd;
    int running;
    char buffer[4096];
    int buffer_pos;
    pthread_t thread;
    void (*callback)(void *data, struct aprs_station *station);
    void *callback_data;
};

static unsigned char nmea_checksum(const char *sentence) {
    unsigned char checksum = 0;
    const char *p = sentence;
    if (*p == '$') p++;
    while (*p && *p != '*') {
        checksum ^= (unsigned char)*p++;
    }
    return checksum;
}

static double parse_dm_to_decimal(const char *dm) {
    double degrees;
    if (sscanf(dm, "%lf", &degrees) != 1) {
        return 0.0;
    }
    int deg = (int)(degrees / 100.0);
    double min = degrees - (deg * 100.0);
    return deg + min / 60.0;
}

/* Parse GPWPL sentence type */
static int parse_gpwpl(char **fields, int field_count, struct aprs_station *station) {
    if (field_count < 5) {
        return 0;
    }
    station->position.lat = parse_dm_to_decimal(fields[1]);
    if (fields[2][0] == 'S') {
        station->position.lat = -station->position.lat;
    }
    station->position.lng = parse_dm_to_decimal(fields[3]);
    if (fields[4][0] == 'W') {
        station->position.lng = -station->position.lng;
    }
    if (field_count > 5 && fields[5][0]) {
        station->callsign = g_strdup(fields[5]);
    }
    station->timestamp = time(NULL);
    return 1;
}

/* Parse PGRMW sentence type */
static int parse_pgrmw(char **fields, int field_count, struct aprs_station *station) {
    if (field_count < 4) {
        return 0;
    }
    if (fields[1][0]) {
        station->callsign = g_strdup(fields[1]);
    }
    station->position.lat = parse_dm_to_decimal(fields[2]);
    station->position.lng = parse_dm_to_decimal(fields[3]);
    if (field_count > 4 && fields[4][0]) {
        char lat_dir = fields[4][0];
        if (lat_dir == 'S') {
            station->position.lat = -station->position.lat;
        }
    }
    if (field_count > 5 && fields[5][0]) {
        char lng_dir = fields[5][0];
        if (lng_dir == 'W') {
            station->position.lng = -station->position.lng;
        }
    }
    if (field_count > 6 && fields[6][0]) {
        station->altitude = atof(fields[6]);
    }
    if (field_count > 7 && fields[7][0]) {
        station->symbol_table = fields[7][0];
    }
    if (field_count > 8 && fields[8][0]) {
        station->symbol_code = fields[8][0];
    }
    if (field_count > 9 && fields[9][0]) {
        station->comment = g_strdup(fields[9]);
    }
    station->timestamp = time(NULL);
    return 1;
}

/* Parse PMGNWPL sentence type */
static int parse_pmgnwpl(char **fields, int field_count, struct aprs_station *station) {
    if (field_count < 6) {
        return 0;
    }
    if (fields[1][0]) {
        station->callsign = g_strdup(fields[1]);
    }
    station->position.lat = parse_dm_to_decimal(fields[2]);
    if (fields[3][0] == 'S') {
        station->position.lat = -station->position.lat;
    }
    station->position.lng = parse_dm_to_decimal(fields[4]);
    if (fields[5][0] == 'W') {
        station->position.lng = -station->position.lng;
    }
    if (field_count > 6 && fields[6][0]) {
        station->altitude = atof(fields[6]);
    }
    if (field_count > 7 && fields[7][0]) {
        station->symbol_table = fields[7][0];
    }
    if (field_count > 8 && fields[8][0]) {
        station->symbol_code = fields[8][0];
    }
    if (field_count > 9 && fields[9][0]) {
        station->comment = g_strdup(fields[9]);
    }
    if (field_count > 10 && fields[10][0]) {
        station->course = atoi(fields[10]);
    }
    if (field_count > 11 && fields[11][0]) {
        station->speed = atoi(fields[11]);
    }
    station->timestamp = time(NULL);
    return 1;
}

/* Parse PKWDWPL sentence type */
static int parse_pkwdwpl(char **fields, int field_count, struct aprs_station *station) {
    if (field_count < 5) {
        return 0;
    }
    if (fields[1][0]) {
        station->callsign = g_strdup(fields[1]);
    }
    station->position.lat = parse_dm_to_decimal(fields[2]);
    if (fields[3][0] == 'S') {
        station->position.lat = -station->position.lat;
    }
    station->position.lng = parse_dm_to_decimal(fields[4]);
    if (fields[5][0] == 'W') {
        station->position.lng = -station->position.lng;
    }
    if (field_count > 6 && fields[6][0]) {
        station->symbol_table = fields[6][0];
    }
    if (field_count > 7 && fields[7][0]) {
        station->symbol_code = fields[7][0];
    }
    station->timestamp = time(NULL);
    return 1;
}

int aprs_nmea_parse_sentence(const char *sentence, struct aprs_station *station) {
    char *sentence_copy = g_strdup(sentence);
    char *fields[32];
    int field_count = 0;
    int len = strlen(sentence_copy);
    int result = 0;
    
    if (len < 4 || sentence_copy[0] != '$') {
        g_free(sentence_copy);
        return 0;
    }
    
    if (sentence_copy[len - 3] == '*') {
        unsigned char calc_checksum = nmea_checksum(sentence_copy);
        unsigned char recv_checksum;
        if (sscanf(sentence_copy + len - 2, "%2hhx", &recv_checksum) != 1 ||
            calc_checksum != recv_checksum) {
            dbg(lvl_debug, "NMEA checksum mismatch");
            g_free(sentence_copy);
            return 0;
        }
        sentence_copy[len - 3] = '\0';
    }
    
    char *p = sentence_copy + 1;
    while (*p && field_count < 31) {
        fields[field_count++] = p;
        while (*p && *p != ',') {
            p++;
        }
        if (*p == ',') {
            *p++ = '\0';
        } else {
            break;
        }
    }
    
    if (field_count < 3) {
        g_free(sentence_copy);
        return 0;
    }
    
    const char *sentence_type = fields[0];
    
    if (strncmp(sentence_type, "GPWPL", 5) == 0) {
        result = parse_gpwpl(fields, field_count, station);
    } else if (strncmp(sentence_type, "PGRMW", 5) == 0) {
        result = parse_pgrmw(fields, field_count, station);
    } else if (strncmp(sentence_type, "PMGNWPL", 7) == 0) {
        result = parse_pmgnwpl(fields, field_count, station);
    } else if (strncmp(sentence_type, "PKWDWPL", 7) == 0) {
        result = parse_pkwdwpl(fields, field_count, station);
    }
    
    g_free(sentence_copy);
    return result;
}

static void *aprs_nmea_thread(void *data) {
    struct aprs_nmea *nmea = (struct aprs_nmea *)data;
    char line[512];
    
    while (nmea->running) {
        int max_read = sizeof(nmea->buffer) - nmea->buffer_pos - 1;
        if (max_read <= 0) {
            nmea->buffer_pos = 0;
            max_read = sizeof(nmea->buffer) - 1;
        }
        
        int n = read(nmea->fd, nmea->buffer + nmea->buffer_pos, max_read);
        if (n <= 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                dbg(lvl_error, "NMEA read error: %s", strerror(errno));
                break;
            }
            usleep(100000);
            continue;
        }
        
        nmea->buffer_pos += n;
        if (nmea->buffer_pos >= (int)sizeof(nmea->buffer)) {
            nmea->buffer_pos = sizeof(nmea->buffer) - 1;
        }
        nmea->buffer[nmea->buffer_pos] = '\0';
        
        char *p = nmea->buffer;
        char *line_start = nmea->buffer;
        while (*p && (p - nmea->buffer) < nmea->buffer_pos) {
            if (*p == '\n' || *p == '\r') {
                int line_len = p - line_start;
                if (line_len > 0 && line_len < (int)sizeof(line)) {
                    memcpy(line, line_start, line_len);
                    line[line_len] = '\0';
                    struct aprs_station *station = aprs_station_new();
                    if (aprs_nmea_parse_sentence(line, station)) {
                        if (nmea->callback) {
                            nmea->callback(nmea->callback_data, station);
                        }
                    } else {
                        aprs_station_free(station);
                    }
                }
                p++;
                while (*p == '\n' || *p == '\r') {
                    p++;
                }
                line_start = p;
            } else {
                p++;
            }
        }
        
        int remaining = p - line_start;
        if (remaining > 0 && remaining < (int)sizeof(nmea->buffer)) {
            memmove(nmea->buffer, line_start, remaining);
            nmea->buffer[remaining] = '\0';
            nmea->buffer_pos = remaining;
        } else {
            nmea->buffer_pos = 0;
        }
    }
    
    return NULL;
}

struct aprs_nmea *aprs_nmea_new(const struct aprs_nmea_config *config) {
    struct aprs_nmea *nmea = g_new0(struct aprs_nmea, 1);
    nmea->config.device = g_strdup(config->device);
    nmea->config.baud_rate = config->baud_rate;
    nmea->config.parity = config->parity;
    nmea->config.data_bits = config->data_bits ? config->data_bits : 8;
    nmea->config.stop_bits = config->stop_bits ? config->stop_bits : 1;
    nmea->fd = -1;
    nmea->running = 0;
    return nmea;
}

void aprs_nmea_destroy(struct aprs_nmea *nmea) {
    if (!nmea) return;
    aprs_nmea_stop(nmea);
    if (nmea->fd >= 0) {
        close(nmea->fd);
    }
    g_free(nmea->config.device);
    g_free(nmea);
}

/* Configure terminal attributes for NMEA device */
static int configure_termios(int fd, const struct aprs_nmea_config *config) {
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        dbg(lvl_error, "Failed to get terminal attributes: %s", strerror(errno));
        return 0;
    }
    
    speed_t speed;
    switch (config->baud_rate) {
    case 4800: speed = B4800; break;
    case 9600: speed = B9600; break;
    case 19200: speed = B19200; break;
    case 38400: speed = B38400; break;
    case 57600: speed = B57600; break;
    case 115200: speed = B115200; break;
    default: speed = B4800; break;
    }
    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);
    
    tty.c_cflag &= ~PARENB;
    if (config->parity == 'E') {
        tty.c_cflag |= PARENB | PARODD;
    } else if (config->parity == 'O') {
        tty.c_cflag |= PARENB;
    }
    
    tty.c_cflag &= ~CSTOPB;
    if (config->stop_bits == 2) {
        tty.c_cflag |= CSTOPB;
    }
    
    tty.c_cflag &= ~CSIZE;
    if (config->data_bits == 7) {
        tty.c_cflag |= CS7;
    } else {
        tty.c_cflag |= CS8;
    }
    
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CRTSCTS;
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_oflag &= ~OPOST;
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 10;
    
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        dbg(lvl_error, "Failed to set terminal attributes: %s", strerror(errno));
        return 0;
    }
    
    return 1;
}

int aprs_nmea_start(struct aprs_nmea *nmea) {
    if (!nmea || nmea->running) {
        return 0;
    }
    
    nmea->fd = open(nmea->config.device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (nmea->fd < 0) {
        dbg(lvl_error, "Failed to open NMEA device %s: %s", 
            nmea->config.device, strerror(errno));
        return 0;
    }
    
    if (!configure_termios(nmea->fd, &nmea->config)) {
        close(nmea->fd);
        nmea->fd = -1;
        return 0;
    }
    
    nmea->running = 1;
    nmea->buffer_pos = 0;
    memset(&nmea->thread, 0, sizeof(nmea->thread));
    
    if (pthread_create(&nmea->thread, NULL, aprs_nmea_thread, nmea) != 0) {
        dbg(lvl_error, "Failed to create NMEA thread");
        nmea->running = 0;
        close(nmea->fd);
        nmea->fd = -1;
        return 0;
    }
    
    dbg(lvl_info, "NMEA input started on %s at %d baud", 
        nmea->config.device, nmea->config.baud_rate);
    return 1;
}

int aprs_nmea_stop(struct aprs_nmea *nmea) {
    if (!nmea || !nmea->running) return 0;
    
    nmea->running = 0;
    
    pthread_join(nmea->thread, NULL);
    
    if (nmea->fd >= 0) {
        close(nmea->fd);
        nmea->fd = -1;
    }
    
    dbg(lvl_info, "NMEA input stopped");
    return 1;
}

int aprs_nmea_is_running(const struct aprs_nmea *nmea) {
    return nmea && nmea->running;
}

void aprs_nmea_set_callback(struct aprs_nmea *nmea, 
                            void (*callback)(void *data, struct aprs_station *station),
                            void *data) {
    if (nmea) {
        nmea->callback = callback;
        nmea->callback_data = data;
    }
}

