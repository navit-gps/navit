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
}

static void font_destroy(struct graphics_font_priv *font)
{

}

static struct graphics_font_methods font_methods = {
	font_destroy
};

static struct graphics_font_priv *font_new(struct graphics_priv *gr, struct graphics_font_methods *meth, int size)
{
	*meth=font_methods;
	return &graphics_font_priv;
}

static void
gc_destroy(struct graphics_gc_priv *gc)
{
}

static void
gc_set_linewidth(struct graphics_gc_priv *gc, int w)
{
}

static void
gc_set_dashes(struct graphics_gc_priv *gc, unsigned char *dash_list, int n)
{
}

static void
gc_set_foreground(struct graphics_gc_priv *gc, struct color *c)
{
}

static void
gc_set_background(struct graphics_gc_priv *gc, struct color *c)
{
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
	*meth=gc_methods;
	return &graphics_gc_priv;
}


static struct graphics_image_priv *
image_new(struct graphics_priv *gr, struct graphics_image_methods *meth, char *name, int *w, int *h)
{
	return &graphics_image_priv;
}

static void
draw_lines(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count)
{
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
}

static void
draw_image(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p, struct graphics_image_priv *img)
{
}

static void
draw_image_warp(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p, int count, char *data)
{
}

static void
draw_restore(struct graphics_priv *gr, struct point *p, int w, int h)
{
}

static void
background_gc(struct graphics_priv *gr, struct graphics_gc_priv *gc)
{
}

static void
draw_mode(struct graphics_priv *gr, enum draw_mode_num mode)
{
}

static struct graphics_priv * overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p, int w, int h);

static void *
get_data(struct graphics_priv *this, char *type)
{
	return &dummy;
}



static void
register_resize_callback(struct graphics_priv *this, void (*callback)(void *data, int w, int h), void *data)
{
}

static void
register_motion_callback(struct graphics_priv *this, void (*callback)(void *data, struct point *p), void *data)
{
}

static void
register_button_callback(struct graphics_priv *this, void (*callback)(void *data, int press, int button, struct point *p), void *data)
{
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
	draw_image_warp,
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
};

static struct graphics_priv *
overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p, int w, int h)
{
	*meth=graphics_methods;
	return &graphics_priv;
}


static struct graphics_priv *
graphics_null_new(struct graphics_methods *meth, struct attr **attrs)
{
	*meth=graphics_methods;
	return &graphics_priv;
}

void
plugin_init(void)
{
        plugin_register_graphics_type("null", graphics_null_new);
}
