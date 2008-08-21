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

#include <math.h>
#include <glib.h>
#include "config.h"
#include <GL/glc.h>
#include "point.h"
#include "graphics.h"
#include "color.h"
#include "plugin.h"

#include "debug.h"
#include "navit_actor.h"

struct graphics_priv {
	int button_timeout;
	struct point p;
	int width;
	int height;
	int library_init;
	int visible;
	struct graphics_priv *parent;
	struct graphics_priv *overlays;
	struct graphics_priv *next;
	struct graphics_gc_priv *background_gc;
	enum draw_mode_num mode;
	void (*resize_callback)(void *data, int w, int h);
	void *resize_callback_data;
	void (*motion_callback)(void *data, struct point *p);
	void *motion_callback_data;
	void (*button_callback)(void *data, int press, int button, struct point *p);
	void *button_callback_data;
	ClutterActor *testActor;
};

struct graphics_font_priv {
#if 0
        FT_Face face;
#endif
};

struct graphics_gc_priv {
	struct graphics_priv *gr;
	float fr,fg,fb,fa;
	float br,bg,bb,ba;
	int linewidth;
};

struct graphics_image_priv {
	int w;
	int h;
};

static void
graphics_destroy(struct graphics_priv *gr)
{
}


static void font_destroy(struct graphics_font_priv *font)
{
	g_free(font);
	/* TODO: free font->face */
}

static struct graphics_font_methods font_methods = {
	font_destroy
};

static struct graphics_font_priv *font_new(struct graphics_priv *gr, struct graphics_font_methods *meth, char *fontfamily, int size)
{
	return NULL;
}

static void
gc_destroy(struct graphics_gc_priv *gc)
{
	g_free(gc);
}

static void
gc_set_linewidth(struct graphics_gc_priv *gc, int w)
{
	gc->linewidth=w;
}

static void
gc_set_dashes(struct graphics_gc_priv *gc, int width, int offset, unsigned char *dash_list, int n)
{

}


static void
gc_set_foreground(struct graphics_gc_priv *gc, struct color *c)
{
	gc->fr=c->r/65535.0;
	gc->fg=c->g/65535.0;
	gc->fb=c->b/65535.0;
	gc->fa=c->a/65535.0;
// 	printf("new alpha : %i\n",c->a);
}

static void
gc_set_background(struct graphics_gc_priv *gc, struct color *c)
{
	gc->br=c->r/65535.0;
	gc->bg=c->g/65535.0;
	gc->bb=c->b/65535.0;
	gc->ba=c->a/65535.0;
}

static struct graphics_gc_methods gc_methods = {
	gc_destroy,
	gc_set_linewidth,
	gc_set_dashes,	
	gc_set_foreground,	
	gc_set_background	
};

static struct graphics_gc_priv *gc_new(struct graphics_priv *gr, struct graphics_gc_methods *meth)
{
	struct graphics_gc_priv *gc=g_new(struct graphics_gc_priv, 1);

	*meth=gc_methods;
	gc->gr=gr;
	gc->linewidth=1;
	return gc;
}


static struct graphics_image_priv *
image_new(struct graphics_priv *gr, struct graphics_image_methods *meth, char *name, int *w, int *h)
{
	return NULL;
}

static void
draw_lines(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count)
{
	 int i;

	for (i = 0 ; i < count-1 ; i++) {
		dbg(0," Line : [%s,%s] -> [%s,%s]\n",p[i].x,p[i].y,p[i+1].x,p[i+1].y);
	}
}

static void
draw_polygon(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count)
{
	
}

static void
draw_rectangle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int w, int h)
{

}

static void
draw_circle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int r)
{

}

static void
draw_text(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct graphics_gc_priv *bg, struct graphics_font_priv *font, char *text, struct point *p, int dx, int dy)
{
	dbg(0,"Text : %s [%i,%i] %f\n",text,dx,dy,(180*atan2(dx,dy)/3.14));
}

static void
draw_image(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p, struct graphics_image_priv *img)
{

}


static void
overlay_draw(struct graphics_priv *parent, struct graphics_priv *overlay, int window)
{

}

static void
draw_restore(struct graphics_priv *gr, struct point *p, int w, int h)
{
}

static void
background_gc(struct graphics_priv *gr, struct graphics_gc_priv *gc)
{
	gr->background_gc=gc;
}

static void
draw_mode(struct graphics_priv *gr, enum draw_mode_num mode)
{
/*
	if (gr->DLid) {
		if (mode == draw_mode_begin)
			glNewList(gr->DLid,GL_COMPILE);
		if (mode == draw_mode_end)
			glEndList();
	}
*/
}


static struct graphics_priv *
overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p, int w, int h)
{
	return NULL;
}

/**
 * Links the graphics driver to the gui. The gui calls it by 
 * specifying what kind of graphic driver is expected.
 *
 * @param this The graphics instance
 * @param type A string identifying what kind of driver is expected
 * @returns a pointer to the component used to draw in the graphics
 */
static void *
get_data(struct graphics_priv *this, char *type)
{
	if (strcmp(type,"navit_clutter_actor"))
		return NULL;
        return &this->testActor;
}

static void
register_resize_callback(struct graphics_priv *this, void (*callback)(void *data, int w, int h), void *data)
{
	this->resize_callback=callback;
	this->resize_callback_data=data;
}

static void
register_motion_callback(struct graphics_priv *this, void (*callback)(void *data, struct point *p), void *data)
{
	this->motion_callback=callback;
	this->motion_callback_data=data;
}

static void
register_button_callback(struct graphics_priv *this, void (*callback)(void *data, int press, int button, struct point *p), void *data)
{
	this->button_callback=callback;
	this->button_callback_data=data;
}

static struct graphics_methods graphics_methods = {
 	graphics_destroy,
	draw_mode,
	draw_lines,
	draw_polygon,
	draw_rectangle,
	draw_circle,
	draw_text,
	draw_image,
#ifdef HAVE_IMLIB2
	draw_image_warp,
#else
	NULL,
#endif
	draw_restore,
	font_new,
	gc_new,
	background_gc,
	overlay_new,
	image_new,
	get_data,
	register_resize_callback,
	register_button_callback,
	register_motion_callback,
	NULL,	// image_free
};

static struct graphics_priv *
graphics_cogl_new(struct navit *nav, struct graphics_methods *meth, struct attr **attrs)
{
	struct graphics_priv *this=g_new0(struct graphics_priv,1);
	*meth=graphics_methods;

	// Initialize the fonts

	return this;
}

void
plugin_init(void)
{
        plugin_register_graphics_type("cogl", graphics_cogl_new);
}
