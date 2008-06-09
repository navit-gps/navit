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
#include <libintl.h>
#include <glib.h>
#include "config.h"
#include "item.h"
#include "navit.h"
#include "debug.h"
#include "gui.h"
#include "coord.h"
#include "point.h"
#include "plugin.h"
#include "graphics.h"
#include "transform.h"
#include "color.h"
#include "config.h"

#define STATE_VISIBLE 1
#define STATE_SELECTED 2
#define STATE_HIGHLIGHTED 4
#define STATE_SENSITIVE 8

enum widget_type {
	widget_box=1,
	widget_button,
	widget_label,
	widget_image,
};

enum gravity {
	gravity_none, gravity_left_top, gravity_top, gravity_left, gravity_center,
};

enum orientation {
	orientation_none, orientation_horizontal, orientation_vertical,
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
	void (*func)(struct gui_priv *priv, struct widget *widget);
	void *data;
	int state;
	struct point p;
	int wmin,hmin;
	int w,h;
	int bl,br,bt,bb,spx,spy;
	enum gravity gravity;
	enum orientation orientation;
	GList *children;
};

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
struct gui_priv {
        struct navit *nav;
	struct graphics *gra;
	struct graphics_gc *background;
	struct graphics_gc *background2;
	struct graphics_gc *highlight_background;
	struct graphics_gc *foreground;
	struct graphics_font *font;
	struct graphics_font *font2;
	int w,h;
	int pressed;
	struct widget *widgets;
	int widgets_count;
	struct widget root;
	struct widget *highlighted;
	struct widget *highlighted_menu;
};

static void gui_internal_widget_render(struct gui_priv *this, struct widget *w);

static struct widget *
gui_internal_button_new_with_callback(struct gui_priv *this, char *text, struct graphics_image *image, enum orientation orientation, void(*func)(struct gui_priv *priv, struct widget *widget), void *data)
{
	struct widget *widget=g_new0(struct widget, 1);

	widget->type=widget_button;
	widget->func=func;
	widget->data=data;
	widget->orientation=orientation;
	if (func)
		widget->state |= STATE_SENSITIVE;
	if (text) {
		widget->text=g_strdup(text);
		widget->h=40;
		widget->w=strlen(text)*20;
	}
	if (text && image) {
		if (orientation == orientation_horizontal)
			widget->w+=10;
		else
			widget->h+=10;
		
	}
	if (image) {
		widget->img=image;
		if (orientation == orientation_horizontal) {
			if (widget->h < widget->img->height)
				widget->h=widget->img->height;
			widget->w+=widget->img->width;
		} else {
			if (widget->w < widget->img->width)
				widget->w=widget->img->width;
			widget->h+=widget->img->height;
		}
	}
#if 0
	widget->foreground_frame=this->foreground;
#endif
	return widget;
}

static struct widget *
gui_internal_button_new(struct gui_priv *this, char *text, struct graphics_image *image, enum orientation orientation)
{
	return gui_internal_button_new_with_callback(this, text, image, orientation, NULL, NULL);
}

static struct widget *
gui_internal_label_new(struct gui_priv *this, char *text)
{
	struct point p[4];

	struct widget *widget=g_new0(struct widget, 1);
	widget->type=widget_label;
	widget->text=g_strdup(text);
	graphics_get_text_bbox(this->gra, this->font2, text, 0x10000, 0x0, p);
	widget->h=40;
	dbg(0,"h=%d\n", widget->h);
	widget->w=p[2].x-p[0].x+8;
	widget->gravity=gravity_center;

	return widget;
}

void
gui_internal_label_render(struct gui_priv *this, struct widget *wi)
{
	struct point pnt=wi->p;
	if (wi->state & STATE_HIGHLIGHTED) 
		graphics_draw_rectangle(this->gra, this->highlight_background, &pnt, wi->w-1, wi->h-1);
	else {
		if (wi->background)
			graphics_draw_rectangle(this->gra, wi->background, &pnt, wi->w-1, wi->h-1);
	}
	pnt.y+=wi->h-10;
	graphics_draw_text(this->gra, this->foreground, NULL, this->font2, wi->text, &pnt, 0x10000, 0x0);
}



//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void gui_internal_button_render(struct gui_priv *this, struct widget *wi)
{
	struct point pnt[5];
	struct point p2[5];
	int i,th=0,tw=0,b=0;
	pnt[0]=wi->p;
	pnt[0].x+=1;
	pnt[0].y+=1;
	if (wi->state & STATE_HIGHLIGHTED) 
		graphics_draw_rectangle(this->gra, this->highlight_background, pnt, wi->w-1, wi->h-1);
	else {
		if (wi->background)
			graphics_draw_rectangle(this->gra, wi->background, pnt, wi->w-1, wi->h-1);
	}
	if (wi->text) {
		th=6;
		tw=strlen(wi->text)*20;
		b=30;
		pnt[0]=wi->p;
		if (wi->orientation == orientation_horizontal) {
			if (wi->img)
				pnt[0].x+=wi->img->hot.x*2+10;
		} else {
			pnt[0].x+=(wi->w-tw)/2;
		}
		pnt[0].y+=wi->h-th;
		graphics_draw_text(this->gra, this->foreground, NULL, this->font2, wi->text, &pnt[0], 0x10000, 0x0);
#if 0
		graphics_get_text_bbox(this->gra, this->font2, wi->text, 0x10000, 0x0, p2);
		for (i = 0 ; i < 4 ; i++) {
			p2[i].x+=pnt[0].x;
			p2[i].y+=pnt[0].y;
		}
		p2[4]=p2[0];
		graphics_draw_lines(this->gra, this->foreground, p2, 5);
#endif
	}

	if (wi->img) {
		if (wi->orientation == orientation_horizontal) {
			pnt[0]=wi->p;
			pnt[0].y+=wi->h/2-wi->img->hot.y;
		} else {
			pnt[0]=wi->p;
			pnt[0].x+=wi->w/2-wi->img->hot.x;
			pnt[0].y+=(wi->h-th-b)/2-wi->img->hot.y;
		}
		graphics_draw_image(this->gra, this->foreground, &pnt[0], wi->img);
	}

	pnt[0]=wi->p;
	pnt[1].x=pnt[0].x+wi->w;
	pnt[1].y=pnt[0].y;
	pnt[2].x=pnt[0].x+wi->w;
	pnt[2].y=pnt[0].y+wi->h;
	pnt[3].x=pnt[0].x;
	pnt[3].y=pnt[0].y+wi->h;
	pnt[4]=pnt[0];
	if (wi->state & STATE_SELECTED) {
		graphics_gc_set_linewidth(this->foreground, 4);
		b=2;
		pnt[0].x+=b;
		pnt[0].y+=b;
		pnt[1].x-=b-1;
		pnt[1].y+=b;
		pnt[2].x-=b-1;
		pnt[2].y-=b-1;
		pnt[3].x+=b;
		pnt[3].y-=b-1;
		pnt[4].x+=b;
		pnt[4].y+=b;
	}
	if (wi->foreground_frame)
		graphics_draw_lines(this->gra, wi->foreground_frame, pnt, 5);
	if (wi->state & STATE_SELECTED) 
		graphics_gc_set_linewidth(this->foreground, 1);
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
	dbg(0,"w=%d h=%d\n", this->w, this->h);
	graphics_draw_rectangle(gra, this->background, &pnt, this->w, this->h);
}

static struct widget *
gui_internal_find_widget(struct widget *wi, struct point *p, int flags)
{
	GList *l=wi->children;
	struct widget *ret,*child;

	if (wi->p.x > p->x || wi->p.y > p->y || wi->p.x + wi->w < p->x || wi->p.y + wi->h < p->y) {
		return NULL;
	}
	if (wi->state & flags) {
		return wi;
	}
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
	}
}

static struct widget *
gui_internal_box_new_with_label(struct gui_priv *this, enum gravity gravity, enum orientation orientation, char *label)
{
	struct widget *widget=g_new0(struct widget, 1);

	if (label) 
		widget->text=g_strdup(label);
	widget->type=widget_box;
	widget->gravity=gravity;
	return widget;
}

static struct widget *
gui_internal_box_new(struct gui_priv *this, enum gravity gravity, enum orientation orientation)
{
	return gui_internal_box_new_with_label(this, gravity, orientation, NULL);
}


static void gui_internal_box_render(struct gui_priv *this, struct widget *w)
{
	struct widget *wc;
	int x,y,width;
	GList *l;
	if (w->background) 
		graphics_draw_rectangle(this->gra, w->background, &w->p, w->w, w->h);
	x=w->p.x+w->bl;
	y=w->p.y+w->bt;
	switch (w->gravity) {
	case gravity_center:
		l=w->children;
		width=0;
		while (l) {
			wc=l->data;
			width+=wc->w+w->spx;
			l=g_list_next(l);
		}
		x=w->p.x+(w->w-width)/2;
	case gravity_left:
		l=w->children;
		while (l) {
			wc=l->data;
			wc->p.x=x;
			wc->p.y=w->p.y+(w->h-wc->h)/2;
			gui_internal_widget_render(this, wc);
			x+=wc->w+w->spx;
			l=g_list_next(l);
		}
		break;
	case gravity_left_top:
		l=w->children;
		while (l) {
			wc=l->data;
			wc->p.x=x;
			wc->p.y=y;
			gui_internal_widget_render(this, wc);
			y+=wc->h+w->spy;
			l=g_list_next(l);
		}
		break;
	case gravity_top:
		l=w->children;
		while (l) {
			wc=l->data;
			wc->p.x=x;
			wc->p.y=w->p.y+(w->h-wc->h)/2;
			gui_internal_widget_render(this, wc);
			x+=wc->w+w->spx;
			l=g_list_next(l);
		}
		break;

	case gravity_none:
		l=w->children;
		while (l) {
			wc=l->data;
			gui_internal_widget_render(this, wc);
			l=g_list_next(l);
		}
	}
}

static void gui_internal_widget_append(struct widget *parent, struct widget *child)
{
	if (! child->background)
		child->background=parent->background;
	parent->children=g_list_append(parent->children, child);
}

static void gui_internal_widget_destroy(struct gui_priv *this, struct widget *w)
{
	GList *l;
	struct widget *wc;

	l=w->children;
	while (l) {
		wc=l->data;
		gui_internal_widget_destroy(this, wc);
		l=g_list_next(l);
	}
	g_free(w->text);
	if (w->img)
		graphics_image_free(this->gra, w->img);
	g_free(w);
}


static void
gui_internal_widget_render(struct gui_priv *this, struct widget *w)
{
	switch (w->type) {
	case widget_box:
		gui_internal_box_render(this, w);
		break;
	case widget_button:
		gui_internal_button_render(this, w);
		break;
	case widget_label:
		gui_internal_label_render(this, w);
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
	this->highlighted->func(this, this->highlighted);
}

static void
gui_internal_prune_menu(struct gui_priv *this, struct widget *w)
{
	GList *l;
	while ((l = g_list_last(this->root.children))) {
		if (l->data == w) {
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
	dbg(0,"enter\n");
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
	struct widget *w,*wm,*wh,*wc,*wcn,*dots;
	GList *res=NULL,*l;
	int width;

	w=gui_internal_box_new(this, gravity_left, orientation_horizontal);
	w->w=this->w;
	w->h=56;
	w->bl=6;
	w->spx=5;	
	w->background=this->background2;
	wm=gui_internal_button_new_with_callback(this, NULL,
		graphics_image_new(this->gra, "xpm/gui_map.svg"), orientation_none,
		gui_internal_cmd_return, NULL);
	wh=gui_internal_button_new_with_callback(this, NULL,
		graphics_image_new(this->gra, "xpm/gui_home.svg"), orientation_none,
		gui_internal_cmd_main_menu, NULL);
	gui_internal_widget_append(w, wm);
	gui_internal_widget_append(w, wh);
	width=w->w-wm->w-wh->w;
	l=g_list_last(this->root.children);
	dots=gui_internal_label_new(this,"..");
	while (l) {
		if (g_list_previous(l) || !g_list_next(l)) {
			wc=l->data;
			wcn=gui_internal_label_new(this, wc->text);
			wcn->func=gui_internal_cmd_return;
			wcn->data=wc;
			wcn->state |= STATE_SENSITIVE;
			res=g_list_prepend(res, wcn);
			if (g_list_previous(l) && g_list_previous(g_list_previous(l)))
				res=g_list_prepend(res, gui_internal_label_new(this, "»"));
		}
		l=g_list_previous(l);
	}
	l=res;
	while (l) {
		gui_internal_widget_append(w, l->data);
		l=g_list_next(l);
	}
	if (dots)
		gui_internal_widget_destroy(this, dots);
#if 0		
	gui_internal_widget_append(w, gui_internal_button_new(this, "Main menu", NULL));
	gui_internal_widget_append(w, gui_internal_button_new(this, "»", NULL));
	gui_internal_widget_append(w, gui_internal_button_new(this, "Main menu", NULL));
#endif
	return w;
}


static void
gui_internal_cmd_rules(struct gui_priv *this, struct widget *wm)
{
	struct widget *menu,*w;
	menu=gui_internal_box_new_with_label(this, gravity_none, orientation_none, "Rules");
	menu->w=this->root.w;
	menu->h=this->root.h;
	gui_internal_widget_append(&this->root, menu);
	gui_internal_clear(this);
	w=gui_internal_top_bar(this);
	gui_internal_widget_append(menu, w);
	w=gui_internal_box_new(this, gravity_left_top, orientation_vertical);
	w->p.y=56;
	w->bl=20;
	w->bt=20;
	w->spy=20;
	w->w=this->w;
	w->h=this->h-56;
	gui_internal_widget_append(menu, w);
	gui_internal_widget_append(w,
		gui_internal_button_new(this, "Stick to roads, obey traffic rules",
			graphics_image_new(this->gra, "xpm/gui_active.svg"), orientation_horizontal));
	gui_internal_widget_append(w,
		gui_internal_button_new(this, "Keep orientation to the North",
			graphics_image_new(this->gra, "xpm/gui_active.svg"), orientation_horizontal));
	gui_internal_widget_append(w,
		gui_internal_button_new(this, "Warn about wrong directions",
			graphics_image_new(this->gra, "xpm/gui_inactive.svg"), orientation_horizontal));
	gui_internal_widget_append(w,
		gui_internal_button_new(this, "Attack defenseless civilians",
			graphics_image_new(this->gra, "xpm/gui_active.svg"), orientation_horizontal));

	graphics_draw_mode(this->gra, draw_mode_begin);
	gui_internal_widget_render(this, menu);
	graphics_draw_mode(this->gra, draw_mode_end);
}
static void
gui_internal_cmd_settings(struct gui_priv *this, struct widget *wm)
{
	struct widget *menu,*w;
	menu=gui_internal_box_new_with_label(this, gravity_none, orientation_none, "Settings");
	menu->w=this->root.w;
	menu->h=this->root.h;
	gui_internal_widget_append(&this->root, menu);
	gui_internal_clear(this);
	w=gui_internal_top_bar(this);
	gui_internal_widget_append(menu, w);
	w=gui_internal_box_new(this, gravity_center, orientation_horizontal);
	w->p.y=56;
	w->spx=20;
	w->w=this->w;
	w->h=this->h-56;
	gui_internal_widget_append(menu, w);
	gui_internal_widget_append(w,
		gui_internal_button_new(this, "Display",
			graphics_image_new_scaled(this->gra, "xpm/gui_display.svg", 96, 96), orientation_vertical));
	gui_internal_widget_append(w,
		gui_internal_button_new(this, "Maps",
			graphics_image_new_scaled(this->gra, "xpm/gui_maps.svg", 96, 96), orientation_vertical));
	gui_internal_widget_append(w,
		gui_internal_button_new(this, "Sound",
			graphics_image_new_scaled(this->gra, "xpm/gui_sound.svg", 96, 96), orientation_vertical));
	gui_internal_widget_append(w,
		gui_internal_button_new_with_callback(this, "Rules",
			graphics_image_new_scaled(this->gra, "xpm/gui_rules.svg", 96, 96), orientation_vertical,
			gui_internal_cmd_rules, NULL));

	graphics_draw_mode(this->gra, draw_mode_begin);
	gui_internal_widget_render(this, menu);
	graphics_draw_mode(this->gra, draw_mode_end);
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
	struct widget *menu,*w;
	menu=gui_internal_box_new_with_label(this, gravity_none, orientation_horizontal, "Main menu");
	menu->w=this->root.w;
	menu->h=this->root.h;
	gui_internal_widget_append(&this->root, menu);
	gui_internal_clear(this);
	w=gui_internal_top_bar(this);
	gui_internal_widget_append(menu, w);
	w=gui_internal_box_new(this, gravity_center, orientation_horizontal);
	w->p.y=56;
	w->spx=20;
	w->w=this->w;
	w->h=this->h-56;
	gui_internal_widget_append(menu, w);
	gui_internal_widget_append(w,
		gui_internal_button_new(this, "Actions",
			graphics_image_new_scaled(this->gra, "xpm/gui_actions.svg", 96, 96), orientation_vertical));
	gui_internal_widget_append(w,
		gui_internal_button_new_with_callback(this, "Settings",
			graphics_image_new_scaled(this->gra, "xpm/gui_settings.svg", 96, 96), orientation_vertical,
			gui_internal_cmd_settings, NULL));
	gui_internal_widget_append(w,
		gui_internal_button_new(this, "Tools",
			graphics_image_new_scaled(this->gra, "xpm/gui_tools.svg", 96, 96), orientation_vertical));

	graphics_draw_mode(this->gra, draw_mode_begin);
	gui_internal_widget_render(this, menu);
	graphics_draw_mode(this->gra, draw_mode_end);
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
	if (!this->root.children) {	
		// check whether the position of the mouse changed during press/release OR if it is the scrollwheel 
		if (!navit_handle_button(this->nav, pressed, button, p, NULL) || button >=4) // Maybe there's a better way to do this
			return;
		// draw menu
		this->root.p.x=0;
		this->root.p.y=0;
		this->root.w=this->w;
		this->root.h=this->h;
		this->root.background=this->background;
		gui_internal_menu_root(this);
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
		if (! this->root.children)
			navit_draw_displaylist(this->nav);
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
	this->w=w;
	this->h=h;
	dbg(0,"w=%d h=%d\n", w, h);
	if (!this->root.children)
		navit_resize(this->nav, w, h);
} 


//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static int gui_internal_set_graphics(struct gui_priv *this, struct graphics *gra)
{
	void *graphics;
#if 0
	struct color cb={0x7fff,0x7fff,0x7fff,0xffff};
#endif
	struct color cb={0x0,0x0,0x0,0xffff};
	struct color cb2={0x4141,0x4141,0x4141,0xffff};
	struct color cbh={0x9fff,0x9fff,0x9fff,0xffff};
	struct color cf={0xbfff,0xbfff,0xbfff,0xffff};
	struct transformation *trans=navit_get_trans(this->nav);
	
	dbg(0,"enter\n");
	graphics=graphics_get_data(gra, "window");
        if (! graphics)
                return 1;
	this->gra=gra;
	transform_get_size(trans, &this->w, &this->h);
	graphics_register_resize_callback(gra, gui_internal_resize, this);
	graphics_register_button_callback(gra, gui_internal_button, this);
	graphics_register_motion_callback(gra, gui_internal_motion, this);
	this->background=graphics_gc_new(gra);
	graphics_gc_set_foreground(this->background, &cb);
	this->background2=graphics_gc_new(gra);
	graphics_gc_set_foreground(this->background2, &cb2);
	this->highlight_background=graphics_gc_new(gra);
	graphics_gc_set_foreground(this->highlight_background, &cbh);
	this->foreground=graphics_gc_new(gra);
	graphics_gc_set_foreground(this->foreground, &cf);
	this->font=graphics_font_new(gra, 200, 1);
	this->font2=graphics_font_new(gra, 545, 1);
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
	*meth=gui_internal_methods;
	this=g_new0(struct gui_priv, 1);
	this->nav=nav;

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
