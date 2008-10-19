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
#include <libintl.h>
#include <glib.h>
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

#define STATE_VISIBLE 1
#define STATE_SELECTED 2
#define STATE_HIGHLIGHTED 4
#define STATE_SENSITIVE 8
#define STATE_EDIT 16
#define STATE_CLEAR 32

enum widget_type {
	widget_box=1,
	widget_button,
	widget_label,
	widget_image,
	widget_table,
	widget_table_row
};

enum flags {
	gravity_none=0x00,
	gravity_left=1,
	gravity_xcenter=2,
	gravity_right=4,
	gravity_top=8,
	gravity_ycenter=16,
	gravity_bottom=32,
	gravity_left_top=gravity_left|gravity_top,
	gravity_top_center=gravity_xcenter|gravity_top,
	gravity_right_top=gravity_right|gravity_top,
	gravity_left_center=gravity_left|gravity_ycenter,
	gravity_center=gravity_xcenter|gravity_ycenter,
	gravity_right_center=gravity_right|gravity_ycenter,
	gravity_left_bottom=gravity_left|gravity_bottom,
	gravity_bottom_center=gravity_xcenter|gravity_bottom,
	gravity_right_bottom=gravity_right|gravity_bottom,
	flags_expand=0x100,
	flags_fill=0x200,
	orientation_horizontal=0x10000,
	orientation_vertical=0x20000,
	orientation_horizontal_vertical=0x40000,
};


//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
struct widget {
	enum widget_type type;
	struct graphics_gc *background;
	struct graphics_gc *foreground_frame;
	char *text;
	struct graphics_image *img;
	 /**
	  * A function to be invoked on actions.
	  * @li widget The widget that is receiving the button press.
	  *
	  */
	void (*func)(struct gui_priv *priv, struct widget *widget);
	int reason;
	int datai;
	void *data;
	void (*data_free)(void *data);
	char *prefix;
	char *name;
	char *speech;
	struct pcoord c;
	struct item item;
	int state;
	struct point p;
	int wmin,hmin;
	int w,h;
	int bl,br,bt,bb,spx,spy;
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
  {545,32,40,44,2}
    , {545,32,48,96,5}
      ,{200,16,32,48,2}
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
	struct widget *list;
	int clickp_valid, vehicle_valid;
	struct pcoord clickp, vehiclep;
	struct search_list *sl;
	int ignore_button;
	int menu_on_map_click;
	char *country_iso2;
	int speech;
	/**
	 * The setting information read from the configuration file.
	 * values of -1 indicate no value was specified in the config file.
	 */
        struct gui_config_settings config;
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

static struct widget* gui_internal_widget_table_new(struct gui_priv * this, enum flags flags);
static void gui_internal_widget_table_add_row(struct widget * table,  struct widget * row);
static void gui_internal_table_render(struct gui_priv * this, struct widget * w);
static void gui_internal_cmd_route(struct gui_priv * this, struct widget * w);
static void gui_internal_table_pack(struct gui_priv * this, struct widget * w);
static void gui_internal_table_button_next(struct gui_priv * this, struct widget * wm);
static void gui_internal_table_button_prev(struct gui_priv * this, struct widget * wm);
static void gui_internal_table_data_free(void * d);

static struct graphics_image *
image_new_scaled(struct gui_priv *this, char *name, int w, int h)
{
	struct graphics_image *ret=NULL;
	char *full_name=NULL;
	int i;

	for (i = 1 ; i < 6 ; i++) {
		full_name=NULL;
		switch (i) {
		case 1:
			full_name=g_strdup_printf("%s/xpm/%s.svg", getenv("NAVIT_SHAREDIR"), name);
			break;
		case 2:
			full_name=g_strdup_printf("%s/xpm/%s.svgz", getenv("NAVIT_SHAREDIR"), name);
			break;
		case 3:
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
		graphics_get_text_bbox(this->gra, this->font, text, 0x10000, 0x0, p);
		w=p[2].x-p[0].x;
		h=p[0].y-p[2].y;
	}
	widget->h=h+this->spacing;
	widget->w=w+this->spacing;
	widget->flags=gravity_center;

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
		pnt.y+=w->h-this->spacing;
		graphics_draw_text(this->gra, this->text_foreground, this->text_background, this->font, w->text, &pnt, 0x10000, 0x0);
	}
}

static struct widget *
gui_internal_text_new(struct gui_priv *this, char *text)
{
	char *s=g_strdup(text),*s2,*tok;
	struct widget *ret=gui_internal_box_new(this, gravity_center|orientation_vertical);
	s2=s;
	while ((tok=strtok(s2,"\n"))) {
		gui_internal_widget_append(ret, gui_internal_label_new(this, tok));
		s2=NULL;
	}
	return ret;
}


static struct widget *
gui_internal_button_new_with_callback(struct gui_priv *this, char *text, struct graphics_image *image, enum flags flags, void(*func)(struct gui_priv *priv, struct widget *widget), void *data)
{
	struct widget *ret=NULL;
	ret=gui_internal_box_new(this, flags);
	if (ret) {
		if (image)
			gui_internal_widget_append(ret, gui_internal_image_new(this, image));
		if (text)
			gui_internal_widget_append(ret, gui_internal_text_new(this, text));
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
gui_internal_button_attr_pressed(struct gui_priv *this, struct widget *w)
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
	ret->get_attr=navit_get_attr;
	ret->set_attr=navit_set_attr;
	ret->remove_cb=navit_remove_callback;
	ret->instance=this->nav;
	ret->cb=callback_new_attr_2(gui_internal_button_attr_callback, on->type, this, ret);
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
	ret->get_attr=map_get_attr;
	ret->set_attr=map_set_attr;
	ret->remove_cb=map_remove_callback;
	ret->instance=map;
	ret->redraw=1;
	ret->cb=callback_new_attr_2(gui_internal_button_attr_callback, on->type, this, ret);
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


	if (p && (wi->p.x > p->x || wi->p.y > p->y || wi->p.x + wi->w < p->x || wi->p.y + wi->h < p->y)) {
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
}
//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void gui_internal_highlight(struct gui_priv *this, struct point *p)
{
	struct widget *menu,*found=NULL;

	if (p) {
		menu=g_list_last(this->root.children)->data;
		found=gui_internal_find_widget(menu, p, STATE_SENSITIVE);
	}
	gui_internal_highlight_do(this, found);
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
#if 0
	struct point pnt[5];
	pnt[0]=w->p;
        pnt[1].x=pnt[0].x+w->w;
        pnt[1].y=pnt[0].y;
        pnt[2].x=pnt[0].x+w->w;
        pnt[2].y=pnt[0].y+w->h;
        pnt[3].x=pnt[0].x;
        pnt[3].y=pnt[0].y+w->h;
        pnt[4]=pnt[0];
	graphics_draw_lines(this->gra, this->foreground, pnt, 5);
#endif

	l=w->children;
	while (l) {
		wc=l->data;	
		gui_internal_widget_render(this, wc);
		l=g_list_next(l);
	}
}

static void gui_internal_box_pack(struct gui_priv *this, struct widget *w)
{
	struct widget *wc;
	int x0,x=0,y=0,width=0,height=0,owidth=0,oheight=0,expand=0,count=0,rows=0,cols=w->cols ? w->cols : 3;
	GList *l;
	int orientation=w->flags & 0xffff0000;

	
	l=w->children;
	while (l) {
		count++;
		l=g_list_next(l);
	}
	if (orientation == orientation_horizontal_vertical && count <= cols)
		orientation=orientation_horizontal;
	switch (orientation) {
	case orientation_horizontal:
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
		break;
	}
	if (! w->w && ! w->h) {
		w->w=w->bl+w->br+width;
		w->h=w->bt+w->bb+height;
	}
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
	l=w->children;
	while (l) {
		wc=l->data;
		gui_internal_widget_pack(this, wc);
		l=g_list_next(l);
	}
}

static void gui_internal_widget_append(struct widget *parent, struct widget *child)
{
	if (! child->background)
		child->background=parent->background;
	parent->children=g_list_append(parent->children, child);
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
	g_free(w);
}


static void
gui_internal_widget_render(struct gui_priv *this, struct widget *w)
{
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
	this->highlighted->func(this, this->highlighted);
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
	while ((l = g_list_last(this->root.children))) {
		if (l->data == w) {
			gui_internal_say(this, w, 0);
			gui_internal_widget_render(this, w);
			return;
		}
		gui_internal_widget_destroy(this, l->data);
		this->root.children=g_list_remove(this->root.children, l->data);
	}
}

static void
gui_internal_cmd_return(struct gui_priv *this, struct widget *wm)
{
	gui_internal_prune_menu(this, wm->data);
}

static void
gui_internal_cmd_main_menu(struct gui_priv *this, struct widget *wm)
{
	gui_internal_prune_menu(this, this->root.children->data);
}

static struct widget *
gui_internal_top_bar(struct gui_priv *this)
{
	struct widget *w,*wm,*wh,*wc,*wcn;
	int dots_len, sep_len;
	GList *res=NULL,*l;
	int width,width_used=0,use_sep,incomplete=0;

	w=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|flags_fill);
	w->bl=this->spacing;
	w->spx=this->spacing;
	w->background=this->background2;
	wm=gui_internal_button_new_with_callback(this, NULL,
		image_new_s(this, "gui_map"), gravity_center|orientation_vertical,
		gui_internal_cmd_return, NULL);
	wm->speech=g_strdup("Back to map");
	gui_internal_widget_pack(this, wm);
	wh=gui_internal_button_new_with_callback(this, NULL,
		image_new_s(this, "gui_home"), gravity_center|orientation_vertical,
		gui_internal_cmd_main_menu, NULL);
	wh->speech=g_strdup("Main Menu");
	gui_internal_widget_pack(this, wh);
	gui_internal_widget_append(w, wm);
	gui_internal_widget_append(w, wh);
	width=this->root.w-w->bl-wm->w-w->spx-wh->w;
	l=g_list_last(this->root.children);
	wcn=gui_internal_label_new(this,".. »");
	dots_len=wcn->w;
	gui_internal_widget_destroy(this, wcn);
	wcn=gui_internal_label_new(this,"»");
	sep_len=wcn->w;
	gui_internal_widget_destroy(this, wcn);
	while (l) {
		if (g_list_previous(l) || !g_list_next(l)) {
			wc=l->data;
			wcn=gui_internal_label_new(this, wc->text);
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
				res=g_list_prepend(res, gui_internal_label_new(this, "»"));
				width_used+=sep_len+w->spx;
			}
			width_used+=wcn->w;
			wcn->func=gui_internal_cmd_return;
			wcn->data=wc;
			wcn->state |= STATE_SENSITIVE;
			res=g_list_prepend(res, wcn);
			
		}
		l=g_list_previous(l);
	}
	if (incomplete) {
		if (! res) {
			wcn=gui_internal_label_new_abbrev(this, wc->text, width-width_used-w->spx-dots_len);
			wcn->func=gui_internal_cmd_return;
			wcn->data=wc;
			wcn->state |= STATE_SENSITIVE;
			res=g_list_prepend(res, wcn);
			l=g_list_previous(l);
			wc=l->data;
		}
		wcn=gui_internal_label_new(this, ".. »");
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
#if 0
	if (dots)
		gui_internal_widget_destroy(this, dots);
#endif
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

  /**
   * Select default values from profile based on the screen.
   */
  if(this->root.w > 320 || this->root.h > 320)
  {
    if(this->root.w > 640 || this->root.h > 640) 
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
gui_internal_menu(struct gui_priv *this, char *label)
{
	struct widget *menu,*w;
	

	menu=gui_internal_box_new_with_label(this, gravity_center|orientation_vertical, label);
	menu->w=this->root.w;
	menu->h=this->root.h;
	menu->background=this->background;
	gui_internal_apply_config(this);
	this->font=graphics_font_new(this->gra,this->font_size,1);
 
	gui_internal_widget_append(&this->root, menu);
	w=gui_internal_top_bar(this);
	gui_internal_widget_append(menu, w);
	w=gui_internal_box_new(this, gravity_center|orientation_horizontal_vertical|flags_expand|flags_fill);
	w->spx=4*this->spacing;
	gui_internal_widget_append(menu, w);

	return w;
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
gui_internal_cmd_set_destination(struct gui_priv *this, struct widget *wm)
{
	struct widget *w=wm->data;
	dbg(0,"c=%d:0x%x,0x%x\n", w->c.pro, w->c.x, w->c.y);
	navit_set_destination(this->nav, &w->c, w->name);
	gui_internal_prune_menu(this, NULL);
}

static void
gui_internal_cmd_set_position(struct gui_priv *this, struct widget *wm)
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
gui_internal_cmd_add_bookmark_clicked(struct gui_priv *this, struct widget *widget)
{
	gui_internal_cmd_add_bookmark_do(this, widget->data);
}

static void
gui_internal_cmd_add_bookmark_changed(struct gui_priv *this, struct widget *wm)
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
gui_internal_cmd_add_bookmark(struct gui_priv *this, struct widget *wm)
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
	gui_internal_widget_append(w, gui_internal_keyboard(this,0));
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

static void gui_internal_cmd_pois(struct gui_priv *this, struct widget *wm);

static struct widget *
gui_internal_cmd_pois_selector(struct gui_priv *this, struct pcoord *c)
{
	struct widget *wl,*wb;
	int i;
	wl=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|flags_fill);
	for (i = 0 ; i < sizeof(selectors)/sizeof(struct selector) ; i++) {
	gui_internal_widget_append(wl, wb=gui_internal_button_new_with_callback(this, NULL,
		image_new_xs(this, selectors[i].icon), gravity_center|orientation_vertical,
		gui_internal_cmd_pois, &selectors[i]));
		wb->c=*c;
	}
	return wl;
}

static struct widget *
gui_internal_cmd_pois_item(struct gui_priv *this, struct coord *center, struct item *item, struct coord *c, int dist)
{
	char coordbuf[64];
	struct param_list param[5];
	char distbuf[32];
	char dirbuf[32];
	char *type;
	struct attr attr;
	struct widget *wl;

	wl=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|flags_fill);

	sprintf(distbuf,"%d", dist/1000);
	gui_internal_widget_append(wl, gui_internal_label_new(this, distbuf));
	get_direction(dirbuf, transform_get_angle_delta(center, c, 0), 1);
	gui_internal_widget_append(wl, gui_internal_label_new(this, dirbuf));
	type=item_to_name(item->type);
	gui_internal_widget_append(wl, gui_internal_label_new(this, type));
	if (item_attr_get(item, attr_label, &attr)) {
		wl->name=g_strdup_printf("%s %s",type,attr.u.str);
	} else {
		attr.u.str="";
		wl->name=g_strdup(type);
	}
	gui_internal_widget_append(wl, gui_internal_label_new(this, attr.u.str));

	return wl;
}

static gint
gui_internal_cmd_pois_sort_num(gconstpointer a, gconstpointer b, gpointer user_data)
{
	struct widget *wa=a;
	struct widget *wb=b;
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

static void gui_internal_cmd_position(struct gui_priv *this, struct widget *wm);

static void
gui_internal_cmd_pois(struct gui_priv *this, struct widget *wm)
{
	struct map_selection *sel,*selm;
	struct coord c,center;
	struct mapset_handle *h;
	struct map *m;
	struct map_rect *mr;
	struct item *item;
	int idist,dist=400000;
	struct widget *wi,*w,*w2,*wb;
	enum projection pro=wm->c.pro;
	struct selector *isel=wm->data;

	wb=gui_internal_menu(this, isel ? isel->name : "POIs");	
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
gui_internal_cmd_view_on_map(struct gui_priv *this, struct widget *wm)
{
	struct widget *w=wm->data;
	navit_set_center(this->nav, &w->c);
	gui_internal_prune_menu(this, NULL);
}


static void
gui_internal_cmd_view_attributes(struct gui_priv *this, struct widget *wm)
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
gui_internal_cmd_view_in_browser(struct gui_priv *this, struct widget *wm)
{
	struct widget *w,*wb;
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
		system(cmd);
		g_free(cmd);
	}
}

/* wm->data: 0 Nothing special
	     1 Map Point
	     2 Item
	     3 Town 
*/

static void gui_internal_search_street_in_town(struct gui_priv *this, struct widget *widget);

static void
gui_internal_cmd_position(struct gui_priv *this, struct widget *wm)
{
	struct widget *wb,*w,*wc,*wbc;
	struct coord_geo g;
	struct coord c;
	char *coord,*name;
	int display_attributes=(wm->data == 2);
	int display_view_on_map=(wm->data != 1);
	int display_items=(wm->data == 1);
	int display_streets=(wm->data == 3);

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
			wc=gui_internal_button_new_with_callback(this, "Streets",
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
					wb=gui_internal_button_new_with_callback(this, "View in Browser",
						image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
						gui_internal_cmd_view_in_browser, NULL));
				wb->item=wm->item;
			}
			gui_internal_widget_append(w,
				wb=gui_internal_button_new_with_callback(this, "View Attributes",
					image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
					gui_internal_cmd_view_attributes, NULL));
			wb->item=wm->item;
		}
		map_rect_destroy(mr);
	}
	gui_internal_widget_append(w,
		gui_internal_button_new_with_callback(this, "Set as destination",
			image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
			gui_internal_cmd_set_destination, wm));
	gui_internal_widget_append(w,
		gui_internal_button_new_with_callback(this, "Set as position",
			image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
			gui_internal_cmd_set_position, wm));
	gui_internal_widget_append(w,
		wbc=gui_internal_button_new_with_callback(this, "Add as bookmark",
			image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
			gui_internal_cmd_add_bookmark, wm));
	wbc->c=wm->c;
	gui_internal_widget_append(w,
		wbc=gui_internal_button_new_with_callback(this, "POIs",
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
			gui_internal_button_new_with_callback(this, "View on map",
				image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
				gui_internal_cmd_view_on_map, wm));
	}
	if (display_items) {
		int i,dist=10;
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
		for (i = 0 ; i < layer_end ; i++) {
			sel.order[i]=18;
		}
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
gui_internal_cmd_bookmarks(struct gui_priv *this, struct widget *wm)
{
	struct attr attr,mattr;
	struct map_rect *mr=NULL;
	struct item *item;
	char *label_full,*l,*prefix,*pos;
	int len,plen,hassub;
	struct widget *wb,*w,*wbm;
	GHashTable *hash;
	struct coord c;


	wb=gui_internal_menu(this, wm->text ? wm->text : "Bookmarks");
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
						wbm->name=g_strdup_printf("Bookmark %s",label_full);
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
			wi->func(this, wi);
		}
		gui_internal_widget_render(this, wi);
	}
}


static void
gui_internal_cmd_keypress(struct gui_priv *this, struct widget *wm)
{
	gui_internal_keypress_do(this, (char *) wm->data);
}

static void
gui_internal_search_changed(struct gui_priv *this, struct widget *wm)
{
	GList *l;
	gui_internal_widget_children_destroy(this, this->list);
	char *text, *name;
	dbg(0,"%s now '%s'\n", wm->name, wm->text);
	if (wm->text && g_utf8_strlen(wm->text, -1) >= 2) {
		struct attr search_attr;
		struct search_list_result *res;
		struct widget *wc;

		dbg(0,"process\n");
		if (! strcmp(wm->name,"Town"))
			search_attr.type=attr_town_name;
		if (! strcmp(wm->name,"Street"))
			search_attr.type=attr_street_name;
		search_attr.u.str=wm->text;
		search_list_search(this->sl, &search_attr, 1);
		while((res=search_list_get_result(this->sl))) {
			if (! strcmp(wm->name,"Town")) {
				name=res->town->name;
				text=g_strdup(name);
			}
			if (! strcmp(wm->name,"Street")) {
				name=res->street->name;
				text=g_strdup_printf("%s %s", res->town->name, res->street->name);
			}
			dbg(0,"res=%s\n", res->town->name);
			gui_internal_widget_append(this->list,
				wc=gui_internal_button_new_with_callback(this, text,
				image_new_xs(this, res->country->flag), gravity_left_center|orientation_horizontal|flags_fill,
				gui_internal_cmd_position, (void *)3));
			g_free(text);
			wc->name=g_strdup(name);
			wc->c=*res->c;
			wc->item.id_lo=res->id;
		}
		gui_internal_widget_pack(this, this->list);
	}
	l=g_list_last(this->root.children);
	gui_internal_widget_render(this, l->data);
}

static struct widget *
gui_internal_keyboard_key_data(struct gui_priv *this, struct widget *wkbd, char *text, void(*func)(struct gui_priv *priv, struct widget *widget), void *data, void (*data_free)(void *data))
{
	struct widget *wk;

	gui_internal_widget_append(wkbd, wk=gui_internal_button_new_with_callback(this, text,
		NULL, gravity_center|orientation_vertical, func, data));
	wk->data_free=data_free;
	wk->background=this->background;
	wk->bl=6;
	wk->br=6;
	wk->bt=6;
	wk->bb=6;
	return wk;
}

static struct widget *
gui_internal_keyboard_key(struct gui_priv *this, struct widget *wkbd, char *text, char *key)
{
	return gui_internal_keyboard_key_data(this, wkbd, text, gui_internal_cmd_keypress, g_strdup(key), g_free);
}

static void gui_internal_keyboard_change(struct gui_priv *this, struct widget *key);

static struct widget *
gui_internal_keyboard_do(struct gui_priv *this, struct widget *key, int mode)
{
	struct widget *wkbdb,*wkbd,*wk;
	int i;
	int render=0;

	if (key) {
		gui_internal_highlight(this, NULL);
		wkbdb=key->data;
		mode=key->datai;
		render=1;
		gui_internal_widget_children_destroy(this, wkbdb);
	} else
		wkbdb=gui_internal_box_new(this, gravity_center|orientation_horizontal_vertical|flags_fill);
	wkbd=gui_internal_box_new(this, gravity_center|orientation_horizontal_vertical|flags_fill);
	wkbd->background=this->background;
	wkbd->cols=8;
	wkbd->spx=3;
	wkbd->spy=3;
	if (mode >= 0 && mode < 8) {
		for (i = 0 ; i < 26 ; i++) {
			char text[]={'A'+i,'\0'};
			gui_internal_keyboard_key(this, wkbd, text, text);
		}
		gui_internal_keyboard_key(this, wkbd, "_"," ");
		if (mode == 0) {
			gui_internal_keyboard_key(this, wkbd, "-","-");
			gui_internal_keyboard_key(this, wkbd, "'","'");
			gui_internal_keyboard_key_data(this, wkbd, "", NULL, NULL, NULL);
		} else {
			gui_internal_keyboard_key_data(this, wkbd, "", NULL, NULL, NULL);
			wk=gui_internal_keyboard_key_data(this, wkbd, "a", gui_internal_keyboard_change, wkbd, NULL);
			wk->datai=mode+8;
			wk=gui_internal_keyboard_key_data(this, wkbd, "1", gui_internal_keyboard_change, wkbd, NULL);
			wk->datai=mode+16;
		}
		wk=gui_internal_keyboard_key_data(this, wkbd, "Ä",gui_internal_keyboard_change, wkbdb,NULL);
		wk->datai=mode+24;
		gui_internal_keyboard_key(this, wkbd, "<-","\b");
	}
	if (mode >= 8 && mode < 16) {
		for (i = 0 ; i < 26 ; i++) {
			char text[]={'a'+i,'\0'};
			gui_internal_keyboard_key(this, wkbd, text, text);
		}
		gui_internal_keyboard_key(this, wkbd, "_"," ");
		if (mode == 8) {
			gui_internal_keyboard_key(this, wkbd, "-","-");
			gui_internal_keyboard_key(this, wkbd, "'","'");
			gui_internal_keyboard_key_data(this, wkbd, "", NULL, NULL, NULL);
		} else {
			gui_internal_keyboard_key_data(this, wkbd, "", NULL, NULL, NULL);
			wk=gui_internal_keyboard_key_data(this, wkbd, "A", gui_internal_keyboard_change, wkbd, NULL);
			wk->datai=mode-8;
			wk=gui_internal_keyboard_key_data(this, wkbd, "1", gui_internal_keyboard_change, wkbd, NULL);
			wk->datai=mode+8;
		}
		wk=gui_internal_keyboard_key_data(this, wkbd, "Ä",gui_internal_keyboard_change,wkbdb,NULL);
		wk->datai=mode+16;
		gui_internal_keyboard_key(this, wkbd, "<-","\b");
	}
	if (mode >= 16 && mode < 24) {
		for (i = 0 ; i < 10 ; i++) {
			char text[]={'0'+i,'\0'};
			gui_internal_keyboard_key(this, wkbd, text, text);
		}
		gui_internal_keyboard_key(this, wkbd, ".",".");
		gui_internal_keyboard_key(this, wkbd, "°","°");
		gui_internal_keyboard_key(this, wkbd, "'","'");
		gui_internal_keyboard_key(this, wkbd, "\"","\"");
		gui_internal_keyboard_key(this, wkbd, "-","-");
		gui_internal_keyboard_key(this, wkbd, "+","+");
		gui_internal_keyboard_key(this, wkbd, "*","*");
		gui_internal_keyboard_key(this, wkbd, "/","/");
		gui_internal_keyboard_key(this, wkbd, "(","(");
		gui_internal_keyboard_key(this, wkbd, ")",")");
		gui_internal_keyboard_key(this, wkbd, "=","=");
		gui_internal_keyboard_key(this, wkbd, "?","?");
		for (i = 0 ; i < 5 ; i++) {
			gui_internal_keyboard_key_data(this, wkbd, "", NULL, NULL, NULL);
		}
		if (mode == 8) {
			gui_internal_keyboard_key(this, wkbd, "-","-");
			gui_internal_keyboard_key(this, wkbd, "'","'");
			gui_internal_keyboard_key_data(this, wkbd, "", NULL, NULL, NULL);
		} else {
			gui_internal_keyboard_key_data(this, wkbd, "", NULL, NULL, NULL);
			wk=gui_internal_keyboard_key_data(this, wkbd, "A", gui_internal_keyboard_change, wkbd, NULL);
			wk->datai=mode-16;
			wk=gui_internal_keyboard_key_data(this, wkbd, "a", gui_internal_keyboard_change, wkbd, NULL);
			wk->datai=mode-8;
		}
		wk=gui_internal_keyboard_key_data(this, wkbd, "Ä",gui_internal_keyboard_change,wkbdb,NULL);
		wk->datai=mode+8;
		gui_internal_keyboard_key(this, wkbd, "<-","\b");
	}
	if (mode >= 24 && mode < 32) {
		gui_internal_keyboard_key(this, wkbd, "Ä","Ä");
		gui_internal_keyboard_key(this, wkbd, "Ö","Ö");
		gui_internal_keyboard_key(this, wkbd, "Ü","Ü");
		for (i = 0 ; i < 27 ; i++) {
			gui_internal_keyboard_key_data(this, wkbd, "", NULL, NULL, NULL);
		}
		wk=gui_internal_keyboard_key_data(this, wkbd, "A",gui_internal_keyboard_change,wkbdb,NULL);
		wk->datai=mode-24;
		gui_internal_keyboard_key(this, wkbd, "<-","\b");
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
	return gui_internal_keyboard_do(this, NULL, mode);
}

static void
gui_internal_keyboard_change(struct gui_priv *this, struct widget *key)
{
	gui_internal_keyboard_do(this, key, 0);
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

static void gui_internal_search_street(struct gui_priv *this, struct widget *widget);

static void
gui_internal_search(struct gui_priv *this, char *what, char *type)
{
	struct widget *wb,*wk,*w,*wr,*we,*wl,*wnext;
	char *country;
	gui_internal_search_list_new(this);
	wb=gui_internal_menu(this, what);
	w=gui_internal_box_new(this, gravity_center|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(wb, w);
	wr=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(w, wr);
	we=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|flags_fill);
	gui_internal_widget_append(wr, we);
	
	country=g_strdup_printf("country_%s", this->country_iso2);
	gui_internal_widget_append(we, gui_internal_image_new(this, image_new_xs(this, country)));
	g_free(country);

	gui_internal_widget_append(we, wk=gui_internal_label_new(this, NULL));
	gui_internal_widget_append(we, wnext=gui_internal_image_new(this, image_new_xs(this, "gui_active")));
	wnext->state |= STATE_SENSITIVE;
	wnext->func = gui_internal_search_street;
	wl=gui_internal_box_new(this, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(wr, wl);
	this->list=wl;
	wk->state |= STATE_EDIT;
	wk->background=this->background;
	wk->flags |= flags_expand|flags_fill;
	wk->func = gui_internal_search_changed;
	wk->name=g_strdup(type);
	gui_internal_widget_append(w, gui_internal_keyboard(this,0));
	gui_internal_menu_render(this);
}

static void
gui_internal_search_street(struct gui_priv *this, struct widget *widget)
{
	gui_internal_search(this,"Street","Street");
}

static void
gui_internal_search_street_in_town(struct gui_priv *this, struct widget *widget)
{
	dbg(0,"id %d\n", widget->item.id_lo);
	search_list_select(this->sl, attr_town_name, 0, 0);
	search_list_select(this->sl, attr_town_name, widget->item.id_lo, 1);
	gui_internal_search(this,"Street","Street");
}

static void
gui_internal_cmd_town(struct gui_priv *this, struct widget *wm)
{
	gui_internal_search(this,"Town","Town");
}


static void
gui_internal_cmd_street(struct gui_priv *this, struct widget *wm)
{
	gui_internal_search(this,"Town","Town");
}


static void
gui_internal_cmd_layout(struct gui_priv *this, struct widget *wm)
{
	struct attr attr;
	struct widget *w,*wb,*wl;
	struct attr_iter *iter;


	wb=gui_internal_menu(this, "Layout");
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
gui_internal_cmd_fullscreen(struct gui_priv *this, struct widget *wm)
{
	this->fullscreen=!this->fullscreen;
	this->win->fullscreen(this->win, this->fullscreen);
}


static void
gui_internal_cmd_display(struct gui_priv *this, struct widget *wm)
{
	struct widget *w;

	w=gui_internal_menu(this, "Display");	
	gui_internal_widget_append(w,
		gui_internal_button_new_with_callback(this, "Layout",
			image_new_l(this, "gui_display"), gravity_center|orientation_vertical,
			gui_internal_cmd_layout, NULL));
	if (this->fullscreen) {
		gui_internal_widget_append(w,
			gui_internal_button_new_with_callback(this, "Window Mode",
				image_new_l(this, "gui_leave_fullscreen"), gravity_center|orientation_vertical,
				gui_internal_cmd_fullscreen, NULL));
	} else {
		gui_internal_widget_append(w,
			gui_internal_button_new_with_callback(this, "Fullscreen",
				image_new_l(this, "gui_fullscreen"), gravity_center|orientation_vertical,
				gui_internal_cmd_fullscreen, NULL));
	}
	gui_internal_menu_render(this);
}

static void
gui_internal_cmd_quit(struct gui_priv *this, struct widget *wm)
{
	struct navit *nav=this->nav;
	navit_destroy(nav);
	main_remove_navit(nav);
}

static void
gui_internal_cmd_abort_navigation(struct gui_priv *this, struct widget *wm)
{
	navit_set_destination(this->nav, NULL, NULL);
}


static void
gui_internal_cmd_actions(struct gui_priv *this, struct widget *wm)
{
	struct widget *w,*wc;
	char *coord;

	w=gui_internal_menu(this, "Actions");	
	gui_internal_widget_append(w,
		gui_internal_button_new_with_callback(this, "Bookmarks",
			image_new_l(this, "gui_bookmark"), gravity_center|orientation_vertical,
			gui_internal_cmd_bookmarks, NULL));
	if (this->clickp_valid) {
		coord=coordinates(&this->clickp, '\n');
		gui_internal_widget_append(w,
			wc=gui_internal_button_new_with_callback(this, coord,
				image_new_l(this, "gui_map"), gravity_center|orientation_vertical,
				gui_internal_cmd_position, (void *)1));
		wc->name=g_strdup("Map Point");
		wc->c=this->clickp;
		g_free(coord);
	}
	if (this->vehicle_valid) {
		coord=coordinates(&this->vehiclep, '\n');
		gui_internal_widget_append(w,
			wc=gui_internal_button_new_with_callback(this, coord,
				image_new_l(this, "gui_rules"), gravity_center|orientation_vertical,
				gui_internal_cmd_position, NULL));
		wc->name=g_strdup("Vehicle Position");
		wc->c=this->vehiclep;
		g_free(coord);
	}
	gui_internal_widget_append(w,
		gui_internal_button_new_with_callback(this, "Town",
			image_new_l(this, "gui_rules"), gravity_center|orientation_vertical,
			gui_internal_cmd_town, NULL));
	gui_internal_widget_append(w,
		gui_internal_button_new_with_callback(this, "Street",
			image_new_l(this, "gui_rules"), gravity_center|orientation_vertical,
			gui_internal_cmd_street, NULL));
	gui_internal_widget_append(w,
		gui_internal_button_new_with_callback(this, "Quit",
			image_new_l(this, "gui_quit"), gravity_center|orientation_vertical,
			gui_internal_cmd_quit, NULL));
	gui_internal_widget_append(w,
		gui_internal_button_new_with_callback(this, "Stop\nNavigation",
			image_new_l(this, "gui_stop"), gravity_center|orientation_vertical,
			gui_internal_cmd_abort_navigation, NULL));
	gui_internal_menu_render(this);
}

static void
gui_internal_cmd_maps(struct gui_priv *this, struct widget *wm)
{
	struct attr attr, on, off, description, type, data;
	struct widget *w,*wb,*wma;
	char *label;
	struct attr_iter *iter;


	wb=gui_internal_menu(this, "Maps");
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
gui_internal_cmd_vehicle(struct gui_priv *this, struct widget *wm)
{
	struct attr attr,vattr;
	struct widget *w,*wb,*wl;
	struct attr_iter *iter;


	wb=gui_internal_menu(this, "Vehicle");
	w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	w->spy=this->spacing*3;
	gui_internal_widget_append(wb, w);
	iter=navit_attr_iter_new();
	while(navit_get_attr(this->nav, attr_vehicle, &attr, iter)) {
		vehicle_get_attr(attr.u.vehicle, attr_name, &vattr);
		wl=gui_internal_button_navit_attr_new(this, vattr.u.str, gravity_left_center|orientation_horizontal|flags_fill,
			&attr, NULL);
		gui_internal_widget_append(w, wl);
	}
	navit_attr_iter_destroy(iter);
	gui_internal_menu_render(this);
}


static void
gui_internal_cmd_rules(struct gui_priv *this, struct widget *wm)
{
	struct widget *wb,*w;
	struct attr on,off;
	wb=gui_internal_menu(this, "Rules");	
	w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	w->spy=this->spacing*3;
	gui_internal_widget_append(wb, w);
	on.u.num=1;
	off.u.num=0;
	on.type=off.type=attr_tracking;
	gui_internal_widget_append(w,
		gui_internal_button_navit_attr_new(this, "Lock on road", gravity_left_center|orientation_horizontal|flags_fill,
			&on, &off));
	on.type=off.type=attr_orientation;
	gui_internal_widget_append(w,
		gui_internal_button_navit_attr_new(this, "Northing", gravity_left_center|orientation_horizontal|flags_fill,
			&on, &off));
	on.type=off.type=attr_cursor;
	gui_internal_widget_append(w,
		gui_internal_button_navit_attr_new(this, "Map follows Vehicle", gravity_left_center|orientation_horizontal|flags_fill,
			&on, &off));
	gui_internal_widget_append(w,
		gui_internal_button_new(this, "Attack defenseless civilians",
			image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill));
	gui_internal_menu_render(this);
}

static void
gui_internal_cmd_settings(struct gui_priv *this, struct widget *wm)
{
	struct widget *w;

	w=gui_internal_menu(this, "Settings");	
	gui_internal_widget_append(w,
		gui_internal_button_new_with_callback(this, "Display",
			image_new_l(this, "gui_display"), gravity_center|orientation_vertical,
			gui_internal_cmd_display, NULL));
	gui_internal_widget_append(w,
		gui_internal_button_new_with_callback(this, "Maps",
			image_new_l(this, "gui_maps"), gravity_center|orientation_vertical,
			gui_internal_cmd_maps, NULL));
	gui_internal_widget_append(w,
		gui_internal_button_new_with_callback(this, "Vehicle",
			image_new_l(this, "gui_sound"), gravity_center|orientation_vertical,
			gui_internal_cmd_vehicle, NULL));
	gui_internal_widget_append(w,
		gui_internal_button_new_with_callback(this, "Rules",
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
	graphics_draw_mode(this->gra, draw_mode_begin);
	gui_internal_highlight(this, p);
	graphics_draw_mode(this->gra, draw_mode_end);
	
}


static void gui_internal_menu_root(struct gui_priv *this)
{
	struct widget *w;

	graphics_draw_mode(this->gra, draw_mode_begin);
	w=gui_internal_menu(this, "Main menu");	
	w->spx=this->spacing*10;
	gui_internal_widget_append(w, gui_internal_button_new_with_callback(this, "Actions",
			image_new_l(this, "gui_actions"), gravity_center|orientation_vertical,
			gui_internal_cmd_actions, NULL));
	gui_internal_widget_append(w, gui_internal_button_new_with_callback(this, "Settings",
			image_new_l(this, "gui_settings"), gravity_center|orientation_vertical,
			gui_internal_cmd_settings, NULL));
	gui_internal_widget_append(w, gui_internal_button_new(this, "Tools",
			image_new_l(this, "gui_tools"), gravity_center|orientation_vertical));

#if 0
	gui_internal_widget_append(w, gui_internal_button_new_with_callback(this, "Table Test",
			image_new_l(this, "gui_settings"), gravity_center|orientation_vertical,
			gui_internal_cmd_route, NULL));
#endif

							      
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
		&& vehicle_get_attr(attr.u.vehicle, attr_position_coord_geo, &attrp)) {
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
gui_internal_check_exit(struct gui_priv *this)
{
	struct graphics *gra=this->gra;
	if (! this->root.children) {
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
		graphics_draw_mode(gra, draw_mode_begin);
		gui_internal_highlight(this, p);
		graphics_draw_mode(gra, draw_mode_end);
	} else {
		this->pressed=0;
		graphics_draw_mode(gra, draw_mode_begin);
		gui_internal_call_highlighted(this);
		gui_internal_highlight(this, NULL);
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
	this->root.w=w;
	this->root.h=h;
	dbg(0,"w=%d h=%d children=%p\n", w, h, this->root.children);
	navit_resize(this->nav, w, h);
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
#if 0
	struct color cb={0x7fff,0x7fff,0x7fff,0xffff};
#endif
	struct color cb={0x0,0x0,0x0,0xffff};
	struct color cb2={0x4141,0x4141,0x4141,0xffff};
	struct color cbh={0x9fff,0x9fff,0x9fff,0xffff};
	struct color cf={0xbfff,0xbfff,0xbfff,0xffff};
	struct color cw={0xffff,0xffff,0xffff,0xffff};
	struct transformation *trans=navit_get_trans(this->nav);
	
	win=graphics_get_data(gra, "window");
        if (! win)
                return 1;
	this->gra=gra;
	this->win=win;
	transform_get_size(trans, &this->root.w, &this->root.h);
	graphics_register_resize_callback(gra, gui_internal_resize, this);
	graphics_register_button_callback(gra, gui_internal_button, this);
	graphics_register_motion_callback(gra, gui_internal_motion, this);
	graphics_register_keypress_callback(gra, gui_internal_keypress, this);
	this->background=graphics_gc_new(gra);
	graphics_gc_set_foreground(this->background, &cb);
	this->background2=graphics_gc_new(gra);
	graphics_gc_set_foreground(this->background2, &cb2);
	this->highlight_background=graphics_gc_new(gra);
	graphics_gc_set_foreground(this->highlight_background, &cbh);
	this->foreground=graphics_gc_new(gra);
	graphics_gc_set_foreground(this->foreground, &cf);
	this->text_background=graphics_gc_new(gra);
	graphics_gc_set_foreground(this->text_background, &cb);
	this->text_foreground=graphics_gc_new(gra);
	graphics_gc_set_foreground(this->text_foreground, &cw);
	return 0;
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
	navit_command_register(nav,"gui_internal_menu",callback_new_3(gui_internal_cmd_menu,this,NULL,1));
	navit_command_register(nav,"gui_internal_fullscreen",callback_new_2(gui_internal_cmd_fullscreen,this,NULL));

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
struct widget * gui_internal_widget_table_new(struct gui_priv * this, enum flags flags)
{
	struct widget * widget = g_new0(struct widget,1);
	struct table_data * data = NULL;
	widget->type=widget_table;
	widget->flags=flags;
	widget->data = g_new0(struct table_data,1);
	widget->data_free=g_free;
	data = (struct table_data*)widget->data;
	

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
	data->button_box->background=this->background2;
	data->button_box->bl=this->spacing;
	widget->children=g_list_append(widget->children,data->button_box);
	gui_internal_widget_pack(this,data->button_box);
	
	return widget;

}

/**
 *
 * @brief Adds a row to a table_widget.
 *
 * @param table The table to add the row to.
 * @param row A table_row widget that will be added to the table.
 */
void gui_internal_widget_table_add_row(struct widget * table,  struct widget *  row)
{
  table->children=g_list_append(table->children,row);
  
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
static GList * gui_internal_compute_table_dimensions(struct widget * w)
{
  
	GList * column_desc = NULL;
	GList * current_desc=NULL;
	GList * cur_row = w->children;
	struct widget * cur_row_widget=NULL;
	GList * cur_column=NULL;
	struct widget * cell_w=NULL;
	struct table_column_desc * current_cell=NULL;
	int height=0;
	int width=0;

  
	/**
	 * Scroll through the the table and
	 * 1. Compute the maximum width + height of each column across all rows.
	 */
	
	for(cur_row=w->children;  cur_row ; cur_row = g_list_next(cur_row) )
	{
		cur_row_widget = (struct widget*) cur_row->data;
		current_desc = column_desc;
		for(cur_column = cur_row_widget->children; cur_column; 
		    cur_column=g_list_next(cur_column))
		{
			cell_w = (struct widget*) cur_column->data;
			if(current_desc == 0) 
			{
				current_cell = g_new0(struct table_column_desc,1);	
				column_desc = g_list_append(column_desc,current_cell);
				current_desc = g_list_last(column_desc);
				current_cell->height=cell_w->h; 
				current_cell->width=cell_w->w; 
				
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
					current_cell->width = width;
				}
				current_desc = g_list_next(current_desc);
			}
			
		}/* column loop */
		
	} /*row loop */
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
  
	struct widget * row_widget=NULL;
	int height=0;
	int width=0;
	int count=0;
	GList * column_data = gui_internal_compute_table_dimensions(w);
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
		width = width + cell_desc->width;
		if(height < cell_desc->height) 
		{
			height = cell_desc->height;
		}
	}

	for(current=w->children; current; current=g_list_next(current))
	{
		if(current->data!= table_data->button_box)
		{
			count++;
		}
	}
	gui_internal_widget_pack(this,table_data->button_box);  
	w->h = count * height + table_data->button_box;
	
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
	int idx=0;
	int is_skipped=0;
	struct table_column_desc * dim=NULL;
	
	column_desc = gui_internal_compute_table_dimensions(w);
	
	y=w->p.y;
	
	/**
	 * Skip rows that are on previous pages.
	 */
	cur_row = w->children;
	if(table_data->top_row )
	{
		cur_row = table_data->top_row;
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
		x =w->p.x;        
		if(cur_row_widget == table_data->button_box )
		{
			continue;
		}
		dim = (struct table_column_desc*)current_desc->data;
		
		if( y + dim->height + table_data->button_box->h  >= w->p.y + w->h )
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
			x=x+dim->width;
			max_height = dim->height;
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
	if(table_data  && table_data->button_box )
	{
		table_data->button_box->p.y =w->c.y+w->h-table_data->button_box->h;
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
 * TEST function for demonstrating table.
 */
void gui_internal_cmd_route(struct gui_priv * this, struct widget * wm)
{
#if 0 
	struct widget * menu;
	struct widget * w;
	int idx;
	char buffer[10];
	
	w = gui_internal_widget_table_new(this,0);
	w->c = wm->c;
	for(idx=0; idx < 50; idx++)
	{
		struct widget * row = gui_internal_widget_table_row_new
			(this,gravity_left_top);
		sprintf(buffer, "Test %d",idx);
		row->children = g_list_append(row->children,
					      gui_internal_label_new(this,buffer));
		row->children= g_list_append(row->children,
					     gui_internal_label_new(this,"column 2 4 6 8"));
		gui_internal_widget_table_add_row(w,row);
	}
	menu=gui_internal_menu(this,"Route");
	gui_internal_widget_append(menu,w);
	gui_internal_menu_render(this);
#endif
}



/**
 * @brief handles the 'next page' table event.
 * A callback function that is invoked when the 'next page' button is pressed
 * to advance the contents of a table widget.
 *
 * @param this The graphics context.
 * @param wm The button widget that was pressed.
 */
static void gui_internal_table_button_next(struct gui_priv * this, struct widget * wm)
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
static void gui_internal_table_button_prev(struct gui_priv * this, struct widget * wm)
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

