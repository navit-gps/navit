#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <glib.h>
#include <libintl.h>
#include <math.h>
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

#define _(STRING)    gettext(STRING)
/**
 * @defgroup navit the navit core instance
 * @{
 */

struct navit_vehicle {
	char *name;
	int update;
	int update_curr;
	int follow;
	int follow_curr;
	struct coord coord;
	int dir;
	int speed;
	struct color c;
	struct menu *menu;
	struct cursor *cursor;
	struct vehicle *vehicle;
	struct attr callback;
};

struct navit {
	struct attr self;
	GList *mapsets;
	GList *layouts;
	struct gui *gui;
	char *gui_type;
	struct layout *layout_current;
	struct graphics *gra;
	char *gra_type;
	struct action *action;
	struct transformation *trans;
	struct compass *compass;
	struct menu *menu;
	struct menu *toolbar;
	struct statusbar *statusbar;
	struct menu *menubar;
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
	GList *vehicles;
	GList *windows_items;
	struct navit_vehicle *vehicle;
	struct callback_list *vehicle_cbl;
	struct callback_list *init_cbl;
	struct callback_list *attr_cbl;
	int pid;
	struct callback *nav_speech_cb;
	struct callback *roadbook_callback;
	struct datawindow *roadbook_window;
	struct menu *bookmarks;
	GHashTable *bookmarks_hash;
	struct menu *destinations;
	struct point pressed, last, current;
	int button_pressed,moved,popped;
	guint button_timeout, motion_timeout;
};

struct gui *main_loop_gui;

static void navit_vehicle_update(struct navit *this_, struct navit_vehicle *nv);
static void navit_vehicle_draw(struct navit *this_, struct navit_vehicle *nv, struct point *pnt);

void
navit_add_mapset(struct navit *this_, struct mapset *ms)
{
	this_->mapsets = g_list_append(this_->mapsets, ms);
}

struct mapset *
navit_get_mapset(struct navit *this_)
{
	return this_->mapsets->data;
}

struct tracking *
navit_get_tracking(struct navit *this_)
{
	return this_->tracking;
}

void
navit_add_layout(struct navit *this_, struct layout *lay)
{
	this_->layouts = g_list_append(this_->layouts, lay);
}

void
navit_draw(struct navit *this_)
{
	GList *l;
	struct navit_vehicle *nv;

	transform_setup_source_rect(this_->trans);
	graphics_draw(this_->gra, this_->displaylist, this_->mapsets, this_->trans, this_->layouts);
	l=this_->vehicles;
	while (l) {
		nv=l->data;
		navit_vehicle_draw(this_, nv, NULL);
		l=g_list_next(l);
	}
	this_->ready=1;
}

void
navit_draw_displaylist(struct navit *this_)
{
	if (this_->ready) 
		graphics_displaylist_draw(this_->gra, this_->displaylist, this_->trans, this_->layouts);
}

static void
navit_resize(void *data, int w, int h)
{
	struct navit *this_=data;
	struct map_selection sel;
	memset(&sel, 0, sizeof(sel));
	sel.u.p_rect.rl.x=w;
	sel.u.p_rect.rl.y=h;
	transform_set_screen_selection(this_->trans, &sel);
	navit_draw(this_);
}

static gboolean
navit_popup(void *data)
{
	struct navit *this_=data;
	popup(this_, 1, &this_->pressed);
	this_->button_timeout=0;
	this_->popped=1;
	return FALSE;
}

static void
navit_button(void *data, int pressed, int button, struct point *p)
{
	struct navit *this_=data;
	int border=16;

	if (pressed) {
		this_->pressed=*p;
		this_->last=*p;
		if (button == 1) {
			this_->button_pressed=1;
			this_->moved=0;
			this_->popped=0;
			this_->button_timeout=g_timeout_add(500, navit_popup, data);
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
			g_source_remove(this_->button_timeout);	
			this_->button_timeout=0;
			if (! this_->moved && ! transform_within_border(this_->trans, p, border))
				navit_set_center_screen(this_, p);
				
		}
		if (this_->motion_timeout) {
			g_source_remove(this_->motion_timeout);	
			this_->motion_timeout=0;
		}
		if (this_->moved) {
			struct point p;
			transform_get_size(this_->trans, &p.x, &p.y);
			p.x/=2;
			p.y/=2;
			p.x-=this_->last.x-this_->pressed.x;
			p.y-=this_->last.y-this_->pressed.y;
			navit_set_center_screen(this_, &p);
		}
	}
}


static gboolean
navit_motion_timeout(void *data)
{
	struct navit *this_=data;
	int dx, dy;

	dx=(this_->current.x-this_->last.x);
	dy=(this_->current.y-this_->last.y);
	if (dx || dy) {
		this_->last=this_->current;
		graphics_displaylist_move(this_->displaylist, dx, dy);
		graphics_displaylist_draw(this_->gra, this_->displaylist, this_->trans, this_->layouts);
		this_->moved=1;
	}
	this_->motion_timeout=0;
	return FALSE;
}

static void
navit_motion(void *data, struct point *p)
{
	struct navit *this_=data;
	int dx, dy;

	if (this_->button_pressed && !this_->popped) {
		dx=(p->x-this_->pressed.x);
		dy=(p->y-this_->pressed.y);
		if (dx < -8 || dx > 8 || dy < -8 || dy > 8) {
			if (this_->button_timeout) {
				g_source_remove(this_->button_timeout);	
				this_->button_timeout=0;
			}
			this_->current=*p;
			if (! this_->motion_timeout)
				this_->motion_timeout=g_timeout_add(100, navit_motion_timeout, data);
		}
	}
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
navit_new(struct attr **attrs)
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
	this_->vehicle_cbl=callback_list_new();
	this_->init_cbl=callback_list_new();
	this_->attr_cbl=callback_list_new();

	f=popen("pidof /usr/bin/ipaq-sleep","r");
	if (f) {
		fscanf(f,"%d",&this_->pid);
		dbg(1,"ipaq_sleep pid=%d\n", this_->pid);
		pclose(f);
	}

	this_->bookmarks_hash=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	this_->cursor_flag=1;
	this_->orient_north_flag=0;
	this_->tracking_flag=1;

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
	return this_;
}

void
navit_set_gui(struct navit *this_, struct gui *gui, char *type)
{
	this_->gui=gui;
	this_->gui_type=g_strdup(type);
	if (gui_has_main_loop(this_->gui)) {
		if (! main_loop_gui) {
			main_loop_gui=this_->gui;
		} else {
			g_warning("gui with main loop already active, ignoring this instance");
			return;
		}
	}
	this_->menubar=gui_menubar_new(this_->gui);
	this_->toolbar=gui_toolbar_new(this_->gui);
	this_->statusbar=gui_statusbar_new(this_->gui);
}

void
navit_set_graphics(struct navit *this_, struct graphics *gra, char *type)
{
	this_->gra=gra;
	this_->gra_type=g_strdup(type);
	graphics_register_resize_callback(this_->gra, navit_resize, this_);
	graphics_register_button_callback(this_->gra, navit_button, this_);
	graphics_register_motion_callback(this_->gra, navit_motion, this_);
}

struct graphics *
navit_get_graphics(struct navit *this_)
{
	return this_->gra;
}

static void
navit_map_toggle(struct menu *menu, struct navit *this_, struct map *map)
{
	if ((menu_get_toggle(menu) != 0) != (map_get_active(map) != 0)) {
		map_set_active(map, (menu_get_toggle(menu) != 0));
		navit_draw(this_);
	}
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

static void
navit_add_menu_destinations(struct navit *this_, char *name, struct menu *rmen, GHashTable *h, struct callback *cb)
{
	char buffer2[2048];
	char *i,*n;
	struct menu *men,*nmen;

	if (rmen) {
		i=name;
		n=name;
		men=rmen;
		while (h && (i=strchr(n, '/'))) {
			strcpy(buffer2, name);
			buffer2[i-name]='\0';
			if (!(nmen=g_hash_table_lookup(h, buffer2))) {
				nmen=menu_add(men, buffer2+(n-name), menu_type_submenu, NULL);
				g_hash_table_insert(h, g_strdup(buffer2), nmen);
			}
			n=i+1;
			men=nmen;
		}
		menu_add(men, n, menu_type_menu, cb);
	}
}

static const char *
navit_proj2str(enum projection pro)
{
	switch (pro) {
		case projection_mg:
			return "mg";
		case projection_garmin:
			return "garmin";
		default:
			return "";
	}
}
static void
navit_append_coord(struct navit *this_, char *file, struct pcoord *c, char *type, char *description, struct menu *rmen, GHashTable *h, void (*cb_func)(void))
{
	FILE *f;
	int offset=0;
	char *buffer;
	const char *prostr;
	struct callback *cb;

	f=fopen(file, "a");
	if (f) {
		offset=ftell(f);
		if (c) {
			prostr = navit_proj2str(c->pro);
			fprintf(f,"%s%s0x%x 0x%x type=%s label=\"%s\"\n",
				 prostr, *prostr ? ":" : "", c->x, c->y, type, description);
		} else
			fprintf(f,"\n");
		fclose(f);
	}
	if (c) {
		buffer=g_strdup(description);
		cb=callback_new_2(cb_func, this_, (void *)offset);
		navit_add_menu_destinations(this_, buffer, rmen, h, cb);
		g_free(buffer);
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


static void
navit_set_destination_from_file(struct navit *this_, char *file, int bookmark, int offset)
{
	FILE *f;
	char *name, *description, buffer[2048];
	struct pcoord c;
	
	f=fopen(file, "r");
	if (! f)
		return;
	fseek(f, offset, SEEK_SET);
	if (parse_line(f, buffer, &name, &c) <= 0)
		return;
	if (bookmark) {
		description=g_strdup_printf("Bookmark %s", name);
		navit_set_destination(this_, &c, description);
		g_free(description);
	} else 
		navit_set_destination(this_, &c, name);
}

static void
navit_set_destination_from_destination(struct navit *this_, void *offset_p)
{
	navit_set_destination_from_file(this_, "destination.txt", 0, (int)offset_p);
}

static void
navit_set_destination_from_bookmark(struct navit *this_, void *offset_p)
{
	navit_set_destination_from_file(this_, "bookmark.txt", 1, (int)offset_p);
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
	navit_append_coord(this_, "destination.txt", c, "former_destination", description, this_->destinations, NULL, callback_cast(navit_set_destination_from_destination));
	if (this_->route) {
		route_set_destination(this_->route, c);
		navit_draw(this_);
	}
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
navit_add_bookmark(struct navit *this_, struct pcoord *c, char *description)
{
	navit_append_coord(this_,"bookmark.txt", c, "bookmark", description, this_->bookmarks, this_->bookmarks_hash, callback_cast(navit_set_destination_from_bookmark));
}

struct navit *global_navit;


/**
 * Deprecated
 */
static void
navit_debug(struct navit *this_)
{
#if 0
	struct attr attr;
#include "search.h"
	struct search_list *sl;
	struct search_list_result *res;

	debug_level_set("data_mg:town_search_get_item",9);
	debug_level_set("data_mg:town_search_compare",9);
	debug_level_set("data_mg:tree_search_next",9);
	debug_level_set("data_mg:map_search_new_mg",9);
	sl=search_list_new(this_->mapsets->data);
	attr.type=attr_country_all;
	attr.u.str="Fra";
	search_list_search(sl, &attr, 1);
	while ((res=search_list_get_result(sl))) {
		printf("country result\n");
	}
	attr.type=attr_town_name;
	attr.u.str="seclin";
	search_list_search(sl, &attr, 1);
	while ((res=search_list_get_result(sl))) {
		printf("town result\n");
	}
	attr.type=attr_street_name;
	attr.u.str="rue";
	search_list_search(sl, &attr, 1);
	while ((res=search_list_get_result(sl))) {
		printf("street result\n");
	}
	search_list_destroy(sl);
	exit(0);
#endif
}

void
navit_add_menu_layouts(struct navit *this_, struct menu *men)
{
	menu_add(men, "Test", menu_type_menu, NULL);
}

void
navit_add_menu_layout(struct navit *this_, struct menu *men)
{
	navit_add_menu_layouts(this_, menu_add(men, _("Layout"), menu_type_submenu, NULL));
}

void
navit_add_menu_projections(struct navit *this_, struct menu *men)
{
	struct callback *cb;
	cb=callback_new_2(callback_cast(navit_projection_set), this_, (void *)projection_mg);
	menu_add(men, "M&G", menu_type_menu, cb);
	cb=callback_new_2(callback_cast(navit_projection_set), this_, (void *)projection_garmin);
	menu_add(men, "Garmin", menu_type_menu, cb);
}

void
navit_add_menu_projection(struct navit *this_, struct menu *men)
{
	navit_add_menu_projections(this_, menu_add(men, _("Projection"), menu_type_submenu, NULL));
}

void
navit_add_menu_maps(struct navit *this_, struct mapset *ms, struct menu *men)
{
	struct mapset_handle *handle;
	struct map *map;
	struct menu *mmen;
	struct callback *cb;

	handle=mapset_open(ms);
	while ((map=mapset_next(handle,0))) {
		char *s=g_strdup_printf("%s:%s", map_get_type(map), map_get_filename(map));
		cb=callback_new_3(callback_cast(navit_map_toggle), NULL, this_, map);
		mmen=menu_add(men, s, menu_type_toggle, cb);
		callback_set_arg(cb, 0, mmen);
		menu_set_toggle(mmen, map_get_active(map));
		g_free(s);
	}
	mapset_close(handle);
}

static void
navit_add_menu_destinations_from_file(struct navit *this_, char *file, struct menu *rmen, GHashTable *h, struct route *route, void (*cb_func)(void))
{
	int pos,flag=0;
	FILE *f;
	char buffer[2048];
	struct pcoord c;
	char *name;
	int offset=0;
	struct callback *cb;
	
	f=fopen(file, "r");
	if (f) {
		while (! feof(f) && (pos=parse_line(f, buffer, &name, &c)) > -3) {
			if (pos > 0) {
				cb=callback_new_2(cb_func, this_, (void *)offset);
				navit_add_menu_destinations(this_, name, rmen, h, cb);
				flag=1;
			} else
				flag=0;
			offset=ftell(f);
		}
		fclose(f);
		if (route && flag)
			route_set_destination(route, &c);
	}
}

void
navit_add_menu_former_destinations(struct navit *this_, struct menu *men, struct route *route)
{
	if (men)
		this_->destinations=menu_add(men, _("Former Destinations"), menu_type_submenu, NULL);
	else
		this_->destinations=NULL;
	navit_add_menu_destinations_from_file(this_, "destination.txt", this_->destinations, NULL, route, callback_cast(navit_set_destination_from_destination));
}

void
navit_add_menu_bookmarks(struct navit *this_, struct menu *men)
{
	if (men)
		this_->bookmarks=menu_add(men, _("Bookmarks"), menu_type_submenu, NULL);
	else
		this_->bookmarks=NULL;
	navit_add_menu_destinations_from_file(this_, "bookmark.txt", this_->bookmarks, this_->bookmarks_hash, NULL, callback_cast(navit_set_destination_from_bookmark));
}

static void
navit_vehicle_toggle(struct navit *this_, struct navit_vehicle *nv)
{
	if (menu_get_toggle(nv->menu)) {
		if (this_->vehicle && this_->vehicle != nv) 
			menu_set_toggle(this_->vehicle->menu, 0);
		this_->vehicle=nv;
	} else {
		if (this_->vehicle == nv) 
			this_->vehicle=NULL;
	}
}

void
navit_add_menu_vehicles(struct navit *this_, struct menu *men)
{
	struct navit_vehicle *nv;
	struct callback *cb;
	GList *l;
	l=this_->vehicles;
	while (l) {
		nv=l->data;
		cb=callback_new_2(callback_cast(navit_vehicle_toggle), this_, nv);
		nv->menu=menu_add(men, nv->name, menu_type_toggle, cb);
		menu_set_toggle(nv->menu, this_->vehicle == nv);
		l=g_list_next(l);
	}
}

void
navit_add_menu_vehicle(struct navit *this_, struct menu *men)
{
	men=menu_add(men, _("Vehicle"), menu_type_submenu, NULL);
	navit_add_menu_vehicles(this_, men);
}

void
navit_speak(struct navit *this_)
{
	struct navigation *nav=this_->navigation;
	struct navigation_list *list;
	struct item *item;
	struct attr attr;

	list=navigation_list_new(nav);	
	item=navigation_list_get_item(list);
	if (item_attr_get(item, attr_navigation_speech, &attr)) 
		speech_say(this_->speech, attr.u.str);
	navigation_list_destroy(list);
}

static void
navit_window_roadbook_update(struct navit *this_)
{
	struct navigation *nav=this_->navigation;
	struct navigation_list *list;
	struct item *item;
	struct attr attr;
	struct param_list param[5];

	dbg(1,"enter\n");	
	datawindow_mode(this_->roadbook_window, 1);
	list=navigation_list_new(nav);
	while ((item=navigation_list_get_item(list))) {
		attr.u.str=NULL;
		item_attr_get(item, attr_navigation_long, &attr);
		dbg(2, "Command='%s'\n", attr.u.str);
		param[0].name=_("Command");
		param[0].value=g_strdup(attr.u.str);

		item_attr_get(item, attr_length, &attr);
		dbg(2, "Length=%d\n", attr.u.num);
		param[1].name=_("Length");
		param[1].value=g_strdup_printf("%d m",attr.u.num);

		item_attr_get(item, attr_time, &attr);
		dbg(2, "Time=%d\n", attr.u.num);
		param[2].name=_("Time");
		param[2].value=g_strdup_printf("%d:%d",attr.u.num / 60, attr.u.num % 60);

		item_attr_get(item, attr_destination_length, &attr);
		dbg(2, "Destlength=%d\n", attr.u.num);
		param[3].name=_("Destination Length");
		param[3].value=g_strdup_printf("%d m",attr.u.num);

		item_attr_get(item, attr_destination_time, &attr);
		dbg(2, "Desttime=%d\n", attr.u.num);
		param[4].name=_("Destination Time");
		param[4].value=g_strdup_printf("%d:%d",attr.u.num / 60, attr.u.num % 60);

		datawindow_add(this_->roadbook_window, param, 5);
	}
	navigation_list_destroy(list);
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
		dbg(2,"m=%p %s\n", m, map_get_filename(m));
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

void
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
	struct menu *men;
	struct mapset *ms;
	struct map *map;
	GList *l;
	struct navit_vehicle *nv;

	if (!this_->gui) {
		g_warning("failed to instantiate gui '%s'\n",this_->gui_type);
		navit_destroy(this_);
		return;
	}
	if (!this_->gra) {
		g_warning("failed to instantiate gui '%s'\n",this_->gra_type);
		navit_destroy(this_);
		return;
	}
	if (gui_set_graphics(this_->gui, this_->gra)) {
		g_warning("failed to connect graphics '%s' to gui '%s'\n", this_->gra_type, this_->gui_type);
		g_warning(" Please see http://navit.sourceforge.net/wiki/index.php/Failed_to_connect_graphics_to_gui\n");
		g_warning(" for explanations and solutions\n");

		navit_destroy(this_);
		return;
	}
	graphics_init(this_->gra);
	l=this_->vehicles;
	while (l) {
		dbg(1,"parsed one vehicle\n");
		nv=l->data;
		nv->cursor=cursor_new(this_->gra, &nv->c);
		nv->callback.type=attr_callback;
		nv->callback.u.callback=callback_new_2(callback_cast(navit_vehicle_update), this_, nv);
		vehicle_add_attr(nv->vehicle, &nv->callback, NULL);
		vehicle_set_attr(nv->vehicle, &this_->self, NULL);
		l=g_list_next(l);
	}
	if (this_->mapsets) {
		ms=this_->mapsets->data;
		if (this_->route) {
			if ((map=route_get_map(this_->route)))
				mapset_add(ms, map);
			if ((map=route_get_graph_map(this_->route))) {
				mapset_add(ms, map);
				map_set_active(map, 0);
			}
			route_set_mapset(this_->route, ms);
		}
		if (this_->tracking)
			tracking_set_mapset(this_->tracking, ms);
		if (this_->navigation)
			navigation_set_mapset(this_->navigation, ms);
		if (this_->menubar) {
			men=menu_add(this_->menubar, _("Map"), menu_type_submenu, NULL);
			if (men) {
				navit_add_menu_layout(this_, men);
				navit_add_menu_projection(this_, men);
				navit_add_menu_vehicle(this_, men);
				navit_add_menu_maps(this_, ms, men);
			}
			men=menu_add(this_->menubar, _("Route"), menu_type_submenu, NULL);
			if (men) {
				navit_add_menu_former_destinations(this_, men, this_->route);
				navit_add_menu_bookmarks(this_, men);
			}
		} else
			navit_add_menu_former_destinations(this_, NULL, this_->route);
	}
	if (this_->navigation && this_->speech) {
		this_->nav_speech_cb=callback_new_1(callback_cast(navit_speak), this_);
		navigation_register_callback(this_->navigation, attr_navigation_speech, this_->nav_speech_cb);
#if 0
#endif
	}
	if (this_->menubar) {
		men=menu_add(this_->menubar, "Data", menu_type_submenu, NULL);
		if (men) {
			navit_add_menu_windows_items(this_, men);
		}
	}
	global_navit=this_;
#if 0
	navit_window_roadbook_new(this_);
	navit_window_items_new(this_);
#endif
	navit_debug(this_);
	callback_list_call_1(this_->init_cbl, this_);
}

void
navit_set_center(struct navit *this_, struct coord *center)
{
	struct coord *c=transform_center(this_->trans);
	*c=*center;
	if (this_->ready)
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
	if (this_->ready)
		navit_draw(this_);
}


void
navit_set_center_screen(struct navit *this_, struct point *p)
{
	struct coord c;
	transform_reverse(this_->trans, p, &c);
	navit_set_center(this_, &c);
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
	case attr_tracking:
		if (this_->tracking_flag != !!attr->u.num) {
			this_->tracking_flag=!!attr->u.num;
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
	default:
		return 0;
	}
	if (attr_updated) {
		callback_list_call_attr(this_->attr_cbl, attr->type, 1, &attr);
	}
	return 1;
}

int
navit_get_attr(struct navit *this_, enum attr_type type, struct attr *attr)
{
	switch (type) {
	case attr_cursor:
		attr->u.num=this_->cursor_flag;
		break;
	case attr_tracking:
		attr->u.num=this_->tracking_flag;
		break;
	case attr_orientation:
		attr->u.num=this_->orient_north_flag;
		break;
	default:
		return 0;
	}
	attr->type=type;
	return 1;
}

void
navit_add_attr_cb(struct navit *this_, struct callback *cb)
{
	callback_list_add(this_->attr_cbl, cb);
}

void
navit_remove_attr_cb(struct navit *this_, struct callback *cb)
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

	if (! this_->ready)
		return;

	if (! vehicle_position_attr_get(nv->vehicle, attr_position_direction, &attr_dir) ||
	    ! vehicle_position_attr_get(nv->vehicle, attr_position_speed, &attr_speed) ||
	    ! vehicle_position_attr_get(nv->vehicle, attr_position_coord_geo, &attr_pos))
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
			if (this_->route && nv->update_curr == 1) 
				route_set_position_from_tracking(this_->route, this_->tracking);
		}
	} else {
		if (this_->route && nv->update_curr == 1) {
			cursor_pc.pro = pro;
			cursor_pc.x = nv->coord.x;
			cursor_pc.y = nv->coord.y;
			route_set_position(this_->route, &cursor_pc);
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

	if (this_->pid && nv->speed > 2)
		kill(this_->pid, SIGWINCH);
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
	callback_list_call_2(this_->vehicle_cbl, this_, nv->vehicle);
	if (pnt) 
		navit_vehicle_draw(this_, nv, pnt);
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
		if (this_->navigation) {
			navigation_update(this_->navigation, this_->route);
		}
	}
	navit_draw(this_);
}

/**
 * Register a new vehicle
 *
 * @param navit The navit instance
 * @param v The vehicle instance
 * @param name Guess? :)
 * @param c The color to use for the cursor, currently only used in GTK
 * @param update Wether to refresh the map each time this vehicle position changes (instead of only when it reaches a border)
 * @param follow Wether to center the map on this vehicle position
 * @returns a vehicle instance
 */
struct navit_vehicle *
navit_add_vehicle(struct navit *this_, struct vehicle *v, struct attr **attrs)
{
	struct navit_vehicle *nv=g_new0(struct navit_vehicle, 1);
	struct attr *name,*update,*follow,*color,*active;
	nv->vehicle=v;
	nv->update=1;
	nv->follow=0;
	nv->name="Noname";
	if ((name=attr_search(attrs, NULL, attr_name)))
		nv->name=g_strdup(name->u.str);
	if ((update=attr_search(attrs, NULL, attr_update)))
		nv->update=nv->update=update->u.num;
	if ((follow=attr_search(attrs, NULL, attr_follow)))
		nv->follow=nv->follow=follow->u.num;
	if ((color=attr_search(attrs, NULL, attr_color))) 
		nv->c=*(color->u.color);
	nv->update_curr=nv->update;
	nv->follow_curr=nv->follow;
	this_->vehicles=g_list_append(this_->vehicles, nv);
	if ((active=attr_search(attrs, NULL, attr_active)) && active->u.num) 
		navit_set_vehicle(this_, nv);

	return nv;
}

void
navit_add_vehicle_cb(struct navit *this_, struct callback *cb)
{
	callback_list_add(this_->vehicle_cbl, cb);
}

void
navit_remove_vehicle_cb(struct navit *this_, struct callback *cb)
{
	callback_list_remove(this_->vehicle_cbl, cb);
}

void
navit_add_init_cb(struct navit *this_, struct callback *cb)
{
	callback_list_add(this_->init_cbl, cb);
}

void
navit_remove_init_cb(struct navit *this_, struct callback *cb)
{
	callback_list_remove(this_->init_cbl, cb);
}

void
navit_set_vehicle(struct navit *this_, struct navit_vehicle *nv)
{
	this_->vehicle=nv;
}

void
navit_tracking_add(struct navit *this_, struct tracking *tracking)
{
	this_->tracking=tracking;
}

void
navit_route_add(struct navit *this_, struct route *route)
{
	this_->route=route;
}

void
navit_navigation_add(struct navit *this_, struct navigation *navigation)
{
	this_->navigation=navigation;
}

void
navit_set_speech(struct navit *this_, struct speech *speech)
{
	this_->speech=speech;
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
navit_destroy(struct navit *this_)
{
	/* TODO: destroy objects contained in this_ */
	main_remove_navit(this_);
	g_free(this_);
}

/** @} */
