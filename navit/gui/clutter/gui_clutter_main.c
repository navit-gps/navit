/**
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

#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#include "glib.h"
#include <stdio.h>
#include <libintl.h>

#include "navit.h"
#include "config.h"
#include "plugin.h"
#include "gui.h"
#include "debug.h"

#include "projection.h"

#include "item.h"
#include "navit.h"
#include "vehicle.h"	
#include "profile.h"
#include "transform.h"
#include "coord.h"
#include "callback.h"
#include "point.h"
#include "graphics.h"
#include "navigation.h"
#include "attr.h"
#include "track.h"
#include "menu.h"
#include "map.h"

// Specific to this gui :
#include "gui_clutter.h"
ClutterTimeline *timeline = NULL;

#include "gui_clutter_animators.h"
#include "gui_clutter_menuhack.h"

ClutterActor *testActor;

struct gui_priv {
	struct navit *nav;
	int dyn_counter;
};

int gui_clutter_set_graphics(struct gui_priv *this_, struct graphics *gra);
struct gui_priv * gui_clutter_new(struct navit *nav, struct gui_methods *meth, struct attr **attrs);


int gui_clutter_main_loop(struct gui *this_)
{
	GSource *timeout;
	dbg(0,"Entering main loop\n");
}


struct gui_methods gui_clutter_methods = {
	gui_clutter_menubar_new,
	NULL, //gui_gtk_popup_new,
	gui_clutter_set_graphics, // Needed
	gui_clutter_main_loop, //gui_main_loop,
	NULL, //gui_gtk_datawindow_new,
	NULL, //gui_gtk_add_bookmark,
};


int
gui_clutter_set_graphics(struct gui_priv *this_, struct graphics *gra)
{
	dbg(0,"setting up the graphics\n");

	testActor=(ClutterActor *)graphics_get_data(gra, "navit_clutter_actor");
	if (!testActor) 
		return 1;
	return 0;
}


static void
gui_gtk_init(struct gui_priv *this, struct navit *nav)
{

	dbg(0,"More init\n");
	/*
	gui_gtk_toggle_init(this);
	gui_gtk_layouts_init(this);
	gui_gtk_projections_init(this);
	gui_gtk_vehicles_init(this);
	gui_gtk_maps_init(this);
	gui_gtk_destinations_init(this);
	gui_gtk_bookmarks_init(this);
	*/
}

static gboolean
gui_clutter_delete(ClutterActor *actor, ClutterEvent *event, struct navit *nav)
{
	dbg(0,"Exiting clutter gui\n");
	/* FIXME remove attr_navit callback */
	navit_destroy(nav);
	g_object_unref (timeline);
	dbg(0,"Done exiting clutter gui\n");
	return TRUE;
}

/**
 * Register a vehicule callback. It can be used to get infos like 
 * satellites in view, used, or any other information provided via 
 * nmea / gpsd.
 *
 * @param navit The navit instance
 * @param vehicle pointer to the active vehicule
 * @returns nothing
 */
static void vehicle_callback_handler( struct navit *nav, struct vehicle *v){
	char buffer [50];
	struct attr attr;
	int sats=0, sats_used=0;
	dbg(0,"Entering vehicle_callback_handler\n");

	if (vehicle_get_attr(v, attr_position_speed, &attr))
		sprintf (buffer, "%02.02f km/h", *attr.u.numd);
	else
		strcpy (buffer, "N/A");
  	dbg(0,"speed : %i ",buffer);

	if (vehicle_get_attr(v, attr_position_height, &attr))
		sprintf (buffer, "%.f m", *attr.u.numd);
	else
		strcpy (buffer, "N/A");
 	dbg(0,"alt : %i ",buffer);

	if (vehicle_get_attr(v, attr_position_sats, &attr))
		sats=attr.u.num;
	if (vehicle_get_attr(v, attr_position_sats_used, &attr))
		sats_used=attr.u.num;
 	dbg(0," sats : %i, used %i: \n",sats,sats_used);

}

/**
 * Perform the gui initialization
 *
 * @param navit The navit instance
 * @param meth an array of the methods used by the gui
 * @param attrs pointer to the config attributes
 * @returns a pointer to the gui instance
 */
struct gui_priv * gui_clutter_new(struct navit *nav, struct gui_methods *meth, struct attr **attrs) {

	struct gui_priv *this;

	this=g_new0(struct gui_priv, 1);
	this->nav=nav;


	*meth=gui_clutter_methods;

	init_clutter_gui();
	
	//  g_signal_connect(NULL, "delete-event", G_CALLBACK(gui_clutter_delete), nav);

// 	Extra init
// 	navit_add_callback(nav, callback_new_attr_1(callback_cast(gui_gtk_init), attr_navit, this));

	struct callback *cb=callback_new_attr_0(callback_cast(vehicle_callback_handler), attr_position_coord_geo);
	navit_add_callback(nav,cb);
	dbg(0,"Vehicule callback registered\n");

	return this;
}

void
plugin_init(void)
{
	plugin_register_gui_type("clutter", gui_clutter_new);
}
