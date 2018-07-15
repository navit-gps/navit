/*
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "config.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <glib.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <serial_io.h>
#else
#include <termios.h>
#endif
#include <math.h>
#include "debug.h"
#include "callback.h"
#include "plugin.h"
#include "coord.h"
#include "item.h"
#include "event.h"
#include "vehicle.h"
#include "file.h"
#ifdef HAVE_SOCKET
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#ifdef HAVE_WINSOCK
#include <winsock2.h>
int inet_aton(const char *cp, struct in_addr *inp);

int inet_aton(const char *cp, struct in_addr *inp) {
    unsigned long addr = inet_addr(cp);
    inp->S_un.S_addr = addr;
    return addr!=-1;
}
#endif

/**
 * @defgroup vehicle-file Vehicle File
 * @ingroup vehicle-plugins
 * @brief The Vehicle to gain position data from a file, pipe, serial interface or a socket
 *
 * @{
 */

static void vehicle_file_disable_watch(struct vehicle_priv *priv);
static void vehicle_file_enable_watch(struct vehicle_priv *priv);
static int vehicle_file_parse(struct vehicle_priv *priv, char *buffer);
static int vehicle_file_open(struct vehicle_priv *priv);
static void vehicle_file_close(struct vehicle_priv *priv);


enum file_type {
    file_type_pipe = 1, file_type_device, file_type_file, file_type_socket, file_type_serial
};

static int buffer_size = 1024;

struct gps_sat {
    int prn;
    int elevation;
    int azimuth;
    int snr;
};


struct vehicle_priv {
    char *source;
    struct callback_list *cbl;
    int fd;
    struct callback *cb,*cbt,*cb_fix_timeout;
    char *buffer;
    int buffer_pos;
    char *nmea_data;
    char *nmea_data_buf;

    struct coord_geo geo;
    double speed;
    double direction;
    double height;
    double hdop;
    double vdop;
    char fixtime[20];
    int fixyear;
    int fixmonth;
    int fixday;
    int status;
    int sats_used;
    int sats_visible;
    int sats_signal;
    int time;
    int on_eof;
#ifdef _WIN32
    int no_data_count;
    struct event_timeout * timeout;
    struct callback *timeout_callback;
#endif
    enum file_type file_type;
    FILE *file;
    struct event_watch *watch;
    struct event_timeout *ev_fix_timeout;
    speed_t baudrate;
    struct attr ** attrs;
    char fixiso8601[128];
    int checksum_ignore;
    int magnetic_direction;
    int current_count;
    struct gps_sat current[24];
    int next_count;
    struct gps_sat next[24];
    struct item sat_item;
    int valid;
    char *statefile;
    int process_statefile;
};

/**
* @brief Callback of the plugin
*
* @param priv Pointer on the private data of the plugin
*
* @return Always 1
*/
#ifdef _WIN32
static int vehicle_win32_serial_track(struct vehicle_priv *priv) {
    static char buffer[2048] = {0,};
    static int current_index = 0;
    const int chunk_size = 1024;
    int rc = 0;
    int dwBytes;

    dbg(lvl_debug, "enter, *priv='%x', priv->source='%s'", priv, priv->source);

    if ( priv->no_data_count > 5 ) {
        vehicle_file_close( priv );
        priv->no_data_count = 0;
        vehicle_file_open( priv );
        vehicle_file_enable_watch(priv);
    }

    //if ( priv->fd <= 0 )
    //{
    //    vehicle_file_open( priv );
    //}

    if ( current_index >= ( sizeof( buffer ) - chunk_size ) ) {
        // discard
        current_index = 0;
        memset( buffer, 0, sizeof( buffer ) );
    }

    dwBytes = serial_io_read( priv->fd, &buffer[ current_index ], chunk_size );
    if ( dwBytes > 0 ) {
        char* return_pos = NULL;
        current_index += dwBytes;

        while ( ( return_pos = strchr( buffer, '\n' ) ) != NULL ) {
            char return_buffer[1024];
            int bytes_to_copy = return_pos - buffer + 1;
            memcpy( return_buffer, buffer, bytes_to_copy );
            return_buffer[ bytes_to_copy + 1 ] = '\0';
            return_buffer[ bytes_to_copy ] = '\0';

            // printf( "received %d : '%s' bytes to copy\n", bytes_to_copy, return_buffer );
            rc += vehicle_file_parse( priv, return_buffer );

            current_index -= bytes_to_copy;
            memmove( buffer, &buffer[ bytes_to_copy ], sizeof( buffer ) - bytes_to_copy );
        }
        if (rc) {
            priv->no_data_count = 0;
            callback_list_call_attr_0(priv->cbl, attr_position_coord_geo);
            if (rc > 1)
                dbg(lvl_error, "Can not keep with gps data delay is %d seconds", rc - 1);

        }
    } else {
        priv->no_data_count++;
    }
    dbg(lvl_info, "leave, return '1', priv->no_data_count='%d'", priv->no_data_count);
    return 1;
}
#endif

/**
* @brief Open dialogue with the GPS
*
* @param priv Pointer on the private data of the plugin
*
* @return 1 if ok, 0 if error
*/
static int vehicle_file_open(struct vehicle_priv *priv) {
    char *name;
#ifndef _WIN32
    struct termios tio;
#else
#define O_NDELAY 0
#endif

    name = priv->source + 5;
    if (!strncmp(priv->source, "file:", 5)) {
        priv->fd = open(name, O_RDONLY | O_NDELAY);
        if (priv->fd < 0)
            return 0;
        if (file_is_reg(name)) {
            priv->file_type = file_type_file;
        }
#ifndef _WIN32
        else {
            tcgetattr(priv->fd, &tio);
            cfmakeraw(&tio);
            cfsetispeed(&tio, priv->baudrate);
            cfsetospeed(&tio, priv->baudrate);
            tio.c_cc[VMIN] = 0;
            tio.c_cc[VTIME] = 200;
            tcsetattr(priv->fd, TCSANOW, &tio);
            priv->file_type = file_type_device;
        }
    } else if (!strncmp(priv->source,"pipe:", 5)) {
        priv->file = popen(name, "r");
        if (!priv->file)
            return 0;
        priv->fd = fileno(priv->file);
        priv->file_type = file_type_pipe;
#endif //!_WIN32
#if defined(HAVE_SOCKET) || defined(HAVE_WINSOCK)
    } else if (!strncmp(priv->source,"socket:", 7)) {
#ifdef _WIN32
        WSADATA wsi;
        WSAStartup(0x00020002,&wsi);
#endif
        char *p,*s=g_strdup(priv->source+7);
        struct sockaddr_in sin;
        p=strchr(s,':');
        if (!p) {
            dbg(lvl_error,"port number missing in %s",s);
            g_free(s);
            return 0;
        }
        *p++='\0';
        sin.sin_family=AF_INET;
        sin.sin_port=ntohs(atoi(p));
        if (!inet_aton(s, &sin.sin_addr)) {
            dbg(lvl_error,"failed to parse %s",s);
            g_free(s);
            return 0;
        }
        priv->fd = socket(PF_INET, SOCK_STREAM, 0);
        if (priv->fd != -1) {
            if (connect(priv->fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
                dbg(lvl_error,"failed to connect to %s:%s",s,p);
                g_free(s);
                return 0;
            }
        }
        p=strchr(p,':');
        if (p) {
            p++;
            int write_result;
            write_result = write(priv->fd, p, strlen(p));
            if (write_result == -1) {
                dbg(lvl_warning, "write failed.");
            }
        }
        priv->file_type = file_type_socket;
#endif //HAVE_SOCKET
    } else if (!strncmp(priv->source,"serial:",7)) {
#ifdef _WIN32
        char* raw_setting_str = g_strdup( priv->source );

        char* strport = strchr(raw_setting_str, ':' );
        char* strsettings = strchr(raw_setting_str, ' ' );

        if ( strport && strsettings ) {
            strport++;
            *strsettings = '\0';
            strsettings++;

            priv->fd=serial_io_init( strport, strsettings );
        }
        g_free( raw_setting_str );
        priv->file_type = file_type_serial;
        // Add the callback
        dbg(lvl_info, "Add the callback ...", priv->source);
        priv->timeout_callback=callback_new_1(callback_cast(vehicle_win32_serial_track), priv);
#else
        //TODO - add linux serial
#endif //!_WIN32
    }
    return(priv->fd != -1);
}

/**
* @brief Close dialogue with the GPS
*
* @param priv Pointer on the private data of the plugin
*/
static void vehicle_file_close(struct vehicle_priv *priv) {
    dbg(lvl_debug, "enter, priv->fd='%d'", priv->fd);
    vehicle_file_disable_watch(priv);
#ifdef _WIN32
    if(priv->file_type == file_type_serial) {
        if (priv->timeout_callback) {
            callback_destroy(priv->timeout_callback);
            priv->timeout_callback=NULL;	// dangling pointer! prevent double freeing.
        }
        serial_io_shutdown( priv->fd );
    } else
#endif
    {
        if (priv->file) {
#ifndef _MSC_VER
            pclose(priv->file);
#endif /* _MSC_VER */
        } else if (priv->fd >= 0) {
            close(priv->fd);
        }
        priv->file = NULL;
        priv->fd = -1;
    }
}

/**
* @brief Enable watch timer to get GPS data
*
* @param priv Pointer on the private data of the plugin
*
* @return Always 0
*/
static int vehicle_file_enable_watch_timer(struct vehicle_priv *priv) {
    dbg(lvl_debug, "enter");
    vehicle_file_enable_watch(priv);

    return FALSE;
}


/**
* @brief This is a callback function, called when the fix timeout
*        is done. Set the position to invalid.
*
* @param priv Pointer on the private data of the plugin
*/
static void vehicle_file_fix_timeout_cb(struct vehicle_priv *priv) {
    priv->valid = attr_position_valid_invalid;
    priv->ev_fix_timeout = NULL;
    callback_list_call_attr_0(priv->cbl, attr_position_coord_geo);
}


/**
* @brief Cancel previous fix timeout event and add a new one
*
* @param priv Pointer on the private data of the plugin
*/
static void vehicle_file_restart_fix_timeout(struct vehicle_priv *priv) {
    if (priv->ev_fix_timeout != NULL)
        event_remove_timeout(priv->ev_fix_timeout);
    priv->ev_fix_timeout = event_add_timeout(10000, 0, priv->cb_fix_timeout);
}


/**
* @brief Parse the buffer
*
* @param priv Pointer on the private data of the plugin
* @param buffer Data buffer (null terminated)
*
* @return 1 if new coords were received (fixtime changed) or changed to invalid,
*         0 if not found
*/
static int vehicle_file_parse(struct vehicle_priv *priv, char *buffer) {
    char *nmea_data_buf, *p, *item[32];
    double lat, lng;
    int i, j, bcsum;
    int len = strlen(buffer);
    unsigned char csum = 0;
    int valid=0;
    int ret = 0;

    dbg(lvl_info, "enter: buffer='%s'", buffer);
    for (;;) {
        if (len < 4) {
            dbg(lvl_error, "'%s' too short", buffer);
            return ret;
        }
        if (buffer[len - 1] == '\r' || buffer[len - 1] == '\n') {
            buffer[--len] = '\0';
            if (buffer[len - 1] == '\r')
                buffer[--len] = '\0';
        } else
            break;
    }
    if (buffer[0] != '$') {
        dbg(lvl_error, "no leading $ in '%s'", buffer);
        return ret;
    }
    if (buffer[len - 3] != '*') {
        dbg(lvl_error, "no *XX in '%s'", buffer);
        return ret;
    }
    for (i = 1; i < len - 3; i++) {
        csum ^= (unsigned char) (buffer[i]);
    }
    if (!sscanf(buffer + len - 2, "%x", &bcsum) && priv->checksum_ignore != 2) {
        dbg(lvl_error, "no checksum in '%s'", buffer);
        return ret;
    }
    if (bcsum != csum && priv->checksum_ignore == 0) {
        dbg(lvl_error, "wrong checksum in '%s was %x should be %x'", buffer,bcsum,csum);
        return ret;
    }

    if (!priv->nmea_data_buf || strlen(priv->nmea_data_buf) < 65536) {
        nmea_data_buf=g_strconcat(priv->nmea_data_buf ? priv->nmea_data_buf : "", buffer, "\n", NULL);
        g_free(priv->nmea_data_buf);
        priv->nmea_data_buf=nmea_data_buf;
    } else {
        dbg(lvl_error, "nmea buffer overflow (len %zu), discarding '%s'", priv->nmea_data_buf?strlen(priv->nmea_data_buf):-1,
            buffer);
    }
    i = 0;
    p = buffer;
    while (i < 31) {
        item[i++] = p;
        while (*p && *p != ',')
            p++;
        if (!*p)
            break;
        *p++ = '\0';
    }

    if (!strncmp(&buffer[3], "GGA", 3)) {
        /*                                                           1 1111
           0      1          2         3 4          5 6 7  8   9     0 1234
           $GPGGA,184424.505,4924.2811,N,01107.8846,E,1,05,2.5,408.6,M,,,,0000*0C
           UTC of Fix[1],Latitude[2],N/S[3],Longitude[4],E/W[5],Quality(0=inv,1=gps,2=dgps)[6],Satelites used[7],
           HDOP[8],Altitude[9],"M"[10],height of geoid[11], "M"[12], time since dgps update[13], dgps ref station [14]
         */
        if (*item[2] && *item[3] && *item[4] && *item[5] && *item[6] > 0) {
            lat = g_ascii_strtod(item[2], NULL);
            priv->geo.lat = floor(lat / 100);
            lat -= priv->geo.lat * 100;
            priv->geo.lat += lat / 60;

            if (!g_ascii_strcasecmp(item[3],"S"))
                priv->geo.lat=-priv->geo.lat;

            lng = g_ascii_strtod(item[4], NULL);
            priv->geo.lng = floor(lng / 100);
            lng -= priv->geo.lng * 100;
            priv->geo.lng += lng / 60;

            if (!g_ascii_strcasecmp(item[5],"W"))
                priv->geo.lng=-priv->geo.lng;

            if (priv->valid == attr_position_valid_invalid)
                ret = 1;
            priv->valid = attr_position_valid_valid;
            vehicle_file_restart_fix_timeout(priv);

            if (*item[1] && strncmp(priv->fixtime, item[1], sizeof(priv->fixtime))) {
                ret = 1;
                strncpy(priv->fixtime, item[1], sizeof(priv->fixtime));
            }

            dbg(lvl_info, "latitude '%2.4f' longitude %2.4f", priv->geo.lat, priv->geo.lng);

        } else {
            if (priv->valid == attr_position_valid_valid)
                ret = 1;
            priv->valid=attr_position_valid_invalid;
        }

        if (*item[6])
            sscanf(item[6], "%d", &priv->status);
        if (*item[7])
            sscanf(item[7], "%d", &priv->sats_used);
        if (*item[8])
            sscanf(item[8], "%lf", &priv->hdop);
        if (*item[9])
            sscanf(item[9], "%lf", &priv->height);

        g_free(priv->nmea_data);
        priv->nmea_data=priv->nmea_data_buf;
        priv->nmea_data_buf=NULL;
        if (priv->file_type == file_type_file) {
            if (priv->watch) {
                vehicle_file_disable_watch(priv);
                event_add_timeout(priv->time, 0, priv->cbt);
            }
        }
    }
    if (!strncmp(&buffer[3], "VTG", 3)) {
        /* 0      1      2 34 5    6 7   8
           $GPVTG,143.58,T,,M,0.26,N,0.5,K*6A
           Course Over Ground Degrees True[1],"T"[2],Course Over Ground Degrees Magnetic[3],"M"[4],
           Speed in Knots[5],"N"[6],"Speed in KM/H"[7],"K"[8]
         */
        if (item[1] && item[7])
            valid = 1;
        if (i >= 10 && (*item[9] == 'A' || *item[9] == 'D'))
            valid = 1;
        if (valid) {
            priv->direction = g_ascii_strtod( item[1], NULL );
            priv->speed = g_ascii_strtod( item[7], NULL );
            dbg(lvl_info,"direction %lf, speed %2.1lf", priv->direction, priv->speed);
        }
    }
    if (!strncmp(&buffer[3], "RMC", 3)) {
        /*                                                           1     1
           0      1      2 3        4 5         6 7     8     9      0     1
           $GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A
           Time[1],Active/Void[2],lat[3],N/S[4],long[5],W/E[6],speed in knots[7],track angle[8],date[9],
           magnetic variation[10],magnetic variation direction[11]
         */
        if (*item[2] == 'A')
            valid = 1;
        if (i >= 13 && (*item[12] == 'A' || *item[12] == 'D'))
            valid = 1;
        if (valid) {
            priv->direction = g_ascii_strtod( item[8], NULL );
            priv->speed = g_ascii_strtod( item[7], NULL );
            priv->speed *= 1.852;
            sscanf(item[9], "%02d%02d%02d",
                   &priv->fixday,
                   &priv->fixmonth,
                   &priv->fixyear);
            priv->fixyear += 2000;

            lat = g_ascii_strtod(item[3], NULL);
            priv->geo.lat = floor(lat / 100);
            lat -= priv->geo.lat * 100;
            priv->geo.lat += lat / 60;

            if (!g_ascii_strcasecmp(item[4],"S"))
                priv->geo.lat=-priv->geo.lat;

            lng = g_ascii_strtod(item[5], NULL);
            priv->geo.lng = floor(lng / 100);
            lng -= priv->geo.lng * 100;
            priv->geo.lng += lng / 60;

            if (!g_ascii_strcasecmp(item[6],"W"))
                priv->geo.lng=-priv->geo.lng;

            if (priv->valid == attr_position_valid_invalid)
                ret = 1;
            priv->valid=attr_position_valid_valid;
            vehicle_file_restart_fix_timeout(priv);

            if (*item[1] && strncmp(priv->fixtime, item[1], sizeof(priv->fixtime))) {
                ret = 1;
                strncpy(priv->fixtime, item[1], sizeof(priv->fixtime));
            }

            dbg(lvl_info, "latitude '%2.4f' longitude %2.4f", priv->geo.lat, priv->geo.lng);

        } else {
            if (priv->valid == attr_position_valid_valid)
                ret = 1;
            priv->valid=attr_position_valid_invalid;
        }
    }
    if (!strncmp(buffer, "$GPGSV", 6) && i >= 4) {
        /*
        	0 GSV	   Satellites in view
        	1 2 	   Number of sentences for full data
        	2 1 	   sentence 1 of 2
        	3 08	   Number of satellites in view

        	4 01	   Satellite PRN number
        	5 40	   Elevation, degrees
        	6 083	   Azimuth, degrees
        	7 46	   SNR - higher is better
        		   for up to 4 satellites per sentence
        	*75	   the checksum data, always begins with *
        */
        if (item[3]) {
            sscanf(item[3], "%d", &priv->sats_visible);
        }
        j=4;
        while (j+4 <= i && priv->current_count < 24) {
            struct gps_sat *sat=&priv->next[priv->next_count++];
            sat->prn=atoi(item[j]);
            sat->elevation=atoi(item[j+1]);
            sat->azimuth=atoi(item[j+2]);
            sat->snr=atoi(item[j+3]);
            j+=4;
        }
        if (!strcmp(item[1], item[2])) {
            priv->sats_signal=0;
            for (i = 0 ; i < priv->next_count ; i++) {
                priv->current[i]=priv->next[i];
                if (priv->current[i].snr)
                    priv->sats_signal++;
            }
            priv->current_count=priv->next_count;
            priv->next_count=0;
        }
    }
    if (!strncmp(&buffer[3], "ZDA", 3)) {
        /*
        	0        1        2  3  4    5  6
        	$GPZDA,hhmmss.ss,dd,mm,yyyy,xx,yy*CC
        		hhmmss    HrMinSec(UTC)
        		dd,mm,yyy Day,Month,Year
        		xx        local zone hours -13..13
        		yy        local zone minutes 0..59
        */
        if (item[1] && item[2] && item[3] && item[4]) {
            strncpy(priv->fixtime, item[1], strlen(priv->fixtime));
            priv->fixday = atoi(item[2]);
            priv->fixmonth = atoi(item[3]);
            priv->fixyear = atoi(item[4]);
        }
    }
    if (!strncmp(buffer, "$IISMD", 6)) {
        /*
        	0      1   2     3      4
        	$IISMD,dir,press,height,temp*CC"
        		dir 	  Direction (0-359)
        		press	  Pressure (hpa, i.e. 1032)
        		height    Barometric height above ground (meter)
        		temp      Temperature (Degree Celsius)
        */
        if (item[1]) {
            priv->magnetic_direction = g_ascii_strtod( item[1], NULL );
            dbg(lvl_debug,"magnetic %d", priv->magnetic_direction);
        }
    }
    return ret;
}

/**
* @brief Function to get data from GPS
*
* @param  priv Pointer on the private data of the plugin
*/
static void vehicle_file_io(struct vehicle_priv *priv) {
    int size, rc = 0;
    char *str, *tok;
    dbg(lvl_debug, "vehicle_file_io : enter");

    if (priv->process_statefile) {
        unsigned char *data;
        priv->process_statefile=0;
        if (file_get_contents(priv->statefile, &data, &size)) {
            if (size > buffer_size)
                size=buffer_size;
            memcpy(priv->buffer, data, size);
            priv->buffer_pos=0;
            g_free(data);
        } else
            return;
    } else {
        size = read(priv->fd, priv->buffer + priv->buffer_pos, buffer_size - priv->buffer_pos - 1);
    }
    if (size <= 0) {
        switch (priv->on_eof) {
        case 0:
            vehicle_file_close(priv);
            vehicle_file_open(priv);
            break;
        case 1:
            vehicle_file_disable_watch(priv);
            break;
        case 2:
            exit(0);
            break;
        }
        return;
    }
    priv->buffer_pos += size;
    priv->buffer[priv->buffer_pos] = '\0';
    dbg(lvl_debug, "size=%d pos=%d buffer='%s'", size,
        priv->buffer_pos, priv->buffer);
    str = priv->buffer;
    while ((tok = strchr(str, '\n'))) {
        *tok++ = '\0';
        dbg(lvl_debug, "line='%s'", str);
        rc +=vehicle_file_parse(priv, str);
        str = tok;
        if (priv->file_type == file_type_file && rc)
            break;
    }

    if (str != priv->buffer) {
        size = priv->buffer + priv->buffer_pos - str;
        memmove(priv->buffer, str, size + 1);
        priv->buffer_pos = size;
        dbg(lvl_debug, "now pos=%d buffer='%s'",
            priv->buffer_pos, priv->buffer);
    } else if (priv->buffer_pos == buffer_size - 1) {
        dbg(lvl_debug,
            "Overflow. Most likely wrong baud rate or no nmea protocol");
        priv->buffer_pos = 0;
    }
    if (rc)
        callback_list_call_attr_0(priv->cbl, attr_position_coord_geo);
}

/**
* @brief Enable watch
*
* @param priv Pointer on the private data of the plugin
*/
static void vehicle_file_enable_watch(struct vehicle_priv *priv) {
    dbg(lvl_debug, "enter");
#ifdef _WIN32
    // add an event : don't use glib timers and g_timeout_add
    if (priv->file_type == file_type_serial) {
        if (priv->timeout_callback != NULL)
            priv->timeout = event_add_timeout(500, 1, priv->timeout_callback);
        else
            dbg(lvl_warning, "error : watch not enabled : priv->timeout_callback is null");
    } else
#endif
    {
        if (! priv->watch)
            priv->watch = event_add_watch(priv->fd, event_watch_cond_read, priv->cb);
    }
}

/**
* @brief Disable watch
*
* @param priv Pointer on the private data of the plugin
*/
static void vehicle_file_disable_watch(struct vehicle_priv *priv) {
    dbg(lvl_debug, "vehicle_file_disable_watch : enter");
#ifdef _WIN32
    if(priv->file_type == file_type_serial) {
        if (priv->timeout) {
            event_remove_timeout(priv->timeout);
            priv->timeout=NULL;		// dangling pointer! prevent double freeing.
        }
    } else
#endif //!_WIN32
    {
        if (priv->watch)
            event_remove_watch(priv->watch);
        priv->watch = NULL;
    }
}

/**
* @brief Function called to uninitialize the plugin
*
* @param priv Pointer on the private data of the plugin
*
* @remarks private data is freed by this function (g_free)
*/
static void vehicle_file_destroy(struct vehicle_priv *priv) {
    if (priv->statefile && priv->nmea_data) {
        struct attr readwrite= {attr_readwrite};
        struct attr create= {attr_create};
        struct attr *attrs[]= {&readwrite,&create,NULL};
        struct file *f;
        readwrite.u.num=1;
        create.u.num=1;
        f=file_create(priv->statefile, attrs);
        if (f) {
            file_data_write(f, 0, strlen(priv->nmea_data), priv->nmea_data);
            file_fsync(f);
            file_destroy(f);
        }
    }
    vehicle_file_close(priv);
    callback_destroy(priv->cb);
    callback_destroy(priv->cbt);
    if (priv->statefile)
        g_free(priv->statefile);
    if (priv->source)
        g_free(priv->source);
    if (priv->buffer)
        g_free(priv->buffer);
    g_free(priv);
}

/**
* @brief Function called to get attribute
*
* @param priv Pointer on the private data of the plugin
* @param type Attribute type called
* @param attr Structure to return the attribute value
*
* @return 1 if ok, 0 for unknown or invalid attribute
*/
static int vehicle_file_position_attr_get(struct vehicle_priv *priv,
        enum attr_type type, struct attr *attr) {
    switch (type) {
    case attr_position_fix_type:
        attr->u.num = priv->status;
        break;
    case attr_position_height:
        attr->u.numd = &priv->height;
        break;
    case attr_position_speed:
        attr->u.numd = &priv->speed;
        break;
    case attr_position_direction:
        attr->u.numd = &priv->direction;
        break;
    case attr_position_magnetic_direction:
        attr->u.num = priv->magnetic_direction;
        break;
    case attr_position_hdop:
        attr->u.numd = &priv->hdop;
        break;
    case attr_position_qual:
        attr->u.num = priv->sats_visible;
        break;
    case attr_position_sats_signal:
        attr->u.num = priv->sats_signal;
        break;
    case attr_position_sats_used:
        attr->u.num = priv->sats_used;
        break;
    case attr_position_coord_geo:
        attr->u.coord_geo = &priv->geo;
        break;
    case attr_position_nmea:
        attr->u.str=priv->nmea_data;
        if (! attr->u.str)
            return 0;
        break;
    case attr_position_time_iso8601:
        if (!priv->fixyear || !priv->fixtime[0])
            return 0;
        sprintf(priv->fixiso8601, "%04d-%02d-%02dT%.2s:%.2s:%sZ",
                priv->fixyear, priv->fixmonth, priv->fixday,
                priv->fixtime, (priv->fixtime+2), (priv->fixtime+4));
        attr->u.str=priv->fixiso8601;
        break;
    case attr_position_sat_item:
        dbg(lvl_debug,"at here");
        priv->sat_item.id_lo++;
        if (priv->sat_item.id_lo > priv->current_count) {
            priv->sat_item.id_lo=0;
            return 0;
        }
        attr->u.item=&priv->sat_item;
        break;
    case attr_position_valid:
        attr->u.num=priv->valid;
        break;
    default:
        return 0;
    }
    if (type != attr_position_sat_item)
        priv->sat_item.id_lo=0;
    attr->type = type;
    return 1;
}

/**
* @brief Function called to get satellite attribute
*
* @param priv Pointer on the private data of the plugin
* @param type Attribute type called
* @param attr Structure to return the attribute value
*
* @return 1 if ok, 0 for unknown attribute
*/
static int vehicle_file_sat_attr_get(void *priv_data, enum attr_type type, struct attr *attr) {
    struct gps_sat *sat;
    struct vehicle_priv *priv=priv_data;
    if (priv->sat_item.id_lo < 1)
        return 0;
    if (priv->sat_item.id_lo > priv->current_count)
        return 0;
    sat=&priv->current[priv->sat_item.id_lo-1];
    switch (type) {
    case attr_sat_prn:
        attr->u.num=sat->prn;
        break;
    case attr_sat_elevation:
        attr->u.num=sat->elevation;
        break;
    case attr_sat_azimuth:
        attr->u.num=sat->azimuth;
        break;
    case attr_sat_snr:
        attr->u.num=sat->snr;
        break;
    default:
        return 0;
    }
    attr->type = type;
    return 1;
}

static struct item_methods vehicle_file_sat_methods = {
    NULL,
    NULL,
    NULL,
    vehicle_file_sat_attr_get,
};

static struct vehicle_methods vehicle_file_methods = {
    vehicle_file_destroy,
    vehicle_file_position_attr_get,
};

/**
* @brief Function called to initialize the plugin
*
* @param meth ?
* @param cbl ?
* @param attrs ?
*
* @return Pointer on the private data of the plugin
*
* @remarks Private data is allocated by this function (g_new0)
*/
static struct vehicle_priv *vehicle_file_new_file(struct vehicle_methods
        *meth, struct callback_list
        *cbl, struct attr **attrs) {
    struct vehicle_priv *ret;
    struct attr *source;
    struct attr *time;
    struct attr *on_eof;
    struct attr *baudrate;
    struct attr *checksum_ignore;
    struct attr *state_file;

    dbg(lvl_debug, "enter");

    source = attr_search(attrs, NULL, attr_source);
    if(source == NULL) {
        dbg(lvl_error,"Missing source attribute");
        return NULL;
    }
    ret = g_new0(struct vehicle_priv, 1);   // allocate and initialize to 0
    ret->fd = -1;
    ret->cbl = cbl;
    ret->source = g_strdup(source->u.str);
    ret->buffer = g_malloc(buffer_size);
    ret->time=1000;
    ret->baudrate=B4800;
    ret->fixtime[0] = '\0';
    ret->ev_fix_timeout = NULL;
    state_file=attr_search(attrs, NULL, attr_state_file);
    if (state_file)
        ret->statefile=g_strdup(state_file->u.str);
    time = attr_search(attrs, NULL, attr_time);
    if (time)
        ret->time=time->u.num;
    baudrate = attr_search(attrs, NULL, attr_baudrate);
    if (baudrate) {
        switch (baudrate->u.num) {
        case 4800:
            ret->baudrate=B4800;
            break;
        case 9600:
            ret->baudrate=B9600;
            break;
        case 19200:
            ret->baudrate=B19200;
            break;
#ifdef B38400
        case 38400:
            ret->baudrate=B38400;
            break;
#endif
#ifdef B57600
        case 57600:
            ret->baudrate=B57600;
            break;
#endif
#ifdef B115200
        case 115200:
            ret->baudrate=B115200;
            break;
#endif
        }
    }
    checksum_ignore = attr_search(attrs, NULL, attr_checksum_ignore);
    if (checksum_ignore)
        ret->checksum_ignore=checksum_ignore->u.num;
    ret->attrs = attrs;
    on_eof = attr_search(attrs, NULL, attr_on_eof);
    if (on_eof && !g_ascii_strcasecmp(on_eof->u.str, "stop"))
        ret->on_eof=1;
    if (on_eof && !g_ascii_strcasecmp(on_eof->u.str, "exit"))
        ret->on_eof=2;
    dbg(lvl_debug,"on_eof=%d", ret->on_eof);
    *meth = vehicle_file_methods;
    ret->cb=callback_new_1(callback_cast(vehicle_file_io), ret);
    ret->cbt=callback_new_1(callback_cast(vehicle_file_enable_watch_timer), ret);
    ret->cb_fix_timeout=callback_new_1(callback_cast(vehicle_file_fix_timeout_cb), ret);
    if (ret->statefile && file_exists(ret->statefile)) {
        ret->process_statefile=1;
        event_add_timeout(1000, 0, ret->cb);
    }
    ret->sat_item.type=type_position_sat;
    ret->sat_item.id_hi=ret->sat_item.id_lo=0;
    ret->sat_item.priv_data=ret;
    ret->sat_item.meth=&vehicle_file_sat_methods;
#ifdef _WIN32
    ret->no_data_count = 0;
#endif

    dbg(lvl_debug, "vehicle_file_new_file:open");
    if (!vehicle_file_open(ret)) {
        dbg(lvl_error, "Failed to open '%s'", ret->source);
    }

    vehicle_file_enable_watch(ret);
    // vehicle_file_destroy(ret);
    // return NULL;
    dbg(lvl_debug, "leave");
    return ret;
}

/**
* @brief Initialisation of vehicle_file plugin
*/
void plugin_init(void) {
    dbg(lvl_debug, "vehicle_file:plugin_init:enter");
    plugin_register_category_vehicle("file", vehicle_file_new_file);
    plugin_register_category_vehicle("pipe", vehicle_file_new_file);
    plugin_register_category_vehicle("socket", vehicle_file_new_file);
    plugin_register_category_vehicle("serial", vehicle_file_new_file);
}
