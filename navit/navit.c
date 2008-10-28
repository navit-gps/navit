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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <glib.h>
#include <libintl.h>
#include <math.h>
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
#include "cursor.h"
#include "popup.h"
#include "data_window.h"
#include "route.h"
#include "navigation.h"
#include "speech.h"
#include "track.h"
#include "vehicle.h"
#include "color.h"
#include "layout.h"
#include "log.h"
#include "attr.h"

#define _(STRING)    gettext(STRING)
/**
 * @defgroup navit the navit core instance. navit is the object containing nearly everything: A set of maps, one or more vehicle, a graphics object for rendering the map, a gui object for displaying the user interface, a route object, a navigation object and so on. Be warned that it is theoretically possible to have more than one navit object
 * @{
 */

//! The navit_vehicule
struct navit_vehicle {
	int update;
	/*! Limit of the update counter. See navit_add_vehicle */
	int update_curr;
	/*! Deprecated : Update counter itself. When it reaches 'update' counts, route is updated */
	int follow;
	/*! Limit of the follow counter. See navit_add_vehicle */
	int follow_curr;
	/*! Deprecated : follow counter itself. When it reaches 'update' counts, map is recentered*/
	struct coord coord;
	int dir;
	int speed;
	struct color c;
	struct color *c2;
	struct cursor *cursor;
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
	int cursor_flag;
	int tracking_flag;
	int orient_north_flag;
	int recentdest_count;
	GList *vehicles;
	GList *windows_items;
	struct navit_vehicle *vehicle;
	struct callback_list *attr_cbl;
	int pid;
	struct callback *nav_speech_cb;
	struct callback *roadbook_callback;
	struct callback *popup_callback;
	struct datawindow *roadbook_window;
	struct map *bookmark;
	struct map *former_destination;
	GHashTable *bookmarks_hash;
	struct point pressed, last, current;
	int button_pressed,moved,popped;
	struct event_timer *button_timeout, *motion_timeout;
	struct callback *motion_timeout_callback;
	int ignore_button;
	struct log *textfile_debug_log;
	struct pcoord destination;
	int destination_valid;
	int blocked;
	int w,h;
	int drag_bitmap;
	GHashTable *commands;
	struct callback *resize_callback,*button_callback,*motion_callback;
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
	exit(-1);
}

struct tracking *
navit_get_tracking(struct navit *this_)
{
	return this_->tracking;
}

void
navit_draw(struct navit *this_)
{
	GList *l;
	struct navit_vehicle *nv;

	if (this_->blocked) {
		this_->blocked |= 2;
		return;
	}
	transform_setup_source_rect(this_->trans);
	graphics_draw(this_->gra, this_->displaylist, this_->mapsets, this_->trans, this_->layout_current);
	l=this_->vehicles;
	while (l) {
		nv=l->data;
		navit_vehicle_draw(this_, nv, NULL);
		l=g_list_next(l);
	}
}

void
navit_draw_displaylist(struct navit *this_)
{
	if (this_->ready == 3)
		graphics_displaylist_draw(this_->gra, this_->displaylist, this_->trans, this_->layout_current, 1);
}

void
navit_resize(void *data, int w, int h)
{
	struct navit *this_=data;
	struct map_selection sel;
	memset(&sel, 0, sizeof(sel));
	sel.u.p_rect.rl.x=w;
	sel.u.p_rect.rl.y=h;
	this_->w=w;
	this_->h=h;
	transform_set_screen_selection(this_->trans, &sel);
	this_->ready |= 2;
	if (this_->ready == 3)
		navit_draw(this_);
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
	this_->ignore_button=1;
}

int
navit_handle_button(struct navit *this_, int pressed, int button, struct point *p, struct callback *popup_callback)
{
	int border=16;

	callback_list_call_attr_4(this_->attr_cbl, attr_button, this_, pressed, button, p);
	if (this_->ignore_button) {
		this_->ignore_button=0;
		return 0;
	}
	if (pressed) {
		this_->pressed=*p;
		this_->last=*p;
		if (button == 1) {
			this_->button_pressed=1;
			this_->moved=0;
			this_->popped=0;
			if (popup_callback)
				this_->button_timeout=event_add_timeout(500, 0, popup_callback);
		}
		if (button == 2)
			navit_set_center_screen(this_, p);
		if (button == 3)
			popup(this_, button, p);
		if (button == 4)
			navit_zoom_in(this_, 2, p);
		if (button == 5)
			navit_zoom_out(this_, 2, p);
	} else {
		this_->button_pressed=0;
		if (this_->button_timeout) {
			event_remove_timeout(this_->button_timeout);
			this_->button_timeout=NULL;
			if (! this_->moved && ! transform_within_border(this_->trans, p, border))
				navit_set_center_screen(this_, p);

		}
		if (this_->motion_timeout) {
			event_remove_timeout(this_->motion_timeout);
			this_->motion_timeout=NULL;
		}
		if (this_->moved) {
			struct point pt;
			this_->last=*p;
			transform_get_size(this_->trans, &pt.x, &pt.y);
			pt.x/=2;
			pt.y/=2;
			pt.x-=this_->last.x-this_->pressed.x;
			pt.y-=this_->last.y-this_->pressed.y;
			graphics_draw_drag(this_->gra, NULL);
			graphics_overlay_disable(this_->gra, 0);
			navit_set_center_screen(this_, &pt);
		} else
			return 1;
	}
	return 0;
}

static void
navit_button(void *data, int pressed, int button, struct point *p)
{
	struct navit *this=data;
	if (! this->popup_callback)
		this->popup_callback=callback_new_1(navit_popup, this);
	navit_handle_button(this, pressed, button, p, this->popup_callback);
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
		this_->last=this_->current;
		graphics_overlay_disable(this_->gra, 1);
		graphics_displaylist_move(this_->displaylist, dx, dy);
		graphics_displaylist_draw(this_->gra, this_->displaylist, this_->trans, this_->layout_current, 0);
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
	navit_handle_motion((struct navit *)data, p);
}

static void
navit_scale(struct navit *this_, long scale, struct point *p)
{
	struct coord c1, c2, *center;
	if (p)
		transform_reverse(this_->trans, p, &c1);
	transform_set_scale(this_->trans, scale);
	if (p) {
		transform_reverse(this_->trans, p, &c2);
		center = transform_center(this_->trans);
		center->x += c1.x - c2.x;
		center->y += c1.y - c2.y;
	}
	navit_draw(this_);
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
	navit_scale(this_, scale, p);
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
	navit_scale(this_, scale, p);
}

struct navit *
navit_new(struct attr *parent, struct attr **attrs)
{
	struct navit *this_=g_new0(struct navit, 1);
	struct pcoord center;
	struct coord co;
	struct coord_geo g;
	enum projection pro=projection_mg;
	int zoom = 256;
	FILE *f;
	g.lat=53.13;
	g.lng=11.70;

	main_add_navit(this_);
	this_->self.type=attr_navit;
	this_->self.u.navit=this_;
	this_->attr_cbl=callback_list_new();

#if !defined(_WIN32) && !defined(__CEGCC__)
	f=popen("pidof /usr/bin/ipaq-sleep","r");
	if (f) {
		fscanf(f,"%d",&this_->pid);
		dbg(1,"ipaq_sleep pid=%d\n", this_->pid);
		pclose(f);
	}
#endif

	this_->bookmarks_hash=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	this_->cursor_flag=1;
	this_->orient_north_flag=0;
	this_->tracking_flag=1;
	this_->recentdest_count=10;

	for (;*attrs; attrs++) {
		switch((*attrs)->type) {
		case attr_zoom:
			zoom=(*attrs)->u.num;
			break;
		case attr_center:
			g=*((*attrs)->u.coord_geo);
			break;
		case attr_cursor:
			this_->cursor_flag=!!(*attrs)->u.num;
			break;
		case attr_orientation:
			this_->orient_north_flag=!!(*attrs)->u.num;
			break;
		case attr_tracking:
			this_->tracking_flag=!!(*attrs)->u.num;
			break;
		case attr_recent_dest:
			this_->recentdest_count=(*attrs)->u.num;
			break;
		case attr_drag_bitmap:
			this_->drag_bitmap=!!(*attrs)->u.num;
			break;
		default:
			dbg(0, "Unexpected attribute %x\n",(*attrs)->type);
			break;
		}
	}
	transform_from_geo(pro, &g, &co);
	center.x=co.x;
	center.y=co.y;
	center.pro = pro;

	this_->trans=transform_new();
	transform_setup(this_->trans, &center, zoom, 0);
	this_->displaylist=graphics_displaylist_new();
	this_->commands=g_hash_table_new(g_str_hash, g_str_equal);
	navit_command_register(this_, "zoom_in", callback_new_3(navit_zoom_in, this_, 2, NULL));
	navit_command_register(this_, "zoom_out", callback_new_3(navit_zoom_out, this_, 2, NULL));
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

static int
navit_set_graphics(struct navit *this_, struct graphics *gra)
{
	if (this_->gra)
		return 0;
	this_->gra=gra;
	this_->resize_callback=callback_new_attr_1(navit_resize, attr_resize, this_);
	graphics_add_callback(gra, this_->resize_callback);
	this_->button_callback=callback_new_attr_1(navit_button, attr_button, this_);
	graphics_add_callback(gra, this_->button_callback);
	this_->motion_callback=callback_new_attr_1(navit_motion, attr_motion, this_);
	graphics_add_callback(gra, this_->motion_callback);
	return 1;
}

struct graphics *
navit_get_graphics(struct navit *this_)
{
	return this_->gra;
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
navit_append_coord(struct navit *this_, char *file, struct pcoord *c, char *type, char *description, GHashTable *h, int limit)
{
	FILE *f;
	int offset=0;
	char *buffer;
	int ch,prev,lines=0;
	int numc,readc;
	int fd;
	const char *prostr;
	struct callback *cb;

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
			prostr = projection_to_name(c->pro);
			fprintf(f,"%s%s%s0x%x %s0x%x type=%s label=\"%s\"\n",
				 prostr, *prostr ? ":" : "", 
				 c->x >= 0 ? "":"-", c->x >= 0 ? c->x : -c->x, 
				 c->y >= 0 ? "":"-", c->y >= 0 ? c->y : -c->y, 
				 type, description);
		} else
			fprintf(f,"\n");
		fclose(f);
	}
}

static int
parse_line(FILE *f, char *buffer, char **name, struct pcoord *c)
{
	int pos;
	char *s,*i;
	struct coord co;
	char *cp;
	enum projection pro = projection_mg;
	*name=NULL;
	if (! fgets(buffer, 2048, f))
		return -3;
	cp = buffer;
	pos=coord_parse(cp, pro, &co);
	if (!pos)
		return -2;
	if (!cp[pos] || cp[pos] == '\n')
		return -1;
	cp[strlen(cp)-1]='\0';
	s=cp+pos+1;
	if (!strncmp(s,"type=", 5)) {
		i=strchr(s, '"');
		if (i) {
			s=i+1;
			i=strchr(s, '"');
			if (i)
				*i='\0';
		}
	}
	*name=s;
	c->x = co.x;
	c->y = co.y;
	c->pro = pro;
	return pos;
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
			perror(dir);
			return;
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
	return g_strjoin(NULL, navit_get_user_data_directory(create), "destination.txt", NULL);
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
	return g_strjoin(NULL, navit_get_user_data_directory(create), "bookmark.txt", NULL);
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
	return g_strjoin(NULL, navit_get_user_data_directory(create), "center.txt", NULL);
}

static void
navit_set_center_from_file(struct navit *this_, char *file)
{
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
navit_set_destination(struct navit *this_, struct pcoord *c, char *description)
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
		route_set_destination(this_->route, c);
		if (this_->navigation) {
			navigation_flush(this_->navigation);
			navigation_update(this_->navigation, this_->route);
		}

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
		if(!fseek(f, -2, SEEK_END) && fread(buffer, 2, 1, f) == 1 && buffer[0]!='\n' || buffer[1]!='\n') 
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
	if (valid) 
		route_set_destination(this_->route, &pc);
}


static void
navit_textfile_debug_log(struct navit *this_, const char *fmt, ...)
{
	va_list ap;
	char *str1,*str2;
	va_start(ap, fmt);
	if (this_->textfile_debug_log && this_->vehicle) {
		str1=g_strdup_vprintf(fmt, ap);
		str2=g_strdup_printf("0x%x 0x%x%s%s\n", this_->vehicle->coord.x, this_->vehicle->coord.y, strlen(str1) ? " " : "", str1);
		log_write(this_->textfile_debug_log, str2, strlen(str2));
		g_free(str2);
		g_free(str1);
	}
       	va_end(ap);
}

void
navit_say(struct navit *this_, char *text)
{
	speech_say(this_->speech, text);
}

void
navit_speak(struct navit *this_)
{
	struct navigation *nav=this_->navigation;
	struct map *map=NULL;
	struct map_rect *mr=NULL;
	struct item *item;
	struct attr attr;

	if (nav)
		map=navigation_get_map(nav);
	if (map)
		mr=map_rect_new(map, NULL);
	if (mr) {
		item=map_rect_get_item(mr);
		if (item && item_attr_get(item, attr_navigation_speech, &attr)) {
			speech_say(this_->speech, attr.u.str);
			navit_textfile_debug_log(this_, "item=point_debug debug=\"speech_say('%s')\"", attr.u.str);
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
			item_attr_get(item, attr_navigation_long, &attr);
			dbg(2, "Command='%s'\n", attr.u.str);
			param[0].name=_("Command");
			param[0].value=g_strdup(attr.u.str);

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
	if (this_->roadbook_callback || this_->roadbook_window) {
		return;
	}

	this_->roadbook_callback=callback_new_1(callback_cast(navit_window_roadbook_update), this_);
	navigation_register_callback(this_->navigation, attr_navigation_long, this_->roadbook_callback);
	this_->roadbook_window=gui_datawindow_new(this_->gui, _("Roadbook"), NULL, callback_new_1(callback_cast(navit_window_roadbook_destroy), this_));
	navit_window_roadbook_update(this_);
}

static void
get_direction(char *buffer, int angle, int mode)
{
	angle=angle%360;
	switch (mode) {
	case 0:
		sprintf(buffer,"%d",angle);
		break;
	case 1:
		if (angle < 69 || angle > 291)
			*buffer++='N';
		if (angle > 111 && angle < 249)
			*buffer++='S';
		if (angle > 22 && angle < 158)
			*buffer++='E';
		if (angle > 202 && angle < 338)
			*buffer++='W';
		*buffer++='\0';
		break;
	case 2:
		angle=(angle+15)/30;
		if (! angle)
			angle=12;
		sprintf(buffer,"%d H", angle);
		break;
	}
}

struct navit_window_items {
	struct datawindow *win;
	struct callback *click;
	char *name;
	int distance;
	GHashTable *hash;
	GList *list;
};

static void
navit_window_items_click(struct navit *this_, struct navit_window_items *nwi, char **col)
{
	struct pcoord c;
	char *description;

	// FIXME
	dbg(0,"enter col=%s,%s,%s,%s,%s\n", col[0], col[1], col[2], col[3], col[4]);
	sscanf(col[4], "0x%x,0x%x", &c.x, &c.y);
	c.pro = projection_mg;
	dbg(0,"0x%x,0x%x\n", c.x, c.y);
	description=g_strdup_printf("%s %s", nwi->name, col[3]);
	navit_set_destination(this_, &c, description);
	g_free(description);
}

static void
navit_window_items_open(struct navit *this_, struct navit_window_items *nwi)
{
	struct map_selection sel;
	struct coord c,*center;
	struct mapset_handle *h;
	struct map *m;
	struct map_rect *mr;
	struct item *item;
	struct attr attr;
	int idist,dist;
	struct param_list param[5];
	char distbuf[32];
	char dirbuf[32];
	char coordbuf[64];

	dbg(0, "distance=%d\n", nwi->distance);
	if (nwi->distance == -1)
		dist=40000000;
	else
		dist=nwi->distance*1000;
	param[0].name="Distance";
	param[1].name="Direction";
	param[2].name="Type";
	param[3].name="Name";
	param[4].name=NULL;
	sel.next=NULL;
#if 0
	sel.order[layer_town]=18;
	sel.order[layer_street]=18;
	sel.order[layer_poly]=18;
#else
	sel.order[layer_town]=0;
	sel.order[layer_street]=0;
	sel.order[layer_poly]=0;
#endif
	center=transform_center(this_->trans);
	sel.u.c_rect.lu.x=center->x-dist;
	sel.u.c_rect.lu.y=center->y+dist;
	sel.u.c_rect.rl.x=center->x+dist;
	sel.u.c_rect.rl.y=center->y-dist;
	dbg(2,"0x%x,0x%x - 0x%x,0x%x\n", sel.u.c_rect.lu.x, sel.u.c_rect.lu.y, sel.u.c_rect.rl.x, sel.u.c_rect.rl.y);
	nwi->click=callback_new_2(callback_cast(navit_window_items_click), this_, nwi);
	nwi->win=gui_datawindow_new(this_->gui, nwi->name, nwi->click, NULL);
	h=mapset_open(navit_get_mapset(this_));
        while ((m=mapset_next(h, 1))) {
#if 0
		dbg(2,"m=%p %s\n", m, map_get_filename(m));
#endif
		mr=map_rect_new(m, &sel);
		dbg(2,"mr=%p\n", mr);
		while ((item=map_rect_get_item(mr))) {
			if (item_coord_get(item, &c, 1)) {
				if (coord_rect_contains(&sel.u.c_rect, &c) && g_hash_table_lookup(nwi->hash, &item->type)) {
					if (! item_attr_get(item, attr_label, &attr))
						attr.u.str="";
					idist=transform_distance(map_projection(item->map), center, &c);
					if (idist < dist) {
						get_direction(dirbuf, transform_get_angle_delta(center, &c, 0), 1);
						param[0].value=distbuf;
						param[1].value=dirbuf;
						param[2].value=item_to_name(item->type);
						sprintf(distbuf,"%d", idist/1000);
						param[3].value=attr.u.str;
						sprintf(coordbuf, "0x%x,0x%x", c.x, c.y);
						param[4].value=coordbuf;
						datawindow_add(nwi->win, param, 5);
					}
					/* printf("gefunden %s %s %d\n",item_to_name(item->type), attr.u.str, idist/1000); */
				}
				if (item->type >= type_line)
					while (item_coord_get(item, &c, 1));
			}
		}
		map_rect_destroy(mr);
	}
	mapset_close(h);
}

struct navit_window_items *
navit_window_items_new(const char *name, int distance)
{
	struct navit_window_items *nwi=g_new0(struct navit_window_items, 1);
	nwi->name=g_strdup(name);
	nwi->distance=distance;
	nwi->hash=g_hash_table_new(g_int_hash, g_int_equal);

	return nwi;
}

void
navit_window_items_add_item(struct navit_window_items *nwi, enum item_type type)
{
	nwi->list=g_list_prepend(nwi->list, (void *)type);
	g_hash_table_insert(nwi->hash, &nwi->list->data, (void *)1);
}

void
navit_add_window_items(struct navit *this_, struct navit_window_items *nwi)
{
	this_->windows_items=g_list_append(this_->windows_items, nwi);
}

static void
navit_add_menu_windows_items(struct navit *this_, struct menu *men)
{
	struct navit_window_items *nwi;
	struct callback *cb;
	GList *l;
	l=this_->windows_items;
	while (l) {
		nwi=l->data;
		cb=callback_new_2(callback_cast(navit_window_items_open), this_, nwi);
		menu_add(men, nwi->name, menu_type_menu, cb);
		l=g_list_next(l);
	}
}

void
navit_init(struct navit *this_)
{
	struct mapset *ms;
	struct map *map;
	GList *l;
	struct navit_vehicle *nv;

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
	graphics_init(this_->gra);
	l=this_->vehicles;
	while (l) {
		dbg(1,"parsed one vehicle\n");
		nv=l->data;
		nv->cursor=cursor_new(this_->gra, &nv->c, nv->c2, nv->animate_cursor);
		nv->callback.type=attr_callback;
		nv->callback.u.callback=callback_new_2(callback_cast(navit_vehicle_update), this_, nv);
		vehicle_add_attr(nv->vehicle, &nv->callback);
		vehicle_set_attr(nv->vehicle, &this_->self, NULL);
		l=g_list_next(l);
	}
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
	if (this_->navigation && this_->speech) {
		this_->nav_speech_cb=callback_new_1(callback_cast(navit_speak), this_);
		navigation_register_callback(this_->navigation, attr_navigation_speech, this_->nav_speech_cb);
	}
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
	callback_list_call_attr_1(this_->attr_cbl, attr_navit, this_);
	this_->ready|=1;
	if (this_->ready == 3)
		navit_draw(this_);
}

/**
 * Change the current zoom level
 *
 * @param navit The navit instance
 * @param center The point where to center the map, including its projection
 * @returns nothing
 */
void
navit_set_center(struct navit *this_, struct pcoord *center)
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
	if (this_->ready == 3)
		navit_draw(this_);
}

static void
navit_set_center_cursor(struct navit *this_, struct coord *cursor, int dir, int xpercent, int ypercent)
{
	struct coord *c=transform_center(this_->trans);
	int width, height;
	struct point p;
	struct coord cnew;

	transform_get_size(this_->trans, &width, &height);
	*c=*cursor;
	transform_set_angle(this_->trans, dir);
	p.x=(100-xpercent)*width/100;
	p.y=(100-ypercent)*height/100;
	transform_reverse(this_->trans, &p, &cnew);
	*c=cnew;
	if (this_->ready == 3)
		navit_draw(this_);
}


void
navit_set_center_screen(struct navit *this_, struct point *p)
{
	struct coord c;
	struct pcoord pc;
	transform_reverse(this_->trans, p, &c);
	pc.x = c.x;
	pc.y = c.y;
	pc.pro = transform_get_projection(this_->trans);
	navit_set_center(this_, &pc);
}

int
navit_set_attr(struct navit *this_, struct attr *attr)
{
	int dir=0, orient_old=0, attr_updated=0;

	switch (attr->type) {
	case attr_cursor:
		if (this_->cursor_flag != !!attr->u.num) {
			this_->cursor_flag=!!attr->u.num;
			attr_updated=1;
		}
		break;
	case attr_layout:
		if(this_->layout_current!=attr->u.layout) {
			this_->layout_current=attr->u.layout;
			graphics_font_destroy_all(this_->gra);
			navit_draw(this_);
			attr_updated=1;
		}
		break;
	case attr_orientation:
		orient_old=this_->orient_north_flag;
		this_->orient_north_flag=!!attr->u.num;
		if (this_->orient_north_flag) {
			dir = 0;
		} else {
			if (this_->vehicle) {
				dir = this_->vehicle->dir;
			}
		}
		transform_set_angle(this_->trans, dir);
		if (orient_old != this_->orient_north_flag) {
			navit_draw(this_);
			attr_updated=1;
		}
		break;
	case attr_projection:
		if(this_->trans && transform_get_projection(this_->trans) != attr->u.projection) {
			navit_projection_set(this_, attr->u.projection);
			attr_updated=1;
		}
		break;
	case attr_tracking:
		if (this_->tracking_flag != !!attr->u.num) {
			this_->tracking_flag=!!attr->u.num;
			attr_updated=1;
		}
		break;
	case attr_vehicle:
		if (!this_->vehicle || this_->vehicle->vehicle != attr->u.vehicle) {
			GList *l;
			l=this_->vehicles;
			while(l) {
				if (((struct navit_vehicle *)l->data)->vehicle == attr->u.vehicle) {
					this_->vehicle=(struct navit_vehicle *)l->data;
					attr_updated=1;
				}
				l=g_list_next(l);
			}
		}
		break;
	default:
		return 0;
	}
	if (attr_updated) {
		callback_list_call_attr_2(this_->attr_cbl, attr->type, this_, attr);
	}
	return 1;
}

int
navit_get_attr(struct navit *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter)
{
	switch (type) {
	case attr_bookmark_map:
		attr->u.map=this_->bookmark;
		break;
	case attr_cursor:
		attr->u.num=this_->cursor_flag;
		break;
	case attr_destination:
		if (! this_->destination_valid)
			return 0;
		attr->u.pcoord=&this_->destination;
		break;
	case attr_former_destination_map:
		attr->u.map=this_->former_destination;
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
	case attr_orientation:
		attr->u.num=this_->orient_north_flag;
		break;
	case attr_projection:
		if(this_->trans) {
			attr->u.num=transform_get_projection(this_->trans);
		} else {
			return 0;
		}
		break;
	case attr_tracking:
		attr->u.num=this_->tracking_flag;
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
	default:
		return 0;
	}
	attr->type=type;
	return 1;
}

static int
navit_add_log(struct navit *this_, struct log *log)
{
	struct attr type_attr;
	if (!log_get_attr(log, attr_type, &type_attr, NULL))
		return 0;
	if (!strcmp(type_attr.u.str, "textfile_debug")) {
		if (this_->textfile_debug_log)
			return 0;
		this_->textfile_debug_log=log;
		return 1;
	}
	return 0;
}

int
navit_add_attr(struct navit *this_, struct attr *attr)
{
	switch (attr->type) {
	case attr_log:
		return navit_add_log(this_, attr->u.log);
	case attr_gui:
		return navit_set_gui(this_, attr->u.gui);
	case attr_graphics:
		return navit_set_graphics(this_, attr->u.graphics);
	case attr_layout:
		this_->layouts = g_list_append(this_->layouts, attr->u.layout);
		if(!this_->layout_current) 
			this_->layout_current=attr->u.layout;
		return 1;
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
		return navit_add_vehicle(this_, attr->u.vehicle);
	default:
		return 0;
	}
	return 1;
}

struct attr_iter *
navit_attr_iter_new()
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
	struct point pnt2;
	enum projection pro;
	if (this_->blocked)
		return;
	if (pnt)
		pnt2=*pnt;
	else {
		pro=transform_get_projection(this_->trans);
		transform(this_->trans, pro, &nv->coord, &pnt2, 1, 0);
	}
#if 1
	cursor_draw(nv->cursor, &pnt2, nv->dir-transform_get_angle(this_->trans, 0), nv->speed > 2, pnt == NULL);
#else
	cursor_draw(nv->cursor, &pnt2, nv->dir-transform_get_angle(this_->trans, 0), nv->speed > 2, 1);
#endif
}

static void
navit_vehicle_update(struct navit *this_, struct navit_vehicle *nv)
{
	struct attr attr_dir, attr_speed, attr_pos;
	struct pcoord cursor_pc;
	struct point cursor_pnt, *pnt=&cursor_pnt;
	enum projection pro;
	int border=16;
	int route_path_set=0;

	if (this_->ready != 3)
		return;

	if (! vehicle_get_attr(nv->vehicle, attr_position_direction, &attr_dir, NULL) ||
	    ! vehicle_get_attr(nv->vehicle, attr_position_speed, &attr_speed, NULL) ||
	    ! vehicle_get_attr(nv->vehicle, attr_position_coord_geo, &attr_pos, NULL))
		return;
	nv->dir=*attr_dir.u.numd;
	nv->speed=*attr_speed.u.numd;
	pro=transform_get_projection(this_->trans);
	transform_from_geo(pro, attr_pos.u.coord_geo, &nv->coord);
	if (nv != this_->vehicle) {
		navit_vehicle_draw(this_, nv, NULL);
		return;
	}
	if (this_->route)
		route_path_set=route_get_path_set(this_->route);
	if (this_->tracking && this_->tracking_flag) {
		if (tracking_update(this_->tracking, &nv->coord, nv->dir)) {
			if (this_->route && nv->update_curr == 1) {
				route_set_position_from_tracking(this_->route, this_->tracking);
				callback_list_call_attr_0(this_->attr_cbl, attr_position);
			}
		}
	} else {
		if (this_->route && nv->update_curr == 1) {
			cursor_pc.pro = pro;
			cursor_pc.x = nv->coord.x;
			cursor_pc.y = nv->coord.y;
			navit_set_position(this_, &cursor_pc);
		}
	}
	transform(this_->trans, pro, &nv->coord, &cursor_pnt, 1, 0);
	if (!transform_within_border(this_->trans, &cursor_pnt, border)) {
		if (!this_->cursor_flag)
			return;
		if (nv->follow_curr != 1) {
			if (this_->orient_north_flag)
				navit_set_center_cursor(this_, &nv->coord, 0, 50 - 30.*sin(M_PI*nv->dir/180.), 50 + 30.*cos(M_PI*nv->dir/180.));
			else
				navit_set_center_cursor(this_, &nv->coord, nv->dir, 50, 80);
			pnt=NULL;
		}
	}

#ifndef _WIN32
	if (this_->pid && nv->speed > 2)
		kill(this_->pid, SIGWINCH);
#endif
	if (this_->route && nv->update_curr == 1)
		navigation_update(this_->navigation, this_->route);
	if (this_->cursor_flag && nv->follow_curr == 1) {
		navit_set_center_cursor(this_, &nv->coord, nv->dir, 50, 80);
		pnt=NULL;
	}
	if (pnt && this_->route && !route_path_set && route_get_path_set(this_->route))
		navit_draw(this_);
	if (nv->follow_curr > 1)
		nv->follow_curr--;
	else
		nv->follow_curr=nv->follow;
	if (nv->update_curr > 1)
		nv->update_curr--;
	else
		nv->update_curr=nv->update;
	callback_list_call_attr_2(this_->attr_cbl, attr_position_coord_geo, this_, nv->vehicle);
	if (pnt)
		navit_vehicle_draw(this_, nv, pnt);

	/* Finally, if we reached our destination, stop navigation. */
	if (route_destination_reached(this_->route)) {
		navit_set_destination(this_, NULL, NULL);
	}
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
		if (this_->navigation) {
			navigation_update(this_->navigation, this_->route);
		}
	}
	navit_draw(this_);
}

static void
navit_set_vehicle(struct navit *this_, struct navit_vehicle *nv)
{
	this_->vehicle=nv;
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
	struct attr update,follow,color,active, color2, animate;
	nv->vehicle=v;
	nv->update=1;
	nv->follow=0;
	nv->animate_cursor=0;
	if ((vehicle_get_attr(v, attr_update, &update, NULL)))
		nv->update=nv->update=update.u.num;
	if ((vehicle_get_attr(v, attr_follow, &follow, NULL)))
		nv->follow=nv->follow=follow.u.num;
	if ((vehicle_get_attr(v, attr_color, &color, NULL)))
		nv->c=*(color.u.color);
	if ((vehicle_get_attr(v, attr_color2, &color2, NULL)))
		nv->c2=color2.u.color;
	else
		nv->c2=NULL;
	nv->update_curr=nv->update;
	nv->follow_curr=nv->follow;
	this_->vehicles=g_list_append(this_->vehicles, nv);
	if ((vehicle_get_attr(v, attr_active, &active, NULL)) && active.u.num)
		navit_set_vehicle(this_, nv);
	if ((vehicle_get_attr(v, attr_animate, &animate, NULL)))
		nv->animate_cursor=animate.u.num;
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

int
navit_block(struct navit *this_, int block)
{
	if (block) {
		this_->blocked |= 1;
		return;
	}
	if (this_->blocked & 2) {
		this_->blocked=0;
		navit_draw(this_);
		return 1;
	}
	this_->blocked=0;
	return 0;
}

int
navit_command_register(struct navit *this_, char *command, struct callback *cb)
{
	dbg(1,"registering '%s'\n", command);
	g_hash_table_insert(this_->commands, command, cb);

	return 0;
}

struct callback *
navit_command_unregister(struct navit *this_, char *command)
{
	struct callback *ret=g_hash_table_lookup(this_->commands, command);
	if (ret) {
		g_hash_table_remove(this_->commands, command);
	}

	return ret;
}

int
navit_command_call(struct navit *this_, char *command)
{
	struct callback *cb=g_hash_table_lookup(this_->commands, command);
	dbg(0,"calling callback %p for '%s'\n", cb, command);
	if (! cb)
		return 1;
	callback_call_1(cb, command);
}

void
navit_destroy(struct navit *this_)
{
	/* TODO: destroy objects contained in this_ */
	main_remove_navit(this_);
	char *center_file = navit_get_center_file(TRUE);
	navit_write_center_to_file(this_, center_file);
	g_free(center_file);
	callback_destroy(navit_command_unregister(this_, "zoom_in"));
	callback_destroy(navit_command_unregister(this_, "zoom_out"));
	g_hash_table_destroy(this_->commands);
	g_free(this_);
}

/** @} */
