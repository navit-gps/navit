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


#include <stdio.h>
#include <glib.h>
#include "config.h"
#include "point.h"
#include "graphics.h"
#include "color.h"
#include "plugin.h"

static int dummy;
static struct graphics_priv {
	int dummy;
} graphics_priv;

static struct graphics_font_priv {
	int dummy;
} graphics_font_priv;

static struct graphics_gc_priv {
	int dummy;
} graphics_gc_priv;

static struct graphics_image_priv {
	int dummy;
} graphics_image_priv;

static void
graphics_destroy(struct graphics_priv *gr)
{
	fprintf(stderr,"graphics_destroy");
}

static void font_destroy(struct graphics_font_priv *font)
{
	fprintf(stderr,"font_destroy\n");

}

static struct graphics_font_methods font_methods = {
	font_destroy
};

static struct graphics_font_priv *font_new(struct graphics_priv *gr, struct graphics_font_methods *meth, int size)
{
	fprintf(stderr,"font_new\n");
	*meth=font_methods;
	return &graphics_font_priv;
}

static void
gc_destroy(struct graphics_gc_priv *gc)
{
	fprintf(stderr,"gc_destroy\n");
}

static void
gc_set_linewidth(struct graphics_gc_priv *gc, int w)
{
	fprintf(stderr,"gc_set_linewidth\n");
}

static void
gc_set_dashes(struct graphics_gc_priv *gc, int w, int offset, unsigned char *dash_list, int n)
{
	fprintf(stderr,"gc_set_dashes\n");
}

static void
gc_set_foreground(struct graphics_gc_priv *gc, struct color *c)
{
	fprintf(stderr,"gc_set_foreground\n");
}

static void
gc_set_background(struct graphics_gc_priv *gc, struct color *c)
{
	fprintf(stderr,"gc_set_background\n");
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
	fprintf(stderr,"gc_new\n");
	*meth=gc_methods;
	return &graphics_gc_priv;
}


static struct graphics_image_priv *
image_new(struct graphics_priv *gr, struct graphics_image_methods *meth, char *name, int *w, int *h)
{
	fprintf(stderr,"image_new\n");
	return &graphics_image_priv;
}

static void
draw_lines(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count)
{
	fprintf(stderr,"draw_lines\n");
}

static void
draw_polygon(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count)
{
	fprintf(stderr,"draw_polygon\n");
}

static void
draw_rectangle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int w, int h)
{
	fprintf(stderr,"draw_rectangle\n");
}

static void
draw_circle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int r)
{
	fprintf(stderr,"draw_circle\n");
}


static void
draw_text(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct graphics_gc_priv *bg, struct graphics_font_priv *font, char *text, struct point *p, int dx, int dy)
{
	fprintf(stderr,"draw_text\n");
}

static void
draw_image(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p, struct graphics_image_priv *img)
{
	fprintf(stderr,"draw_image\n");
}

static void
draw_image_warp(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p, int count, char *data)
{
	fprintf(stderr,"draw_image_warp\n");
}

static void
draw_restore(struct graphics_priv *gr, struct point *p, int w, int h)
{
	fprintf(stderr,"draw_restore\n");
}

static void
background_gc(struct graphics_priv *gr, struct graphics_gc_priv *gc)
{
	fprintf(stderr,"background_gc\n");
}

static void
draw_mode(struct graphics_priv *gr, enum draw_mode_num mode)
{
	fprintf(stderr,"draw_mode\n");
}

static struct graphics_priv * overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p, int w, int h);

static void *
get_data(struct graphics_priv *this, char *type)
{
	fprintf(stderr,"get_data\n");
	return &dummy;
}



static void
register_resize_callback(struct graphics_priv *this, void (*callback)(void *data, int w, int h), void *data)
{
	fprintf(stderr,"register_resize_callback\n");
}

static void
register_motion_callback(struct graphics_priv *this, void (*callback)(void *data, struct point *p), void *data)
{
	fprintf(stderr,"register_motion_callback\n");
}

static void
register_button_callback(struct graphics_priv *this, void (*callback)(void *data, int press, int button, struct point *p), void *data)
{
	fprintf(stderr,"register_button_callback\n");
}

/*
static struct graphics_methods graphics_methods = {
	graphics_destroy,
};
*/
static struct graphics_methods graphics_methods = {
	graphics_destroy,
	draw_mode,
	draw_lines,
	draw_polygon,
	draw_rectangle,
	draw_circle,
	draw_text,
	draw_image,
	draw_image_warp,
	draw_restore,
	font_new,
	gc_new,
	background_gc,
	overlay_new,
	image_new,
	get_data,
	//register_resize_callback,
	//register_button_callback,
	//register_motion_callback,
};

static struct graphics_priv *
overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p, int w, int h)
{
	fprintf(stderr,"overlay_new\n");
	*meth=graphics_methods;
	return &graphics_priv;
}


static struct graphics_priv *
graphics_directfb_new(struct navit *nav, struct graphics_methods *meth, struct attr **attrs)
{
	fprintf(stderr,"graphics_directfb_new");
	*meth=graphics_methods;
	return &graphics_priv;
}

plugin_init(void)
{
	fprintf(stderr,"registering directFB graphics\n");	
        plugin_register_graphics_type("directfb", graphics_directfb_new);
}
