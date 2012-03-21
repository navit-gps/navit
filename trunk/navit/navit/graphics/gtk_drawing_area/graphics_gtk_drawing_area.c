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

#define GDK_ENABLE_BROKEN
#include "config.h"
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <locale.h> /* For WIN32 */
#if !defined(GDK_Book) || !defined(GDK_Calendar)
#include <X11/XF86keysym.h>
#endif
#ifdef HAVE_IMLIB2
#include <Imlib2.h>
#endif

#ifndef _WIN32
#include <gdk/gdkx.h>
#endif
#include "event.h"
#include "debug.h"
#include "point.h"
#include "graphics.h"
#include "color.h"
#include "item.h"
#include "window.h"
#include "callback.h"
#include "keys.h"
#include "plugin.h"
#include "navit/font/freetype/font_freetype.h"
#include "navit.h"

#ifndef GDK_Book
#define GDK_Book XF86XK_Book
#endif
#ifndef GDK_Calendar
#define GDK_Calendar XF86XK_Calendar
#endif


struct graphics_priv {
	GdkEventButton button_event;
	int button_timeout;
	GtkWidget *widget;
	GtkWidget *win;
	struct window window;
	GdkDrawable *drawable;
	GdkDrawable *background;
	int background_ready;
	GdkColormap *colormap;
	struct point p;
	struct point pclean;
	int cleanup;
	int width;
	int height;
	int win_w;
	int win_h;
	int visible;
	int overlay_disabled;
	int overlay_autodisabled;
	int a;
	int wraparound;
	struct graphics_priv *parent;
	struct graphics_priv *overlays;
	struct graphics_priv *next;
	struct graphics_gc_priv *background_gc;
	enum draw_mode_num mode;
	struct callback_list *cbl;
	struct font_freetype_methods freetype_methods;
	struct navit *nav;
	int pid;
	struct timeval button_press[8];
	struct timeval button_release[8];
	int timeout;
	int delay;
};


struct graphics_gc_priv {
	GdkGC *gc;
	GdkPixmap *pixmap;
	struct graphics_priv *gr;
	struct color c;
};

struct graphics_image_priv {
	GdkPixbuf *pixbuf;
	int w;
	int h;
};

static GHashTable *hImageData;   /*hastable for uncompressed image data*/
static int hImageDataCount;
static struct graphics_image_priv image_error;

static void
graphics_destroy_image(gpointer data)
{
	struct graphics_image_priv *priv = (struct graphics_image_priv*)data;

	if (priv == &image_error)
		return;

	if (priv->pixbuf)
		g_object_unref(priv->pixbuf);
	g_free(priv);
}

static void
graphics_destroy(struct graphics_priv *gr)
{
	dbg(0,"enter parent %p\n",gr->parent);
	if (!gr->parent) {
		dbg(0,"enter win %p\n",gr->win);
		if (gr->win)
			gtk_widget_destroy(gr->win);
		dbg(0,"widget %p\n",gr->widget);
		if (gr->widget)
			gtk_widget_destroy(gr->widget);
	}
	dbg(0,"hImageDataCount %d\n",hImageDataCount);
	if (!--hImageDataCount)
		g_hash_table_destroy(hImageData);
	g_free(gr);
}

static void
gc_destroy(struct graphics_gc_priv *gc)
{
	g_object_unref(gc->gc);
	g_free(gc);
}

static void
gc_set_linewidth(struct graphics_gc_priv *gc, int w)
{
	gdk_gc_set_line_attributes(gc->gc, w, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
}

static void
gc_set_dashes(struct graphics_gc_priv *gc, int w, int offset, unsigned char *dash_list, int n)
{
	gdk_gc_set_dashes(gc->gc, offset, (gint8 *)dash_list, n);
	gdk_gc_set_line_attributes(gc->gc, w, GDK_LINE_ON_OFF_DASH, GDK_CAP_ROUND, GDK_JOIN_ROUND);
}

static void
gc_set_color(struct graphics_gc_priv *gc, struct color *c, int fg)
{
	GdkColor gdkc;
	gdkc.pixel=0;
	gdkc.red=c->r;
	gdkc.green=c->g;
	gdkc.blue=c->b;
	gdk_colormap_alloc_color(gc->gr->colormap, &gdkc, FALSE, TRUE);
	gdk_colormap_query_color(gc->gr->colormap, gdkc.pixel, &gdkc);
	gc->c=*c;
	if (fg) {
		gdk_gc_set_foreground(gc->gc, &gdkc);
	} else
		gdk_gc_set_background(gc->gc, &gdkc);
}

static void
gc_set_foreground(struct graphics_gc_priv *gc, struct color *c)
{
	gc_set_color(gc, c, 1);
}

static void
gc_set_background(struct graphics_gc_priv *gc, struct color *c)
{
	gc_set_color(gc, c, 0);
}

static void
gc_set_stipple(struct graphics_gc_priv *gc, struct graphics_image_priv *img)
{
	char data[2]={0x2,0x1};
	gdk_gc_set_fill(gc->gc, GDK_STIPPLED);
	gc->pixmap=gdk_bitmap_create_from_data(gc->gr->widget->window, data, 2, 2);
	gdk_gc_set_stipple(gc->gc, gc->pixmap);
}

static struct graphics_gc_methods gc_methods = {
	gc_destroy,
	gc_set_linewidth,
	gc_set_dashes,
	gc_set_foreground,
	gc_set_background,
	gc_set_stipple,
};

static struct graphics_gc_priv *gc_new(struct graphics_priv *gr, struct graphics_gc_methods *meth)
{
	struct graphics_gc_priv *gc=g_new(struct graphics_gc_priv, 1);

	*meth=gc_methods;
	gc->gc=gdk_gc_new(gr->widget->window);
	gc->gr=gr;
	return gc;
}


static struct graphics_image_priv *
image_new(struct graphics_priv *gr, struct graphics_image_methods *meth, char *name, int *w, int *h, struct point *hot, int rotation)
{
	GdkPixbuf *pixbuf;
	struct graphics_image_priv *ret;
	const char *option;

	char* hash_key = g_strdup_printf("%s_%d_%d_%d",name,*w,*h,rotation);

	//check if image already exists in hashmap
	struct graphics_image_priv *curr_elem = g_hash_table_lookup(hImageData, hash_key);
	if(curr_elem == &image_error) {
		//found but couldn't be loaded
		g_free(hash_key);
		return NULL;
	}
	else if(curr_elem) {
		//found and OK -> use hashtable entry
		g_free(hash_key);
		*w = curr_elem->w;
		*h = curr_elem->h;
		hot->x = curr_elem->w / 2 - 1;
		hot->y = curr_elem->h / 2 - 1;
		ret=g_new0(struct graphics_image_priv, 1);
		*ret = *curr_elem;
		g_object_ref(ret->pixbuf);
		return ret;
	}
	else {
		if (*w == -1 && *h == -1)
			pixbuf=gdk_pixbuf_new_from_file(name, NULL);
		else
			pixbuf=gdk_pixbuf_new_from_file_at_size(name, *w, *h, NULL);

		if (!pixbuf) {
			g_hash_table_insert(hImageData, g_strdup(hash_key), &image_error);
			g_free(hash_key);
			return NULL;
		}

		if (rotation) {
			GdkPixbuf *tmp;
			switch (rotation) {
				case 90:
					rotation=270;
					break;
				case 180:
					break;
				case 270:
					rotation=90;
					break;
				default:
					g_hash_table_insert(hImageData, g_strdup(hash_key), &image_error);
					g_free(hash_key);
					return NULL;
			}

			tmp=gdk_pixbuf_rotate_simple(pixbuf, rotation);

			if (!tmp) {
				g_hash_table_insert(hImageData, g_strdup(hash_key), &image_error);
				g_free(hash_key);
				g_object_unref(pixbuf);
				return NULL;
			}

			g_object_unref(pixbuf);
			pixbuf=tmp;
		}

		ret=g_new0(struct graphics_image_priv, 1);
		ret->pixbuf=pixbuf;
		ret->w=gdk_pixbuf_get_width(pixbuf);
		ret->h=gdk_pixbuf_get_height(pixbuf);
		*w=ret->w;
		*h=ret->h;
		if (hot) {
			option=gdk_pixbuf_get_option(pixbuf, "x_hot");
			if (option)
				hot->x=atoi(option);
			else
				hot->x=ret->w/2-1;
			option=gdk_pixbuf_get_option(pixbuf, "y_hot");
			if (option)
				hot->y=atoi(option);
			else
				hot->y=ret->h/2-1;
		}
		struct graphics_image_priv *cached = g_new0(struct graphics_image_priv, 1);
		*cached = *ret;
		g_hash_table_insert(hImageData, g_strdup(hash_key), cached);
		g_object_ref(pixbuf);
		g_free(hash_key);
		return ret;
	}
}

static void 
image_free(struct graphics_priv *gr, struct graphics_image_priv *priv)
{
	g_object_unref(priv->pixbuf);
	g_free(priv);
}

static void
draw_lines(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count)
{
	if (gr->mode == draw_mode_begin || gr->mode == draw_mode_end)
		gdk_draw_lines(gr->drawable, gc->gc, (GdkPoint *)p, count);
	if (gr->mode == draw_mode_end || gr->mode == draw_mode_cursor)
		gdk_draw_lines(gr->widget->window, gc->gc, (GdkPoint *)p, count);
}

static void
draw_polygon(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count)
{
	if (gr->mode == draw_mode_begin || gr->mode == draw_mode_end)
		gdk_draw_polygon(gr->drawable, gc->gc, TRUE, (GdkPoint *)p, count);
	if (gr->mode == draw_mode_end || gr->mode == draw_mode_cursor)
		gdk_draw_polygon(gr->widget->window, gc->gc, TRUE, (GdkPoint *)p, count);
}

static void
draw_rectangle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int w, int h)
{
	gdk_draw_rectangle(gr->drawable, gc->gc, TRUE, p->x, p->y, w, h);
}

static void
draw_circle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int r)
{
	if (gr->mode == draw_mode_begin || gr->mode == draw_mode_end)
		gdk_draw_arc(gr->drawable, gc->gc, FALSE, p->x-r/2, p->y-r/2, r, r, 0, 64*360);
	if (gr->mode == draw_mode_end || gr->mode == draw_mode_cursor)
		gdk_draw_arc(gr->widget->window, gc->gc, FALSE, p->x-r/2, p->y-r/2, r, r, 0, 64*360);
}

static void
display_text_draw(struct font_freetype_text *text, struct graphics_priv *gr, struct graphics_gc_priv *fg, struct graphics_gc_priv *bg, int color, struct point *p)
{
	int i,x,y,stride;
	struct font_freetype_glyph *g, **gp;
	unsigned char *shadow,*glyph;
	struct color transparent={0x0,0x0,0x0,0x0};
	struct color white={0xffff,0xffff,0xffff,0xffff};

	gp=text->glyph;
	i=text->glyph_count;
	x=p->x << 6;
	y=p->y << 6;
	while (i-- > 0)
	{
		g=*gp++;
		if (g->w && g->h && bg ) {
#if 1
			stride=g->w+2;
			shadow=g_malloc(stride*(g->h+2));
			if (gr->freetype_methods.get_shadow(g, shadow, 8, stride, &white, &transparent))
				gdk_draw_gray_image(gr->drawable, bg->gc, ((x+g->x)>>6)-1, ((y+g->y)>>6)-1, g->w+2, g->h+2, GDK_RGB_DITHER_NONE, shadow, stride);
			g_free(shadow);
			if (color) {
				stride*=3;
				shadow=g_malloc(stride*(g->h+2));
				gr->freetype_methods.get_shadow(g, shadow, 24, stride, &bg->c, &transparent);
				gdk_draw_rgb_image(gr->drawable, fg->gc, ((x+g->x)>>6)-1, ((y+g->y)>>6)-1, g->w+2, g->h+2, GDK_RGB_DITHER_NONE, shadow, stride);
				g_free(shadow);
			} 
#else
			GdkImage *image;
			stride=(g->w+9)/8;
			shadow=malloc(stride*(g->h+2));
			
			gr->freetype_methods.get_shadow(g, shadow, 1, stride);
			image=gdk_image_new_bitmap(gdk_visual_get_system(),shadow,g->w+2, g->h+2);
			gdk_draw_image(gr->drawable, bg->gc, image, 0, 0, ((x+g->x)>>6)-1, ((y+g->y)>>6)-1, g->w+2, g->h+2);
			g_object_unref(image);
#endif
			
		}
		x+=g->dx;
		y+=g->dy;
	}
	x=p->x << 6;
	y=p->y << 6;
	gp=text->glyph;
	i=text->glyph_count;
	while (i-- > 0)
	{
		g=*gp++;
		if (g->w && g->h) {
			if (color) {
				stride=g->w;
				if (bg) {
					glyph=g_malloc(stride*g->h);
					gr->freetype_methods.get_glyph(g, glyph, 8, stride, &fg->c, &bg->c, &transparent);
					gdk_draw_gray_image(gr->drawable, bg->gc, (x+g->x)>>6, (y+g->y)>>6, g->w, g->h, GDK_RGB_DITHER_NONE, glyph, g->w);
					g_free(glyph);
				}
				stride*=3;
				glyph=g_malloc(stride*g->h);
				gr->freetype_methods.get_glyph(g, glyph, 24, stride, &fg->c, bg?&bg->c:&transparent, &transparent);
				gdk_draw_rgb_image(gr->drawable, fg->gc, (x+g->x)>>6, (y+g->y)>>6, g->w, g->h, GDK_RGB_DITHER_NONE, glyph, stride);
				g_free(glyph);
			} else
				gdk_draw_gray_image(gr->drawable, fg->gc, (x+g->x)>>6, (y+g->y)>>6, g->w, g->h, GDK_RGB_DITHER_NONE, g->pixmap, g->w);
		}
		x+=g->dx;
		y+=g->dy;
	}
}

static void
draw_text(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct graphics_gc_priv *bg, struct graphics_font_priv *font, char *text, struct point *p, int dx, int dy)
{
	struct font_freetype_text *t;
	int color=0;

	if (! font)
	{
		dbg(0,"no font, returning\n");
		return;
	}
#if 0 /* Temporarily disabled because it destroys text rendering of overlays and in gui internal in some places */
	/* 
	 This needs an improvement, no one checks if the strings are visible
	*/
	if (p->x > gr->width-50 || p->y > gr->height-50) {
		return;
	}
	if (p->x < -50 || p->y < -50) {
		return;
	}
#endif

	if (bg) {
		if (COLOR_IS_BLACK(fg->c) && COLOR_IS_WHITE(bg->c)) {
			gdk_gc_set_function(fg->gc, GDK_AND_INVERT);
			gdk_gc_set_function(bg->gc, GDK_OR);
		} else if (COLOR_IS_WHITE(fg->c) && COLOR_IS_BLACK(bg->c)) {
			gdk_gc_set_function(fg->gc, GDK_OR);
			gdk_gc_set_function(bg->gc, GDK_AND_INVERT);
		} else  {
			gdk_gc_set_function(fg->gc, GDK_OR);
			gdk_gc_set_function(bg->gc, GDK_AND_INVERT);
			color=1;
		}
	} else {
		gdk_gc_set_function(fg->gc, GDK_OR);
		color=1;
	}
	t=gr->freetype_methods.text_new(text, (struct font_freetype_font *)font, dx, dy);
	display_text_draw(t, gr, fg, bg, color, p);
	gr->freetype_methods.text_destroy(t);
	gdk_gc_set_function(fg->gc, GDK_COPY);
	if (bg)
		gdk_gc_set_function(bg->gc, GDK_COPY);
#if 0
	{
		struct point pnt[5];
		int i;
		gr->freetype_methods.get_text_bbox(gr, font, text, dx, dy, pnt, 1);
		for (i = 0 ; i < 4 ; i++) {
			pnt[i].x+=p->x;
			pnt[i].y+=p->y;
		}
		pnt[4]=pnt[0];
		gdk_draw_lines(gr->drawable, fg->gc, (GdkPoint *)pnt, 5);
	}
#endif
}

static void
draw_image(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p, struct graphics_image_priv *img)
{
	gdk_draw_pixbuf(gr->drawable, fg->gc, img->pixbuf, 0, 0, p->x, p->y,
		    img->w, img->h, GDK_RGB_DITHER_NONE, 0, 0);
}

#ifdef HAVE_IMLIB2
static void
draw_image_warp(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p, int count, char *data)
{
	void *image;
	int w,h;
	dbg(1,"draw_image_warp data=%s\n", data);
	image = imlib_load_image(data);
	imlib_context_set_display(gdk_x11_drawable_get_xdisplay(gr->widget->window));
	imlib_context_set_colormap(gdk_x11_colormap_get_xcolormap(gtk_widget_get_colormap(gr->widget)));
	imlib_context_set_visual(gdk_x11_visual_get_xvisual(gtk_widget_get_visual(gr->widget)));
	imlib_context_set_drawable(gdk_x11_drawable_get_xid(gr->drawable));
	imlib_context_set_image(image);
	w = imlib_image_get_width();
	h = imlib_image_get_height();
	if (count == 3) {
		/* 0 1
        	   2   */
		imlib_render_image_on_drawable_skewed(0, 0, w, h, p[0].x, p[0].y, p[1].x-p[0].x, p[1].y-p[0].y, p[2].x-p[0].x, p[2].y-p[0].y);
	}
	if (count == 2) {
		/* 0 
        	     1 */
		imlib_render_image_on_drawable_skewed(0, 0, w, h, p[0].x, p[0].y, p[1].x-p[0].x, 0, 0, p[1].y-p[0].y);
	}
	if (count == 1) {
		/* 
                   0 
        	     */
		imlib_render_image_on_drawable_skewed(0, 0, w, h, p[0].x-w/2, p[0].y-h/2, w, 0, 0, h);
	}
	imlib_free_image();
}
#endif

static void
overlay_rect(struct graphics_priv *parent, struct graphics_priv *overlay, int clean, GdkRectangle *r)
{
	if (clean) {
		r->x=overlay->pclean.x;
		r->y=overlay->pclean.y;
	} else {
		r->x=overlay->p.x;
		r->y=overlay->p.y;
	}
	r->width=overlay->width;
	r->height=overlay->height;
	if (!overlay->wraparound)
		return;
	if (r->x < 0)
		r->x += parent->width;
	if (r->y < 0)
		r->y += parent->height;
	if (r->width < 0)
		r->width += parent->width;
	if (r->height < 0)
		r->height += parent->height;
}

static void
overlay_draw(struct graphics_priv *parent, struct graphics_priv *overlay, GdkRectangle *re, GdkPixmap *pixmap, GdkGC *gc)
{
	GdkPixbuf *pixbuf,*pixbuf2;
	guchar *pixels1, *pixels2, *p1, *p2, r=0, g=0, b=0, a=0;
	int x,y;
	int rowstride1,rowstride2;
	int n_channels1,n_channels2;
	GdkRectangle or,ir;
	struct graphics_gc_priv *bg=overlay->background_gc;
	if (bg) {
		r=bg->c.r>>8;
		g=bg->c.g>>8;
		b=bg->c.b>>8;
		a=bg->c.a>>8;
	}

	if (parent->overlay_disabled || overlay->overlay_disabled || overlay->overlay_autodisabled)
		return;
	dbg(1,"r->x=%d r->y=%d r->width=%d r->height=%d\n", re->x, re->y, re->width, re->height);
	overlay_rect(parent, overlay, 0, &or);
	dbg(1,"or.x=%d or.y=%d or.width=%d or.height=%d\n", or.x, or.y, or.width, or.height);
	if (! gdk_rectangle_intersect(re, &or, &ir))
		return;
	or.x-=re->x;
	or.y-=re->y;
	pixbuf=gdk_pixbuf_get_from_drawable(NULL, overlay->drawable, NULL, 0, 0, 0, 0, or.width, or.height);
	pixbuf2=gdk_pixbuf_new(gdk_pixbuf_get_colorspace(pixbuf), TRUE, gdk_pixbuf_get_bits_per_sample(pixbuf),
				or.width, or.height);
	rowstride1 = gdk_pixbuf_get_rowstride (pixbuf);
	rowstride2 = gdk_pixbuf_get_rowstride (pixbuf2);
	pixels1=gdk_pixbuf_get_pixels (pixbuf);
	pixels2=gdk_pixbuf_get_pixels (pixbuf2);
	n_channels1 = gdk_pixbuf_get_n_channels (pixbuf);
	n_channels2 = gdk_pixbuf_get_n_channels (pixbuf2);
	for (y = 0 ; y < or.height ; y++) {
		for (x = 0 ; x < or.width ; x++) {
			p1 = pixels1 + y * rowstride1 + x * n_channels1;
			p2 = pixels2 + y * rowstride2 + x * n_channels2;
			p2[0]=p1[0];
			p2[1]=p1[1];
			p2[2]=p1[2];
			if (bg && p1[0] == r && p1[1] == g && p1[2] == b) 
				p2[3]=a;
			else 
				p2[3]=overlay->a;
		}
	}
	gdk_draw_pixbuf(pixmap, gc, pixbuf2, 0, 0, or.x, or.y, or.width, or.height, GDK_RGB_DITHER_NONE, 0, 0);
	g_object_unref(pixbuf);
	g_object_unref(pixbuf2);
}

static void
draw_restore(struct graphics_priv *gr, struct point *p, int w, int h)
{
	GtkWidget *widget=gr->widget;
	gdk_draw_drawable(widget->window,
                        widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
                        gr->drawable,
                        p->x, p->y, p->x, p->y, w, h);

}

static void
draw_drag(struct graphics_priv *gr, struct point *p)
{
	if (!gr->cleanup) {
		gr->pclean=gr->p;
		gr->cleanup=1;
	}
	if (p)
		gr->p=*p;
	else {
		gr->p.x=0;
		gr->p.y=0;
	}
}


static void
background_gc(struct graphics_priv *gr, struct graphics_gc_priv *gc)
{
	gr->background_gc=gc;
}

static void
gtk_drawing_area_draw(struct graphics_priv *gr, GdkRectangle *r)
{
	GdkPixmap *pixmap;
	GtkWidget *widget=gr->widget;
	GdkGC *gc=widget->style->fg_gc[GTK_WIDGET_STATE(widget)];
	struct graphics_priv *overlay;

	if (! gr->drawable)
		return;
	pixmap = gdk_pixmap_new(widget->window, r->width, r->height, -1);
	if ((gr->p.x || gr->p.y) && gr->background_gc) 
		gdk_draw_rectangle(pixmap, gr->background_gc->gc, TRUE, 0, 0, r->width, r->height);
	gdk_draw_drawable(pixmap, gc, gr->drawable, r->x, r->y, gr->p.x, gr->p.y, r->width, r->height);
	overlay=gr->overlays;
	while (overlay) {
		overlay_draw(gr,overlay,r,pixmap,gc);
		overlay=overlay->next;
	}
	gdk_draw_drawable(widget->window, gc, pixmap, 0, 0, r->x, r->y, r->width, r->height);
	g_object_unref(pixmap);
}

static void
draw_mode(struct graphics_priv *gr, enum draw_mode_num mode)
{
	GdkRectangle r;
	struct graphics_priv *overlay;
#if 0
	if (mode == draw_mode_begin) {
		if (! gr->parent && gr->background_gc)
			gdk_draw_rectangle(gr->drawable, gr->background_gc->gc, TRUE, 0, 0, gr->width, gr->height);
	}
#endif
	if (mode == draw_mode_end && gr->mode != draw_mode_cursor) {
		if (gr->parent) {
			if (gr->cleanup) {
				overlay_rect(gr->parent, gr, 1, &r);
				gtk_drawing_area_draw(gr->parent, &r);
				gr->cleanup=0;
			}
			overlay_rect(gr->parent, gr, 0, &r);
			gtk_drawing_area_draw(gr->parent, &r);
		} else {
			r.x=0;
			r.y=0;
			r.width=gr->width;
			r.height=gr->height;
			gtk_drawing_area_draw(gr, &r);
			overlay=gr->overlays;
			while (overlay) {
				overlay->cleanup=0;
				overlay=overlay->next;
			}
		}
	}
	gr->mode=mode;
}

/* Events */

static gint
configure(GtkWidget * widget, GdkEventConfigure * event, gpointer user_data)
{
	struct graphics_priv *gra=user_data;
	if (! gra->visible)
		return TRUE;
	if (gra->drawable != NULL) {
                g_object_unref(gra->drawable);
        }
	if(gra->background_ready && gra->background != NULL) {
	       g_object_unref(gra->background);
	       gra->background_ready = 0;
	}
#ifndef _WIN32
	dbg(1,"window=%d\n", GDK_WINDOW_XID(widget->window));
#endif
	gra->width=widget->allocation.width;
	gra->height=widget->allocation.height;
        gra->drawable = gdk_pixmap_new(widget->window, gra->width, gra->height, -1);
	callback_list_call_attr_2(gra->cbl, attr_resize, GINT_TO_POINTER(gra->width), GINT_TO_POINTER(gra->height));
	return TRUE;
}

static gint
expose(GtkWidget * widget, GdkEventExpose * event, gpointer user_data)
{
	struct graphics_priv *gra=user_data;

	gra->visible=1;
	if (! gra->drawable)
		configure(widget, NULL, user_data);
	gtk_drawing_area_draw(gra, &event->area);
#if 0
        gdk_draw_drawable(widget->window, widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
                        gra->drawable, event->area.x, event->area.y,
                        event->area.x, event->area.y,
                        event->area.width, event->area.height);
#endif

	return FALSE;
}

#if 0
static gint
button_timeout(gpointer user_data)
{
#if 0
	struct container *co=user_data;
	int x=co->gra->gra->button_event.x;
	int y=co->gra->gra->button_event.y;
	int button=co->gra->gra->button_event.button;

	co->gra->gra->button_timeout=0;
	popup(co, x, y, button);

	return FALSE;
#endif
}
#endif

static int
tv_delta(struct timeval *old, struct timeval *new)
{
	if (new->tv_sec-old->tv_sec >= INT_MAX/1000)
		return INT_MAX;
	return (new->tv_sec-old->tv_sec)*1000+(new->tv_usec-old->tv_usec)/1000;
}

static gint
button_press(GtkWidget * widget, GdkEventButton * event, gpointer user_data)
{
	struct graphics_priv *this=user_data;
	struct point p;
	struct timeval tv;
	struct timezone tz;

	gettimeofday(&tv, &tz);

	if (event->button < 8) {
		if (tv_delta(&this->button_press[event->button], &tv) < this->timeout)
			return FALSE;
		this->button_press[event->button]= tv;
		this->button_release[event->button].tv_sec=0;
		this->button_release[event->button].tv_usec=0;
	}
	p.x=event->x;
	p.y=event->y;
	callback_list_call_attr_3(this->cbl, attr_button, GINT_TO_POINTER(1), GINT_TO_POINTER(event->button), (void *)&p);
	return FALSE;
}

static gint
button_release(GtkWidget * widget, GdkEventButton * event, gpointer user_data)
{
	struct graphics_priv *this=user_data;
	struct point p;
	struct timeval tv;
	struct timezone tz;

	gettimeofday(&tv, &tz);

	if (event->button < 8) {
		if (tv_delta(&this->button_release[event->button], &tv) < this->timeout)
			return FALSE;
		this->button_release[event->button]= tv;
		this->button_press[event->button].tv_sec=0;
		this->button_press[event->button].tv_usec=0;
	}
	p.x=event->x;
	p.y=event->y;
	callback_list_call_attr_3(this->cbl, attr_button, GINT_TO_POINTER(0), GINT_TO_POINTER(event->button), (void *)&p);
	return FALSE;
}



static gint
scroll(GtkWidget * widget, GdkEventScroll * event, gpointer user_data)
{
	struct graphics_priv *this=user_data;
	struct point p;
	int button;

	p.x=event->x;
	p.y=event->y;
	switch (event->direction) {
	case GDK_SCROLL_UP:
		button=4;
		break;
	case GDK_SCROLL_DOWN:
		button=5;
		break;
	default:
		button=-1;
		break;
	}
	if (button != -1) {
		callback_list_call_attr_3(this->cbl, attr_button, GINT_TO_POINTER(1), GINT_TO_POINTER(button), (void *)&p);
		callback_list_call_attr_3(this->cbl, attr_button, GINT_TO_POINTER(0), GINT_TO_POINTER(button), (void *)&p);
	}
	return FALSE;
}

static gint
motion_notify(GtkWidget * widget, GdkEventMotion * event, gpointer user_data)
{
	struct graphics_priv *this=user_data;
	struct point p;

	p.x=event->x;
	p.y=event->y;
	callback_list_call_attr_1(this->cbl, attr_motion, (void *)&p);
	return FALSE;
}

/* *
 * * Exit navit (X pressed)
 * * @param widget active widget
 * * @param event the event (delete_event)
 * * @param user_data Pointer to private data structure
 * * @returns TRUE
 * */
static gint
delete(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	struct graphics_priv *this=user_data;
	dbg(0,"enter this->win=%p\n",this->win);
	if (this->delay & 2) {
		if (this->win) 
			this->win=NULL;
	} else {
		callback_list_call_attr_0(this->cbl, attr_window_closed);
	}
	return TRUE;
}

static gint
keypress(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	struct graphics_priv *this=user_data;
	int len,ucode;
	char key[8];
	ucode=gdk_keyval_to_unicode(event->keyval);
	len=g_unichar_to_utf8(ucode, key);
	key[len]='\0';
	
	switch (event->keyval) {
	case GDK_Up:
		key[0]=NAVIT_KEY_UP;
		key[1]='\0';
		break;
	case GDK_Down:
		key[0]=NAVIT_KEY_DOWN;
		key[1]='\0';
		break;
	case GDK_Left:
		key[0]=NAVIT_KEY_LEFT;
		key[1]='\0';
		break;
	case GDK_Right:
		key[0]=NAVIT_KEY_RIGHT;
		key[1]='\0';
		break;
	case GDK_BackSpace:
		key[0]=NAVIT_KEY_BACKSPACE;
		key[1]='\0';
		break;
	case GDK_Tab:
		key[0]='\t';
		key[1]='\0';
		break;
	case GDK_Delete:
		key[0]=NAVIT_KEY_DELETE;
		key[1]='\0';
		break;
	case GDK_Escape:
		key[0]=NAVIT_KEY_BACK;
		key[1]='\0';
		break;
	case GDK_Return:
	case GDK_KP_Enter:
		key[0]=NAVIT_KEY_RETURN;
		key[1]='\0';
		break;
	case GDK_Book:
#ifdef USE_HILDON
	case GDK_F7:
#endif
		key[0]=NAVIT_KEY_ZOOM_IN;
		key[1]='\0';
		break;
	case GDK_Calendar:
#ifdef USE_HILDON
	case GDK_F8:
#endif
		key[0]=NAVIT_KEY_ZOOM_OUT;
		key[1]='\0';
		break;
	}
	if (key[0])
		callback_list_call_attr_1(this->cbl, attr_keypress, (void *)key);
	else
		dbg(0,"keyval 0x%x\n", event->keyval);
	
	return FALSE;
}

static struct graphics_priv *graphics_gtk_drawing_area_new_helper(struct graphics_methods *meth);

static void
overlay_disable(struct graphics_priv *gr, int disabled)
{
	gr->overlay_disabled=disabled;
}

static void
overlay_resize(struct graphics_priv *this, struct point *p, int w, int h, int alpha, int wraparound)
{
	//do not dereference parent for non overlay osds
	if(!this->parent) {
		return;
	}

	int changed = 0;
	int w2,h2;

	if (w == 0) {
		w2 = 1;
	} else {
		w2 = w;
	}

	if (h == 0) {
		h2 = 1;
	} else {
		h2 = h;
	}

	this->p = *p;
	if (this->width != w2) {
		this->width = w2;
		changed = 1;
	}

	if (this->height != h2) {
		this->height = h2;
		changed = 1;
	}

	this->a = alpha >> 8;
	this->wraparound = wraparound;

	if (changed) {
		// Set the drawables to the right sizes
		g_object_unref(this->drawable);
		g_object_unref(this->background);

		this->drawable=gdk_pixmap_new(this->parent->widget->window, w2, h2, -1);
		this->background=gdk_pixmap_new(this->parent->widget->window, w2, h2, -1);

		if ((w == 0) || (h == 0)) {
			this->overlay_autodisabled = 1;
		} else {
			this->overlay_autodisabled = 0;
		}

		callback_list_call_attr_2(this->cbl, attr_resize, GINT_TO_POINTER(this->width), GINT_TO_POINTER(this->height));
	}
}

static void
get_data_window(struct graphics_priv *this, unsigned int xid)
{
	if (!xid)
		this->win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	else
		this->win = gtk_plug_new(xid);
	if (!gtk_widget_get_parent(this->widget)) 
		gtk_widget_ref(this->widget);
	gtk_window_set_default_size(GTK_WINDOW(this->win), this->win_w, this->win_h);
	dbg(1,"h= %i, w= %i\n",this->win_h, this->win_w);
	gtk_window_set_title(GTK_WINDOW(this->win), "Navit");
	gtk_window_set_wmclass (GTK_WINDOW (this->win), "navit", "Navit");
	gtk_widget_realize(this->win);
	if (gtk_widget_get_parent(this->widget)) 
		gtk_widget_reparent(this->widget, this->win);
	else
		gtk_container_add(GTK_CONTAINER(this->win), this->widget);
	gtk_widget_show_all(this->win);
	GTK_WIDGET_SET_FLAGS (this->widget, GTK_CAN_FOCUS);
       	gtk_widget_set_sensitive(this->widget, TRUE);
	gtk_widget_grab_focus(this->widget);
	g_signal_connect(G_OBJECT(this->widget), "key-press-event", G_CALLBACK(keypress), this);
	g_signal_connect(G_OBJECT(this->win), "delete_event", G_CALLBACK(delete), this);
}

static int
set_attr(struct graphics_priv *gr, struct attr *attr)
{
	dbg(0,"enter\n");
	switch (attr->type) {
	case attr_windowid:
		get_data_window(gr, attr->u.num);
		return 1;
	default:
		return 0;
	}
}

static struct graphics_priv *
overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p, int w, int h, int alpha, int wraparound)
{
	int w2,h2;
	struct graphics_priv *this=graphics_gtk_drawing_area_new_helper(meth);
	this->colormap=gr->colormap;
	this->widget=gr->widget;
	this->p=*p;
	this->width=w;
	this->height=h;
	this->parent=gr;

	/* If either height or width is 0, we set it to 1 to avoid warnings, and
	 * disable the overlay. */
	if (h == 0) {
		h2 = 1;
	} else {
		h2 = h;
	}

	if (w == 0) {
		w2 = 1;
	} else {
		w2 = w;
	}

	this->background=gdk_pixmap_new(gr->widget->window, w2, h2, -1);
	this->drawable=gdk_pixmap_new(gr->widget->window, w2, h2, -1);

	if ((w == 0) || (h == 0)) {
		this->overlay_autodisabled = 1;
	} else {
		this->overlay_autodisabled = 0;
	}

	this->next=gr->overlays;
	this->a=alpha >> 8;
	this->wraparound=wraparound;
	gr->overlays=this;
	return this;
}

static int gtk_argc;
static char **gtk_argv={NULL};


static int
graphics_gtk_drawing_area_fullscreen(struct window *w, int on)
{
	struct graphics_priv *gr=w->priv;
	if (on)
                gtk_window_fullscreen(GTK_WINDOW(gr->win));
	else
                gtk_window_unfullscreen(GTK_WINDOW(gr->win));
	return 1;
}		

static void
graphics_gtk_drawing_area_disable_suspend(struct window *w)
{
	struct graphics_priv *gr=w->priv;

#ifndef _WIN32
	if (gr->pid)
		kill(gr->pid, SIGWINCH);
#else
    dbg(1, "failed to kill() under Windows\n");
#endif
}


static void *
get_data(struct graphics_priv *this, char const *type)
{
	FILE *f;
	if (!strcmp(type,"gtk_widget"))
		return this->widget;
#ifndef _WIN32
	if (!strcmp(type,"xwindow_id"))
		return (void *)GDK_WINDOW_XID(this->widget->window);
#endif
	if (!strcmp(type,"window")) {
		char *cp = getenv("NAVIT_XID");
		unsigned xid = 0;
		if (cp) 
			xid = strtol(cp, NULL, 0);
		if (!(this->delay & 1))
			get_data_window(this, xid);
		this->window.fullscreen=graphics_gtk_drawing_area_fullscreen;
		this->window.disable_suspend=graphics_gtk_drawing_area_disable_suspend;
		this->window.priv=this;
#if !defined(_WIN32) && !defined(__CEGCC__)
		f=popen("pidof /usr/bin/ipaq-sleep","r");
		if (f) {
			fscanf(f,"%d",&this->pid);
			dbg(1,"ipaq_sleep pid=%d\n", this->pid);
			pclose(f);
		}
#endif
		return &this->window;
	}
	return NULL;
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
	draw_drag,
	NULL, /* font_new */
	gc_new,
	background_gc,
	overlay_new,
	image_new,
	get_data,
	image_free,
	NULL, /* get_text_bbox */
	overlay_disable,
	overlay_resize,
	set_attr,
};

static struct graphics_priv *
graphics_gtk_drawing_area_new_helper(struct graphics_methods *meth)
{
	struct font_priv * (*font_freetype_new)(void *meth);
	font_freetype_new=plugin_get_font_type("freetype");
	if (!font_freetype_new)
		return NULL;
	struct graphics_priv *this=g_new0(struct graphics_priv,1);
	font_freetype_new(&this->freetype_methods);
	*meth=graphics_methods;
	meth->font_new=(struct graphics_font_priv *(*)(struct graphics_priv *, struct graphics_font_methods *, char *,  int, int))this->freetype_methods.font_new;
	meth->get_text_bbox=this->freetype_methods.get_text_bbox;

	return this;
}

static struct graphics_priv *
graphics_gtk_drawing_area_new(struct navit *nav, struct graphics_methods *meth, struct attr **attrs, struct callback_list *cbl)
{
	int i;
	GtkWidget *draw;
	struct attr *attr;

	if (! event_request_system("glib","graphics_gtk_drawing_area_new"))
		return NULL;

	draw=gtk_drawing_area_new();
	struct graphics_priv *this=graphics_gtk_drawing_area_new_helper(meth);
	this->nav = nav;
	this->widget=draw;
	this->win_w=792;
	if ((attr=attr_search(attrs, NULL, attr_w))) 
		this->win_w=attr->u.num;
	this->win_h=547;
	if ((attr=attr_search(attrs, NULL, attr_h))) 
		this->win_h=attr->u.num;
	this->timeout=100;
	if ((attr=attr_search(attrs, NULL, attr_timeout))) 
		this->timeout=attr->u.num;
	this->delay=0;
	if ((attr=attr_search(attrs, NULL, attr_delay))) 
		this->delay=attr->u.num;
	this->cbl=cbl;
	this->colormap=gdk_colormap_new(gdk_visual_get_system(),FALSE);
	gtk_widget_set_events(draw, GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK|GDK_POINTER_MOTION_MASK|GDK_KEY_PRESS_MASK);
	g_signal_connect(G_OBJECT(draw), "expose_event", G_CALLBACK(expose), this);
        g_signal_connect(G_OBJECT(draw), "configure_event", G_CALLBACK(configure), this);
	g_signal_connect(G_OBJECT(draw), "button_press_event", G_CALLBACK(button_press), this);
	g_signal_connect(G_OBJECT(draw), "button_release_event", G_CALLBACK(button_release), this);
	g_signal_connect(G_OBJECT(draw), "scroll_event", G_CALLBACK(scroll), this);
	g_signal_connect(G_OBJECT(draw), "motion_notify_event", G_CALLBACK(motion_notify), this);
	g_signal_connect(G_OBJECT(draw), "delete_event", G_CALLBACK(delete), nav);

	for (i = 0; i < 8; i++) {
		this->button_press[i].tv_sec = 0;
		this->button_press[i].tv_usec = 0;
		this->button_release[i].tv_sec = 0;
		this->button_release[i].tv_usec = 0;
	}

	//create hash table for uncompressed image data
	if (!hImageDataCount++)
		hImageData = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, graphics_destroy_image);

	return this;
}

void
plugin_init(void)
{
	gtk_init(&gtk_argc, &gtk_argv);
	gtk_set_locale();
#ifdef HAVE_API_WIN32
	setlocale(LC_NUMERIC, "C"); /* WIN32 gtk resets LC_NUMERIC */
#endif
	plugin_register_graphics_type("gtk_drawing_area", graphics_gtk_drawing_area_new);
}
