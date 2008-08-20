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

#include "glib.h"
#include "debug.h"
#include <clutter/clutter.h>
#include "gui_clutter.h"
#include "navit_actor.h"

gboolean on_rect_button_release (ClutterActor *actor, ClutterEvent *event, gpointer data);
gboolean init_clutter_gui(void);

/**
 * Makes the button 'bumps' when clicked
 *
 * @param actor The actor which was clicked
 * @param event The event leading to the callback. Here we only handle button_release
 * @param data Extra informations about the event.
 * @returns a pointer to the gui instance
 */
gboolean on_rect_button_release (ClutterActor *actor, ClutterEvent *event, gpointer data)
{
	ClutterAlpha    *alpha;
	ClutterBehaviour *behave;
	
	gint x = 0;
	gint y = 0;
	clutter_event_get_coords (event, &x, &y);
	
	timeline = clutter_timeline_new_for_duration (200);
	
	alpha    = clutter_alpha_new_full (timeline,
					CLUTTER_ALPHA_SINE,
					NULL, NULL);
	
	behave = clutter_behaviour_scale_new (alpha,
						1.0, 1.0,  /* scale start */
						1.5, 1.5); /* scale end */
	
	clutter_actor_move_anchor_point_from_gravity (actor, CLUTTER_GRAVITY_CENTER);
	clutter_behaviour_apply (behave, actor );
	clutter_timeline_start (timeline);

	return TRUE; /* Stop further handling of this event. */
}

/**
 * Initialize the clutter GUI.
 * Currently, all widgets are hardcoded, but later we can change this using JSON
 *
 * @returns TRUE on success
 */

gboolean init_clutter_gui(void){

	ClutterActor *rect = NULL;

	ClutterColor stage_color = { 0x00, 0x00, 0x00, 0xff };
	ClutterColor rect_color = { 0xff, 0xff, 0xff, 0x99 };
	
	clutter_init (NULL,NULL); //&argc, &argv);
	
	/* Get the stage and set its size and color: */
	ClutterActor *stage = clutter_stage_get_default ();
	clutter_actor_set_size (stage, 800, 600);
	clutter_stage_set_color (CLUTTER_STAGE (stage), &stage_color);

	
	
	/* Add a button to the stage: */
	rect = clutter_rectangle_new_with_color (&rect_color);
	clutter_actor_set_size (rect, 70, 30);
	clutter_actor_set_position (rect, 700, 100);
	clutter_container_add_actor (CLUTTER_CONTAINER (stage), rect);
	
	/* Allow the actor to emit events.
	* By default only the stage does this. 	*/
	clutter_actor_set_reactive (rect, TRUE);
	g_signal_connect (rect, "button-release-event", G_CALLBACK (on_rect_button_release), NULL);
	
	clutter_actor_show (rect);

	
	/* Add another button to the stage: */
	rect = clutter_rectangle_new_with_color (&rect_color);
	clutter_actor_set_size (rect, 70, 30);
	clutter_actor_set_position (rect, 700, 150);
	clutter_container_add_actor (CLUTTER_CONTAINER (stage), rect);
	
	clutter_actor_set_reactive (rect, TRUE);
	g_signal_connect (rect, "button-release-event", G_CALLBACK (on_rect_button_release), NULL);

	clutter_actor_show (rect);
	
	/* The map drawing area */
	ClutterActor *testActor=NULL;
	testActor = foo_actor_new ();
	clutter_container_add_actor (CLUTTER_CONTAINER (stage), testActor);
	
	clutter_actor_set_rotation (testActor, CLUTTER_Y_AXIS, -30, 200, 0, 0);
	clutter_actor_set_position (testActor, 0, 100);
	/* End of the map drawing area */


	/* Show the stage: */
	clutter_actor_show (stage);
	dbg(0,"Clutter init OK\n");
	return TRUE;
}
