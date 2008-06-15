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
	char *prefix;
	char *name;
	struct coord c;
	int state;
	struct point p;
	int wmin,hmin;
	int w,h;
	int bl,br,bt,bb,spx,spy;
	enum flags flags;
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
	int font_size;
	struct graphics_font *font;
	int icon_large, icon_small;
	int pressed;
	struct widget *widgets;
	int widgets_count;
	struct widget root;
	struct widget *highlighted;
	struct widget *highlighted_menu;
};

static void gui_internal_widget_render(struct gui_priv *this, struct widget *w);
static void gui_internal_widget_pack(struct gui_priv *this, struct widget *w);
static struct widget * gui_internal_box_new(struct gui_priv *this, enum flags flags);
static void gui_internal_widget_append(struct widget *parent, struct widget *child);
static void gui_internal_widget_destroy(struct gui_priv *this, struct widget *w);

static char *
image_path(char *name, int w, int h)
{
	char *full_name;
	full_name=g_strdup_printf("%s/xpm/%s.svg", getenv("NAVIT_SHAREDIR"), name);
	if (file_exists(full_name))
		return full_name;
	full_name=g_strdup_printf("%s/xpm/%s.png", getenv("NAVIT_SHAREDIR"), name);
	if (file_exists(full_name))
		return full_name;
	return NULL;
}

static struct graphics_image *
image_new(struct gui_priv *this, char *name)
{
	char *full_name=image_path(name, -1, -1);
	struct graphics_image *ret=NULL;

	if (full_name);
		ret=graphics_image_new(this->gra, full_name);
	if (! ret)
		dbg(0,"Failed to load %s\n", full_name);
	g_free(full_name);
	return ret;
}


static struct graphics_image *
image_new_scaled(struct gui_priv *this, char *name, int w, int h)
{
	char *full_name=image_path(name, w, h);
	struct graphics_image *ret=NULL;

	if (full_name);
		ret=graphics_image_new_scaled(this->gra, full_name, w, h);
	if (! ret)
		dbg(0,"Failed to load %s\n", full_name);
	g_free(full_name);
	return ret;
}

static struct graphics_image *
image_new_s(struct gui_priv *this, char *name)
{
	return image_new_scaled(this, name, this->icon_small, this->icon_small);
}

static struct graphics_image *
image_new_l(struct gui_priv *this, char *name)
{
	return image_new_scaled(this, name, this->icon_large, this->icon_large);
}

static void
gui_internal_background_render(struct gui_priv *this, struct widget *w)
{
	struct point pnt=w->p;
	if (w->state & STATE_HIGHLIGHTED) 
		graphics_draw_rectangle(this->gra, this->highlight_background, &pnt, w->w-1, w->h-1);
	else {
		if (w->background)
			graphics_draw_rectangle(this->gra, w->background, &pnt, w->w-1, w->h-1);
	}
}
static struct widget *
gui_internal_label_new(struct gui_priv *this, char *text)
{
	struct point p[4];

	struct widget *widget=g_new0(struct widget, 1);
	widget->type=widget_label;
	widget->text=g_strdup(text);
	graphics_get_text_bbox(this->gra, this->font, text, 0x10000, 0x0, p);
	widget->h=40;
	widget->w=p[2].x-p[0].x+8;
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
		dbg(0, "tmp=%s\n", tmp);
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
	pnt=w->p;
	pnt.x+=w->w/2-w->img->hot.x;
	pnt.y+=w->h/2-w->img->hot.y;
	graphics_draw_image(this->gra, this->foreground, &pnt, w->img);
}

static void
gui_internal_label_render(struct gui_priv *this, struct widget *w)
{
	struct point pnt=w->p;
	gui_internal_background_render(this, w);
	pnt.y+=w->h-10;
	graphics_draw_text(this->gra, this->foreground, NULL, this->font, w->text, &pnt, 0x10000, 0x0);
}



static struct widget *
gui_internal_button_new_with_callback(struct gui_priv *this, char *text, struct graphics_image *image, enum flags flags, void(*func)(struct gui_priv *priv, struct widget *widget), void *data)
{
	struct widget *ret=NULL;
	if (text && image) {
		ret=gui_internal_box_new(this, flags);
		gui_internal_widget_append(ret, gui_internal_image_new(this, image));
		gui_internal_widget_append(ret, gui_internal_label_new(this, text));
	} else {
		if (text)
			ret=gui_internal_label_new(this, text);
		if (image)
			ret=gui_internal_image_new(this, image);
		if (! ret)
			ret=gui_internal_box_new(this, flags);
	}
	if (ret) {
		ret->func=func;
		ret->data=data;
		if (func) 
			ret->state |= STATE_SENSITIVE;
	}
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
	int x=0,y=0,width=0,height=0,owidth=0,oheight=0,expand=0;
	GList *l;

	switch (w->flags & 0xffff0000) {
	case orientation_horizontal:
		l=w->children;
		while (l) {
			wc=l->data;
			gui_internal_widget_pack(this, wc);
			if (height < wc->h)
				height=wc->h;
			width+=wc->w;
			if (wc->flags & flags_expand)
				expand+=wc->w;
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
				expand+=wc->h;
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
	switch (w->flags & 0xffff0000) {
	case orientation_horizontal:
		l=w->children;
		while (l) {
			wc=l->data;
			wc->p.x=x;
			if (wc->flags & flags_fill)
				wc->h=w->h;
			if (wc->flags & flags_expand)
				wc->w=wc->w*expand/100;
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
			if (wc->flags & flags_expand)
				wc->h=wc->h*expand/100;
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
	if (w->prefix)
		g_free(w->prefix);
	if (w->name)
		g_free(w->name);
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
	w->bl=6;
	w->spx=5;
	w->background=this->background2;
	wm=gui_internal_button_new_with_callback(this, NULL,
		image_new_s(this, "gui_map"), gravity_center,
		gui_internal_cmd_return, NULL);
	wh=gui_internal_button_new_with_callback(this, NULL,
		image_new_s(this, "gui_home"), gravity_center,
		gui_internal_cmd_main_menu, NULL);
	gui_internal_widget_append(w, wm);
	gui_internal_widget_append(w, wh);
	width=this->root.w-w->bl-wm->w-w->spx-wh->w;
	l=g_list_last(this->root.children);
	wcn=gui_internal_label_new(this,"..");
	dots_len=wcn->w;
	gui_internal_widget_destroy(this, wcn);
	wcn=gui_internal_label_new(this,"»");
	sep_len=wcn->w;
	gui_internal_widget_destroy(this, wcn);
	while (l) {
		if (g_list_previous(l) || !g_list_next(l)) {
			wc=l->data;
			wcn=gui_internal_label_new(this, wc->text);
			if (g_list_previous(l) && g_list_previous(g_list_previous(l))) 
				use_sep=1;
			else
				use_sep=0;
			if (wcn->w + width_used + w->spx + (use_sep ? sep_len : 0) > width) {
				incomplete=1;
				gui_internal_widget_destroy(this, wcn);
				break;
			}
			width_used+=wcn->w;
			wcn->func=gui_internal_cmd_return;
			wcn->data=wc;
			wcn->state |= STATE_SENSITIVE;
			res=g_list_prepend(res, wcn);
			if (use_sep) {
				res=g_list_prepend(res, gui_internal_label_new(this, "»"));
				width_used+=sep_len+w->spx;
			}
			
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
		wcn=gui_internal_label_new(this, "..");
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

static struct widget *
gui_internal_menu(struct gui_priv *this, char *label)
{
	struct widget *menu,*w;
	menu=gui_internal_box_new_with_label(this, gravity_center|orientation_vertical, label);
	menu->w=this->root.w;
	menu->h=this->root.h;
	if (this->root.w > 320 || this->root.h > 320) {
		this->font_size=20;
		this->icon_small=48;
		this->icon_large=96;
		this->font=graphics_font_new(this->gra, 545, 1);
	} else {
		this->font_size=5;
		this->icon_large=24;
		this->icon_small=12;
		this->font=graphics_font_new(this->gra, 136, 1);
	}
	gui_internal_widget_append(&this->root, menu);
	gui_internal_clear(this);
	w=gui_internal_top_bar(this);
	gui_internal_widget_append(menu, w);
	w=gui_internal_box_new(this, gravity_center|orientation_horizontal|flags_expand|flags_fill);
	w->spx=20;
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
	gui_internal_widget_pack(this, menu);
	graphics_draw_mode(this->gra, draw_mode_begin);
	gui_internal_widget_render(this, menu);
	graphics_draw_mode(this->gra, draw_mode_end);
}

static void
gui_internal_cmd_rules(struct gui_priv *this, struct widget *wm)
{
	struct widget *wb,*w;
	wb=gui_internal_menu(this, "Rules");	
	w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(wb, w);
	gui_internal_widget_append(w,
		gui_internal_button_new(this, "Stick to roads, obey traffic rules",
			image_new(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill));
	gui_internal_widget_append(w,
		gui_internal_button_new(this, "Keep orientation to the North",
			image_new(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill));
	gui_internal_widget_append(w,
		gui_internal_button_new(this, "Warn about wrong directions",
			image_new(this, "gui_inactive"), gravity_left_center|orientation_horizontal|flags_fill));
	gui_internal_widget_append(w,
		gui_internal_button_new(this, "Attack defenseless civilians",
			image_new(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill));
	gui_internal_menu_render(this);
}

static void
gui_internal_cmd_position(struct gui_priv *this, struct widget *wm)
{
	struct widget *wb,*w;
	wb=gui_internal_menu(this, wm->text);	
	w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(wb, w);
	gui_internal_widget_append(w,
		gui_internal_button_new(this, "Set as destination",
			image_new(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill));
	gui_internal_widget_append(w,
		gui_internal_button_new(this, "Add to tour",
			image_new(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill));
	gui_internal_widget_append(w,
		gui_internal_button_new(this, "Add as bookmark",
			image_new(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill));
	gui_internal_widget_append(w,
		gui_internal_button_new(this, "View on map",
			image_new(this, "gui_active"), gravity_left_center|orientation_horizontal|flags_fill));
	gui_internal_menu_render(this);
}

static void
gui_internal_cmd_bookmarks(struct gui_priv *this, struct widget *wm)
{
	struct attr attr;
	struct map_rect *mr=NULL;
	struct item *item;
	char *label_full,*l,*prefix,*pos;
	int len,plen,hassub;
	struct widget *wb,*w,*wbm;
	GHashTable *hash;


	wb=gui_internal_menu(this, wm->text ? wm->text : "Bookmarks");
	w=gui_internal_box_new(this, gravity_top_center|orientation_vertical|flags_expand|flags_fill);
	gui_internal_widget_append(wb, w);

	prefix=wm->prefix;
	if (! prefix)
		prefix="";
	plen=strlen(prefix);

	if(navit_get_attr(this->nav, attr_bookmark_map, &attr, NULL) && attr.u.map && (mr=map_rect_new(attr.u.map, NULL))) {
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
						image_new(this, hassub ? "gui_inactive" : "gui_active" ), gravity_left_center|orientation_horizontal|flags_fill,
							hassub ? gui_internal_cmd_bookmarks : gui_internal_cmd_position, NULL);
					if (item_coord_get(item, &wbm->c, 1)) {
						wbm->name=g_strdup(label_full);
						wbm->text=g_strdup(l);
						gui_internal_widget_append(w, wbm);
						g_hash_table_insert(hash, g_strdup(l), (void *)1);
						wbm->prefix=g_malloc(plen+len+2);
						strncpy(wbm->prefix, label_full, plen+len+1);
						wbm->prefix[plen+len+1]='\0';
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

static void
gui_internal_cmd_actions(struct gui_priv *this, struct widget *wm)
{
	struct widget *w;

	w=gui_internal_menu(this, "Actions");	
	gui_internal_widget_append(w,
		gui_internal_button_new_with_callback(this, "Bookmarks",
			image_new_l(this, "gui_bookmark"), gravity_center|orientation_vertical,
			gui_internal_cmd_bookmarks, NULL));
	gui_internal_menu_render(this);
}

static void
gui_internal_cmd_settings(struct gui_priv *this, struct widget *wm)
{
	struct widget *w;

	w=gui_internal_menu(this, "Settings");	
	gui_internal_widget_append(w,
		gui_internal_button_new(this, "Display",
			image_new_l(this, "gui_display"), gravity_center|orientation_vertical));
	gui_internal_widget_append(w,
		gui_internal_button_new(this, "Maps",
			image_new_l(this, "gui_maps"), gravity_center|orientation_vertical));
	gui_internal_widget_append(w,
		gui_internal_button_new(this, "Sound",
			image_new_l(this, "gui_sound"), gravity_center|orientation_vertical));
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

	w=gui_internal_menu(this, "Main menu");	
	gui_internal_widget_append(w, gui_internal_button_new_with_callback(this, "Actions",
			image_new_l(this, "gui_actions"), gravity_center|orientation_vertical,
			gui_internal_cmd_actions, NULL));
	gui_internal_widget_append(w, gui_internal_button_new_with_callback(this, "Settings",
			image_new_l(this, "gui_settings"), gravity_center|orientation_vertical,
			gui_internal_cmd_settings, NULL));
	gui_internal_widget_append(w, gui_internal_button_new(this, "Tools",
			image_new_l(this, "gui_tools"), gravity_center|orientation_vertical));
	gui_internal_menu_render(this);
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
	this->root.w=w;
	this->root.h=h;
	dbg(0,"w=%d h=%d\n", w, h);
	if (!this->root.children) {
		navit_resize(this->nav, w, h);
		return;
	}
	gui_internal_prune_menu(this, NULL);
	gui_internal_menu_root(this);
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
	
	graphics=graphics_get_data(gra, "window");
        if (! graphics)
                return 1;
	this->gra=gra;
	transform_get_size(trans, &this->root.w, &this->root.h);
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
