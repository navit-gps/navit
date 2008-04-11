#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#if !defined(GDK_Book) || !defined(GDK_Calendar)
#include <X11/XF86keysym.h>
#endif
#include <libintl.h>
#include <gtk/gtk.h>
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

struct gui_priv {
        struct navit *nav;
	struct graphics *gra;
	struct graphics_gc *background;
	struct graphics_gc *highlight_background;
	struct graphics_gc *foreground;
	struct graphics_font *font;
	int w,h;
	int menu;
	struct widget *widgets;
	int widgets_count;
};

struct widget {
	int type;
	char *text;
	char *icon;
	void (*func)(struct gui_priv *priv, struct widget *widget);
	void *data;
	int state;
	struct point p;
	int w,h;
};

static void
action_return(struct gui_priv *this, struct widget *wi)
{
	this->menu=0;
	wi->state &= ~STATE_HIGHLIGHTED;
	navit_draw_displaylist(this->nav);
}

static void
action_zoom_in(struct gui_priv *this, struct widget *wi)
{
	this->menu=0;
	wi->state &= ~STATE_HIGHLIGHTED;
	navit_zoom_in(this->nav,2,NULL);
}

static void
action_zoom_out(struct gui_priv *this, struct widget *wi)
{
	this->menu=0;
	wi->state &= ~STATE_HIGHLIGHTED;
	navit_zoom_out(this->nav,2,NULL);
}

struct widget main_menu[] = {
	{0, "Return", "xpm/unknown.xpm",action_return},
	{0, "Ziel", "xpm/flag_bk_wh.xpm"},
	{0, "Tour", "xpm/unknown.xpm" },
	{0, "Fahrzeug", "xpm/cursor.xpm"},
	{0, "Kartenpunkt", "xpm/unknown.xpm" },
	{0, "Karte", "xpm/unknown.xpm",NULL,NULL,STATE_SELECTED},
	{0, "Steckenliste", "xpm/unknown.xpm" },
	{0, "Zoom In", "xpm/unknown.xpm",action_zoom_in},
	{0, "Zoom Out", "xpm/unknown.xpm",action_zoom_out},
};


static void
gui_internal_draw_button(struct gui_priv *this, struct widget *wi)
{
	struct point pnt[5];
	struct graphics_image *img;
	int th=10,tw=40,b=5;
	pnt[0]=wi->p;
	pnt[0].x+=1;
	pnt[0].y+=1;
	if (wi->state & STATE_HIGHLIGHTED) 
		graphics_draw_rectangle(this->gra, this->highlight_background, &pnt, wi->w-1, wi->h-1);
	else
		graphics_draw_rectangle(this->gra, this->background, &pnt, wi->w-1, wi->h-1);
	pnt[0]=wi->p;
	pnt[0].x+=(wi->w-tw)/2;
	pnt[0].y+=wi->h-th-b;
	graphics_draw_text(this->gra, this->foreground, NULL, this->font, wi->text, &pnt[0], 0x10000, 0);

	img=graphics_image_new(this->gra, wi->icon);	
	if (img) {
		pnt[0]=wi->p;
		pnt[0].x+=wi->w/2-img->hot.x;
		pnt[0].y+=(wi->h-th-b)/2-img->hot.y;
		graphics_draw_image(this->gra, this->foreground, &pnt[0], img);
		graphics_image_free(this->gra, img);
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
	graphics_draw_lines(this->gra, this->foreground, pnt, 5);
	if (wi->state & STATE_SELECTED) 
		graphics_gc_set_linewidth(this->foreground, 1);
}

static void
gui_internal_clear(struct gui_priv *this)
{
	struct graphics *gra=this->gra;
	struct point pnt;
	pnt.x=0;
	pnt.y=0;
	dbg(0,"w=%d h=%d\n", this->w, this->h);
	graphics_draw_rectangle(gra, this->background, &pnt, this->w, this->h);
}

static void
gui_internal_render_menu(struct gui_priv *this, int offset, struct widget *wi, int count)
{
	struct graphics *gra=this->gra;
	struct point pnt;
	int w,h,x,y,i;

	dbg(0,"menu\n");
	w=this->w/4;
	h=this->h/4;
	for (i = 0 ; i < count ; i++)
		wi[i].state&=~STATE_VISIBLE;
	i=offset;
	for (y = 0 ; y < this->h ; y+=h) {
		for (x = 0 ; x < this->w ; x+=w) {
			if (i < count) {
				wi[i].p.x=x;
				wi[i].p.y=y;
				wi[i].w=w;
				wi[i].h=h;
				wi[i].state|=STATE_VISIBLE;
				gui_internal_draw_button(this, &wi[i]);
			}
			i++;
		}
	}
	this->widgets=wi;
	this->widgets_count=count;
}

static void
gui_internal_highlight(struct gui_priv *this, struct point *p)
{
	struct widget *wi,*found=NULL;
	int i;

	for (i = 0 ; i < this->widgets_count ; i++) {
		wi=&this->widgets[i];
		if (p && wi->state & STATE_VISIBLE && wi->p.x < p->x && wi->p.y < p->y && wi->p.x + wi->w > p->x && wi->p.y + wi->h > p->y)
			found=wi;	
		else
			if (wi->state & STATE_HIGHLIGHTED) {
				wi->state &= ~STATE_HIGHLIGHTED;
				gui_internal_draw_button(this, wi);
			}
	}
	if (! found)
		return;
	if (! (found->state & STATE_HIGHLIGHTED)) {
		found->state |= STATE_HIGHLIGHTED;
		gui_internal_draw_button(this, found);
	}
}

static void
gui_internal_call_highlighted(struct gui_priv *this)
{
	struct widget *wi;
	int i;

	for (i = 0 ; i < this->widgets_count ; i++) {
		wi=&this->widgets[i];
		if (wi->state & STATE_HIGHLIGHTED) {
			if (wi->func)
				wi->func(this, wi);
		}
	}
}
static void
gui_internal_motion(struct gui_priv *this, struct point *p)
{
	if (!this->menu) 
		navit_handle_motion(this->nav, p);
}

static void
gui_internal_button(struct gui_priv *this, int pressed, int button, struct point *p)
{
	struct graphics *gra=this->gra;
	struct point pnt;
	
	// if still on the map (not in the menu, yet):
	if (!this->menu) {	
		// check whether the position of the mouse changed during press/release OR if it is the scrollwheel 
		if (!navit_handle_button(this->nav, pressed, button, p, NULL) || button >=4)
			return;
		// draw menu
		this->menu++;
		graphics_draw_mode(gra, draw_mode_begin);
		gui_internal_clear(this);
		gui_internal_render_menu(this, 0, main_menu, sizeof(main_menu)/sizeof(struct widget));
		graphics_draw_mode(gra, draw_mode_end);
		return;
	}
	
	// if already in the menu:
	if (pressed) {
		graphics_draw_mode(gra, draw_mode_begin);
		gui_internal_highlight(this, p);
		graphics_draw_mode(gra, draw_mode_end);
	} else {
		graphics_draw_mode(gra, draw_mode_begin);
		gui_internal_highlight(this, p);
		graphics_draw_mode(gra, draw_mode_end);
		gui_internal_call_highlighted(this);
		if (this->menu) {
			graphics_draw_mode(gra, draw_mode_begin);
			gui_internal_highlight(this, NULL);
			graphics_draw_mode(gra, draw_mode_end);
		}
	}
}

static void
gui_internal_resize(struct gui_priv *this, int w, int h)
{
	this->w=w;
	this->h=h;
	dbg(0,"w=%d h=%d\n", w, h);
	if (!this->menu)
		navit_resize(this->nav, w, h);
} 


static int
gui_internal_set_graphics(struct gui_priv *this, struct graphics *gra)
{
	void *graphics;
	struct color cb={0x7fff,0x7fff,0x7fff,0xffff};
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
	this->highlight_background=graphics_gc_new(gra);
	graphics_gc_set_foreground(this->highlight_background, &cbh);
	this->foreground=graphics_gc_new(gra);
	graphics_gc_set_foreground(this->foreground, &cf);
	this->font=graphics_font_new(gra, 200, 1);
	return 0;
}


struct gui_methods gui_internal_methods = {
	NULL,
	NULL,
        gui_internal_set_graphics,
};

static struct gui_priv *
gui_internal_new(struct navit *nav, struct gui_methods *meth, struct attr **attrs) 
{
	struct gui_priv *this;
	*meth=gui_internal_methods;
	this=g_new0(struct gui_priv, 1);
	this->nav=nav;

	return this;
}

void
plugin_init(void)
{
	plugin_register_gui_type("internal", gui_internal_new);
}
