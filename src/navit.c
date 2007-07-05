#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <glib.h>
#include "debug.h"
#include "navit.h"
#include "gui.h"
#include "map.h"
#include "mapset.h"
#include "coord.h"
#include "point.h"
#include "transform.h"
#include "projection.h"
#include "menu.h"
#include "graphics.h"
#include "cursor.h"
#include "popup.h"
#include "route.h"
#include "navigation.h"
#include "track.h"

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
	struct cursor *cursor;
	struct speech *speech;
	struct vehicle *vehicle;
	struct track *track;
	struct map_flags *flags;
	int ready;
	struct window *win;
	struct displaylist *displaylist;
	int cursor_flag;
	int update;
	int follow;
	int update_curr;
	int follow_curr;
};

struct gui *
main_loop_gui;


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
	transform_setup_source_rect(this_->trans);
	graphics_draw(this_->gra, this_->displaylist, this_->mapsets, this_->trans, this_->layouts, this_->route);
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

	this_->cursor_flag=1;
	this_->trans=transform_new();
	transform_set_projection(this_->trans, pro);

	transform_setup(this_->trans, center, zoom, 0);
	/* this_->flags=g_new0(struct map_flags, 1); */
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
navit_set_destination_menu(struct menu *menu, void *this__p, void *c_p)
{
	struct navit *this_=this__p;
	struct coord *c=c_p;
	if (this_->route) {
                route_set_destination(this_->route, c);
                navit_draw(this_);
        }

}

void
navit_set_destination(struct navit *this_, struct coord *c, char *description)
{
	int fd;
	char *buffer;
	buffer=g_strdup_printf("0x%x 0x%x %s\n", c->x, c->y, description);
	fd=open("destination.txt", O_RDWR|O_CREAT|O_APPEND, 0644);
	if (fd != -1)
		write(fd, buffer, strlen(buffer));
	close(fd);
	g_free(buffer);
	if (this_->route) {
                route_set_destination(this_->route, c);
                navit_draw(this_);
        }
}

struct navit *global_navit;

static void
navit_show_roadbook(struct navigation *nav, void *data)
{
	struct navigation_list *list;
	char *str;
	
	list=navigation_list_new(nav);
	while ((str=navigation_list_get(list, navigation_mode_long))) {
		printf("%s\n", str);
	}
	navigation_list_destroy(list);
}

void
navit_init(struct navit *this_)
{
	struct menu *mapmen,*men,*men2;
	struct map *map;
	struct mapset_handle *handle;
	struct mapset *ms=this_->mapsets->data;

	if (this_->menubar) {
		mapmen=menu_add(this_->menubar, "Map", menu_type_submenu, NULL, NULL, NULL);
		// menu_add(map, "Test", menu_type_menu, NULL, NULL);
		men=menu_add(mapmen, "Layout", menu_type_submenu, NULL, NULL, NULL);
		menu_add(men, "Test", menu_type_menu, NULL, NULL, NULL);
		men=menu_add(mapmen, "Projection", menu_type_submenu, NULL, NULL, NULL);
		menu_add(men, "M&G", menu_type_menu, navit_projection_set, this_, (void *)projection_mg);
		menu_add(men, "Garmin", menu_type_menu, navit_projection_set, this_, (void *)projection_garmin);
		handle=mapset_open(ms);
		while ((map=mapset_next(handle,0))) {
			char *s=g_strdup_printf("%s:%s", map_get_type(map), map_get_filename(map));
			men2=menu_add(mapmen, s, menu_type_toggle, navit_map_toggle, this_, map);
			menu_set_toggle(men2, map_get_active(map));
			g_free(s);
		}
		mapset_close(handle);
	}
	{
		struct mapset *ms=this_->mapsets->data;
		struct coord c;
		int pos,flag=0;
		FILE *f;

		char buffer[2048];
		this_->route=route_new(ms);
		this_->navigation=navigation_new(ms);
		dbg(0,"navigation_register_callback(%p, ... %p)\n", this_->navigation, this_);
		navigation_register_callback(this_->navigation, navigation_mode_long, navit_show_roadbook, this_);
#if 1
		this_->track=track_new(ms);
#endif
		men=NULL;
		if (this_->menubar) {
			men=menu_add(this_->menubar, "Route", menu_type_submenu, NULL, NULL, NULL);
			men=menu_add(men, "Former Destinations", menu_type_submenu, NULL, NULL, NULL);
		}
		f=fopen("destination.txt", "r");
		if (f) {
			while (! feof(f) && fgets(buffer, 2048, f)) {
				if ((pos=coord_parse(buffer, projection_mg, &c))) {
					if (buffer[pos] && buffer[pos] != '\n' ) {
						struct coord *cn=g_new(struct coord, 1);
						*cn=c;
						buffer[strlen(buffer)-1]='\0';
						if (men)
							menu_add(men, buffer+pos+1, menu_type_menu, navit_set_destination_menu, this_, cn);
					}
					flag=1;
				}
			}
			fclose(f);
			if (flag)
				route_set_destination(this_->route, &c);
		}
	}
	global_navit=this_;
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

static void
navit_cursor_offscreen(struct cursor *cursor, void *this__p)
{
	struct navit *this_=this__p;

	if (this_->cursor_flag)
		navit_set_center(this_, cursor_pos_get(cursor));
}

static void
navit_cursor_update(struct cursor *cursor, void *this__p)
{
	struct navit *this_=this__p;
	struct coord *cursor_c=cursor_pos_get(cursor);
	int dir=cursor_get_dir(cursor);

	if (this_->track) {
		struct coord c=*cursor_c;
		if (track_update(this_->track, &c, dir)) {
			cursor_c=&c;
			cursor_pos_set(cursor, cursor_c);
			if (this_->route && this_->update_curr == 1)
				route_set_position_from_track(this_->route, this_->track);
		}
	} else {
		if (this_->route && this_->update_curr == 1)
			route_set_position(this_->route, cursor_c);
	}
	if (this_->route && this_->update_curr == 1)
		navigation_update(this_->navigation, this_->route);
	if (this_->cursor_flag) {
		if (this_->follow_curr == 1)
			navit_set_center_cursor(this_, cursor_c, dir, 50, 80);
	}
	if (this_->follow_curr > 1)
		this_->follow_curr--;
	else
		this_->follow_curr=this_->follow;
	if (this_->update_curr > 1)
		this_->update_curr--;
	else
		this_->update_curr=this_->update;
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

void
navit_vehicle_add(struct navit *this_, struct vehicle *v, struct color *c, int update, int follow)
{
	this_->vehicle=v;
	this_->update_curr=this_->update=update;
	this_->follow_curr=this_->follow=follow;
	this_->cursor=cursor_new(this_->gra, v, c, this_->trans);
	cursor_register_offscreen_callback(this_->cursor, navit_cursor_offscreen, this_);
	cursor_register_update_callback(this_->cursor, navit_cursor_update, this_);
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
	g_free(this_);
}

