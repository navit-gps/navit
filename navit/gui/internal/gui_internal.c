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

//##############################################################################################################
//#
//# File: gui_internal.c
//# Description: New "internal" GUI for use with any graphics library
//# Comment: Trying to make a touchscreen friendly GUI
//# Authors: Martin Schaller (04/2008), Stefan Klumpp (04/2008)
//#
//##############################################################################################################


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <glib.h>
#include <time.h>
#include "config.h"
#include "item.h"
#include "file.h"
#include "navit.h"
#include "debug.h"
#include "gui.h"
#include "coord.h"
#include "point.h"
#include "plugin.h"
#include "graphics.h"
#include "transform.h"
#include "color.h"
#include "map.h"
#include "layout.h"
#include "callback.h"
#include "vehicle.h"
#include "window.h"
#include "main.h"
#include "keys.h"
#include "mapset.h"
#include "route.h"
#include "search.h"
#include "track.h"
#include "country.h"
#include "config.h"
#include "event.h"
#include "navit_nls.h"
#include "navigation.h"
#include "gui_internal.h"
#include "command.h"

struct menu_data {
	struct widget *search_list;
	struct widget *keyboard;
	struct widget *button_bar;
	int keyboard_mode;
	void (*redisplay)(struct gui_priv *priv, struct widget *widget, void *data);
	struct widget *redisplay_widget;
};

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
struct widget {
	enum widget_type type;
	struct graphics_gc *background,*text_background;
	struct graphics_gc *foreground_frame;
	struct graphics_gc *foreground;
	char *text;
	struct graphics_image *img;
	 /**
	  * A function to be invoked on actions.
	  * @li widget The widget that is receiving the button press.
	  *
	  */
	void (*func)(struct gui_priv *priv, struct widget *widget, void *data);
	int reason;
	int datai;
	void *data;
	/**
	 * @brief A function to deallocate data
	 */
	void (*data_free)(void *data);

	/**
	 * @brief a function that will be called as the widget is being destroyed.
	 * This function can act as a destructor for the widget. It allows for 
	 * on deallocation actions to be specified on a per widget basis.
	 * This function will call g_free on the widget (if required).
	 */
	void (*free) (struct gui_priv *this_, struct widget * w);
	char *prefix;
	char *name;
	char *speech;
	struct pcoord c;
	struct item item;
	int selection_id;
	int state;
	struct point p;
	int wmin,hmin;
	int w,h;
	int textw,texth;
	int bl,br,bt,bb,spx,spy;
	int border;
	/**
	 * The number of widgets to layout horizontally when doing
	 * a orientation_horizontal_vertical layout
	 */
	int cols;
	enum flags flags;
	void *instance;
	int (*set_attr)(void *, struct attr *);
	int (*get_attr)(void *, enum attr_type, struct attr *, struct attr_iter *);
	void (*remove_cb)(void *, struct callback *cb);
	struct callback *cb;
	struct attr on;
	struct attr off;
	int deflt;
	int is_on;
	int redraw;
	struct menu_data *menu_data;
	GList *children;  
};

/**
 * @brief A structure to store configuration values.
 *
 * This structure stores configuration values for how gui elements in the internal GUI
 * should be drawn.  
 */
struct gui_config_settings {
  
  /**
   * The base size (in fractions of a point) to use for text.
   */
  int font_size;
  /**
   * The size (in pixels) that xs style icons should be scaled to.
   */
  int icon_xs;
  /**
   * The size (in pixels) that s style icons (small) should be scaled to
   */
  int icon_s;
  /**
   * The size (in pixels) that l style icons should be scaled to
   */
  int icon_l;
  /**
   * The default amount of spacing (in pixels) to place between GUI elements.
   */
  int spacing;
  
};

/**
 * Indexes into the config_profiles array.
 */
const int LARGE_PROFILE=0;
const int MEDIUM_PROFILE=1;
const int SMALL_PROFILE=2;

/**
 * The default config profiles.
 * 
 * [0] =>  LARGE_PROFILE (screens 640 in one dimension)
 * [1] =>  MEDIUM PROFILE (screens larger than 320 in one dimension
 * [2] => Small profile (default)
 */
static struct gui_config_settings config_profiles[]={
      {545,32,48,96,10}
    , {545,32,48,96,5}
      ,{200,16,32,48,2}
};

struct route_data {
  struct widget * route_table;
  int route_showing;

};

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
struct gui_priv {
	struct navit *nav;
	struct window *win;
	struct graphics *gra;
	struct graphics_gc *background;
	struct graphics_gc *background2;
	struct graphics_gc *highlight_background;
	struct graphics_gc *foreground;
	struct graphics_gc *text_foreground;
	struct graphics_gc *text_background;
	struct color background_color, background2_color, text_foreground_color, text_background_color;
	int spacing;
	int font_size;
	int fullscreen;
	struct graphics_font *font;
	int icon_xs, icon_s, icon_l;
	int pressed;
	struct widget *widgets;
	int widgets_count;
	int redraw;
	struct widget root;
	struct widget *highlighted;
	struct widget *highlighted_menu;
	int clickp_valid, vehicle_valid;
	struct pcoord clickp, vehiclep;
	struct search_list *sl;
	int ignore_button;
	int menu_on_map_click;
	char *country_iso2;
	int speech;
	int keyboard;
	/**
	 * The setting information read from the configuration file.
	 * values of -1 indicate no value was specified in the config file.
	 */
        struct gui_config_settings config;
	struct event_idle *idle;
	struct callback *motion_cb,*button_cb,*resize_cb,*keypress_cb,*idle_cb, *motion_timeout_callback;
	struct event_timeout *motion_timeout_event;
	struct point current;

	struct callback * vehicle_cb;
	  /**
	   * Stores information about the route.
	   */
	struct route_data route_data;

	struct gui_internal_data data;
	struct callback_list *cbl;
	int flags;
	int cols;
	struct attr osd_configuration;
};


/**
 * @brief A structure to store information about a table.
 *
 * The table_data widget stores pointers to extra information needed by the
 * table widget.
 *
 * The table_data structure needs to be freed with data_free along with the widget.
 * 
 */
struct table_data
{
  /**
   * A GList pointer into a widget->children list that indicates the row
   * currently being rendered at the top of the table.
   */
  GList * top_row;
  /**
   * A Glist pointer into a widget->children list that indicates the row
   * currently being rendered at the bottom of the table.
   */
  GList * bottom_row;

  /**
   * A list of table_row widgets that mark the
   * top rows for each page of the table.
   * This is needed for the 'previous page' function of the table.
   */
  GList * page_headers;

  /**
   * A container box that is the child of the table widget that contains+groups
   * the next and previous button.
   */
  struct widget * button_box;

  /**
   * A button widget to handle 'next page' requests
   */
  struct widget * next_button;
  /**
   * A button widget to handle 'previous page' requests.
   */
  struct widget * prev_button;
    

  /**
   * a pointer to the gui context.
   * This is needed by the free function to destory the buttons.
   */
  struct  gui_priv *  this;
};

/**
 * A data structure that holds information about a column that makes up a table.
 *
 *
 */
struct table_column_desc
{

  /**
   * The computed height of a cell in the table.
   */
  int height;

  /**
   * The computed width of a cell in the table.
   */
  int width;
};


static void gui_internal_widget_render(struct gui_priv *this, struct widget *w);
static void gui_internal_widget_pack(struct gui_priv *this, struct widget *w);
static struct widget * gui_internal_box_new(struct gui_priv *this, enum flags flags);
static void gui_internal_widget_append(struct widget *parent, struct widget *child);
static void gui_internal_widget_destroy(struct gui_priv *this, struct widget *w);
static void gui_internal_apply_config(struct gui_priv *this);

static struct widget* gui_internal_widget_table_new(struct gui_priv * this, enum flags flags, int buttons);
static struct widget * gui_internal_widget_table_row_new(struct gui_priv * this, enum flags flags);
static void gui_internal_table_render(struct gui_priv * this, struct widget * w);
static void gui_internal_cmd_route(struct gui_priv * this, struct widget * w,void *);
static void gui_internal_table_pack(struct gui_priv * this, struct widget * w);
static void gui_internal_table_button_next(struct gui_priv * this, struct widget * wm, void *data);
static void gui_internal_table_button_prev(struct gui_priv * this, struct widget * wm, void *data);
static void gui_internal_widget_table_clear(struct gui_priv * this,struct widget * table);
static struct widget * gui_internal_widget_table_row_new(struct gui_priv * this, enum flags flags);
static void gui_internal_table_data_free(void * d);
static void gui_internal_route_update(struct gui_priv * this, struct navit * navit,
				      struct vehicle * v);
static void gui_internal_route_screen_free(struct gui_priv * this_,struct widget * w);
static void gui_internal_populate_route_table(struct gui_priv * this,
				       struct navit * navit);
static void gui_internal_search_idle_end(struct gui_priv *this);
static void gui_internal_search(struct gui_priv *this, char *what, char *type, int flags);
static void gui_internal_search_street(struct gui_priv *this, struct widget *widget, void *data);
static void gui_internal_search_street_in_town(struct gui_priv *this, struct widget *widget, void *data);
static void gui_internal_search_town(struct gui_priv *this, struct widget *wm, void *data);
static void gui_internal_search_town_in_country(struct gui_priv *this, struct widget *wm);
static void gui_internal_search_country(struct gui_priv *this, struct widget *widget, void *data);

static struct widget *gui_internal_keyboard_do(struct gui_priv *this, struct widget *wkbdb, int mode);
static struct menu_data * gui_internal_menu_data(struct gui_priv *this);
/*
 * * Display image scaled to specific size
 * * searches for scaleable and pre-scaled image
 * * @param this Our gui context
 * * @param name image name
 * * @param w desired width of image
 * * @param h desired height of image
 * * @returns image_struct Ptr to scaled image struct or NULL if not scaled or found
 * */
static struct graphics_image *
image_new_scaled(struct gui_priv *this, char *name, int w, int h)
{
	struct graphics_image *ret=NULL;
	char *full_name=NULL;
	int i;

	for (i = 1 ; i < 6 ; i++) {
		full_name=NULL;
		switch (i) {
		case 3:
			full_name=g_strdup_printf("%s/xpm/%s.svg", getenv("NAVIT_SHAREDIR"), name);
			break;
		case 2:
			full_name=g_strdup_printf("%s/xpm/%s.svgz", getenv("NAVIT_SHAREDIR"), name);
			break;
		case 1:
			if (w != -1 && h != -1) {
				full_name=g_strdup_printf("%s/xpm/%s_%d_%d.png", getenv("NAVIT_SHAREDIR"), name, w, h);
			}
			break;
		case 4:
			full_name=g_strdup_printf("%s/xpm/%s.png", getenv("NAVIT_SHAREDIR"), name);
			break;
		case 5:
			full_name=g_strdup_printf("%s/xpm/%s.xpm", getenv("NAVIT_SHAREDIR"), name);
			break;
		}
		dbg(1,"trying '%s'\n", full_name);
		if (! full_name)
			continue;
		if (!file_exists(full_name)) {
			g_free(full_name);
			continue;
		}
		ret=graphics_image_new_scaled(this->gra, full_name, w, h);
		dbg(1,"ret=%p\n", ret);
		g_free(full_name);
		if (ret) 
			return ret;	
	}
	dbg(0,"failed to load %s with %d,%d\n", name, w, h);
	return NULL;
}


static struct graphics_image *
image_new_o(struct gui_priv *this, char *name)
{
	return image_new_scaled(this, name, -1, -1);
}

static struct graphics_image *
image_new_xs(struct gui_priv *this, char *name)
{
	return image_new_scaled(this, name, this->icon_xs, this->icon_xs);
}


static struct graphics_image *
image_new_s(struct gui_priv *this, char *name)
{
	return image_new_scaled(this, name, this->icon_s, this->icon_s);
}

static struct graphics_image *
image_new_l(struct gui_priv *this, char *name)
{
	return image_new_scaled(this, name, this->icon_l, this->icon_l);
}

static char *
coordinates(struct pcoord *pc, char sep)
{
	char latc='N',lngc='E';
	int lat_deg,lat_min,lat_sec;
	int lng_deg,lng_min,lng_sec;
	struct coord_geo g;
	struct coord c;
	c.x=pc->x;
	c.y=pc->y;
	transform_to_geo(pc->pro, &c, &g);

	if (g.lat < 0) {
		g.lat=-g.lat;
		latc='S';
	}
	if (g.lng < 0) {
		g.lng=-g.lng;
		lngc='W';
	}
	lat_deg=g.lat;
	lat_min=fmod(g.lat*60,60);
	lat_sec=fmod(g.lat*3600,60);
	lng_deg=g.lng;
	lng_min=fmod(g.lng*60,60);
	lng_sec=fmod(g.lng*3600,60);
	return g_strdup_printf("%d°%d'%d\" %c%c%d°%d'%d\" %c",lat_deg,lat_min,lat_sec,latc,sep,lng_deg,lng_min,lng_sec,lngc);
}

static void
gui_internal_background_render(struct gui_priv *this, struct widget *w)
{
	struct point pnt=w->p;
	if (w->state & STATE_HIGHLIGHTED) 
		graphics_draw_rectangle(this->gra, this->highlight_background, &pnt, w->w, w->h);
	else {
		if (w->background)
			graphics_draw_rectangle(this->gra, w->background, &pnt, w->w, w->h);
	}
}
static struct widget *
gui_internal_label_new(struct gui_priv *this, char *text)
{
	struct point p[4];
	int w=0;
	int h=0;
	
	struct widget *widget=g_new0(struct widget, 1);
	widget->type=widget_label;
	if (text) {
		widget->text=g_strdup(text);
		graphics_get_text_bbox(this->gra, this->font, text, 0x10000, 0x0, p, 0);
		w=p[2].x-p[0].x;
		h=p[0].y-p[2].y;
	}
	widget->h=h+this->spacing;
	widget->texth=h;
	widget->w=w+this->spacing;
	widget->textw=w;
	widget->flags=gravity_center;
	widget->foreground=this->text_foreground;
	widget->text_background=this->text_background;

	return widget;
}

static struct widget *
gui_internal_label_new_abbrev(struct gui_priv *this, char *text, int maxwidth)
{
	struct widget *ret=NULL;
	char *tmp=g_malloc(strlen(text)+3);
	int i;

	i=strlen(text)-1;
	while (i >= 0) {
		strcpy(tmp, text);
		strcpy(tmp+i,"..");
		ret=gui_internal_label_new(this, tmp);
		if (ret->w < maxwidth)
			break;
		gui_internal_widget_destroy(this, ret);
		ret=NULL;
		i--;
	}
	g_free(tmp);
	return ret;
}

static struct widget *
gui_internal_image_new(struct gui_priv *this, struct graphics_image *image)
{
	struct widget *widget=g_new0(struct widget, 1);
	widget->type=widget_image;
	widget->img=image;
	if (image) {
		widget->w=image->width;
		widget->h=image->height;
	}
	return widget;
}

static void
gui_internal_image_render(struct gui_priv *this, struct widget *w)
{
	struct point pnt;

	gui_internal_background_render(this, w);
	if (w->img) {
		pnt=w->p;
		pnt.x+=w->w/2-w->img->hot.x;
		pnt.y+=w->h/2-w->img->hot.y;
		graphics_draw_image(this->gra, this->foreground, &pnt, w->img);
	}
}

static void
gui_internal_label_render(struct gui_priv *this, struct widget *w)
{
	struct point pnt=w->p;
	gui_internal_background_render(this, w);
	if (w->text) {
		if (w->flags & gravity_right) {
			pnt.y+=w->h-this->spacing;
			pnt.x+=w->w-w->textw-this->spacing;
			graphics_draw_text(this->gra, w->foreground, w->text_background, this->font, w->text, &pnt, 0x10000, 0x0);
		} else {
			pnt.y+=w->h-this->spacing;
			graphics_draw_text(this->gra, w->foreground, w->text_background, this->font, w->text, &pnt, 0x10000, 0x0);
		}
	}
}

/**
 * @brief A text box is a widget that renders a text string containing newlines.
 * The string will be broken up into label widgets at each newline with a vertical layout.
 *
 */
static struct widget *
gui_internal_text_new(struct gui_priv *this, char *text, enum flags flags)
{
	char *s=g_strdup(text),*s2,*tok;
	struct widget *ret=gui_internal_box_new(this, flags);
	s2=s;
	while ((tok=strtok(s2,"\n"))) {
		gui_internal_widget_append(ret, gui_internal_label_new(this, tok));
		s2=NULL;
	}
	gui_internal_widget_pack(this,ret);
	return ret;
}


static struct widget *
gui_internal_button_new_with_callback(struct gui_priv *this, char *text, struct graphics_image *image, enum flags flags, void(*func)(struct gui_priv *priv, struct widget *widget, void *data), void *data)
{
	struct widget *ret=NULL;
	ret=gui_internal_box_new(this, flags);
	if (ret) {
		if (image)
			gui_internal_widget_append(ret, gui_internal_image_new(this, image));
		if (text)
			gui_internal_widget_append(ret, gui_internal_text_new(this, text, gravity_center|orientation_vertical));
		ret->func=func;
		ret->data=data;
		if (func) {
			ret->state |= STATE_SENSITIVE;
			ret->speech=g_strdup(text);
		}
	}
	return ret;

}

static int
gui_internal_button_attr_update(struct gui_priv *this, struct widget *w)
{
	struct widget *wi;
	int is_on=0;
	struct attr curr;
	GList *l;

	if (w->get_attr(w->instance, w->on.type, &curr, NULL))
		is_on=curr.u.data == w->on.u.data;
	else
		is_on=w->deflt;
	if (is_on != w->is_on) {
		if (w->redraw)
			this->redraw=1;
		w->is_on=is_on;
		l=g_list_first(w->children);
		if (l) {
			wi=l->data;
			if (wi->img)
				graphics_image_free(this->gra, wi->img);
			wi->img=image_new_xs(this, is_on ? "gui_active" : "gui_inactive");
		}
		if (w->is_on && w->off.type == attr_none)
			w->state &= ~STATE_SENSITIVE;
		else
			w->state |= STATE_SENSITIVE;
		return 1;
	}
	return 0;
}

static void
gui_internal_button_attr_callback(struct gui_priv *this, struct widget *w)
{
	if (gui_internal_button_attr_update(this, w))
		gui_internal_widget_render(this, w);
}
static void
gui_internal_button_attr_pressed(struct gui_priv *this, struct widget *w, void *data)
{
	if (w->is_on) 
		w->set_attr(w->instance, &w->off);
	else
		w->set_attr(w->instance, &w->on);
	gui_internal_button_attr_update(this, w);
	
}

static struct widget *
gui_internal_button_navit_attr_new(struct gui_priv *this, char *text, enum flags flags, struct attr *on, struct attr *off)
{
	struct graphics_image *image=image_new_xs(this, "gui_inactive");
	struct widget *ret;
	ret=gui_internal_button_new_with_callback(this, text, image, flags, gui_internal_button_attr_pressed, NULL);
	if (on)
		ret->on=*on;
	if (off)
		ret->off=*off;
	ret->get_attr=(int (*)(void *, enum attr_type, struct attr *, struct attr_iter *))navit_get_attr;
	ret->set_attr=(int (*)(void *, struct attr *))navit_set_attr;
	ret->remove_cb=(void (*)(void *, struct callback *))navit_remove_callback;
	ret->instance=this->nav;
	ret->cb=callback_new_attr_2(callback_cast(gui_internal_button_attr_callback), on->type, this, ret);
	navit_add_callback(this->nav, ret->cb);
	gui_internal_button_attr_update(this, ret);
	return ret;
}

static struct widget *
gui_internal_button_map_attr_new(struct gui_priv *this, char *text, enum flags flags, struct map *map, struct attr *on, struct attr *off, int deflt)
{
	struct graphics_image *image=image_new_xs(this, "gui_inactive");
	struct widget *ret;
	ret=gui_internal_button_new_with_callback(this, text, image, flags, gui_internal_button_attr_pressed, NULL);
	if (on)
		ret->on=*on;
	if (off)
		ret->off=*off;
	ret->deflt=deflt;
	ret->get_attr=(int (*)(void *, enum attr_type, struct attr *, struct attr_iter *))map_get_attr;
	ret->set_attr=(int (*)(void *, struct attr *))map_set_attr;
	ret->remove_cb=(void (*)(void *, struct callback *))map_remove_callback;
	ret->instance=map;
	ret->redraw=1;
	ret->cb=callback_new_attr_2(callback_cast(gui_internal_button_attr_callback), on->type, this, ret);
	map_add_callback(map, ret->cb);
	gui_internal_button_attr_update(this, ret);
	return ret;
}

static struct widget *
gui_internal_button_new(struct gui_priv *this, char *text, struct graphics_image *image, enum flags flags)
{
	return gui_internal_button_new_with_callback(this, text, image, flags, NULL, NULL);
}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void gui_internal_clear(struct gui_priv *this)
{
	struct graphics *gra=this->gra;
	struct point pnt;
	pnt.x=0;
	pnt.y=0;
	graphics_draw_rectangle(gra, this->background, &pnt, this->root.w, this->root.h);
}

static struct widget *
gui_internal_find_widget(struct widget *wi, struct point *p, int flags)
{
	struct widget *ret,*child;
	GList *l=wi->children;

	if (p) { 
		if (wi->p.x > p->x )
			return NULL;
		if (wi->p.y > p->y )
			return NULL;
		if ( wi->p.x + wi->w < p->x) 
			return NULL;
		if ( wi->p.y + wi->h < p->y) 
			return NULL;
	}
	if (wi->state & flags) 
		return wi;
	while (l) {
		child=l->data;
		ret=gui_internal_find_widget(child, p, flags);
		if (ret) {
			return ret;
		}
		l=g_list_next(l);
	}
	return NULL;
	
}

static void
gui_internal_highlight_do(struct gui_priv *this, struct widget *found)
{
	if (found == this->highlighted)
		return;
	
	graphics_draw_mode(this->gra, draw_mode_begin);
	if (this->highlighted) {
		this->highlighted->state &= ~STATE_HIGHLIGHTED;
		if (this->root.children && this->highlighted_menu == g_list_last(this->root.children)->data) 
			gui_internal_widget_render(this, this->highlighted);
		this->highlighted=NULL;
		this->highlighted_menu=NULL;
	}
	if (found) {
		this->highlighted=found;
		this->highlighted_menu=g_list_last(this->root.children)->data;
		this->highlighted->state |= STATE_HIGHLIGHTED;
		gui_internal_widget_render(this, this->highlighted);
		dbg(1,"%d,%d %dx%d\n", found->p.x, found->p.y, found->w, found->h);
	}	
	graphics_draw_mode(this->gra, draw_mode_end);
}
//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void gui_internal_highlight(struct gui_priv *this)
{
	struct widget *menu,*found=NULL;
	if (this->current.x > -1 && this->current.y > -1) {
		menu=g_list_last(this->root.children)->data;
		found=gui_internal_find_widget(menu, &this->current, STATE_SENSITIVE);
	}
	gui_internal_highlight_do(this, found);
	this->motion_timeout_event=NULL;
}

static struct widget *
gui_internal_box_new_with_label(struct gui_priv *this, enum flags flags, char *label)
{
	struct widget *widget=g_new0(struct widget, 1);

	if (label) 
		widget->text=g_strdup(label);
	widget->type=widget_box;
	widget->flags=flags;
	return widget;
}

static struct widget *
gui_internal_box_new(struct gui_priv *this, enum flags flags)
{
	return gui_internal_box_new_with_label(this, flags, NULL);
}


static void gui_internal_box_render(struct gui_priv *this, struct widget *w)
{
	struct widget *wc;
	GList *l;

	gui_internal_background_render(this, w);
#if 1
	if (w->foreground && w->border) {
	struct point pnt[5];
	pnt[0]=w->p;
        pnt[1].x=pnt[0].x+w->w;
        pnt[1].y=pnt[0].y;
        pnt[2].x=pnt[0].x+w->w;
        pnt[2].y=pnt[0].y+w->h;
        pnt[3].x=pnt[0].x;
        pnt[3].y=pnt[0].y+w->h;
        pnt[4]=pnt[0];
	graphics_gc_set_linewidth(w->foreground, w->border ? w->border : 1);
	graphics_draw_lines(this->gra, w->foreground, pnt, 5);
	graphics_gc_set_linewidth(w->foreground, 1);
	}
#endif

	l=w->children;
	while (l) {
		wc=l->data;	
		gui_internal_widget_render(this, wc);
		l=g_list_next(l);
	}
}


/**
 * @brief Compute the size and location for the widget.
 *
 *
 */
static void gui_internal_box_pack(struct gui_priv *this, struct widget *w)
{
	struct widget *wc;
	int x0,x=0,y=0,width=0,height=0,owidth=0,oheight=0,expand=0,count=0,rows=0,cols=w->cols ? w->cols : 0;
	GList *l;
	int orientation=w->flags & 0xffff0000;

	if (!cols)
		cols=this->cols;
	if (!cols) {
		 height=navit_get_height(this->nav);
		 width=navit_get_width(this->nav);
		 if ( (height/width) > 1.0 )
			 cols=3;
		 else
			 cols=2;
		 width=0;
		 height=0;
	}
	
	
	/**
	 * count the number of children
	 */
	l=w->children;
	while (l) {
		count++;
		l=g_list_next(l);
	}
	if (orientation == orientation_horizontal_vertical && count <= cols)
		orientation=orientation_horizontal;
	switch (orientation) {
	case orientation_horizontal:
		/**
		 * For horizontal orientation:
		 * pack each child and find the largest height and 
		 * compute the total width. x spacing (spx) is considered
		 *
		 * If any children want to be expanded
		 * we keep track of this
		 */
		l=w->children;
		while (l) {
			wc=l->data;
			gui_internal_widget_pack(this, wc);
			if (height < wc->h)
				height=wc->h;
			width+=wc->w;
			if (wc->flags & flags_expand)
				expand+=wc->w ? wc->w : 1;
			l=g_list_next(l);
			if (l)
				width+=w->spx;
		}
		owidth=width;
		if (expand && w->w) {
			expand=100*(w->w-width+expand)/expand;
			owidth=w->w;
		} else
			expand=100;
		break;
	case orientation_vertical:
		/**
		 * For vertical layouts:
		 * We pack each child and compute the largest width and
		 * the total height.  y spacing (spy) is considered
		 *
		 * If the expand flag is set then teh expansion amount
		 * is computed.
		 */
		l=w->children;
		while (l) {
			wc=l->data;
			gui_internal_widget_pack(this, wc);
			if (width < wc->w)
				width=wc->w;
			height+=wc->h;
			if (wc->flags & flags_expand)
				expand+=wc->h ? wc->h : 1;
			l=g_list_next(l);
			if (l)
				height+=w->spy;
		}
		oheight=height;
		if (expand && w->h) {
			expand=100*(w->h-height+expand)/expand;
			oheight=w->h;
		} else
			expand=100;
		break;
	case orientation_horizontal_vertical:
		/**
		 * For horizontal_vertical orientation
		 * pack the children.
		 * Compute the largest height and largest width.
		 * Layout the widgets based on having the
		 * number of columns specified by (cols)
		 */
		count=0;
		l=w->children;
		while (l) {
			wc=l->data;
			gui_internal_widget_pack(this, wc);
			if (height < wc->h)
				height=wc->h;
			if (width < wc->w) 
				width=wc->w;
			l=g_list_next(l);
			count++;
		}
		if (count < cols)
			cols=count;
		rows=(count+cols-1)/cols;
		width*=cols;
		height*=rows;
		width+=w->spx*(cols-1);
		height+=w->spy*(rows-1);
		owidth=width;
		oheight=height;
		expand=100;
		break;
	default:
		/**
		 * No orientation was specified.
		 * width and height are both 0.
		 * The width & height of this widget
		 * will be used.
		 */
		if(!w->w && !w->h)
			dbg(0,"Warning width and height of a widget are 0");
		break;
	}
	if (! w->w && ! w->h) {
		w->w=w->bl+w->br+width;
		w->h=w->bt+w->bb+height;
	}

	/**
	 * At this stage the width and height of this
	 * widget has been computed.
	 * We now make a second pass assigning heights,
	 * widths and coordinates to each child widget.
	 */

	if (w->flags & gravity_left) 
		x=w->p.x+w->bl;
	if (w->flags & gravity_xcenter)
		x=w->p.x+w->w/2-owidth/2;
	if (w->flags & gravity_right)
		x=w->p.x+w->w-w->br-owidth;
	if (w->flags & gravity_top)
		y=w->p.y+w->bt;
	if (w->flags & gravity_ycenter) 
		y=w->p.y+w->h/2-oheight/2;
	if (w->flags & gravity_bottom) 
		y=w->p.y+w->h-w->bb-oheight;
	l=w->children;
	switch (orientation) {
	case orientation_horizontal:
		l=w->children;
		while (l) {
			wc=l->data;
			wc->p.x=x;
			if (wc->flags & flags_fill)
				wc->h=w->h;
			if (wc->flags & flags_expand) {
				if (! wc->w)
					wc->w=1;
				wc->w=wc->w*expand/100;
			}
			if (w->flags & gravity_top) 
				wc->p.y=y;
			if (w->flags & gravity_ycenter) 
				wc->p.y=y-wc->h/2;
			if (w->flags & gravity_bottom) 
				wc->p.y=y-wc->h;
			x+=wc->w+w->spx;
			l=g_list_next(l);
		}
		break;
	case orientation_vertical:
		l=w->children;
		while (l) {
			wc=l->data;
			wc->p.y=y;
			if (wc->flags & flags_fill)
				wc->w=w->w;
			if (wc->flags & flags_expand) {
				if (! wc->h)
					wc->h=1;
				wc->h=wc->h*expand/100;
			}
			if (w->flags & gravity_left) 
				wc->p.x=x;
			if (w->flags & gravity_xcenter) 
				wc->p.x=x-wc->w/2;
			if (w->flags & gravity_right) 
				wc->p.x=x-wc->w;
			y+=wc->h+w->spy;
			l=g_list_next(l);
		}
		break;
	case orientation_horizontal_vertical:
		l=w->children;
		x0=x;
		count=0;
		width/=cols;
		height/=rows;
		while (l) {
			wc=l->data;
			wc->p.x=x;
			wc->p.y=y;
			if (wc->flags & flags_fill) {
				wc->w=width;
				wc->h=height;
			}
			if (w->flags & gravity_left) 
				wc->p.x=x;
			if (w->flags & gravity_xcenter) 
				wc->p.x=x+(width-wc->w)/2;
			if (w->flags & gravity_right) 
				wc->p.x=x+width-wc->w;
			if (w->flags & gravity_top) 
				wc->p.y=y;
			if (w->flags & gravity_ycenter) 
				wc->p.y=y+(height-wc->h)/2;
			if (w->flags & gravity_bottom) 
				wc->p.y=y-height-wc->h;
			x+=width;
			if (++count == cols) {
				count=0;
				x=x0;
				y+=height;
			}
			l=g_list_next(l);
		}
		break;
	default:
		break;
	}
	/**
	 * Call pack again on each child,
	 * the child has now had its size and coordinates
	 * set so they can repack their children.
	 */
	l=w->children;
	while (l) {
		wc=l->data;		
		gui_internal_widget_pack(this, wc);
		l=g_list_next(l);
	}
}

static void
gui_internal_widget_append(struct widget *parent, struct widget *child)
{
	if (! child)
		return;
	if (! child->background)
		child->background=parent->background;
	parent->children=g_list_append(parent->children, child);
}

static void gui_internal_widget_prepend(struct widget *parent, struct widget *child)
{
	if (! child->background)
		child->background=parent->background;
	parent->children=g_list_prepend(parent->children, child);
}

static void gui_internal_widget_children_destroy(struct gui_priv *this, struct widget *w)
{
	GList *l;
	struct widget *wc;

	l=w->children;
	while (l) {
		wc=l->data;
		gui_internal_widget_destroy(this, wc);
		l=g_list_next(l);
	}
	g_list_free(w->children);
	w->children=NULL;
}

static void gui_internal_widget_destroy(struct gui_priv *this, struct widget *w)
{
	gui_internal_widget_children_destroy(this, w);
	g_free(w->speech);
	g_free(w->text);
	if (w->img)
		graphics_image_free(this->gra, w->img);
	if (w->prefix)
		g_free(w->prefix);
	if (w->name)
		g_free(w->name);
	if (w->data_free)
		w->data_free(w->data);
	if (w->cb && w->remove_cb)
		w->remove_cb(w->instance, w->cb);
	if(w->free)
		w->free(this,w);
	else
		g_free(w);
}


static void
gui_internal_widget_render(struct gui_priv *this, struct widget *w)
{
	if(w->p.x > navit_get_width(this->nav) || w->p.y > navit_get_height(this->nav))
		return;

	switch (w->type) {
	case widget_box:
		gui_internal_box_render(this, w);
		break;
	case widget_label:
		gui_internal_label_render(this, w);
		break;
	case widget_image:
		gui_internal_image_render(this, w);
		break;
	case widget_table:
		gui_internal_table_render(this,w);
		break;
	default:
		break;
	}
}

static void
gui_internal_widget_pack(struct gui_priv *this, struct widget *w)
{
	switch (w->type) {
	case widget_box:
		gui_internal_box_pack(this, w);
		break;
	case widget_table:
	  gui_internal_table_pack(this,w);
	default:
		break;
	}
}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void gui_internal_call_highlighted(struct gui_priv *this)
{
	if (! this->highlighted || ! this->highlighted->func)
		return;
	this->highlighted->reason=1;
	this->highlighted->func(this, this->highlighted, this->highlighted->data);
}

static void
gui_internal_say(struct gui_priv *this, struct widget *w, int questionmark)
{
	char *text=w->speech;
	if (! this->speech)
		return;
	if (!text)
		text=w->text;
	if (!text)
		text=w->name;
	if (text) {
		text=g_strdup_printf("%s%c", text, questionmark ? '?':'\0');
		navit_say(this->nav, text);
		g_free(text);
	}
}

static void
gui_internal_prune_menu(struct gui_priv *this, struct widget *w)
{
	GList *l;
	struct widget *wr;
	gui_internal_search_idle_end(this);
	while ((l = g_list_last(this->root.children))) {
		if (l->data == w) {
			void (*redisplay)(struct gui_priv *priv, struct widget *widget, void *data);
			gui_internal_say(this, w, 0);
			redisplay=w->menu_data->redisplay;
			wr=w->menu_data->redisplay_widget;
			if (!w->menu_data->redisplay) {
				gui_internal_widget_render(this, w);
				return;
			}
			gui_internal_widget_destroy(this, l->data);
			this->root.children=g_list_remove(this->root.children, l->data);
			redisplay(this, wr, wr->data);
			return;
		}
		gui_internal_widget_destroy(this, l->data);
		this->root.children=g_list_remove(this->root.children, l->data);
	}
}

static void
gui_internal_prune_menu_count(struct gui_priv *this, int count, int render)
{
	GList *l;
	gui_internal_search_idle_end(this);
	while ((l = g_list_last(this->root.children)) && count-- > 0) {
		gui_internal_widget_destroy(this, l->data);
		this->root.children=g_list_remove(this->root.children, l->data);
	}
	if (l && render) {
		gui_internal_say(this, l->data, 0);
		gui_internal_widget_render(this, l->data);
	}
}

static void
gui_internal_back(struct gui_priv *this, struct widget *w, void *data)
{
	gui_internal_prune_menu_count(this, 1, 1);
}

static void
gui_internal_cmd_return(struct gui_priv *this, struct widget *wm, void *data)
{
	gui_internal_prune_menu(this, wm->data);
}

static void
gui_internal_cmd_main_menu(struct gui_priv *this, struct widget *wm, void *data)
{
	gui_internal_prune_menu(this, this->root.children->data);
}

static struct widget *
gui_internal_top_bar(struct gui_priv *this)
{
	struct widget *w,*wm,*wh,*wc,*wcn;
	int dots_len, sep_len;
	GList *res=NULL,*l;
	int width,width_used=0,use_sep=0,incomplete=0;
	struct graphics_gc *foreground=(this->flags & 256 ? this->background : this->text_foreground);
/* flags
        1:Don't expand bar to screen width
        2:Don't show Map Icon
        4:Don't show Home Icon
        8:Show only current menu
        16:Don't use menu titles as button
        32:Show navit version
	64:Show time
	128:Show help
	256:Use background for menu headline
	512:Set osd_configuration and zoom to route when setting position
*/

	w=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|(this->flags & 1 ? 0:flags_fill));
	w->bl=this->spacing;
	w->spx=this->spacing;
	w->background=this->background2;
	if ((this->flags & 6) == 6) {
		w->bl=10;
		w->br=10;
		w->bt=6;
		w->bb=6;
	}
	width=this->root.w-w->bl;
	if (! (this->flags & 2)) {
		wm=gui_internal_button_new_with_callback(this, NULL,
			image_new_s(this, "gui_map"), gravity_center|orientation_vertical,
			gui_internal_cmd_return, NULL);
		wm->speech=g_strdup(_("Back to map"));
		gui_internal_widget_pack(this, wm);
		gui_internal_widget_append(w, wm);
		width-=wm->w;
	}
	if (! (this->flags & 4)) {
		wh=gui_internal_button_new_with_callback(this, NULL,
			image_new_s(this, "gui_home"), gravity_center|orientation_vertical,
			gui_internal_cmd_main_menu, NULL);
		wh->speech=g_strdup(_("Main Menu"));
		gui_internal_widget_pack(this, wh);
		gui_internal_widget_append(w, wh);
		width-=wh->w;
	}
	if (!(this->flags & 6))
		width-=w->spx;
	l=g_list_last(this->root.children);
	wcn=gui_internal_label_new(this,".. »");
	wcn->foreground=foreground;
	dots_len=wcn->w;
	gui_internal_widget_destroy(this, wcn);
	wcn=gui_internal_label_new(this,"»");
	wcn->foreground=foreground;
	sep_len=wcn->w;
	gui_internal_widget_destroy(this, wcn);
	while (l) {
		if (g_list_previous(l) || !g_list_next(l)) {
			wc=l->data;
			wcn=gui_internal_label_new(this, wc->text);
			wcn->foreground=foreground;
			if (g_list_next(l)) 
				use_sep=1;
			else
				use_sep=0;
			dbg(1,"%d (%s) + %d + %d + %d > %d\n", wcn->w, wc->text, width_used, w->spx, use_sep ? sep_len : 0, width);
			if (wcn->w + width_used + w->spx + (use_sep ? sep_len : 0) + (g_list_previous(l) ? dots_len : 0) > width) {
				incomplete=1;
				gui_internal_widget_destroy(this, wcn);
				break;
			}
			if (use_sep) {
				struct widget *wct=gui_internal_label_new(this, "»");
				wct->foreground=foreground;
				res=g_list_prepend(res, wct);
				width_used+=sep_len+w->spx;
			}
			width_used+=wcn->w;
			if (!(this->flags & 16)) {
				wcn->func=gui_internal_cmd_return;
				wcn->data=wc;
				wcn->state |= STATE_SENSITIVE;
			}
			res=g_list_prepend(res, wcn);
			if (this->flags & 8)
				break;	
		}
		l=g_list_previous(l);
	}
	if (incomplete) {
		if (! res) {
			wcn=gui_internal_label_new_abbrev(this, wc->text, width-width_used-w->spx-dots_len);
			wcn->foreground=foreground;
			wcn->func=gui_internal_cmd_return;
			wcn->data=wc;
			wcn->state |= STATE_SENSITIVE;
			res=g_list_prepend(res, wcn);
			l=g_list_previous(l);
			wc=l->data;
		}
		wcn=gui_internal_label_new(this, ".. »");
		wcn->foreground=foreground;
		wcn->func=gui_internal_cmd_return;
		wcn->data=wc;
		wcn->state |= STATE_SENSITIVE;
		res=g_list_prepend(res, wcn);
	}
	l=res;
	while (l) {
		gui_internal_widget_append(w, l->data);
		l=g_list_next(l);
	}
	if (this->flags & 32) {
		extern char *version;
		char *version_text=g_strdup_printf("Navit %s",version);
		wcn=gui_internal_label_new(this, version_text);
		g_free(version_text);
		wcn->flags=gravity_right_center|flags_expand;
		gui_internal_widget_append(w, wcn);
	}
#if 0
	if (dots)
		gui_internal_widget_destroy(this, dots);
#endif
	return w;
}

static struct widget *
gui_internal_time_help(struct gui_priv *this)
{
	struct widget *w,*wm,*wh,*wc,*wcn;
	int dots_len, sep_len;
	GList *res=NULL,*l;
	int width,width_used=0,use_sep,incomplete=0;
	char timestr[64];
	struct tm *tm;
	time_t timep;

	w=gui_internal_box_new(this, gravity_right_center|orientation_horizontal|flags_fill);
	w->bl=this->spacing;
	w->spx=this->spacing;
	w->spx=10;
	w->bl=10;
	w->br=10;
	w->bt=6;
	w->bb=6;
	if (this->flags & 64) {
		wc=gui_internal_box_new(this, gravity_right_top|orientation_vertical|flags_fill);
		wc->bl=10;
		wc->br=20;
		wc->bt=6;
		wc->bb=6;
		timep=time(NULL);
		tm=localtime(&timep);
		strftime(timestr, 64, "%H:%M %d.%m.%Y", tm);
		wcn=gui_internal_label_new(this, timestr);
		gui_internal_widget_append(wc, wcn);
		gui_internal_widget_append(w, wc);
	}
	if (this->flags & 128) {
		wcn=gui_internal_button_new_with_callback(this, _("Help"), image_new_l(this, "gui_help"), gravity_center|orientation_vertical|flags_fill, NULL, NULL);
		gui_internal_widget_append(w, wcn);
	}
	return w;
}


/**
 * Applys the configuration values to this based on the settings
 * specified in the configuration file (this->config) and
 * the most approriate default profile based on screen resolution.
 *
 * This function should be run after this->root is setup and could
 * be rerun after the window is resized.
 *
 * @authors Steve Singer <ssinger_pg@sympatico.ca> (09/2008)
 */
static void gui_internal_apply_config(struct gui_priv *this)
{
  struct gui_config_settings *  current_config=0;

  dbg(0,"w=%d h=%d\n", this->root.w, this->root.h);
  /**
   * Select default values from profile based on the screen.
   */
  if((this->root.w > 320 || this->root.h > 320) && this->root.w > 240 && this->root.h > 240)
  {
    if((this->root.w > 640 || this->root.h > 640) && this->root.w > 480 && this->root.h > 480 )
    {
      current_config = &config_profiles[LARGE_PROFILE];
    }
    else
    {
      current_config = &config_profiles[MEDIUM_PROFILE];
    }
  }
  else
  {
    current_config = &config_profiles[SMALL_PROFILE];
  }
  
  /**
   * Apply override values from config file
   */
  if(this->config.font_size == -1 )
  {
    this->font_size = current_config->font_size;
  }
  else
  {
    this->font_size = this->config.font_size;
  }

  if(this->config.icon_xs == -1 )
  {
      this->icon_xs = current_config->icon_xs;
  }
  else
  {
    this->icon_xs = this->config.icon_xs;
  }
  
  if(this->config.icon_s == -1 )
  {
    this->icon_s = current_config->icon_s;
  }
  else
  {
    this->icon_s = this->config.icon_s;
  }
  if(this->config.icon_l == -1 )
  {
    this->icon_l = current_config->icon_l;
  }
  else
  {
    this->icon_l = this->config.icon_l;
  }
  if(this->config.spacing == -1 )
  {
    this->spacing = current_config->spacing;
  }
  else
  {
    this->spacing = current_config->spacing;
  }
       	
}

static struct widget *
gui_internal_button_label(struct gui_priv *this, char *label, int mode)
{
	struct widget *wl,*wlb;
	struct widget *wb=gui_internal_menu_data(this)->button_bar;
	wlb=gui_internal_box_new(this, gravity_right_center|orientation_vertical);
	wl=gui_internal_label_new(this, label);
	wlb->border=1;
	wlb->foreground=this->text_foreground;
	wlb->bl=20;
	wlb->br=20;
	wlb->bb=6;
	wlb->bt=6;
	gui_internal_widget_append(wlb, wl);
	if (mode == 1)
		gui_internal_widget_prepend(wb, wlb);
	if (mode == -1)
		gui_internal_widget_append(wb, wlb);

	return wlb;
}


static struct widget *
gui_internal_menu(struct gui_priv *this, char *label)
{
	struct widget *menu,*w,*w1,*topbox;

	gui_internal_search_idle_end(this);
	topbox=gui_internal_box_new_with_label(this, 0, label);
        topbox->w=this->root.w;
        topbox->h=this->root.h;
        gui_internal_widget_append(&this->root, topbox);
	menu=gui_internal_box_new(this, gravity_left_center|orientation_vertical);
	menu->w=this->root.w;
	menu->h=this->root.h;
	menu->background=this->background;
	gui_internal_apply_config(this);
	this->font=graphics_font_new(this->gra,this->font_size,1);
 	topbox->menu_data=g_new0(struct menu_data, 1);
	gui_internal_widget_append(topbox, menu);
	w=gui_internal_top_bar(this);
	gui_internal_widget_append(menu, w);
	w=gui_internal_box_new(this, gravity_center|orientation_horizontal_vertical|flags_expand|flags_fill);
	w->spx=4*this->spacing;
	w->w=menu->w;
	gui_internal_widget_append(menu, w);
	if (this->flags & 16) {
		struct widget *wl,*wlb,*wb,*wm=w;
		wm->flags=gravity_center|orientation_vertical|flags_expand|flags_fill;
		w=gui_internal_box_new(this, gravity_center|orientation_horizontal|flags_expand|flags_fill);
		dbg(0,"topbox->menu_data=%p\n", topbox->menu_data);
		gui_internal_widget_append(wm, w);
		wb=gui_internal_box_new(this, gravity_right_center|orientation_horizontal|flags_fill);
		wb->bl=6;
		wb->br=6;
		wb->bb=6;
		wb->bt=6;
		wb->spx=6;
		topbox->menu_data->button_bar=wb;
		gui_internal_widget_append(wm, wb);
		wlb=gui_internal_button_label(this,_("Back"),1);
		wlb->func=gui_internal_back;
		wlb->state |= STATE_SENSITIVE;
	}
	if (this->flags & 192) {
		menu=gui_internal_box_new(this, gravity_left_center|orientation_vertical);
		menu->w=this->root.w;
		menu->h=this->root.h;
		w1=gui_internal_time_help(this);
		gui_internal_widget_append(menu, w1);
		w1=gui_internal_box_new(this, gravity_center|orientation_horizontal_vertical|flags_expand|flags_fill);
		gui_internal_widget_append(menu, w1);
		gui_internal_widget_append(topbox, menu);
		menu->background=NULL;
	}
	return w;
}

static struct menu_data *
gui_internal_menu_data(struct gui_priv *this)
{
	GList *l;
	struct widget *menu;

	l=g_list_last(this->root.children);
	menu=l->data;
	return menu->menu_data;
}

static void
gui_internal_menu_render(struct gui_priv *this)
{
	GList *l;
	struct widget *menu;

	l=g_list_last(this->root.children);
	menu=l->data;
	gui_internal_say(this, menu, 0);
	gui_internal_widget_pack(this, menu);
	gui_internal_widget_render(this, menu);
}

static void
gui_internal_cmd_set_destination(struct gui_priv *this, struct widget *wm, void *data)
{
	struct widget *w=wm->data;
	dbg(0,"c=%d:0x%x,0x%x\n", w->c.pro, w->c.x, w->c.y);
	navit_set_destination(this->nav, &w->c, w->name);
	if (this->flags & 512) {
		struct attr follow;
		follow.type=attr_follow;
		follow.u.num=180;
		navit_set_attr(this->nav, &this->osd_configuration);
		navit_set_attr(this->nav, &follow);
		navit_zoom_to_route(this->nav, 0);
	}	
	gui_internal_prune_menu(this, NULL);
}

static void
gui_internal_cmd_set_position(struct gui_priv *this, struct widget *wm, void *data)
{
	struct widget *w=wm->data;
	navit_set_position(this->nav, &w->c);
	gui_internal_prune_menu(this, NULL);
}

static void
gui_internal_cmd_add_bookmark_do(struct gui_priv *this, struct widget *widget)
{
	GList *l;
	dbg(0,"text='%s'\n", widget->text);
	if (widget->text && strlen(widget->text)) 
		navit_add_bookmark(this->nav, &widget->c, widget->text);
	g_free(widget->text);
	widget->text=NULL;
	l=g_list_previous(g_list_last(this->root.children));
	gui_internal_prune_menu(this, l->data);
}

static void
gui_internal_cmd_add_bookmark_clicked(struct gui_priv *this, struct widget *widget, void *data)
{
	gui_internal_cmd_add_bookmark_do(this, widget->data);
}

static void
gui_internal_cmd_add_bookmark_changed(struct gui_priv *this, struct widget *wm, void *data)
{
	int len;
	dbg(1,"enter\n");
	if (wm->text) {
		len=strlen(wm->text);
		dbg(1,"len=%d\n", len);
		if (len && (wm->text[len-1] == '\n' || wm->text[len-1] == '\r')) {
			wm->text[len-1]='\0';
			gui_internal_cmd_add_bookmark_do(this, wm);
		}
	}
}


static struct widget * gui_internal_keyboard(struct gui_priv *this, int mode);

static void
gui_internal_cmd_add_bookmark(struct gui_priv *this, struct widget *wm, void *data)
{
	struct widget *w,*wb,*wk,*wl,*we,*wnext,*wp=wm->data;
	wb=gui_internal_menu(this, "Add Bookmark");	
	w=gui_internal_box_new(this, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(wb, w);
	we=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|flags_fill);
	gui_internal_widget_append(w, we);
	gui_internal_widget_append(we, wk=gui_internal_label_new(this, wp->name ? wp->name : wp->text));
	wk->state |= STATE_EDIT|STATE_CLEAR;
	wk->background=this->background;
	wk->flags |= flags_expand|flags_fill;
	wk->func = gui_internal_cmd_add_bookmark_changed;
	wk->c=wm->c;
	gui_internal_widget_append(we, wnext=gui_internal_image_new(this, image_new_xs(this, "gui_active")));
	wnext->state |= STATE_SENSITIVE;
	wnext->func = gui_internal_cmd_add_bookmark_clicked;
	wnext->data=wk;
	wl=gui_internal_box_new(this, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(w, wl);
	if (this->keyboard)
		gui_internal_widget_append(w, gui_internal_keyboard(this,2));
	gui_internal_menu_render(this);
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

struct selector {
	char *icon;
	char *name;
	enum item_type *types;
} selectors[]={
	{"bank","Bank",(enum item_type []){type_poi_bank,type_poi_bank,type_none}},
	{"fuel","Fuel",(enum item_type []){type_poi_fuel,type_poi_fuel,type_none}},
	{"hotel","Hotel",(enum item_type []) {
		type_poi_hotel,type_poi_camp_rv,
		type_poi_camping,type_poi_camping,
		type_poi_resort,type_poi_resort,
		type_none}},
	{"restaurant","Restaurant",(enum item_type []) {
		type_poi_bar,type_poi_picnic,
		type_poi_burgerking,type_poi_fastfood,
		type_poi_restaurant,type_poi_restaurant,
		type_none}},
	{"shopping","Shopping",(enum item_type []) {
		type_poi_mall,type_poi_mall,
		type_poi_shop_grocery,type_poi_shop_grocery,
		type_none}},
	{"hospital","Service",(enum item_type []) {
		type_poi_marina,type_poi_marina,
		type_poi_hospital,type_poi_hospital,
		type_poi_public_utilities,type_poi_public_utilities,
		type_poi_police,type_poi_autoservice,
		type_poi_information,type_poi_information,
		type_poi_personal_service,type_poi_repair_service,
		type_poi_rest_room,type_poi_rest_room,
		type_poi_restroom,type_poi_restroom,
		type_none}},
	{"parking","Parking",(enum item_type []){type_poi_car_parking,type_poi_car_parking,type_none}},
	{"peak","Land Features",(enum item_type []){
		type_poi_land_feature,type_poi_rock,
		type_poi_dam,type_poi_dam,
		type_none}},
	{"unknown","Other",(enum item_type []){
		type_point_unspecified,type_poi_land_feature-1,
		type_poi_rock+1,type_poi_fuel-1,
		type_poi_marina+1,type_poi_car_parking-1,
		type_poi_car_parking+1,type_poi_bar-1,
		type_poi_bank+1,type_poi_dam-1,
		type_poi_dam+1,type_poi_information-1,
		type_poi_information+1,type_poi_mall-1,
		type_poi_mall+1,type_poi_personal_service-1,
		type_poi_restaurant+1,type_poi_restroom-1,
		type_poi_restroom+1,type_poi_shop_grocery-1,
		type_poi_shop_grocery+1,type_poi_wifi,
		type_none}},
};

static void gui_internal_cmd_pois(struct gui_priv *this, struct widget *wm, void *data);

static struct widget *
gui_internal_cmd_pois_selector(struct gui_priv *this, struct pcoord *c)
{
	struct widget *wl,*wb;
	int i;
	wl=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|flags_fill);
	for (i = 0 ; i < sizeof(selectors)/sizeof(struct selector) ; i++) {
	gui_internal_widget_append(wl, wb=gui_internal_button_new_with_callback(this, NULL,
		image_new_xs(this, selectors[i].icon), gravity_left_center|orientation_vertical,
		gui_internal_cmd_pois, &selectors[i]));
		wb->c=*c;
		wb->bt=10;
	}
	return wl;
}

static struct widget *
gui_internal_cmd_pois_item(struct gui_priv *this, struct coord *center, struct item *item, struct coord *c, int dist)
{
	char distbuf[32];
	char dirbuf[32];
	char *type;
	struct attr attr;
	struct widget *wl;
	char *text;

	wl=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|flags_fill);

	sprintf(distbuf,"%d", dist/1000);
	get_direction(dirbuf, transform_get_angle_delta(center, c, 0), 1);
	type=item_to_name(item->type);
	if (item_attr_get(item, attr_label, &attr)) {
		wl->name=g_strdup_printf("%s %s",type,attr.u.str);
	} else {
		attr.u.str="";
		wl->name=g_strdup(type);
	}
        text=g_strdup_printf("%s %s %s %s", distbuf, dirbuf, type, attr.u.str);
	gui_internal_widget_append(wl, gui_internal_label_new(this, text));
	g_free(text);

	return wl;
}

static gint
gui_internal_cmd_pois_sort_num(gconstpointer a, gconstpointer b, gpointer user_data)
{
	const struct widget *wa=a;
	const struct widget *wb=b;
	struct widget *wac=wa->children->data;
	struct widget *wbc=wb->children->data;
	int ia,ib;
	ia=atoi(wac->text);
	ib=atoi(wbc->text);

	return ia-ib;
}

static int
gui_internal_cmd_pois_item_selected(struct selector *sel, enum item_type type)
{
	enum item_type *types;
	if (type >= type_line)
		return 0;
	if (! sel || !sel->types)
		return 1;
	types=sel->types;
	while (*types != type_none) {
		if (type >= types[0] && type <= types[1]) {
			return 1;
		}
		types+=2;	
	}	
	return 0;
}

static void gui_internal_cmd_position(struct gui_priv *this, struct widget *wm, void *data);

static void
gui_internal_cmd_pois(struct gui_priv *this, struct widget *wm, void *data)
{
	struct map_selection *sel,*selm;
	struct coord c,center;
	struct mapset_handle *h;
	struct map *m;
	struct map_rect *mr;
	struct item *item;
	int idist,dist=4000;
	struct widget *wi,*w,*w2,*wb;
	enum projection pro=wm->c.pro;
	struct selector *isel=wm->data;

	wb=gui_internal_menu(this, isel ? isel->name : _("POIs"));
	w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(wb, w);
	if (! isel)
		gui_internal_widget_append(w, gui_internal_cmd_pois_selector(this,&wm->c));
	w2=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(w, w2);

	sel=map_selection_rect_new(&wm->c, dist, 18);
	center.x=wm->c.x;
	center.y=wm->c.y;
	h=mapset_open(navit_get_mapset(this->nav));
        while ((m=mapset_next(h, 1))) {
		selm=map_selection_dup_pro(sel, pro, map_projection(m));
		mr=map_rect_new(m, selm);
		dbg(2,"mr=%p\n", mr);
		if (mr) {
			while ((item=map_rect_get_item(mr))) {
				if (gui_internal_cmd_pois_item_selected(isel, item->type) && 
				    item_coord_get_pro(item, &c, 1, pro) && 
				    coord_rect_contains(&sel->u.c_rect, &c) && 
				    (idist=transform_distance(pro, &center, &c)) < dist) {
					gui_internal_widget_append(w2, wi=gui_internal_cmd_pois_item(this, &center, item, &c, idist));
					wi->func=gui_internal_cmd_position;
					wi->state |= STATE_SENSITIVE;
					wi->c.x=c.x;
					wi->c.y=c.y;
					wi->c.pro=pro;
				}
			}
			map_rect_destroy(mr);
		}
		map_selection_destroy(selm);
	}
	map_selection_destroy(sel);
	mapset_close(h);
	w2->children=g_list_sort_with_data(w2->children,  gui_internal_cmd_pois_sort_num, (void *)1);
	gui_internal_menu_render(this);
}

static void
gui_internal_cmd_view_on_map(struct gui_priv *this, struct widget *wm, void *data)
{
	struct widget *w=wm->data;
	int highlight=(w->data == (void *)2 || w->data == (void *)3 || w->data == (void *)4);
	if (highlight) {
		graphics_clear_selection(this->gra, NULL);
		graphics_add_selection(this->gra, &w->item, NULL);
	}
	navit_set_center(this->nav, &w->c);
	gui_internal_prune_menu(this, NULL);
}


static void
gui_internal_cmd_view_attributes(struct gui_priv *this, struct widget *wm, void *data)
{
	struct widget *w,*wb;
	struct map_rect *mr;
	struct item *item;
	struct attr attr;
	char *text;

	dbg(0,"item=%p 0x%x 0x%x\n", wm->item.map,wm->item.id_hi, wm->item.id_lo);
	wb=gui_internal_menu(this, "Attributes");
	w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(wb, w);
	mr=map_rect_new(wm->item.map, NULL);
	item = map_rect_get_item_byid(mr, wm->item.id_hi, wm->item.id_lo);
	dbg(0,"item=%p\n", item);
	if (item) {
		while(item_attr_get(item, attr_any, &attr)) {
			text=g_strdup_printf("%s:%s", attr_to_name(attr.type), attr_to_text(&attr, wm->item.map, 1));
			gui_internal_widget_append(w,
			wb=gui_internal_button_new_with_callback(this, text,
				NULL, gravity_left_center|orientation_horizontal|flags_fill,
				gui_internal_cmd_view_attributes, NULL));
			g_free(text);
		}
	}
	map_rect_destroy(mr);
	gui_internal_menu_render(this);
}

static void
gui_internal_cmd_view_in_browser(struct gui_priv *this, struct widget *wm, void *data)
{
	struct map_rect *mr;
	struct item *item;
	struct attr attr;
	char *cmd=NULL;

	dbg(0,"item=%p 0x%x 0x%x\n", wm->item.map,wm->item.id_hi, wm->item.id_lo);
	mr=map_rect_new(wm->item.map, NULL);
	item = map_rect_get_item_byid(mr, wm->item.id_hi, wm->item.id_lo);
	dbg(0,"item=%p\n", item);
	if (item) {
		while(item_attr_get(item, attr_url_local, &attr)) {
			if (! cmd)
				cmd=g_strdup_printf("navit-browser.sh '%s' &",attr.u.str);
		}
	}
	map_rect_destroy(mr);
	if (cmd) {
#ifdef HAVE_SYSTEM
		system(cmd);
#else
		dbg(0,"calling external cmd '%s' is not supported\n",cmd);
#endif
		g_free(cmd);
	}
}

/* wm->data: 0 Nothing special
	     1 Map Point
	     2 Item
	     3 Town 
*/


static void
gui_internal_cmd_position(struct gui_priv *this, struct widget *wm, void *data)
{
	struct widget *wb,*w,*wc,*wbc;
	struct coord_geo g;
	struct coord c;
	char *coord,*name;
	int display_attributes=(wm->data == (void *)2);
	int display_view_on_map=(wm->data != (void *)1);
	int display_items=(wm->data == (void *)1);
	int display_streets=(wm->data == (void *)3);
	if (wm->data == (void *)4) {
		gui_internal_search_town_in_country(this, wm);
		return;
	}
#if 0
	switch ((int)wm->data) {
		case 0:
#endif
			c.x=wm->c.x;
			c.y=wm->c.y;
			dbg(0,"x=0x%x y=0x%x\n", c.x, c.y);
			transform_to_geo(wm->c.pro, &c, &g);
#if 0
			break;
		case 1:
			g=this->click;
			break;
		case 2:
			g=this->vehicle;
			break;
	}
#endif
	name=wm->name ? wm->name : wm->text;
	wb=gui_internal_menu(this, name);
	w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(wb, w);
	coord=coordinates(&wm->c, ' ');
	gui_internal_widget_append(w, gui_internal_label_new(this, coord));
	g_free(coord);
	if (display_streets) {
		gui_internal_widget_append(w,
			wc=gui_internal_button_new_with_callback(this, _("Streets"),
				image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
				gui_internal_search_street_in_town, wm));
		wc->item=wm->item;
	}
	if (display_attributes) {
		struct map_rect *mr;
		struct item *item;
		struct attr attr;
		mr=map_rect_new(wm->item.map, NULL);
		item = map_rect_get_item_byid(mr, wm->item.id_hi, wm->item.id_lo);
		if (item) {
			if (item_attr_get(item, attr_description, &attr)) 
				gui_internal_widget_append(w, gui_internal_label_new(this, attr.u.str));
			if (item_attr_get(item, attr_url_local, &attr)) {
				gui_internal_widget_append(w,
					wb=gui_internal_button_new_with_callback(this, _("View in Browser"),
						image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
						gui_internal_cmd_view_in_browser, NULL));
				wb->item=wm->item;
			}
			gui_internal_widget_append(w,
				wb=gui_internal_button_new_with_callback(this, _("View Attributes"),
					image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
					gui_internal_cmd_view_attributes, NULL));
			wb->item=wm->item;
		}
		map_rect_destroy(mr);
	}
	gui_internal_widget_append(w,
		gui_internal_button_new_with_callback(this, _("Set as destination"),
			image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
			gui_internal_cmd_set_destination, wm));
	gui_internal_widget_append(w,
		gui_internal_button_new_with_callback(this, _("Set as position"),
			image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
			gui_internal_cmd_set_position, wm));
	gui_internal_widget_append(w,
		wbc=gui_internal_button_new_with_callback(this, _("Add as bookmark"),
			image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
			gui_internal_cmd_add_bookmark, wm));
	wbc->c=wm->c;
	gui_internal_widget_append(w,
		wbc=gui_internal_button_new_with_callback(this, _("POIs"),
			image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
			gui_internal_cmd_pois, NULL));
	wbc->c=wm->c;
#if 0
	gui_internal_widget_append(w,
		gui_internal_button_new(this, "Add to tour",
			image_new_o(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill));
	gui_internal_widget_append(w,
		gui_internal_button_new(this, "Add as bookmark",
			image_new_o(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill));
#endif
	if (display_view_on_map) {
		gui_internal_widget_append(w,
			gui_internal_button_new_with_callback(this, _("View on map"),
				image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
				gui_internal_cmd_view_on_map, wm));
	}
	if (display_items) {
		int dist=10;
		struct mapset *ms;
		struct mapset_handle *h;
		struct map_rect *mr;
		struct map *m;
		struct item *item;
		struct street_data *data;
		struct map_selection sel;
		struct transformation *trans;
		enum projection pro;
		struct attr attr;
		char *label,*text;

		trans=navit_get_trans(this->nav);
		pro=transform_get_projection(trans);
		transform_from_geo(pro, &g, &c);
		ms=navit_get_mapset(this->nav);
		sel.next=NULL;
		sel.u.c_rect.lu.x=c.x-dist;
		sel.u.c_rect.lu.y=c.y+dist;
		sel.u.c_rect.rl.x=c.x+dist;
		sel.u.c_rect.rl.y=c.y-dist;
		sel.order=18;
		sel.range=item_range_all;
		h=mapset_open(ms);
		while ((m=mapset_next(h,1))) {
			mr=map_rect_new(m, &sel);
			if (! mr)
				continue;
			while ((item=map_rect_get_item(mr))) {
				data=street_get_data(item);
				if (transform_within_dist_item(&c, item->type, data->c, data->count, dist)) {
					if (item_attr_get(item, attr_label, &attr)) {
						label=map_convert_string(m, attr.u.str);
						text=g_strdup_printf("%s %s", item_to_name(item->type), label);
						map_convert_free(label);
					} else 
						text=g_strdup_printf("%s", item_to_name(item->type));
					gui_internal_widget_append(w,
						wc=gui_internal_button_new_with_callback(this, text,
						image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
						gui_internal_cmd_position, (void *)2));
					wc->c.x=data->c[0].x;
					wc->c.y=data->c[0].y;
					wc->c.pro=pro;
					wc->name=g_strdup(text);
					wc->item=*item;
					g_free(text);
				}
				street_data_free(data);
			}
			map_rect_destroy(mr);
		}
		mapset_close(h);
	}
	
	gui_internal_menu_render(this);
}

static void
gui_internal_cmd_bookmarks(struct gui_priv *this, struct widget *wm, void *data)
{
	struct attr attr,mattr;
	struct map_rect *mr=NULL;
	struct item *item;
	char *label_full,*l,*prefix,*pos;
	int len,plen,hassub;
	struct widget *wb,*w,*wbm;
	GHashTable *hash;
	struct coord c;


	wb=gui_internal_menu(this, wm->text ? wm->text : _("Bookmarks"));
	w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	w->spy=this->spacing*3;
	gui_internal_widget_append(wb, w);

	prefix=wm->prefix;
	if (! prefix)
		prefix="";
	plen=strlen(prefix);

	if(navit_get_attr(this->nav, attr_bookmark_map, &mattr, NULL) && mattr.u.map && (mr=map_rect_new(mattr.u.map, NULL))) {
		hash=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
		while ((item=map_rect_get_item(mr))) {
			if (item->type != type_bookmark) continue;
			if (!item_attr_get(item, attr_label, &attr)) continue;
			label_full=attr.u.str;
			if (!strncmp(label_full, prefix, plen)) {
				pos=strchr(label_full+plen, '/');
				if (pos) {
					hassub=1;
					len=pos-label_full;
				} else {
					hassub=0;
					len=strlen(label_full);
				}
				l=g_malloc(len-plen+1);
				strncpy(l, label_full+plen, len-plen);
				l[len-plen]='\0';
				if (!g_hash_table_lookup(hash, l)) {
					wbm=gui_internal_button_new_with_callback(this, l,
						image_new_xs(this, hassub ? "gui_inactive" : "gui_active" ), gravity_left_center|orientation_horizontal|flags_fill,
							hassub ? gui_internal_cmd_bookmarks : gui_internal_cmd_position, NULL);
					if (item_coord_get(item, &c, 1)) {
						wbm->c.x=c.x;
						wbm->c.y=c.y;
						wbm->c.pro=map_projection(mattr.u.map);
						wbm->name=g_strdup_printf(_("Bookmark %s"),label_full);
						wbm->text=g_strdup(l);
						gui_internal_widget_append(w, wbm);
						g_hash_table_insert(hash, g_strdup(l), (void *)1);
						wbm->prefix=g_malloc(len+2);
						strncpy(wbm->prefix, label_full, len+1);
						wbm->prefix[len+1]='\0';
					} else {
						gui_internal_widget_destroy(this, wbm);
					}
				}
				g_free(l);
			}

		}
		g_hash_table_destroy(hash);
	}
	gui_internal_menu_render(this);
}

static void gui_internal_keypress_do(struct gui_priv *this, char *key)
{
	struct widget *wi,*menu;
	int len=0;
	char *text=NULL;

	menu=g_list_last(this->root.children)->data;
	wi=gui_internal_find_widget(menu, NULL, STATE_EDIT);
	if (wi) {
		if (*key == NAVIT_KEY_BACKSPACE) {
			dbg(0,"backspace\n");
			if (wi->text && wi->text[0]) {
				len=g_utf8_prev_char(wi->text+strlen(wi->text))-wi->text;
				wi->text[len]=' ';	
				text=g_strdup_printf("%s ", wi->text);
			}
		} else {
			if (wi->state & STATE_CLEAR) {
				dbg(0,"wi->state=0x%x\n", wi->state);
				g_free(wi->text);
				wi->text=NULL;
				wi->state &= ~STATE_CLEAR;
				dbg(0,"wi->state=0x%x\n", wi->state);
			}
			text=g_strdup_printf("%s%s", wi->text ? wi->text : "", key);
		}
		g_free(wi->text);
		wi->text=text;
		if (*key == NAVIT_KEY_BACKSPACE && wi->text) {
			gui_internal_widget_render(this, wi);
			wi->text[len]='\0';
		}
		if (wi->func) {
			wi->reason=2;
			wi->func(this, wi, wi->data);
		}
		gui_internal_widget_render(this, wi);
	}
}


static void
gui_internal_cmd_keypress(struct gui_priv *this, struct widget *wm, void *data)
{
	struct menu_data *md=gui_internal_menu_data(this);
	gui_internal_keypress_do(this, (char *) wm->data);
	if (md->keyboard_mode == 2) 
		gui_internal_keyboard_do(this, md->keyboard, 10);
}

static void
gui_internal_search_idle_end(struct gui_priv *this)
{
	if (this->idle) {
		event_remove_idle(this->idle);
		this->idle=NULL;
	}
	if (this->idle_cb) {
		callback_destroy(this->idle_cb);
		this->idle_cb=NULL;
	}
}

static void
gui_internal_search_idle(struct gui_priv *this, char *wm_name, struct widget *search_list, void *param)
{
	char *text=NULL,*name=NULL;
	struct search_list_result *res;
	struct widget *wc;
	struct item *item=NULL;
	GList *l;

	res=search_list_get_result(this->sl);
	if (! res) {
		gui_internal_search_idle_end(this);
		return;
	}

	if (! strcmp(wm_name,"Country")) {
		name=res->country->name;
		item=&res->country->item;
		text=g_strdup_printf("%s", res->country->name);
	}
	if (! strcmp(wm_name,"Town")) {
		name=res->town->name;
		item=&res->town->item;
		text=g_strdup(name);
	}
	if (! strcmp(wm_name,"Street")) {
		name=res->street->name;
		item=&res->street->item;
		text=g_strdup_printf("%s %s", res->town->name, res->street->name);
	}
#if 0
			dbg(0,"res=%s\n", res->town->name);
#endif
	dbg(1,"res->country->flag=%s\n", res->country->flag);
	gui_internal_widget_append(search_list,
		wc=gui_internal_button_new_with_callback(this, text,
		image_new_xs(this, res->country->flag), gravity_left_center|orientation_horizontal|flags_fill,
		gui_internal_cmd_position, param));
	g_free(text);
              
	wc->name=g_strdup(name);
	if (res->c)
	  wc->c=*res->c;
	wc->selection_id=res->id;
	if (item)
		wc->item=*item;
	gui_internal_widget_pack(this, search_list);
	l=g_list_last(this->root.children);
	graphics_draw_mode(this->gra, draw_mode_begin);
	gui_internal_widget_render(this, l->data);
	graphics_draw_mode(this->gra, draw_mode_end);
}

static void
gui_internal_search_idle_start(struct gui_priv *this, char *wm_name, struct widget *search_list, void *param)
{
	this->idle_cb=callback_new_4(callback_cast(gui_internal_search_idle), this, wm_name, search_list, param);
	this->idle=event_add_idle(50,this->idle_cb);
	callback_call_0(this->idle_cb);
}


/**
 *
 * @param wm The widget that generated the event for the search changed,
 *        if this was generated by a key on the virtual keyboard then
 *        wm is the key button widget.
 */
static void
gui_internal_search_changed(struct gui_priv *this, struct widget *wm, void *data)
{
	GList *l;
	struct widget *search_list=gui_internal_menu_data(this)->search_list;
	gui_internal_widget_children_destroy(this, search_list);

	void *param=(void *)3;
	int minlen=1;
	if (! strcmp(wm->name,"Country")) {
		param=(void *)4;
		minlen=1;
	}
	dbg(0,"%s now '%s'\n", wm->name, wm->text);

	gui_internal_search_idle_end(this);
	if (wm->text && g_utf8_strlen(wm->text, -1) >= minlen) {
		struct attr search_attr;

		dbg(0,"process\n");
		if (! strcmp(wm->name,"Country"))
			search_attr.type=attr_country_all;
		if (! strcmp(wm->name,"Town"))
			search_attr.type=attr_town_name;
		if (! strcmp(wm->name,"Street"))
			search_attr.type=attr_street_name;
		search_attr.u.str=wm->text;
		search_list_search(this->sl, &search_attr, 1);
		gui_internal_search_idle_start(this, wm->name, search_list, param);
	}
	l=g_list_last(this->root.children);
	gui_internal_widget_render(this, l->data);
}

static struct widget *
gui_internal_keyboard_key_data(struct gui_priv *this, struct widget *wkbd, char *text, void(*func)(struct gui_priv *priv, struct widget *widget, void *data), void *data, void (*data_free)(void *data), int w, int h)
{
	struct widget *wk;
	gui_internal_widget_append(wkbd, wk=gui_internal_button_new_with_callback(this, text,
		NULL, gravity_center|orientation_vertical, func, data));
	wk->data_free=data_free;
	wk->background=this->background;
	wk->bl=w/2;
	wk->br=0;
	wk->bt=h/2;
	wk->bb=0;
	return wk;
}

static struct widget *
gui_internal_keyboard_key(struct gui_priv *this, struct widget *wkbd, char *text, char *key, int w, int h)
{
	return gui_internal_keyboard_key_data(this, wkbd, text, gui_internal_cmd_keypress, g_strdup(key), g_free,w,h);
}

static void gui_internal_keyboard_change(struct gui_priv *this, struct widget *key, void *data);

static struct widget *
gui_internal_keyboard_do(struct gui_priv *this, struct widget *wkbdb, int mode)
{
	struct widget *wkbd,*wk;
	struct menu_data *md=gui_internal_menu_data(this);
	int i, max_w=navit_get_width(this->nav), max_h=navit_get_height(this->nav);
	int render=0;

	if (wkbdb) {
		this->current.x=-1;
		this->current.y=-1;
		gui_internal_highlight(this);
		render=1;
		gui_internal_widget_children_destroy(this, wkbdb);
	} else
		wkbdb=gui_internal_box_new(this, gravity_center|orientation_horizontal_vertical|flags_fill);
	md->keyboard=wkbdb;
	md->keyboard_mode=mode;
	wkbd=gui_internal_box_new(this, gravity_center|orientation_horizontal_vertical|flags_fill);
	wkbd->background=this->background;
	wkbd->cols=8;
	wkbd->spx=3;
	wkbd->spy=3;
	max_w=max_w/9;
	max_h=max_h/6;

	if (mode >= 0 && mode < 8) {
		for (i = 0 ; i < 26 ; i++) {
			char text[]={'A'+i,'\0'};
			gui_internal_keyboard_key(this, wkbd, text, text,max_w,max_h);
		}
		gui_internal_keyboard_key(this, wkbd, "_"," ",max_w,max_h);
		if (mode == 0) {
			gui_internal_keyboard_key(this, wkbd, "-","-",max_w,max_h);
			gui_internal_keyboard_key(this, wkbd, "'","'",max_w,max_h);
			gui_internal_keyboard_key_data(this, wkbd, "", NULL, NULL, NULL,max_w,max_h);
		} else {
			gui_internal_keyboard_key_data(this, wkbd, "", NULL, NULL, NULL,max_w,max_h);
			wk=gui_internal_keyboard_key_data(this, wkbd, "a", gui_internal_keyboard_change, wkbd, NULL,max_w,max_h);
			wk->datai=mode+8;
			wk=gui_internal_keyboard_key_data(this, wkbd, "1", gui_internal_keyboard_change, wkbd, NULL,max_w,max_h);
			wk->datai=mode+16;
		}
		wk=gui_internal_keyboard_key_data(this, wkbd, "Ä",gui_internal_keyboard_change, wkbdb,NULL,max_w,max_h);
		wk->datai=mode+24;
		gui_internal_keyboard_key(this, wkbd, "<-","\b",max_w,max_h);
	}
	if (mode >= 8 && mode < 16) {
		for (i = 0 ; i < 26 ; i++) {
			char text[]={'a'+i,'\0'};
			gui_internal_keyboard_key(this, wkbd, text, text,max_w,max_h);
		}
		gui_internal_keyboard_key(this, wkbd, "_"," ",max_w,max_h);
		if (mode == 8) {
			gui_internal_keyboard_key(this, wkbd, "-","-",max_w,max_h);
			gui_internal_keyboard_key(this, wkbd, "'","'",max_w,max_h);
			gui_internal_keyboard_key_data(this, wkbd, "", NULL, NULL, NULL,max_w,max_h);
		} else {
			gui_internal_keyboard_key_data(this, wkbd, "", NULL, NULL, NULL,max_w,max_h);
			wk=gui_internal_keyboard_key_data(this, wkbd, "A", gui_internal_keyboard_change, wkbd, NULL,max_w,max_h);
			wk->datai=mode-8;
			wk=gui_internal_keyboard_key_data(this, wkbd, "1", gui_internal_keyboard_change, wkbd, NULL,max_w,max_h);
			wk->datai=mode+8;
		}
		wk=gui_internal_keyboard_key_data(this, wkbd, "ä",gui_internal_keyboard_change,wkbdb,NULL,max_w,max_h);
		wk->datai=mode+24;
		gui_internal_keyboard_key(this, wkbd, "<-","\b",max_w,max_h);
	}
	if (mode >= 16 && mode < 24) {
		for (i = 0 ; i < 10 ; i++) {
			char text[]={'0'+i,'\0'};
			gui_internal_keyboard_key(this, wkbd, text, text,max_w,max_h);
		}
		gui_internal_keyboard_key(this, wkbd, ".",".",max_w,max_h);
		gui_internal_keyboard_key(this, wkbd, "°","°",max_w,max_h);
		gui_internal_keyboard_key(this, wkbd, "'","'",max_w,max_h);
		gui_internal_keyboard_key(this, wkbd, "\"","\"",max_w,max_h);
		gui_internal_keyboard_key(this, wkbd, "-","-",max_w,max_h);
		gui_internal_keyboard_key(this, wkbd, "+","+",max_w,max_h);
		gui_internal_keyboard_key(this, wkbd, "*","*",max_w,max_h);
		gui_internal_keyboard_key(this, wkbd, "/","/",max_w,max_h);
		gui_internal_keyboard_key(this, wkbd, "(","(",max_w,max_h);
		gui_internal_keyboard_key(this, wkbd, ")",")",max_w,max_h);
		gui_internal_keyboard_key(this, wkbd, "=","=",max_w,max_h);
		gui_internal_keyboard_key(this, wkbd, "?","?",max_w,max_h);
		for (i = 0 ; i < 5 ; i++) {
			gui_internal_keyboard_key_data(this, wkbd, "", NULL, NULL, NULL,max_w,max_h);
		}
		if (mode == 8) {
			gui_internal_keyboard_key(this, wkbd, "-","-",max_w,max_h);
			gui_internal_keyboard_key(this, wkbd, "'","'",max_w,max_h);
			gui_internal_keyboard_key_data(this, wkbd, "", NULL, NULL, NULL,max_w,max_h);
		} else {
			gui_internal_keyboard_key_data(this, wkbd, "", NULL, NULL, NULL,max_w,max_h);
			wk=gui_internal_keyboard_key_data(this, wkbd, "A", gui_internal_keyboard_change, wkbd, NULL,max_w,max_h);
			wk->datai=mode-16;
			wk=gui_internal_keyboard_key_data(this, wkbd, "a", gui_internal_keyboard_change, wkbd, NULL,max_w,max_h);
			wk->datai=mode-8;
		}
		wk=gui_internal_keyboard_key_data(this, wkbd, "Ä",gui_internal_keyboard_change,wkbdb,NULL,max_w,max_h);
		wk->datai=mode+8;
		gui_internal_keyboard_key(this, wkbd, "<-","\b",max_w,max_h);
	}
	if (mode >= 24 && mode < 32) {
		gui_internal_keyboard_key(this, wkbd, "Ä","Ä",max_w,max_h);
		gui_internal_keyboard_key(this, wkbd, "Ö","Ö",max_w,max_h);
		gui_internal_keyboard_key(this, wkbd, "Ü","Ü",max_w,max_h);
		gui_internal_keyboard_key(this, wkbd, "Æ","Æ",max_w,max_h);
		gui_internal_keyboard_key(this, wkbd, "Ø","Ø",max_w,max_h);
		gui_internal_keyboard_key(this, wkbd, "Å","Å",max_w,max_h);
		for (i = 0 ; i < 23 ; i++) {
			gui_internal_keyboard_key_data(this, wkbd, "", NULL, NULL, NULL,max_w,max_h);
		}
		wk=gui_internal_keyboard_key_data(this, wkbd, "A",gui_internal_keyboard_change,wkbdb,NULL,max_w,max_h);
		wk->datai=mode-24;
		gui_internal_keyboard_key(this, wkbd, "<-","\b",max_w,max_h);
	}
	if (mode >= 32 && mode < 40) {
		gui_internal_keyboard_key(this, wkbd, "ä","ä",max_w,max_h);
		gui_internal_keyboard_key(this, wkbd, "ö","ö",max_w,max_h);
		gui_internal_keyboard_key(this, wkbd, "ü","ü",max_w,max_h);
		gui_internal_keyboard_key(this, wkbd, "æ","æ",max_w,max_h);
		gui_internal_keyboard_key(this, wkbd, "ø","ø",max_w,max_h);
		gui_internal_keyboard_key(this, wkbd, "å","å",max_w,max_h);
		for (i = 0 ; i < 23 ; i++) {
			gui_internal_keyboard_key_data(this, wkbd, "", NULL, NULL, NULL,max_w,max_h);
		}
		wk=gui_internal_keyboard_key_data(this, wkbd, "a",gui_internal_keyboard_change,wkbdb,NULL,max_w,max_h);
		wk->datai=mode-24;
		gui_internal_keyboard_key(this, wkbd, "<-","\b",max_w,max_h);
	}
	gui_internal_widget_append(wkbdb, wkbd);
	if (render) {
		gui_internal_widget_pack(this, wkbdb);
		gui_internal_widget_render(this, wkbdb);
	}
	return wkbdb;
}

static struct widget *
gui_internal_keyboard(struct gui_priv *this, int mode)
{
	if (! this->keyboard)
		return NULL;
	return gui_internal_keyboard_do(this, NULL, mode);
}

static void
gui_internal_keyboard_change(struct gui_priv *this, struct widget *key, void *data)
{
	gui_internal_keyboard_do(this, key->data, key->datai);
}

static void
gui_internal_search_list_set_default_country(struct gui_priv *this)
{
	struct attr search_attr, country_name, country_iso2, *country_attr;
	struct item *item;
	struct country_search *cs;
	struct tracking *tracking;
	struct search_list_result *res;

	country_attr=country_default();
	tracking=navit_get_tracking(this->nav);
	if (tracking && tracking_get_current_attr(tracking, attr_country_id, &search_attr))
		country_attr=&search_attr;
	if (country_attr) {
		cs=country_search_new(country_attr, 0);
		item=country_search_get_item(cs);
		if (item && item_attr_get(item, attr_country_name, &country_name)) {
			search_attr.type=attr_country_all;
			dbg(0,"country %s\n", country_name.u.str);
			search_attr.u.str=country_name.u.str;
			search_list_search(this->sl, &search_attr, 0);
			while((res=search_list_get_result(this->sl)));
			g_free(this->country_iso2);
			if (item_attr_get(item, attr_country_iso2, &country_iso2)) 
				this->country_iso2=g_strdup(country_iso2.u.str);
		}
		country_search_destroy(cs);
	} else {
		dbg(0,"warning: no default country found\n");
	}
}

static void
gui_internal_search_list_new(struct gui_priv *this)
{
	struct mapset *ms=navit_get_mapset(this->nav);
	if (! this->sl) {
		this->sl=search_list_new(ms);
		gui_internal_search_list_set_default_country(this);
	}
}

static void
gui_internal_search_list_destroy(struct gui_priv *this)
{
	if (this->sl) {
		search_list_destroy(this->sl);
		this->sl=NULL;
	}
}


static void
gui_internal_search(struct gui_priv *this, char *what, char *type, int flags)
{
	struct widget *wb,*wk,*w,*wr,*we,*wl,*wnext=NULL;
	char *country;
	gui_internal_search_list_new(this);
	wb=gui_internal_menu(this, what);
	w=gui_internal_box_new(this, gravity_center|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(wb, w);
	wr=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(w, wr);
	we=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|flags_fill);
	gui_internal_widget_append(wr, we);

	if (!strcmp(type,"Country")) {
		wnext=gui_internal_image_new(this, image_new_xs(this, "gui_select_town"));
		wnext->func=gui_internal_search_town;
	} else if (!strcmp(type,"Town")) {
		if (this->country_iso2)
			country=g_strdup_printf("country_%s", this->country_iso2);
		else
			country=strdup("gui_select_country");
		gui_internal_widget_append(we, wb=gui_internal_image_new(this, image_new_xs(this, country)));
		wb->state |= STATE_SENSITIVE;
		if (flags)
			wb->func = gui_internal_search_country;
		else
			wb->func = gui_internal_back;
		wnext=gui_internal_image_new(this, image_new_xs(this, "gui_select_street"));
		wnext->func=gui_internal_search_street;
		g_free(country);
	} else if (!strcmp(type,"Street")) {
		gui_internal_widget_append(we, wb=gui_internal_image_new(this, image_new_xs(this, "gui_select_town")));
		wb->state |= STATE_SENSITIVE;
		wb->func = gui_internal_back;
	}
	gui_internal_widget_append(we, wk=gui_internal_label_new(this, NULL));
	if (wnext) {
		gui_internal_widget_append(we, wnext);
		wnext->state |= STATE_SENSITIVE;
	}
	wl=gui_internal_box_new(this, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(wr, wl);
	gui_internal_menu_data(this)->search_list=wl;
	wk->state |= STATE_EDIT;
	wk->background=this->background;
	wk->flags |= flags_expand|flags_fill;
	wk->func = gui_internal_search_changed;
	wk->name=g_strdup(type);
	if (this->keyboard)
		gui_internal_widget_append(w, gui_internal_keyboard(this,0));
	gui_internal_menu_render(this);
}

static void
gui_internal_search_street(struct gui_priv *this, struct widget *widget, void *data)
{
	search_list_select(this->sl, attr_town_name, 0, 0);
	gui_internal_search(this,_("Street"),"Street",0);
}

static void
gui_internal_search_street_in_town(struct gui_priv *this, struct widget *widget, void *data)
{
	dbg(0,"id %d\n", widget->selection_id);
	search_list_select(this->sl, attr_town_name, 0, 0);
	search_list_select(this->sl, attr_town_name, widget->selection_id, 1);
	gui_internal_search(this,_("Street"),"Street",0);
}

static void
gui_internal_search_town(struct gui_priv *this, struct widget *wm, void *data)
{
	if (this->sl)
		search_list_select(this->sl, attr_country_all, 0, 0);
	g_free(this->country_iso2);
	this->country_iso2=NULL;
	gui_internal_search(this,_("Town"),"Town",0);
}

static void
gui_internal_search_town_in_country(struct gui_priv *this, struct widget *widget)
{
	struct search_list_common *slc;
	dbg(0,"id %d\n", widget->selection_id);
	search_list_select(this->sl, attr_country_all, 0, 0);
	slc=search_list_select(this->sl, attr_country_all, widget->selection_id, 1);
	if (slc) {
		g_free(this->country_iso2);
		this->country_iso2=((struct search_list_country *)slc)->iso2;
	}
	gui_internal_search(this,widget->name,"Town",0);
}

static void
gui_internal_search_country(struct gui_priv *this, struct widget *widget, void *data)
{
	gui_internal_prune_menu_count(this, 1, 0);
	gui_internal_search(this,_("Country"),"Country",0);
}

static void
gui_internal_cmd_town(struct gui_priv *this, struct widget *wm, void *data)
{
	if (this->sl)
		search_list_select(this->sl, attr_country_all, 0, 0);
	gui_internal_search(this,_("Town"),"Town",1);
}

static void
gui_internal_cmd_layout(struct gui_priv *this, struct widget *wm, void *data)
{
	struct attr attr;
	struct widget *w,*wb,*wl;
	struct attr_iter *iter;


	wb=gui_internal_menu(this, _("Layout"));
	w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	w->spy=this->spacing*3;
	gui_internal_widget_append(wb, w);
	iter=navit_attr_iter_new();
	while(navit_get_attr(this->nav, attr_layout, &attr, iter)) {
		wl=gui_internal_button_navit_attr_new(this, attr.u.layout->name, gravity_left_center|orientation_horizontal|flags_fill,
			&attr, NULL);
		gui_internal_widget_append(w, wl);
	}
	navit_attr_iter_destroy(iter);
	gui_internal_menu_render(this);
}

static void
gui_internal_cmd_fullscreen(struct gui_priv *this, struct widget *wm, void *data)
{
	graphics_draw_mode(this->gra, draw_mode_end);
	this->fullscreen=!this->fullscreen;
	this->win->fullscreen(this->win, this->fullscreen);
	graphics_draw_mode(this->gra, draw_mode_begin);
}

static void
gui_internal_cmd_2d(struct gui_priv *this, struct widget *wm, void *data)
{
	struct transformation *trans=navit_get_trans(this->nav);
	transform_set_pitch(trans, 0);
	this->redraw=1;
}

static void
gui_internal_cmd_3d(struct gui_priv *this, struct widget *wm, void *data)
{
	struct transformation *trans=navit_get_trans(this->nav);
	transform_set_pitch(trans, 20);
	this->redraw=1;
}

static void
gui_internal_cmd_display(struct gui_priv *this, struct widget *wm, void *data)
{
	struct widget *w;
	struct transformation *trans;

	w=gui_internal_menu(this, _("Display"));
	gui_internal_widget_append(w,
		gui_internal_button_new_with_callback(this, _("Layout"),
			image_new_l(this, "gui_display"), gravity_center|orientation_vertical,
			gui_internal_cmd_layout, NULL));
	if (this->fullscreen) {
		gui_internal_widget_append(w,
			gui_internal_button_new_with_callback(this, _("Window Mode"),
				image_new_l(this, "gui_leave_fullscreen"), gravity_center|orientation_vertical,
				gui_internal_cmd_fullscreen, NULL));
	} else {
		gui_internal_widget_append(w,
			gui_internal_button_new_with_callback(this, _("Fullscreen"),
				image_new_l(this, "gui_fullscreen"), gravity_center|orientation_vertical,
				gui_internal_cmd_fullscreen, NULL));
	}
	trans=navit_get_trans(this->nav);
	if (transform_get_pitch(trans)) {
		gui_internal_widget_append(w,
			gui_internal_button_new_with_callback(this, _("2D"),
				image_new_l(this, "gui_map"), gravity_center|orientation_vertical,
				gui_internal_cmd_2d, NULL));
		
	} else {
		gui_internal_widget_append(w,
			gui_internal_button_new_with_callback(this, _("3D"),
				image_new_l(this, "gui_map"), gravity_center|orientation_vertical,
				gui_internal_cmd_3d, NULL));
	}
	gui_internal_menu_render(this);
}

static void
gui_internal_cmd_quit(struct gui_priv *this, struct widget *wm, void *data)
{
	struct navit *nav=this->nav;
	navit_destroy(nav);
	main_remove_navit(nav);
}

static void
gui_internal_cmd_abort_navigation(struct gui_priv *this, struct widget *wm, void *data)
{
	navit_set_destination(this->nav, NULL, NULL);
}


static void
gui_internal_cmd_actions(struct gui_priv *this, struct widget *wm, void *data)
{
	struct widget *w,*wc;
	char *coord;

	w=gui_internal_menu(this, _("Actions"));
	gui_internal_widget_append(w,
		gui_internal_button_new_with_callback(this, _("Bookmarks"),
			image_new_l(this, "gui_bookmark"), gravity_center|orientation_vertical,
			gui_internal_cmd_bookmarks, NULL));
	if (this->clickp_valid) {
		coord=coordinates(&this->clickp, '\n');
		gui_internal_widget_append(w,
			wc=gui_internal_button_new_with_callback(this, coord,
				image_new_l(this, "gui_map"), gravity_center|orientation_vertical,
				gui_internal_cmd_position, (void *)1));
		wc->name=g_strdup(_("Map Point"));
		wc->c=this->clickp;
		g_free(coord);
	}
	if (this->vehicle_valid) {
		coord=coordinates(&this->vehiclep, '\n');
		gui_internal_widget_append(w,
			wc=gui_internal_button_new_with_callback(this, coord,
				image_new_l(this, "gui_vehicle"), gravity_center|orientation_vertical,
				gui_internal_cmd_position, NULL));
		wc->name=g_strdup(_("Vehicle Position"));
		wc->c=this->vehiclep;
		g_free(coord);
	}
	gui_internal_widget_append(w,
		gui_internal_button_new_with_callback(this, _("Town"),
			image_new_l(this, "gui_town"), gravity_center|orientation_vertical,
			gui_internal_cmd_town, NULL));
	gui_internal_widget_append(w,
		gui_internal_button_new_with_callback(this, _("Quit"),
			image_new_l(this, "gui_quit"), gravity_center|orientation_vertical,
			gui_internal_cmd_quit, NULL));
	
	if (navit_check_route(this->nav)) {
		gui_internal_widget_append(w,
								   gui_internal_button_new_with_callback(this, _("Stop\nNavigation"),
								 image_new_l(this, "gui_stop"), gravity_center|orientation_vertical,
								 gui_internal_cmd_abort_navigation, NULL));
	}
	gui_internal_menu_render(this);
}

static void
gui_internal_cmd_maps(struct gui_priv *this, struct widget *wm, void *wdata)
{
	struct attr attr, on, off, description, type, data;
	struct widget *w,*wb,*wma;
	char *label;
	struct attr_iter *iter;


	wb=gui_internal_menu(this, _("Maps"));
	w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	w->spy=this->spacing*3;
	gui_internal_widget_append(wb, w);
	iter=navit_attr_iter_new();
	on.type=off.type=attr_active;
	on.u.num=1;
	off.u.num=0;
	while(navit_get_attr(this->nav, attr_map, &attr, iter)) {
		if (map_get_attr(attr.u.map, attr_description, &description, NULL)) {
			label=g_strdup(description.u.str);
		} else {
			if (!map_get_attr(attr.u.map, attr_type, &type, NULL))
				type.u.str="";
			if (!map_get_attr(attr.u.map, attr_data, &data, NULL))
				data.u.str="";
			label=g_strdup_printf("%s:%s", type.u.str, data.u.str);
		}
		wma=gui_internal_button_map_attr_new(this, label, gravity_left_center|orientation_horizontal|flags_fill,
			attr.u.map, &on, &off, 1);
		gui_internal_widget_append(w, wma);
		g_free(label);
	}
	navit_attr_iter_destroy(iter);
	gui_internal_menu_render(this);
	
}
static void
gui_internal_cmd_set_active_vehicle(struct gui_priv *this, struct widget *wm, void *data)
{
	struct attr vehicle = {attr_vehicle,{wm->data}};
	navit_set_attr(this->nav, &vehicle);
}

static void
gui_internal_cmd_show_satellite_status(struct gui_priv *this, struct widget *wm, void *data)
{
	struct widget *w,*wb,*row;
	struct attr attr,sat_attr;
	struct vehicle *v=wm->data;
	char *str;
	int i;
	enum attr_type types[]={attr_sat_prn, attr_sat_elevation, attr_sat_azimuth, attr_sat_snr};

	wb=gui_internal_menu(this, _("Show Satellite Status"));
	gui_internal_menu_data(this)->redisplay=gui_internal_cmd_show_satellite_status;
	gui_internal_menu_data(this)->redisplay_widget=wm;
	w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(wb, w);
	w = gui_internal_widget_table_new(this,gravity_center | orientation_vertical | flags_expand | flags_fill, 0);
	row = gui_internal_widget_table_row_new(this,gravity_left_top);
	gui_internal_widget_append(row, gui_internal_label_new(this, " PRN "));
	gui_internal_widget_append(row, gui_internal_label_new(this, " Elevation "));
	gui_internal_widget_append(row, gui_internal_label_new(this, " Azimuth "));
	gui_internal_widget_append(row, gui_internal_label_new(this, " SNR "));
	gui_internal_widget_append(w,row);
	while (vehicle_get_attr(v, attr_position_sat_item, &attr, NULL)) {
		row = gui_internal_widget_table_row_new(this,gravity_left_top);
		for (i = 0 ; i < sizeof(types)/sizeof(enum attr_type) ; i++) {
			if (item_attr_get(attr.u.item, types[i], &sat_attr)) 
				str=g_strdup_printf("%d", sat_attr.u.num);
			else
				str=g_strdup("");
			gui_internal_widget_append(row, gui_internal_label_new(this, str));
			g_free(str);
		}
		gui_internal_widget_append(w,row);
	}
	gui_internal_widget_append(wb, w);
	gui_internal_menu_render(this);
}

static void
gui_internal_cmd_show_nmea_data(struct gui_priv *this, struct widget *wm, void *data)
{
	struct widget *w,*wb;
	struct attr attr;
	struct vehicle *v=wm->data;
	wb=gui_internal_menu(this, _("Show NMEA Data"));
	gui_internal_menu_data(this)->redisplay=gui_internal_cmd_show_nmea_data;
	gui_internal_menu_data(this)->redisplay_widget=wm;
	w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(wb, w);
	if (vehicle_get_attr(v, attr_position_nmea, &attr, NULL)) 
		gui_internal_widget_append(w, gui_internal_text_new(this, attr.u.str, gravity_left_center|orientation_vertical));
	gui_internal_menu_render(this);
}


static void
gui_internal_cmd_vehicle_settings(struct gui_priv *this, struct widget *wm, void *data)
{
	struct widget *w,*wb;
	struct attr attr,active_vehicle;
	struct vehicle *v=wm->data;
	wb=gui_internal_menu(this, wm->text);
	w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(wb, w);
	if (!navit_get_attr(this->nav, attr_vehicle, &active_vehicle, NULL))
		active_vehicle.u.vehicle=NULL;
	if (active_vehicle.u.vehicle != v) {
		gui_internal_widget_append(w,
			gui_internal_button_new_with_callback(this, _("Set as active"),
				image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
				gui_internal_cmd_set_active_vehicle, wm->data));
	}
	if (vehicle_get_attr(v, attr_position_sat_item, &attr, NULL)) {
		gui_internal_widget_append(w,
			gui_internal_button_new_with_callback(this, _("Show Satellite status"),
				image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
				gui_internal_cmd_show_satellite_status, wm->data));
	}
	if (vehicle_get_attr(v, attr_position_nmea, &attr, NULL)) {
		gui_internal_widget_append(w,
			gui_internal_button_new_with_callback(this, _("Show NMEA data"),
				image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
				gui_internal_cmd_show_nmea_data, wm->data));
	}
	callback_list_call_attr_2(this->cbl, attr_vehicle, w, wm->data);
	gui_internal_menu_render(this);
}

static void
gui_internal_cmd_vehicle(struct gui_priv *this, struct widget *wm, void *data)
{
	struct attr attr,vattr;
	struct widget *w,*wb,*wl;
	struct attr_iter *iter;
	struct attr active_vehicle;


	wb=gui_internal_menu(this, _("Vehicle"));
	w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	w->spy=this->spacing*3;
	gui_internal_widget_append(wb, w);
	if (!navit_get_attr(this->nav, attr_vehicle, &active_vehicle, NULL))
		active_vehicle.u.vehicle=NULL;
	iter=navit_attr_iter_new();
	while(navit_get_attr(this->nav, attr_vehicle, &attr, iter)) {
		vehicle_get_attr(attr.u.vehicle, attr_name, &vattr, NULL);
		wl=gui_internal_button_new_with_callback(this, vattr.u.str,
			image_new_l(this, attr.u.vehicle == active_vehicle.u.vehicle ? "gui_active" : "gui_inactive"), gravity_left_center|orientation_horizontal|flags_fill,
			gui_internal_cmd_vehicle_settings, attr.u.vehicle);
		wl->text=g_strdup(vattr.u.str);
		gui_internal_widget_append(w, wl);
	}
	navit_attr_iter_destroy(iter);
	gui_internal_menu_render(this);
}


static void
gui_internal_cmd_rules(struct gui_priv *this, struct widget *wm, void *data)
{
	struct widget *wb,*w;
	struct attr on,off;
	wb=gui_internal_menu(this, _("Rules"));
	w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	w->spy=this->spacing*3;
	gui_internal_widget_append(wb, w);
	on.u.num=1;
	off.u.num=0;
	on.type=off.type=attr_tracking;
	gui_internal_widget_append(w,
		gui_internal_button_navit_attr_new(this, _("Lock on road"), gravity_left_center|orientation_horizontal|flags_fill,
			&on, &off));
	on.u.num=0;
	off.u.num=-1;
	on.type=off.type=attr_orientation;
	gui_internal_widget_append(w,
		gui_internal_button_navit_attr_new(this, _("Northing"), gravity_left_center|orientation_horizontal|flags_fill,
			&on, &off));
	on.u.num=1;
	off.u.num=0;
	on.type=off.type=attr_cursor;
	gui_internal_widget_append(w,
		gui_internal_button_navit_attr_new(this, _("Map follows Vehicle"), gravity_left_center|orientation_horizontal|flags_fill,
			&on, &off));
	gui_internal_menu_render(this);
}

static void
gui_internal_cmd_settings(struct gui_priv *this, struct widget *wm, void *data)
{
	struct widget *w;

	w=gui_internal_menu(this, _("Settings"));	
	gui_internal_widget_append(w,
		gui_internal_button_new_with_callback(this, _("Display"),
			image_new_l(this, "gui_display"), gravity_center|orientation_vertical,
			gui_internal_cmd_display, NULL));
	gui_internal_widget_append(w,
		gui_internal_button_new_with_callback(this, _("Maps"),
			image_new_l(this, "gui_maps"), gravity_center|orientation_vertical,
			gui_internal_cmd_maps, NULL));
	gui_internal_widget_append(w,
		gui_internal_button_new_with_callback(this, _("Vehicle"),
			image_new_l(this, "gui_vehicle"), gravity_center|orientation_vertical,
			gui_internal_cmd_vehicle, NULL));
	gui_internal_widget_append(w,
		gui_internal_button_new_with_callback(this, _("Rules"),
			image_new_l(this, "gui_rules"), gravity_center|orientation_vertical,
			gui_internal_cmd_rules, NULL));
	gui_internal_menu_render(this);
}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void gui_internal_motion(void *data, struct point *p)
{

	struct gui_priv *this=data;
	if (!this->root.children) {
		navit_handle_motion(this->nav, p);
		return;
	}
	if (!this->pressed)
		return;
	this->current=*p;
	if(!this->motion_timeout_callback)
		this->motion_timeout_callback=callback_new_1(callback_cast(gui_internal_highlight), this);
	if(!this->motion_timeout_event)
		this->motion_timeout_event=event_add_timeout(100,0, this->motion_timeout_callback);
}


static void gui_internal_menu_root(struct gui_priv *this)
{
	struct widget *w;

	graphics_draw_mode(this->gra, draw_mode_begin);
	w=gui_internal_menu(this, _("Main menu"));
	w->spx=this->spacing*10;
	gui_internal_widget_append(w, gui_internal_button_new_with_callback(this, _("Actions"),
			image_new_l(this, "gui_actions"), gravity_center|orientation_vertical,
			gui_internal_cmd_actions, NULL));
	if (this->flags & 2) {
		gui_internal_widget_append(w, gui_internal_button_new_with_callback(this, _("Show\nMap"),
				image_new_l(this, "gui_map"), gravity_center|orientation_vertical,
				gui_internal_cmd_settings, NULL));
	}
	gui_internal_widget_append(w, gui_internal_button_new_with_callback(this, _("Settings"),
			image_new_l(this, "gui_settings"), gravity_center|orientation_vertical,
			gui_internal_cmd_settings, NULL));
	gui_internal_widget_append(w, gui_internal_button_new(this, _("Tools"),
			image_new_l(this, "gui_tools"), gravity_center|orientation_vertical));

	gui_internal_widget_append(w, gui_internal_button_new_with_callback(this, "Route",
			image_new_l(this, "gui_settings"), gravity_center|orientation_vertical,
			gui_internal_cmd_route, NULL));


	callback_list_call_attr_1(this->cbl, attr_gui, w);
							      
	gui_internal_menu_render(this);
	graphics_draw_mode(this->gra, draw_mode_end);
}

static void
gui_internal_cmd_menu(struct gui_priv *this, struct point *p, int ignore)
{
	struct graphics *gra=this->gra;
	struct transformation *trans;
	struct coord c;
	struct attr attr,attrp;

	this->ignore_button=ignore;
	this->clickp_valid=this->vehicle_valid=0;

	navit_block(this->nav, 1);
	graphics_overlay_disable(gra, 1);
	trans=navit_get_trans(this->nav);
	if (p) {
		transform_reverse(trans, p, &c);
		dbg(0,"x=0x%x y=0x%x\n", c.x, c.y);
		this->clickp.pro=transform_get_projection(trans);
		this->clickp.x=c.x;
		this->clickp.y=c.y;
		this->clickp_valid=1;
	}
	if (navit_get_attr(this->nav, attr_vehicle, &attr, NULL) && attr.u.vehicle
		&& vehicle_get_attr(attr.u.vehicle, attr_position_coord_geo, &attrp, NULL)) {
		this->vehiclep.pro=transform_get_projection(trans);
		transform_from_geo(this->vehiclep.pro, attrp.u.coord_geo, &c);
		this->vehiclep.x=c.x;
		this->vehiclep.y=c.y;
		this->vehicle_valid=1;
	}	
	// draw menu
	this->root.p.x=0;
	this->root.p.y=0;
	this->root.background=this->background;
	gui_internal_menu_root(this);
}

static void
gui_internal_cmd_menu2(struct gui_priv *this)
{
	gui_internal_cmd_menu(this, NULL, 1);
}

static void
gui_internal_check_exit(struct gui_priv *this)
{
	struct graphics *gra=this->gra;
	if (! this->root.children) {
		gui_internal_search_idle_end(this);
		gui_internal_search_list_destroy(this);
		graphics_overlay_disable(gra, 0);
		if (!navit_block(this->nav, 0)) {
			if (this->redraw)
				navit_draw(this->nav);
			else
				navit_draw_displaylist(this->nav);
		}
	}
}

//##############################################################################################################
//# Description: Function to handle mouse clicks and scroll wheel movement
//# Comment: 
//# Authors: Martin Schaller (04/2008), Stefan Klumpp (04/2008)
//##############################################################################################################
static void gui_internal_button(void *data, int pressed, int button, struct point *p)
{
	struct gui_priv *this=data;
	struct graphics *gra=this->gra;
	
	dbg(1,"enter %d %d\n", pressed, button);
	// if still on the map (not in the menu, yet):
	if (!this->root.children || this->ignore_button) {
		this->ignore_button=0;
		// check whether the position of the mouse changed during press/release OR if it is the scrollwheel 
		if (!navit_handle_button(this->nav, pressed, button, p, NULL) || button >=4) // Maybe there's a better way to do this
			return;
		if (this->menu_on_map_click)
			gui_internal_cmd_menu(this, p, 0);	
		return;
	}
	
	
	// if already in the menu:
	if (pressed) {
		this->pressed=1;
		this->current=*p;
		gui_internal_highlight(this);
	} else {
		this->pressed=0;
		this->current.x=-1;
		this->current.y=-1;
		graphics_draw_mode(gra, draw_mode_begin);
		gui_internal_call_highlighted(this);
		gui_internal_highlight(this);
		graphics_draw_mode(gra, draw_mode_end);
		gui_internal_check_exit(this);
	}
}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void gui_internal_resize(void *data, int w, int h)
{
	struct gui_priv *this=data;

	if( this->root.w==w && this->root.h==h)
                return;

	this->root.w=w;
	this->root.h=h;
	dbg(0,"w=%d h=%d children=%p\n", w, h, this->root.children);
	navit_handle_resize(this->nav, w, h);
	if (this->root.children) {
		gui_internal_prune_menu(this, NULL);
		gui_internal_menu_root(this);
	}
} 

static void
gui_internal_keynav_point(struct widget *w, int dx, int dy, struct point *p)
{
	p->x=w->p.x+w->w/2;
	p->y=w->p.y+w->h/2;
	if (dx < 0)
		p->x=w->p.x;
	if (dx > 0)
		p->x=w->p.x+w->w;
	if (dy < 0) 
		p->y=w->p.y;
	if (dy > 0)
		p->y=w->p.y+w->h;
}

static void
gui_internal_keynav_find_closest(struct widget *wi, struct point *p, int dx, int dy, int *distance, struct widget **result)
{
	GList *l=wi->children;
	if (wi->state & STATE_SENSITIVE) {
		int dist1,dist2;
		struct point wp;
		gui_internal_keynav_point(wi, -dx, -dy, &wp);
		if (dx) {
			dist1=(wp.x-p->x)*dx;
			dist2=wp.y-p->y;
		} else if (dy) {
			dist1=(wp.y-p->y)*dy;
			dist2=wp.x-p->x;
		} else {
			dist2=wp.x-p->x;
			dist1=wp.y-p->y;
			if (dist1 < 0)
				dist1=-dist1;
		}
		dbg(1,"checking %d,%d %d %d against %d,%d-%d,%d result %d,%d\n", p->x, p->y, dx, dy, wi->p.x, wi->p.y, wi->p.x+wi->w, wi->p.y+wi->h, dist1, dist2);
		if (dist1 >= 0) {
			if (dist2 < 0)
				dist1-=dist2;
			else
				dist1+=dist2;
			if (dist1 < *distance) {
				*result=wi;
				*distance=dist1;
			}
		}
	}
	while (l) {
		struct widget *child=l->data;
		gui_internal_keynav_find_closest(child, p, dx, dy, distance, result);
		l=g_list_next(l);
	}
}

static void
gui_internal_keynav_highlight_next(struct gui_priv *this, int dx, int dy)
{
	struct widget *result,*menu=g_list_last(this->root.children)->data;
	struct point p;
	int distance;
	if (this->highlighted && this->highlighted_menu == g_list_last(this->root.children)->data)
		gui_internal_keynav_point(this->highlighted, dx, dy, &p);
	else {
		p.x=0;
		p.y=0;
		distance=INT_MAX;
		result=NULL;
		gui_internal_keynav_find_closest(menu, &p, 0, 0, &distance, &result);
		if (result) {
			gui_internal_keynav_point(result, dx, dy, &p);
			dbg(1,"result origin=%p p=%d,%d\n", result, p.x, p.y);
		}
	}
	result=NULL;
	distance=INT_MAX;
	gui_internal_keynav_find_closest(menu, &p, dx, dy, &distance, &result);
	dbg(1,"result=%p\n", result);
	if (! result) {
		if (dx < 0)
			p.x=this->root.w;
		if (dx > 0)
			p.x=0;
		if (dy < 0)
			p.y=this->root.h;
		if (dy > 0)
			p.y=0;
		result=NULL;
		distance=INT_MAX;
		gui_internal_keynav_find_closest(menu, &p, dx, dy, &distance, &result);
		dbg(1,"wraparound result=%p\n", result);
	}
	gui_internal_highlight_do(this, result);
	if (result)
		gui_internal_say(this, result, 1);
}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void gui_internal_keypress(void *data, char *key)
{
	struct gui_priv *this=data;
	int w,h;
	struct point p;
	if (!this->root.children) {
		transform_get_size(navit_get_trans(this->nav), &w, &h);
		switch (*key) {
		case NAVIT_KEY_UP:
			p.x=w/2;
			p.y=0;
			navit_set_center_screen(this->nav, &p);
			break;
		case NAVIT_KEY_DOWN:
			p.x=w/2;
			p.y=h;
			navit_set_center_screen(this->nav, &p);
			break;
		case NAVIT_KEY_LEFT:
			p.x=0;
			p.y=h/2;
			navit_set_center_screen(this->nav, &p);
			break;
		case NAVIT_KEY_RIGHT:
			p.x=w;
			p.y=h/2;
			navit_set_center_screen(this->nav, &p);
			break;
		case NAVIT_KEY_ZOOM_IN:
			navit_zoom_in(this->nav, 2, NULL);
			break;
		case NAVIT_KEY_ZOOM_OUT:
			navit_zoom_out(this->nav, 2, NULL);
			break;
		case NAVIT_KEY_RETURN:
			gui_internal_cmd_menu(this, NULL, 0);
			break;
		}
		return;
	}
	graphics_draw_mode(this->gra, draw_mode_begin);
	switch (*key) {
	case NAVIT_KEY_LEFT:
		gui_internal_keynav_highlight_next(this,-1,0);
		break;
	case NAVIT_KEY_RIGHT:
		gui_internal_keynav_highlight_next(this,1,0);
		break;
	case NAVIT_KEY_UP:
		gui_internal_keynav_highlight_next(this,0,-1);
		break;
	case NAVIT_KEY_DOWN:
		gui_internal_keynav_highlight_next(this,0,1);
		break;
	case NAVIT_KEY_RETURN:
		if (this->highlighted && this->highlighted_menu == g_list_last(this->root.children)->data)
			gui_internal_call_highlighted(this);
		else
			gui_internal_keypress_do(this, key);
		break;
	default:
		gui_internal_keypress_do(this, key);
	}
	graphics_draw_mode(this->gra, draw_mode_end);
	gui_internal_check_exit(this);
} 


//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static int gui_internal_set_graphics(struct gui_priv *this, struct graphics *gra)
{
	struct window *win;
	struct color cbh={0x9fff,0x9fff,0x9fff,0xffff};
	struct color cf={0xbfff,0xbfff,0xbfff,0xffff};
	struct color cw={0xffff,0xffff,0xffff,0xffff};
	struct color cbl={0x0000,0x0000,0x0000,0xffff};
	struct transformation *trans=navit_get_trans(this->nav);
	
	win=graphics_get_data(gra, "window");
        if (! win)
                return 1;
	navit_ignore_graphics_events(this->nav, 1);
	this->gra=gra;
	this->win=win;
	navit_ignore_graphics_events(this->nav, 1);
	transform_get_size(trans, &this->root.w, &this->root.h);
	this->resize_cb=callback_new_attr_1(callback_cast(gui_internal_resize), attr_resize, this);
	graphics_add_callback(gra, this->resize_cb);
	this->button_cb=callback_new_attr_1(callback_cast(gui_internal_button), attr_button, this);
	graphics_add_callback(gra, this->button_cb);
	this->motion_cb=callback_new_attr_1(callback_cast(gui_internal_motion), attr_motion, this);
	graphics_add_callback(gra, this->motion_cb);
	this->keypress_cb=callback_new_attr_1(callback_cast(gui_internal_keypress), attr_keypress, this);
	graphics_add_callback(gra, this->keypress_cb);
	this->background=graphics_gc_new(gra);
	this->background2=graphics_gc_new(gra);
	this->highlight_background=graphics_gc_new(gra);
	graphics_gc_set_foreground(this->highlight_background, &cbh);
	this->foreground=graphics_gc_new(gra);
	graphics_gc_set_foreground(this->foreground, &cf);
	this->text_background=graphics_gc_new(gra);
	this->text_foreground=graphics_gc_new(gra);
	graphics_gc_set_foreground(this->background, &this->background_color);
	graphics_gc_set_foreground(this->background2, &this->background2_color);
	graphics_gc_set_foreground(this->text_background, &this->text_background_color);
	graphics_gc_set_foreground(this->text_foreground, &this->text_foreground_color);
	
	// set fullscreen if needed
	if (this->fullscreen)
		this->win->fullscreen(this->win, this->fullscreen);
	return 0;
}

static int gui_internal_disable_suspend(struct gui_priv *this)
{
	if (this->win->disable_suspend)
		this->win->disable_suspend(this->win);
}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
struct gui_methods gui_internal_methods = {
	NULL,
	NULL,
        gui_internal_set_graphics,
	NULL,
	NULL,
	NULL,
	gui_internal_disable_suspend,
};

static void
gui_internal_get_data(struct gui_priv *priv, char *command, struct attr **in, struct attr ***out)
{
	struct attr private_data = (struct attr) { attr_private_data, {(void *)&priv->data}};
	if (out)  
		*out=attr_generic_add_attr(*out, &private_data);
}

static void
gui_internal_add_callback(struct gui_priv *priv, struct callback *cb)
{
	callback_list_add(priv->cbl, cb);
}

static void
gui_internal_remove_callback(struct gui_priv *priv, struct callback *cb)
{
	callback_list_remove(priv->cbl, cb);
}


static struct gui_internal_methods gui_internal_methods_ext = {
	gui_internal_add_callback,
	gui_internal_remove_callback,
	gui_internal_menu_render,
        image_new_xs,
        image_new_l,
};


static enum flags 
gui_internal_get_flags(struct widget *widget)
{
	return widget->flags;
}

static void
gui_internal_set_flags(struct widget *widget, enum flags flags)
{
	widget->flags=flags;
}

static int
gui_internal_get_state(struct widget *widget)
{
	return widget->state;
}

static void
gui_internal_set_state(struct widget *widget, int state)
{
	widget->state=state;
}

static void
gui_internal_set_func(struct widget *widget, void (*func)(struct gui_priv *priv, struct widget *widget, void *data))
{
	widget->func=func;
}

static void
gui_internal_set_data(struct widget *widget, void *data)
{
	widget->data=data;
}

static void
gui_internal_set_default_background(struct gui_priv *this, struct widget *widget)
{
	widget->background=this->background;
}

static struct gui_internal_widget_methods gui_internal_widget_methods = {
    	gui_internal_widget_append,
        gui_internal_button_new,
        gui_internal_button_new_with_callback,
        gui_internal_box_new,
        gui_internal_label_new,
        gui_internal_image_new,
        gui_internal_keyboard,
	gui_internal_menu,
	gui_internal_get_flags,
	gui_internal_set_flags,
	gui_internal_get_state,
	gui_internal_set_state,
	gui_internal_set_func,
	gui_internal_set_data,
	gui_internal_set_default_background,
};

static struct command_table commands[] = {
	{"menu",gui_internal_cmd_menu2},
	{"fullscreen",gui_internal_cmd_fullscreen},
	{"get_data",gui_internal_get_data},
};


//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static struct gui_priv * gui_internal_new(struct navit *nav, struct gui_methods *meth, struct attr **attrs) 
{
	struct gui_priv *this;
	struct attr *attr;
	*meth=gui_internal_methods;
	this=g_new0(struct gui_priv, 1);
	this->nav=nav;
	if ((attr=attr_search(attrs, NULL, attr_menu_on_map_click)))
		this->menu_on_map_click=attr->u.num;
	else
		this->menu_on_map_click=1;
	if ((attr=attr_search(attrs, NULL, attr_callback_list))) {
		dbg(0,"register\n");
		command_add_table(attr->u.callback_list, commands, sizeof(commands)/sizeof(struct command_table), this);
	}

	if( (attr=attr_search(attrs,NULL,attr_font_size)))
        {
	  this->config.font_size=attr->u.num;
	}
	else
	{
	  this->config.font_size=-1;
	}
	if( (attr=attr_search(attrs,NULL,attr_icon_xs)))
	{
	  this->config.icon_xs=attr->u.num;
	}
	else
	{
	  this->config.icon_xs=-1;
	}
	if( (attr=attr_search(attrs,NULL,attr_icon_l)))
	{
	  this->config.icon_l=attr->u.num;
	}
	else
        {
	  this->config.icon_l=-1;
	}
	if( (attr=attr_search(attrs,NULL,attr_icon_s)))
	{
	  this->config.icon_s=attr->u.num;
	}
	else
        {
	  this->config.icon_s=-1;
	}
	if( (attr=attr_search(attrs,NULL,attr_spacing)))
	{
	  this->config.spacing=attr->u.num;
	}
	else
	{
	  this->config.spacing=-1;	  
	}
	if( (attr=attr_search(attrs,NULL,attr_gui_speech)))
	{
	  this->speech=attr->u.num;
	}
	if( (attr=attr_search(attrs,NULL,attr_keyboard)))
	  this->keyboard=attr->u.num;
        else
	  this->keyboard=1;
	
    if( (attr=attr_search(attrs,NULL,attr_fullscreen)))
      this->fullscreen=attr->u.num;

	if( (attr=attr_search(attrs,NULL,attr_flags)))
	      this->flags=attr->u.num;
	if( (attr=attr_search(attrs,NULL,attr_background_color)))
	      this->background_color=*attr->u.color;
	else
	      this->background_color=(struct color){0x0,0x0,0x0,0xffff};
	if( (attr=attr_search(attrs,NULL,attr_background_color2))) 
		this->background2_color=*attr->u.color;
	else
		this->background2_color=(struct color){0x4141,0x4141,0x4141,0xffff};
	if( (attr=attr_search(attrs,NULL,attr_text_color)))
	      this->text_foreground_color=*attr->u.color;
	else
	      this->text_foreground_color=(struct color){0xffff,0xffff,0xffff,0xffff};
	if( (attr=attr_search(attrs,NULL,attr_columns)))
	      this->cols=attr->u.num;
	if( (attr=attr_search(attrs,NULL,attr_osd_configuration)))
	      this->osd_configuration=*attr;
	this->data.priv=this;
	this->data.gui=&gui_internal_methods_ext;
	this->data.widget=&gui_internal_widget_methods;
	this->cbl=callback_list_new();

	return this;
}

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
void plugin_init(void)
{
	plugin_register_gui_type("internal", gui_internal_new);
}

/**
 * @brief Creates a new table widget.
 * 
 * Creates and returns a new table widget.  This function will
 * setup next/previous buttons as children.
 * 
 * @param this The graphics context.
 * @param flags widget sizing flags.
 * @returns The newly created widget
 */
struct widget * gui_internal_widget_table_new(struct gui_priv * this, enum flags flags, int buttons)
{
	struct widget * widget = g_new0(struct widget,1);
	struct table_data * data = NULL;
	widget->type=widget_table;
	widget->flags=flags;
	widget->data = g_new0(struct table_data,1);
	widget->data_free=gui_internal_table_data_free;
	data = (struct table_data*)widget->data;
	

	if (buttons) {
	data->next_button = gui_internal_button_new_with_callback
		(this,"Next",image_new_xs(this, "gui_active") ,
		 gravity_left_center  |orientation_vertical,
		 gui_internal_table_button_next,NULL);
	data->next_button->data=widget;
	
  
	data->prev_button =  gui_internal_button_new_with_callback
		(this,"Prev",
		 image_new_xs(this, "gui_active")
		 ,gravity_right_center |orientation_vertical,
		 gui_internal_table_button_prev,NULL);
	
	data->prev_button->data=widget;
	
	data->this=this;
	
	data->button_box=gui_internal_box_new(this,
					      gravity_center|orientation_horizontal);
	data->button_box->children=g_list_append(data->button_box->children,
						 data->next_button);
	data->button_box->children=g_list_append(data->button_box->children,
						 data->prev_button);
	//data->button_box->background=this->background2;
	data->button_box->bl=this->spacing;
	widget->children=g_list_append(widget->children,data->button_box);
	gui_internal_widget_pack(this,data->button_box);
	}
	
	return widget;

}

/**
 * @brief Clears all the rows from the table.
 * This function removes all rows from a table.
 * New rows can later be added to the table.
 */
void gui_internal_widget_table_clear(struct gui_priv * this,struct widget * table)
{
  GList * iter;
  struct table_data * table_data = (struct table_data* ) table->data;
  
  iter = table->children; 
  while(iter ) {
	  if(iter->data != table_data->button_box) {		  
		  struct widget * child = (struct widget*)iter->data;
		  gui_internal_widget_destroy(this,child);
		  if(table->children == iter) {			  
			  table->children = g_list_remove(iter,iter->data);
			  iter=table->children;
		  }
		  else 
			  iter = g_list_remove(iter,iter->data);
	  }
	  else {
		  iter = g_list_next(iter);
	  }
    
  }
  table_data->top_row=NULL;
  table_data->bottom_row=NULL;
  if(table_data->page_headers)
	  g_list_free(table_data->page_headers);
  table_data->page_headers=NULL;
}


/**
 * Creates a new table_row widget.
 * @param this The graphics context
 * @param flags Sizing flags for the row
 * @returns The new table_row widget.
 */
struct widget * gui_internal_widget_table_row_new(struct gui_priv * this, enum flags flags)
{
	struct widget * widget = g_new0(struct widget,1);
	widget->type=widget_table_row;
	widget->flags=flags;
	return widget;
}



/**
 * @brief Computes the column dimensions for the table.
 *
 * @param w The table widget to compute dimensions for.
 *
 * This function examines all of the rows and columns for the table w
 * and returns a list (GList) of table_column_desc elements that
 * describe each column of the table.
 *
 * The caller is responsible for freeing the returned list.
 */
static GList * gui_internal_compute_table_dimensions(struct gui_priv * this,struct widget * w)
{
  
	GList * column_desc = NULL;
	GList * current_desc=NULL;
	GList * cur_row = w->children;
	struct widget * cur_row_widget=NULL;
	GList * cur_column=NULL;
	struct widget * cell_w=NULL;
	struct table_column_desc * current_cell=NULL;
	struct table_data * table_data=NULL;
	int height=0;
	int width=0;
	int total_width=0;
	int column_count=0;
  
	/**
	 * Scroll through the the table and
	 * 1. Compute the maximum width + height of each column across all rows.
	 */
	table_data = (struct table_data*) w->data;
	for(cur_row=w->children;  cur_row ; cur_row = g_list_next(cur_row) )
	{
		cur_row_widget = (struct widget*) cur_row->data;
		current_desc = column_desc;
		if(cur_row_widget == table_data->button_box)
		{
			continue;
		}
		column_count=0;
		for(cur_column = cur_row_widget->children; cur_column; 
		    cur_column=g_list_next(cur_column))
		{
			cell_w = (struct widget*) cur_column->data;
			gui_internal_widget_pack(this,cell_w);
			if(current_desc == 0) 
			{
				current_cell = g_new0(struct table_column_desc,1);	
				column_desc = g_list_append(column_desc,current_cell);
				current_desc = g_list_last(column_desc);
				current_cell->height=cell_w->h; 
				current_cell->width=cell_w->w; 
				total_width+=cell_w->w;
				
			}
			else
			{
				current_cell = current_desc->data;
				height = cell_w->h;
				width = cell_w->w;
				if(current_cell->height < height )
				{
					current_cell->height = height;
				}
				if(current_cell->width < width)
				{
					total_width += (width-current_cell->width);
					current_cell->width = width;


					
				}
				current_desc = g_list_next(current_desc);
			}
			column_count++;
			
		}/* column loop */
		
	} /*row loop */


	/**
	 * If the width of all columns is less than the width off
	 * the table expand each cell proportionally.
	 *
	 */
	if(total_width+(this->spacing*column_count) < w->w ) {
		for(current_desc=column_desc; current_desc; current_desc=g_list_next(current_desc)) { 
			current_cell = (struct table_column_desc*) current_desc->data;
			current_cell->width= ( (current_cell->width+this->spacing)/(float)total_width) * w->w ;
		}
	}

	return column_desc;
}


/**
 * @brief Computes the height and width for the table.
 *
 * The height and widht are computed to display all cells in the table
 * at the requested height/width.
 *
 * @param this The graphics context
 * @param w The widget to pack.
 *
 */
void gui_internal_table_pack(struct gui_priv * this, struct widget * w)
{
  
	int height=0;
	int width=0;
	int count=0;
	GList * column_data = gui_internal_compute_table_dimensions(this,w);
	GList * current=0;
	struct table_column_desc * cell_desc=0;
	struct table_data * table_data = (struct table_data*)w->data;
	
	for(current = column_data; current; current=g_list_next(current))
	{
		if(table_data->button_box == current->data )
		{
			continue;
		}
		cell_desc = (struct table_column_desc *) current->data;
		width = width + cell_desc->width + this->spacing;
		if(height < cell_desc->height) 
		{
			height = cell_desc->height ;
		}
	}



	for(current=w->children; current; current=g_list_next(current))
	{
		if(current->data!= table_data->button_box)
		{
			count++;
		}
	}
	if (table_data->button_box)
		gui_internal_widget_pack(this,table_data->button_box);  



	if(w->h + w->c.y   > this->root.h   )
	{
		/**
		 * Do not allow the widget to exceed the screen.
		 * 
		 */
		w->h = this->root.h- w->c.y  - height;
	}
	w->w = width;
	
	/**
	 * Deallocate column descriptions.
	 */
	current = column_data;
	while( (current = g_list_last(current)) )
	{
		current = g_list_remove(current,current->data);
	}
	
}



/**
 * @brief Renders a table widget.
 * 
 * @param this The graphics context
 * @param w The table widget to render.
 */
void gui_internal_table_render(struct gui_priv * this, struct widget * w)
{

	int x;
	int y;
	GList * column_desc=NULL;
	GList * cur_row = NULL;
	GList * current_desc=NULL;
	struct table_data * table_data = (struct table_data*)w->data;
	int is_skipped=0;
	int is_first_page=1;
	struct table_column_desc * dim=NULL;
	
	column_desc = gui_internal_compute_table_dimensions(this,w);
	y=w->p.y;
	
	/**
	 * Skip rows that are on previous pages.
	 */
	cur_row = w->children;
	if(table_data->top_row && table_data->top_row != w->children )
	{
		cur_row = table_data->top_row;
		is_first_page=0;
	}
	
	
	/**
	 * Loop through each row.  Drawing each cell with the proper sizes, 
	 * at the proper positions.
	 */
	for(table_data->top_row=cur_row; cur_row; cur_row = g_list_next(cur_row))
	{
		GList * cur_column=NULL;
		current_desc = column_desc;
		struct widget * cur_row_widget = (struct widget*)cur_row->data;
		int max_height=0;
		x =w->p.x+this->spacing;        
		if(cur_row_widget == table_data->button_box )
		{
			continue;
		}
		dim = (struct table_column_desc*)current_desc->data;
		
		if( y + dim->height + (table_data->button_box ? table_data->button_box->h : 0) + this->spacing >= w->p.y + w->h )
		{
			/*
			 * No more drawing space left.
			 */
			is_skipped=1;
			break;
			
		}      
		for(cur_column = cur_row_widget->children; cur_column; 
		    cur_column=g_list_next(cur_column))
		{
			struct  widget * cur_widget = (struct widget*) cur_column->data;
			dim = (struct table_column_desc*)current_desc->data;
			
			cur_widget->p.x=x;
			cur_widget->w=dim->width;
			cur_widget->p.y=y;
			cur_widget->h=dim->height;
			x=x+cur_widget->w;
			max_height = dim->height;
			/* We pack the widget before rendering to ensure that the x and y 
			 * coordinates get pushed down.
			 */
			gui_internal_widget_pack(this,cur_widget);
			gui_internal_widget_render(this,cur_widget);
      
			if(dim->height > max_height)
			{
				max_height = dim->height;
			}
		}
		y = y + max_height;
		table_data->bottom_row=cur_row;
		current_desc = g_list_next(current_desc);
	}
	if(table_data  && table_data->button_box && (is_skipped || !is_first_page)  )
	{
		table_data->button_box->p.y =w->p.y+w->h-table_data->button_box->h - 
			this->spacing;
		if(table_data->button_box->p.y < y )
		{
			table_data->button_box->p.y=y;
		}	
		table_data->button_box->p.x = w->p.x;
		table_data->button_box->w = w->w;
		//    table_data->button_box->h = w->h - y;
		//    table_data->next_button->h=table_data->button_box->h;    
		//    table_data->prev_button->h=table_data->button_box->h;
		//    table_data->next_button->c.y=table_data->button_box->c.y;    
		//    table_data->prev_button->c.y=table_data->button_box->c.y;    

		gui_internal_widget_pack(this,table_data->button_box);
		if(table_data->next_button->p.y > w->p.y + w->h + table_data->next_button->h)
		{
		
			table_data->button_box->p.y = w->p.y + w->h - 
				table_data->button_box->h;
		}
		if(is_skipped) 
	        {
			table_data->next_button->state|= STATE_SENSITIVE;
		}
		else
		{
			table_data->next_button->state&= ~STATE_SENSITIVE;
		}
		
		if(table_data->top_row != w->children)
		{
			table_data->prev_button->state|= STATE_SENSITIVE;
		}
		else
		{
			table_data->prev_button->state&= ~STATE_SENSITIVE;
		}
		gui_internal_widget_render(this,table_data->button_box);
		

	}
	
	/**
	 * Deallocate column descriptions.
	 */
	current_desc = column_desc;
	while( (current_desc = g_list_last(current_desc)) )
	{
		current_desc = g_list_remove(current_desc,current_desc->data);
	}
}


/**
 * @brief Displays Route information
 *
 * @li The name of the active vehicle
 * @param wm The button that was pressed.
 * @param v Unused
 */
void gui_internal_cmd_route(struct gui_priv * this, struct widget * wm,void *v)
{


	struct widget * menu;
	struct widget * row;


	if(! this->vehicle_cb)
	{
	  /**
	   * Register the callback on vehicle updates.
	   */
	  this->vehicle_cb = callback_new_attr_1(callback_cast(gui_internal_route_update),
						       attr_position_coord_geo,this);
	  navit_add_callback(this->nav,this->vehicle_cb);
	}
	
	this->route_data.route_table = gui_internal_widget_table_new(this,gravity_left_top | flags_fill | flags_expand |orientation_vertical,1);
	row = gui_internal_widget_table_row_new(this,gravity_left | orientation_horizontal | flags_fill);

	row = gui_internal_widget_table_row_new(this,gravity_left | orientation_horizontal | flags_fill);


	menu=gui_internal_menu(this,"Route");
	
	menu->free=gui_internal_route_screen_free;
	this->route_data.route_showing=1;
	this->route_data.route_table->spx = this->spacing;

	
	struct widget * box = gui_internal_box_new(this, gravity_left_top| orientation_vertical | flags_fill | flags_expand);

	//	gui_internal_widget_append(box,gui_internal_box_new_with_label(this,"Test"));
	gui_internal_widget_append(box,this->route_data.route_table);
	box->w=menu->w;
	box->spx = this->spacing;
	this->route_data.route_table->w=box->w;
	gui_internal_widget_append(menu,box);
	gui_internal_populate_route_table(this,this->nav);
	gui_internal_menu_render(this);

}



/**
 * @brief handles the 'next page' table event.
 * A callback function that is invoked when the 'next page' button is pressed
 * to advance the contents of a table widget.
 *
 * @param this The graphics context.
 * @param wm The button widget that was pressed.
 */
static void gui_internal_table_button_next(struct gui_priv * this, struct widget * wm, void *data)
{
	struct widget * table_widget = (struct widget * ) wm->data;
	struct table_data * table_data = NULL;
	int found=0;
	GList * iterator;
	
	if(table_widget)
	{
		table_data = (struct table_data*) table_widget->data;
		
	}
	if(table_data)
	{
		/**
		 * Before advancing to the next page we need to ensure
		 * that the current top_row is in the list of previous top_rows 
		 * so previous page can work.
		 *
		 */
		for(iterator=table_data->page_headers; iterator != NULL;
		    iterator = g_list_next(iterator) )
		{
			if(iterator->data == table_data->top_row)
			{
				found=1;
				break;
			}
			
		}
		if( ! found)
		{
			table_data->page_headers=g_list_append(table_data->page_headers,
							       table_data->top_row);
		}
		
		table_data->top_row = g_list_next(table_data->bottom_row);
	}
	wm->state&= ~STATE_HIGHLIGHTED;
	gui_internal_menu_render(this);
}



/**
 * @brief handles the 'previous page' table event.
 * A callback function that is invoked when the 'previous page' button is pressed
 * to go back in the contents of a table widget.
 *
 * @param this The graphics context.
 * @param wm The button widget that was pressed.
 */
static void gui_internal_table_button_prev(struct gui_priv * this, struct widget * wm, void *data)
{
	struct widget * table_widget = (struct widget * ) wm->data;
	struct table_data * table_data = NULL;
	GList * current_page_top=NULL;

	GList * iterator;
	if(table_widget)
	{
		table_data = (struct table_data*) table_widget->data;
		current_page_top = table_data->top_row;
		if(table_data) 
		{
			for(iterator = table_data->page_headers; iterator != NULL;
			    iterator = g_list_next(iterator))
			{
				if(current_page_top == iterator->data)
				{
					break;
				}
				table_data->top_row = (GList*) iterator->data;
			}
		}
	}
	wm->state&= ~STATE_HIGHLIGHTED;
	gui_internal_menu_render(this);
}


/**
 * @brief deallocates a table_data structure.
 *
 */
void gui_internal_table_data_free(void * p)
{


	/**
	 * @note button_box and its children (next_button,prev_button)
	 * have their memory managed by the table itself.
	 */
	struct table_data * table_data =  (struct table_data*) p;
	g_list_free(table_data->page_headers);
	g_free(p);


}


/**
 * @brief Called when the route is updated.
 */
void gui_internal_route_update(struct gui_priv * this, struct navit * navit, struct vehicle *v)
{

	if(this->route_data.route_showing) {
		gui_internal_populate_route_table(this,navit);	  
		graphics_draw_mode(this->gra, draw_mode_begin);
		gui_internal_menu_render(this);
		graphics_draw_mode(this->gra, draw_mode_end);
	}

	
}


/**
 * @brief Called when the route screen is closed (deallocated).
 *
 * The main purpose of this function is to remove the widgets from 
 * references route_data because those widgets are about to be freed.
 */
void gui_internal_route_screen_free(struct gui_priv * this_,struct widget * w)
{
	if(this_) {
		this_->route_data.route_showing=0;
		this_->route_data.route_table=NULL;
		g_free(w);
	}
  
}

/**
 * @brief Populates the route  table with route information
 *
 * @param this The gui context
 * @param navit The navit object
 */
void gui_internal_populate_route_table(struct gui_priv * this,
				       struct navit * navit)
{
	struct map * map=NULL;
	struct map_rect * mr=NULL;
	struct navigation * nav = NULL;
	struct item * item =NULL;
	struct attr attr;
	struct widget * label = NULL;
	struct widget * row = NULL;
	nav = navit_get_navigation(navit);
	if(!nav) {
		return;
	}
	map = navigation_get_map(nav);
	if(map)
	  mr = map_rect_new(map,NULL);
	if(mr) {
		gui_internal_widget_table_clear(this,this->route_data.route_table);
		while((item = map_rect_get_item(mr))) {
			if(item_attr_get(item,attr_navigation_long,&attr)) {
			  label = gui_internal_label_new(this,attr.u.str);
			  row = gui_internal_widget_table_row_new(this,
								  gravity_left 
								  | flags_fill 
								  | orientation_horizontal);
			  row->children=g_list_append(row->children,label);
			  gui_internal_widget_append(this->route_data.route_table,row);
			}
			
		}
		
	}       	             
}
