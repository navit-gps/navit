/* vim: set tabstop=4 expandtab: */
/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2014 Navit Team
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

/*
   This plugin implements a small subset of the SAE j1850 protocal used in some cars.
   So far the code assumes that it is run on Linux. It allows Navit to read the steering
   wheel inputs and some metrics like RPM or the fuel tank level
   */

#include <math.h>
#include <stdio.h>
#include <glib.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "config.h"
#include <navit/item.h>
#include <navit/xmlconfig.h>
#include <navit/main.h>
#include <navit/debug.h>
#include <navit/map.h>
#include <navit/navit.h>
#include <navit/callback.h>
#include <navit/file.h>
#include <navit/plugin.h>
#include <navit/event.h>
#include <navit/command.h>
#include <navit/config_.h>
#include "graphics.h"
#include "color.h"
#include "osd.h"

struct j1850 {
    struct navit *nav;
    int status;
    int device;
    int index;
    char message[255];
    char * filename;
    struct event_idle *idle;
    struct callback *callback;
    struct osd_item osd_item;
    int width;
    struct graphics_gc *orange,*white;
    struct callback *click_cb;
    int init_string_index;

    int engine_rpm;
    int trans_rpm;
    int map;
    int tank_level;
    int odo;
};

/**
 * @brief	Generates a fake sentence. Used for debugging
 * @param[in]	dest	- the char * to which we will write the sentence
 *              lenght  - the length of the sentence to generate
 *
 * @return	nothing
 *
 * Generates a fake string to simulate data from the serial port
 *
 */
void rand_str(char *dest, size_t length) {
    char charset[] = "0123456789"
                     "ABCDEF";

    while (length-- > 0) {
        size_t index = (double) rand() / RAND_MAX * (sizeof charset - 1);
        *dest++ = charset[index];
    }
    *dest = '\0';
}

/**
 * @brief	Writes 'cmd' to the serial port'
 * @param[in]	cmd	- the char * that we will write to the serial port
 *              device  - the serial port device
 *
 * @return	nothing
 *
 * Write the cmd to the serial port
 *
 */
void write_to_serial_port(unsigned char *cmd, int device) {
    int n_written = 0;
    do {
        n_written += write( device, &cmd[n_written], 1 );
    } while (cmd[n_written-1] != '\r' && n_written > 0);
    dbg(lvl_info,"sent %s to the serial port",cmd);
}

/**
 * @brief	Function called when navit is idle. Does the continous reading
 * @param[in]	j1850	- the j1850 struct containing the state of the plugin
 *
 * @return	nothing
 *
 * This is the main function of this plugin. It is called when navit is idle,
 * and performs the initialization of the obd device if needed, then reads
 * one char each time it is called, and puts this char in a buffer. When it
 * reads an EOL character, the buffer is parsed, and the appropriate action is
 * taken. The buffer is then cleared and we start over.
 *
 */
static void j1850_idle(struct j1850 *j1850) {
    int n;             // used to keep track of the numbers of char read from the device
    int value;         // used to convert the ascii char to an int
    char buf = '\0';   // the buffer where we store the char read from the device
    char header[3];    // a buffer to store the j1850 header for easier matching
    struct timeval tv; // used to timestamp the logs
    struct attr navit;

    const char *init_string[] = {
        "ATZ\r\n",
        "ATI\r\n",
        "ATL1\r\n",
        "ATH1\r\n",
        "ATS1\r\n",
        "ATAL\r\n",
        "ATMA\r\n",
        NULL
    };

    // Make sure we sent all init commands before trying to read
    if ( init_string[j1850->init_string_index]) {
        dbg(lvl_info,"Sending next init command : %s",init_string[j1850->init_string_index]);
        if (j1850->device > 0 ) {
            write_to_serial_port(init_string[j1850->init_string_index++],j1850->device);
        }

        // Did we reach the last init command?
        if ( !init_string[j1850->init_string_index]) {
            // if yes, switch to idle read instead of timed read
            event_remove_timeout(j1850->idle);
            j1850->idle=event_add_idle(125, j1850->callback);
        }
        return;
    }
    navit.type=attr_navit;
    navit.u.navit=j1850->nav;

    // If not connected, generate random messages for debugging purpose
    if (j1850->device < 0 ) {
        rand_str(j1850->message,8);
        return;
    }

    n = read( j1850->device, &buf, 1 );
    if(n == -1) {
        dbg(lvl_debug,"x");
    } else if (n==0) {
        dbg(lvl_debug,".");
    } else {
        if( buf == 13 ) {
            gettimeofday(&tv, NULL);
            unsigned long long millisecondsSinceEpoch =
                (unsigned long long)(tv.tv_sec) * 1000 +
                (unsigned long long)(tv.tv_usec) / 1000;

            j1850->message[j1850->index]='\0';
            FILE *fp;
            fp = fopen(j1850->filename,"a");
            fprintf(fp, "%llu,%s\n", millisecondsSinceEpoch, j1850->message);
            fclose(fp);
            strncpy(header, j1850->message, 2);
            header[2]='\0';
            if( strncmp(header,"10",2)==0 ) {
                char * w1 = strndup(j1850->message+2, 2);
                char * w2 = strndup(j1850->message+4, 2);
                char * w3 = strndup(j1850->message+6, 2);
                j1850->engine_rpm = ((int)strtol(w1, NULL, 16) ) / 4 ;
                j1850->trans_rpm  = ((int)strtol(w2, NULL, 16) ) / 4 ;
                j1850->map        =  (int)strtol(w3, NULL, 16);
            } else if( strncmp(header,"3D",2)==0 ) {
                if (strcmp(j1850->message, "3D110000EE") == 0) {
                    // noise
                } else if (strcmp(j1850->message, "3D1120009B") == 0) {
                    dbg(lvl_error,"L1");
                    command_evaluate(&navit, "gui.spotify_volume_up()" );
                } else if (strcmp(j1850->message, "3D110080C8") == 0) {
                    dbg(lvl_error,"L2");
                    command_evaluate(&navit, "gui.spotify_volume_toggle()" );
                } else if (strcmp(j1850->message, "3D1110005A") == 0) {
                    dbg(lvl_error,"L3");
                    command_evaluate(&navit, "gui.spotify_volume_down()" );
                } else if (strcmp(j1850->message, "3D110400C3") == 0) {
                    dbg(lvl_error,"R1");
                    command_evaluate(&navit, "gui.spotify_next_track()" );
                } else if (strcmp(j1850->message, "3D110002D4") == 0) {
                    dbg(lvl_error,"R2");
                    command_evaluate(&navit, "gui.spotify_toggle()" );
                } else if (strcmp(j1850->message, "3D11020076") == 0) {
                    dbg(lvl_error,"R3");
                    command_evaluate(&navit, "gui.spotify_previous_track()" );
                } else {
                    dbg(lvl_error,"Got button from %s", j1850->message);
                }
            } else if( strncmp(header,"72",2)==0 ) {
                char * data=strndup(j1850->message+2, 8);
                j1850->odo=((int)strtol(data, NULL, 16) )/8000;
            } else if( strncmp(header,"90",2)==0 ) {
            } else if( strncmp(header,"A4",2)==0 ) {
                char * w1 =strndup(j1850->message+2, 4);
                j1850->tank_level = ((int)strtol(w1, NULL, 16) ) / 4 ;
            } else {
                // printf(" ascii: %i [%s] with header [%s]\n",buf, response, header);
            }
            // Message has been processed. Let's clear it
            j1850->message[0]='\0';
            j1850->index=0;
        } else {
            value=buf-48;
            if(value==-16 || buf == 10 ) {
                //space and newline, discard
                return;
            } else if (value>16) {
                // chars, need to shift down
                value-=7;
                j1850->message[j1850->index]=buf;
                j1850->index++;
            } else if (buf == '<' ) {
                // We have a data error. Let's truncate the message
                j1850->message[j1850->index]='\0';
                j1850->index++;
            } else {
                j1850->message[j1850->index]=buf;
                j1850->index++;
            }
            // printf("{%c:%i}", buf,value);
        }
    }
}

/**
 * @brief	Draws the j1850 OSD
 * @param[in]	j1850	- the j1850 struct containing the state of the plugin
 *              nav     - the navit object
 * 		v	- the vehicle object
 *
 * @return	nothing
 *
 * Draws the j1850 OSD. Currently it only displays the last parsed message
 *
 */
static void osd_j1850_draw(struct j1850 *this, struct navit *nav,
                           struct vehicle *v) {
    osd_std_draw(&this->osd_item);

    struct point p, bbox[4];

    graphics_get_text_bbox(this->osd_item.gr, this->osd_item.font, this->message, 0x10000, 0, bbox, 0);
    p.x=(this->osd_item.w-bbox[2].x)/2;
    p.y = this->osd_item.h-this->osd_item.h/10;

    struct graphics_gc *curr_color = this->white;
    // online? use  this->bActive?this->white:this->orange;
    graphics_draw_text(this->osd_item.gr, curr_color, NULL, this->osd_item.font, this->message, &p, 0x10000, 0);
    graphics_draw_mode(this->osd_item.gr, draw_mode_end);
}

/**
 * @brief	Initialize the j1850 OSD
 * @param[in]	j1850	- the j1850 struct containing the state of the plugin
 *              nav     - the navit object
 *
 * @return	nothing
 *
 * Initialize the j1850 OSD
 *
 */
static void osd_j1850_init(struct j1850 *this, struct navit *nav) {

    struct color c;
    osd_set_std_graphic(nav, &this->osd_item, (struct osd_priv *)this);

    // Used when debugging or when the device is offline
    this->orange = graphics_gc_new(this->osd_item.gr);
    c.r = 0xFFFF;
    c.g = 0xA5A5;
    c.b = 0x0000;
    c.a = 65535;
    graphics_gc_set_foreground(this->orange, &c);
    graphics_gc_set_linewidth(this->orange, this->width);

    // Used when we are receiving real datas from the device
    this->white = graphics_gc_new(this->osd_item.gr);
    c.r = 65535;
    c.g = 65535;
    c.b = 65535;
    c.a = 65535;
    graphics_gc_set_foreground(this->white, &c);
    graphics_gc_set_linewidth(this->white, this->width);


    graphics_gc_set_linewidth(this->osd_item.graphic_fg_white, this->width);

    event_add_timeout(500, 1, callback_new_1(callback_cast(osd_j1850_draw), this));

    j1850_init_serial_port(this);

    // navit_add_callback(nav, this->click_cb = callback_new_attr_1(callback_cast (osd_j1850_click), attr_button, this));

    osd_j1850_draw(this, nav, NULL);
}

/**
 * @brief	Sends 'cmd' and reads the reply from the device
 * @param[in]	cmd	- the char * that we will write to the serial port
 *              device  - the serial port device
 *
 * @return	nothing
 *
 * Sends 'cmd' and reads the reply from the device
 *
 */
void send_and_read(unsigned char *cmd, int USB) {
    int n_written = 0;
    do {
        n_written += write( USB, &cmd[n_written], 1 );
    } while (cmd[n_written-1] != '\r' && n_written > 0);

    int n = 0;
    char buf = '\0';

    /* Whole response*/
    char response[255];

    do {
        n = read( USB, &buf, 1 );
        if(n == -1) {
            dbg(lvl_debug,"x");
        } else if (n==0) {
            dbg(lvl_debug,".");
        } else {
            dbg(lvl_debug,"[%s]", &buf);
        }
    } while( buf != '\r' && n > 0);

    if (n < 0) {
        dbg(lvl_error,"Read error");
    } else if (n == 0) {
        dbg(lvl_error,"Nothing to read?");
    } else {
        dbg(lvl_error,"Response : ");
    }
}

/**
 * @brief	Opens the serial port and saves state to the j1850 object
 * @param[in]	j1850	- the j1850 struct containing the state of the plugin
 *
 * @return	nothing
 *
 * Opens the serial port and saves state to the j1850 object
 *
 */
void j1850_init_serial_port(struct j1850 *j1850) {
    j1850->callback=callback_new_1(callback_cast(j1850_idle), j1850);
    // Fixme : we should read the device path from the config file
    j1850->device = open( "/dev/ttyUSB0", O_RDWR| O_NOCTTY );
    if ( j1850->device < 0 ) {
        dbg(lvl_error,"Can't open port");
        j1850->idle=event_add_timeout(100, 1, j1850->callback);
        return;
    }

    struct termios tty;
    struct termios tty_old;
    memset (&tty, 0, sizeof tty);

    /* Error Handling */
    if ( tcgetattr ( j1850->device, &tty ) != 0 ) {
        dbg(lvl_error,"Error");
        return;
    }

    /* Save old tty parameters */
    tty_old = tty;

    /* Set Baud Rate */
    cfsetospeed (&tty, (speed_t)B115200);
    cfsetispeed (&tty, (speed_t)B115200);

    /* Setting other Port Stuff */
    tty.c_cflag     &=  ~PARENB;        // Make 8n1
    tty.c_cflag     &=  ~CSTOPB;
    tty.c_cflag     &=  ~CSIZE;
    tty.c_cflag     |=  CS8;

    tty.c_cflag     &=  ~CRTSCTS;       // no flow control
    tty.c_cc[VMIN]      =   1;                  // read doesn't block
    tty.c_cc[VTIME]     =   10;                  // 0.5 seconds read timeout
    tty.c_cflag     |=  CREAD | CLOCAL;     // turn on READ & ignore ctrl lines

    /* Make raw */
    cfmakeraw(&tty);

    /* Flush Port, then applies attributes */
    tcflush( j1850->device, TCIFLUSH );
    if ( tcsetattr ( j1850->device, TCSANOW, &tty ) != 0) {
        dbg(lvl_error,"Flush error");
        return;
    }

    dbg(lvl_error,"Port init ok");
    // For the init part, we want to wait 1sec before each init string
    j1850->idle=event_add_timeout(1000, 1, j1850->callback);
}

/**
 * @brief	Creates the j1850 OSD and set some default properties
 * @param[in]	nav	- the navit object
 *              meth    - the osd_methods
 * 		attrs	- pointer to the attributes
 *
 * @return	nothing
 *
 * Creates the j1850 OSD and set some default properties
 *
 */
static struct osd_priv *osd_j1850_new(struct navit *nav, struct osd_methods *meth,
                                      struct attr **attrs) {
    struct j1850 *this=g_new0(struct j1850, 1);
    this->nav=nav;
    time_t current_time = time(NULL);
    // FIXME : make sure that the directory we log to exists!
    this->filename=g_strdup_printf("/home/navit/.navit/obd/%ld.log",(long)current_time);
    dbg(lvl_error,"Will log to %s", this->filename);
    this->init_string_index=0;
    struct attr *attr;
    this->osd_item.p.x = 120;
    this->osd_item.p.y = 20;
    this->osd_item.w = 160;
    this->osd_item.h = 20;
    this->osd_item.navit = nav;
    this->osd_item.font_size = 200;
    this->osd_item.meth.draw = osd_draw_cast(osd_j1850_draw);

    osd_set_std_attr(attrs, &this->osd_item, 2);
    attr = attr_search(attrs, NULL, attr_width);
    this->width=attr ? attr->u.num : 2;
    navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_j1850_init), attr_graphics_ready, this));
    return (struct osd_priv *) this;
}

/**
 * @brief	The plugin entry point
 *
 * @return	nothing
 *
 * The plugin entry point
 *
 */
void plugin_init(void) {
    struct attr callback,navit;
    struct attr_iter *iter;

    plugin_register_category_osd("j1850", osd_j1850_new);
}
