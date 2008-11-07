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

#include <glib.h>
#include <gd.h>
#include "config.h"
#include "point.h"
#include "graphics.h"
#include "color.h"
#include "plugin.h"
#include "callback.h"
#include "window.h"
#include "navit.h"
#include "debug.h"
#include "navit/font/freetype/font_freetype.h"

#define NAVIT_GD_XPM_TRANSPARENCY_HACK

#ifdef NAVIT_GD_XPM_TRANSPARENCY_HACK
#include <X11/xpm.h>

BGD_DECLARE(gdImagePtr) gdImageCreateFromXpm (char *filename)
{
  XpmInfo info;
  XpmImage image;
  int i, j, k, number;
  char buf[5];
  gdImagePtr im = 0;
  int *pointer;
  int red = 0, green = 0, blue = 0, alpha = 0;
  int *colors;
  int ret;

  ret = XpmReadFileToXpmImage (filename, &image, &info);
  if (ret != XpmSuccess)
    return 0;

  if (!(im = gdImageCreate (image.width, image.height)))
    return 0;

  number = image.ncolors;
	if (overflow2(sizeof (int), number)) {
		return 0;
	}
  colors = (int *) gdMalloc (sizeof (int) * number);
  if (colors == NULL)
    return (0);
  for (i = 0; i < number; i++)
    {
      alpha = 0;
      switch (strlen (image.colorTable[i].c_color))
	{
	case 4:
	  if (!strcasecmp(image.colorTable[i].c_color,"none")) {
	    red = 0;
	    green = 0;
            blue = 0;
            alpha = 127;
	  } else {
    	    buf[1] = '\0';
	    buf[0] = image.colorTable[i].c_color[1];
	    red = strtol (buf, NULL, 16);

	    buf[0] = image.colorTable[i].c_color[3];
	    green = strtol (buf, NULL, 16);

	    buf[0] = image.colorTable[i].c_color[5];
	    blue = strtol (buf, NULL, 16);
          } 
	  break;
	case 7:
	  buf[2] = '\0';
	  buf[0] = image.colorTable[i].c_color[1];
	  buf[1] = image.colorTable[i].c_color[2];
	  red = strtol (buf, NULL, 16);

	  buf[0] = image.colorTable[i].c_color[3];
	  buf[1] = image.colorTable[i].c_color[4];
	  green = strtol (buf, NULL, 16);

	  buf[0] = image.colorTable[i].c_color[5];
	  buf[1] = image.colorTable[i].c_color[6];
	  blue = strtol (buf, NULL, 16);
	  break;
	case 10:
	  buf[3] = '\0';
	  buf[0] = image.colorTable[i].c_color[1];
	  buf[1] = image.colorTable[i].c_color[2];
	  buf[2] = image.colorTable[i].c_color[3];
	  red = strtol (buf, NULL, 16);
	  red /= 64;

	  buf[0] = image.colorTable[i].c_color[4];
	  buf[1] = image.colorTable[i].c_color[5];
	  buf[2] = image.colorTable[i].c_color[6];
	  green = strtol (buf, NULL, 16);
	  green /= 64;

	  buf[0] = image.colorTable[i].c_color[7];
	  buf[1] = image.colorTable[i].c_color[8];
	  buf[2] = image.colorTable[i].c_color[9];
	  blue = strtol (buf, NULL, 16);
	  blue /= 64;
	  break;
	case 13:
	  buf[4] = '\0';
	  buf[0] = image.colorTable[i].c_color[1];
	  buf[1] = image.colorTable[i].c_color[2];
	  buf[2] = image.colorTable[i].c_color[3];
	  buf[3] = image.colorTable[i].c_color[4];
	  red = strtol (buf, NULL, 16);
	  red /= 256;

	  buf[0] = image.colorTable[i].c_color[5];
	  buf[1] = image.colorTable[i].c_color[6];
	  buf[2] = image.colorTable[i].c_color[7];
	  buf[3] = image.colorTable[i].c_color[8];
	  green = strtol (buf, NULL, 16);
	  green /= 256;

	  buf[0] = image.colorTable[i].c_color[9];
	  buf[1] = image.colorTable[i].c_color[10];
	  buf[2] = image.colorTable[i].c_color[11];
	  buf[3] = image.colorTable[i].c_color[12];
	  blue = strtol (buf, NULL, 16);
	  blue /= 256;
	  break;
	}


      colors[i] = gdImageColorResolveAlpha(im, red, green, blue, alpha);
      if (colors[i] == -1)
	fprintf (stderr, "ARRRGH\n");
    }

  pointer = (int *) image.data;
  for (i = 0; i < image.height; i++)
    {
      for (j = 0; j < image.width; j++)
	{
	  k = *pointer++;
	  gdImageSetPixel (im, j, i, colors[k]);
	}
    }
  gdFree (colors);
  return (im);
}
#endif


struct graphics_priv {
	gdImagePtr im;
	int w,h;
	struct callback *cb;
	struct callback_list *cbl;
	struct navit *nav;
	struct graphics_gc_priv *background;
	struct font_freetype_methods freetype_methods;
	struct window window;
};

struct graphics_gc_priv {
	struct graphics_priv *gr;
	int color;
	int bgcolor;
	int width;
	unsigned char *dash_list;
	int dash_count;
	int dash_list_len;
};

struct graphics_image_priv {
	gdImagePtr im;
};


static void
graphics_destroy(struct graphics_priv *gr)
{
	g_free(gr);
}

static void
gc_destroy(struct graphics_gc_priv *gc)
{
	if (gc->color != -1)
		gdImageColorDeallocate(gc->gr->im, gc->color);
	if (gc->bgcolor != -1)
		gdImageColorDeallocate(gc->gr->im, gc->bgcolor);
	g_free(gc->dash_list);
	g_free(gc);
}

static void
gc_set_linewidth(struct graphics_gc_priv *gc, int w)
{
	gc->width=w;
}

static void
gc_set_dashes(struct graphics_gc_priv *gc, int w, int offset, unsigned char *dash_list, int n)
{
	int i,count=0;
	g_free(gc->dash_list);
	gc->dash_list=g_new(unsigned char, n);
	for (i = 0 ; i < n ; i++) {
		gc->dash_list[i]=dash_list[i];
		count+=dash_list[i];
	}
	gc->dash_list_len=n;
	gc->dash_count=count;
}

static void
gc_set_foreground(struct graphics_gc_priv *gc, struct color *c)
{
	gc->color=gdImageColorAllocate(gc->gr->im, c->r>>8, c->g>>8, c->b>>8); 
}

static void
gc_set_background(struct graphics_gc_priv *gc, struct color *c)
{
	gc->bgcolor=gdImageColorAllocate(gc->gr->im, c->r>>8, c->g>>8, c->b>>8);
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
	struct graphics_gc_priv *ret=g_new0(struct graphics_gc_priv, 1);
	ret->gr=gr;
	ret->width=1;
	ret->color=-1;
	ret->bgcolor=-1;
	*meth=gc_methods;
	return ret;
}


static struct graphics_image_priv *
image_new(struct graphics_priv *gr, struct graphics_image_methods *meth, char *name, int *w, int *h, struct point *hot, int rotation)
{
	FILE *file;
	struct graphics_image_priv *ret=NULL;
	gdImagePtr im=NULL;
	int len;

	if (! name)
		return NULL;
	len=strlen(name);
	if (len < 4)
		return NULL;
	file=fopen(name,"r");
	if (file) {
		if (!strcmp(name+len-4,".png"))
			im=gdImageCreateFromPng(file);
		else if (!strcmp(name+len-4,".xpm"))
			im=gdImageCreateFromXpm(name);
		fclose(file);
	}
	if (im) {
		ret=g_new0(struct graphics_image_priv, 1);
		ret->im=im;
		*w=im->sx;
		*h=im->sy;
		hot->x=im->sx/2;
		hot->y=im->sy/2;
	}
	return ret;
}

static void
draw_lines(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count)
{
	int color[gc->dash_count],cc;
	int i,j,k=0;

	if (gc->dash_count) {
		cc=gc->color;
		for (i = 0 ; i < gc->dash_list_len ; i++) {
			for (j = 0 ; j	< gc->dash_list[i] ; j++) {
				color[k++]=cc;
			}
			if (cc == gdTransparent)
				cc=gc->color;
			else
				cc=gdTransparent;
		}
	}
	gdImageSetStyle(gr->im, color, gc->dash_count);
	gdImageSetThickness(gr->im, gc->width);
	gdImageOpenPolygon(gr->im, (gdPointPtr) p, count, gc->dash_count ? gdStyled : gc->color);  
}

static void
draw_polygon(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int count)
{
	
	gdImageFilledPolygon(gr->im, (gdPointPtr) p, count, gc->color);  
}

static void
draw_rectangle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int w, int h)
{
#if 0
	gdImageFilledRectangle(gr->im, p->x, p->y, p->x+w, p->y+h, gc->color);
#endif
}

static void
draw_circle(struct graphics_priv *gr, struct graphics_gc_priv *gc, struct point *p, int r)
{
	gdImageSetStyle(gr->im, NULL, 0);
	gdImageSetThickness(gr->im, gc->width);
	gdImageArc(gr->im, p->x, p->y, r, r, 0, 360, gc->color);
}


static void
draw_text(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct graphics_gc_priv *bg, struct graphics_font_priv *font, char *text, struct point *p, int dx, int dy)
{
	struct font_freetype_text *t;
	struct font_freetype_glyph *g, **gp;
	gdImagePtr im;
	int i,x,y,w,h;
	t=gr->freetype_methods.text_new(text, (struct font_freetype_font *)font, dx, dy);
	struct color transparent = {0x0, 0x0, 0x0, 0x7f7f};
	struct color white = {0xffff, 0xffff, 0xffff, 0x0};
	struct color black = {0x0, 0x0, 0x0, 0x0};

	x=p->x << 6;
	y=p->y << 6;
	gp=t->glyph;
	i=t->glyph_count;
	while (i-- > 0)
	{
		g=*gp++;
		w=g->w;
		h=g->h;
		if (w && h) {
			im=gdImageCreateTrueColor(w+2, h+2);
			gr->freetype_methods.get_shadow(g,(unsigned char *)(im->tpixels),32,0,&white,&transparent);
			gdImageCopy(gr->im, im, ((x+g->x)>>6)-1, ((y+g->y)>>6)-1, 0, 0, w+2, h+2);
			gdImageDestroy(im);
		}
		x+=g->dx;
		y+=g->dy;
	}
	x=p->x << 6;
	y=p->y << 6;
	gp=t->glyph;
	i=t->glyph_count;
	while (i-- > 0)
	{
		g=*gp++;
		w=g->w;
		h=g->h;
		if (w && h) {
			im=gdImageCreateTrueColor(w, h);
			gr->freetype_methods.get_glyph(g,(unsigned char *)(im->tpixels),32,0,&black,&white,&transparent);
			gdImageCopy(gr->im, im, (x+g->x)>>6, (y+g->y)>>6, 0, 0, w, h);
			gdImageDestroy(im);
		}
		x+=g->dx;
		y+=g->dy;
	}
	gr->freetype_methods.text_destroy(t);
}

static void
draw_image(struct graphics_priv *gr, struct graphics_gc_priv *fg, struct point *p, struct graphics_image_priv *img)
{
	gdImageCopy(gr->im, img->im, p->x, p->y, 0, 0, img->im->sx, img->im->sy);
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
	gr->background=gc;
}

static void
draw_mode(struct graphics_priv *gr, enum draw_mode_num mode)
{
	FILE *pngout;
	dbg(0,"enter\n");
	if (mode == draw_mode_begin && gr->background) {
		gdImageFilledRectangle(gr->im, 0, 0, gr->w, gr->h, gr->background->color);
	}
	if (mode == draw_mode_end) {
		pngout=fopen("test.png", "wb");
		gdImagePng(gr->im, pngout);
		fclose(pngout);
	}
}

static struct graphics_priv * overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p, int w, int h, int alpha);

static void *
get_data(struct graphics_priv *this, char *type)
{
	if (strcmp(type,"window"))
		return NULL;
	return &this->window;
}


static void
image_free(struct graphics_priv *gr, struct graphics_image_priv *priv)
{	
	gdImageDestroy(priv->im);
	g_free(priv);
}

static void
overlay_disable(struct graphics_priv *gr, int disable)
{
	dbg(0,"enter\n");
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
	NULL,
	NULL,
	gc_new,
	background_gc,
	overlay_new,
	image_new,
	get_data,
	image_free,
	NULL,
	overlay_disable,
};

static struct graphics_priv *
overlay_new(struct graphics_priv *gr, struct graphics_methods *meth, struct point *p, int w, int h, int alpha)
{
	*meth=graphics_methods;
	return NULL;
}

static void
emit_callback(struct graphics_priv *priv)
{
	callback_list_call_attr_2(priv->cbl, attr_resize, (void *)priv->w, (void *)priv->h);
}


static struct graphics_priv *
graphics_gd_new(struct navit *nav, struct graphics_methods *meth, struct attr **attrs, struct callback_list *cbl)
{
	struct font_priv * (*font_freetype_new)(void *meth);
	struct graphics_priv *ret;
	struct attr *attr;
        font_freetype_new=plugin_get_font_type("freetype");
        if (!font_freetype_new)
                return NULL;
	*meth=graphics_methods;
	ret=g_new(struct graphics_priv, 1);
        font_freetype_new(&ret->freetype_methods);
        meth->font_new=(struct graphics_font_priv *(*)(struct graphics_priv *, struct graphics_font_methods *, char *,  int, int))ret->freetype_methods.font_new;
        meth->get_text_bbox=ret->freetype_methods.get_text_bbox;
	ret->cb=callback_new_attr_1(callback_cast(emit_callback), attr_navit, ret);
	navit_add_callback(nav, ret->cb);
	ret->cbl=cbl;
	ret->nav=nav;
	attr=attr_search(attrs, NULL, attr_w);
	if (attr)
		ret->w=attr->u.num;
	else
		ret->w=800;
	attr=attr_search(attrs, NULL, attr_h);
	if (attr)
		ret->h=attr->u.num;
	else
		ret->h=600;
	ret->im=gdImageCreateTrueColor(ret->w,ret->h);
	return ret;
}

void
plugin_init(void)
{
        plugin_register_graphics_type("gd", graphics_gd_new);
}
