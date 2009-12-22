/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2009 Navit Team
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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <glib.h>
#include <math.h>
#include <time.h>
#include "config.h"
#include "debug.h"
#include "navit.h"
#include "callback.h"
#include "gui.h"
#include "item.h"
#include "projection.h"
#include "map.h"
#include "mapset.h"
#include "main.h"
#include "coord.h"
#include "point.h"
#include "transform.h"
#include "param.h"
#include "menu.h"
#include "graphics.h"
#include "popup.h"
#include "data_window.h"
#include "route.h"
#include "navigation.h"
#include "speech.h"
#include "track.h"
#include "vehicle.h"
#include "layout.h"
#include "log.h"
#include "attr.h"
#include "event.h"
#include "file.h"
#include "profile.h"
#include "command.h"
#include "navit_nls.h"
#include "util.h"
#include "messages.h"
#include "vehicleprofile.h"
#include "sunriset.h"

/**
 * @defgroup navit the navit core instance. navit is the object containing nearly everything: A set of maps, one or more vehicle, a graphics object for rendering the map, a gui object for displaying the user interface, a route object, a navigation object and so on. Be warned that it is theoretically possible to have more than one navit object
 * @{
 */

//! The navit_vehicule
struct navit_vehicle {
	int follow;
	/*! Limit of the follow counter. See navit_add_vehicle */
	int follow_curr;
	/*! Deprecated : follow counter itself. When it reaches 'update' counts, map is recentered*/
	struct coord coord;
	int dir;
	int speed;
	struct coord last; /*< Position of the last update of this vehicle */
	struct vehicle *vehicle;
	struct attr callback;
	int animate_cursor;
};

struct navit {
	struct attr self;
	GList *mapsets;
	GList *layouts;
	struct gui *gui;
	struct layout *layout_current;
	struct graphics *gra;
	struct action *action;
	struct transformation *trans;
	struct compass *compass;
	struct route *route;
	struct navigation *navigation;
	struct speech *speech;
	struct tracking *tracking;
	int ready;
	struct window *win;
	struct displaylist *displaylist;
	int tracking_flag;
	int orientation;
	int recentdest_count;
	int osd_configuration;
	GList *vehicles;
	GList *windows_items;
	struct navit_vehicle *vehicle;
	struct callback_list *attr_cbl;
	struct callback *nav_speech_cb, *roadbook_callback, *popup_callback, *route_cb;
	struct datawindow *roadbook_window;
	struct map *bookmark;
	struct map *former_destination;
	GHashTable *bookmarks_hash;
	struct point pressed, last, current;
	int button_pressed,moved,popped,zoomed;
	int center_timeout;
	int autozoom_secs;
	int autozoom_min;
	int autozoom_active;
	struct event_timeout *button_timeout, *motion_timeout;
	struct callback *motion_timeout_callback;
	int ignore_button;
	int ignore_graphics_events;
	struct log *textfile_debug_log;
	struct pcoord destination;
	int destination_valid;
	int blocked;
	int w,h;
	int drag_bitmap;
	int use_mousewheel;
	struct messagelist *messages;
	struct callback *resize_callback,*button_callback,*motion_callback;
	struct vehicleprofile *vehicleprofile;
	GList *vehicleprofiles;
	int pitch;
	int follow_cursor;
	int prevTs;
	int graphics_flags;
	int zoom_min, zoom_max;
};

struct gui *main_loop_gui;

struct attr_iter {
	union {
		GList *list;
		struct mapset_handle *mapset_handle;
	} u;
};

static void navit_vehicle_update(struct navit *this_, struct navit_vehicle *nv);
static void navit_vehicle_draw(struct navit *this_, struct navit_vehicle *nv, struct point *pnt);
static int navit_add_vehicle(struct navit *this_, struct vehicle *v);
static int navit_set_attr_do(struct navit *this_, struct attr *attr, int init);
static int navit_get_cursor_pnt(struct navit *this_, struct point *p, int *dir);
static void navit_set_cursors(struct navit *this_);
static void navit_cmd_zoom_to_route(struct navit *this);
static void navit_cmd_set_center_cursor(struct navit *this_);
static void navit_cmd_announcer_toggle(struct navit *this_);
static void navit_set_vehicle(struct navit *this_, struct navit_vehicle *nv);

void
navit_add_mapset(struct navit *this_, struct mapset *ms)
{
	this_->mapsets = g_list_append(this_->mapsets, ms);
}

struct mapset *
navit_get_mapset(struct navit *this_)
{
	if(this_->mapsets){
		return this_->mapsets->data;
	} else {
		dbg(0,"No mapsets enabled! Is it on purpose? Navit can't draw a map. Please check your navit.xml\n");
	}
	return NULL;
}

struct tracking *
navit_get_tracking(struct navit *this_)
{
	return this_->tracking;
}

static void
navit_draw_async(struct navit *this_, int async)
{
	GList *l;
	struct navit_vehicle *nv;

	if (this_->blocked) {
		this_->blocked |= 2;
		return;
	}
	transform_setup_source_rect(this_->trans);
	l=this_->vehicles;
	while (l) {
		nv=l->data;
		navit_vehicle_draw(this_, nv, NULL);
		l=g_list_next(l);
	}
	graphics_draw(this_->gra, this_->displaylist, this_->mapsets->data, this_->trans, this_->layout_current, async, NULL, this_->graphics_flags|1);
}

void
navit_draw(struct navit *this_)
{
	navit_draw_async(this_, 0);
}

int
navit_get_ready(struct navit *this_)
{
	return this_->ready;
}



void
navit_draw_displaylist(struct navit *this_)
{
	if (this_->ready == 3)
		graphics_displaylist_draw(this_->gra, this_->displaylist, this_->trans, this_->layout_current, this_->graphics_flags|1);
}

static void
navit_redraw_route(struct navit *this_, struct route *route, struct attr *attr)
{
	int updated;
	if (attr->type != attr_route_status)
		return;
	updated=attr->u.num;
	if (this_->ready != 3)
		return;
	if (updated != route_status_path_done_new)
		return;
	if (this_->vehicle) {
		if (this_->vehicle->follow_curr == 1)
			return;
		if (this_->vehicle->follow_curr <= this_->vehicle->follow)
			this_->vehicle->follow_curr=this_->vehicle->follow;
	}
	navit_draw(this_);
}

void
navit_handle_resize(struct navit *this_, int w, int h)
{
	struct map_selection sel;
	int callback=(this_->ready == 1);
	this_->ready |= 2;
	if (this_->w != w || this_->h != h) {
		memset(&sel, 0, sizeof(sel));
		this_->w=w;
		this_->h=h;
		sel.u.p_rect.rl.x=w;
		sel.u.p_rect.rl.y=h;
		transform_set_screen_selection(this_->trans, &sel);
		graphics_init(this_->gra);
		graphics_set_rect(this_->gra, &sel.u.p_rect);
	}
	if (this_->ready == 3)
		navit_draw(this_);
	if (callback)
		callback_list_call_attr_1(this_->attr_cbl, attr_graphics_ready, this_);
}

static void
navit_resize(void *data, int w, int h)
{
	struct navit *this=data;
	if (!this->ignore_graphics_events)
		navit_handle_resize(this, w, h);
}

int
navit_get_width(struct navit *this_)
{
	return this_->w;
}


int
navit_get_height(struct navit *this_)
{
	return this_->h;
}

static void
navit_popup(void *data)
{
	struct navit *this_=data;
	popup(this_, 1, &this_->pressed);
	this_->button_timeout=NULL;
	this_->popped=1;
}


int
navit_ignore_button(struct navit *this_)
{
	if (this_->ignore_button)
		return 1;
	this_->ignore_button=1;
	return 0;
}

void
navit_ignore_graphics_events(struct navit *this_, int ignore)
{
	this_->ignore_graphics_events=ignore;
}

static void
update_transformation(struct transformation *tr, struct point *old, struct point *new, struct point *rot)
{
	struct coord co,cn;
	struct coord c,*cp;
	int yaw;
	double angleo,anglen;

	if (!transform_reverse(tr, old, &co))
		return;
	if (rot) {
		angleo=atan2(old->y-rot->y, old->x-rot->x)*180/M_PI;
		anglen=atan2(new->y-rot->y, new->x-rot->x)*180/M_PI;
		yaw=transform_get_yaw(tr)+angleo-anglen;
		transform_set_yaw(tr, yaw % 360);
	}
	if (!transform_reverse(tr, new, &cn))
		return;
	cp=transform_get_center(tr);
	c.x=cp->x+co.x-cn.x;
	c.y=cp->y+co.y-cn.y;
	dbg(1,"from 0x%x,0x%x to 0x%x,0x%x\n", cp->x, cp->y, c.x, c.y);
	transform_set_center(tr, &c);
}

static void
navit_set_timeout(struct navit *this_)
{
	struct attr follow;
	follow.type=attr_follow;
	follow.u.num=this_->center_timeout;
	navit_set_attr(this_, &follow);
}

int
navit_handle_button(struct navit *this_, int pressed, int button, struct point *p, struct callback *popup_callback)
{
	int border=16;

	dbg(1,"enter %d %d (ignore %d)\n",pressed,button,this_->ignore_button);
	callback_list_call_attr_4(this_->attr_cbl, attr_button, this_, GINT_TO_POINTER(pressed), GINT_TO_POINTER(button), p);
	if (this_->ignore_button) {
		this_->ignore_button=0;
		return 0;
	}
	if (pressed) {
		this_->pressed=*p;
		this_->last=*p;
		this_->zoomed=0;
		if (button == 1) {
			this_->button_pressed=1;
			this_->moved=0;
			this_->popped=0;
			if (popup_callback)
				this_->button_timeout=event_add_timeout(500, 0, popup_callback);
		}
		if (button == 2)
			navit_set_center_screen(this_, p, 1);
		if (button == 3)
			popup(this_, button, p);
		if (button == 4 && this_->use_mousewheel) {
			this_->zoomed = 1;
			navit_zoom_in(this_, 2, p);
		}
		if (button == 5 && this_->use_mousewheel) {
			this_->zoomed = 1;
			navit_zoom_out(this_, 2, p);
		}
	} else {

		this_->button_pressed=0;
		if (this_->button_timeout) {
			event_remove_timeout(this_->button_timeout);
			this_->button_timeout=NULL;
			if (! this_->moved && ! transform_within_border(this_->trans, p, border)) {
				navit_set_center_screen(this_, p, !this_->zoomed);
			}
		}
		if (this_->motion_timeout) {
			event_remove_timeout(this_->motion_timeout);
			this_->motion_timeout=NULL;
		}
		if (this_->moved) {
			struct point pr;
			pr.x=this_->w/2;
			pr.y=this_->h;
#if 0
			update_transformation(this_->trans, &this_->pressed, p, &pr);
#else
			update_transformation(this_->trans, &this_->pressed, p, NULL);
#endif
			graphics_draw_drag(this_->gra, NULL);
			graphics_overlay_disable(this_->gra, 0);
			if (!this_->zoomed) 
				navit_set_timeout(this_);
			navit_draw(this_);
		} else
			return 1;
	}
	return 0;
}

static void
navit_button(void *data, int pressed, int button, struct point *p)
{
	struct navit *this=data;
	dbg(1,"enter %d %d ignore %d\n",pressed,button,this->ignore_graphics_events);
	if (!this->ignore_graphics_events) {
		if (! this->popup_callback)
			this->popup_callback=callback_new_1(callback_cast(navit_popup), this);
		navit_handle_button(this, pressed, button, p, this->popup_callback);
	}
}


static void
navit_motion_timeout(struct navit *this_)
{
	int dx, dy;

	if (this_->drag_bitmap) {
		struct point point;
		point.x=(this_->current.x-this_->pressed.x);
		point.y=(this_->current.y-this_->pressed.y);
		if (graphics_draw_drag(this_->gra, &point)) {
			graphics_overlay_disable(this_->gra, 1);
			graphics_draw_mode(this_->gra, draw_mode_end);
			this_->moved=1;
			this_->motion_timeout=NULL;
			return;
		}
	} 
	dx=(this_->current.x-this_->last.x);
	dy=(this_->current.y-this_->last.y);
	if (dx || dy) {
		struct transformation *tr;
		struct point pr;
		this_->last=this_->current;
		graphics_overlay_disable(this_->gra, 1);
		tr=transform_dup(this_->trans);
		pr.x=this_->w/2;
		pr.y=this_->h;
#if 0
		update_transformation(tr, &this_->pressed, &this_->current, &pr);
#else
		update_transformation(tr, &this_->pressed, &this_->current, NULL);
#endif
#if 0
		graphics_displaylist_move(this_->displaylist, dx, dy);
#endif
		graphics_draw_cancel(this_->gra, this_->displaylist);
		graphics_displaylist_draw(this_->gra, this_->displaylist, tr, this_->layout_current, this_->graphics_flags);
		transform_destroy(tr);
		this_->moved=1;
	}
	this_->motion_timeout=NULL;
	return;
}

void
navit_handle_motion(struct navit *this_, struct point *p)
{
	int dx, dy;

	if (this_->button_pressed && !this_->popped) {
		dx=(p->x-this_->pressed.x);
		dy=(p->y-this_->pressed.y);
		if (dx < -8 || dx > 8 || dy < -8 || dy > 8) {
			this_->moved=1;
			if (this_->button_timeout) {
				event_remove_timeout(this_->button_timeout);
				this_->button_timeout=NULL;
			}
			this_->current=*p;
			if (! this_->motion_timeout_callback)
				this_->motion_timeout_callback=callback_new_1(callback_cast(navit_motion_timeout), this_);
			if (! this_->motion_timeout)
				this_->motion_timeout=event_add_timeout(100, 0, this_->motion_timeout_callback);
		}
	}
}

static void
navit_motion(void *data, struct point *p)
{
	struct navit *this=data;
	if (!this->ignore_graphics_events) 
		navit_handle_motion(this, p);
}

static void
navit_scale(struct navit *this_, long scale, struct point *p, int draw)
{
	struct coord c1, c2, *center;
	if (scale < this_->zoom_min)
		scale=this_->zoom_min;
	if (scale > this_->zoom_max)
		scale=this_->zoom_max;
	if (p)
		transform_reverse(this_->trans, p, &c1);
	transform_set_scale(this_->trans, scale);
	if (p) {
		transform_reverse(this_->trans, p, &c2);
		center = transform_center(this_->trans);
		center->x += c1.x - c2.x;
		center->y += c1.y - c2.y;
	}
	if (draw)
		navit_draw(this_);
}

/**
 * @brief Automatically adjusts zoom level
 *
 * This function automatically adjusts the current
 * zoom level according to the current speed.
 *
 * @param this_ The navit struct
 * @param center The "immovable" point - i.e. the vehicles position if we're centering on the vehicle
 * @param speed The vehicles speed in meters per second
 * @param dir The direction into which the vehicle moves
 */
static void
navit_autozoom(struct navit *this_, struct coord *center, int speed, int draw)
{
	struct point pc;
	int distance,w,h;
	double new_scale;
	long scale;

	if (! this_->autozoom_active) {
		return;
	}

	distance = speed * this_->autozoom_secs;

	transform_get_size(this_->trans, &w, &h);
	transform(this_->trans, transform_get_projection(this_->trans), center, &pc, 1, 0, 0, NULL);
	scale = transform_get_scale(this_->trans);

	/* We make sure that the point we want to see is within a certain range
	 * around the vehicle. The radius of this circle is the size of the
	 * screen. This doesn't necessarily mean the point is visible because of
	 * perspective etc. Quite rough, but should be enough. */
	
	if (w > h) {
		new_scale = (double)distance / h * 16; 
	} else {
		new_scale = (double)distance / w * 16; 
	}

	if (abs(new_scale - scale) < 2) { 
		return; // Smoothing
	}
	
	if (new_scale >= this_->autozoom_min) {
		navit_scale(this_, (long)new_scale, &pc, 0);
	} else {
		if (scale != this_->autozoom_min) {
			navit_scale(this_, this_->autozoom_min, &pc, 0);
		}
	}
}

/**
 * Change the current zoom level, zooming closer to the ground
 *
 * @param navit The navit instance
 * @param factor The zoom factor, usually 2
 * @param p The invariant point (if set to NULL, default to center)
 * @returns nothing
 */
void
navit_zoom_in(struct navit *this_, int factor, struct point *p)
{
	long scale=transform_get_scale(this_->trans)/factor;
	if (scale < 1)
		scale=1;
	navit_scale(this_, scale, p, 1);
}

/**
 * Change the current zoom level
 *
 * @param navit The navit instance
 * @param factor The zoom factor, usually 2
 * @param p The invariant point (if set to NULL, default to center)
 * @returns nothing
 */
void
navit_zoom_out(struct navit *this_, int factor, struct point *p)
{
	long scale=transform_get_scale(this_->trans)*factor;
	navit_scale(this_, scale, p, 1);
}

static int
navit_cmd_zoom_in(struct navit *this_)
{
	struct point p;
	if (this_->vehicle && this_->vehicle->follow_curr == 1 && navit_get_cursor_pnt(this_, &p, NULL)) {
		navit_zoom_in(this_, 2, &p);
		this_->vehicle->follow_curr=this_->vehicle->follow;
	} else
		navit_zoom_in(this_, 2, NULL);
	return 0;
}

static int
navit_cmd_zoom_out(struct navit *this_)
{
	struct point p;
	if (this_->vehicle && this_->vehicle->follow_curr == 1 && navit_get_cursor_pnt(this_, &p, NULL)) {
		navit_zoom_out(this_, 2, &p);
		this_->vehicle->follow_curr=this_->vehicle->follow;
	} else
		navit_zoom_out(this_, 2, NULL);
	return 0;
}

static struct command_table commands[] = {
	{"zoom_in",command_cast(navit_cmd_zoom_in)},
	{"zoom_out",command_cast(navit_cmd_zoom_out)},
	{"zoom_to_route",command_cast(navit_cmd_zoom_to_route)},
	{"set_center_cursor",command_cast(navit_cmd_set_center_cursor)},
	{"announcer_toggle",command_cast(navit_cmd_announcer_toggle)},
};
	

struct navit *
navit_new(struct attr *parent, struct attr **attrs)
{
	struct navit *this_=g_new0(struct navit, 1);
	struct pcoord center;
	struct coord co;
	struct coord_geo g;
	enum projection pro=projection_mg;
	int zoom = 256;
	g.lat=53.13;
	g.lng=11.70;

	this_->self.type=attr_navit;
	this_->self.u.navit=this_;
	this_->attr_cbl=callback_list_new();

	this_->bookmarks_hash=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	this_->orientation=-1;
	this_->tracking_flag=1;
	this_->recentdest_count=10;
	this_->osd_configuration=-1;

	this_->center_timeout = 10;
	this_->use_mousewheel = 1;
	this_->autozoom_secs = 10;
	this_->autozoom_min = 7;
	this_->autozoom_active = 0;
	this_->zoom_min = 1;
	this_->zoom_max = 2097152;
	this_->follow_cursor = 1;

	this_->trans = transform_new();
	transform_from_geo(pro, &g, &co);
	center.x=co.x;
	center.y=co.y;
	center.pro = pro;
	
	this_->prevTs=0;

	transform_setup(this_->trans, &center, zoom, (this_->orientation != -1) ? this_->orientation : 0);
	for (;*attrs; attrs++) {
		navit_set_attr_do(this_, *attrs, 1);
	}
	this_->displaylist=graphics_displaylist_new();
	command_add_table(this_->attr_cbl, commands, sizeof(commands)/sizeof(struct command_table), this_);

	this_->messages = messagelist_new(attrs);
	
	return this_;
}

static int
navit_set_gui(struct navit *this_, struct gui *gui)
{
	if (this_->gui)
		return 0;
	this_->gui=gui;
	if (gui_has_main_loop(this_->gui)) {
		if (! main_loop_gui) {
			main_loop_gui=this_->gui;
		} else {
			dbg(0,"gui with main loop already active, ignoring this instance");
			return 0;
		}
	}
	return 1;
}

void 
navit_add_message(struct navit *this_, char *message)
{
	message_new(this_->messages, message);
}

struct message
*navit_get_messages(struct navit *this_)
{
	return message_get(this_->messages);
}

static int
navit_set_graphics(struct navit *this_, struct graphics *gra)
{
	if (this_->gra)
		return 0;
	this_->gra=gra;
	this_->resize_callback=callback_new_attr_1(callback_cast(navit_resize), attr_resize, this_);
	graphics_add_callback(gra, this_->resize_callback);
	this_->button_callback=callback_new_attr_1(callback_cast(navit_button), attr_button, this_);
	graphics_add_callback(gra, this_->button_callback);
	this_->motion_callback=callback_new_attr_1(callback_cast(navit_motion), attr_motion, this_);
	graphics_add_callback(gra, this_->motion_callback);
	return 1;
}

struct graphics *
navit_get_graphics(struct navit *this_)
{
	return this_->gra;
}

struct vehicleprofile *
navit_get_vehicleprofile(struct navit *this_)
{
	return this_->vehicleprofile;
}

GList *
navit_get_vehicleprofiles(struct navit *this_)
{
	return this_->vehicleprofiles;
}

static void
navit_projection_set(struct navit *this_, enum projection pro)
{
	struct coord_geo g;
	struct coord *c;

	c=transform_center(this_->trans);
	transform_to_geo(transform_get_projection(this_->trans), c, &g);
	transform_set_projection(this_->trans, pro);
	transform_from_geo(pro, &g, c);
	navit_draw(this_);
}

/**
 * @param limit Limits the number of entries in the "backlog". Set to 0 for "infinite"
 */
static void
navit_append_coord(struct navit *this_, char *file, struct pcoord *c, const char *type, const char *description, GHashTable *h, int limit)
{
	FILE *f;
	int offset=0;
	char *buffer;
	int ch,prev,lines=0;
	int numc,readc;
	int fd;
	const char *prostr;

	f=fopen(file, "r");
	if (!f)
		goto new_file;
	if (limit != 0) {
		prev = '\n';
		while ((ch = fgetc(f)) != EOF) {
			if ((ch == '\n') && (prev != '\n')) {
				lines++;
			}
			prev = ch;
		}

		if (prev != '\n') { // Last line did not end with a newline
			lines++;
		}

		fclose(f);
		f = fopen(file, "r+");
		fd = fileno(f);
		while (lines >= limit) { // We have to "scroll up"
			rewind(f);
			numc = 0; // Counts how many bytes we have in our line to scroll up
			while ((ch = fgetc(f)) != EOF) {
				numc++;
				if (ch == '\n') {
					break;
				}
			}

			buffer=g_malloc(numc);
			offset = numc; // Offset holds where we currently are
			
			do {
				fseek(f,offset,SEEK_SET);
				readc = fread(buffer,1,numc,f);
				
				fseek(f,-(numc+readc),SEEK_CUR);
				fwrite(buffer,1,readc,f);

				offset += readc;
			} while (readc == numc);

			g_free(buffer);
			fflush(f);
			ftruncate(fd,(offset-numc));
#ifdef HAVE_FSYNC
			fsync(fd);
#endif

			lines--;
		}
		fclose(f);
	}

new_file:
	f=fopen(file, "a");
	if (f) {
		if (c) {
			prostr = projection_to_name(c->pro,NULL);
			fprintf(f,"%s%s%s0x%x %s0x%x type=%s label=\"%s\"\n",
				 prostr, *prostr ? ":" : "", 
				 c->x >= 0 ? "":"-", c->x >= 0 ? c->x : -c->x, 
				 c->y >= 0 ? "":"-", c->y >= 0 ? c->y : -c->y, 
				 type, description);
		} else
			fprintf(f,"\n");
	}
	fclose(f);
}

/*
 * navit_get_user_data_directory
 * 
 * returns the directory used to store user data files (center.txt,
 * destination.txt, bookmark.txt, ...)
 *
 * arg: gboolean create: create the directory if it does not exist
 */
static char*
navit_get_user_data_directory(gboolean create) {
	char *dir;
	dir = getenv("NAVIT_USER_DATADIR");
	if (create && !file_exists(dir)) {
		dbg(0,"creating dir %s\n", dir);
		if (file_mkdir(dir,0)) {
			dbg(0,"failed creating dir %s\n", dir);
			return NULL;
		}
	}

	return dir;
}

/*
 * navit_get_destination_file
 * 
 * returns the name of the file used to store destinations with its
 * full path
 *
 * arg: gboolean create: create the directory where the file is stored
 * if it does not exist
 */
static char*
navit_get_destination_file(gboolean create)
{
	return g_strjoin(NULL, navit_get_user_data_directory(create), "/destination.txt", NULL);
}

/*
 * navit_get_bookmark_file
 * 
 * returns the name of the file used to store bookmarks with its
 * full path
 *
 * arg: gboolean create: create the directory where the file is stored
 * if it does not exist
 */
static char*
navit_get_bookmark_file(gboolean create)
{
	return g_strjoin(NULL, navit_get_user_data_directory(create), "/bookmark.txt", NULL);
}


/*
 * navit_get_bookmark_file
 * 
 * returns the name of the file used to store the center file  with its
 * full path
 *
 * arg: gboolean create: create the directory where the file is stored
 * if it does not exist
 */
static char*
navit_get_center_file(gboolean create)
{
	return g_strjoin(NULL, navit_get_user_data_directory(create), "/center.txt", NULL);
}

static void
navit_set_center_from_file(struct navit *this_, char *file)
{
#ifndef HAVE_API_ANDROID
	FILE *f;
	char *line = NULL;

	size_t line_size = 0;
	enum projection pro;
	struct coord *center;

	f = fopen(file, "r");
	if (! f)
		return;
	getline(&line, &line_size, f);
	fclose(f);
	if (line) {
		center = transform_center(this_->trans);
		pro = transform_get_projection(this_->trans);
		coord_parse(g_strchomp(line), pro, center);
		free(line);
	}
	return;
#endif
}
 
static void
navit_write_center_to_file(struct navit *this_, char *file)
{
	FILE *f;
	enum projection pro;
	struct coord *center;

	f = fopen(file, "w+");
	if (f) {
		center = transform_center(this_->trans);
		pro = transform_get_projection(this_->trans);
		coord_print(pro, center, f);
		fclose(f);
	} else {
		perror(file);
	}
	return;
}


/**
 * Start the route computing to a given set of coordinates
 *
 * @param navit The navit instance
 * @param c The coordinate to start routing to
 * @param description A label which allows the user to later identify this destination in the former destinations selection
 * @returns nothing
 */
void
navit_set_destination(struct navit *this_, struct pcoord *c, const char *description, int async)
{
	if (c) {
		this_->destination=*c;
		this_->destination_valid=1;
	} else
		this_->destination_valid=0;
	char *destination_file = navit_get_destination_file(TRUE);
	navit_append_coord(this_, destination_file, c, "former_destination", description, NULL, this_->recentdest_count);
	g_free(destination_file);
	callback_list_call_attr_0(this_->attr_cbl, attr_destination);
	if (this_->route) {
		route_set_destination(this_->route, c, async);

		if (this_->ready == 3)
			navit_draw(this_);
	}
}

/**
 * @brief Checks if a route is calculated
 *
 * This function checks if a route is calculated.
 *
 * @param this_ The navit struct whose route should be checked.
 * @return True if the route is set, false otherwise.
 */
int
navit_check_route(struct navit *this_)
{
	if (this_->route) {
		return route_get_path_set(this_->route);
	}

	return 0;
}

/**
 * Record the given set of coordinates as a bookmark
 *
 * @param navit The navit instance
 * @param c The coordinate to store
 * @param description A label which allows the user to later identify this bookmark
 * @returns nothing
 */
void
navit_add_bookmark(struct navit *this_, struct pcoord *c, const char *description)
{
	char *bookmark_file = navit_get_bookmark_file(TRUE);
	navit_append_coord(this_,bookmark_file, c, "bookmark", description, this_->bookmarks_hash,0);
	g_free(bookmark_file);

	callback_list_call_attr_0(this_->attr_cbl, attr_bookmark_map);
}

struct navit *global_navit;

static void
navit_add_bookmarks_from_file(struct navit *this_)
{
	char *bookmark_file = navit_get_bookmark_file(FALSE);
	struct attr parent={attr_navit, .u.navit=this_};
	struct attr type={attr_type, {"textfile"}}, data={attr_data, {bookmark_file}};
	struct attr *attrs[]={&type, &data, NULL};

	this_->bookmark=map_new(&parent, attrs);
	g_free(bookmark_file);
}

static int
navit_former_destinations_active(struct navit *this_)
{
	char *destination_file = navit_get_destination_file(FALSE);
	FILE *f;
	int active=0;
	char buffer[3];
	f=fopen(destination_file,"r");
	if (f) {
		if(!fseek(f, -2, SEEK_END) && fread(buffer, 2, 1, f) == 1 && (buffer[0]!='\n' || buffer[1]!='\n')) 
			active=1;
		fclose(f);
	}
	g_free(destination_file);
	return active;
}

static void
navit_add_former_destinations_from_file(struct navit *this_)
{
	char *destination_file = navit_get_destination_file(FALSE);
	struct attr parent={attr_navit, .u.navit=this_};
	struct attr type={attr_type, {"textfile"}}, data={attr_data, {destination_file}};
	struct attr *attrs[]={&type, &data, NULL};
	struct map_rect *mr;
	struct item *item;
	int valid=0;
	struct coord c;
	struct pcoord pc;

	this_->former_destination=map_new(&parent, attrs);
	g_free(destination_file);
	if (!this_->route || !navit_former_destinations_active(this_))
		return;	
	mr=map_rect_new(this_->former_destination, NULL);
	while ((item=map_rect_get_item(mr))) {
		if (item->type == type_former_destination && item_coord_get(item, &c, 1)) 
			valid=1;
	}
	map_rect_destroy(mr);
	pc.pro=map_projection(this_->former_destination);
	pc.x=c.x;
	pc.y=c.y;
	if (valid) {
		route_set_destination(this_->route, &pc, 1);
		this_->destination=pc;
		this_->destination_valid=1;
	}
}


void
navit_textfile_debug_log(struct navit *this_, const char *fmt, ...)
{
	va_list ap;
	char *str1,*str2;
	va_start(ap, fmt);
	if (this_->textfile_debug_log && this_->vehicle) {
		str1=g_strdup_vprintf(fmt, ap);
		str2=g_strdup_printf("0x%x 0x%x%s%s\n", this_->vehicle->coord.x, this_->vehicle->coord.y, strlen(str1) ? " " : "", str1);
		log_write(this_->textfile_debug_log, str2, strlen(str2), 0);
		g_free(str2);
		g_free(str1);
	}
       	va_end(ap);
}

int 
navit_speech_estimate(struct navit *this_, char *str)
{
	return speech_estimate_duration(this_->speech, str);
}

void
navit_say(struct navit *this_, char *text)
{
	speech_say(this_->speech, text);
}

/**
 * @brief Toggles the navigation announcer for navit
 * @param this_ The navit object
 */
static void
navit_cmd_announcer_toggle(struct navit *this_)
{
    struct attr attr, speechattr;

    // search for the speech attribute
    if(!navit_get_attr(this_, attr_speech, &speechattr, NULL))
        return;
    // find out if the corresponding attribute attr_active has been set
    if(speech_get_attr(speechattr.u.speech, attr_active, &attr, NULL)) {
        // flip it then...
        attr.u.num = !attr.u.num;
    } else {
        // otherwise disable it because voice is enabled by default
        attr.type = attr_active;
        attr.u.num = 0;
    }

    // apply the new state
    if(!speech_set_attr(speechattr.u.speech, &attr))
        return;

    // announce that the speech attribute has changed
    callback_list_call_attr_0(this_->attr_cbl, attr_speech);
}

void
navit_speak(struct navit *this_)
{
	struct navigation *nav=this_->navigation;
	struct map *map=NULL;
	struct map_rect *mr=NULL;
	struct item *item;
	struct attr attr;

    if (!speech_get_attr(this_->speech, attr_active, &attr, NULL))
        attr.u.num = 1;
    dbg(1, "this_.speech->active %i\n", attr.u.num);
    if(!attr.u.num)
        return;

	if (nav)
		map=navigation_get_map(nav);
	if (map)
		mr=map_rect_new(map, NULL);
	if (mr) {
		while ((item=map_rect_get_item(mr)) && (item->type == type_nav_position || item->type == type_nav_none));
		if (item && item_attr_get(item, attr_navigation_speech, &attr)) {
			speech_say(this_->speech, attr.u.str);
			navit_add_message(this_, attr.u.str);
			navit_textfile_debug_log(this_, "type=announcement label=\"%s\"", attr.u.str);
		}
		map_rect_destroy(mr);
	}
}

static void
navit_window_roadbook_update(struct navit *this_)
{
	struct navigation *nav=this_->navigation;
	struct map *map=NULL;
	struct map_rect *mr=NULL;
	struct item *item;
	struct attr attr;
	struct param_list param[5];
	int secs;

	dbg(1,"enter\n");
	datawindow_mode(this_->roadbook_window, 1);
	if (nav)
		map=navigation_get_map(nav);
	if (map)
		mr=map_rect_new(map, NULL);
	dbg(0,"nav=%p map=%p mr=%p\n", nav, map, mr);
	if (mr) {
		dbg(0,"while loop\n");
		while ((item=map_rect_get_item(mr))) {
			dbg(0,"item=%p\n", item);
			attr.u.str=NULL;
			if (item->type != type_nav_position) {
				item_attr_get(item, attr_navigation_long, &attr);
				if (attr.u.str == NULL) {
					continue;
				}
				dbg(2, "Command='%s'\n", attr.u.str);
				param[0].value=g_strdup(attr.u.str);
			} else
				param[0].value=_("Position");
			param[0].name=_("Command");

			item_attr_get(item, attr_length, &attr);
			dbg(2, "Length=%d\n", attr.u.num);
			param[1].name=_("Length");

			if ( attr.u.num >= 2000 )
			{
				param[1].value=g_strdup_printf("%5.1f %s",(float)attr.u.num / 1000, _("km") );
			}
			else
			{
				param[1].value=g_strdup_printf("%7d %s",attr.u.num, _("m"));
			}

			item_attr_get(item, attr_time, &attr);
			dbg(2, "Time=%d\n", attr.u.num);
			secs=attr.u.num/10;
			param[2].name=_("Time");
			if ( secs >= 3600 )
			{
				param[2].value=g_strdup_printf("%d:%02d:%02d",secs / 60, ( secs / 60 ) % 60 , secs % 60);
			}
			else
			{
				param[2].value=g_strdup_printf("%d:%02d",secs / 60, secs % 60);
			}

			item_attr_get(item, attr_destination_length, &attr);
			dbg(2, "Destlength=%d\n", attr.u.num);
			param[3].name=_("Destination Length");
			if ( attr.u.num >= 2000 )
			{
				param[3].value=g_strdup_printf("%5.1f %s",(float)attr.u.num / 1000, _("km") );
			}
			else
			{
				param[3].value=g_strdup_printf("%d %s",attr.u.num, _("m"));
			}

			item_attr_get(item, attr_destination_time, &attr);
			dbg(2, "Desttime=%d\n", attr.u.num);
			secs=attr.u.num/10;
			param[4].name=_("Destination Time");
			if ( secs >= 3600 )
			{
				param[4].value=g_strdup_printf("%d:%02d:%02d",secs / 3600, (secs / 60 ) % 60 , secs % 60);
			}
			else
			{
				param[4].value=g_strdup_printf("%d:%02d",secs / 60, secs % 60);
			}
			datawindow_add(this_->roadbook_window, param, 5);
		}
		map_rect_destroy(mr);
	}
	datawindow_mode(this_->roadbook_window, 0);
}

void
navit_window_roadbook_destroy(struct navit *this_)
{
	dbg(0, "enter\n");
	navigation_unregister_callback(this_->navigation, attr_navigation_long, this_->roadbook_callback);
	this_->roadbook_window=NULL;
	this_->roadbook_callback=NULL;
}
void
navit_window_roadbook_new(struct navit *this_)
{
	if (!this_->gui || this_->roadbook_callback || this_->roadbook_window) {
		return;
	}

	this_->roadbook_callback=callback_new_1(callback_cast(navit_window_roadbook_update), this_);
	navigation_register_callback(this_->navigation, attr_navigation_long, this_->roadbook_callback);
	this_->roadbook_window=gui_datawindow_new(this_->gui, _("Roadbook"), NULL, callback_new_1(callback_cast(navit_window_roadbook_destroy), this_));
	navit_window_roadbook_update(this_);
}

void
navit_init(struct navit *this_)
{
	struct mapset *ms;
	struct map *map;
	int callback;

	dbg(2,"enter gui %p graphics %p\n",this_->gui,this_->gra);
	if (!this_->gui) {
		dbg(0,"no gui\n");
		navit_destroy(this_);
		return;
	}
	if (!this_->gra) {
		dbg(0,"no graphics\n");
		navit_destroy(this_);
		return;
	}
	dbg(2,"Connecting gui to graphics\n");
	if (gui_set_graphics(this_->gui, this_->gra)) {
		struct attr attr_type_gui, attr_type_graphics;
		gui_get_attr(this_->gui, attr_type, &attr_type_gui, NULL);
		graphics_get_attr(this_->gra, attr_type, &attr_type_graphics, NULL);
		dbg(0,"failed to connect graphics '%s' to gui '%s'\n", attr_type_graphics.u.str, attr_type_gui.u.str);
		dbg(0," Please see http://wiki.navit-project.org/index.php/Failed_to_connect_graphics_to_gui\n");
		dbg(0," for explanations and solutions\n");

		navit_destroy(this_);
		return;
	}
	dbg(2,"Initializing graphics\n");
	dbg(2,"Setting Vehicle\n");
	navit_set_vehicle(this_, this_->vehicle);
	dbg(2,"Adding dynamic maps to mapset %p\n",this_->mapsets);
	if (this_->mapsets) {
		ms=this_->mapsets->data;
		if (this_->route) {
			if ((map=route_get_map(this_->route)))
				mapset_add_attr(ms, &(struct attr){attr_map,.u.map=map});
			if ((map=route_get_graph_map(this_->route))) {
				mapset_add_attr(ms, &(struct attr){attr_map,.u.map=map});
				map_set_attr(map, &(struct attr ){attr_active,.u.num=0});
			}
			route_set_mapset(this_->route, ms);
			route_set_projection(this_->route, transform_get_projection(this_->trans));
		}
		if (this_->tracking) {
			tracking_set_mapset(this_->tracking, ms);
			if (this_->route)
				tracking_set_route(this_->tracking, this_->route);
		}
		if (this_->navigation) {
			if ((map=navigation_get_map(this_->navigation))) {
				mapset_add_attr(ms, &(struct attr){attr_map,.u.map=map});
				map_set_attr(map, &(struct attr ){attr_active,.u.num=0});
			}
		}
		if (this_->tracking) {
			if ((map=tracking_get_map(this_->tracking))) {
				mapset_add_attr(ms, &(struct attr){attr_map,.u.map=map});
				map_set_attr(map, &(struct attr ){attr_active,.u.num=0});
			}
		}
		navit_add_bookmarks_from_file(this_);
		navit_add_former_destinations_from_file(this_);
	}
	if (this_->route) {
		struct attr callback;
		this_->route_cb=callback_new_attr_1(callback_cast(navit_redraw_route), attr_route_status, this_);
		callback.type=attr_callback;
		callback.u.callback=this_->route_cb;
		route_add_attr(this_->route, &callback);
	}
	if (this_->navigation) {
		if (this_->speech) {
			this_->nav_speech_cb=callback_new_1(callback_cast(navit_speak), this_);
			navigation_register_callback(this_->navigation, attr_navigation_speech, this_->nav_speech_cb);
		}
		if (this_->route)
			navigation_set_route(this_->navigation, this_->route);
	}
	dbg(2,"Setting Center\n");
	char *center_file = navit_get_center_file(FALSE);
	navit_set_center_from_file(this_, center_file);
	g_free(center_file);
#if 0
	if (this_->menubar) {
		men=menu_add(this_->menubar, "Data", menu_type_submenu, NULL);
		if (men) {
			navit_add_menu_windows_items(this_, men);
		}
	}
#endif
	global_navit=this_;
#if 0
	navit_window_roadbook_new(this_);
	navit_window_items_new(this_);
#endif

	messagelist_init(this_->messages);

	navit_set_cursors(this_);

	callback_list_call_attr_1(this_->attr_cbl, attr_navit, this_);
	callback=(this_->ready == 2);
	this_->ready|=1;
	dbg(2,"ready=%d\n",this_->ready);
	if (this_->ready == 3)
		navit_draw(this_);
	if (callback)
		callback_list_call_attr_1(this_->attr_cbl, attr_graphics_ready, this_);
#if 0
	routech_test(this_);
#endif
}

void
navit_zoom_to_route(struct navit *this_, int orientation)
{
	struct map *map;
	struct map_rect *mr=NULL;
	struct item *item;
	struct coord c;
	struct coord_rect r;
	int count=0,scale=16;
	if (! this_->route)
		return;
	dbg(1,"enter\n");
	map=route_get_map(this_->route);
	dbg(1,"map=%p\n",map);
	if (map)
		mr=map_rect_new(map, NULL);
	dbg(1,"mr=%p\n",mr);
	if (mr) {
		while ((item=map_rect_get_item(mr))) {
			dbg(1,"item=%s\n", item_to_name(item->type));
			while (item_coord_get(item, &c, 1)) {
				dbg(1,"coord\n");
				if (!count) 
					r.lu=r.rl=c;
				else
					coord_rect_extend(&r, &c);	
				count++;
			}
		}
	}
	if (! count)
		return;
	c.x=(r.rl.x+r.lu.x)/2;
	c.y=(r.rl.y+r.lu.y)/2;
	dbg(1,"count=%d\n",count);
	if (orientation != -1)
		transform_set_yaw(this_->trans, orientation);
	transform_set_center(this_->trans, &c);
	dbg(1,"%x,%x-%x,%x\n", r.rl.x,r.rl.y,r.lu.x,r.lu.y);
	while (scale < 1<<20) {
		struct point p1,p2;
		transform_set_scale(this_->trans, scale);
		transform_setup_source_rect(this_->trans);
		transform(this_->trans, transform_get_projection(this_->trans), &r.lu, &p1, 1, 0, 0, NULL);
		transform(this_->trans, transform_get_projection(this_->trans), &r.rl, &p2, 1, 0, 0, NULL);
		dbg(1,"%d,%d-%d,%d\n",p1.x,p1.y,p2.x,p2.y);
		if (p1.x < 0 || p2.x < 0 || p1.x > this_->w || p2.x > this_->w ||
		    p1.y < 0 || p2.y < 0 || p1.y > this_->h || p2.y > this_->h)
			scale*=2;
		else
			break;
	
	}
	if (this_->ready == 3)
		navit_draw_async(this_,0);
}

static void
navit_cmd_zoom_to_route(struct navit *this)
{
	navit_zoom_to_route(this, 0);
}


/**
 * Change the current zoom level
 *
 * @param navit The navit instance
 * @param center The point where to center the map, including its projection
 * @returns nothing
 */
void
navit_set_center(struct navit *this_, struct pcoord *center, int set_timeout)
{
	struct coord *c=transform_center(this_->trans);
	struct coord c1,c2;
	enum projection pro = transform_get_projection(this_->trans);
	if (pro != center->pro) {
		c1.x = center->x;
		c1.y = center->y;
		transform_from_to(&c1, center->pro, &c2, pro);
	} else {
		c2.x = center->x;
		c2.y = center->y;
	}
	*c=c2;
	if (set_timeout) 
		navit_set_timeout(this_);
	if (this_->ready == 3)
		navit_draw(this_);
}

static void
navit_set_center_coord_screen(struct navit *this_, struct coord *c, struct point *p, int set_timeout)
{
	int width, height;
	struct point po;
	transform_set_center(this_->trans, c);
	transform_get_size(this_->trans, &width, &height);
	po.x=width/2;
	po.y=height/2;
	update_transformation(this_->trans, &po, p, NULL);
	if (set_timeout)
		navit_set_timeout(this_);
}

/**
 * Links all vehicles to a cursor depending on the current profile.
 *
 * @param this_ A navit instance
 * @author Ralph Sennhauser (10/2009)
 */
static void
navit_set_cursors(struct navit *this_)
{
	struct attr name;
	struct navit_vehicle *nv;
	struct cursor *c;
	GList *v;

	v=g_list_first(this_->vehicles); // GList of navit_vehicles
	while (v) {
		nv=v->data;
		if (vehicle_get_attr(nv->vehicle, attr_cursorname, &name, NULL))
			c=layout_get_cursor(this_->layout_current, name.u.str);
		else
			c=layout_get_cursor(this_->layout_current, "default");
		vehicle_set_cursor(nv->vehicle, c);
		v=g_list_next(v);
	}
	return;
}

static int
navit_get_cursor_pnt(struct navit *this_, struct point *p, int *dir)
{
	int width, height;
	struct navit_vehicle *nv=this_->vehicle;

        float offset=30;            // Cursor offset from the center of the screen (percent).
#if 0 /* Better improve track.c to get that issue resolved or make it configurable with being off the default, the jumping back to the center is a bit annoying */
        float min_offset = 0.;      // Percent offset at min_offset_speed.
        float max_offset = 30.;     // Percent offset at max_offset_speed.
        int min_offset_speed = 2;   // Speed in km/h
        int max_offset_speed = 50;  // Speed ini km/h
        // Calculate cursor offset from the center of the screen, upon speed.
        if (nv->speed <= min_offset_speed) {
            offset = min_offset;
        } else if (nv->speed > max_offset_speed) {
            offset = max_offset;
        } else {
            offset = (max_offset - min_offset) / (max_offset_speed - min_offset_speed) * (nv->speed - min_offset_speed);
        }
#endif

	transform_get_size(this_->trans, &width, &height);
	if (this_->orientation == -1) {
		p->x=50*width/100;
		p->y=(50 + offset)*height/100;
		if (dir)
			*dir=nv->dir;
	} else {
		int mdir;
		if (this_->tracking && this_->tracking_flag) {
			mdir = tracking_get_angle(this_->tracking) - this_->orientation;
		} else {
			mdir=nv->dir-this_->orientation;
		}

		p->x=(50 - offset*sin(M_PI*mdir/180.))*width/100;
		p->y=(50 + offset*cos(M_PI*mdir/180.))*height/100;
		if (dir)
			*dir=this_->orientation;
	}
	return 1;
}

static void
navit_set_center_cursor(struct navit *this_)
{
	int dir;
	struct point pn;
	struct navit_vehicle *nv=this_->vehicle;
	navit_get_cursor_pnt(this_, &pn, &dir);
	transform_set_yaw(this_->trans, dir);
	navit_set_center_coord_screen(this_, &nv->coord, &pn, 0);
	navit_autozoom(this_, &nv->coord, nv->speed, 0);
	if (this_->ready == 3)
		navit_draw_async(this_, 1);
}

static void
navit_cmd_set_center_cursor(struct navit *this_)
{
	navit_set_center_cursor(this_);
}

void
navit_set_center_screen(struct navit *this_, struct point *p, int set_timeout)
{
	struct coord c;
	struct pcoord pc;
	transform_reverse(this_->trans, p, &c);
	pc.x = c.x;
	pc.y = c.y;
	pc.pro = transform_get_projection(this_->trans);
	navit_set_center(this_, &pc, set_timeout);
}

#if 0
		switch((*attrs)->type) {
		case attr_zoom:
			zoom=(*attrs)->u.num;
			break;
		case attr_center:
			g=*((*attrs)->u.coord_geo);
			break;
#endif

static int
navit_set_attr_do(struct navit *this_, struct attr *attr, int init)
{
	int dir=0, orient_old=0, attr_updated=0;
	struct coord co;
	long zoom;
	GList *l;
	struct navit_vehicle *nv;
	struct attr active=(struct attr){attr_active,{(void *)0}};
	struct layout *lay;

	switch (attr->type) {
	case attr_autozoom:
		attr_updated=(this_->autozoom_secs != attr->u.num);
		this_->autozoom_secs = attr->u.num;
		break;
	case attr_autozoom_active:
		attr_updated=(this_->autozoom_active != attr->u.num);
		this_->autozoom_active = attr->u.num;
		break;
	case attr_center:
		transform_from_geo(transform_get_projection(this_->trans), attr->u.coord_geo, &co);
		dbg(1,"0x%x,0x%x\n",co.x,co.y);
		transform_set_center(this_->trans, &co);
		break;
	case attr_drag_bitmap:
		attr_updated=(this_->drag_bitmap != !!attr->u.num);
		this_->drag_bitmap=!!attr->u.num;
		break;
	case attr_flags_graphics:
		attr_updated=(this_->graphics_flags != attr->u.num);
		this_->graphics_flags=attr->u.num;
		break;
	case attr_follow:
		if (!this_->vehicle)
			return 0;
		attr_updated=(this_->vehicle->follow_curr != attr->u.num);
		this_->vehicle->follow_curr = attr->u.num;
		break;
	case attr_layout:
		if(this_->layout_current!=attr->u.layout) {
			this_->layout_current=attr->u.layout;
			graphics_font_destroy_all(this_->gra);
			navit_set_cursors(this_);
			if (this_->ready == 3)
				navit_draw(this_);
			attr_updated=1;
		}
		break;
	case attr_layout_name:
		l=this_->layouts;
		while (l) {
			lay=l->data;
			if (!strcmp(lay->name,attr->u.str)) {
				struct attr attr;
				attr.type=attr_layout;
				attr.u.layout=lay;
				return navit_set_attr_do(this_, &attr, init);
			}
			l=g_list_next(l);
		} 		
		return 0;
	case attr_orientation:
		orient_old=this_->orientation;
		this_->orientation=attr->u.num;
		if (!init) {
			if (this_->orientation != -1) {
				dir = this_->orientation;
			} else {
				if (this_->vehicle) {
					dir = this_->vehicle->dir;
				}
			}
			transform_set_yaw(this_->trans, dir);
			if (orient_old != this_->orientation) {
#if 0
				if (this_->ready == 3)
					navit_draw(this_);
#endif
				attr_updated=1;
			}
		}
		break;
	case attr_osd_configuration:
		dbg(0,"setting osd_configuration to %d (was %d)\n", attr->u.num, this_->osd_configuration);
		attr_updated=(this_->osd_configuration != attr->u.num);
		this_->osd_configuration=attr->u.num;
		break;
	case attr_pitch:
		attr_updated=(this_->pitch != attr->u.num);
		this_->pitch=attr->u.num;
		transform_set_pitch(this_->trans, this_->pitch);
		if (!init && attr_updated && this_->ready == 3)
			navit_draw(this_);
		break;
	case attr_projection:
		if(this_->trans && transform_get_projection(this_->trans) != attr->u.projection) {
			navit_projection_set(this_, attr->u.projection);
			attr_updated=1;
		}
		break;
	case attr_recent_dest:
		attr_updated=(this_->recentdest_count != attr->u.num);
		this_->recentdest_count=attr->u.num;
		break;
	case attr_speech:
        	if(this_->speech && this_->speech != attr->u.speech) {
			attr_updated=1;
			this_->speech = attr->u.speech;
        	}
		break;
	case attr_timeout:
		attr_updated=(this_->center_timeout != attr->u.num);
		this_->center_timeout = attr->u.num;
		break;
	case attr_tracking:
		attr_updated=(this_->tracking_flag != !!attr->u.num);
		this_->tracking_flag=!!attr->u.num;
		break;
	case attr_use_mousewheel:
		attr_updated=(this_->use_mousewheel != !!attr->u.num);
		this_->use_mousewheel=!!attr->u.num;
		break;
	case attr_vehicle:
		l=this_->vehicles;
		while(l) {
			nv=l->data;
			if (nv->vehicle == attr->u.vehicle) {
				if (!this_->vehicle || this_->vehicle->vehicle != attr->u.vehicle) {
					if (this_->vehicle)
						vehicle_set_attr(this_->vehicle->vehicle, &active);
					active.u.num=1;
					vehicle_set_attr(nv->vehicle, &active);
					attr_updated=1;
				}
				navit_set_vehicle(this_, nv);
			}
			l=g_list_next(l);
		}
		break;
	case attr_zoom:
		zoom=transform_get_scale(this_->trans);
		attr_updated=(zoom != attr->u.num);
		transform_set_scale(this_->trans, attr->u.num);
		if (attr_updated && !init) 
			navit_draw(this_);
		break;
	case attr_zoom_min:
		attr_updated=(attr->u.num != this_->zoom_min);
		this_->zoom_min=attr->u.num;
		break;
	case attr_zoom_max:
		attr_updated=(attr->u.num != this_->zoom_max);
		this_->zoom_max=attr->u.num;
		break;
	case attr_message:
		navit_add_message(this_, attr->u.str);
		break;
	case attr_follow_cursor:
		attr_updated=(this_->follow_cursor != !!attr->u.num);
		this_->follow_cursor=!!attr->u.num;
		break;
	default:
		return 0;
	}
	if (attr_updated && !init) {
		callback_list_call_attr_2(this_->attr_cbl, attr->type, this_, attr);
		if (attr->type == attr_osd_configuration)
			graphics_draw_mode(this_->gra, draw_mode_end);
	}
	return 1;
}

int
navit_set_attr(struct navit *this_, struct attr *attr)
{
	return navit_set_attr_do(this_, attr, 0);
}

int
navit_get_attr(struct navit *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter)
{
	struct message *msg;
	int len,offset;
	int ret=1;

	switch (type) {
	case attr_message:
		msg = navit_get_messages(this_);
		
		if (!msg) {
			return 0;
		}

		len = 0;
		while (msg) {
			len += strlen(msg->text) + 1;
			msg = msg->next;
		}
		attr->u.str = g_malloc(len + 1);
		
		msg = navit_get_messages(this_);
		offset = 0;
		while (msg) {
			g_stpcpy((attr->u.str + offset), msg->text);
			offset += strlen(msg->text);
			attr->u.str[offset] = '\n';
			offset++;

			msg = msg->next;
		}

		attr->u.str[len] = '\0';
		break;
	case attr_bookmark_map:
		attr->u.map=this_->bookmark;
		break;
	case attr_callback_list:
		attr->u.callback_list=this_->attr_cbl;
		break;
	case attr_destination:
		if (! this_->destination_valid)
			return 0;
		attr->u.pcoord=&this_->destination;
		break;
	case attr_displaylist:
		attr->u.displaylist=this_->displaylist;
		return (attr->u.displaylist != NULL);
	case attr_follow:
		if (!this_->vehicle)
			return 0;
		attr->u.num=this_->vehicle->follow_curr;
		break;
	case attr_former_destination_map:
		attr->u.map=this_->former_destination;
		break;
	case attr_graphics:
		attr->u.graphics=this_->gra;
		ret=(attr->u.graphics != NULL);
		break;
	case attr_gui:
		attr->u.gui=this_->gui;
		ret=(attr->u.gui != NULL);
		break;
	case attr_layout:
		if (iter) {
			if (iter->u.list) {
				iter->u.list=g_list_next(iter->u.list);
			} else { 
				iter->u.list=this_->layouts;
			}
			if (!iter->u.list)
				return 0;
			attr->u.layout=(struct layout *)iter->u.list->data;
		} else {
			attr->u.layout=this_->layout_current;
		}
		break;
	case attr_map:
		if (iter && this_->mapsets) {
			if (!iter->u.mapset_handle) {
				iter->u.mapset_handle=mapset_open((struct mapset *)this_->mapsets->data);
			}
			attr->u.map=mapset_next(iter->u.mapset_handle, 0);
			if(!attr->u.map) {
				mapset_close(iter->u.mapset_handle);
				return 0;
			}
		} else {
			return 0;
		}
		break;
	case attr_mapset:
		attr->u.mapset=this_->mapsets->data;
		return (attr->u.mapset != NULL);
	case attr_navigation:
		attr->u.navigation=this_->navigation;
		break;
	case attr_orientation:
		attr->u.num=this_->orientation;
		break;
	case attr_osd_configuration:
		attr->u.num=this_->osd_configuration;
		break;
	case attr_pitch:
		attr->u.num=transform_get_pitch(this_->trans);
		break;
	case attr_projection:
		if(this_->trans) {
			attr->u.num=transform_get_projection(this_->trans);
		} else {
			return 0;
		}
		break;
	case attr_route:
		attr->u.route=this_->route;
		break;
	case attr_speech:
	        attr->u.speech=this_->speech;
	        break;
	case attr_tracking:
		attr->u.num=this_->tracking_flag;
		break;
	case attr_transformation:
		attr->u.transformation=this_->trans;
		break;
	case attr_vehicle:
		if(iter) {
			if(iter->u.list) {
				iter->u.list=g_list_next(iter->u.list);
			} else { 
				iter->u.list=this_->vehicles;
			}
			if(!iter->u.list)
				return 0;
			attr->u.vehicle=((struct navit_vehicle*)iter->u.list->data)->vehicle;
		} else {
			if(this_->vehicle) {
				attr->u.vehicle=this_->vehicle->vehicle;
			} else {
				return 0;
			}
		}
		break;
	case attr_zoom:
		attr->u.num=transform_get_scale(this_->trans);
		break;
	case attr_autozoom_active:
		attr->u.num=this_->autozoom_active;
		break;
	case attr_follow_cursor:
		attr->u.num=this_->follow_cursor;
		break;
	default:
		return 0;
	}
	attr->type=type;
	return ret;
}

static int
navit_add_log(struct navit *this_, struct log *log)
{
	struct attr type_attr;
	if (!log_get_attr(log, attr_type, &type_attr, NULL))
		return 0;
	if (!strcmp(type_attr.u.str, "textfile_debug")) {
		char *header = "type=track_tracked\n";
		if (this_->textfile_debug_log)
			return 0;
		log_set_header(log, header, strlen(header));
		this_->textfile_debug_log=log;
		return 1;
	}
	return 0;
}

int
navit_add_attr(struct navit *this_, struct attr *attr)
{
	int ret=1;
	switch (attr->type) {
	case attr_callback:
		navit_add_callback(this_, attr->u.callback);
		break;
	case attr_log:
		ret=navit_add_log(this_, attr->u.log);
		break;
	case attr_gui:
		ret=navit_set_gui(this_, attr->u.gui);
		break;
	case attr_graphics:
		ret=navit_set_graphics(this_, attr->u.graphics);
		break;
	case attr_layout:
		this_->layouts = g_list_append(this_->layouts, attr->u.layout);
		if(!this_->layout_current) 
			this_->layout_current=attr->u.layout;
		break;
	case attr_route:
		this_->route=attr->u.route;
		break;
	case attr_mapset:
		this_->mapsets = g_list_append(this_->mapsets, attr->u.mapset);
		break;
	case attr_navigation:
		this_->navigation=attr->u.navigation;
		break;
	case attr_recent_dest:
		this_->recentdest_count = attr->u.num;
		break;
	case attr_speech:
		this_->speech=attr->u.speech;
		break;
	case attr_tracking:
		this_->tracking=attr->u.tracking;
		break;
	case attr_vehicle:
		ret=navit_add_vehicle(this_, attr->u.vehicle);
		break;
	case attr_vehicleprofile:
		this_->vehicleprofiles=g_list_prepend(this_->vehicleprofiles, attr->u.vehicleprofile);
		break;
	case attr_autozoom_min:
		this_->autozoom_min = attr->u.num;
		break;
	default:
		return 0;
	}
	callback_list_call_attr_2(this_->attr_cbl, attr->type, this_, attr);
	return ret;
}

int
navit_remove_attr(struct navit *this_, struct attr *attr)
{
	int ret=1;
	switch (attr->type) {
	case attr_callback:
		navit_remove_callback(this_, attr->u.callback);
		break;
	default:
		return 0;
	}
	return ret;
}

struct attr_iter *
navit_attr_iter_new(void)
{
	return g_new0(struct attr_iter, 1);
}

void
navit_attr_iter_destroy(struct attr_iter *iter)
{
	g_free(iter);
}

void
navit_add_callback(struct navit *this_, struct callback *cb)
{
	callback_list_add(this_->attr_cbl, cb);
}

void
navit_remove_callback(struct navit *this_, struct callback *cb)
{
	callback_list_remove(this_->attr_cbl, cb);
}

/**
 * Toggle the cursor update : refresh the map each time the cursor has moved (instead of only when it reaches a border)
 *
 * @param navit The navit instance
 * @returns nothing
 */

static void
navit_vehicle_draw(struct navit *this_, struct navit_vehicle *nv, struct point *pnt)
{
	struct point cursor_pnt;
	enum projection pro;

	if (this_->blocked)
		return;
	if (pnt)
		cursor_pnt=*pnt;
	else {
		pro=transform_get_projection(this_->trans);
		transform(this_->trans, pro, &nv->coord, &cursor_pnt, 1, 0, 0, NULL);
	}
	vehicle_draw(nv->vehicle, this_->gra, &cursor_pnt, pnt ? 0:1, nv->dir-transform_get_yaw(this_->trans), nv->speed);
#if 0	
	if (pnt)
		pnt2=*pnt;
	else {
		pro=transform_get_projection(this_->trans);
		transform(this_->trans, pro, &nv->coord, &pnt2, 1);
	}
#if 1
	cursor_draw(nv->cursor, &pnt2, nv->dir-transform_get_angle(this_->trans, 0), nv->speed > 2, pnt == NULL);
#else
	cursor_draw(nv->cursor, &pnt2, nv->dir-transform_get_angle(this_->trans, 0), nv->speed > 2, 1);
#endif
#endif
}

static void
navit_vehicle_update(struct navit *this_, struct navit_vehicle *nv)
{
	struct attr attr_valid, attr_dir, attr_speed, attr_pos;
	struct pcoord cursor_pc;
	struct point cursor_pnt, *pnt=&cursor_pnt;
	struct tracking *tracking=NULL;
	enum projection pro=transform_get_projection(this_->trans);
	int border=16;
	int (*get_attr)(void *, enum attr_type, struct attr *, struct attr_iter *);
	void *attr_object;

	profile(0,NULL);
	if (this_->ready != 3) {
		profile(0,"return 1\n");
		return;
	}
	navit_layout_switch(this_);
	if (this_->vehicle == nv && this_->tracking_flag)
		tracking=this_->tracking;
	if (tracking) {
		tracking_update(tracking, nv->vehicle, this_->vehicleprofile, pro);
		attr_object=tracking;
		get_attr=(int (*)(void *, enum attr_type, struct attr *, struct attr_iter *))tracking_get_attr;
	} else {
		attr_object=nv->vehicle;
		get_attr=(int (*)(void *, enum attr_type, struct attr *, struct attr_iter *))vehicle_get_attr;
	}
	if (get_attr(attr_object, attr_position_valid, &attr_valid, NULL))
		if (!attr_valid.u.num != attr_position_valid_invalid)
			return;
	if (! get_attr(attr_object, attr_position_direction, &attr_dir, NULL) ||
	    ! get_attr(attr_object, attr_position_speed, &attr_speed, NULL) ||
	    ! get_attr(attr_object, attr_position_coord_geo, &attr_pos, NULL)) {
		profile(0,"return 2\n");
		return;
	}
	nv->dir=*attr_dir.u.numd;
	nv->speed=*attr_speed.u.numd;
	transform_from_geo(pro, attr_pos.u.coord_geo, &nv->coord);
	if (nv != this_->vehicle) {
		navit_vehicle_draw(this_, nv, NULL);
		profile(0,"return 3\n");
		return;
	}
	cursor_pc.x = nv->coord.x;
	cursor_pc.y = nv->coord.y;
	cursor_pc.pro = pro;
	if (this_->route) {
		if (tracking)
			route_set_position_from_tracking(this_->route, tracking, pro);
		else
			route_set_position(this_->route, &cursor_pc);
	}
	callback_list_call_attr_0(this_->attr_cbl, attr_position);
	navit_textfile_debug_log(this_, "type=trackpoint_tracked");
	if (this_->gui && nv->speed > 2)
		gui_disable_suspend(this_->gui);

	transform(this_->trans, pro, &nv->coord, &cursor_pnt, 1, 0, 0, NULL);
	if (this_->button_pressed != 1 && this_->follow_cursor && nv->follow_curr <= nv->follow && 
		(nv->follow_curr == 1 || !transform_within_border(this_->trans, &cursor_pnt, border)))
		navit_set_center_cursor(this_);
	else
		navit_vehicle_draw(this_, nv, pnt);

	if (nv->follow_curr > 1)
		nv->follow_curr--;
	else
		nv->follow_curr=nv->follow;
	callback_list_call_attr_2(this_->attr_cbl, attr_position_coord_geo, this_, nv->vehicle);

	/* Finally, if we reached our destination, stop navigation. */
	if (this_->route && route_destination_reached(this_->route)) {
		navit_set_destination(this_, NULL, NULL, 0);
	}
	profile(0,"return 5\n");
}

/**
 * Set the position of the vehicle
 *
 * @param navit The navit instance
 * @param c The coordinate to set as position
 * @returns nothing
 */

void
navit_set_position(struct navit *this_, struct pcoord *c)
{
	if (this_->route) {
		route_set_position(this_->route, c);
		callback_list_call_attr_0(this_->attr_cbl, attr_position);
	}
	if (this_->ready == 3)
		navit_draw(this_);
}

static int
navit_set_vehicleprofile(struct navit *this_, char *name)
{
	struct attr attr;
	GList *l;
	l=this_->vehicleprofiles;
	while (l) {
		if (vehicleprofile_get_attr(l->data, attr_name, &attr, NULL)) {
			if (!strcmp(attr.u.str, name)) {
				this_->vehicleprofile=l->data;
				if (this_->route)
					route_set_profile(this_->route, this_->vehicleprofile);
				return 1;
			}
		}
		l=g_list_next(l);
	}
	return 0;
}

static void
navit_set_vehicle(struct navit *this_, struct navit_vehicle *nv)
{
	struct attr attr;
	this_->vehicle=nv;
	if (nv && vehicle_get_attr(nv->vehicle, attr_profilename, &attr, NULL)) {
		if (navit_set_vehicleprofile(this_, attr.u.str))
			return;
	}
	navit_set_vehicleprofile(this_,"car");
}

/**
 * Register a new vehicle
 *
 * @param navit The navit instance
 * @param v The vehicle instance
 * @returns 1 for success
 */
static int
navit_add_vehicle(struct navit *this_, struct vehicle *v)
{
	struct navit_vehicle *nv=g_new0(struct navit_vehicle, 1);
	struct attr follow, active, animate;
	nv->vehicle=v;
	nv->follow=0;
	nv->last.x = 0;
	nv->last.y = 0;
	nv->animate_cursor=0;
	if ((vehicle_get_attr(v, attr_follow, &follow, NULL)))
		nv->follow=nv->follow=follow.u.num;
	nv->follow_curr=nv->follow;
	this_->vehicles=g_list_append(this_->vehicles, nv);
	if ((vehicle_get_attr(v, attr_active, &active, NULL)) && active.u.num)
		navit_set_vehicle(this_, nv);
	if ((vehicle_get_attr(v, attr_animate, &animate, NULL)))
		nv->animate_cursor=animate.u.num;
	nv->callback.type=attr_callback;
	nv->callback.u.callback=callback_new_attr_2(callback_cast(navit_vehicle_update), attr_position_coord_geo, this_, nv);
	vehicle_add_attr(nv->vehicle, &nv->callback);
	vehicle_set_attr(nv->vehicle, &this_->self);
	return 1;
}




struct gui *
navit_get_gui(struct navit *this_)
{
	return this_->gui;
}

struct transformation *
navit_get_trans(struct navit *this_)
{
	return this_->trans;
}

struct route *
navit_get_route(struct navit *this_)
{
	return this_->route;
}

struct navigation *
navit_get_navigation(struct navit *this_)
{
	return this_->navigation;
}

struct displaylist *
navit_get_displaylist(struct navit *this_)
{
	return this_->displaylist;
}

void
navit_layout_switch(struct navit *n) 
{

    int currTs=0;
    struct attr iso8601_attr,geo_attr,valid_attr,layout_attr;
    double trise,tset,trise_actual;
    struct layout *l;
    int year, month, day;
    
    if (navit_get_attr(n,attr_layout,&layout_attr,NULL)!=1) {
	return; //No layout - nothing to switch
    }
    l=layout_attr.u.layout;
    
    if (l->dayname || l->nightname) {
	//Ok, we know that we have profile to switch
	
	//Check that we aren't calculating too fast
	if (vehicle_get_attr(n->vehicle->vehicle, attr_position_time_iso8601,&iso8601_attr,NULL)==1) {
		currTs=iso8601_to_secs(iso8601_attr.u.str);
	}
	if (currTs-(n->prevTs)<60) {
	    //We've have to wait a little
	    return;
	}
	if (sscanf(iso8601_attr.u.str,"%d-%02d-%02dT",&year,&month,&day) != 3)
		return;
	if (vehicle_get_attr(n->vehicle->vehicle, attr_position_coord_geo,&geo_attr,NULL)!=1) {
		//No position - no sun
		return;
	}
	if (vehicle_get_attr(n->vehicle->vehicle, attr_position_valid, &valid_attr,NULL) && valid_attr.u.num==attr_position_valid_invalid) {
		return; //No valid fix yet
	}
	
	//We calculate sunrise anyway, cause it is need both for day and for night
        if (__sunriset__(year,month,day,geo_attr.u.coord_geo->lat,geo_attr.u.coord_geo->lng,35,1,&trise,&tset)!=0) {
		//near the pole sun never rises/sets, so we should never switch profiles
		n->prevTs=currTs;
		return;
	    }
	
        trise_actual=trise;
	
	if (l->dayname) {
	
	    if ((HOURS(trise)*60+MINUTES(trise)==(currTs%86400)/60) || 
		    (n->prevTs==0 && ((HOURS(trise)*60+MINUTES(trise)<(currTs%86400)/60)))) {
		//The sun is rising now!
		if (strcmp(l->name,l->dayname)) {
		    navit_set_layout_by_name(n,l->dayname);
		}
	    }
	}
	if (l->nightname) {
	    if (__sunriset__(year,month,day,geo_attr.u.coord_geo->lat,geo_attr.u.coord_geo->lng,-12,0,&trise,&tset)!=0) {
		//near the pole sun never rises/sets, so we should never switch profiles
		n->prevTs=currTs;
		return;
	    }
	    
	    if (HOURS(tset)*60+MINUTES(tset)==((currTs%86400)/60)
		|| (n->prevTs==0 && (((HOURS(tset)*60+MINUTES(tset)<(currTs%86400)/60)) || 
			((HOURS(trise_actual)*60+MINUTES(trise_actual)>(currTs%86400)/60))))) {
		//Time to sleep
		if (strcmp(l->name,l->nightname)) {
		    navit_set_layout_by_name(n,l->nightname);
		}
	    }	
	}
	
	n->prevTs=currTs;
    }
}

int 
navit_set_layout_by_name(struct navit *n,char *name) 
{
    struct layout *l;
    struct attr_iter iter;
    struct attr layout_attr;

    iter.u.list=0x00;

    if (navit_get_attr(n,attr_layout,&layout_attr,&iter)!=1) {
	return 0; //No layouts - nothing to do
    }
    if (iter.u.list==NULL) {
	return 0;
    }
    
    iter.u.list=g_list_first(iter.u.list);
    
    while(iter.u.list) {
	l=(struct layout*)iter.u.list->data;
	if (!strcmp(name,l->name)) {
	    layout_attr.u.layout=l;
	    layout_attr.type=attr_layout;
	    navit_set_attr(n,&layout_attr);
	    iter.u.list=g_list_first(iter.u.list);
	    return 1;
	}
	iter.u.list=g_list_next(iter.u.list);
    }

    iter.u.list=g_list_first(iter.u.list);
    return 0;
}

int
navit_block(struct navit *this_, int block)
{
	if (block) {
		this_->blocked |= 1;
		if (graphics_draw_cancel(this_->gra, this_->displaylist))
			this_->blocked |= 2;
		return 0;
	}
	if (this_->blocked & 2) {
		this_->blocked=0;
		navit_draw(this_);
		return 1;
	}
	this_->blocked=0;
	return 0;
}

void
navit_destroy(struct navit *this_)
{
	/* TODO: destroy objects contained in this_ */
	if (this_->vehicle)
		vehicle_destroy(this_->vehicle->vehicle);
	char *center_file = navit_get_center_file(TRUE);
	navit_write_center_to_file(this_, center_file);
	g_free(center_file);
	callback_destroy(this_->nav_speech_cb);
	callback_destroy(this_->roadbook_callback);
	callback_destroy(this_->popup_callback);
	callback_destroy(this_->motion_timeout_callback);
	if(this_->gra)
	  graphics_remove_callback(this_->gra, this_->resize_callback);
	callback_destroy(this_->resize_callback);
	if(this_->gra)
	  graphics_remove_callback(this_->gra, this_->button_callback);
	callback_destroy(this_->button_callback);
	if(this_->gra)
	  graphics_remove_callback(this_->gra, this_->motion_callback);
	callback_destroy(this_->motion_callback);
	g_free(this_);
}

/** @} */
