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



#include <math.h>
#include <stdio.h>
#include <glib.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>

#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <glib.h>
#include "config.h"
#include "config_.h"
#include "navit.h"
#include "coord.h"
#include "point.h"
#include "plugin.h"
#include "debug.h"
#include "item.h"
#include "xmlconfig.h"
#include "attr.h"
#include "layout.h"
#include "navigation.h"
#include "command.h"
#include "callback.h"
#include "graphics.h"
#include "track.h"
#include "vehicle.h"
#include "vehicleprofile.h"
#include "map.h"
#include "event.h"
#include "mapset.h"
#include "osd.h"
#include "route.h"
#include "search.h"
#include "callback.h"
#include "gui.h"
#include "util.h"

struct switch_layout {
    struct navit *nav;
    struct layout *layout;
};

extern void gui_change_color(struct colors* colors);

bool 
check_interior_light(void){
	FILE *fp;
    char result[5];
    // Open the command for reading.
    fp = popen("gpio read 6", "r");
    if (fp == NULL) {
        dbg(lvl_error, "Failed to run command\n" );
        return(NULL);
    }
    // Read the output a line at a time - output it.
    while (fgets(result, sizeof(result)-1, fp) != NULL) {
        strtok(result, "\n");
        if(result[0] == '1'){
			return true;
		}
	}
	return false;
}

static void
check_switch_layout(struct switch_layout *this)
{
	struct attr navit;
	navit.type=attr_navit;
    navit.u.navit=this->nav;
	
	
	
	
	
	bool light_on = check_interior_light();
	this->layout = navit_get_current_layout(this->nav);
	if(this->layout->dayname){
		if(!light_on){
			navit_set_layout_by_name(this->nav, this->layout->dayname);
			command_evaluate(&navit, "gui.change_gui_color_day()");
		}
	}
	if(this->layout->nightname){
		if(light_on){
			navit_set_layout_by_name(this->nav, this->layout->nightname);
			command_evaluate(&navit, "gui.change_gui_color_night()");

		}
	}
	
}

static void 
switch_layout_main(struct switch_layout *this, struct navit *nav){
	dbg(lvl_error, "switch_layout_main\n");
	event_add_timeout(1500, 1, callback_new_1(callback_cast(check_switch_layout), this));
}

static void 
switch_layout_init(struct switch_layout *this, struct navit *nav)
{
	dbg(lvl_error, "switch_layout_init\n");
	this->nav=nav;
	
	navit_add_callback(nav,callback_new_attr_1(callback_cast(switch_layout_main),attr_graphics_ready, this));
}
/**
 * @brief	The plugin entry point
 *
 * @return	nothing
 *
 * The plugin entry point
 *
 */
void
plugin_init(void)
{
	struct attr callback; 
	struct switch_layout *this=g_new0(struct switch_layout, 1);
	callback.type=attr_callback;
	callback.u.callback=callback_new_attr_1(callback_cast(switch_layout_init),attr_navit,this);
	config_add_attr(config, &callback);
	system("gpio mode 6 tri");
}
