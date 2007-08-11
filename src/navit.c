#include <stdio.h>
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
#include "map.h"
#include "mapset.h"
#include "main.h"
#include "coord.h"
#include "point.h"
#include "transform.h"
#include "projection.h"
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

#define _(STRING)    gettext(STRING)

struct navit_vehicle {
	char *name;
	int update;
	int update_curr;
	int follow;
	int follow_curr;
	struct menu *menu;
	struct cursor *cursor;
	struct vehicle *vehicle;
	struct callback *update_cb;
};

struct navit {
	GList *mapsets;
	GList *layouts;
	struct gui *gui;
	struct layout *layout_current;
	struct graphics *gra;
	struct action *action;
	struct transformation *trans;
	struct compass *compass;
	struct map_data *map_data;
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
	int pid;
	struct callback *nav_speech_cb;
	struct callback *roadbook_callback;
	struct datawindow *roadbook_window;
	struct menu *bookmarks;
	GHashTable *bookmarks_hash;
	struct menu *destinations;
};

struct gui *main_loop_gui;


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
	graphics_draw(this_->gra, this_->displaylist, this_->mapsets, this_->trans, this_->layouts, this_->route);
	l=this_->vehicles;
	while (l) {
		nv=l->data;
		cursor_redraw(nv->cursor);
		l=g_list_next(l);
	}
	this_->ready=1;
}

void
navit_draw_displaylist(struct navit *this_)
{
	if (this_->ready) {
		graphics_displaylist_draw(this_->gra, this_->displaylist, this_->trans, this_->layouts, this_->route);
	}
}

static void
navit_resize(void *data, int w, int h)
{
	struct navit *this_=data;
	transform_set_size(this_->trans, w, h);
	navit_draw(this_);
}

static void
navit_button(void *data, int pressed, int button, struct point *p)
{
	struct navit *this_=data;
	if (pressed && button == 1) {
		int border=16;
		if (! transform_within_border(this_->trans, p, border)) {
			navit_set_center_screen(this_, p);
		} else
			popup(this_, button, p);
	}
	if (pressed && button == 2)
		navit_set_center_screen(this_, p);
	if (pressed && button == 3)
		popup(this_, button, p);
	if (pressed && button == 4)
		navit_zoom_in(this_, 2);
	if (pressed && button == 5)
		navit_zoom_out(this_, 2);
}

void
navit_zoom_in(struct navit *this_, int factor)
{
	long scale=transform_get_scale(this_->trans)/factor;
	if (scale < 1)
		scale=1;
	transform_set_scale(this_->trans, scale);
	navit_draw(this_);
}

void
navit_zoom_out(struct navit *this_, int factor)
{
	long scale=transform_get_scale(this_->trans)*factor;
	transform_set_scale(this_->trans,scale);
	navit_draw(this_);
}

struct navit *
navit_new(const char *ui, const char *graphics, struct coord *center, enum projection pro, int zoom)
{
	struct navit *this_=g_new0(struct navit, 1);
	FILE *f;

	main_add_navit(this_);
	this_->vehicle_cbl=callback_list_new();

	f=popen("pidof /usr/bin/ipaq-sleep","r");
	if (f) {
		fscanf(f,"%d",&this_->pid);
		dbg(1,"ipaq_sleep pid=%d\n", this_->pid);
		pclose(f);
	}

	this_->bookmarks_hash=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	this_->cursor_flag=1;
	this_->tracking_flag=1;
	this_->trans=transform_new();
	transform_set_projection(this_->trans, pro);

	transform_setup(this_->trans, center, zoom, 0);
	this_->displaylist=graphics_displaylist_new();
	this_->gui=gui_new(this_, ui, 792, 547);
	if (! this_->gui) {
		g_warning("failed to create gui '%s'", ui);
		navit_destroy(this_);
		return NULL;
	}
	if (gui_has_main_loop(this_->gui)) {
		if (! main_loop_gui) {
			main_loop_gui=this_->gui;
		} else {
			g_warning("gui with main loop already active, ignoring this instance");
			navit_destroy(this_);
			return NULL;
		}
	}
	this_->menubar=gui_menubar_new(this_->gui);
	this_->toolbar=gui_toolbar_new(this_->gui);
	this_->statusbar=gui_statusbar_new(this_->gui);
	this_->gra=graphics_new(graphics);
	if (! this_->gra) {
		g_warning("failed to create graphics '%s'", graphics);
		navit_destroy(this_);
		return NULL;
	}
	graphics_register_resize_callback(this_->gra, navit_resize, this_);
	graphics_register_button_callback(this_->gra, navit_button, this_);
	if (gui_set_graphics(this_->gui, this_->gra)) {
		g_warning("failed to connect graphics '%s' to gui '%s'\n", graphics, ui);
		navit_destroy(this_);
		return NULL;
	}
	graphics_init(this_->gra);
	return this_;
}

static void
navit_map_toggle(struct menu *menu, void *this__p, void *map_p)
{
	if ((menu_get_toggle(menu) != 0) != (map_get_active(map_p) != 0)) {
		map_set_active(map_p, (menu_get_toggle(menu) != 0));
		navit_draw(this__p);
	}
}

static void
navit_projection_set(struct menu *menu, void *this__p, void *pro_p)
{
	struct navit *this_=this__p;
	enum projection pro=(enum projection) pro_p;
	struct coord_geo g;
	struct coord *c;

	c=transform_center(this_->trans);
	transform_to_geo(transform_get_projection(this_->trans), c, &g);
	transform_set_projection(this_->trans, pro);
	transform_from_geo(pro, &g, c);
	navit_draw(this_);
}

static void
navit_add_menu_destinations(struct navit *this_, char *name, int offset, struct menu *rmen, GHashTable *h, void (*callback)(struct menu *menu, void *data1, void *data2))
{
	char buffer2[2048];
	char *i,*n;
	struct menu *men,*nmen;

	if (rmen) {
		i=name;
		n=name;
		men=rmen;
		while (h && (i=index(n, '/'))) {
			strcpy(buffer2, name);
			buffer2[i-name]='\0';
			if (!(nmen=g_hash_table_lookup(h, buffer2))) {
				nmen=menu_add(men, buffer2+(n-name), menu_type_submenu, NULL, NULL, NULL);
				g_hash_table_insert(h, g_strdup(buffer2), nmen);
			}
			n=i+1;
			men=nmen;
		}
		menu_add(men, n, menu_type_menu, callback, this_, (void *)offset);
	}
}


static void
navit_append_coord(struct navit *this_, char *file, struct coord *c, char *type, char *description, struct menu *rmen, GHashTable *h, void (*callback)(struct menu *menu, void *data1, void *data2))
{
	FILE *f;
	int offset=0;
	char *buffer;

	f=fopen(file, "a");
	if (f) {
		offset=ftell(f);
		if (c) 
			fprintf(f,"0x%x 0x%x type=%s label=\"%s\"\n", c->x, c->y, type, description);
		else
			fprintf(f,"\n");
		fclose(f);
	}
	if (c) {
		buffer=g_strdup(description);
		navit_add_menu_destinations(this_, buffer, offset, rmen, h, callback);
		g_free(buffer);
	}
}

static int
parse_line(FILE *f, char *buffer, char **name, struct coord *c)
{
	int pos;
	char *s,*i;
	*name=NULL;
	if (! fgets(buffer, 2048, f))
		return -3;
	pos=coord_parse(buffer, projection_mg, c);
	if (! pos)
		return -2;
	if (!buffer[pos] || buffer[pos] == '\n') 
		return -1;
	buffer[strlen(buffer)-1]='\0';
	s=buffer+pos+1;
	if (!strncmp(s,"type=", 5)) {
		i=index(s, '"');
		if (i) {
			s=i+1;
			i=index(s, '"');
			if (i) 
				*i='\0';
		}
	}
	*name=s;
	return pos;
}


static void
navit_set_destination_from_file(struct navit *this_, char *file, int bookmark, int offset)
{
	FILE *f;
	char *name, *description, buffer[2048];
	struct coord c;
	
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
navit_set_destination_from_destination(struct menu *menu, void *this_p, void *offset_p)
{
	navit_set_destination_from_file((struct navit *)this_p, "destination.txt", 0, (int)offset_p);
}

static void
navit_set_destination_from_bookmark(struct menu *menu, void *this_p, void *offset_p)
{
	navit_set_destination_from_file((struct navit *)this_p, "bookmark.txt", 1, (int)offset_p);
}

void
navit_set_destination(struct navit *this_, struct coord *c, char *description)
{
	navit_append_coord(this_, "destination.txt", c, "former_destination", description, this_->destinations, NULL, navit_set_destination_from_destination);
	if (this_->route) {
                route_set_destination(this_->route, c);
                navit_draw(this_);
        }
}


void
navit_add_bookmark(struct navit *this_, struct coord *c, char *description)
{
	navit_append_coord(this_,"bookmark.txt", c, "bookmark", description, this_->bookmarks, this_->bookmarks_hash, navit_set_destination_from_bookmark);
}

struct navit *global_navit;

static void
navit_debug(struct navit *this_)
{
#if 0
#include "attr.h"
#include "item.h"
#include "search.h"
	struct attr attr;
	struct search_list *sl;
	struct search_list_result *res;

	debug_level_set("data_mg:town_search_get_item",1);
	debug_level_set("data_mg:town_search_compare",1);
	debug_level_set("data_mg:tree_search_next",1);
	sl=search_list_new(this_->mapsets->data);
	attr.type=attr_country_all;
	attr.u.str="Deu";
	search_list_search(sl, &attr, 1);
	while (res=search_list_get_result(sl)) {
		printf("country result\n");
	}
	attr.type=attr_town_name;
	attr.u.str="U";
	search_list_search(sl, &attr, 1);
	while (res=search_list_get_result(sl)) {
		printf("town result\n");
	}
	search_list_destroy(sl);
#endif
}

void
navit_add_menu_layouts(struct navit *this_, struct menu *men)
{
	menu_add(men, "Test", menu_type_menu, NULL, NULL, NULL);
}

void
navit_add_menu_layout(struct navit *this_, struct menu *men)
{
	navit_add_menu_layouts(this_, menu_add(men, _("Layout"), menu_type_submenu, NULL, NULL, NULL));
}

void
navit_add_menu_projections(struct navit *this_, struct menu *men)
{
	menu_add(men, "M&G", menu_type_menu, navit_projection_set, this_, (void *)projection_mg);
	menu_add(men, "Garmin", menu_type_menu, navit_projection_set, this_, (void *)projection_garmin);
}

void
navit_add_menu_projection(struct navit *this_, struct menu *men)
{
	navit_add_menu_projections(this_, menu_add(men, _("Projection"), menu_type_submenu, NULL, NULL, NULL));
}

void
navit_add_menu_maps(struct navit *this_, struct mapset *ms, struct menu *men)
{
	struct mapset_handle *handle;
	struct map *map;
	struct menu *mmen;

	handle=mapset_open(ms);
	while ((map=mapset_next(handle,0))) {
		char *s=g_strdup_printf("%s:%s", map_get_type(map), map_get_filename(map));
		mmen=menu_add(men, s, menu_type_toggle, navit_map_toggle, this_, map);
		menu_set_toggle(mmen, map_get_active(map));
		g_free(s);
	}
	mapset_close(handle);
}

static void
navit_add_menu_destinations_from_file(struct navit *this_, char *file, struct menu *rmen, GHashTable *h, struct route *route, void (*callback)(struct menu *menu, void *data1, void *data2))
{
	int pos,flag=0;
	FILE *f;
	char buffer[2048];
	struct coord c;
	char *name;
	int offset=0;
	
	f=fopen(file, "r");
	if (f) {
		while (! feof(f) && (pos=parse_line(f, buffer, &name, &c)) > -3) {
			if (pos > 0) {
				navit_add_menu_destinations(this_, name, offset, rmen, h, callback);
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
		this_->destinations=menu_add(men, "Former Destinations", menu_type_submenu, NULL, NULL, NULL);
	else
		this_->destinations=NULL;
	navit_add_menu_destinations_from_file(this_, "destination.txt", this_->destinations, NULL, route, navit_set_destination_from_destination);
}

void
navit_add_menu_bookmarks(struct navit *this_, struct menu *men)
{
	if (men)
		this_->bookmarks=menu_add(men, "Bookmarks", menu_type_submenu, NULL, NULL, NULL);
	else
		this_->bookmarks=NULL;
	navit_add_menu_destinations_from_file(this_, "bookmark.txt", this_->bookmarks, this_->bookmarks_hash, NULL, navit_set_destination_from_bookmark);
}

static void
navit_vehicle_toggle(struct menu *menu, void *this__p, void *nv_p)
{
	struct navit *this_=(struct navit *)this__p;
	struct navit_vehicle *nv=(struct navit_vehicle *)nv_p;
	if (menu_get_toggle(menu)) {
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
	GList *l;
	l=this_->vehicles;
	while (l) {
		nv=l->data;
		nv->menu=menu_add(men, nv->name, menu_type_toggle, navit_vehicle_toggle, this_, nv);
		menu_set_toggle(nv->menu, this_->vehicle == nv);
		l=g_list_next(l);
	}
}

void
navit_add_menu_vehicle(struct navit *this_, struct menu *men)
{
	men=menu_add(men, "Vehicle", menu_type_submenu, NULL, NULL, NULL);
	navit_add_menu_vehicles(this_, men);
}

void
navit_speak(struct navit *this_)
{
	struct navigation *nav=this_->navigation;
	struct navigation_list *list;
	char *text;

	list=navigation_list_new(nav);	
	text=navigation_list_get(list, navigation_mode_speech);
	speech_say(this_->speech, text);
	navigation_list_destroy(list);
}

static void
navit_window_roadbook_update(struct navit *this_)
{
	struct navigation *nav=this_->navigation;
	struct navigation_list *list;
	char *str;
	struct param_list param[1];

	dbg(1,"enter\n");	
	datawindow_mode(this_->roadbook_window, 1);
	list=navigation_list_new(nav);
	while ((str=navigation_list_get(list, navigation_mode_long_exact))) {
		dbg(2, "Command='%s'\n", str);
		param[0].name="Command";
		param[0].value=str;
		datawindow_add(this_->roadbook_window, param, 1);
	}
	navigation_list_destroy(list);
	datawindow_mode(this_->roadbook_window, 0);
}

void
navit_window_roadbook_destroy(struct navit *this_)
{
	dbg(0, "enter\n");
	navigation_unregister_callback(this_->navigation, navigation_mode_long, this_->roadbook_callback);
	this_->roadbook_window=NULL;
	this_->roadbook_callback=NULL;
}
void
navit_window_roadbook_new(struct navit *this_)
{
	this_->roadbook_callback=callback_new_1(callback_cast(navit_window_roadbook_update), this_);
	navigation_register_callback(this_->navigation, navigation_mode_long, this_->roadbook_callback);
	this_->roadbook_window=gui_datawindow_new(this_->gui, "Roadbook", NULL, callback_new_1(callback_cast(navit_window_roadbook_destroy), this_));
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
	struct coord c;
	char *description;

	dbg(0,"enter col=%s,%s,%s,%s,%s\n", col[0], col[1], col[2], col[3], col[4]);
	sscanf(col[4], "0x%x,0x%x", &c.x, &c.y);
	dbg(0,"0x%x,0x%x\n", c.x, c.y);
	description=g_strdup_printf("%s %s", nwi->name, col[3]);
	navit_set_destination(this_, &c, description);
	g_free(description);
}

static void
navit_window_items_open(struct menu *men, struct navit *this_, struct navit_window_items *nwi)
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
	sel.rect.lu.x=center->x-dist;
	sel.rect.lu.y=center->y+dist;
	sel.rect.rl.x=center->x+dist;
	sel.rect.rl.y=center->y-dist;
	dbg(2,"0x%x,0x%x - 0x%x,0x%x\n", sel.rect.lu.x, sel.rect.lu.y, sel.rect.rl.x, sel.rect.rl.y);
	nwi->click=callback_new_2(navit_window_items_click, this_, nwi);
	nwi->win=gui_datawindow_new(this_->gui, nwi->name, nwi->click, NULL);
	h=mapset_open(navit_get_mapset(this_));
        while ((m=mapset_next(h, 1))) {
		dbg(2,"m=%p %s\n", m, map_get_filename(m));
		mr=map_rect_new(m, &sel);
		dbg(2,"mr=%p\n", mr);
		while ((item=map_rect_get_item(mr))) {
			if (item_coord_get(item, &c, 1)) {
				if (coord_rect_contains(&sel.rect, &c) && g_hash_table_lookup(nwi->hash, &item->type)) {
					if (! item_attr_get(item, attr_label, &attr)) 
						attr.u.str="";
					idist=transform_distance(center, &c);
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
navit_window_items_new(char *name, int distance)
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
	GList *l;
	l=this_->windows_items;
	while (l) {
		nwi=l->data;
		menu_add(men, nwi->name, menu_type_menu, navit_window_items_open, this_, nwi);
		l=g_list_next(l);
	}
}

void
navit_init(struct navit *this_)
{
	struct menu *men;
	struct mapset *ms;

	if (this_->mapsets) {
		ms=this_->mapsets->data;
		if (this_->route)
			route_set_mapset(this_->route, ms);
		if (this_->tracking)
			tracking_set_mapset(this_->tracking, ms);
		if (this_->navigation)
			navigation_set_mapset(this_->navigation, ms);
		if (this_->menubar) {
			men=menu_add(this_->menubar, "Map", menu_type_submenu, NULL, NULL, NULL);
			if (men) {
				navit_add_menu_layout(this_, men);
				navit_add_menu_projection(this_, men);
				navit_add_menu_vehicle(this_, men);
				navit_add_menu_maps(this_, ms, men);
			}
			men=menu_add(this_->menubar, "Route", menu_type_submenu, NULL, NULL, NULL);
			if (men) {
				navit_add_menu_former_destinations(this_, men, this_->route);
				navit_add_menu_bookmarks(this_, men);
			}
		} else
			navit_add_menu_former_destinations(this_, NULL, this_->route);
	}
	if (this_->navigation && this_->speech) {
		this_->nav_speech_cb=callback_new_1(callback_cast(navit_speak), this_);
		navigation_register_callback(this_->navigation, navigation_mode_speech, this_->nav_speech_cb);
#if 0
#endif
	}
	if (this_->menubar) {
		men=menu_add(this_->menubar, "Data", menu_type_submenu, NULL, NULL, NULL);
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

void
navit_toggle_cursor(struct navit *this_)
{
	this_->cursor_flag=1-this_->cursor_flag;
}

void
navit_toggle_tracking(struct navit *this_)
{
	this_->tracking_flag=1-this_->tracking_flag;
}

void
navit_toggle_orient_north(struct navit *this_)
{
	this_->orient_north_flag=1-this_->orient_north_flag;
}

static void
navit_cursor_update(struct navit *this_, struct cursor *cursor)
{
	struct point pnt;
	struct coord *cursor_c=cursor_pos_get(cursor);
	int dir=cursor_get_dir(cursor);
	int speed=cursor_get_speed(cursor);
	enum projection pro;
	int border=30;

	if (!this_->vehicle || this_->vehicle->cursor != cursor)
		return;

	cursor_c=cursor_pos_get(cursor);
	dir=cursor_get_dir(cursor);
	speed=cursor_get_speed(cursor);
	pro=vehicle_projection(this_->vehicle);

	if (!transform(this_->trans, pro, cursor_c, &pnt) || !transform_within_border(this_->trans, &pnt, border)) {
		if (!this_->cursor_flag)
			return;
		if(this_->orient_north_flag)
			navit_set_center_cursor(this_, cursor_c, 0, 50 - 30.*sin(M_PI*dir/180.), 50 + 30.*cos(M_PI*dir/180.));
		else
			navit_set_center_cursor(this_, cursor_c, dir, 50, 80);
		transform(this_->trans, pro, cursor_c, &pnt);
	}

	if (this_->pid && speed > 2)
		kill(this_->pid, SIGWINCH);
	if (this_->tracking && this_->tracking_flag) {
		struct coord c=*cursor_c;
		if (tracking_update(this_->tracking, &c, dir)) {
			cursor_c=&c;
			cursor_pos_set(cursor, cursor_c);
			if (this_->route && this_->vehicle->update_curr == 1) 
				route_set_position_from_tracking(this_->route, this_->tracking);
		}
	} else {
		if (this_->route && this_->vehicle->update_curr == 1)
			route_set_position(this_->route, cursor_c);
	}
	if (this_->route && this_->vehicle->update_curr == 1)
		navigation_update(this_->navigation, this_->route);
	if (this_->cursor_flag) {
		if (this_->vehicle->follow_curr == 1)
			navit_set_center_cursor(this_, cursor_c, dir, 50, 80);
	}
	if (this_->vehicle->follow_curr > 1)
		this_->vehicle->follow_curr--;
	else
		this_->vehicle->follow_curr=this_->vehicle->follow;
	if (this_->vehicle->update_curr > 1)
		this_->vehicle->update_curr--;
	else
		this_->vehicle->update_curr=this_->vehicle->update;
	callback_list_call_2(this_->vehicle_cbl, this_, this_->vehicle->vehicle);
}

void
navit_set_position(struct navit *this_, struct coord *c)
{
	if (this_->route) {
		route_set_position(this_->route, c);
		if (this_->navigation) {
			navigation_update(this_->navigation, this_->route);
		}
	}
	navit_draw(this_);
}

struct navit_vehicle *
navit_add_vehicle(struct navit *this_, struct vehicle *v, const char *name, struct color *c, int update, int follow)
{
	struct navit_vehicle *nv=g_new0(struct navit_vehicle, 1);
	nv->vehicle=v;
	nv->name=g_strdup(name);
	nv->update_curr=nv->update=update;
	nv->follow_curr=nv->follow=follow;
	nv->cursor=cursor_new(this_->gra, v, c, this_->trans);
	nv->update_cb=callback_new_1(callback_cast(navit_cursor_update), this_);
	cursor_add_callback(nv->cursor, nv->update_cb);

	this_->vehicles=g_list_append(this_->vehicles, nv);
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

