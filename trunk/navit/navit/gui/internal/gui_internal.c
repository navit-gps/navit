/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2010 Navit Team
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
#ifdef HAVE_API_WIN32_BASE
#include <windows.h>
#endif
#ifndef _MSC_VER
#include <sys/time.h>
#else
#define snprintf sprintf_s
#endif /* _MSC_VER */
#include "item.h"
#include "file.h"
#include "navit.h"
#include "navit_nls.h"
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
#include "vehicleprofile.h"
#include "window.h"
#include "config_.h"
#include "keys.h"
#include "mapset.h"
#include "route.h"
#include "navit/search.h"
#include "track.h"
#include "country.h"
#include "config.h"
#include "event.h"
#include "navit_nls.h"
#include "navigation.h"
#include "gui_internal.h"
#include "command.h"
#include "xmlconfig.h"
#include "util.h"
#include "bookmarks.h"
#include "debug.h"
#include "fib.h"
#include "types.h"


extern char *version;

struct form {
	char *onsubmit;
};


struct menu_data {
	struct widget *search_list;
	struct widget *keyboard;
	struct widget *button_bar;
	struct widget *menu;
	int keyboard_mode;
	void (*redisplay)(struct gui_priv *priv, struct widget *widget, void *data);
	struct widget *redisplay_widget;
	char *href;
	struct attr refresh_callback_obj,refresh_callback;
};

enum gui_internal_reason {
  gui_internal_reason_click=1,
  gui_internal_reason_keypress,
  gui_internal_reason_keypress_finish
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
	enum gui_internal_reason reason;
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
	void (*wfree) (struct gui_priv *this_, struct widget * w);
	char *prefix;
	char *name;
	char *speech;
	char *command;
	struct pcoord c;
	struct item item;
	int selection_id;
	int state;
	struct point p;
	int wmin,hmin;
	int w,h;
	int textw,texth;
	int font_idx;
	int bl,br,bt,bb,spx,spy;
	int border;
	int packed;
	/**
	 * The number of widgets to layout horizontally when doing
	 * a orientation_horizontal_vertical layout
	 */
	int cols;
	enum flags flags;
	int flags2;
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
	struct form *form;
	GList *children;
	struct widget *parent;
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
   * This icon size is typically used in various lists and should be set to value which allows a list row to be easily cliked or dragged.
   */
  int icon_xs;
  /**
   * The size (in pixels) that s style icons (small) should be scaled to, used for the menu top row icons
   */
  int icon_s;
  /**
   * The size (in pixels) that l style icons should be scaled to, used for icons defined in the menu html
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
    , {300,32,48,64,3}
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
	struct attr self;
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
	struct graphics_font *fonts[3];
	/**
	 * The size (in pixels) that xs style icons should be scaled to.
	 * This icon size can be too small to click it on some devices.
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
	int pressed;
	struct widget *widgets;
	int widgets_count;
	int redraw;
	struct widget root;
	struct widget *highlighted,*editable;
	struct widget *highlighted_menu;
	struct pcoord clickp, vehiclep;
	struct attr *click_coord_geo, *position_coord_geo;
	struct search_list *sl;
	int ignore_button;
	int menu_on_map_click;
	char *on_map_click;
	int signal_on_map_click;
	char *country_iso2;
	int speech;
	int keyboard;
	int keyboard_required;
	/**
	 * The setting information read from the configuration file.
	 * values of -1 indicate no value was specified in the config file.
	 */
	struct gui_config_settings config;
	struct event_idle *idle;
	struct callback *motion_cb,*button_cb,*resize_cb,*keypress_cb,*window_closed_cb,*idle_cb, *motion_timeout_callback;
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
	int pitch;
	int flags_town,flags_street,flags_house_number;
	int radius;
	int mouse_button_clicked_on_map;
/* html */
	char *html_text;
	int html_depth;
	struct widget *html_container;
	int html_skip;
	char *html_anchor;
	char *href;
	int html_anchor_found;
	struct form *form;
	struct html {
		int skip;
		enum html_tag {
			html_tag_none,
			html_tag_a,
			html_tag_h1,
			html_tag_html,
			html_tag_img,
			html_tag_script,
			html_tag_form,
			html_tag_input,
			html_tag_div,
		} tag;
		char *command;
		char *name;
		char *href;
		char *refresh_cond;
		struct widget *w;
		struct widget *container;
	} html[10];

/* gestures */	

	struct gesture_elem {
		int msec;
		struct point p;
	} gesture_ring[GESTURE_RINGSIZE];
	int gesture_ring_last, gesture_ring_first;


	int results_map_population;
};





struct html_tag_map {
	char *tag_name;
	enum html_tag tag;
} html_tag_map[] = {
	{"a",html_tag_a},
	{"h1",html_tag_h1},
	{"html",html_tag_html},
	{"img",html_tag_img},
	{"script",html_tag_script},
	{"form",html_tag_form},
	{"input",html_tag_input},
	{"div",html_tag_div},
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
   * A container box that is the child of the table widget that contains+groups
   * the next and previous button.
   */
  struct widget * button_box;

  /**
   * Button box should not be displayed if button_box_hide is not zero.
   */
  int button_box_hide;
  
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
static void gui_internal_widget_prepend(struct widget *parent, struct widget *child);
static void gui_internal_widget_insert_before(struct widget *parent, struct widget *sibling, struct widget *child);
static void gui_internal_widget_insert_sorted(struct widget *parent, struct widget *child, GCompareFunc func);
static void gui_internal_widget_destroy(struct gui_priv *this, struct widget *w);
static void gui_internal_apply_config(struct gui_priv *this);

static struct widget* gui_internal_widget_table_new(struct gui_priv * this, enum flags flags, int buttons);
static struct widget * gui_internal_widget_table_row_new(struct gui_priv * this, enum flags flags);
static void gui_internal_table_hide_rows(struct table_data * table_data);
static void gui_internal_table_render(struct gui_priv * this, struct widget * w);
static void gui_internal_table_pack(struct gui_priv * this, struct widget * w);
static void gui_internal_table_button_next(struct gui_priv * this, struct widget * wm, void *data);
static void gui_internal_table_button_prev(struct gui_priv * this, struct widget * wm, void *data);
static void gui_internal_widget_table_clear(struct gui_priv * this,struct widget * table);
static int gui_internal_widget_table_is_empty(struct gui_priv *this,struct widget * table);
static GList * gui_internal_widget_table_top_row(struct gui_priv *this, struct widget * table);
static GList * gui_internal_widget_table_next_row(GList * row);
static GList * gui_internal_widget_table_prev_row(GList * row);
static GList * gui_internal_widget_table_first_row(GList * row);
static struct widget * gui_internal_widget_table_row_new(struct gui_priv * this, enum flags flags);
static void gui_internal_table_data_free(void * d);
static void gui_internal_route_update(struct gui_priv * this, struct navit * navit,
				      struct vehicle * v);
static void gui_internal_route_screen_free(struct gui_priv * this_,struct widget * w);
static void gui_internal_populate_route_table(struct gui_priv * this,
				       struct navit * navit);
static void gui_internal_search_idle_end(struct gui_priv *this);
static void gui_internal_search(struct gui_priv *this, char *what, char *type, int flags);
static void gui_internal_search_house_number(struct gui_priv *this, struct widget *widget, void *data);
static void gui_internal_search_house_number_in_street(struct gui_priv *this, struct widget *widget, void *data);
static void gui_internal_search_street(struct gui_priv *this, struct widget *widget, void *data);
static void gui_internal_search_street_in_town(struct gui_priv *this, struct widget *widget, void *data);
static void gui_internal_search_town(struct gui_priv *this, struct widget *wm, void *data);
static void gui_internal_search_town_in_country(struct gui_priv *this, struct widget *wm);
static void gui_internal_search_country(struct gui_priv *this, struct widget *widget, void *data);
static void gui_internal_check_exit(struct gui_priv *this);
static void gui_internal_cmd_view_in_browser(struct gui_priv *this, struct widget *wm, void *data);

static struct widget *gui_internal_keyboard_do(struct gui_priv *this, struct widget *wkbdb, int mode);
static struct menu_data * gui_internal_menu_data(struct gui_priv *this);

static int gui_internal_is_active_vehicle(struct gui_priv *this, struct vehicle *vehicle);
static void gui_internal_html_menu(struct gui_priv *this, const char *document, char *anchor);
static void gui_internal_html_load_href(struct gui_priv *this, char *href, int replace);
static void gui_internal_destroy(struct gui_priv *this);
static void gui_internal_enter(struct gui_priv *this, int ignore);
static void gui_internal_enter_setup(struct gui_priv *this);
static void gui_internal_html_main_menu(struct gui_priv *this);
static void gui_internal_menu_vehicle_settings(struct gui_priv *this, struct vehicle *v, char *name);


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
image_new_scaled(struct gui_priv *this, const char *name, int w, int h)
{
	struct graphics_image *ret=NULL;
	char *full_path=NULL;
	full_path=graphics_icon_path(name);
	ret=graphics_image_new_scaled(this->gra, full_path, w, h);
	dbg(1,"Trying to load image '%s' (w=%d, h=%d): %s\n", name, w, h, ret ? "OK" : "NOT FOUND");
	g_free(full_path);
	if (!ret)
		dbg(0,"Failed to load image for '%s' (w=%d, h=%d)\n", name, w, h);
	return ret;
}

#if 0
static struct graphics_image *
image_new_o(struct gui_priv *this, char *name)
{
	return image_new_scaled(this, name, -1, -1);
}
#endif

/*
 * * Display image scaled to xs (extra small) size
 * * This image size can be too small to click it on some devices.
 * * @param this Our gui context
 * * @param name image name
 * * @returns image_struct Ptr to scaled image struct or NULL if not scaled or found
 * */
static struct graphics_image *
image_new_xs(struct gui_priv *this, const char *name)
{
	return image_new_scaled(this, name, this->icon_xs, this->icon_xs);
}

/*
 * * Display image scaled to s (small) size
 * * @param this Our gui context
 * * @param name image name
 * * @returns image_struct Ptr to scaled image struct or NULL if not scaled or found
 * */

static struct graphics_image *
image_new_s(struct gui_priv *this, const char *name)
{
	return image_new_scaled(this, name, this->icon_s, this->icon_s);
}

/*
 * * Display image scaled to l (large) size
 * * @param this Our gui context
 * * @param name image name
 * * @returns image_struct Ptr to scaled image struct or NULL if not scaled or found
 * */
static struct graphics_image *
image_new_l(struct gui_priv *this, const char *name)
{
	return image_new_scaled(this, name, this->icon_l, this->icon_l);
}

static char *
coordinates_geo(const struct coord_geo *gc, char sep)
{
	char latc='N',lngc='E';
	int lat_deg,lat_min,lat_sec;
	int lng_deg,lng_min,lng_sec;
	struct coord_geo g=*gc;

	if (g.lat < 0) {
		g.lat=-g.lat;
		latc='S';
	}
	if (g.lng < 0) {
		g.lng=-g.lng;
		lngc='W';
	}
	lat_sec=fmod(g.lat*3600+0.5,60);
	lat_min=fmod(g.lat*60-lat_sec/60.0+0.5,60);
	lat_deg=g.lat-lat_min/60.0-lat_sec/3600.0+0.5;
	lng_sec=fmod(g.lng*3600+0.5,60);
	lng_min=fmod(g.lng*60-lng_sec/60.0+0.5,60);
	lng_deg=g.lng-lng_min/60.0-lng_sec/3600.0+0.5;;
	return g_strdup_printf("%d°%d'%d\" %c%c%d°%d'%d\" %c",lat_deg,lat_min,lat_sec,latc,sep,lng_deg,lng_min,lng_sec,lngc);
}

static char *
coordinates(struct pcoord *pc, char sep)
{
	struct coord_geo g;
	struct coord c;
	c.x=pc->x;
	c.y=pc->y;
	transform_to_geo(pc->pro, &c, &g);
	return coordinates_geo(&g, sep);

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
gui_internal_label_font_new(struct gui_priv *this, char *text, int font)
{
	struct point p[4];
	int w=0;
	int h=0;

	struct widget *widget=g_new0(struct widget, 1);
	widget->type=widget_label;
	widget->font_idx=font;
	if (text) {
		widget->text=g_strdup(text);
		graphics_get_text_bbox(this->gra, this->fonts[font], text, 0x10000, 0x0, p, 0);
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
gui_internal_label_new(struct gui_priv *this, char *text)
{
	return gui_internal_label_font_new(this, text, 0);
}

static struct widget *
gui_internal_label_new_abbrev(struct gui_priv *this, char *text, int maxwidth)
{
	struct widget *ret=NULL;
	char *tmp=g_malloc(strlen(text)+3), *p;
	p=text+strlen(text);
	while ((p=g_utf8_find_prev_char(text, p)) >= text) {
		int i=p-text;
		strcpy(tmp, text);
		strcpy(tmp+i,"..");
		ret=gui_internal_label_new(this, tmp);
		if (ret->w < maxwidth)
			break;
		gui_internal_widget_destroy(this, ret);
		ret=NULL;
	}
	if(!ret)
		ret=gui_internal_label_new(this, "");
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
	if (w->state & STATE_EDIT)
		graphics_draw_rectangle(this->gra, this->highlight_background, &pnt, w->w, w->h);
	if (w->text) {
		char *text;
		char *startext=(char*)g_alloca(strlen(w->text)+1);
		text=w->text;
		if (w->flags2 & 1) {
			int i;
			for (i = 0 ; i < strlen(text); i++)
				startext[i]='*';
			startext[i]='\0';
			text=startext;
		}
		if (w->flags & gravity_right) {
			pnt.y+=w->h-this->spacing;
			pnt.x+=w->w-w->textw-this->spacing;
			graphics_draw_text(this->gra, w->foreground, w->text_background, this->fonts[w->font_idx], text, &pnt, 0x10000, 0x0);
		} else {
			pnt.y+=w->h-this->spacing;
			graphics_draw_text(this->gra, w->foreground, w->text_background, this->fonts[w->font_idx], text, &pnt, 0x10000, 0x0);
		}
	}
}

/**
 * @brief A text box is a widget that renders a text string containing newlines.
 * The string will be broken up into label widgets at each newline with a vertical layout.
 *
 */
static struct widget *
gui_internal_text_font_new(struct gui_priv *this, char *text, int font, enum flags flags)
{
	char *s=g_strdup(text),*s2,*tok;
	struct widget *ret=gui_internal_box_new(this, flags);
	s2=s;
	while ((tok=strtok(s2,"\n"))) {
		gui_internal_widget_append(ret, gui_internal_label_font_new(this, tok, font));
		s2=NULL;
	}
	gui_internal_widget_pack(this,ret);
	g_free(s);
	return ret;
}

static struct widget *
gui_internal_text_new(struct gui_priv *this, char *text, enum flags flags)
{
	return gui_internal_text_font_new(this, text, 0, flags);
}


static struct widget *
gui_internal_button_font_new_with_callback(struct gui_priv *this, char *text, int font, struct graphics_image *image, enum flags flags, void(*func)(struct gui_priv *priv, struct widget *widget, void *data), void *data)
{
	struct widget *ret=NULL;
	ret=gui_internal_box_new(this, flags);
	if (ret) {
		if (image)
			gui_internal_widget_append(ret, gui_internal_image_new(this, image));
		if (text)
			gui_internal_widget_append(ret, gui_internal_text_font_new(this, text, font, gravity_center|orientation_vertical));
		ret->func=func;
		ret->data=data;
		if (func) {
			ret->state |= STATE_SENSITIVE;
			ret->speech=g_strdup(text);
		}
	}
	return ret;

}

static struct widget *
gui_internal_button_new_with_callback(struct gui_priv *this, char *text, struct graphics_image *image, enum flags flags, void(*func)(struct gui_priv *priv, struct widget *widget, void *data), void *data)
{
	return gui_internal_button_font_new_with_callback(this, text, 0, image, flags, func, data);
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
	struct graphics_image *image=NULL;
	struct widget *ret;
	if (!on && !off)
		return NULL;
	image=image_new_xs(this, "gui_inactive");
	ret=gui_internal_button_new_with_callback(this, text, image, flags, gui_internal_button_attr_pressed, NULL);
	if (on)
		ret->on=*on;
	if (off)
		ret->off=*off;
	ret->get_attr=(int (*)(void *, enum attr_type, struct attr *, struct attr_iter *))navit_get_attr;
	ret->set_attr=(int (*)(void *, struct attr *))navit_set_attr;
	ret->remove_cb=(void (*)(void *, struct callback *))navit_remove_callback;
	ret->instance=this->nav;
	ret->cb=callback_new_attr_2(callback_cast(gui_internal_button_attr_callback), on?on->type:off->type, this, ret);
	navit_add_callback(this->nav, ret->cb);
	gui_internal_button_attr_update(this, ret);
	return ret;
}

static struct widget *
gui_internal_button_map_attr_new(struct gui_priv *this, char *text, enum flags flags, struct map *map, struct attr *on, struct attr *off, int deflt)
{
	struct graphics_image *image=NULL;
	struct widget *ret;
	image=image_new_xs(this, "gui_inactive");
	if (!on && !off)
		return NULL;
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
	ret->cb=callback_new_attr_2(callback_cast(gui_internal_button_attr_callback), on?on->type:off->type, this, ret);
	map_add_callback(map, ret->cb);
	gui_internal_button_attr_update(this, ret);
	return ret;
}

static struct widget *
gui_internal_button_new(struct gui_priv *this, char *text, struct graphics_image *image, enum flags flags)
{
	return gui_internal_button_new_with_callback(this, text, image, flags, NULL, NULL);
}

#if 0
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
#endif

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
		if (!found) {
			found=gui_internal_find_widget(menu, &this->current, STATE_EDITABLE);
			if (found) {
				if (this->editable && this->editable != found) {
					this->editable->state &= ~ STATE_EDIT;
					gui_internal_widget_render(this, this->editable);
				}
				found->state |= STATE_EDIT;
				gui_internal_widget_render(this, found);
				this->editable=found;
				found=NULL;
			}
		}
	}
	gui_internal_highlight_do(this, found);
	this->motion_timeout_event=NULL;
}


static void gui_internal_gesture_ring_clear(struct gui_priv *this)
{
	this->gesture_ring_last=this->gesture_ring_first=0;
};


static struct gesture_elem * gui_internal_gesture_ring_get(struct gui_priv *this, int i)
{
	int n=(this->gesture_ring_last-i)%GESTURE_RINGSIZE;
	if(n==this->gesture_ring_first)
		return NULL;
	return this->gesture_ring+n;
};

static void gui_internal_gesture_ring_add(struct gui_priv *this, struct point *p)
{
	int msec;
#ifndef HAVE_API_WIN32_CE
	struct timeval tv;
	gettimeofday(&tv,NULL);
	msec=tv.tv_sec*1000+tv.tv_usec/1000;
#else
	msec=GetTickCount();
#endif
	this->gesture_ring_last++;
	this->gesture_ring_last%=GESTURE_RINGSIZE;
	if(this->gesture_ring_last==this->gesture_ring_first) {
		this->gesture_ring_first++;
		this->gesture_ring_first%=GESTURE_RINGSIZE;
   	}
	this->gesture_ring[this->gesture_ring_last].p=*p;
	this->gesture_ring[this->gesture_ring_last].msec=msec;
	dbg(2,"msec=%d x=%d y=%d\n",msec,p->x,p->y);
};


/*
 * @brief Calculate movement vector and timing of the gesture.
 * @param in this gui context
 * @param in msec time in milliseconds to find gesture within
 * @param out p0 pointer to the point object, where gesture starting point coordinates should be placed. Can be NULL.
 * @param out dx pointer to variable to store horizontal movement of the gesture.
 * @param out dy pointer to variable to store vertical movement of the gesture.
 * @returns amount of time the actual movement took.
 */
static int gui_internal_gesture_get_vector(struct gui_priv *this, int msec, struct point *p0, int *dx, int *dy)
{
	struct gesture_elem *g;
	int x,y,dt;
	int i;

	dt=0;

	if(dx) *dx=0;
	if(dy) *dy=0;
	if(p0) {
		p0->x=-1;
		p0->y=-1;
	}

	g=gui_internal_gesture_ring_get(this,0);
	if(!g)
		return 0;
	x=g->p.x;
	y=g->p.y;
	if(p0) {
		*p0=g->p;
	}
	msec=g->msec;
	dbg(2,"%d %d %d\n",g->msec, g->p.x, g->p.y);
	for(i=1;(g=gui_internal_gesture_ring_get(this,i))!=NULL;i++) {
		if( msec-g->msec > 1000 )
			break;
		dt=msec-g->msec;
		if(dx) *dx=x-g->p.x;
		if(dy) *dy=y-g->p.y;
		if(p0) {
			*p0=g->p;
		}
		dbg(2,"%d %d %d\n",g->msec, g->p.x, g->p.y);
	}
	return dt;
}

static int gui_internal_gesture_do(struct gui_priv *this)
{
	int dt;
	int dx,dy;
	
	dt=gui_internal_gesture_get_vector(this, 1000, NULL, &dx, &dy);

	if( abs(dx) > this->icon_s*3 && abs(dy) < this->icon_s ) {
		struct widget *wt;
		dbg(1,"horizontal dx=%d dy=%d\n",dx,dy);

		/* Prevent swiping if widget was scrolled beforehand */
		if(this->pressed==2)
			return 0;

		for(wt=this->highlighted;wt && wt->type!=widget_table;wt=wt->parent);
		if(!wt || wt->type!=widget_table || !wt->data)
			return 0;
		if(this->highlighted) {
			this->highlighted->state &= ~STATE_HIGHLIGHTED;
			this->highlighted=NULL;
		}
		if(dx<0)
			gui_internal_table_button_next(this,NULL,wt);
		else
			gui_internal_table_button_prev(this,NULL,wt);
		return 1;
	} else if( abs(dy) > this->icon_s*3 && abs(dx) < this->icon_s ) {
		dbg(1,"vertical dx=%d dy=%d\n",dx,dy);
	} else if (dt>300 && abs(dx) <this->icon_s && abs(dy) <this->icon_s ) {
		dbg(1,"longtap dx=%d dy=%d\n",dx,dy);
	} else {
		dbg(1,"none dx=%d dy=%d\n",dx,dy);
	}
	
	return 0;

};

static void gui_internal_motion_cb(struct gui_priv *this)
{
	this->motion_timeout_event=NULL;
	gui_internal_gesture_ring_add(this, &(this->current));

	/* Check for scrollable table below the highligted item if there's a movement with the button pressed */
	if (this->pressed && this->highlighted) {
		struct widget *wt=NULL;
		struct widget *wr=NULL;
		int dx,dy;
		
		/* Guard against accidental scrolling when user is likely going to swipe */
		gui_internal_gesture_get_vector(this, 1000, NULL, &dx, &dy);
		if(abs(dx)>abs(dy) || abs(dy)<this->icon_s)
			return;
	
		if(this->highlighted)
			for(wr=this->highlighted;wr && wr->type!=widget_table_row;wr=wr->parent);
		if(wr)
			wt=wr->parent;

		if(wt && wt->type==widget_table && (wt->state & STATE_SCROLLABLE)) {
			struct table_data *td=wt->data;
			GList *top=NULL;
			GList *btm=NULL;
			GList *ttop, *tbtm;
			
			

			if(!wr || !wr->h)
				return;
			
			if(this->current.y < wr->p.y  && wr!=td->top_row->data ) {
				int n=(wr->p.y-this->current.y)/wr->h+1;

				btm=td->bottom_row;
				top=td->top_row;

				while(n-->0 && (tbtm=gui_internal_widget_table_next_row(btm))!=NULL && (ttop=gui_internal_widget_table_next_row(top))!=NULL) {
					top=ttop;
					btm=tbtm;
					if(top->data==wr)
						break;
				}
				this->pressed=2;
			} else if (this->current.y > wr->p.y + wr->h ) {
				int y=wt->p.y+wt->h-wr->h;
				int n;

				if(td->button_box && td->button_box->p.y!=0)
		    			y=td->button_box->p.y - td->button_box->h;

				if(y>this->current.y)
					y=this->current.y;

				n=(y - wr->p.y )/wr->h;

				btm=td->bottom_row;
				top=td->top_row;
				
				while(n-->0 && (ttop=gui_internal_widget_table_prev_row(top))!=NULL && (tbtm=gui_internal_widget_table_prev_row(btm))!=NULL) {
					btm=tbtm;
					top=ttop;
					if(btm->data==wr)
						break;
				}
				this->pressed=2;
			}
			if( top && btm && (td->top_row!=top || td->bottom_row!=btm) ) {
				gui_internal_table_hide_rows(wt->data);
				td->top_row=top;
				td->bottom_row=btm;
				graphics_draw_mode(this->gra, draw_mode_begin);
				gui_internal_widget_render(this,wt);
				graphics_draw_mode(this->gra, draw_mode_end);
			}

			return;
		}
	}

	/* Else, just move highlight after pointer if there's nothing to scroll */
	gui_internal_highlight(this);
}

static struct widget *
gui_internal_box_new_with_label(struct gui_priv *this, enum flags flags, const char *label)
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
	w->border=1;
	w->foreground=this->foreground;
#endif
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
	int x0,x=0,y=0,width=0,height=0,owidth=0,oheight=0,expand=0,expandd=1,count=0,rows=0,cols=w->cols ? w->cols : 0;
	GList *l;
	int orientation=w->flags & 0xffff0000;

	if (!cols)
		cols=this->cols;
	if (!cols) {
		 if ( this->root.w > this->root.h )
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
			expandd=w->w-width+expand;
			owidth=w->w;
		} else
			expandd=expand=1;
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
			expandd=w->h-height+expand;
			oheight=w->h;
		} else
			expandd=expand=1;
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
		expandd=expand=1;
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
		w->packed=1;
	}
#if 0
	if (expand < 100)
		expand=100;
#endif

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
				wc->w=wc->w*expandd/expand;
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
				wc->h=wc->h*expandd/expand;
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
gui_internal_widget_reset_pack(struct gui_priv *this, struct widget *w)
{
	struct widget *wc;
	GList *l;

	l=w->children;
	while (l) {
		wc=l->data;
		gui_internal_widget_reset_pack(this, wc);
		l=g_list_next(l);
	}
	if (w->packed) {
		w->w=0;
		w->h=0;
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
	child->parent=parent;
}

static void gui_internal_widget_prepend(struct widget *parent, struct widget *child)
{
	if (! child->background)
		child->background=parent->background;
	parent->children=g_list_prepend(parent->children, child);
	child->parent=parent;
}

static void gui_internal_widget_insert_before(struct widget *parent, struct widget *sibling, struct widget *child)
{
	GList *sib=NULL;
	if (! child->background)
		child->background=parent->background;

	if(sibling) 
		sib=g_list_find(parent->children,sibling);
	parent->children=g_list_insert_before(parent->children, sib, child);
	child->parent=parent;
}

static void gui_internal_widget_insert_sorted(struct widget *parent, struct widget *child, GCompareFunc func)
{
	if (! child->background)
		child->background=parent->background;

	parent->children=g_list_insert_sorted(parent->children, child, func);
	child->parent=parent;
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
	g_free(w->command);
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
	if (w==this->highlighted)
	    this->highlighted=NULL;
	if(w->wfree)
		w->wfree(this,w);
	else
		g_free(w);
}


static void
gui_internal_widget_render(struct gui_priv *this, struct widget *w)
{
	if(w->p.x > this->root.w || w->p.y > this->root.h)
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
	this->highlighted->reason=gui_internal_reason_click;
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
gui_internal_menu_destroy(struct gui_priv *this, struct widget *w)
{
	struct menu_data *menu_data=w->menu_data;
	if (menu_data) {
		if (menu_data->refresh_callback_obj.type) {
			struct object_func *func;
			func=object_func_lookup(menu_data->refresh_callback_obj.type);
			if (func && func->remove_attr)
				func->remove_attr(menu_data->refresh_callback_obj.u.data, &menu_data->refresh_callback);
		}
		if (menu_data->refresh_callback.u.callback)
			callback_destroy(menu_data->refresh_callback.u.callback);

		g_free(menu_data->href);
		g_free(menu_data);
	}
	gui_internal_widget_destroy(this, w);
	this->root.children=g_list_remove(this->root.children, w);
}

static void
gui_internal_prune_menu_do(struct gui_priv *this, struct widget *w, int render)
{
	GList *l;
	struct widget *wr,*wd;
	gui_internal_search_idle_end(this);
	while ((l = g_list_last(this->root.children))) {
		wd=l->data;
		if (wd == w) {
			void (*redisplay)(struct gui_priv *priv, struct widget *widget, void *data);
			if (!render)
				return;
			gui_internal_say(this, w, 0);
			redisplay=w->menu_data->redisplay;
			wr=w->menu_data->redisplay_widget;
			if (!w->menu_data->redisplay && !w->menu_data->href) {
				gui_internal_widget_render(this, w);
				return;
			}
			if (redisplay) {
				gui_internal_menu_destroy(this, w);
				redisplay(this, wr, wr->data);
			} else {
				char *href=g_strdup(w->menu_data->href);
				gui_internal_menu_destroy(this, w);
				gui_internal_html_load_href(this, href, 0);
				g_free(href);
			}
			return;
		}
		gui_internal_menu_destroy(this, wd);
	}
}

static void
gui_internal_prune_menu(struct gui_priv *this, struct widget *w)
{
	gui_internal_prune_menu_do(this, w, 1);
}

static void
gui_internal_prune_menu_count(struct gui_priv *this, int count, int render)
{
	GList *l=g_list_last(this->root.children);
	struct widget *w=NULL;
	while (l && count-- > 0)
		l=g_list_previous(l);
	if (l) {
		w=l->data;
		gui_internal_prune_menu_do(this, w, render);
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
gui_internal_cmd2_back(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	graphics_draw_mode(this->gra, draw_mode_begin);
	gui_internal_back(this, NULL, NULL);
	graphics_draw_mode(this->gra, draw_mode_end);
	gui_internal_check_exit(this);
}

static void
gui_internal_cmd2_back_to_map(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	gui_internal_prune_menu(this, NULL);
}

static void
gui_internal_cmd_main_menu(struct gui_priv *this, struct widget *wm, void *data)
{
	struct widget *w=this->root.children->data;
	if (w && w->menu_data && w->menu_data->href && !strcmp(w->menu_data->href,"#Main Menu")) 
		gui_internal_prune_menu(this, w);
	else
		gui_internal_html_main_menu(this);
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
	1024:Don't show back button
	2048:No highlighting of keyboard
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
			wc=l?l->data:NULL;
		}
		if(wc) {
			wcn=gui_internal_label_new(this, ".. »");
			wcn->foreground=foreground;
			wcn->func=gui_internal_cmd_return;
			wcn->data=wc;
			wcn->state |= STATE_SENSITIVE;
			res=g_list_prepend(res, wcn);
		}
	}
	l=res;
	while (l) {
		gui_internal_widget_append(w, l->data);
		l=g_list_next(l);
	}
	if (this->flags & 32) {
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
	struct widget *w,*wc,*wcn;
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

  dbg(1,"w=%d h=%d\n", this->root.w, this->root.h);
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
gui_internal_menu(struct gui_priv *this, const char *label)
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
	if (!this->fonts[0]) {
		this->fonts[0]=graphics_font_new(this->gra,this->font_size,1);
		this->fonts[1]=graphics_font_new(this->gra,this->font_size*66/100,1);
		this->fonts[2]=graphics_font_new(this->gra,this->font_size*50/100,1);
	}
 	topbox->menu_data=g_new0(struct menu_data, 1);
	gui_internal_widget_append(topbox, menu);
	w=gui_internal_top_bar(this);
	gui_internal_widget_append(menu, w);
	w=gui_internal_box_new(this, gravity_center|orientation_horizontal_vertical|flags_expand|flags_fill);
	w->spx=4*this->spacing;
	w->w=menu->w;
	gui_internal_widget_append(menu, w);
	if (this->flags & 16 && !(this->flags & 1024)) {
		struct widget *wlb,*wb,*wm=w;
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
	gui_internal_widget_pack(this, topbox);
	gui_internal_widget_reset_pack(this, topbox);
        topbox->w=this->root.w;
        topbox->h=this->root.h;
	menu->w=this->root.w;
	menu->h=this->root.h;
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
gui_internal_menu_reset_pack(struct gui_priv *this)
{
	GList *l;
	struct widget *top_box;

	l=g_list_last(this->root.children);
	top_box=l->data;
	gui_internal_widget_reset_pack(this, top_box);
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
	char *name=data;
	dbg(0,"c=%d:0x%x,0x%x\n", wm->c.pro, wm->c.x, wm->c.y);
	navit_set_destination(this->nav, &wm->c, name, 1);
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
	struct attr v;
	if(data) {
		v.type=attr_vehicle;
		v.u.vehicle=NULL;
		navit_set_attr(this->nav, &v);
	}
	navit_set_position(this->nav, &wm->c);
	gui_internal_prune_menu(this, NULL);
}

static void
gui_internal_cmd_add_bookmark_do(struct gui_priv *this, struct widget *widget)
{
	GList *l;
	struct attr attr;
	dbg(0,"text='%s'\n", widget->text);
	if (widget->text && strlen(widget->text)){
		navit_get_attr(this->nav, attr_bookmarks, &attr, NULL);
		bookmarks_add_bookmark(attr.u.bookmarks, &widget->c, widget->text);
	}
	g_free(widget->text);
	widget->text=NULL;
	l=g_list_previous(g_list_last(this->root.children));
	gui_internal_prune_menu(this, l->data);
}

static void
gui_internal_cmd_add_bookmark_folder_do(struct gui_priv *this, struct widget *widget)
{
	GList *l;
	struct attr attr;
	dbg(0,"text='%s'\n", widget->text);
	if (widget->text && strlen(widget->text)){
		navit_get_attr(this->nav, attr_bookmarks, &attr, NULL);
		bookmarks_add_bookmark(attr.u.bookmarks, NULL, widget->text);
	}
	g_free(widget->text);
	widget->text=NULL;
	l=g_list_previous(g_list_previous(g_list_last(this->root.children)));
	gui_internal_prune_menu(this, l->data);
}

static void
gui_internal_cmd_add_bookmark_clicked(struct gui_priv *this, struct widget *widget, void *data)
{
	gui_internal_cmd_add_bookmark_do(this, widget->data);
}

static void
gui_internal_cmd_add_bookmark_folder_clicked(struct gui_priv *this, struct widget *widget, void *data)
{
	gui_internal_cmd_add_bookmark_folder_do(this, widget->data);
}

static void
gui_internal_cmd_rename_bookmark_clicked(struct gui_priv *this, struct widget *widget,void *data)
{
	struct widget *w=(struct widget*)widget->data;
	GList *l;
	struct attr attr;
	dbg(0,"text='%s'\n", w->text);
	if (w->text && strlen(w->text)){
		navit_get_attr(this->nav, attr_bookmarks, &attr, NULL);
		bookmarks_rename_bookmark(attr.u.bookmarks, w->name, w->text);
	}
	g_free(w->text);
	g_free(w->name);
	w->text=NULL;
	w->name=NULL;
	l=g_list_previous(g_list_previous(g_list_previous(g_list_last(this->root.children))));
	gui_internal_prune_menu(this, l->data);
}

/**
 * @brief Generic notification function for Editable widgets to call Another widget notification function when Enter is pressed in editable field.
 * The Editable widget should have data member pointing to the Another widget.
 */
static void
gui_internal_call_linked_on_finish(struct gui_priv *this, struct widget *wm, void *data)
{
	if (wm->reason==gui_internal_reason_keypress_finish && data) {
			struct widget *w=data;
			if(w->func)
				w->func(this, w, w->data);
	}
}

static struct widget * gui_internal_keyboard(struct gui_priv *this, int mode);

static void
gui_internal_cmd_add_bookmark2(struct gui_priv *this, struct widget *wm, void *data)
{
	struct widget *w,*wb,*wk,*wl,*we,*wnext;
	char *name=data;
	wb=gui_internal_menu(this,_("Add Bookmark"));
	w=gui_internal_box_new(this, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(wb, w);
	we=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|flags_fill);
	gui_internal_widget_append(w, we);
	gui_internal_widget_append(we, wk=gui_internal_label_new(this, name));
	wk->state |= STATE_EDIT|STATE_EDITABLE|STATE_CLEAR;
	wk->background=this->background;
	wk->flags |= flags_expand|flags_fill;
	wk->func = gui_internal_call_linked_on_finish;
	wk->c=wm->c;
	gui_internal_widget_append(we, wnext=gui_internal_image_new(this, image_new_xs(this, "gui_active")));
	wnext->state |= STATE_SENSITIVE;
	wnext->func = gui_internal_cmd_add_bookmark_clicked;
	wnext->data=wk;
	wk->data=wnext;
	wl=gui_internal_box_new(this, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(w, wl);
	if (this->keyboard)
		gui_internal_widget_append(w, gui_internal_keyboard(this,2));
	gui_internal_menu_render(this);
}

static void
gui_internal_cmd_add_bookmark_folder2(struct gui_priv *this, struct widget *wm, void *data)
{
	struct widget *w,*wb,*wk,*wl,*we,*wnext;
	char *name=data;
	wb=gui_internal_menu(this,_("Add Bookmark folder"));
	w=gui_internal_box_new(this, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(wb, w);
	we=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|flags_fill);
	gui_internal_widget_append(w, we);
	gui_internal_widget_append(we, wk=gui_internal_label_new(this, name));
	wk->state |= STATE_EDIT|STATE_EDITABLE|STATE_CLEAR;
	wk->background=this->background;
	wk->flags |= flags_expand|flags_fill;
	wk->func = gui_internal_call_linked_on_finish;
	wk->c=wm->c;
	gui_internal_widget_append(we, wnext=gui_internal_image_new(this, image_new_xs(this, "gui_active")));
	wnext->state |= STATE_SENSITIVE;
	wnext->func = gui_internal_cmd_add_bookmark_folder_clicked;
	wnext->data=wk;
	wk->data=wnext;
	wl=gui_internal_box_new(this, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(w, wl);
	if (this->keyboard)
		gui_internal_widget_append(w, gui_internal_keyboard(this,2));
	gui_internal_menu_render(this);
}

static void
gui_internal_cmd_rename_bookmark(struct gui_priv *this, struct widget *wm, void *data)
{
	struct widget *w,*wb,*wk,*wl,*we,*wnext;
	char *name=wm->text;
	wb=gui_internal_menu(this,_("Rename"));
	w=gui_internal_box_new(this, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(wb, w);
	we=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|flags_fill);
	gui_internal_widget_append(w, we);
	gui_internal_widget_append(we, wk=gui_internal_label_new(this, name));
	wk->state |= STATE_EDIT|STATE_EDITABLE|STATE_CLEAR;
	wk->background=this->background;
	wk->flags |= flags_expand|flags_fill;
	wk->func = gui_internal_call_linked_on_finish;
	wk->c=wm->c;
	wk->name=g_strdup(name);
	gui_internal_widget_append(we, wnext=gui_internal_image_new(this, image_new_xs(this, "gui_active")));
	wnext->state |= STATE_SENSITIVE;
	wnext->func = gui_internal_cmd_rename_bookmark_clicked;
	wnext->data=wk;
	wk->data=wnext;
	wl=gui_internal_box_new(this, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(w, wl);
	if (this->keyboard)
		gui_internal_widget_append(w, gui_internal_keyboard(this,2));
	gui_internal_menu_render(this);
}

static void
gui_internal_cmd_cut_bookmark(struct gui_priv *this, struct widget *wm, void *data)
{
	struct attr mattr;
	GList *l;
	navit_get_attr(this->nav, attr_bookmarks, &mattr, NULL);
	bookmarks_cut_bookmark(mattr.u.bookmarks,wm->text);
	l=g_list_previous(g_list_previous(g_list_last(this->root.children)));
	gui_internal_prune_menu(this, l->data);
}

static void
gui_internal_cmd_copy_bookmark(struct gui_priv *this, struct widget *wm, void *data)
{
	struct attr mattr;
	GList *l;
	navit_get_attr(this->nav, attr_bookmarks, &mattr, NULL);
	bookmarks_copy_bookmark(mattr.u.bookmarks,wm->text);
	l=g_list_previous(g_list_previous(g_list_last(this->root.children)));
	gui_internal_prune_menu(this, l->data);
}

static void
gui_internal_cmd_paste_bookmark(struct gui_priv *this, struct widget *wm, void *data)
{
	struct attr mattr;
	GList *l;
	navit_get_attr(this->nav, attr_bookmarks, &mattr, NULL);
	bookmarks_paste_bookmark(mattr.u.bookmarks);
	l=g_list_previous(g_list_last(this->root.children));
	gui_internal_prune_menu(this, l->data);
}

static void
gui_internal_cmd_delete_bookmark(struct gui_priv *this, struct widget *wm, void *data)
{
	struct attr mattr;
	GList *l;
	navit_get_attr(this->nav, attr_bookmarks, &mattr, NULL);
	bookmarks_delete_bookmark(mattr.u.bookmarks,wm->text);
	l=g_list_previous(g_list_previous(g_list_last(this->root.children)));
	gui_internal_prune_menu(this, l->data);
}

static void
gui_internal_cmd_delete_bookmark_folder(struct gui_priv *this, struct widget *wm, void *data)
{
	struct attr mattr;
	GList *l;
	navit_get_attr(this->nav, attr_bookmarks, &mattr, NULL);
	bookmarks_move_up(mattr.u.bookmarks);
	bookmarks_delete_bookmark(mattr.u.bookmarks,wm->prefix);
	l=g_list_first(this->root.children);
	gui_internal_prune_menu(this, l->data);
}

static void
gui_internal_cmd_load_bookmarks_as_waypoints(struct gui_priv *this, struct widget *wm, void *data)
{
	struct attr mattr;

	if(navit_get_attr(this->nav, attr_bookmarks, &mattr, NULL) ) {
		struct attr attr;
		struct item *item;
		struct coord c;
		struct pcoord *pc;
		enum projection pro=bookmarks_get_projection(mattr.u.bookmarks);
		int i, bm_count;

		navit_set_destination(this->nav, NULL, NULL, 0);

		bm_count=bookmarks_get_bookmark_count(mattr.u.bookmarks);
		pc=g_alloca(bm_count*sizeof(struct pcoord));
		bookmarks_item_rewind(mattr.u.bookmarks);
		i=0;
		while ((item=bookmarks_get_item(mattr.u.bookmarks))) {
			if (!item_attr_get(item, attr_label, &attr)) 
				continue;
			if (item->type == type_bookmark) {
				if (item_coord_get(item, &c, 1)) {
					pc[i].x=c.x;
					pc[i].y=c.y;
					pc[i].pro=pro;
					i++;
				}
			}
		}
		bm_count=i;

		if (bm_count>0){
			navit_set_destinations(this->nav, pc, bm_count, wm->prefix, 1);
			if (this->flags & 512) {
				struct attr follow;
				follow.type=attr_follow;
				follow.u.num=180;
				navit_set_attr(this->nav, &this->osd_configuration);
				navit_set_attr(this->nav, &follow);
				navit_zoom_to_route(this->nav, 0);
			}
		}
	}

	gui_internal_prune_menu(this, NULL);
}

static void
gui_internal_cmd_replace_bookmarks_from_waypoints(struct gui_priv *this, struct widget *wm, void *data)
{
	struct attr mattr;

	if(navit_get_attr(this->nav, attr_bookmarks, &mattr, NULL) ) {
		struct attr attr;
		char *desc=NULL;
		struct pcoord *pc;
		int i, bm_count;

		if (bookmarks_get_bookmark_count(mattr.u.bookmarks)>0){
			struct item *item;
			bookmarks_item_rewind(mattr.u.bookmarks);

			while ((item=bookmarks_get_item(mattr.u.bookmarks))) {

				if (!item_attr_get(item, attr_label, &attr)) 
					continue;

				if (item->type == type_bookmark) 
					bookmarks_delete_bookmark(mattr.u.bookmarks,  attr.u.str);

				bookmarks_move_down(mattr.u.bookmarks,wm->prefix);
			}
		}
		bookmarks_item_rewind(mattr.u.bookmarks);

		bm_count=navit_get_destination_count(this->nav);
		pc=g_alloca(bm_count*sizeof(struct pcoord));
		navit_get_destinations(this->nav, pc, bm_count);

		for (i=0; i<bm_count; i++){
			desc=g_strdup_printf("%s WP%d", navit_get_destination_description(this->nav, i), i+1);
			navit_get_attr(this->nav, attr_bookmarks, &attr, NULL);
			bookmarks_add_bookmark(attr.u.bookmarks, &pc[i], desc);
			bookmarks_move_down(mattr.u.bookmarks,wm->prefix);
			g_free(desc);
		}
	}

	gui_internal_prune_menu(this, NULL);
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

static void gui_internal_cmd_position(struct gui_priv *this, struct widget *wm, void *data);

struct selector {
	char *icon;
	char *name;
	enum item_type *types;
};
static enum item_type selectors_BankTypes[]={type_poi_bank,type_poi_bank, type_poi_atm,type_poi_atm, type_none};
static enum item_type selectors_FuelTypes[]={type_poi_fuel,type_poi_fuel,type_none};
static enum item_type selectors_HotelTypes[]={type_poi_hotel,type_poi_camp_rv,type_poi_camping,type_poi_camping,
    type_poi_resort,type_poi_resort,type_poi_motel,type_poi_hostel,type_none};
static enum item_type selectors_RestaurantTypes[]={type_poi_bar,type_poi_picnic,type_poi_burgerking,type_poi_fastfood,
    type_poi_restaurant,type_poi_restaurant,type_poi_cafe,type_poi_cafe,type_poi_pub,type_poi_pub,type_none};
static enum item_type selectors_ShoppingTypes[]={type_poi_mall,type_poi_mall,type_poi_shop_grocery,type_poi_shop_grocery,
    type_poi_shopping,type_poi_shopping,type_poi_shop_butcher,type_poi_shop_baker,type_poi_shop_fruit,
    type_poi_shop_fruit,type_poi_shop_beverages,type_poi_shop_beverages,type_none};
static enum item_type selectors_ServiceTypes[]={type_poi_marina,type_poi_marina,type_poi_hospital,type_poi_hospital,
    type_poi_public_utilities,type_poi_public_utilities,type_poi_police,type_poi_autoservice,type_poi_information,
    type_poi_information,type_poi_pharmacy,type_poi_pharmacy,type_poi_personal_service,type_poi_repair_service,
    type_poi_restroom,type_poi_restroom,type_none};
static enum item_type selectors_ParkingTypes[]={type_poi_car_parking,type_poi_car_parking,type_none};
static enum item_type selectors_LandFeaturesTypes[]={type_poi_land_feature,type_poi_rock,type_poi_dam,type_poi_dam,
    type_poi_peak,type_poi_peak,type_none};
static enum item_type selectors_OtherTypes[]={type_point_unspecified,type_poi_land_feature-1,type_poi_rock+1,type_poi_fuel-1,
    type_poi_marina+1,type_poi_shopping-1,type_poi_shopping+1,type_poi_car_parking-1,type_poi_car_parking+1,
    type_poi_bar-1,type_poi_bank+1,type_poi_dam-1,type_poi_dam+1,type_poi_information-1,type_poi_information+1,
    type_poi_mall-1,type_poi_mall+1,type_poi_personal_service-1,type_poi_pharmacy+1,type_poi_repair_service-1,
    type_poi_repair_service+1,type_poi_restaurant-1,type_poi_restaurant+1,type_poi_restroom-1,type_poi_restroom+1,
    type_poi_shop_grocery-1,type_poi_shop_grocery+1,type_poi_peak-1,type_poi_peak+1,type_poi_motel-1,type_poi_hostel+1,
    type_poi_shop_butcher-1,type_poi_shop_baker+1,type_poi_shop_fruit-1,type_poi_shop_fruit+1,type_poi_shop_beverages-1,
    type_poi_shop_beverages+1,type_poi_pub-1,type_poi_atm+1,type_line-1,type_none};
/*static enum item_type selectors_UnknownTypes[]={type_point_unkn,type_point_unkn,type_none};*/
struct selector selectors[]={
	{"bank","Bank",selectors_BankTypes},
	{"fuel","Fuel",selectors_FuelTypes},
	{"hotel","Hotel",selectors_HotelTypes},
	{"restaurant","Restaurant",selectors_RestaurantTypes},
	{"shopping","Shopping",selectors_ShoppingTypes},
	{"hospital","Service",selectors_ServiceTypes},
	{"parking","Parking",selectors_ParkingTypes},
	{"peak","Land Features",selectors_LandFeaturesTypes},
	{"unknown","Other",selectors_OtherTypes},
/*	{"unknown","Unknown",selectors_UnknownTypes},*/
};


/*
 *  Get a utf-8 string, return the same prepared for case insensetive search. Result shoud be g_free()d after use.
 */

static char *
removecase(char *s) 
{
	char *r;
	r=g_utf8_casefold(s,-1);
	return r;
}

/**
 * POI search/filtering parameters.
 *
 */

struct poi_param {

		/**
 		 * =1 if selnb is defined, 0 otherwize.
		 */
		unsigned char sel;

		/**
 		 * Index to struct selector selectors[], shows what type of POIs is defined.
		 */		
		unsigned char selnb;
		/**
 		 * Page number to display.
		 */		
		unsigned char pagenb;
		/**
 		 * Radius (number of 10-kilometer intervals) to search for POIs.
		 */		
		unsigned char dist;
		/**
 		 * Should filter phrase be compared to postal address of the POI.
 		 * =1 - address filter, =0 - name filter
		 */		
		unsigned char isAddressFilter;
		/**
 		 * Filter string, casefold()ed and divided into substrings at the spaces, which are replaced by ASCII 0*.
		 */		
		char *filterstr; 
		/**
 		 * list of pointers to individual substrings of filterstr.
		 */		
		GList *filter;
		/**
		 * Number of POIs in this list
		 */
		int count;
};


/**
 * @brief Free poi_param structure.
 *
 * @param p reference to the object to be freed.
 */
static void
gui_internal_poi_param_free(void *p) 
{
	if(((struct poi_param *)p)->filterstr)
	   g_free(((struct poi_param *)p)->filterstr);
	if(((struct poi_param *)p)->filter)
	   g_list_free(((struct poi_param *)p)->filter);
	g_free(p);
};

/**
 * @brief Clone poi_param structure.
 *
 * @param p reference to the object to be cloned.
 * @return  Cloned object reference.
 */
static struct poi_param *
gui_internal_poi_param_clone(struct poi_param *p) 
{
	struct poi_param *r=g_new(struct poi_param,1);
	GList *l=p->filter;
	memcpy(r,p,sizeof(struct poi_param));
	r->filter=NULL;
	r->filterstr=NULL;
	if(p->filterstr) {
		char *last=g_list_last(l)->data;
		int len=(last - p->filterstr) + strlen(last)+1;
		r->filterstr=g_memdup(p->filterstr,len);
	}
	while(l) {
		r->filter=g_list_append(r->filter, r->filterstr + ((char*)(l->data) - p->filterstr) );
		l=g_list_next(l);
	}
	return r;
};

/**
 * @brief Set POIs filter data in poi_param structure.
 * @param param poi_param structure with unset filter data.
 * @param text filter text.
 */
static void
gui_internal_poi_param_set_filter(struct poi_param *param, char *text) 
{
	char *s1, *s2;
	
	param->filterstr=removecase(text);
	s1=param->filterstr;
	do {
		s2=g_utf8_strchr(s1,-1,' ');
		if(s2)
			*s2++=0;
		param->filter=g_list_append(param->filter,s1);
		if(s2) {
			while(*s2==' ')
				s2++;
		}
		s1=s2;
	} while(s2 && *s2);
}


static void gui_internal_cmd_pois(struct gui_priv *this, struct widget *wm, void *data);
static void gui_internal_cmd_pois_filter(struct gui_priv *this, struct widget *wm, void *data);


static struct widget *
gui_internal_cmd_pois_selector(struct gui_priv *this, struct pcoord *c, int pagenb)
{
	struct widget *wl,*wb;
	int nitems,nrows;
	int i;
	//wl=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|flags_fill);
	wl=gui_internal_box_new(this, gravity_left_center|orientation_horizontal_vertical|flags_fill);
	wl->background=this->background;
	wl->w=this->root.w;
	wl->cols=this->root.w/this->icon_s;
	nitems=sizeof(selectors)/sizeof(struct selector);
	nrows=nitems/wl->cols + (nitems%wl->cols>0);
	wl->h=this->icon_l*nrows;
	for (i = 0 ; i < nitems ; i++) {
		struct poi_param *p=g_new0(struct poi_param,1);
		p->sel = 1;
		p->selnb = i;
		p->pagenb = pagenb;
		p->dist = 0;
		p->filter=NULL;
		p->filterstr=NULL;
		gui_internal_widget_append(wl, wb=gui_internal_button_new_with_callback(this, NULL,
			image_new_s(this, selectors[i].icon), gravity_left_center|orientation_vertical,
			gui_internal_cmd_pois, p));
		wb->c=*c;
		wb->data_free=gui_internal_poi_param_free;
		wb->bt=10;
	}

	gui_internal_widget_append(wl, wb=gui_internal_button_new_with_callback(this, NULL,
			image_new_s(this, "gui_search"), gravity_left_center|orientation_vertical,
			gui_internal_cmd_pois_filter, NULL));
	wb->c=*c;
	wb->bt=10;
	
	gui_internal_widget_pack(this,wl);
	return wl;
}

/**
 * @brief Get icon for given POI type.
 *
 * @param this pointer to gui context
 * @param type POI type
 * @return  Pointer to graphics_image object, or NULL if no picture available. 
 */
static struct graphics_image * gui_internal_poi_icon(struct gui_priv *this, enum item_type type)
{
	struct attr layout;
	GList *layer;
	navit_get_attr(this->nav, attr_layout, &layout, NULL);
	layer=layout.u.layout->layers;
	while(layer) {
		GList *itemgra=((struct layer *)layer->data)->itemgras;
		while(itemgra) {
			GList *types=((struct itemgra *)itemgra->data)->type;
			while(types) {
				if((long)types->data==type) {
					GList *element=((struct itemgra *)itemgra->data)->elements;
					while(element) {
						struct element * el=element->data;
						if(el->type==element_icon) {
							struct graphics_image *img;
							char *icon=g_strdup(el->u.icon.src);
							char *dot=g_strrstr(icon,".");
							dbg(0,"%s %s\n", item_to_name(type),icon);
							if(dot)
								*dot=0;
							img=image_new_xs(this,icon);
							g_free(icon);
							if(img) 
								return img;
						}
						element=g_list_next(element);
					}
				}
				types=g_list_next(types);	
			}
			itemgra=g_list_next(itemgra);
		}
		layer=g_list_next(layer);
	}
	return NULL;
}

/**
 * @brief Widget to display POI item.
 *
 * @param this pointer to gui context
 * @param center reference to the standing point where angle to be counted from
 * @param item POI item reference
 * @param c POI coordinates
 * @param dist precomputed distance to POI (or -1 if it's not to be displayed)
 * @param name POI name
 * @return  Pointer to new widget.
 */
static struct widget *
gui_internal_cmd_pois_item(struct gui_priv *this, struct coord *center, struct item *item, struct coord *c, int dist, char* name)
{
	char distbuf[32]="";
	char dirbuf[32]="";
	char *type;
	struct widget *wl;
	char *text;
	struct graphics_image *icon;

	if (dist > 10000)
		sprintf(distbuf,"%d ", dist/1000);
	else if (dist>0)
		sprintf(distbuf,"%d.%d ", dist/1000, (dist%1000)/100);
	if(c) {
		int len; 		
		get_direction(dirbuf, transform_get_angle_delta(center, c, 0), 1);
		len=strlen(dirbuf);
		dirbuf[len]=' ';
		dirbuf[len+1]=0;
	}
	
	type=item_to_name(item->type);

	icon=gui_internal_poi_icon(this,item->type);
	if(!icon) {
		icon=image_new_xs(this,"gui_inactive");
		text=g_strdup_printf("%s%s%s %s", distbuf, dirbuf, type, name);
	} else if(strlen(name)>0)
		text=g_strdup_printf("%s%s%s", distbuf, dirbuf, name);
	else 
		text=g_strdup_printf("%s%s%s", distbuf, dirbuf, type);
		
	wl=gui_internal_button_new_with_callback(this, text, icon, gravity_left_center|orientation_horizontal|flags_fill, NULL, NULL);
	wl->datai=dist;
	g_free(text);
	if (name[0]) {
		wl->name=g_strdup_printf("%s %s",type,name);
	} else {
		wl->name=g_strdup(type);
	}
	wl->func=gui_internal_cmd_position;
	wl->data=(void *)9;
	wl->item=*item;
	wl->state|= STATE_SENSITIVE;
	return wl;
}

/**
 * @brief Get string representation of item address suitable for doing search and for display in POI list.
 *
 * @param item reference to item.
 * @return  Pointer to string representation of address. To be g_free()d after use.
 */
char *
gui_internal_compose_item_address_string(struct item *item)
{
	char *s=g_strdup("");
	struct attr attr;
	if(item_attr_get(item, attr_house_number, &attr)) 
		s=g_strjoin(" ",s,attr.u.str,NULL);
	if(item_attr_get(item, attr_street_name, &attr)) 
		s=g_strjoin(" ",s,attr.u.str,NULL);
	if(item_attr_get(item, attr_street_name_systematic, &attr)) 
		s=g_strjoin(" ",s,attr.u.str,NULL);
	if(item_attr_get(item, attr_district_name, &attr)) 
		s=g_strjoin(" ",s,attr.u.str,NULL);
	if(item_attr_get(item, attr_town_name, &attr)) 
		s=g_strjoin(" ",s,attr.u.str,NULL);
	if(item_attr_get(item, attr_county_name, &attr)) 
		s=g_strjoin(" ",s,attr.u.str,NULL);
	if(item_attr_get(item, attr_country_name, &attr)) 
		s=g_strjoin(" ",s,attr.u.str,NULL);
	
	if(item_attr_get(item, attr_address, &attr)) 
		s=g_strjoin(" ",s,"|",attr.u.str,NULL);
	return s;
}

static int
gui_internal_cmd_pois_item_selected(struct poi_param *param, struct item *item)
{
	enum item_type *types;
	struct selector *sel = param->sel? &selectors[param->selnb]: NULL;
	enum item_type type=item->type;
	struct attr attr;
	int match=0;
	if (type >= type_line && param->filter==NULL)
		return 0;
	if (! sel || !sel->types) {
		match=1;
	} else {
		types=sel->types;
		while (*types != type_none) {
			if (item->type >= types[0] && item->type <= types[1]) {
				return 1;
			}
			types+=2;
		}
	}
	if (param->filter) {
		char *long_name, *s;
		GList *f;
		if (param->isAddressFilter) {
			s=gui_internal_compose_item_address_string(item);
		} else if (item_attr_get(item, attr_label, &attr)) {
			s=g_strdup_printf("%s %s", item_to_name(item->type), attr.u.str);
		} else {
			s=g_strdup(item_to_name(item->type));
		}
		long_name=removecase(s);
		g_free(s);
                item_attr_rewind(item);
                
		for(s=long_name,f=param->filter;f && s;f=g_list_next(f)) {
			s=strstr(s,f->data);
			if(!s) 
				break;
			s=g_utf8_strchr(s,-1,' ');
		}
		if(f)
			match=0;
		g_free(long_name);
	}
	return match;
}

struct item_data {
	int dist;
	char *label;
	struct item item;
	struct coord c;
};

/**
 * @brief Event handler for POIs list "more" element.
 *
 * @param this The graphics context.
 * @param wm called widget.
 * @param data event data.
 */
static void
gui_internal_cmd_pois_more(struct gui_priv *this, struct widget *wm, void *data) 
{
	struct widget *w=g_new0(struct widget,1);
	w->data=wm->data;
	w->c=wm->c;
	w->w=wm->w;
	wm->data_free=NULL;
	gui_internal_back(this, NULL, NULL);
	gui_internal_cmd_pois(this, w, w->data);
	free(w);
}


/**
 * @brief Event to apply POIs text filter.
 *
 * @param this The graphics context.
 * @param wm called widget.
 * @param data event data (pointer to editor widget containg filter text).
 */
static void
gui_internal_cmd_pois_filter_do(struct gui_priv *this, struct widget *wm, void *data) 
{
	struct widget *w=data;
	struct poi_param *param;
	
	if(!w->text)
		return;
	
	if(w->data) {
		param=gui_internal_poi_param_clone(w->data);
		param->pagenb=0;
	} else {
		param=g_new0(struct poi_param,1);
	}
	param->isAddressFilter=strcmp(wm->name,"AddressFilter")==0;

	gui_internal_poi_param_set_filter(param, w->text);

	gui_internal_cmd_pois(this,w,param);
	gui_internal_poi_param_free(param);
}

/**
 * @brief POIs filter dialog.
 * Event to handle '\r' '\n' keys pressed.
 * 
 */

static void
gui_internal_cmd_pois_filter_changed(struct gui_priv *this, struct widget *wm, void *data)
{
	if (wm->text && wm->reason==gui_internal_reason_keypress_finish) {
		gui_internal_cmd_pois_filter_do(this, wm, wm);
	}
}


/**
 * @brief POIs filter dialog.
 *
 * @param this The graphics context.
 * @param wm called widget.
 * @param data event data.
 */
static void
gui_internal_cmd_pois_filter(struct gui_priv *this, struct widget *wm, void *data) 
{
	struct widget *wb, *w, *wr, *wk, *we;
	int keyboard_mode=2;
	wb=gui_internal_menu(this,"Filter");
	w=gui_internal_box_new(this, gravity_center|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(wb, w);
	wr=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
        gui_internal_widget_append(w, wr);
        we=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|flags_fill);
        gui_internal_widget_append(wr, we);

	gui_internal_widget_append(we, wk=gui_internal_label_new(this, NULL));
	wk->state |= STATE_EDIT|STATE_EDITABLE;
	wk->func=gui_internal_cmd_pois_filter_changed;
	wk->background=this->background;
	wk->flags |= flags_expand|flags_fill;
	wk->name=g_strdup("POIsFilter");
	wk->c=wm->c;
	gui_internal_widget_append(we, wb=gui_internal_image_new(this, image_new_xs(this, "gui_active")));
	wb->state |= STATE_SENSITIVE;
	wb->func = gui_internal_cmd_pois_filter_do;
	wb->name=g_strdup("NameFilter");
	wb->data=wk;
	gui_internal_widget_append(we, wb=gui_internal_image_new(this, image_new_xs(this, "post")));
	wb->state |= STATE_SENSITIVE;
	wb->name=g_strdup("AddressFilter");
	wb->func = gui_internal_cmd_pois_filter_do;
	wb->data=wk;
	
	if (this->keyboard)
		gui_internal_widget_append(w, gui_internal_keyboard(this,keyboard_mode));
	gui_internal_menu_render(this);


}

/**
 * @brief Do POI search specified by poi_param and display POIs found
 *
 * @param this The graphics context.
 * @param wm called widget.
 * @param data event data, reference to poi_param or NULL.
 */
static void
gui_internal_cmd_pois(struct gui_priv *this, struct widget *wm, void *data)
{
	struct map_selection *sel,*selm;
	struct coord c,center;
	struct mapset_handle *h;
	struct map *m;
	struct map_rect *mr;
	struct item *item;
	struct widget *wi,*w,*w2,*wb, *wtable, *row;
	enum projection pro=wm->c.pro;
	struct poi_param *param;
	int param_free=0;
	int idist,dist;
	struct selector *isel;
	int pagenb;
	int prevdist;
	// Starting value and increment of count of items to be extracted
	const int pagesize = 50; 
	int maxitem, it = 0, i;
	struct item_data *items;
	struct fibheap* fh = fh_makekeyheap();
	int cnt = 0;
	struct table_data *td;
	struct widget *wl,*wt;
	char buffer[32];
	struct poi_param *paramnew;

	if(data) {
	  param = data;
	} else {
	  param = g_new0(struct poi_param,1);
	  param_free=1;
	}
	
	dist=10000*(param->dist+1);
	isel = param->sel? &selectors[param->selnb]: NULL;
	pagenb = param->pagenb;
	prevdist=param->dist*10000;
	maxitem = pagesize*(pagenb+1);
	items= g_new0( struct item_data, maxitem);
	
	
	dbg(0, "Params: sel = %i, selnb = %i, pagenb = %i, dist = %i, filterstr = %s, isAddressFilter= %d\n",
		param->sel, param->selnb, param->pagenb, param->dist, param->filterstr, param->isAddressFilter);

	wb=gui_internal_menu(this, isel ? isel->name : _("POIs"));
	w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(wb, w);
	if (!isel && !param->filter)
		gui_internal_widget_append(w, gui_internal_cmd_pois_selector(this,&wm->c,pagenb));
	w2=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(w, w2);

	sel=map_selection_rect_new(&wm->c,dist*transform_scale(abs(wm->c.y)+dist*1.5),18);
	center.x=wm->c.x;
	center.y=wm->c.y;
	h=mapset_open(navit_get_mapset(this->nav));
        while ((m=mapset_next(h, 1))) {
		selm=map_selection_dup_pro(sel, pro, map_projection(m));
		mr=map_rect_new(m, selm);
		dbg(2,"mr=%p\n", mr);
		if (mr) {
			while ((item=map_rect_get_item(mr))) {
				if (gui_internal_cmd_pois_item_selected(param, item) &&
				    item_coord_get_pro(item, &c, 1, pro) &&
				    coord_rect_contains(&sel->u.c_rect, &c)  &&
				    (idist=transform_distance(pro, &center, &c)) < dist) {
					struct item_data *data;
					struct attr attr;
					char *label;
					
					if (item->type==type_house_number) {
						label=gui_internal_compose_item_address_string(item);
					} else if (item_attr_get(item, attr_label, &attr)) {
						label=g_strdup(attr.u.str);
						// Buildings which label is equal to addr:housenumber value
						// are duplicated by item_house_number. Don't include such 
						// buildings into the list. This is true for OSM maps created with 
						// maptool patched with #859 latest patch.
						// FIXME: For non-OSM maps, we probably would better don't skip these items.
						if(item->type==type_poly_building && item_attr_get(item, attr_house_number, &attr) ) {
							if(strcmp(label,attr.u.str)==0) {
								g_free(label);
								continue;
							}
						}

					} else {
						label=g_strdup("");
					}
					
					if(it>=maxitem) {
						data = fh_extractmin(fh);
						g_free(data->label);
						data->label=NULL;
					} else {
						data = &items[it++];
					}
					data->label=label;
					data->item = *item;
					data->c = c;
					data->dist = idist;
					// Key expression is a workaround to fight
					// probable heap collisions when two objects
					// are at the same distance. But it destroys
					// right order of POIs 2048 km away from cener
					// and if table grows more than 1024 rows.
					fh_insertkey(fh, -((idist<<10) + cnt++), data);
					if (it == maxitem)
						dist = (-fh_minkey(fh))>>10;
				}
			}
			map_rect_destroy(mr);
		}
		map_selection_destroy(selm);
	}
	map_selection_destroy(sel);
	mapset_close(h);
	
	wtable = gui_internal_widget_table_new(this,gravity_left_top | flags_fill | flags_expand |orientation_vertical,1);
	td=wtable->data;

	gui_internal_widget_append(w2,wtable);

	// Move items from heap to the table
	for(i=0;;i++) 
	{
		int key = fh_minkey(fh);
		struct item_data *data = fh_extractmin(fh);
		if (data == NULL)
		{
			dbg(2, "Empty heap: maxitem = %i, it = %i, dist = %i\n", maxitem, it, dist);
			break;
		}
		dbg(2, "dist1: %i, dist2: %i\n", data->dist, (-key)>>10);
		if(i==(it-pagesize*pagenb) && data->dist>prevdist)
			prevdist=data->dist;
		wi=gui_internal_cmd_pois_item(this, &center, &data->item, &data->c, data->dist, data->label);
		wi->c.x=data->c.x;
		wi->c.y=data->c.y;
		wi->c.pro=pro;
		wi->background=this->background;
		row = gui_internal_widget_table_row_new(this,
							  gravity_left
							  | flags_fill
							  | orientation_horizontal);
		gui_internal_widget_append(row,wi);
		row->datai=data->dist;
		gui_internal_widget_prepend(wtable,row);
		free(data->label);
	}

	fh_deleteheap(fh);
	free(items);

	// Add an entry for more POI
	row = gui_internal_widget_table_row_new(this,
						  gravity_left
						  | flags_fill
						  | orientation_horizontal);
	row->datai=100000000; // Really far away for Earth, but won't work for bigger planets.
	gui_internal_widget_append(wtable,row);
	wl=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|flags_fill);
	gui_internal_widget_append(row,wl);
	if (it == maxitem) {
		paramnew=gui_internal_poi_param_clone(param);
		paramnew->pagenb++;
		paramnew->count=it;
		snprintf(buffer, sizeof(buffer), "Get more (up to %d items)...", (paramnew->pagenb+1)*pagesize);
		wt=gui_internal_label_new(this, buffer);
		gui_internal_widget_append(wl, wt);
		wt->func=gui_internal_cmd_pois_more;
		wt->data=paramnew;
		wt->data_free=gui_internal_poi_param_free;
		wt->state |= STATE_SENSITIVE;
		wt->c = wm->c;
	} else {
		static int dist[]={1,5,10,0};
		wt=gui_internal_label_new(this, "Set distance to");
		gui_internal_widget_append(wl, wt);
		for(i=0;dist[i];i++) {
			paramnew=gui_internal_poi_param_clone(param);
			paramnew->dist+=dist[i];
			paramnew->count=it;
			snprintf(buffer, sizeof(buffer), " %i ", 10*(paramnew->dist+1));
			wt=gui_internal_label_new(this, buffer);
			gui_internal_widget_append(wl, wt);
			wt->func=gui_internal_cmd_pois_more;
			wt->data=paramnew;
			wt->data_free=gui_internal_poi_param_free;
			wt->state |= STATE_SENSITIVE;
			wt->c = wm->c;
		}
		wt=gui_internal_label_new(this, "km.");
		gui_internal_widget_append(wl, wt);

	}
	// Rendering now is needed to have table_data->bottomrow filled in.
	gui_internal_menu_render(this);
	td=wtable->data;
	if(td->bottom_row!=NULL)
	{
#if 0
		while(((struct widget*)td->bottom_row->data)->datai<=prevdist
				&& (td->next_button->state & STATE_SENSITIVE))
		{
			gui_internal_table_button_next(this, td->next_button, NULL);
		}
#else
		int firstrow=g_list_index(wtable->children, td->top_row->data);
		while(firstrow>=0) {
			int currow=g_list_index(wtable->children, td->bottom_row->data) - firstrow;
			if(currow<0) {
				dbg(0,"Can't find bottom row in children list. Stop paging.\n");
				break;
			}
			if(currow>=param->count)
				break;
			if(!(td->next_button->state & STATE_SENSITIVE)) {
				dbg(0,"Reached last page but item %i not found. Stop paging.\n",param->count);
				break;
			}
			gui_internal_table_button_next(this, td->next_button, NULL);
		}
#endif
	}
	gui_internal_menu_render(this);
        if(param_free)
        	g_free(param);
}

static void
gui_internal_cmd_view_on_map(struct gui_priv *this, struct widget *wm, void *data)
{
	if (wm->item.type != type_none) {
		enum item_type type;
		if (wm->item.type < type_line)
	           	type=type_selected_point;
		else if (wm->item.type < type_area)
	            	type=type_selected_point;
		else
			type=type_selected_area;
		graphics_clear_selection(this->gra, NULL);
		graphics_add_selection(this->gra, &wm->item, type, NULL);
	}
	navit_set_center(this->nav, &wm->c, 1);
	gui_internal_prune_menu(this, NULL);
}


static void
gui_internal_cmd_view_attribute_details(struct gui_priv *this, struct widget *wm, void *data)
{
	struct widget *w,*wb;
	struct map_rect *mr;
	struct item *item;
	struct attr attr;
	char *text,*url;
	int i;

	text=g_strdup_printf("Attribute %s",wm->name);
	wb=gui_internal_menu(this, text);
	g_free(text);
	w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(wb, w);
	mr=map_rect_new(wm->item.map, NULL);
	item = map_rect_get_item_byid(mr, wm->item.id_hi, wm->item.id_lo);
	for (i = 0 ; i < wm->datai ; i++) {
		item_attr_get(item, attr_any, &attr);
	}
	if (item_attr_get(item, attr_any, &attr)) {
		url=NULL;
		switch (attr.type) {
		case attr_osm_nodeid:
			url=g_strdup_printf("http://www.openstreetmap.org/browse/node/"LONGLONG_FMT"\n",*attr.u.num64);
			break;
		case attr_osm_wayid:
			url=g_strdup_printf("http://www.openstreetmap.org/browse/way/"LONGLONG_FMT"\n",*attr.u.num64);
			break;
		case attr_osm_relationid:
			url=g_strdup_printf("http://www.openstreetmap.org/browse/relation/"LONGLONG_FMT"\n",*attr.u.num64);
			break;
		default:
			break;
		}
		if (url) {
			gui_internal_widget_append(w,
					wb=gui_internal_button_new_with_callback(this, _("View in Browser"),
						image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
						gui_internal_cmd_view_in_browser, NULL));
			wb->name=url;
		}
	}
	map_rect_destroy(mr);
	gui_internal_menu_render(this);
}

static void
gui_internal_cmd_view_attributes(struct gui_priv *this, struct widget *wm, void *data)
{
	struct widget *w,*wb;
	struct map_rect *mr;
	struct item *item;
	struct attr attr;
	char *text;
	int count=0;

	dbg(0,"item=%p 0x%x 0x%x\n", wm->item.map,wm->item.id_hi, wm->item.id_lo);
	wb=gui_internal_menu(this, "Attributes");
	w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(wb, w);
	mr=map_rect_new(wm->item.map, NULL);
	item = map_rect_get_item_byid(mr, wm->item.id_hi, wm->item.id_lo);
	dbg(0,"item=%p\n", item);
	if (item) {
		text=g_strdup_printf("%s:%s", _("Item type"), item_to_name(item->type));
		gui_internal_widget_append(w,
		wb=gui_internal_button_new(this, text,
			NULL, gravity_left_center|orientation_horizontal|flags_fill));
		wb->name=g_strdup(text);
		wb->item=wm->item;
		g_free(text);
		while(item_attr_get(item, attr_any, &attr)) {
			char *attrtxt;
			text=g_strdup_printf("%s:%s", attr_to_name(attr.type), attrtxt=attr_to_text(&attr, wm->item.map, 1));
			g_free(attrtxt);
			gui_internal_widget_append(w,
			wb=gui_internal_button_new_with_callback(this, text,
				NULL, gravity_left_center|orientation_horizontal|flags_fill,
				gui_internal_cmd_view_attribute_details, NULL));
			wb->name=g_strdup(text);
			wb->item=wm->item;
			wb->datai=count++;
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

	if (!wm->name) {
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
	} else {
		cmd=g_strdup_printf("navit-browser.sh '%s' &",wm->name);
	}
	if (cmd) {
#ifdef HAVE_SYSTEM
		system(cmd);
#else
		dbg(0,"calling external cmd '%s' is not supported\n",cmd);
#endif
		g_free(cmd);
	}
}


/*
 * @brief Event to transfer search results to a map.
 *
 * @param this The graphics context.
 * @param wm called widget.
 * @param data event data (pointer to the table widget containing results, or NULL if results map is only have to be cleaned).
 */
static void
gui_internal_cmd_results_to_map(struct gui_priv *this, struct widget *wm, void *data)
{
	struct widget *w;
	struct mapset *ms;	
	struct map *map;
	struct map_selection sel;
	struct map_rect *mr;
	struct item *item;
	GList *l;
	struct coord_rect r;
	struct attr a;
	int count;
	
	ms=navit_get_mapset(this->nav);

	if(!ms)
		return;

	map=mapset_get_map_by_name(ms, "search_results");
	if(!map) {
		struct attr *attrs[10], attrmap;
		enum attr_type types[]={attr_position_longitude,attr_position_latitude,attr_label,attr_none};
		int i;
		
		attrs[0]=g_new0(struct attr,1);
		attrs[0]->type=attr_type;
		attrs[0]->u.str="csv";

		attrs[1]=g_new0(struct attr,1);
		attrs[1]->type=attr_name;
		attrs[1]->u.str="search_results";

		attrs[2]=g_new0(struct attr,1);
		attrs[2]->type=attr_charset;
		attrs[2]->u.str="utf-8";
		
		attrs[3]=g_new0(struct attr,1);
		attrs[3]->type=attr_item_type;
		attrs[3]->u.num=type_found_item;

		attrs[4]=g_new0(struct attr,1);
		attrs[4]->type=attr_attr_types;
		attrs[4]->u.attr_types=types;
		attrs[5]=NULL;
		
		attrmap.type=attr_map;
		map=attrmap.u.map=map_new(NULL,attrs);
		if(map)
			mapset_add_attr(ms,&attrmap);

		for(i=0;attrs[i];i++)
			g_free(attrs[i]);

	}

	if(!map)
		return;

	sel.next=NULL;
	sel.order=18;
	sel.range.min=type_none;
	sel.range.max=type_tec_common;
	

	mr = map_rect_new(map, NULL);

	if(!mr)
		return;
		
	/* Clean the map */
	while((item = map_rect_get_item(mr))!=NULL) {
		item_type_set(item,type_none);
	}

	this->results_map_population=0;
	 
	/* Find the table to pupulate the map */
	for(w=data; w && w->type!=widget_table;w=w->parent);
	
	if(!w) {
		map_rect_destroy(mr);
		dbg(1,"Can't find the results table - only map clean up is done.\n");
		return;
	}

	/* Populate the map with search results*/
	for(l=w->children, count=0;l;l=g_list_next(l)) {
		struct widget *wr=l->data;
		if(wr->type==widget_table_row) {
			struct widget *wi=wr->children->data;
			struct item* it;
			if(wi->name==NULL)
				continue;
			dbg(0,"%s\n",wi->name);
			it=map_rect_create_item(mr,type_found_item);
			if(it) {
				struct coord c;
				struct attr a;
				c.x=wi->c.x;
				c.y=wi->c.y;
				item_coord_set(it, &c, 1, change_mode_modify);
				a.type=attr_label;
				a.u.str=wi->name;
				item_attr_set(it, &a, change_mode_modify);
				if(!count++)
					r.lu=r.rl=c;
				else
					coord_rect_extend(&r,&c);
			}
		}
	}
	map_rect_destroy(mr);
	if(!count)
		return;
	a.type=attr_orientation;
	a.u.num=0;
	navit_set_attr(this->nav,&a);
	navit_zoom_to_rect(this->nav,&r);
	gui_internal_prune_menu(this, NULL);
	this->results_map_population=count;
}

/*
 * @brief Event to remove search results from the a map.
 *
 * @param this The graphics context.
 * @param wm called widget.
 * @param data event data
 */
static void
gui_internal_cmd_results_map_clean(struct gui_priv *this, struct widget *wm, void *data)
{
	gui_internal_cmd_results_to_map(this,wm,NULL);
	gui_internal_prune_menu(this, NULL);
	navit_draw(this->nav);
}


/* meaning of the bits in "flags":
 * 1: "Streets"
 * 2: "House numbers"
 * 4: "View in Browser", "View Attributes"
 * 8: "Set as dest."
 * 16: "Set as pos."
 * 32: "Add as bookm."
 * 64: "POIs"
 * 128: "View on Map"
 * 256: POIs around this point, "Drop search results from the map"
 * 512: "Cut/Copy... bookmark"
 * 1024: "Jump to attributes of top item within this->radius pixels of this point (implies flags|=256)"
 * 2048: "Show search results on the map"
 * TODO define constants for these values
 */
static void
gui_internal_cmd_position_do(struct gui_priv *this, struct pcoord *pc_in, struct coord_geo *g_in, struct widget *wm, char *name, int flags)
{
	struct widget *wb,*w,*wtable,*row,*wc,*wbc,*wclosest=NULL;
	struct coord_geo g;
	struct pcoord pc;
	struct coord c;
	char *coord;

	if (pc_in) {
		pc=*pc_in;
		c.x=pc.x;
		c.y=pc.y;
		dbg(0,"x=0x%x y=0x%x\n", c.x, c.y);
		transform_to_geo(pc.pro, &c, &g);
	} else if (g_in) {
		struct attr attr;
		if (!navit_get_attr(this->nav, attr_projection, &attr, NULL))
			return;
		g=*g_in;
		pc.pro=attr.u.projection;
		transform_from_geo(pc.pro, &g, &c);
		pc.x=c.x;
		pc.y=c.y;
	} else
		return;

	wb=gui_internal_menu(this, name);
	w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(wb, w);
	coord=coordinates(&pc, ' ');
	gui_internal_widget_append(w, gui_internal_label_new(this, coord));
	g_free(coord);
	wtable = gui_internal_widget_table_new(this,gravity_left_top | flags_fill | flags_expand |orientation_vertical,1);
	gui_internal_widget_append(w,wtable);
	
	if ((flags & 1) && wm) {
		gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,gravity_left|orientation_horizontal|flags_fill));
		gui_internal_widget_append(row,
			wc=gui_internal_button_new_with_callback(this, _("Streets"),
				image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
				gui_internal_search_street_in_town, wm));
		wc->item=wm->item;
		wc->selection_id=wm->selection_id;
	}
	if ((flags & 2) && wm) {
		gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,gravity_left|orientation_horizontal|flags_fill));
		gui_internal_widget_append(row,
			wc=gui_internal_button_new_with_callback(this, _("House numbers"),
				image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
				gui_internal_search_house_number_in_street, wm));
		wc->item=wm->item;
		wc->selection_id=wm->selection_id;
	}
	if ((flags & 4) && wm) {
		struct map_rect *mr;
		struct item *item;
		struct attr attr;
		mr=map_rect_new(wm->item.map, NULL);
		item = map_rect_get_item_byid(mr, wm->item.id_hi, wm->item.id_lo);
		if (item) {
			if (item_attr_get(item, attr_description, &attr))
				gui_internal_widget_append(w, gui_internal_label_new(this, attr.u.str));
			if (item_attr_get(item, attr_url_local, &attr)) {
				gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,gravity_left|orientation_horizontal|flags_fill));
				gui_internal_widget_append(row,
					wb=gui_internal_button_new_with_callback(this, _("View in Browser"),
						image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
						gui_internal_cmd_view_in_browser, NULL));
				wb->item=wm->item;
			}
			gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,gravity_left|orientation_horizontal|flags_fill));
			gui_internal_widget_append(row,
				wb=gui_internal_button_new_with_callback(this, _("View Attributes"),
					image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
					gui_internal_cmd_view_attributes, NULL));
			wb->item=wm->item;
		}
		map_rect_destroy(mr);
	}
	if (flags & 8) {
		gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,gravity_left|orientation_horizontal|flags_fill));
		gui_internal_widget_append(row,
			wbc=gui_internal_button_new_with_callback(this, _("Set as destination"),
				image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
				gui_internal_cmd_set_destination, g_strdup(name)));
		wbc->data_free=g_free_func;
		wbc->c=pc;
	}
	if (flags & 16) {
		const char *text;
		struct attr vehicle, source;
		int deactivate=0;
		if (navit_get_attr(this->nav, attr_vehicle, &vehicle, NULL) && vehicle.u.vehicle && 
				!(vehicle_get_attr(vehicle.u.vehicle, attr_source, &source, NULL) && source.u.str && !strcmp("demo://",source.u.str))) 
			deactivate=1;

		text=deactivate? _("Set as position (and deactivate vehicle)") : _("Set as position");

		gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,gravity_left|orientation_horizontal|flags_fill));
		gui_internal_widget_append(row,
			wbc=gui_internal_button_new_with_callback(this, text,
			image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
			gui_internal_cmd_set_position, (void*)deactivate));
		wbc->c=pc;
	}
	if (flags & 32) {
		gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,gravity_left|orientation_horizontal|flags_fill));
		gui_internal_widget_append(row,
			wbc=gui_internal_button_new_with_callback(this, _("Add as bookmark"),
			image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
			gui_internal_cmd_add_bookmark2, g_strdup(name)));
		wbc->data_free=g_free_func;
		wbc->c=pc;
	}
#ifndef _MSC_VER
//POIs are not operational under MSVC yet
	if (flags & 64) {
		gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,gravity_left|orientation_horizontal|flags_fill));
		gui_internal_widget_append(row,
			wbc=gui_internal_button_new_with_callback(this, _("POIs"),
				image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
				gui_internal_cmd_pois, NULL));
		wbc->c=pc;
	}
#endif /* _MSC_VER */
#if 0
	gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,gravity_left|orientation_horizontal|flags_fill));
	gui_internal_widget_append(row,
		gui_internal_button_new(this, "Add to tour",
			image_new_o(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill));
#endif
	if (flags & 128) {
		gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,gravity_left|orientation_horizontal|flags_fill));
		gui_internal_widget_append(row,
			wbc=gui_internal_button_new_with_callback(this, _("View on map"),
				image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
				gui_internal_cmd_view_on_map, NULL));
		wbc->c=pc;
		if ((flags & 4) && wm)
			wbc->item=wm->item;
		else
			wbc->item.type=type_none;
	}
	if(flags & 256 && this->results_map_population) {
		gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,gravity_left|orientation_horizontal|flags_fill));
		gui_internal_widget_append(row,
			wbc=gui_internal_button_new_with_callback(this, _("Remove search results from the map"),
				image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
				gui_internal_cmd_results_map_clean, NULL));
		wbc->data=wm;
	}
	if(flags & 2048) {
		gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,gravity_left|orientation_horizontal|flags_fill));
		gui_internal_widget_append(row,
			wbc=gui_internal_button_new_with_callback(this, _("Show results on the map"),
				image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
				gui_internal_cmd_results_to_map, NULL));
		wbc->data=wm;
	}
	if ((flags & 256) || (flags & 1024)) {
		struct displaylist_handle *dlh;
		struct displaylist *display;
		struct attr attr;
		struct point p;
		struct transformation *trans;
		
		char *text, *label;
		struct map_selection *sel;
		GList *l, *ll;
		
		c.x=pc.x;
		c.y=pc.y;

		trans=navit_get_trans(this->nav);
		transform(trans,pc.pro,&c,&p,1,0,0,0);
		display=navit_get_displaylist(this->nav);
		dlh=graphics_displaylist_open(display);
		sel=displaylist_get_selection(display);
		l=displaylist_get_clicked_list(display, &p, this->radius);
		for(ll=l;ll;ll=g_list_next(ll)) {
			struct displayitem *di;
			struct item *item;
			struct map_rect *mr;
			struct item *itemo;

			di=(struct displayitem*)ll->data;
			item=graphics_displayitem_get_item(di);
			
			mr=map_rect_new(item->map, sel);
			itemo=map_rect_get_item_byid(mr, item->id_hi, item->id_lo);
				
			if (item_attr_get(itemo, attr_label, &attr)) {
				label=map_convert_string(itemo->map, attr.u.str);
				text=g_strdup_printf("%s %s", item_to_name(item->type), label);
				map_convert_free(label);
			} else
				text=g_strdup_printf("%s", item_to_name(item->type));
			gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,gravity_left|orientation_horizontal|flags_fill));
			gui_internal_widget_append(row,	wc=gui_internal_cmd_pois_item(this, NULL, itemo, NULL, -1, text));
			wc->c=pc;
			g_free(wc->name);
			wc->name=g_strdup(text);
			wc->item=*itemo;
			g_free(text);
			map_rect_destroy(mr);
			if(!wclosest)
				wclosest=wc;

		}
		g_list_free(l);
		map_selection_destroy(sel);
	       	graphics_displaylist_close(dlh);
	}
	if (flags & 512) {
		gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,gravity_left|orientation_horizontal|flags_fill));
		gui_internal_widget_append(row,
			wbc=gui_internal_button_new_with_callback(this, _("Cut Bookmark"),
				image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
				gui_internal_cmd_cut_bookmark, NULL));
		wbc->text=g_strdup(wm->text);
		gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,gravity_left|orientation_horizontal|flags_fill));
		gui_internal_widget_append(row,
			wbc=gui_internal_button_new_with_callback(this, _("Copy Bookmark"),
				image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
				gui_internal_cmd_copy_bookmark, NULL));
		wbc->text=g_strdup(wm->text);
		gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,gravity_left|orientation_horizontal|flags_fill));
		gui_internal_widget_append(row,
			wbc=gui_internal_button_new_with_callback(this, _("Rename Bookmark"),
				image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
				gui_internal_cmd_rename_bookmark, NULL));
		wbc->text=g_strdup(wm->text);
		gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,gravity_left|orientation_horizontal|flags_fill));
		gui_internal_widget_append(row,
			wbc=gui_internal_button_new_with_callback(this, _("Paste Bookmark"),
				image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
				gui_internal_cmd_paste_bookmark, NULL));
		gui_internal_widget_append(wtable,row=gui_internal_widget_table_row_new(this,gravity_left|orientation_horizontal|flags_fill));
		gui_internal_widget_append(row,
			wbc=gui_internal_button_new_with_callback(this, _("Delete Bookmark"),
				image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
				gui_internal_cmd_delete_bookmark, NULL));
		wbc->text=g_strdup(wm->text);
	}
	
	gui_internal_menu_render(this);

	if((flags & 1024) && wclosest) 
			gui_internal_cmd_view_attributes(this,wclosest,wclosest->data);
}


/* wm->data:	0 Nothing special
		1 Map Point
		2 Item
		3 Town
		4 County
		5 Street
		6 House number
		7 Bookmark
		8 Former destination
		9 Item from the POI list
*/

static void
gui_internal_cmd_position(struct gui_priv *this, struct widget *wm, void *data)
{
	int flags;

	if(!data)
		data=wm->data;
		
	switch ((long) data) {
	case 0:
		flags=8|16|32|64|128|256;
		break;
	case 1:
		flags=8|16|32|64|256;
		break;
	case 2:
		flags=4|8|16|32|64|128;
		break;
	case 3:
		flags=1|8|16|32|64|128|2048;
		flags &= this->flags_town;
		break;
	case 4:
		gui_internal_search_town_in_country(this, wm);
		return;
	case 5:
		flags=2|8|16|32|64|128|2048;
		flags &= this->flags_street;
		break;
	case 6:
		flags=8|16|32|64|128|2048;
		flags &= this->flags_house_number;
		break;
	case 7:
		flags=8|16|64|128|512;
		break;
	case 8:
		flags=8|16|32|64|128;
		break;
	case 9:
		flags=4|8|16|32|64|128|2048;
		break;
	default:
		return;
	}
	switch (flags) {
	case 2:
		gui_internal_search_house_number_in_street(this, wm, NULL);
		return;
	case 8:
		gui_internal_cmd_set_destination(this, wm, NULL);
		return;
	}
	gui_internal_cmd_position_do(this, &wm->c, NULL, wm, wm->name ? wm->name : wm->text, flags);
}

static void
gui_internal_cmd2_position(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	char *name=_("Position");
	int flags=-1;

	dbg(1,"enter\n");
	if (!in || !in[0])
		return;
	if (!ATTR_IS_COORD_GEO(in[0]->type))
		return;
	if (in[1] && ATTR_IS_STRING(in[1]->type)) {
		name=in[1]->u.str;
		if (in[2] && ATTR_IS_INT(in[2]->type))
			flags=in[2]->u.num;
	}
	dbg(1,"flags=0x%x\n",flags);
	gui_internal_cmd_position_do(this, NULL, in[0]->u.coord_geo, NULL, name, flags);
}

static void
gui_internal_cmd2_pois(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	struct widget *w;
	struct poi_param *param;
	struct attr pro;
	struct coord c;

	dbg(1,"enter\n");
	if (!in || !in[0])
		return;
	if (!ATTR_IS_COORD_GEO(in[0]->type))
		return;
	if (!navit_get_attr(this->nav, attr_projection, &pro, NULL))
		return;
	w=g_new0(struct widget,1);
	param=g_new0(struct poi_param,1);
	if (in[1] && ATTR_IS_STRING(in[1]->type)) {
		gui_internal_poi_param_set_filter(param, in[1]->u.str);
		if (in[2] && ATTR_IS_INT(in[2]->type))
			param->isAddressFilter=in[2]->u.num;
	}
	
	transform_from_geo(pro.u.projection,in[0]->u.coord_geo,&c);
	w->c.x=c.x;
	w->c.y=c.y;
	w->c.pro=pro.u.projection;
	gui_internal_cmd_pois(this, w, param);
	g_free(w);
	gui_internal_poi_param_free(param);
}

static void
gui_internal_cmd2_refresh(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	char *href=g_strdup(this->href);
	gui_internal_html_load_href(this, href, 1);
	g_free(href);
}


/**
  * The "Bookmarks" section of the OSD
  * 
  */

static void
gui_internal_cmd_bookmarks(struct gui_priv *this, struct widget *wm, void *data)
{
	struct attr attr,mattr;
	struct item *item;
	char *label_full,*prefix=0;
	int plen=0,hassub,found=0;
	struct widget *wb,*w,*wbm;
	struct coord c;
	struct widget *tbl, *row;

	if (data)
		prefix=g_strdup(data);
	else {
		if (wm && wm->prefix)
			prefix=g_strdup(wm->prefix);
	}
	if ( prefix )
		plen=strlen(prefix);

	gui_internal_prune_menu_count(this, 1, 0);
	wb=gui_internal_menu(this, _("Bookmarks"));
	wb->background=this->background;
	w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	//w->spy=this->spacing*3;
	gui_internal_widget_append(wb, w);

	if(navit_get_attr(this->nav, attr_bookmarks, &mattr, NULL) ) {
		if (!plen) {
			bookmarks_move_root(mattr.u.bookmarks);
		} else {
			if (!strcmp(prefix,"..")) {
				bookmarks_move_up(mattr.u.bookmarks);
				g_free(prefix);
				prefix=g_strdup(bookmarks_item_cwd(mattr.u.bookmarks));
				if (prefix) {
					plen=strlen(prefix);
				} else {
					plen=0;
				}
			} else {
				bookmarks_move_down(mattr.u.bookmarks,prefix);
			}
			
			  // "Back" button, when inside a bookmark folder
			  
			if (plen) {
				wbm=gui_internal_button_new_with_callback(this, "..",
					image_new_xs(this, "gui_inactive"), gravity_left_center|orientation_horizontal|flags_fill,
						gui_internal_cmd_bookmarks, NULL);
						wbm->prefix=g_strdup("..");
				gui_internal_widget_append(w, wbm);

				// load bookmark folder as Waypoints, if any
				if (bookmarks_get_bookmark_count(mattr.u.bookmarks) > 0){
					wbm=gui_internal_button_new_with_callback(this, _("Bookmarks as Waypoints"),
							image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
							gui_internal_cmd_load_bookmarks_as_waypoints, NULL);
					wbm->prefix=g_strdup(prefix);
					gui_internal_widget_append(w, wbm);
				}

				// save Waypoints in bookmark folder, if route exists
				if (navit_get_destination_count(this->nav) > 0){
					if (bookmarks_get_bookmark_count(mattr.u.bookmarks)==0){
						wbm=gui_internal_button_new_with_callback(this, _("Save Waypoints"),
									image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
									gui_internal_cmd_replace_bookmarks_from_waypoints, NULL);
					}else{
						wbm=gui_internal_button_new_with_callback(this, _("Replace Waypoints"),
									image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
									gui_internal_cmd_replace_bookmarks_from_waypoints, NULL);
					}
					wbm->prefix=g_strdup(prefix);
					gui_internal_widget_append(w, wbm);
				}

				// delete empty folder
				if (bookmarks_get_bookmark_count(mattr.u.bookmarks)==0){
					gui_internal_widget_append(w,
							wbm=gui_internal_button_new_with_callback(this, _("Delete Folder"),
							image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
							gui_internal_cmd_delete_bookmark_folder, NULL));
					wbm->prefix=g_strdup(prefix);
				}

			}
		}
		
		// Adds the Bookmark folders
		wbm=gui_internal_button_new_with_callback(this, _("Add Bookmark folder"),
			    image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
				gui_internal_cmd_add_bookmark_folder2, NULL);
		gui_internal_widget_append(w, wbm);

		// Pastes the Bookmark
		wbm=gui_internal_button_new_with_callback(this, _("Paste bookmark"),
				image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
				gui_internal_cmd_paste_bookmark, NULL);
		gui_internal_widget_append(w, wbm);

		bookmarks_item_rewind(mattr.u.bookmarks);
				
		tbl=gui_internal_widget_table_new(this,gravity_left_top | flags_fill | flags_expand |orientation_vertical,1);
		gui_internal_widget_append(w,tbl);

		while ((item=bookmarks_get_item(mattr.u.bookmarks))) {
			if (!item_attr_get(item, attr_label, &attr)) continue;
			label_full=attr.u.str;
			dbg(0,"full_labled: %s\n",label_full);
			
			// hassub == 1 if the item type is a sub-folder
			if (item->type == type_bookmark_folder) {
				hassub=1;
			} else {
				hassub=0;
			}
			
			row=gui_internal_widget_table_row_new(this,gravity_left| flags_fill| orientation_horizontal);
			gui_internal_widget_append(tbl, row);
			wbm=gui_internal_button_new_with_callback(this, label_full,
				image_new_xs(this, hassub ? "gui_inactive" : "gui_active" ), gravity_left_center|orientation_horizontal|flags_fill,
					hassub ? gui_internal_cmd_bookmarks : gui_internal_cmd_position, NULL);

			gui_internal_widget_append(row,wbm);
			if (item_coord_get(item, &c, 1)) {
				wbm->c.x=c.x;
				wbm->c.y=c.y;
				wbm->c.pro=bookmarks_get_projection(mattr.u.bookmarks);
				wbm->name=g_strdup_printf(_("Bookmark %s"),label_full);
				wbm->text=g_strdup(label_full);
				if (!hassub) {
					wbm->data=(void*)7;//Mark us as a bookmark
				}
				wbm->prefix=g_strdup(label_full);
			} else {
				gui_internal_widget_destroy(this, row);
			}
		}
	}

	g_free(prefix);

	if (found)
		gui_internal_check_exit(this);
	else
		gui_internal_menu_render(this);
}

static void
gui_internal_cmd2_bookmarks(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	char *str=NULL;
	if (in && in[0] && ATTR_IS_STRING(in[0]->type)) {
		str=in[0]->u.str;
	}
	gui_internal_cmd_bookmarks(this, NULL, str);
}


static void
gui_internal_cmd_formerdests(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	struct widget *wb,*w,*wbm,*tbl=NULL;
	struct map *formerdests;
	struct map_rect *mr_formerdests;
	struct item *item;
	struct attr attr;
	char *label_full;
	enum projection projection;

	gui_internal_prune_menu_count(this, 1, 0);
	wb=gui_internal_menu(this, _("Former Destinations"));
	wb->background=this->background;

	w=gui_internal_box_new(this,
			gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	w->spy=this->spacing*2;
	gui_internal_widget_append(wb, w);

	formerdests=read_former_destinations_from_file();
	mr_formerdests=map_rect_new(formerdests, NULL);
	projection = map_projection(formerdests);
	while ((item=map_rect_get_item(mr_formerdests))) {
		struct coord c;
		struct widget *row;
		if (!item_attr_get(item, attr_label, &attr)) continue;
		if(!tbl) {
			tbl=gui_internal_widget_table_new(this,gravity_left_top | flags_fill | flags_expand | orientation_vertical,1);
			gui_internal_widget_append(w,tbl);
		}
		row=gui_internal_widget_table_row_new(this,gravity_left| flags_fill| orientation_vertical);
		gui_internal_widget_prepend(tbl, row);
		label_full=attr.u.str;
		wbm=gui_internal_button_new_with_callback(this, label_full,
				image_new_xs(this, "gui_active"),
				gravity_left_center|orientation_horizontal|flags_fill,
				gui_internal_cmd_position, NULL);
		gui_internal_widget_append(row,wbm);
		if (item_coord_get(item, &c, 1)) {
			wbm->c.x=c.x;
			wbm->c.y=c.y;
			wbm->c.pro=projection;
			wbm->name=g_strdup(label_full);
			wbm->text=g_strdup(label_full);
			wbm->data=(void*)8; //Mark us as a former destination 
			wbm->prefix=g_strdup(label_full);
		}
	}
	if (!tbl){
        	wbm=gui_internal_text_new(this, _("- No former destinations available -"), 
		    gravity_left_center|orientation_horizontal|flags_fill);
		gui_internal_widget_append(w, wbm);
	}
	gui_internal_menu_render(this);
	map_rect_destroy(mr_formerdests);
}

static void
gui_internal_keynav_highlight_next(struct gui_priv *this, int dx, int dy);

static void gui_internal_keypress_do(struct gui_priv *this, char *key)
{
	struct widget *wi,*menu,*search_list;
	int len=0;
	char *text=NULL;

	menu=g_list_last(this->root.children)->data;
	wi=gui_internal_find_widget(menu, NULL, STATE_EDIT);
	if (wi) {
                /* select first item of the searchlist */
                if (*key == NAVIT_KEY_RETURN) {
                	search_list=gui_internal_menu_data(this)->search_list;
                	if(search_list) {
				GList *l=gui_internal_widget_table_top_row(this, search_list);
				if (l && l->data) {
					struct widget *w=l->data;
					this->current.x=w->p.x+w->w/2;
					this->current.y=w->p.y+w->h/2;
					gui_internal_highlight(this);
				}
			} else {
				wi->reason=gui_internal_reason_keypress_finish;
				wi->func(this, wi, wi->data);
			}
                       	return; 
		} else if (*key == NAVIT_KEY_BACKSPACE) {
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
			wi->reason=gui_internal_reason_keypress;
			wi->func(this, wi, wi->data);
		}
		gui_internal_widget_render(this, wi);
	}
}


static void
gui_internal_cmd_keypress(struct gui_priv *this, struct widget *wm, void *data)
{
	struct menu_data *md;
	gui_internal_keypress_do(this, (char *) wm->data);
	md=gui_internal_menu_data(this);
	// Switch to lowercase after the first key is pressed
	if (md->keyboard_mode == 2) // Latin
		gui_internal_keyboard_do(this, md->keyboard, 10);
	if (md->keyboard_mode == 26) // Umlaut
		gui_internal_keyboard_do(this, md->keyboard, 34);
	if ((md->keyboard_mode & ~7) == 40) // Russian/Ukrainian/Belorussian
		gui_internal_keyboard_do(this, md->keyboard, 48);
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

static char *
postal_str(struct search_list_result *res, int level)
{
	char *ret=NULL;
	if (res->town->common.postal)
		ret=res->town->common.postal;
	if (res->town->common.postal_mask)
		ret=res->town->common.postal_mask;
	if (level == 1)
		return ret;
	if (res->street->common.postal)
		ret=res->street->common.postal;
	if (res->street->common.postal_mask)
		ret=res->street->common.postal_mask;
	if (level == 2)
		return ret;
	if (res->house_number->common.postal)
		ret=res->house_number->common.postal;
	if (res->house_number->common.postal_mask)
		ret=res->house_number->common.postal_mask;
	return ret;
}

static char *
district_str(struct search_list_result *res, int level)
{
	char *ret=NULL;
	if (res->town->common.district_name)
		ret=res->town->common.district_name;
	if (level == 1)
		return ret;
	if (res->street->common.district_name)
		ret=res->street->common.district_name;
	if (level == 2)
		return ret;
	if (res->house_number->common.district_name)
		ret=res->house_number->common.district_name;
	return ret;
}

static char *
town_str(struct search_list_result *res, int level, int flags)
{
	char *town=res->town->common.town_name;
	char *district=district_str(res, level);
	char *postal=postal_str(res, level);
	char *postal_sep=" ";
	char *district_begin=" (";
	char *district_end=")";
	char *county_sep = ", Co. ";
	char *county = res->town->common.county_name;
	if (!postal)
		postal_sep=postal="";
	if (!district || (flags & 1))
		district_begin=district_end=district="";
	if (!county)
		county_sep=county="";

	return g_strdup_printf("%s%s%s%s%s%s%s%s", postal, postal_sep, town, district_begin, district, district_end, county_sep, county);
}

static int gui_internal_search_cmp(gconstpointer _a, gconstpointer _b)
{
  struct widget *a=(struct widget *)_a, *b=(struct widget *)_b;
  char *sa,*sb;
  int r;
  if(!b)
  if((!a || a->type!=widget_table_row || !a->text) && (!b || b->type!=widget_table_row || !b->text))
  	return 0;
  if(!a || a->type!=widget_table_row || !a->text)
  	return -1;
  if(!b || b->type!=widget_table_row || !b->text)
  	return 1;
  r=a->datai-b->datai;
  if(r<0)
  	return -1;
  if(r>0)
  	return 1;
  sa=removecase(a->text);
  sb=removecase(b->text);
  r=strcmp(sa,sb);
  g_free(sa);
  g_free(sb);
  return r;
}

static void
gui_internal_search_idle(struct gui_priv *this, char *wm_name, struct widget *search_list, void *param)
{
	char *text=NULL,*text2=NULL,*name=NULL;
	struct search_list_result *res;
	struct widget *wc;
	struct item *item=NULL;
	GList *l;
	static char possible_keys[256]="";
        struct widget *wi=NULL;
	struct widget *wr;

	res=search_list_get_result(this->sl);
	if (res) {
		gchar* trunk_name = NULL;

		struct widget *menu=g_list_last(this->root.children)->data;
		wi=gui_internal_find_widget(menu, NULL, STATE_EDIT);

		if (wi) {
			if (! strcmp(wm_name,"Town"))
				trunk_name = g_strrstr(res->town->common.town_name, wi->text);
			if (! strcmp(wm_name,"Street"))
			{
				name=res->street->name;
				if (name)
					trunk_name = g_strrstr(name, wi->text);
				else
					trunk_name = NULL;
			}

			if (trunk_name) {
				char next_char = trunk_name[strlen(wi->text)];
				int i;
				int len = strlen(possible_keys);
				for(i = 0; (i<len) && (possible_keys[i] != next_char) ;i++) ;
				if (i==len || !len) {
					possible_keys[len]=trunk_name[strlen(wi->text)];
					possible_keys[len+1]='\0';
				}
				dbg(1,"%s %s possible_keys:%s \n", wi->text, res->town->common.town_name, possible_keys);
			}
		} else {
			dbg(0, "Unable to find widget");
		}
	}

	if (! res) {
		struct menu_data *md;
		gui_internal_search_idle_end(this);

		md=gui_internal_menu_data(this);
		if (md && md->keyboard && !(this->flags & 2048)) {
			GList *lk=md->keyboard->children;
			graphics_draw_mode(this->gra, draw_mode_begin);
			while (lk) {
				struct widget *child=lk->data;
				GList *lk2=child->children;
				while (lk2) {
					struct widget *child_=lk2->data;
					lk2=g_list_next(lk2);
					if (child_->data && strcmp("\b", child_->data)) { // FIXME don't disable special keys
						if (strlen(possible_keys) == 0)
							child_->state|= STATE_HIGHLIGHTED|STATE_VISIBLE|STATE_SENSITIVE|STATE_CLEAR ;
						else if (g_strrstr(possible_keys, child_->data)!=NULL ) {
							child_->state|= STATE_HIGHLIGHTED|STATE_VISIBLE|STATE_SENSITIVE|STATE_CLEAR ;
						} else {
							child_->state&= ~(STATE_HIGHLIGHTED|STATE_VISIBLE|STATE_SELECTED) ;
						}
						gui_internal_widget_render(this,child_);
					}
				}
				lk=g_list_next(lk);
			}
			gui_internal_widget_render(this,md->keyboard);
			graphics_draw_mode(this->gra, draw_mode_end);
		}

		possible_keys[0]='\0';
		return;
	}

	if (! strcmp(wm_name,"Country")) {
		name=res->country->name;
		item=&res->country->common.item;
		text=g_strdup_printf("%s", res->country->name);
	}
	if (! strcmp(wm_name,"Town")) {
		item=&res->town->common.item;
		name=res->town->common.town_name;
		text=town_str(res, 1, 0);
	}
	if (! strcmp(wm_name,"Street")) {
		name=res->street->name;
		item=&res->street->common.item;
		text=g_strdup(res->street->name);
		text2=town_str(res, 2, 1);
	}
	if (! strcmp(wm_name,"House number")) {
		name=res->house_number->house_number;
		text=g_strdup_printf("%s %s", res->street->name, name);
		text2=town_str(res, 3, 0);
	}
	dbg(1,"res->country->flag=%s\n", res->country->flag);
	wr=gui_internal_widget_table_row_new(this, gravity_left|orientation_horizontal|flags_fill);

		if (!text2) {
			wr->text=g_strdup(text);
			gui_internal_widget_insert_sorted(search_list, wr, gui_internal_search_cmp);
			gui_internal_widget_append(wr,
				wc=gui_internal_button_new_with_callback(this, text,
					image_new_xs(this, res->country->flag),
					gravity_left_center|orientation_horizontal|flags_fill,
					gui_internal_cmd_position, param));
		} else {
			struct widget *wb;
			wr->text=g_strdup_printf("%s %s",text,text2);

			if (! strcmp(wm_name,"House number") && !res->street->name) {
				wr->datai=1000;
			} else if(name) {
				if(!wi)
					dbg(0,"search text widget is NULL\n");
				if(wi && strlen(name)==strlen(wi->text)) {
					dbg(1,"xact %s %s\n",name, wi->text);
					wr->datai=-1000;
				} else {
					dbg(1,"not xact %s %s\n",name, wi->text);
					wr->datai=0;
				}
			}
			gui_internal_widget_insert_sorted(search_list, wr, gui_internal_search_cmp);
			
			wc=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|flags_fill);
			gui_internal_widget_append(wr, wc);
			gui_internal_widget_append(wc, gui_internal_image_new(this, image_new_xs(this, res->country->flag)));
			wb=gui_internal_box_new(this, gravity_left_center|orientation_vertical|flags_fill);
			gui_internal_widget_append(wc, wb);
			gui_internal_widget_append(wb, gui_internal_label_new(this, text));
			gui_internal_widget_append(wb, gui_internal_label_font_new(this, text2, 1));
			wc->func=gui_internal_cmd_position;
			wc->data=param;
			wc->state |= STATE_SENSITIVE;
			wc->speech=g_strdup(text);
		}
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
	g_free(text);
	g_free(text2);
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
	void *param=(void *)3;
	int minlen=1;

	gui_internal_widget_table_clear(this, search_list);

	if (! strcmp(wm->name,"Country"))
		param=(void *)4;
	if (! strcmp(wm->name,"Street"))
		param=(void *)5;
	if (! strcmp(wm->name,"House number"))
		param=(void *)6;
	dbg(0,"%s now '%s'\n", wm->name, wm->text);

	gui_internal_search_idle_end(this);
	if (wm->text && g_utf8_strlen(wm->text, -1) >= minlen) {
		struct attr search_attr;

		dbg(0,"process\n");
		if (! strcmp(wm->name,"Country"))
			search_attr.type=attr_country_all;
		if (! strcmp(wm->name,"Town"))
			search_attr.type=attr_town_or_district_name;
		if (! strcmp(wm->name,"Street"))
			search_attr.type=attr_street_name;
		if (! strcmp(wm->name,"House number"))
			search_attr.type=attr_house_number;
		search_attr.u.str=wm->text;
		search_list_search(this->sl, &search_attr, 1);
		gui_internal_search_idle_start(this, wm->name, search_list, param);
	}
	l=g_list_last(this->root.children);
	gui_internal_widget_render(this, l->data);
}

static struct widget *
gui_internal_keyboard_key_data(struct gui_priv *this, struct widget *wkbd, char *text, int font, void(*func)(struct gui_priv *priv, struct widget *widget, void *data), void *data, void (*data_free)(void *data), int w, int h)
{
	struct widget *wk;
	gui_internal_widget_append(wkbd, wk=gui_internal_button_font_new_with_callback(this, text, font,
		NULL, gravity_center|orientation_vertical, func, data));
	wk->data_free=data_free;
	wk->background=this->background;
	wk->bl=0;
	wk->br=0;
	wk->bt=0;
	wk->bb=0;
	wk->w=w;
	wk->h=h;
	return wk;
}

static struct widget *
gui_internal_keyboard_key(struct gui_priv *this, struct widget *wkbd, char *text, char *key, int w, int h)
{
	return gui_internal_keyboard_key_data(this, wkbd, text, 0, gui_internal_cmd_keypress, g_strdup(key), g_free_func,w,h);
}

static void gui_internal_keyboard_change(struct gui_priv *this, struct widget *key, void *data);


// A list of availiable keyboard modes.
struct gui_internal_keyb_mode {
    char title[16]; // Label to be displayed on keys that switch to it
    int font; // Font size of label
    int case_mode; // Mode to switch to when case CHANGE() key is pressed.
    int umlaut_mode;  // Mode to switch to when UMLAUT() key is pressed.
} gui_internal_keyb_modes[]= {
	/* 0*/ {"ABC", 2,  8, 24},
	/* 8*/ {"abc", 2,  0, 32},
	/*16*/ {"123", 2,  0, 24},
	/*24*/ {"ÄÖÜ", 2, 40, 0},
	/*32*/ {"äöü", 2, 32, 8},
	/*40*/ {"АБВ", 2, 48,  0},
	/*48*/ {"абв", 2, 40,  8}
};


// Some macros that make the keyboard layout easier to visualise in
// the source code. The macros are #undef'd after this function.
#define KEY(x) gui_internal_keyboard_key(this, wkbd, (x), (x), max_w, max_h)
#define SPACER() gui_internal_keyboard_key_data(this, wkbd, "", 0, NULL, NULL, NULL,max_w,max_h)
#define MODE(x) gui_internal_keyboard_key_data(this, wkbd, \
		gui_internal_keyb_modes[(x)/8].title, \
		gui_internal_keyb_modes[(x)/8].font, \
		gui_internal_keyboard_change, wkbdb, NULL,max_w,max_h) \
			-> datai=(mode&7)+((x)&~7)
#define SWCASE() MODE(gui_internal_keyb_modes[mode/8].case_mode)
#define UMLAUT() MODE(gui_internal_keyb_modes[mode/8].umlaut_mode)
static struct widget *
gui_internal_keyboard_do(struct gui_priv *this, struct widget *wkbdb, int mode)
{
	struct widget *wkbd,*wk;
	struct menu_data *md=gui_internal_menu_data(this);
	int i, max_w=this->root.w, max_h=this->root.h;
	int render=0;
	char *space="_";
	char *backspace="←";
	char *hide="▼";
	char *unhide="▲";

	if (wkbdb) {
		this->current.x=-1;
		this->current.y=-1;
		gui_internal_highlight(this);
		if (md->keyboard_mode >= 1024)
			render=2;
		else
			render=1;
		gui_internal_widget_children_destroy(this, wkbdb);
	} else
		wkbdb=gui_internal_box_new(this, gravity_center|orientation_horizontal_vertical|flags_fill);
	md->keyboard=wkbdb;
	md->keyboard_mode=mode;
	wkbd=gui_internal_box_new(this, gravity_center|orientation_horizontal_vertical|flags_fill);
	wkbd->background=this->background;
	wkbd->cols=8;
	wkbd->spx=0;
	wkbd->spy=0;
	max_w=max_w/8;
	max_h=max_h/8; // Allows 3 results in the list when searching for Towns
	wkbd->p.y=max_h*2;
	if(mode>=40&&mode<56) { // Russian/Ukrainian/Belarussian layout needs more space...
		max_h=max_h*4/5;
		max_w=max_w*8/9;
		wkbd->cols=9;
	}

	if (mode >= 0 && mode < 8) {
		for (i = 0 ; i < 26 ; i++) {
			char text[]={'A'+i,'\0'};
			KEY(text);
		}
		gui_internal_keyboard_key(this, wkbd, space," ",max_w,max_h);
		if (mode == 0) {
			KEY("-");
			KEY("'");
			wk=gui_internal_keyboard_key_data(this, wkbd, hide, 0, gui_internal_keyboard_change, wkbdb, NULL,max_w,max_h);
			wk->datai=mode+1024;
		} else {
			wk=gui_internal_keyboard_key_data(this, wkbd, hide, 0, gui_internal_keyboard_change, wkbdb, NULL,max_w,max_h);
			wk->datai=mode+1024;
			SWCASE();
			MODE(16);
			
		}
		UMLAUT();
		gui_internal_keyboard_key(this, wkbd, backspace,"\b",max_w,max_h);
	}
	if (mode >= 8 && mode < 16) {
		for (i = 0 ; i < 26 ; i++) {
			char text[]={'a'+i,'\0'};
			KEY(text);
		}
		gui_internal_keyboard_key(this, wkbd, space," ",max_w,max_h);
		if (mode == 8) {
			KEY("-");
			KEY("'");
			wk=gui_internal_keyboard_key_data(this, wkbd, hide, 0, gui_internal_keyboard_change, wkbdb, NULL,max_w,max_h);
			wk->datai=mode+1024;
		} else {
			wk=gui_internal_keyboard_key_data(this, wkbd, hide, 0, gui_internal_keyboard_change, wkbdb, NULL,max_w,max_h);
			wk->datai=mode+1024;
			SWCASE();
			
			MODE(16);
		}
		UMLAUT();
		gui_internal_keyboard_key(this, wkbd, backspace,"\b",max_w,max_h);
	}
	if (mode >= 16 && mode < 24) {
		for (i = 0 ; i < 10 ; i++) {
			char text[]={'0'+i,'\0'};
			KEY(text);
		}
		KEY("."); KEY("°"); KEY("'"); KEY("\""); KEY("-"); KEY("+");
		KEY("*"); KEY("/"); KEY("("); KEY(")"); KEY("="); KEY("?");

		

		if (mode == 16) {
			for (i = 0 ; i < 5 ; i++) SPACER();
			KEY("-");
			KEY("'");
			wk=gui_internal_keyboard_key_data(this, wkbd, hide, 0, gui_internal_keyboard_change, wkbdb, NULL,max_w,max_h);
			wk->datai=mode+1024;
		} else {
			for (i = 0 ; i < 3 ; i++) SPACER();
			MODE(40);
			MODE(48);
			wk=gui_internal_keyboard_key_data(this, wkbd, hide, 0, gui_internal_keyboard_change, wkbdb, NULL,max_w,max_h);
			wk->datai=mode+1024;
			MODE(0);
			MODE(8);
		}
		UMLAUT();
		gui_internal_keyboard_key(this, wkbd, backspace,"\b",max_w,max_h);
	}
	if (mode >= 24 && mode < 32) {
		KEY("Ä"); KEY("Ë"); KEY("Ï"); KEY("Ö"); KEY("Ü"); KEY("Æ"); KEY("Ø"); KEY("Å");
		KEY("Á"); KEY("É"); KEY("Í"); KEY("Ó"); KEY("Ú"); KEY("Š"); KEY("Č"); KEY("Ž");
		KEY("À"); KEY("È"); KEY("Ì"); KEY("Ò"); KEY("Ù"); KEY("Ś"); KEY("Ć"); KEY("Ź");
		KEY("Â"); KEY("Ê"); KEY("Î"); KEY("Ô"); KEY("Û"); SPACER();

		UMLAUT();

		gui_internal_keyboard_key(this, wkbd, backspace,"\b",max_w,max_h);
	}
	if (mode >= 32 && mode < 40) {
		KEY("ä"); KEY("ë"); KEY("ï"); KEY("ö"); KEY("ü"); KEY("æ"); KEY("ø"); KEY("å");
		KEY("á"); KEY("é"); KEY("í"); KEY("ó"); KEY("ú"); KEY("š"); KEY("č"); KEY("ž");
		KEY("à"); KEY("è"); KEY("ì"); KEY("ò"); KEY("ù"); KEY("ś"); KEY("ć"); KEY("ź");
		KEY("â"); KEY("ê"); KEY("î"); KEY("ô"); KEY("û"); KEY("ß");

		UMLAUT();

		gui_internal_keyboard_key(this, wkbd, backspace,"\b",max_w,max_h);
	}
	if (mode >= 40 && mode < 48) {
		KEY("А"); KEY("Б"); KEY("В"); KEY("Г"); KEY("Д"); KEY("Е"); KEY("Ж"); KEY("З"); KEY("И");
		KEY("Й"); KEY("К"); KEY("Л"); KEY("М"); KEY("Н"); KEY("О"); KEY("П"); KEY("Р"); KEY("С");
		KEY("Т"); KEY("У"); KEY("Ф"); KEY("Х"); KEY("Ц"); KEY("Ч"); KEY("Ш"); KEY("Щ"); KEY("Ъ"); 
		KEY("Ы"); KEY("Ь"); KEY("Э"); KEY("Ю"); KEY("Я"); KEY("Ё"); KEY("І"); KEY("Ї"); KEY("Ў");
		SPACER(); SPACER(); SPACER();
		gui_internal_keyboard_key(this, wkbd, space," ",max_w,max_h);

		wk=gui_internal_keyboard_key_data(this, wkbd, hide, 0, gui_internal_keyboard_change, wkbdb, NULL,max_w,max_h);
		wk->datai=mode+1024;

		SWCASE();

		MODE(16);

		SPACER();

		gui_internal_keyboard_key(this, wkbd, backspace,"\b",max_w,max_h);
	}
	if (mode >= 48 && mode < 56) {
		KEY("а"); KEY("б"); KEY("в"); KEY("г"); KEY("д"); KEY("е"); KEY("ж"); KEY("з"); KEY("и");
		KEY("й"); KEY("к"); KEY("л"); KEY("м"); KEY("н"); KEY("о"); KEY("п"); KEY("р"); KEY("с");
		KEY("т"); KEY("у"); KEY("ф"); KEY("х"); KEY("ц"); KEY("ч"); KEY("ш"); KEY("щ"); KEY("ъ");
		KEY("ы"); KEY("ь"); KEY("э"); KEY("ю"); KEY("я"); KEY("ё"); KEY("і"); KEY("ї"); KEY("ў");
		SPACER(); SPACER(); SPACER();
		gui_internal_keyboard_key(this, wkbd, space," ",max_w,max_h);
		
		wk=gui_internal_keyboard_key_data(this, wkbd, hide, 0, gui_internal_keyboard_change, wkbdb, NULL,max_w,max_h);
		wk->datai=mode+1024;

		SWCASE();

		MODE(16);

		SPACER();

		gui_internal_keyboard_key(this, wkbd, backspace,"\b",max_w,max_h);
	}


	if(md->search_list && md->search_list->type==widget_table) {
		struct table_data *td=(struct table_data*)(md->search_list->data);
		td->button_box_hide=mode<1024;
	}

	if (mode == 1023) { /* special case for coordinates input screen (enter_coord) */
		KEY("0"); KEY("1"); KEY("2"); KEY("3"); KEY("4"); SPACER(); KEY("N"); KEY("S");
		KEY("5"); KEY("6"); KEY("7"); KEY("8"); KEY("9"); SPACER(); KEY("E"); KEY("W");
		KEY("°"); KEY("."); KEY("'"); 
		gui_internal_keyboard_key(this, wkbd, space," ",max_w,max_h);
		SPACER();
		SPACER();
		SPACER();
		gui_internal_keyboard_key(this, wkbd, backspace,"\b",max_w,max_h);
	}	

	if (mode >= 1024) {
		char *text=NULL;
		int font=0;
		struct widget *wkl;
		mode -= 1024;
		text=gui_internal_keyb_modes[mode/8].title;
		font=gui_internal_keyb_modes[mode/8].font;
		wk=gui_internal_box_new(this, gravity_center|orientation_horizontal|flags_fill);
		wk->func=gui_internal_keyboard_change;
		wk->data=wkbdb;
		wk->background=this->background;
		wk->bl=0;
		wk->br=0;
		wk->bt=0;
		wk->bb=0;
		wk->w=max_w;
		wk->h=max_h;
		wk->datai=mode;
		wk->state |= STATE_SENSITIVE;
		gui_internal_widget_append(wk, wkl=gui_internal_label_new(this, unhide));
		wkl->background=NULL;
		gui_internal_widget_append(wk, wkl=gui_internal_label_font_new(this, text, font));
		wkl->background=NULL;
		gui_internal_widget_append(wkbd, wk);
		if (render)
			render=2;
	}
	gui_internal_widget_append(wkbdb, wkbd);
	if (render == 1) {
		gui_internal_widget_pack(this, wkbdb);
		gui_internal_widget_render(this, wkbdb);
	} else if (render == 2) {
		gui_internal_menu_reset_pack(this);
		gui_internal_menu_render(this);
	}
	return wkbdb;
}
#undef KEY
#undef SPACER
#undef SWCASE
#undef UMLAUT
#undef MODE

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
	if (tracking && tracking_get_attr(tracking, attr_country_id, &search_attr, NULL))
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
			if(this->country_iso2) {
				g_free(this->country_iso2);
				this->country_iso2=NULL;
			}
			if (item_attr_get(item, attr_country_iso2, &country_iso2))
				this->country_iso2=g_strdup(country_iso2.u.str);
		}
		country_search_destroy(cs);
	} else {
		dbg(0,"warning: no default country found\n");
		if (this->country_iso2) {
		    dbg(0,"attempting to use country '%s'\n",this->country_iso2);
		    search_attr.type=attr_country_iso2;
		    search_attr.u.str=this->country_iso2;
            search_list_search(this->sl, &search_attr, 0);
            while((res=search_list_get_result(this->sl)));
		}
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
	int keyboard_mode=2;
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
		if (this->country_iso2) {
#if HAVE_API_ANDROID
			char country_iso2[strlen(this->country_iso2)+1];
			strtolower(country_iso2, this->country_iso2);
			country=g_strdup_printf("country_%s", country_iso2);
#else
			country=g_strdup_printf("country_%s", this->country_iso2);
#endif
		} else
			country=g_strdup("gui_select_country");
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
		wnext=gui_internal_image_new(this, image_new_xs(this, "gui_select_house_number"));
		wnext->func=gui_internal_search_house_number;
	} else if (!strcmp(type,"House number")) {
		gui_internal_widget_append(we, wb=gui_internal_image_new(this, image_new_xs(this, "gui_select_street")));
		wb->state |= STATE_SENSITIVE;
		wb->func = gui_internal_back;
		keyboard_mode=18;
	}
	gui_internal_widget_append(we, wk=gui_internal_label_new(this, NULL));
	if (wnext) {
		gui_internal_widget_append(we, wnext);
		wnext->state |= STATE_SENSITIVE;
	}
	wl=gui_internal_widget_table_new(this,gravity_left_top | flags_fill | flags_expand |orientation_vertical,1);//gui_internal_box_new(this, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(wr, wl);
	gui_internal_menu_data(this)->search_list=wl;
	wk->state |= STATE_EDIT|STATE_EDITABLE;
	wk->background=this->background;
	wk->flags |= flags_expand|flags_fill;
	wk->func = gui_internal_search_changed;
	wk->name=g_strdup(type);
	if (this->keyboard)
		gui_internal_widget_append(w, gui_internal_keyboard(this,keyboard_mode));
	gui_internal_menu_render(this);
}

static void
gui_internal_search_house_number(struct gui_priv *this, struct widget *widget, void *data)
{
	search_list_select(this->sl, attr_street_name, 0, 0);
	gui_internal_search(this,_("House number"),"House number",0);
}

static void
gui_internal_search_house_number_in_street(struct gui_priv *this, struct widget *widget, void *data)
{
	dbg(0,"id %d\n", widget->selection_id);
	search_list_select(this->sl, attr_street_name, 0, 0);
	search_list_select(this->sl, attr_street_name, widget->selection_id, 1);
	gui_internal_search(this,_("House number"),"House number",0);
}

static void
gui_internal_search_street(struct gui_priv *this, struct widget *widget, void *data)
{
	search_list_select(this->sl, attr_town_or_district_name, 0, 0);
	gui_internal_search(this,_("Street"),"Street",0);
}

static void
gui_internal_search_street_in_town(struct gui_priv *this, struct widget *widget, void *data)
{
	dbg(0,"id %d\n", widget->selection_id);
	search_list_select(this->sl, attr_town_or_district_name, 0, 0);
	search_list_select(this->sl, attr_town_or_district_name, widget->selection_id, 1);
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
		this->country_iso2=g_strdup(((struct search_list_country *)slc)->iso2);
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
gui_internal_cmd2_town(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	if (this->sl)
		search_list_select(this->sl, attr_country_all, 0, 0);
	gui_internal_search(this,_("Town"),"Town",1);
}

static void
gui_internal_cmd2_setting_layout(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	struct attr attr;
	struct widget *w,*wb,*wl,*row;
	struct attr_iter *iter;


	wb=gui_internal_menu(this, _("Layout"));
	w=gui_internal_widget_table_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill,1);
	gui_internal_widget_append(wb, w);
	iter=navit_attr_iter_new();
	while(navit_get_attr(this->nav, attr_layout, &attr, iter)) {
		gui_internal_widget_append(w, row=gui_internal_widget_table_row_new(this,gravity_left|orientation_horizontal|flags_fill));
		wl=gui_internal_button_navit_attr_new(this, attr.u.layout->name, gravity_left_center|orientation_horizontal|flags_fill,
			&attr, NULL);
		gui_internal_widget_append(row, wl);
	}
	navit_attr_iter_destroy(iter);
	gui_internal_menu_render(this);
}

static char *
gui_internal_cmd_match_expand(char *pattern, struct attr **in)
{
	char p,*ret=g_strdup(pattern),*r=ret,*a;
	int len;
	while ((p=*pattern++)) {
		switch (p) {
		case '*':
			*r='\0';
			a=attr_to_text(*in++,NULL,0);
			len=strlen(ret)+strlen(a)+strlen(pattern)+1;
			r=g_malloc(len);
			strcpy(r, ret);
			strcat(r, a);
			g_free(ret);
			g_free(a);
			ret=r;
			r=ret+strlen(ret);
			break;
		case '\\':
			p=*pattern++;
		default:
			*r++=p;
		}	
	}
	*r++='\0';
	return ret;
}

static int 
gui_internal_match(const char *pattern, const char *string)
{
	char p,s;
	while ((p=*pattern++)) {
		switch (p) {
		case '*':
			while ((s=*string)) {
				if (gui_internal_match(pattern,string))
					return 1;
				string++;
			}
			break;
		case '\\':
			p=*pattern++;
		default:
			if (*string++ != p)
				return 0;
		}
	}
	return 1;
}

static int
gui_internal_set(char *remove, char *add)
{
	char *gui_file=g_strjoin(NULL, navit_get_user_data_directory(TRUE), "/gui_internal.txt", NULL);
	char *gui_file_new=g_strjoin(NULL, navit_get_user_data_directory(TRUE), "/gui_internal_new.txt", NULL);
	FILE *fo=fopen(gui_file_new,"w");
	FILE *fi=fopen(gui_file,"r");
	char *line=NULL;
	int ret;
	size_t size=0;
	if (fi != NULL){
		while (getline(&line,&size,fi) > 0) {
			int len=strlen(line);
			if (len > 0 && line[len-1] == '\n')
				line[len-1]='\0';
			dbg(1,"line=%s\n",line);
			if (!gui_internal_match(remove, line))
				fprintf(fo,"%s\n",line);
		}
		if (line)
			free(line);
		fclose(fi);
	}
	fprintf(fo,"%s;\n",add);
	fclose(fo);
	ret=(rename(gui_file_new, gui_file)==0);
	g_free(gui_file_new);
	g_free(gui_file);

	return ret;
}

static void
gui_internal_cmd2_set(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	char *pattern,*command=NULL;
	if (!in || !in[0] || !ATTR_IS_STRING(in[0]->type)) {
		dbg(0,"first parameter missing or wrong type\n");
		return;
	}
	pattern=in[0]->u.str;
	dbg(0,"pattern %s\n",pattern);
	if (in[1]) {
		command=gui_internal_cmd_match_expand(pattern, in+1);
		dbg(0,"expand %s\n",command);
		gui_internal_set(pattern, command);
		command_evaluate(&this->self, command);
		g_free(command);
	}

}

static void
gui_internal_cmd2_quit(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	struct attr navit;
	gui_internal_prune_menu(this, NULL);
	navit.type=attr_navit;
	navit.u.navit=this->nav;
	config_remove_attr(config, &navit);
	gui_internal_destroy(this);
	event_main_loop_quit();
}

static void
gui_internal_window_closed(struct gui_priv *this)
{
	gui_internal_cmd2_quit(this, NULL, NULL, NULL, NULL);
}

static void
gui_internal_cmd2_abort_navigation(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	navit_set_destination(this->nav, NULL, NULL, 0);
}

static void
gui_internal_cmd_map_download_do(struct gui_priv *this, struct widget *wm, void *data)
{
	char *text=g_strdup_printf(_("Download %s"),wm->name);
	struct widget *w, *wb;
	struct map *map=data;
	double bllon,bllat,trlon,trlat;

	wb=gui_internal_menu(this, text);
	g_free(text);
	w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	w->spy=this->spacing*3;
	gui_internal_widget_append(wb, w);
        if (sscanf(wm->prefix,"%lf,%lf,%lf,%lf",&bllon,&bllat,&trlon,&trlat) == 4) {
		struct coord_geo g;
		struct map_selection sel;
		struct map_rect *mr;
		struct item *item;

		sel.next=NULL;
		sel.order=255;
		g.lng=bllon;
		g.lat=trlat;
		transform_from_geo(projection_mg, &g, &sel.u.c_rect.lu);
		g.lng=trlon;
		g.lat=bllat;
		transform_from_geo(projection_mg, &g, &sel.u.c_rect.rl);
		sel.range.min=type_none;
		sel.range.max=type_last;
		mr=map_rect_new(map, &sel);
		while ((item=map_rect_get_item(mr))) {
			dbg(0,"item\n");
		}
		map_rect_destroy(mr);
	}
	
	dbg(0,"bbox=%s\n",wm->prefix);
	gui_internal_menu_render(this);
}

static void
gui_internal_cmd_map_download(struct gui_priv *this, struct widget *wm, void *data)
{
	struct attr on, off, download_enabled, download_disabled;
	struct widget *w,*wb,*wma;
	struct map *map=data;
	FILE *f;
	char *search,buffer[256];
	int found,sp_match=0;

	dbg(1,"wm=%p prefix=%s\n",wm,wm->prefix);

	search=wm->prefix;
	if (search) {
		found=0;
		while(search[sp_match] == ' ')
			sp_match++;
		sp_match++;
	} else {
		found=1;
	}
	on.type=off.type=attr_active;
	on.u.num=1;
	off.u.num=0;
	wb=gui_internal_menu(this, wm->name?wm->name:_("Map Download"));
	w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	w->spy=this->spacing*3;
	gui_internal_widget_append(wb, w);
	if (!search) {
		wma=gui_internal_button_map_attr_new(this, _("Active"), gravity_left_center|orientation_horizontal|flags_fill, map, &on, &off, 1);
		gui_internal_widget_append(w, wma);
	}

	download_enabled.type=download_disabled.type=attr_update;
	download_enabled.u.num=1;
	download_disabled.u.num=0;
	wma=gui_internal_button_map_attr_new(this
		, _("Download Enabled")
		, gravity_left_center|orientation_horizontal|flags_fill
		, map
		, &download_enabled
		, &download_disabled
		, 0);
	gui_internal_widget_append(w, wma);


	f=fopen("maps/areas.tsv","r");
	while (f && fgets(buffer, sizeof(buffer), f)) {
		char *nl,*description,*description_size,*bbox,*size=NULL;
		int sp=0;
		if ((nl=strchr(buffer,'\n')))
			*nl='\0';
		if ((nl=strchr(buffer,'\r')))
			*nl='\0';
		while(buffer[sp] == ' ')
			sp++;
		if ((bbox=strchr(buffer,'\t')))
			*bbox++='\0';
		if (bbox && (size=strchr(bbox,'\t'))) 
			*size++='\0';
		if (search && !strcmp(buffer, search)) {
			wma=gui_internal_button_new_with_callback(this, _("Download completely"), NULL, 
				gravity_left_center|orientation_horizontal|flags_fill, gui_internal_cmd_map_download_do, map);
			wma->name=g_strdup(buffer+sp);
			wma->prefix=g_strdup(bbox);
			gui_internal_widget_append(w, wma);
			found=1;
		} else if (sp < sp_match)
			found=0;
		if (sp == sp_match && found && buffer[sp]) {
			description=g_strdup(buffer+sp);
			if (size)
				description_size=g_strdup_printf("%s (%s)",description,size);
			else
				description_size=g_strdup(description);
			wma=gui_internal_button_new_with_callback(this, description_size, NULL, 
				gravity_left_center|orientation_horizontal|flags_fill, gui_internal_cmd_map_download, map);
			g_free(description_size);
			wma->prefix=g_strdup(buffer);
			wma->name=description;
			gui_internal_widget_append(w, wma);
		}
	}
	
	gui_internal_menu_render(this);
}

static void
gui_internal_cmd2_setting_maps(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	struct attr attr, on, off, description, type, data, url, active;
	struct widget *w,*wb,*row,*wma;
	char *label;
	struct attr_iter *iter;

	wb=gui_internal_menu(this, _("Maps"));
	//w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	//w->spy=this->spacing*3;
	w = gui_internal_widget_table_new(this,gravity_left_top | flags_fill | flags_expand |orientation_vertical,1);
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
		if (map_get_attr(attr.u.map, attr_url, &url, NULL)) {
			if (!map_get_attr(attr.u.map, attr_active, &active, NULL))
				active.u.num=1;
			wma=gui_internal_button_new_with_callback(this, label, image_new_xs(this, active.u.num ? "gui_active" : "gui_inactive"),
			gravity_left_center|orientation_horizontal|flags_fill,
			gui_internal_cmd_map_download, attr.u.map);
		} else {
			wma=gui_internal_button_map_attr_new(this, label, gravity_left_center|orientation_horizontal|flags_fill,
				attr.u.map, &on, &off, 1);
		}	
		gui_internal_widget_append(w, row=gui_internal_widget_table_row_new(this,gravity_left|orientation_horizontal|flags_fill));
		gui_internal_widget_append(row, wma);
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
	gui_internal_widget_append(row, gui_internal_label_new(this, _(" Elevation ")));
	gui_internal_widget_append(row, gui_internal_label_new(this, _(" Azimuth ")));
	gui_internal_widget_append(row, gui_internal_label_new(this, " SNR "));
	gui_internal_widget_append(w,row);
	while (vehicle_get_attr(v, attr_position_sat_item, &attr, NULL)) {
		row = gui_internal_widget_table_row_new(this,gravity_left_top);
		for (i = 0 ; i < sizeof(types)/sizeof(enum attr_type) ; i++) {
			if (item_attr_get(attr.u.item, types[i], &sat_attr))
				str=g_strdup_printf("%ld", sat_attr.u.num);
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

/**
 * A container to hold the selected vehicle and the desired profile in
 * one data item.
 */
struct vehicle_and_profilename {
	struct vehicle *vehicle;
	char *profilename;
};

/**
 * Figures out whether the given vehicle is the active vehicle.
 *
 * @return true if the vehicle is active, false otherwise.
 */
static int
gui_internal_is_active_vehicle(struct gui_priv *this, struct vehicle
        *vehicle)
{
	struct attr active_vehicle;

	if (!navit_get_attr(this->nav, attr_vehicle, &active_vehicle, NULL))
        active_vehicle.u.vehicle=NULL;

	return active_vehicle.u.vehicle == vehicle;
}

static void
save_vehicle_xml(struct vehicle *v)
{
	struct attr attr;
	struct attr_iter *iter=vehicle_attr_iter_new();
	int childs=0;
	dbg(0,"enter\n");
	printf("<vehicle");
	while (vehicle_get_attr(v, attr_any_xml, &attr, iter)) {
		if (ATTR_IS_OBJECT(attr.type))
			childs=1;
		else	{
			char *attrtxt;
			printf(" %s=\"%s\"",attr_to_name(attr.type),attrtxt=attr_to_text(&attr, NULL, 1));
			g_free(attrtxt);
		}
	}
	if (childs) {
		printf(">\n");
		printf("</vehicle>\n");
	} else
		printf(" />\n");
	vehicle_attr_iter_destroy(iter);
}


/**
 * Reacts to a button press that changes a vehicle's active profile.
 *
 * @see gui_internal_add_vehicle_profile
 */
static void
gui_internal_cmd_set_active_profile(struct gui_priv *this, struct
		widget *wm, void *data)
{
	struct vehicle_and_profilename *vapn = data;
	struct vehicle *v = vapn->vehicle;
	char *profilename = vapn->profilename;
	struct attr vehicle_name_attr;
	char *vehicle_name = NULL;
	struct attr profilename_attr;

	// Get the vehicle name
	vehicle_get_attr(v, attr_name, &vehicle_name_attr, NULL);
	vehicle_name = vehicle_name_attr.u.str;

	dbg(0, "Changing vehicle %s to profile %s\n", vehicle_name,
			profilename);

	// Change the profile name
	profilename_attr.type = attr_profilename;
	profilename_attr.u.str = profilename;
	if(!vehicle_set_attr(v, &profilename_attr)) {
		dbg(0, "Unable to set the vehicle's profile name\n");
	}

    // Notify Navit that the routing should be re-done if this is the
    // active vehicle.
	if (gui_internal_is_active_vehicle(this, v)) {
		struct attr vehicle;
		vehicle.type=attr_vehicle;
		vehicle.u.vehicle=v;
		navit_set_attr(this->nav, &vehicle);
	}
	save_vehicle_xml(v);
	
	gui_internal_prune_menu_count(this, 1, 0);
	gui_internal_menu_vehicle_settings(this, v, vehicle_name);
}

/**
 * Adds the vehicle profile to the GUI, allowing the user to pick a
 * profile for the currently selected vehicle.
 */
static void
gui_internal_add_vehicle_profile(struct gui_priv *this, struct widget
		*parent, struct vehicle *v, struct vehicleprofile *profile)
{
	// Just here to show up in the translation file, nice and close to
	// where the translations are actually used.
	struct attr profile_attr;
	struct attr *attr = NULL;
	char *name = NULL;
	char *active_profile = NULL;
	char *label = NULL;
	int active;
	struct vehicle_and_profilename *context = NULL;

#ifdef ONLY_FOR_TRANSLATION
	char *translations[] = {_n("car"), _n("bike"), _n("pedestrian")};
#endif

	// Figure out the profile name
	attr = attr_search(profile->attrs, NULL, attr_name);
	if (!attr) {
		dbg(0, "Adding vehicle profile failed. attr==NULL");
		return;
	}
	name = attr->u.str;

	// Determine whether the profile is the active one
	if (vehicle_get_attr(v, attr_profilename, &profile_attr, NULL))
		active_profile = profile_attr.u.str;
	active = active_profile != NULL && !strcmp(name, active_profile);

	dbg(0, "Adding vehicle profile %s, active=%s/%i\n", name,
			active_profile, active);

	// Build a translatable label.
	if(active) {
		label = g_strdup_printf(_("Current profile: %s"), _(name));
	} else {
		label = g_strdup_printf(_("Change profile to: %s"), _(name));
	}

	// Create the context object (the vehicle and the desired profile)
	context = g_new0(struct vehicle_and_profilename, 1);
	context->vehicle = v;
	context->profilename = name;

	// Add the button
	gui_internal_widget_append(parent,
		gui_internal_button_new_with_callback(
			this, label,
			image_new_xs(this, active ? "gui_active" : "gui_inactive"),
			gravity_left_center|orientation_horizontal|flags_fill,
			gui_internal_cmd_set_active_profile, context));

	free(label);
}

static void
gui_internal_menu_vehicle_settings(struct gui_priv *this, struct vehicle *v, char *name)
{
	struct widget *w,*wb,*row;
	struct attr attr;
    struct vehicleprofile *profile = NULL;
	GList *profiles;

	wb=gui_internal_menu(this, name);
	w=gui_internal_widget_table_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill,1);
	gui_internal_widget_append(wb, w);

    // Add the "Set as active" button if this isn't the active
    // vehicle.
	if (!gui_internal_is_active_vehicle(this, v)) {
		gui_internal_widget_append(w, row=gui_internal_widget_table_row_new(this,gravity_left|orientation_horizontal|flags_fill));
		gui_internal_widget_append(row,
			gui_internal_button_new_with_callback(this, _("Set as active"),
				image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
				gui_internal_cmd_set_active_vehicle, v));
	}

	if (vehicle_get_attr(v, attr_position_sat_item, &attr, NULL)) {
		gui_internal_widget_append(w, row=gui_internal_widget_table_row_new(this,gravity_left|orientation_horizontal|flags_fill));
		gui_internal_widget_append(row,
			gui_internal_button_new_with_callback(this, _("Show Satellite status"),
				image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
				gui_internal_cmd_show_satellite_status, v));
	}
	if (vehicle_get_attr(v, attr_position_nmea, &attr, NULL)) {
		gui_internal_widget_append(w, row=gui_internal_widget_table_row_new(this,gravity_left|orientation_horizontal|flags_fill));
		gui_internal_widget_append(row,
			gui_internal_button_new_with_callback(this, _("Show NMEA data"),
				image_new_xs(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill,
				gui_internal_cmd_show_nmea_data, v));
	}

    // Add all the possible vehicle profiles to the menu
	profiles = navit_get_vehicleprofiles(this->nav);
    while(profiles) {
        profile = (struct vehicleprofile *)profiles->data;
	gui_internal_widget_append(w, row=gui_internal_widget_table_row_new(this,gravity_left|orientation_horizontal|flags_fill));
        gui_internal_add_vehicle_profile(this, row, v, profile);
		profiles = g_list_next(profiles);
    }

	callback_list_call_attr_2(this->cbl, attr_vehicle, w, v);
	gui_internal_menu_render(this);
}

static void
gui_internal_cmd_vehicle_settings(struct gui_priv *this, struct widget *wm, void *data)
{
	gui_internal_menu_vehicle_settings(this, wm->data, wm->text);
}

static void
gui_internal_cmd2_setting_vehicle(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	struct attr attr,attr2,vattr;
	struct widget *w,*wb,*wl;
	struct attr_iter *iter;
	struct attr active_vehicle;
    
	iter=navit_attr_iter_new();
	if (navit_get_attr(this->nav, attr_vehicle, &attr, iter) && !navit_get_attr(this->nav, attr_vehicle, &attr2, iter)) {
		vehicle_get_attr(attr.u.vehicle, attr_name, &vattr, NULL);
		navit_attr_iter_destroy(iter);
		gui_internal_menu_vehicle_settings(this, attr.u.vehicle, vattr.u.str);
		return;
	}
	navit_attr_iter_destroy(iter);

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
			image_new_xs(this, attr.u.vehicle == active_vehicle.u.vehicle ? "gui_active" : "gui_inactive"), gravity_left_center|orientation_horizontal|flags_fill,
			gui_internal_cmd_vehicle_settings, attr.u.vehicle);
		wl->text=g_strdup(vattr.u.str);
		gui_internal_widget_append(w, wl);
	}
	navit_attr_iter_destroy(iter);
	gui_internal_menu_render(this);
}


static void
gui_internal_cmd2_setting_rules(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
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
	on.type=off.type=attr_follow_cursor;
	gui_internal_widget_append(w,
		gui_internal_button_navit_attr_new(this, _("Map follows Vehicle"), gravity_left_center|orientation_horizontal|flags_fill,
			&on, &off));
	on.u.num=1;
	off.u.num=0;
	on.type=off.type=attr_waypoints_flag;
	gui_internal_widget_append(w,
			gui_internal_button_navit_attr_new(this, _("Plan with Waypoints"), gravity_left_center|orientation_horizontal|flags_fill,
					&on, &off));
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
		this->motion_timeout_callback=callback_new_1(callback_cast(gui_internal_motion_cb), this);
	if(!this->motion_timeout_event)
		this->motion_timeout_event=event_add_timeout(30,0, this->motion_timeout_callback);
}

static const char *
find_attr(const char **names, const char **values, const char *name)
{
	while (*names) {
		if (!g_strcasecmp(*names, name))
			return *values;
		names+=xml_attr_distance;
		values+=xml_attr_distance;
	}
	return NULL;
}

static char *
find_attr_dup(const char **names, const char **values, const char *name)
{
	return g_strdup(find_attr(names, values, name));
}

static void
gui_internal_evaluate(struct gui_priv *this, const char *command)
{
	if (command)
		command_evaluate(&this->self, command);
}


static void
gui_internal_html_command(struct gui_priv *this, struct widget *w, void *data)
{
	gui_internal_evaluate(this,w->command);
}

static void
gui_internal_html_submit_set(struct gui_priv *this, struct widget *w, struct form *form)
{
	GList *l;
	if (w->form == form && w->name) {
		struct attr *attr=attr_new_from_text(w->name, w->text?w->text:"");
		if (attr)
			gui_set_attr(this->self.u.gui, attr);
		attr_free(attr);
	}
	l=w->children;
	while (l) {
		w=l->data;
		gui_internal_html_submit_set(this, w, form);
		l=g_list_next(l);
	}

}

static void
gui_internal_html_submit(struct gui_priv *this, struct widget *w, void *data)
{
	struct widget *menu;
	GList *l;

	dbg(1,"enter form %p %s\n",w->form,w->form->onsubmit);
	l=g_list_last(this->root.children);
	menu=l->data;
	graphics_draw_mode(this->gra, draw_mode_begin);
	gui_internal_highlight_do(this, NULL);
	gui_internal_menu_render(this);
	graphics_draw_mode(this->gra, draw_mode_end);
	gui_internal_html_submit_set(this, menu, w->form);
	gui_internal_evaluate(this,w->form->onsubmit);
}

static void
gui_internal_html_load_href(struct gui_priv *this, char *href, int replace)
{
	if (replace)
		gui_internal_prune_menu_count(this, 1, 0);
	if (href && href[0] == '#') {
		dbg(1,"href=%s\n",href);
		g_free(this->href);
		this->href=g_strdup(href);
		gui_internal_html_menu(this, this->html_text, href+1);
	}
}

static void
gui_internal_html_href(struct gui_priv *this, struct widget *w, void *data)
{
	gui_internal_html_load_href(this, w->command, 0);
}

struct div_flags_map {
	char *attr;
	char *val;
	enum flags flags;
} div_flags_map[] = {
	{"gravity","none",gravity_none},
	{"gravity","left",gravity_left},
	{"gravity","xcenter",gravity_xcenter},
	{"gravity","right",gravity_right},
	{"gravity","top",gravity_top},
	{"gravity","ycenter",gravity_ycenter},
	{"gravity","bottom",gravity_bottom},
	{"gravity","left_top",gravity_left_top},
	{"gravity","top_center",gravity_top_center},
	{"gravity","right_top",gravity_right_top},
	{"gravity","left_center",gravity_left_center},
	{"gravity","center",gravity_center},
	{"gravity","right_center",gravity_right_center},
	{"gravity","left_bottom",gravity_left_bottom},
	{"gravity","bottom_center",gravity_bottom_center},
	{"gravity","right_bottom",gravity_right_bottom},
	{"expand","1",flags_expand},
	{"fill","1",flags_fill},
	{"orientation","horizontal",orientation_horizontal},
	{"orientation","vertical",orientation_vertical},
	{"orientation","horizontal_vertical",orientation_horizontal_vertical},
};

static enum flags
div_flag(const char **names, const char **values, char *name)
{
	int i;
	enum flags ret=0;
	const char *value=find_attr(names, values, name);
	if (!value)
		return ret;
	for (i = 0 ; i < sizeof(div_flags_map)/sizeof(struct div_flags_map); i++) {
		if (!strcmp(div_flags_map[i].attr,name) && !strcmp(div_flags_map[i].val,value))
			ret|=div_flags_map[i].flags;
	}
	return ret;
}

static enum flags
div_flags(const char **names, const char **values)
{
	enum flags flags;
	flags = div_flag(names, values, "gravity");
	flags |= div_flag(names, values, "orientation");
	flags |= div_flag(names, values, "expand");
	flags |= div_flag(names, values, "fill");
	return flags;
}

static struct widget *
html_image(struct gui_priv *this, const char **names, const char **values)
{
	const char *src, *size;
	struct graphics_image *img=NULL;

	src=find_attr(names, values, "src");
	if (!src)
		return NULL;
	size=find_attr(names, values, "size");
	if (!size)
		size="l";
	if (!strcmp(size,"l"))
		img=image_new_l(this, src);
	else if (!strcmp(size,"s"))
		img=image_new_s(this, src);
	else if (!strcmp(size,"xs"))
		img=image_new_xs(this, src);
	if (!img)
		return NULL;
	return gui_internal_image_new(this, img);
}

static void
gui_internal_html_start(void *dummy, const char *tag_name, const char **names, const char **values, void *data, void *error)
{
	struct gui_priv *this=data;
	int i;
	enum html_tag tag=html_tag_none;
	struct html *html=&this->html[this->html_depth];
	const char *cond, *type;

	if (!g_strcasecmp(tag_name,"text"))
		return;
	html->skip=0;
	html->command=NULL;
	html->name=NULL;
	html->href=NULL;
	html->refresh_cond=NULL;
	html->w=NULL;
	html->container=NULL;
	cond=find_attr(names, values, "cond");

	if (cond && !this->html_skip) {
		if (!command_evaluate_to_boolean(&this->self, cond, NULL))
			html->skip=1;
	}

	for (i=0 ; i < sizeof(html_tag_map)/sizeof(struct html_tag_map); i++) {
		if (!g_strcasecmp(html_tag_map[i].tag_name, tag_name)) {
			tag=html_tag_map[i].tag;
			break;
		}
	}
	html->tag=tag;
	if (!this->html_skip && !html->skip) {
		switch (tag) {
		case html_tag_a:
			html->name=find_attr_dup(names, values, "name");
			if (html->name) {
				html->skip=this->html_anchor ? strcmp(html->name,this->html_anchor) : 0;
				if (!html->skip)
					this->html_anchor_found=1;
			}
			html->command=find_attr_dup(names, values, "onclick");
			html->href=find_attr_dup(names, values, "href");
			html->refresh_cond=find_attr_dup(names, values, "refresh_cond");
			break;
		case html_tag_img:
			html->command=find_attr_dup(names, values, "onclick");
			html->w=html_image(this, names, values);
			break;
		case html_tag_form:
			this->form=g_new0(struct form, 1);
			this->form->onsubmit=find_attr_dup(names, values, "onsubmit");
			break;
		case html_tag_input:
			type=find_attr_dup(names, values, "type");
			if (!type)
				break;
			if (!strcmp(type,"image")) {
				html->w=html_image(this, names, values);
				if (html->w) {
					html->w->state|=STATE_SENSITIVE;
					html->w->func=gui_internal_html_submit;
				}
			}
			if (!strcmp(type,"text") || !strcmp(type,"password")) {
				html->w=gui_internal_label_new(this, NULL);
				html->w->background=this->background;
			        html->w->flags |= div_flags(names, values);
				html->w->state|=STATE_EDITABLE;
				if (!this->editable) {
					this->editable=html->w;
					html->w->state|=STATE_EDIT;
				}
				this->keyboard_required=1;
				if (!strcmp(type,"password"))
					html->w->flags2 |= 1;
			}
			if (html->w) {
				html->w->form=this->form;
				html->w->name=find_attr_dup(names, values, "name");
			}
			break;
		case html_tag_div:
			html->w=gui_internal_box_new(this, div_flags(names, values));
			html->container=this->html_container;
			this->html_container=html->w;
			break;
		default:
			break;
		}
	}
	this->html_skip+=html->skip;
	this->html_depth++;
}

static void
gui_internal_html_end(void *dummy, const char *tag_name, void *data, void *error)
{
	struct gui_priv *this=data;
	struct html *html;
	struct html *parent=NULL;

	if (!g_strcasecmp(tag_name,"text"))
		return;
	this->html_depth--;
	html=&this->html[this->html_depth];
	if (this->html_depth > 0)
		parent=&this->html[this->html_depth-1];


	if (!this->html_skip) {
		if (html->command && html->w) {
			html->w->state |= STATE_SENSITIVE;
			html->w->command=html->command;
			html->w->func=gui_internal_html_command;
			html->command=NULL;
		}
		if (parent && (parent->href || parent->command) && html->w) {
			html->w->state |= STATE_SENSITIVE;
			if (parent->command) {
				html->w->command=g_strdup(parent->command);
				html->w->func=gui_internal_html_command;
			} else {
				html->w->command=g_strdup(parent->href);
				html->w->func=gui_internal_html_href;
			}
		}
		switch (html->tag) {
		case html_tag_div:
			this->html_container=html->container;
		case html_tag_img:
		case html_tag_input:
			gui_internal_widget_append(this->html_container, html->w);
			break;
		case html_tag_form:
			this->form=NULL;
			break;
		default:
			break;
		}
	}
	this->html_skip-=html->skip;
	g_free(html->command);
	g_free(html->name);
	g_free(html->href);
	g_free(html->refresh_cond);
}

static void
gui_internal_refresh_callback_called(struct gui_priv *this, struct menu_data *menu_data)
{
	if (gui_internal_menu_data(this) == menu_data) {
		char *href=g_strdup(menu_data->href);
		gui_internal_html_load_href(this, href, 1);
		g_free(href);
	}
}

static void
gui_internal_set_refresh_callback(struct gui_priv *this, char *cond)
{
	dbg(0,"cond=%s\n",cond);
	if (cond) {
		enum attr_type type;
		struct object_func *func;
		struct menu_data *menu_data=gui_internal_menu_data(this);
		dbg(0,"navit=%p\n",this->nav);
		type=command_evaluate_to_attr(&this->self, cond, NULL, &menu_data->refresh_callback_obj);
		if (type == attr_none)
			return;
		func=object_func_lookup(menu_data->refresh_callback_obj.type);
		if (!func || !func->add_attr)
			return;
		menu_data->refresh_callback.type=attr_callback;
		menu_data->refresh_callback.u.callback=callback_new_attr_2(callback_cast(gui_internal_refresh_callback_called),type,this,menu_data);
		func->add_attr(menu_data->refresh_callback_obj.u.data, &menu_data->refresh_callback);
	}
}

static void
gui_internal_html_text(void *dummy, const char *text, int len, void *data, void *error)
{
	struct gui_priv *this=data;
	struct widget *w;
	int depth=this->html_depth-1;
	struct html *html=&this->html[depth];
	gchar *text_stripped;

	if (this->html_skip)
		return;
	while (isspace(text[0])) {
		text++;
		len--;
	}
	while (len > 0 && isspace(text[len-1]))
		len--;

	text_stripped = g_strndup(text, len);
	if (html->tag == html_tag_html && depth > 2) {
		if (this->html[depth-1].tag == html_tag_script) {
			html=&this->html[depth-2];
		}
	}
	switch (html->tag) {
	case html_tag_a:
		if (html->name && len) {
			this->html_container=gui_internal_box_new(this, gravity_center|orientation_horizontal_vertical|flags_expand|flags_fill);
			gui_internal_widget_append(gui_internal_menu(this, _(text_stripped)), this->html_container);
			gui_internal_menu_data(this)->href=g_strdup(this->href);
			gui_internal_set_refresh_callback(this, html->refresh_cond);
			this->html_container->spx=this->spacing*10;
		}
		break;
	case html_tag_h1:
		if (!this->html_container) {
			this->html_container=gui_internal_box_new(this, gravity_center|orientation_horizontal_vertical|flags_expand|flags_fill);
			gui_internal_widget_append(gui_internal_menu(this, _(text_stripped)), this->html_container);
			this->html_container->spx=this->spacing*10;
		}
		break;
	case html_tag_img:
		if (len) {
			w=gui_internal_box_new(this, gravity_center|orientation_vertical);
			gui_internal_widget_append(w, html->w);
			gui_internal_widget_append(w, gui_internal_text_new(this, _(text_stripped), gravity_center|orientation_vertical));
			html->w=w;
		}
		break;
	case html_tag_div:
		if (len) {
			gui_internal_widget_append(html->w, gui_internal_text_new(this, _(text_stripped), gravity_center|orientation_vertical));
		}
		break;
	case html_tag_script:
		dbg(1,"execute %s\n",text_stripped);
		gui_internal_evaluate(this,text_stripped);
		break;
	default:
		break;
	}
	g_free(text_stripped);
}

static void
gui_internal_html_menu(struct gui_priv *this, const char *document, char *anchor)
{
	char *doc=g_strdup(document);
	graphics_draw_mode(this->gra, draw_mode_begin);
	this->html_container=NULL;
	this->html_depth=0;
	this->html_anchor=anchor;
	this->html_anchor_found=0;
	this->form=NULL;
	this->keyboard_required=0;
	this->editable=NULL;
	callback_list_call_attr_2(this->cbl,attr_gui,anchor,&doc);
	xml_parse_text(doc, this, gui_internal_html_start, gui_internal_html_end, gui_internal_html_text);
	g_free(doc);
	if (this->keyboard_required && this->keyboard) {
		this->html_container->flags=gravity_center|orientation_vertical|flags_expand|flags_fill;
		gui_internal_widget_append(this->html_container, gui_internal_keyboard(this,2));
	}
	gui_internal_menu_render(this);
	graphics_draw_mode(this->gra, draw_mode_end);
}


static void
gui_internal_enter(struct gui_priv *this, int ignore)
{
	struct graphics *gra=this->gra;
	if (ignore != -1) 
		this->ignore_button=ignore;

	navit_block(this->nav, 1);
	graphics_overlay_disable(gra, 1);
	this->root.p.x=0;
	this->root.p.y=0;
	this->root.background=this->background;
}

static void
gui_internal_leave(struct gui_priv *this)
{
	graphics_draw_mode(this->gra, draw_mode_end);
}

static void
gui_internal_set_click_coord(struct gui_priv *this, struct point *p)
{
	struct coord c;
	struct coord_geo g;
	struct attr attr;
	struct transformation *trans;
	attr_free(this->click_coord_geo);
	this->click_coord_geo=NULL;
	if (p) {
		trans=navit_get_trans(this->nav);
		transform_reverse(trans, p, &c);
		dbg(1,"x=0x%x y=0x%x\n", c.x, c.y);
		this->clickp.pro=transform_get_projection(trans);
		this->clickp.x=c.x;
		this->clickp.y=c.y;
		transform_to_geo(this->clickp.pro, &c, &g);
		attr.u.coord_geo=&g;
		attr.type=attr_click_coord_geo;
		this->click_coord_geo=attr_dup(&attr);
	}
}

static void
gui_internal_set_position_coord(struct gui_priv *this)
{
	struct transformation *trans;
	struct attr attr,attrp;
	struct coord c;

	attr_free(this->position_coord_geo);
	this->position_coord_geo=NULL;
	if (navit_get_attr(this->nav, attr_vehicle, &attr, NULL) && attr.u.vehicle
		&& vehicle_get_attr(attr.u.vehicle, attr_position_coord_geo, &attrp, NULL)) {
		trans=navit_get_trans(this->nav);
		this->position_coord_geo=attr_dup(&attrp);
		this->vehiclep.pro=transform_get_projection(trans);
		transform_from_geo(this->vehiclep.pro, attrp.u.coord_geo, &c);
		this->vehiclep.x=c.x;
		this->vehiclep.y=c.y;
	}
}

static void
gui_internal_enter_setup(struct gui_priv *this)
{
	if (!this->mouse_button_clicked_on_map)
		gui_internal_set_position_coord(this);
}

static void
gui_internal_html_main_menu(struct gui_priv *this)
{
	gui_internal_prune_menu(this, NULL);
	gui_internal_html_load_href(this, "#Main Menu", 0);
}

static void
gui_internal_cmd_menu(struct gui_priv *this, int ignore, char *href)
{
	dbg(1,"enter\n");
	gui_internal_enter(this, ignore);
	gui_internal_enter_setup(this);
	// draw menu
	if (href)
		gui_internal_html_load_href(this, href, 0);
	else
		gui_internal_html_main_menu(this);
}

static void
gui_internal_cmd_menu2(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	char *href=NULL;
	int i=0, ignore=0, replace=0;

	if (in && in[i] && ATTR_IS_INT(in[i]->type))
		ignore=in[i++]->u.num;

	if (in && in[i] && ATTR_IS_STRING(in[i]->type)) {
		href=in[i++]->u.str;
		if (in[i] && ATTR_IS_INT(in[i]->type))
			replace=in[i++]->u.num;
	}

	if (this->root.children) {
		if (!href)
			return;
		gui_internal_html_load_href(this, href, replace);
		return;
	}
	gui_internal_cmd_menu(this, ignore, href);
}


static void
gui_internal_cmd_log_do(struct gui_priv *this, struct widget *widget)
{
	if (widget->text && strlen(widget->text)) {
		if (this->position_coord_geo)
			navit_textfile_debug_log_at(this->nav, &this->vehiclep, "type=log_entry label=\"%s\"",widget->text);
		else
			navit_textfile_debug_log(this->nav, "type=log_entry label=\"%s\"",widget->text);
	}
	g_free(widget->text);
	widget->text=NULL;
	gui_internal_prune_menu(this, NULL);
	gui_internal_check_exit(this);
}

static void
gui_internal_cmd_log_clicked(struct gui_priv *this, struct widget *widget, void *data)
{
	gui_internal_cmd_log_do(this, widget->data);
}

static void
gui_internal_cmd_log(struct gui_priv *this)
{
	struct widget *w,*wb,*wk,*wl,*we,*wnext;
	gui_internal_enter(this, 1);
	gui_internal_set_click_coord(this, NULL);
	gui_internal_enter_setup(this);
	wb=gui_internal_menu(this, "Log Message");
	w=gui_internal_box_new(this, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(wb, w);
	we=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|flags_fill);
	gui_internal_widget_append(w, we);
	gui_internal_widget_append(we, wk=gui_internal_label_new(this, _("Message")));
	wk->state |= STATE_EDIT|STATE_EDITABLE|STATE_CLEAR;
	wk->background=this->background;
	wk->flags |= flags_expand|flags_fill;
	wk->func = gui_internal_call_linked_on_finish;
	gui_internal_widget_append(we, wnext=gui_internal_image_new(this, image_new_xs(this, "gui_active")));
	wnext->state |= STATE_SENSITIVE;
	wnext->func = gui_internal_cmd_log_clicked;
	wnext->data=wk;
	wk->data=wnext;
	wl=gui_internal_box_new(this, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(w, wl);
	if (this->keyboard)
		gui_internal_widget_append(w, gui_internal_keyboard(this,2));
	gui_internal_menu_render(this);
	gui_internal_leave(this);
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

static int
gui_internal_get_attr(struct gui_priv *this, enum attr_type type, struct attr *attr)
{
	switch (type) {
	case attr_active:
		attr->u.num=this->root.children != NULL;
		break;
	case attr_click_coord_geo:
		if (!this->click_coord_geo)
			return 0;
		*attr=*this->click_coord_geo;
		break;
	case attr_position_coord_geo:
		if (!this->position_coord_geo)
			return 0;
		*attr=*this->position_coord_geo;
		break;
	case attr_pitch:
		attr->u.num=this->pitch;
		break;
	case attr_button:
		attr->u.num=this->mouse_button_clicked_on_map;
		break;
	default:
		return 0;
	}
	attr->type=type;
	return 1;
}

static int
gui_internal_add_attr(struct gui_priv *this, struct attr *attr)
{
	switch (attr->type) {
	case attr_xml_text:
		g_free(this->html_text);
		this->html_text=g_strdup(attr->u.str);
		return 1;
	default:
		return 0;
	}
}

static int
gui_internal_set_attr(struct gui_priv *this, struct attr *attr)
{
	switch (attr->type) {
	case attr_fullscreen:
		if ((this->fullscreen > 0) != (attr->u.num > 0)) {
			graphics_draw_mode(this->gra, draw_mode_end);
			this->win->fullscreen(this->win, attr->u.num > 0);
			graphics_draw_mode(this->gra, draw_mode_begin);
		}
		this->fullscreen=attr->u.num;
		return 1;
	case attr_menu_on_map_click:
		this->menu_on_map_click=attr->u.num;
		return 1;
	case attr_on_map_click:
		g_free(this->on_map_click);
		this->on_map_click=g_strdup(attr->u.str);
		return 1;
	default:
		dbg(0,"%s\n",attr_to_name(attr->type));
		return 1;
	}
}

static void gui_internal_dbus_signal(struct gui_priv *this, struct point *p)
{
	struct displaylist_handle *dlh;
	struct displaylist *display;
	struct displayitem *di;
	struct attr cb,**attr_list=NULL;
	int valid=0;

	display=navit_get_displaylist(this->nav);
	dlh=graphics_displaylist_open(display);
	while ((di=graphics_displaylist_next(dlh))) {
		struct item *item=graphics_displayitem_get_item(di);
		if (item_is_point(*item) && graphics_displayitem_get_displayed(di) &&
			graphics_displayitem_within_dist(display, di, p, this->radius)) {
			struct map_rect *mr=map_rect_new(item->map, NULL);
			struct item *itemo=map_rect_get_item_byid(mr, item->id_hi, item->id_lo);
			struct attr attr;
			if (itemo && item_attr_get(itemo, attr_data, &attr))
				attr_list=attr_generic_add_attr(attr_list, &attr);
			map_rect_destroy(mr);
		}
	}
	graphics_displaylist_close(dlh);
	if (attr_list && navit_get_attr(this->nav, attr_callback_list, &cb, NULL))
		callback_list_call_attr_4(cb.u.callback_list, attr_command, "dbus_send_signal", attr_list, NULL, &valid);
	attr_list_free(attr_list);
}

//##############################################################################################################
//# Description: Convert one geo coordinate in human readable form to double value.
//# Comment:
//# Authors: Martin Bruns (05/2012), mdankov
//##############################################################################################################
static int
gui_internal_coordinate_parse(char *s, char plus, char minus, double *x)
{
	int sign=0;
	char *degree, *minute, *second;
	double tmp;
	
	if(!s)
		return 0;
	
	if (strchr(s, minus)!=NULL) 
		sign=-1; 
	else if (strchr(s, plus)!=NULL) 
		sign=1; 
	
	if(!sign)
		return 0;
	

	/* Can't just use strtok here because ° is multibyte sequence in utf8 */
	degree=s;
	minute=strstr(s,"°");
	if(minute) {
		*minute=0;
		minute+=strlen("°");
	}

	sscanf(degree, "%lf", x);
	
	if(strchr(degree, plus) || strchr(degree, minus)) {
		dbg(3,"degree %c/%c found\n",plus,minus);
	} else {/* DEGREES_MINUTES */
		if(!minute)
			return 0;
		minute = strtok(minute,"'"); 
		sscanf(minute, "%lf", &tmp);
		*x+=tmp/60;
		if(strchr(minute, plus) || strchr(minute, minus)) {
			dbg(3,"minute %c/%c found\n",plus,minus);
		} else { /* DEGREES_MINUTES_SECONDS */
			second=strtok(NULL,"");  
			if(!second)
				return 0;
			sscanf(second, "%lf", &tmp);
			*x+=tmp/3600;
		}		
	}
	*x *= sign;
	return 1;
}

//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Bruns (05/2012)
//##############################################################################################################
static void
gui_internal_cmd_enter_coord_do(struct gui_priv *this, struct widget *widget)
{
	char *lat, *lng;
	char *widgettext;
	double latitude, longitude; 
	dbg(1,"text entered:%s\n", widget->text);

	/* possible entry can be identical to coord_format output but only space between lat and lng is allowed */
	widgettext=g_ascii_strup(widget->text,-1);

	lat=strtok(widgettext," ");
	lng=strtok(NULL,"");

	if(!lat || !lng){
		g_free(widgettext);
		return;
	}
	if( gui_internal_coordinate_parse(lat, 'N', 'S', &latitude) && gui_internal_coordinate_parse(lng, 'E', 'W', &longitude) ) {
		g_free(widgettext);
		widgettext=g_strdup_printf("%lf %lf", longitude, latitude);	
		pcoord_parse(widgettext, projection_mg, &widget->c );
	} else if(!pcoord_parse(widget->text, projection_mg, &widget->c )) {
		g_free(widgettext);
		return;
	}
	g_free(widgettext);
	
	gui_internal_cmd_position(this, widget, (void*)8);
}

//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Bruns (05/2012)
//##############################################################################################################
static void
gui_internal_cmd_enter_coord_clicked(struct gui_priv *this, struct widget *widget, void *data)
{
	dbg(1,"entered\n");
        gui_internal_cmd_enter_coord_do(this, widget->data);
}

//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Bruns (05/2012)
//##############################################################################################################
static void
gui_internal_cmd_enter_coord(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
        struct widget *w, *wb, *wk, *we, *wnext, *row;
/*        gui_internal_enter(this, 1);
        gui_internal_enter_setup(this);*/
        wb=gui_internal_menu(this, _("Enter Coordinates"));
        w=gui_internal_box_new(this, gravity_left_top|orientation_vertical|flags_expand|flags_fill);
        gui_internal_widget_append(wb, w);

        we=gui_internal_box_new(this, gravity_left_center|orientation_horizontal|flags_fill);
        gui_internal_widget_append(w, we);
        gui_internal_widget_append(we, wk=gui_internal_label_new(this, _("Longitude Latitude")));
        wk->state |= STATE_EDIT|STATE_EDITABLE|STATE_CLEAR;
        wk->background=this->background;
        wk->flags |= flags_expand|flags_fill;
        wk->func = gui_internal_call_linked_on_finish;
        gui_internal_widget_append(we, wnext=gui_internal_image_new(this, image_new_xs(this, "gui_active")));
        wnext->state |= STATE_SENSITIVE;
        wnext->func = gui_internal_cmd_enter_coord_clicked; 
        wnext->data=wk;
        wk->data=wnext;
	row=gui_internal_text_new(this, " ", gravity_top_center|flags_fill|orientation_vertical);
	gui_internal_widget_append(w,row);
	row=gui_internal_text_new(this, " ", gravity_top_center|flags_fill|orientation_vertical);
	gui_internal_widget_append(w,row);
	row=gui_internal_text_new(this, _("Enter Coordinates like sample ones below"), gravity_top_center|flags_fill|orientation_vertical);
	gui_internal_widget_append(w,row);
	row=gui_internal_text_new(this, " ", gravity_top_center|flags_fill|orientation_vertical);
	gui_internal_widget_append(w,row);
	row=gui_internal_text_new(this, "52.5219N 19.4127E", gravity_top_center|flags_fill|orientation_vertical);
	gui_internal_widget_append(w,row);
	row=gui_internal_text_new(this, " ", gravity_top_center|flags_fill|orientation_vertical);
	gui_internal_widget_append(w,row);
	row=gui_internal_text_new(this, "52°31.3167N 19°24.7667E", gravity_top_center|flags_fill|orientation_vertical);
	gui_internal_widget_append(w,row);
	row=gui_internal_text_new(this, " ", gravity_top_center|flags_fill|orientation_vertical);
	gui_internal_widget_append(w,row);
	row=gui_internal_text_new(this, "52°31'19N 19°24'46E", gravity_top_center|flags_fill|orientation_vertical);
	gui_internal_widget_append(w,row);
	row=gui_internal_text_new(this, " ", gravity_top_center|flags_fill|orientation_vertical);
	gui_internal_widget_append(w,row);

	if (this->keyboard)
                gui_internal_widget_append(w, gui_internal_keyboard(this,1023));
       gui_internal_menu_render(this);
/*	gui_internal_leave(this);*/
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
	dbg(1,"children=%p ignore_button=%d\n",this->root.children,this->ignore_button);
	if (!this->root.children || this->ignore_button) {

		this->ignore_button=0;
		// check whether the position of the mouse changed during press/release OR if it is the scrollwheel
		if (!navit_handle_button(this->nav, pressed, button, p, NULL)) {
			dbg(1,"navit has handled button\n");
			return;
		}
		dbg(1,"menu_on_map_click=%d\n",this->menu_on_map_click);
		if (button != 1)
			return;
		if (this->on_map_click || this->menu_on_map_click) {
			this->mouse_button_clicked_on_map=1;
			gui_internal_set_click_coord(this, p);
			gui_internal_set_position_coord(this);
			if (this->on_map_click)
				command_evaluate(&this->self, this->on_map_click);
			else
				gui_internal_cmd_menu(this, 0, NULL);
			this->mouse_button_clicked_on_map=0;
		} else if (this->signal_on_map_click) {
			gui_internal_dbus_signal(this, p);
			return;
		}
		return;
	}


	/*
	 * If already in the menu:
	 */

	if (pressed) {
		this->pressed=1;
		this->current=*p;
		gui_internal_gesture_ring_clear(this);
		gui_internal_gesture_ring_add(this, p);
		gui_internal_highlight(this);
	} else {
		int dx,dy;
		gui_internal_gesture_ring_add(this, p);
		gui_internal_gesture_get_vector(this, 300, NULL, &dx, &dy);
		this->current.x=-1;
		this->current.y=-1;
		graphics_draw_mode(gra, draw_mode_begin);
		if(!gui_internal_gesture_do(this) && this->pressed!=2 && abs(dx)<this->icon_s && abs(dy)<this->icon_s)
			gui_internal_call_highlighted(this);
		this->pressed=0;
		if (!event_main_loop_has_quit()) {
			gui_internal_highlight(this);
			graphics_draw_mode(gra, draw_mode_end);
			gui_internal_check_exit(this);
		}
	}
}

static void
gui_internal_setup(struct gui_priv *this)
{
	struct color cbh={0x9fff,0x9fff,0x9fff,0xffff};
	struct color cf={0xbfff,0xbfff,0xbfff,0xffff};
	struct graphics *gra=this->gra;
	unsigned char *buffer;
	char *gui_file;
	int size;

	if (this->background)
		return;
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
	gui_file=g_strjoin(NULL, navit_get_user_data_directory(TRUE), "/gui_internal.txt", NULL);
	if (file_get_contents(gui_file,&buffer,&size)) {
		char *command=g_malloc(size+1);
		strncpy(command,(const char *)buffer,size);
		command[size]=0;
		command_evaluate(&this->self, command);
		g_free(command);
		g_free(buffer);
	}
	g_free(gui_file);
}

//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void gui_internal_resize(void *data, int w, int h)
{
	struct gui_priv *this=data;
	int changed=0;

	gui_internal_setup(this);

	if (this->root.w != w || this->root.h != h) {
		this->root.w=w;
		this->root.h=h;
		changed=1;
	}
	dbg(1,"w=%d h=%d children=%p\n", w, h, this->root.children);
	navit_handle_resize(this->nav, w, h);
	if (this->root.children) {
		if (changed) {
			gui_internal_html_main_menu(this);
		} else {
			gui_internal_menu_render(this);
		}
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
	// Skip hidden elements
	if (wi->p.x==0 && wi->p.y==0 && wi->w==0 && wi->h==0)
		return;
	if ((wi->state & STATE_SENSITIVE) ) {
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
			navit_set_center_screen(this->nav, &p, 1);
			break;
		case NAVIT_KEY_DOWN:
			p.x=w/2;
			p.y=h;
			navit_set_center_screen(this->nav, &p, 1);
			break;
		case NAVIT_KEY_LEFT:
			p.x=0;
			p.y=h/2;
			navit_set_center_screen(this->nav, &p, 1);
			break;
		case NAVIT_KEY_RIGHT:
			p.x=w;
			p.y=h/2;
			navit_set_center_screen(this->nav, &p, 1);
			break;
		case NAVIT_KEY_ZOOM_IN:
			navit_zoom_in(this->nav, 2, NULL);
			break;
		case NAVIT_KEY_ZOOM_OUT:
			navit_zoom_out(this->nav, 2, NULL);
			break;
		case NAVIT_KEY_RETURN:
		case NAVIT_KEY_MENU:
			gui_internal_set_click_coord(this, NULL);
			gui_internal_cmd_menu(this, 0, NULL);
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
	case NAVIT_KEY_BACK:
		if (g_list_length(this->root.children) > 1)
			gui_internal_back(this, NULL, NULL);
		else
			gui_internal_prune_menu(this, NULL);
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
	if (!event_main_loop_has_quit()) {
		graphics_draw_mode(this->gra, draw_mode_end);
		gui_internal_check_exit(this);
	}
}


//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static int gui_internal_set_graphics(struct gui_priv *this, struct graphics *gra)
{
	struct window *win;
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
	this->window_closed_cb=callback_new_attr_1(callback_cast(gui_internal_window_closed), attr_window_closed, this);
	graphics_add_callback(gra, this->window_closed_cb);

	// set fullscreen if needed
	if (this->fullscreen)
		this->win->fullscreen(this->win, this->fullscreen != 0);
	/* Was resize callback already issued? */
	if (navit_get_ready(this->nav) & 2)
		gui_internal_setup(this);
	return 0;
}

static void gui_internal_disable_suspend(struct gui_priv *this)
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
	gui_internal_get_attr,
	gui_internal_add_attr,
	gui_internal_set_attr,
};

	static void
gui_internal_get_data(struct gui_priv *priv, char *command, struct attr **in, struct attr ***out)
{
	struct attr private_data = { attr_private_data, {(void *)&priv->data}};
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

static void
gui_internal_cmd_write(struct gui_priv * this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	char *str=NULL,*str2=NULL;
	dbg(1,"enter %s %p %p %p\n",function,in,out,valid);
	if (!in || !in[0])
		return;
	dbg(1,"%s\n",attr_to_name(in[0]->type));
	if (ATTR_IS_STRING(in[0]->type)) {
		str=in[0]->u.str;
	}
	if (ATTR_IS_COORD_GEO(in[0]->type)) {
		str=str2=coordinates_geo(in[0]->u.coord_geo, '\n');
	}
	if (str) {
		str=g_strdup_printf("<html>%s</html>\n",str);
		xml_parse_text(str, this, gui_internal_html_start, gui_internal_html_end, gui_internal_html_text);
	}
	g_free(str);
	g_free(str2);
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
	widget->state=STATE_SCROLLABLE;
	widget->data=g_new0(struct table_data,1);
	widget->data_free=gui_internal_table_data_free;

	// We have to set background here explicitly
	// because it will be used by inner elements created later in this 
	// function (navigation buttons). Else that elements won't have
	// any background.
	widget->background=this->background;
	data = (struct table_data*)widget->data;

	if (buttons) {
		data->next_button=gui_internal_box_new(this, gravity_center|orientation_horizontal);
		gui_internal_widget_append(data->next_button, gui_internal_text_new(this,_("Next"),gravity_center|orientation_horizontal));
		gui_internal_widget_append(data->next_button, gui_internal_image_new(this, image_new_xs(this, "gui_arrow_right")));
		data->next_button->func=gui_internal_table_button_next;
		data->next_button->data=widget;


		data->prev_button =  gui_internal_button_new_with_callback
			(this, _("Prev"),
			image_new_xs(this, "gui_arrow_left"),
		 	gravity_center |orientation_horizontal,
		 	gui_internal_table_button_prev,NULL);

		data->prev_button->data=widget;

		data->this=this;

		data->button_box=gui_internal_box_new(this,
					      gravity_center|orientation_horizontal);
		gui_internal_widget_append(widget, data->button_box);
		gui_internal_widget_append(data->button_box, data->prev_button);
		gui_internal_widget_append(data->button_box, data->next_button);

		data->button_box->bl=this->spacing;
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
}

/**
 * @brief Check if table has any data rows filled.
 * @param this The graphics context
 * @param table table widget
 * @returns 1 if the table is empty, 0 if there any data rows present.
 */
static int gui_internal_widget_table_is_empty(struct gui_priv *this, struct widget * table)
{
   GList *l;
   struct table_data *td=(struct table_data*) table->data;

   for(l=table->children;l;l=g_list_next(l)) {
   	if(l->data != td->button_box)
   		return 0;
   }

   return 1;
}


/**
 * @brief Move GList pointer to the next table row, skipping other table children (button box, for example).
 * @param row GList pointer into the children list 
 * @returns GList pointer to the next row in the children list, or NULL if there are no any rows left.
 */
static GList * gui_internal_widget_table_next_row(GList * row)
{
  while((row=g_list_next(row))!=NULL) {
   	if(row->data && ((struct widget *)(row->data))->type == widget_table_row)
   		break;
   }
  return row;
}

/**
 * @brief Move GList pointer to the previous table row, skipping other table children (button box, for example).
 * @param row GList pointer into the children list 
 * @returns GList pointer to the previous row in the children list, or NULL if there are no any rows left.
 */
static GList * gui_internal_widget_table_prev_row(GList * row)
{
  while((row=g_list_previous(row))!=NULL) {
   	if(row->data && ((struct widget *)(row->data))->type == widget_table_row)
   		break;
   }
  return row;
}

/**
 * @brief Move GList pointer to the first table row, skipping other table children (button box, for example).
 * @param row GList pointer into the children list 
 * @returns GList pointer to the first row in the children list, or NULL if table is empty.
 */
static GList * gui_internal_widget_table_first_row(GList * row)
{
  if(!row)
  	return NULL;

  if(row->data && ((struct widget *)(row->data))->type == widget_table_row)
  	return row;

  return gui_internal_widget_table_next_row(row);
}

/**
 * @brief Get GList pointer to the table row drawn on the top of the screen.
 * @returns GList pointer to the first row in the children list, or NULL.
 */
static GList * gui_internal_widget_table_top_row(struct gui_priv *this, struct widget * table)
{
	if(table && table->type==widget_table) {
		struct table_data *d=table->data;
		dbg(0,"1\n");
		return gui_internal_widget_table_first_row(d->top_row);
	}
	dbg(0,"2\n");
	return NULL;
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

	w->w = width;
	if(w->w + w->c.x > this->root.w)
	{
		w->w = this->root.w - w->c.x;
	}


	if(w->h + w->c.y   > this->root.h   )
	{
		/**
		 * Do not allow the widget to exceed the screen.
		 *
		 */
		w->h = this->root.h- w->c.y  - height;
	}

	if (table_data->button_box) 
	{
		gui_internal_widget_pack(this,table_data->button_box);
	}


	/**
	 * Deallocate column descriptions.
	 */
	g_list_foreach(column_data,(GFunc)g_free,NULL);
	g_list_free(column_data);
}




/**
 * @brief Invalidates coordinates for previosly rendered table widget rows.
 *
 * @param table_data Data from the table object.
 */
static void gui_internal_table_hide_rows(struct table_data * table_data)
{
	GList*cur_row;
	for(cur_row=table_data->top_row; cur_row ; cur_row = g_list_next(cur_row))
	{
		struct widget * cur_row_widget = (struct widget*)cur_row->data;
		if (cur_row_widget->type!=widget_table_row)
			continue;
		cur_row_widget->p.x=0;
		cur_row_widget->p.y=0;
		cur_row_widget->w=0;
		cur_row_widget->h=0;
		if(cur_row==table_data->bottom_row)
			break;
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

	dbg_assert(table_data);
	column_desc = gui_internal_compute_table_dimensions(this,w);
	if(!column_desc)
		return;
	y=w->p.y;
	gui_internal_table_hide_rows(table_data);
	/**
	 * Skip rows that are on previous pages.
	 */
	cur_row = w->children;
	if(table_data->top_row && table_data->top_row != w->children && !table_data->button_box_hide)
	{
		cur_row = table_data->top_row;
		is_first_page=0;
	} else {
		table_data->top_row=NULL;
	}
	/**
	 * Loop through each row.  Drawing each cell with the proper sizes,
	 * at the proper positions.
	 */
	for(table_data->top_row=cur_row; cur_row; cur_row = g_list_next(cur_row))
	{
		int max_height=0, bbox_height=0;
		struct widget * cur_row_widget;
		GList * cur_column=NULL;
		current_desc = column_desc;
		cur_row_widget = (struct widget*)cur_row->data;
		x =w->p.x+this->spacing;
		if(cur_row_widget == table_data->button_box )
		{
			continue;
		}
		dim = (struct table_column_desc*)current_desc->data;

		if (table_data->button_box && !table_data->button_box_hide)
			bbox_height=table_data->button_box->h;

		if( y + dim->height + bbox_height + this->spacing >= w->p.y + w->h )
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
		
		/* Row object should have its coordinates in actual
		 * state to be able to pass mouse clicks to Column objects
		 */
		cur_row_widget->p.x=w->p.x;
		cur_row_widget->w=w->w;
		cur_row_widget->p.y=y;
		cur_row_widget->h=max_height;
		y = y + max_height;
		table_data->bottom_row=cur_row;
		current_desc = g_list_next(current_desc);
	}
	if(table_data->button_box && (is_skipped || !is_first_page) && !table_data->button_box_hide )
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
	g_list_foreach(column_desc,(GFunc)g_free,NULL);
	g_list_free(column_desc);
}


/**
 * @brief Displays Route information
 *
 * @li The name of the active vehicle
 * @param wm The button that was pressed.
 * @param v Unused
 */
static void
gui_internal_cmd2_route_description(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{


	struct widget * menu;
	struct widget * row;
	struct widget * box;


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


	menu=gui_internal_menu(this,_("Route Description"));

	menu->wfree=gui_internal_route_screen_free;
	this->route_data.route_showing=1;
	this->route_data.route_table->spx = this->spacing;


	box = gui_internal_box_new(this, gravity_left_top| orientation_vertical | flags_fill | flags_expand);

	//	gui_internal_widget_append(box,gui_internal_box_new_with_label(this,"Test"));
	gui_internal_widget_append(box,this->route_data.route_table);
	box->w=menu->w;
	box->spx = this->spacing;
	this->route_data.route_table->w=box->w;
	gui_internal_widget_append(menu,box);
	gui_internal_populate_route_table(this,this->nav);
	gui_internal_menu_render(this);

}

static int
line_intersection(struct coord* a1, struct coord *a2, struct coord * b1, struct coord *b2, struct coord *res)
{
        int n, a, b;
	int adx=a2->x-a1->x;
	int ady=a2->y-a1->y;
	int bdx=b2->x-b1->x;
	int bdy=b2->y-b1->y;
        n = bdy * adx - bdx * ady;
        a = bdx * (a1->y - b1->y) - bdy * (a1->x - b1->x);
        b = adx * (a1->y - b1->y) - ady * (a1->x - b1->x);
        if (n < 0) {
                n = -n;
                a = -a;
                b = -b;
        }
        if (a < 0 || b < 0)
                return 0;
        if (a > n || b > n)
                return 0;
	if (n == 0) {
		dbg(0,"a=%d b=%d n=%d\n", a, b, n);
		dbg(0,"a1=0x%x,0x%x ad %d,%d\n", a1->x, a1->y, adx, ady);
		dbg(0,"b1=0x%x,0x%x bd %d,%d\n", b1->x, b1->y, bdx, bdy);
		dbg_assert(n != 0);
	}
        res->x = a1->x + a * adx / n;
        res->y = a1->y + a * ady / n;
        return 1;
}

struct heightline {
	struct heightline *next;
	int height;
	struct coord_rect bbox;
	int count;
	struct coord c[0];
};

struct diagram_point {
	struct diagram_point *next;
	struct coord c;
};

static struct heightline *
item_get_heightline(struct item *item)
{
	struct heightline *ret=NULL;
	struct street_data *sd;
	struct attr attr;
	int i,height;

	if (item_attr_get(item, attr_label, &attr)) {
		height=atoi(attr.u.str);
		sd=street_get_data(item);
		if (sd && sd->count > 1) {
			ret=g_malloc(sizeof(struct heightline)+sd->count*sizeof(struct coord));
			ret->bbox.lu=sd->c[0];
			ret->bbox.rl=sd->c[0];
			ret->count=sd->count;
			ret->height=height;
			for (i = 0 ; i < sd->count ; i++) {
				ret->c[i]=sd->c[i];
				coord_rect_extend(&ret->bbox, sd->c+i);
			}
		}
		street_data_free(sd);
	}
	return ret;
}


/**
 * @brief Displays Route Height Profile
 *
 * @li The name of the active vehicle
 * @param wm The button that was pressed.
 * @param v Unused
 */
static void
gui_internal_cmd2_route_height_profile(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{


	struct widget * menu, *box;

	struct map * map=NULL;
	struct map_rect * mr=NULL;
	struct route * route;
	struct item * item =NULL;
	struct mapset *ms;
	struct mapset_handle *msh;
	int x,i,first=1,dist=0;
	struct coord c,last,res;
	struct coord_rect rbbox,dbbox;
	struct map_selection sel;
	struct heightline *heightline,*heightlines=NULL;
	struct diagram_point *min,*diagram_point,*diagram_points=NULL;
	sel.next=NULL;
	sel.order=18;
	sel.range.min=type_height_line_1;
	sel.range.max=type_height_line_3;


	menu=gui_internal_menu(this,_("Height Profile"));
	box = gui_internal_box_new(this, gravity_left_top| orientation_vertical | flags_fill | flags_expand);
	gui_internal_widget_append(menu, box);
	route = navit_get_route(this->nav);
	if (route)
		map = route_get_map(route);
	if(map)
		mr = map_rect_new(map,NULL);
	if(mr) {
		while((item = map_rect_get_item(mr))) {
			while (item_coord_get(item, &c, 1)) {
				if (first) {
					first=0;
					sel.u.c_rect.lu=c;
					sel.u.c_rect.rl=c;
				} else
					coord_rect_extend(&sel.u.c_rect, &c);
			}
		}
		map_rect_destroy(mr);
		ms=navit_get_mapset(this->nav);
		if (!first && ms) {
			msh=mapset_open(ms);
			while ((map=mapset_next(msh, 1))) {
				mr=map_rect_new(map, &sel);
				if (mr) {
					while((item = map_rect_get_item(mr))) {
						if (item->type >= sel.range.min && item->type <= sel.range.max) {
							heightline=item_get_heightline(item);
							if (heightline) {
								heightline->next=heightlines;
								heightlines=heightline;
							}
						}
					}
					map_rect_destroy(mr);
				}
			}
			mapset_close(msh);
		}
	}
	map=NULL;
	mr=NULL;
	if (route)
		map = route_get_map(route);
	if(map)
		mr = map_rect_new(map,NULL);
	if(mr && heightlines) {
		while((item = map_rect_get_item(mr))) {
			first=1;
			while (item_coord_get(item, &c, 1)) {
				if (first)
					first=0;
				else {
					heightline=heightlines;
					rbbox.lu=last;
					rbbox.rl=last;
					coord_rect_extend(&rbbox, &c);
					while (heightline) {
						if (coord_rect_overlap(&rbbox, &heightline->bbox)) {
							for (i = 0 ; i < heightline->count - 1; i++) {
								if (heightline->c[i].x != heightline->c[i+1].x || heightline->c[i].y != heightline->c[i+1].y) {
									if (line_intersection(heightline->c+i, heightline->c+i+1, &last, &c, &res)) {
										diagram_point=g_new(struct diagram_point, 1);
										diagram_point->c.x=dist+transform_distance(projection_mg, &last, &res);
										diagram_point->c.y=heightline->height;
										diagram_point->next=diagram_points;
										diagram_points=diagram_point;
										dbg(0,"%d %d\n", diagram_point->c.x, diagram_point->c.y);
									}
								}
							}
						}
						heightline=heightline->next;
					}
					dist+=transform_distance(projection_mg, &last, &c);
				}
				last=c;
			}

		}
		map_rect_destroy(mr);
	}


	gui_internal_menu_render(this);
	first=1;
	diagram_point=diagram_points;
	while (diagram_point) {
		if (first) {
			dbbox.lu=diagram_point->c;
			dbbox.rl=diagram_point->c;
			first=0;
		} else
			coord_rect_extend(&dbbox, &diagram_point->c);
		diagram_point=diagram_point->next;
	}
	dbg(0,"%d %d %d %d\n", dbbox.lu.x, dbbox.lu.y, dbbox.rl.x, dbbox.rl.y);
	if (dbbox.rl.x > dbbox.lu.x && dbbox.lu.x*100/(dbbox.rl.x-dbbox.lu.x) <= 25)
		dbbox.lu.x=0;
	if (dbbox.lu.y > dbbox.rl.y && dbbox.rl.y*100/(dbbox.lu.y-dbbox.rl.y) <= 25)
		dbbox.rl.y=0;
	dbg(0,"%d,%d %dx%d\n", box->p.x, box->p.y, box->w, box->h);
	x=dbbox.lu.x;
	first=1;
	for (;;) {
		struct point p[2];
		min=NULL;
		diagram_point=diagram_points;
		while (diagram_point) {
			if (diagram_point->c.x >= x && (!min || min->c.x > diagram_point->c.x))
				min=diagram_point;
			diagram_point=diagram_point->next;
		}
		if (! min)
			break;
		p[1].x=(min->c.x-dbbox.lu.x)*(box->w-10)/(dbbox.rl.x-dbbox.lu.x)+box->p.x+5;
		p[1].y=(min->c.y-dbbox.rl.y)*(box->h-10)/(dbbox.lu.y-dbbox.rl.y)+box->p.y+5;
		dbg(0,"%d,%d=%d,%d\n",min->c.x, min->c.y, p[1].x,p[1].y);
		graphics_draw_circle(this->gra, this->foreground, &p[1], 2);
		if (first)
			first=0;
		else
			graphics_draw_lines(this->gra, this->foreground, p, 2);
		p[0]=p[1];
		x=min->c.x+1;
	}


}


static void
gui_internal_cmd2_locale(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	struct widget *menu,*wb,*w;
	char *text;

	graphics_draw_mode(this->gra, draw_mode_begin);
	menu=gui_internal_menu(this, _("Show Locale"));
	menu->spx=this->spacing*10;
	wb=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(menu, wb);
	text=g_strdup_printf("LANG=%1$s (1=%3$s 2=%2$s)",getenv("LANG"),"2","1");
	gui_internal_widget_append(wb, w=gui_internal_label_new(this, text));
	w->flags=gravity_left_center|orientation_horizontal|flags_fill;
	g_free(text);
#ifdef HAVE_API_WIN32_BASE
	{
		char country[32],lang[32];
#ifdef HAVE_API_WIN32_CE
		wchar_t wcountry[32],wlang[32];

		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SABBREVLANGNAME, wlang, sizeof(wlang));
		WideCharToMultiByte(CP_ACP,0,wlang,-1,lang,sizeof(lang),NULL,NULL);
#else
		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SABBREVLANGNAME, lang, sizeof(lang));
#endif
		text=g_strdup_printf("LOCALE_SABBREVLANGNAME=%s",lang);
		gui_internal_widget_append(wb, w=gui_internal_label_new(this, text));
		w->flags=gravity_left_center|orientation_horizontal|flags_fill;
		g_free(text);
#ifdef HAVE_API_WIN32_CE
		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SABBREVCTRYNAME, wcountry, sizeof(wcountry));
		WideCharToMultiByte(CP_ACP,0,wcountry,-1,country,sizeof(country),NULL,NULL);
#else
		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SABBREVCTRYNAME, country, sizeof(country));
#endif
		text=g_strdup_printf("LOCALE_SABBREVCTRYNAME=%s",country);
		gui_internal_widget_append(wb, w=gui_internal_label_new(this, text));
		w->flags=gravity_left_center|orientation_horizontal|flags_fill;
		g_free(text);
	}
#endif

	gui_internal_menu_render(this);
	graphics_draw_mode(this->gra, draw_mode_end);
}

static void
gui_internal_cmd2_about(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	struct widget *menu,*wb,*w;
	char *text;

	graphics_draw_mode(this->gra, draw_mode_begin);
	menu=gui_internal_menu(this, _("About Navit"));
	menu->spx=this->spacing*10;
	wb=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand);
	gui_internal_widget_append(menu, wb);

	//Icon
	gui_internal_widget_append(wb, w=gui_internal_image_new(this, image_new_xs(this, "navit")));
	w->flags=gravity_top_center|orientation_horizontal|flags_fill;

	//app name
	text=g_strdup_printf("%s",PACKAGE_NAME);
	gui_internal_widget_append(wb, w=gui_internal_label_new(this, text));
	w->flags=gravity_top_center|orientation_horizontal|flags_expand;
	g_free(text);

	//Version
	text=g_strdup_printf("%s",version);
	gui_internal_widget_append(wb, w=gui_internal_label_new(this, text));
	w->flags=gravity_top_center|orientation_horizontal|flags_expand;
	g_free(text);

	//Site
	text=g_strdup_printf("http://www.navit-project.org/");
	gui_internal_widget_append(wb, w=gui_internal_label_new(this, text));
	w->flags=gravity_top_center|orientation_horizontal|flags_expand;
	g_free(text);

	//Authors
	text=g_strdup_printf("%s:",_("By"));
	gui_internal_widget_append(wb, w=gui_internal_label_new(this, text));
	w->flags=gravity_bottom_center|orientation_horizontal|flags_fill;
	g_free(text);
	text=g_strdup_printf("Martin Schaller");
	gui_internal_widget_append(wb, w=gui_internal_label_new(this, text));
	w->flags=gravity_bottom_center|orientation_horizontal|flags_fill;
	g_free(text);
	text=g_strdup_printf("Michael Farmbauer");
	gui_internal_widget_append(wb, w=gui_internal_label_new(this, text));
	w->flags=gravity_bottom_center|orientation_horizontal|flags_fill;
	g_free(text);
	text=g_strdup_printf("Alexander Atanasov");
	gui_internal_widget_append(wb, w=gui_internal_label_new(this, text));
	w->flags=gravity_bottom_center|orientation_horizontal|flags_fill;
	g_free(text);
	text=g_strdup_printf("Pierre Grandin");
	gui_internal_widget_append(wb, w=gui_internal_label_new(this, text));
	w->flags=gravity_bottom_center|orientation_horizontal|flags_fill;
	g_free(text);

	//Contributors
	text=g_strdup_printf("%s",_("And all the Navit Team"));
	gui_internal_widget_append(wb, w=gui_internal_label_new(this, text));
	w->flags=gravity_bottom_center|orientation_horizontal|flags_fill;
	g_free(text);
	text=g_strdup_printf("%s",_("members and contributors."));
	gui_internal_widget_append(wb, w=gui_internal_label_new(this, text));
	w->flags=gravity_bottom_center|orientation_horizontal|flags_fill;
	g_free(text);

	gui_internal_menu_render(this);
	graphics_draw_mode(this->gra, draw_mode_end);
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
	struct widget * table_widget=NULL;
	struct table_data * table_data = NULL;

	if(wm)
		table_widget = (struct widget * ) wm->data;
	else 
		table_widget = data;

	if(table_widget && table_widget->type==widget_table)
		table_data = (struct table_data*) table_widget->data;

	if(table_data)
	{
		GList *l=g_list_next(table_data->bottom_row);
		if(l)	{
			gui_internal_table_hide_rows(table_data);
			table_data->top_row=l;
		}
	}

	if(wm)
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
	struct widget * table_widget = NULL;
	struct table_data * table_data = NULL;
	
	if(wm)
		table_widget=(struct widget * ) wm->data;
	else 
		table_widget=(struct widget * ) data;

	if(table_widget && table_widget->type==widget_table)
		table_data = (struct table_data*) table_widget->data;

	if(table_data)	{
		int bottomy=table_widget->p.y+table_widget->h;
		int n;
		GList *top=table_data->top_row;
		struct widget *w=(struct widget*)top->data;
		if(table_data->button_box->p.y!=0) {
		    bottomy=table_data->button_box->p.y;
		}
		n=(bottomy-w->p.y)/w->h;
		while(n-- > 0 && (top=g_list_previous(top))!=NULL);
		gui_internal_table_hide_rows(table_data);
		table_data->top_row=top;
	}
	if(wm)
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
	struct coord c;
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
			  row = gui_internal_widget_table_row_new(this,
								  gravity_left
								  | flags_fill
								  | orientation_horizontal);
			  gui_internal_widget_append(this->route_data.route_table,row);

			  label = gui_internal_label_new(this,attr.u.str);
			  gui_internal_widget_append(row,label);

			  label->item=*item;
			  item_coord_get(item, &c, 1);
			  label->c.x=c.x;
			  label->c.y=c.y;
			  label->c.pro=map_projection(map);
			  label->func=gui_internal_cmd_position;
			  label->state|=STATE_SENSITIVE;
			  label->data=(void*)2;	  
			}

		}

	}
}

static struct command_table commands[] = {
	{"abort_navigation",command_cast(gui_internal_cmd2_abort_navigation)},
	{"back",command_cast(gui_internal_cmd2_back)},
	{"back_to_map",command_cast(gui_internal_cmd2_back_to_map)},
	{"bookmarks",command_cast(gui_internal_cmd2_bookmarks)},
	{"formerdests",command_cast(gui_internal_cmd_formerdests)},
	{"get_data",command_cast(gui_internal_get_data)},
	{"locale",command_cast(gui_internal_cmd2_locale)},
	{"log",command_cast(gui_internal_cmd_log)},
	{"menu",command_cast(gui_internal_cmd_menu2)},
	{"position",command_cast(gui_internal_cmd2_position)},
	{"pois",command_cast(gui_internal_cmd2_pois)},
	{"refresh",command_cast(gui_internal_cmd2_refresh)},
	{"route_description",command_cast(gui_internal_cmd2_route_description)},
	{"route_height_profile",command_cast(gui_internal_cmd2_route_height_profile)},
	{"set",command_cast(gui_internal_cmd2_set)},
	{"setting_layout",command_cast(gui_internal_cmd2_setting_layout)},
	{"setting_maps",command_cast(gui_internal_cmd2_setting_maps)},
	{"setting_rules",command_cast(gui_internal_cmd2_setting_rules)},
	{"setting_vehicle",command_cast(gui_internal_cmd2_setting_vehicle)},
	{"town",command_cast(gui_internal_cmd2_town)},
	{"enter_coord",command_cast(gui_internal_cmd_enter_coord)},
	{"quit",command_cast(gui_internal_cmd2_quit)},
	{"write",command_cast(gui_internal_cmd_write)},
	{"about",command_cast(gui_internal_cmd2_about)},

};


//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static struct gui_priv * gui_internal_new(struct navit *nav, struct gui_methods *meth, struct attr **attrs, struct gui *gui)
{
	struct color color_white={0xffff,0xffff,0xffff,0xffff};
	struct color color_black={0x0,0x0,0x0,0xffff};
	struct color back2_color={0x4141,0x4141,0x4141,0xffff};

	struct gui_priv *this;
	struct attr *attr;
	*meth=gui_internal_methods;
	this=g_new0(struct gui_priv, 1);
	this->nav=nav;

	this->self.type=attr_gui;
	this->self.u.gui=gui;

	if ((attr=attr_search(attrs, NULL, attr_menu_on_map_click)))
		this->menu_on_map_click=attr->u.num;
	else
		this->menu_on_map_click=1;

	if ((attr=attr_search(attrs, NULL, attr_on_map_click)))
		this->on_map_click=g_strdup(attr->u.str);

	if ((attr=attr_search(attrs, NULL, attr_signal_on_map_click)))
		this->signal_on_map_click=attr->u.num;

	if ((attr=attr_search(attrs, NULL, attr_callback_list))) {
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
	      this->background_color=color_black;
	if( (attr=attr_search(attrs,NULL,attr_background_color2)))
		this->background2_color=*attr->u.color;
	else
		this->background2_color=back2_color;
	if( (attr=attr_search(attrs,NULL,attr_text_color)))
	      this->text_foreground_color=*attr->u.color;
	else
	      this->text_foreground_color=color_white;
	this->text_background_color=color_black;
	if( (attr=attr_search(attrs,NULL,attr_columns)))
	      this->cols=attr->u.num;
	if( (attr=attr_search(attrs,NULL,attr_osd_configuration)))
	      this->osd_configuration=*attr;

	if( (attr=attr_search(attrs,NULL,attr_pitch)))
	      this->pitch=attr->u.num;
	else
		this->pitch=20;
	if( (attr=attr_search(attrs,NULL,attr_flags_town)))
		this->flags_town=attr->u.num;
	else
		this->flags_town=-1;
	if( (attr=attr_search(attrs,NULL,attr_flags_street)))
		this->flags_street=attr->u.num;
	else
		this->flags_street=-1;
	if( (attr=attr_search(attrs,NULL,attr_flags_house_number)))
		this->flags_house_number=attr->u.num;
	else
		this->flags_house_number=-1;
	if( (attr=attr_search(attrs,NULL,attr_radius)))
		this->radius=attr->u.num;
	else
		this->radius=10;
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

static void
gui_internal_destroy(struct gui_priv *this)
{

	graphics_font_destroy(this->fonts[0]);
	graphics_font_destroy(this->fonts[1]);
	graphics_font_destroy(this->fonts[2]);
	attr_free(this->click_coord_geo);
	attr_free(this->position_coord_geo);
	g_free(this->country_iso2);
	g_free(this->href);
	g_free(this->html_text);
	g_free(this->on_map_click);
	g_free(this);
}
