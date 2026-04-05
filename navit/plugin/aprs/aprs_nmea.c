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

#include "aprs_nmea.h"
#include "aprs_db.h"
#include "config.h"
#include "debug.h"
#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

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
    if (*p == '$')
        p++;
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
    if (!isfinite(degrees)) {
        return 0.0;
    }
    /* Avoid undefined behavior on out-of-range float->int conversion. */
    double deg_d = degrees / 100.0;
    if (deg_d > (double)INT_MAX || deg_d < (double)INT_MIN) {
        return 0.0;
    }
    int deg = (int)deg_d;
    double min = degrees - (deg * 100.0);
    return deg + min / 60.0;
}

/* Parse optional field helper */
static void parse_optional_string_field(char **fields, int field_idx, int field_count, char **dest) {
    if (field_count > field_idx && fields[field_idx][0]) {
        *dest = g_strdup(fields[field_idx]);
    }
}

/* Parse optional numeric field helper */
static void parse_optional_double_field(char **fields, int field_idx, int field_count, double *dest) {
    if (field_count > field_idx && fields[field_idx][0]) {
        *dest = atof(fields[field_idx]);
    }
}

/* Parse optional integer field helper */
static void parse_optional_int_field(char **fields, int field_idx, int field_count, int *dest) {
    if (field_count > field_idx && fields[field_idx][0]) {
        *dest = atoi(fields[field_idx]);
    }
}

/* Parse optional char field helper */
static void parse_optional_char_field(char **fields, int field_idx, int field_count, char *dest) {
    if (field_count > field_idx && fields[field_idx][0]) {
        *dest = fields[field_idx][0];
    }
}

/* Parse position with direction */
static void parse_position_with_dir(char **fields, int lat_idx, int lat_dir_idx, int lng_idx, int lng_dir_idx,
                                    int field_count, struct aprs_station *station) {
    station->position.lat = parse_dm_to_decimal(fields[lat_idx]);
    if (field_count > lat_dir_idx && fields[lat_dir_idx][0] == 'S') {
        station->position.lat = -station->position.lat;
    }
    station->position.lng = parse_dm_to_decimal(fields[lng_idx]);
    if (field_count > lng_dir_idx && fields[lng_dir_idx][0] == 'W') {
        station->position.lng = -station->position.lng;
    }
}

/* Validate NMEA checksum */
static int validate_nmea_checksum(char *sentence, int len) {
    if (sentence[len - 3] != '*') {
        return 1;
    }
    unsigned char calc_checksum = nmea_checksum(sentence);
    unsigned char recv_checksum;
    if (sscanf(sentence + len - 2, "%2hhx", &recv_checksum) != 1 || calc_checksum != recv_checksum) {
        dbg(lvl_debug, "NMEA checksum mismatch");
        return 0;
    }
    sentence[len - 3] = '\0';
    return 1;
}

/* Parse NMEA fields from sentence */
static int parse_nmea_fields(char *sentence, char **fields, int max_fields) {
    int field_count = 0;
    char *p = sentence + 1;
    while (*p && field_count < max_fields - 1) {
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
    return field_count;
}

/* Parse GPWPL sentence type */
static int parse_gpwpl(char **fields, int field_count, struct aprs_station *station) {
    if (field_count < 5) {
        return 0;
    }
    parse_position_with_dir(fields, 1, 2, 3, 4, field_count, station);
    parse_optional_string_field(fields, 5, field_count, &station->callsign);
    station->timestamp = time(NULL);
    return 1;
}

/* Parse PGRMW sentence type */
static int parse_pgrmw(char **fields, int field_count, struct aprs_station *station) {
    if (field_count < 4) {
        return 0;
    }
    parse_optional_string_field(fields, 1, field_count, &station->callsign);
    station->position.lat = parse_dm_to_decimal(fields[2]);
    station->position.lng = parse_dm_to_decimal(fields[3]);
    if (field_count > 4 && fields[4][0] == 'S') {
        station->position.lat = -station->position.lat;
    }
    if (field_count > 5 && fields[5][0] == 'W') {
        station->position.lng = -station->position.lng;
    }
    parse_optional_double_field(fields, 6, field_count, &station->altitude);
    parse_optional_char_field(fields, 7, field_count, &station->symbol_table);
    parse_optional_char_field(fields, 8, field_count, &station->symbol_code);
    parse_optional_string_field(fields, 9, field_count, &station->comment);
    station->timestamp = time(NULL);
    return 1;
}

/* Parse PMGNWPL sentence type */
static int parse_pmgnwpl(char **fields, int field_count, struct aprs_station *station) {
    if (field_count < 6) {
        return 0;
    }
    parse_optional_string_field(fields, 1, field_count, &station->callsign);
    parse_position_with_dir(fields, 2, 3, 4, 5, field_count, station);
    parse_optional_double_field(fields, 6, field_count, &station->altitude);
    parse_optional_char_field(fields, 7, field_count, &station->symbol_table);
    parse_optional_char_field(fields, 8, field_count, &station->symbol_code);
    parse_optional_string_field(fields, 9, field_count, &station->comment);
    parse_optional_int_field(fields, 10, field_count, &station->course);
    parse_optional_int_field(fields, 11, field_count, &station->speed);
    station->timestamp = time(NULL);
    return 1;
}

/* Parse PKWDWPL sentence type */
static int parse_pkwdwpl(char **fields, int field_count, struct aprs_station *station) {
    if (field_count < 5) {
        return 0;
    }
    parse_optional_string_field(fields, 1, field_count, &station->callsign);
    parse_position_with_dir(fields, 2, 3, 4, 5, field_count, station);
    parse_optional_char_field(fields, 6, field_count, &station->symbol_table);
    parse_optional_char_field(fields, 7, field_count, &station->symbol_code);
    station->timestamp = time(NULL);
    return 1;
}

int aprs_nmea_parse_sentence(const char *sentence, struct aprs_station *station) {
    char *sentence_copy = g_strdup(sentence);
    char *fields[32];
    int len = strlen(sentence_copy);
    int result = 0;

    if (len < 4 || sentence_copy[0] != '$') {
        g_free(sentence_copy);
        return 0;
    }

    if (!validate_nmea_checksum(sentence_copy, len)) {
        g_free(sentence_copy);
        return 0;
    }

    int field_count = parse_nmea_fields(sentence_copy, fields, 32);
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

/* Process a single line from buffer */
static void process_nmea_line(const char *line, int line_len, struct aprs_nmea *nmea) {
    if (line_len <= 0 || line_len >= 512) {
        return;
    }
    char line_copy[512];
    memcpy(line_copy, line, line_len);
    line_copy[line_len] = '\0';

    struct aprs_station *station = aprs_station_new();
    if (aprs_nmea_parse_sentence(line_copy, station)) {
        if (nmea->callback) {
            nmea->callback(nmea->callback_data, station);
        }
    } else {
        aprs_station_free(station);
    }
}

/* Process complete lines from buffer */
static void process_buffer_lines(struct aprs_nmea *nmea) {
    char *p = nmea->buffer;
    char *line_start = nmea->buffer;
    int buffer_end = nmea->buffer_pos;

    while (*p && (p - nmea->buffer) < buffer_end) {
        if (*p == '\n' || *p == '\r') {
            int line_len = p - line_start;
            process_nmea_line(line_start, line_len, nmea);
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

/* Read data from NMEA device */
static int read_nmea_data(struct aprs_nmea *nmea) {
    int max_read = sizeof(nmea->buffer) - nmea->buffer_pos - 1;
    if (max_read <= 0) {
        nmea->buffer_pos = 0;
        max_read = sizeof(nmea->buffer) - 1;
    }

    int n = read(nmea->fd, nmea->buffer + nmea->buffer_pos, max_read);
    if (n <= 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            dbg(lvl_error, "NMEA read error: %s", strerror(errno));
            return 0;
        }
        return 1;
    }

    nmea->buffer_pos += n;
    if (nmea->buffer_pos >= (int)sizeof(nmea->buffer)) {
        nmea->buffer_pos = sizeof(nmea->buffer) - 1;
    }
    nmea->buffer[nmea->buffer_pos] = '\0';
    return 1;
}

static void *aprs_nmea_thread(void *data) {
    struct aprs_nmea *nmea = (struct aprs_nmea *)data;

    while (nmea->running) {
        if (!read_nmea_data(nmea)) {
            break;
        }
        process_buffer_lines(nmea);
        usleep(100000);
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
    if (!nmea)
        return;
    aprs_nmea_stop(nmea);
    if (nmea->fd >= 0) {
        close(nmea->fd);
    }
    g_free(nmea->config.device);
    g_free(nmea);
}

/* Get baud rate speed_t from config */
static speed_t get_baud_speed(int baud_rate) {
    switch (baud_rate) {
    case 4800:
        return B4800;
    case 9600:
        return B9600;
    case 19200:
        return B19200;
    case 38400:
        return B38400;
    case 57600:
        return B57600;
    case 115200:
        return B115200;
    default:
        return B4800;
    }
}

/* Configure parity settings */
static void configure_parity(struct termios *tty, char parity) {
    tty->c_cflag &= ~PARENB;
    if (parity == 'E') {
        tty->c_cflag |= PARENB | PARODD;
    } else if (parity == 'O') {
        tty->c_cflag |= PARENB;
    }
}

/* Configure data and stop bits */
static void configure_data_stop_bits(struct termios *tty, int data_bits, int stop_bits) {
    tty->c_cflag &= ~CSTOPB;
    if (stop_bits == 2) {
        tty->c_cflag |= CSTOPB;
    }

    tty->c_cflag &= ~CSIZE;
    if (data_bits == 7) {
        tty->c_cflag |= CS7;
    } else {
        tty->c_cflag |= CS8;
    }
}

/* Configure terminal attributes for NMEA device */
static int configure_termios(int fd, const struct aprs_nmea_config *config) {
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        dbg(lvl_error, "Failed to get terminal attributes: %s", strerror(errno));
        return 0;
    }

    speed_t speed = get_baud_speed(config->baud_rate);
    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);

    configure_parity(&tty, config->parity);
    configure_data_stop_bits(&tty, config->data_bits, config->stop_bits);

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
        dbg(lvl_error, "Failed to open NMEA device %s: %s", nmea->config.device, strerror(errno));
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

    dbg(lvl_info, "NMEA input started on %s at %d baud", nmea->config.device, nmea->config.baud_rate);
    return 1;
}

int aprs_nmea_stop(struct aprs_nmea *nmea) {
    if (!nmea || !nmea->running)
        return 0;

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

void aprs_nmea_set_callback(struct aprs_nmea *nmea, void (*callback)(void *data, struct aprs_station *station),
                            void *data) {
    if (nmea) {
        nmea->callback = callback;
        nmea->callback_data = data;
    }
}
