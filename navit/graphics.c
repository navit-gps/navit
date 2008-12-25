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
//# File: graphics.c
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//#
//##############################################################################################################

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "debug.h"
#include "string.h"
#include "draw_info.h"
#include "point.h"
#include "graphics.h"
#include "projection.h"
#include "item.h"
#include "map.h"
#include "coord.h"
#include "transform.h"
#include "plugin.h"
#include "profile.h"
#include "mapset.h"
#include "layout.h"
#include "route.h"
#include "util.h"
#include "callback.h"
#include "file.h"

static char *navit_sharedir;

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
struct graphics
{
	struct graphics_priv *priv;
	struct graphics_methods meth;
	char *default_font;
	struct graphics_font *font[16];
	struct graphics_gc *gc[3];
	struct attr **attrs;
	struct callback_list *cbl;
	int ready;
	struct point_rect r;
};


struct displaylist {
	GHashTable *dl;
};


void
graphics_set_rect(struct graphics *gra, struct point_rect *pr)
{
	gra->r=*pr;
}

/**
 * Creates a new graphics object
 * attr type required
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
struct graphics * graphics_new(struct attr *parent, struct attr **attrs)
{
	struct graphics *this_;
    	struct attr *type_attr;
	struct graphics_priv * (*graphicstype_new)(struct navit *nav, struct graphics_methods *meth, struct attr **attrs, struct callback_list *cbl);

        if (! (type_attr=attr_search(attrs, NULL, attr_type))) {
                return NULL;
        }

	graphicstype_new=plugin_get_graphics_type(type_attr->u.str);
	if (! graphicstype_new)
		return NULL;
	this_=g_new0(struct graphics, 1);
	this_->cbl=callback_list_new();
	this_->priv=(*graphicstype_new)(parent->u.navit, &this_->meth, attrs, this_->cbl);
	this_->attrs=attr_list_dup(attrs);
	return this_;
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
int graphics_get_attr(struct graphics *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter)
{
	return attr_generic_get_attr(this_->attrs, NULL, type, attr, iter);
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
struct graphics * graphics_overlay_new(struct graphics *parent, struct point *p, int w, int h, int alpha, int wraparound)
{
	struct graphics *this_;
	if (!parent->meth.overlay_new)
		return NULL;
	this_=g_new0(struct graphics, 1);
	this_->priv=parent->meth.overlay_new(parent->priv, &this_->meth, p, w, h, alpha, wraparound);
	if (!this_->priv) {
		g_free(this_);
		this_=NULL;
	}
	return this_;
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void graphics_init(struct graphics *this_)
{
	this_->gc[0]=graphics_gc_new(this_);
	graphics_gc_set_background(this_->gc[0], &(struct color) { 0xffff, 0xefef, 0xb7b7, 0xffff});
	graphics_gc_set_foreground(this_->gc[0], &(struct color) { 0xffff, 0xefef, 0xb7b7, 0xffff });
	this_->gc[1]=graphics_gc_new(this_);
	graphics_gc_set_background(this_->gc[1], &(struct color) { 0x0000, 0x0000, 0x0000, 0xffff });
	graphics_gc_set_foreground(this_->gc[1], &(struct color) { 0xffff, 0xffff, 0xffff, 0xffff });
	this_->gc[2]=graphics_gc_new(this_);
	graphics_gc_set_background(this_->gc[2], &(struct color) { 0xffff, 0xffff, 0xffff, 0xffff });
	graphics_gc_set_foreground(this_->gc[2], &(struct color) { 0x0000, 0x0000, 0x0000, 0xffff });
	graphics_background_gc(this_, this_->gc[0]);
	navit_sharedir = getenv("NAVIT_SHAREDIR");
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void * graphics_get_data(struct graphics *this_, char *type)
{
	return (this_->meth.get_data(this_->priv, type));
}

void graphics_add_callback(struct graphics *this_, struct callback *cb)
{
	callback_list_add(this_->cbl, cb);
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
struct graphics_font * graphics_font_new(struct graphics *gra, int size, int flags)
{
	struct graphics_font *this_;

	this_=g_new0(struct graphics_font,1);
	this_->priv=gra->meth.font_new(gra->priv, &this_->meth, gra->default_font, size, flags);
	return this_;
}

/**
 * Free all loaded fonts.
 * Used when switching layouts.
 * @param gra The graphics instance
 * @returns nothing
 * @author Sarah Nordstrom (05/2008)
 */
void graphics_font_destroy_all(struct graphics *gra) 
{ 
	int i; 
	for(i = 0 ; i < sizeof(gra->font) / sizeof(gra->font[0]); i++) { 
 		if(!gra->font[i]) continue; 
 		gra->font[i]->meth.font_destroy(gra->font[i]->priv); 
             	gra->font[i] = NULL; 
	}
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
struct graphics_gc * graphics_gc_new(struct graphics *gra)
{
	struct graphics_gc *this_;

	this_=g_new0(struct graphics_gc,1);
	this_->priv=gra->meth.gc_new(gra->priv, &this_->meth);
	return this_;
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void graphics_gc_destroy(struct graphics_gc *gc)
{
	gc->meth.gc_destroy(gc->priv);
	g_free(gc);
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void graphics_gc_set_foreground(struct graphics_gc *gc, struct color *c)
{
	gc->meth.gc_set_foreground(gc->priv, c);
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void graphics_gc_set_background(struct graphics_gc *gc, struct color *c)
{
	gc->meth.gc_set_background(gc->priv, c);
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void graphics_gc_set_linewidth(struct graphics_gc *gc, int width)
{
	gc->meth.gc_set_linewidth(gc->priv, width);
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void graphics_gc_set_dashes(struct graphics_gc *gc, int width, int offset, unsigned char dash_list[], int n)
{
	if (gc->meth.gc_set_dashes)
		gc->meth.gc_set_dashes(gc->priv, width, offset, dash_list, n);
}

/**
 * Create a new image from file path scaled to w and h pixels
 * @param gra the graphics instance
 * @param path path of the image to load
 * @param w width to rescale to
 * @param h height to rescale to
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
struct graphics_image * graphics_image_new_scaled(struct graphics *gra, char *path, int w, int h)
{
	struct graphics_image *this_;

	this_=g_new0(struct graphics_image,1);
	this_->height=h;
	this_->width=w;
	this_->priv=gra->meth.image_new(gra->priv, &this_->meth, path, &this_->width, &this_->height, &this_->hot, 0);
	if (! this_->priv) {
		g_free(this_);
		this_=NULL;
	}
	return this_;
}

/**
 * Create a new image from file path scaled to w and h pixels and possibly rotated
 * @param gra the graphics instance
 * @param path path of the image to load
 * @param w width to rescale to
 * @param h height to rescale to
 * @param rotate angle to rotate the image. Warning, graphics might only support 90 degree steps here
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
struct graphics_image * graphics_image_new_scaled_rotated(struct graphics *gra, char *path, int w, int h, int rotate)
{
	struct graphics_image *this_;

	this_=g_new0(struct graphics_image,1);
	this_->height=h;
	this_->width=w;
	this_->priv=gra->meth.image_new(gra->priv, &this_->meth, path, &this_->width, &this_->height, &this_->hot, rotate);
	if (! this_->priv) {
		g_free(this_);
		this_=NULL;
	}
	return this_;
}

/**
 * Create a new image from file path
 * @param gra the graphics instance
 * @param path path of the image to load
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
struct graphics_image * graphics_image_new(struct graphics *gra, char *path)
{
	return graphics_image_new_scaled(gra, path, -1, -1);
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void graphics_image_free(struct graphics *gra, struct graphics_image *img)
{
	if (gra->meth.image_free)
		gra->meth.image_free(gra->priv, img->priv);
	g_free(img);
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void graphics_draw_restore(struct graphics *this_, struct point *p, int w, int h)
{
	this_->meth.draw_restore(this_->priv, p, w, h);
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void graphics_draw_mode(struct graphics *this_, enum draw_mode_num mode)
{
	this_->meth.draw_mode(this_->priv, mode);
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void graphics_draw_lines(struct graphics *this_, struct graphics_gc *gc, struct point *p, int count)
{
	this_->meth.draw_lines(this_->priv, gc->priv, p, count);
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void graphics_draw_circle(struct graphics *this_, struct graphics_gc *gc, struct point *p, int r)
{
	this_->meth.draw_circle(this_->priv, gc->priv, p, r);
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void graphics_draw_rectangle(struct graphics *this_, struct graphics_gc *gc, struct point *p, int w, int h)
{
	this_->meth.draw_rectangle(this_->priv, gc->priv, p, w, h);
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void graphics_draw_text(struct graphics *this_, struct graphics_gc *gc1, struct graphics_gc *gc2, struct graphics_font *font, char *text, struct point *p, int dx, int dy)
{
	this_->meth.draw_text(this_->priv, gc1->priv, gc2 ? gc2->priv : NULL, font->priv, text, p, dx, dy);
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void graphics_get_text_bbox(struct graphics *this_, struct graphics_font *font, char *text, int dx, int dy, struct point *ret, int estimate)
{
	this_->meth.get_text_bbox(this_->priv, font->priv, text, dx, dy, ret, estimate);
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void graphics_overlay_disable(struct graphics *this_, int disable)
{
	if (this_->meth.overlay_disable)
		this_->meth.overlay_disable(this_->priv, disable);
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void graphics_draw_image(struct graphics *this_, struct graphics_gc *gc, struct point *p, struct graphics_image *img)
{
	this_->meth.draw_image(this_->priv, gc->priv, p, img->priv);
}


//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
int
graphics_draw_drag(struct graphics *this_, struct point *p)
{
	if (!this_->meth.draw_drag)
		return 0;
	this_->meth.draw_drag(this_->priv, p);
	return 1;
}

void
graphics_background_gc(struct graphics *this_, struct graphics_gc *gc)
{
	this_->meth.background_gc(this_->priv, gc ? gc->priv : NULL);
}

#include "attr.h"
#include "popup.h"
#include <stdio.h>


#if 0
//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void popup_view_html(struct popup_item *item, char *file)
{
	char command[1024];
	sprintf(command,"firefox %s", file);
	system(command);
}

struct transformatin *tg;
enum projection pg;

//##############################################################################################################
//# Description: 
//# Comment: 
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
static void graphics_popup(struct display_list *list, struct popup_item **popup)
{
	struct item *item;
	struct attr attr;
	struct map_rect *mr;
	struct coord c;
	struct popup_item *curr_item,*last=NULL;
	item=list->data;
	mr=map_rect_new(item->map, NULL, NULL, 0);
	printf("id hi=0x%x lo=0x%x\n", item->id_hi, item->id_lo);
	item=map_rect_get_item_byid(mr, item->id_hi, item->id_lo);
	if (item) {
		if (item_attr_get(item, attr_name, &attr)) {
			curr_item=popup_item_new_text(popup,attr.u.str,1);
			if (item_attr_get(item, attr_info_html, &attr)) {
				popup_item_new_func(&last,"HTML Info",1, popup_view_html, g_strdup(attr.u.str));
			}
			if (item_attr_get(item, attr_price_html, &attr)) {
				popup_item_new_func(&last,"HTML Preis",2, popup_view_html, g_strdup(attr.u.str));
			}
			curr_item->submenu=last;
		}
	}
	map_rect_destroy(mr);
}
#endif

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
struct displayitem {
	struct item item;
	char *label;
	int displayed;
	int count;
	struct coord c[0];
};

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
static int xdisplay_free_list(gpointer key, gpointer value, gpointer user_data)
{
	GList *h, *l;
	h=value;
	l=h;
	while (l) {
		struct displayitem *di=l->data;
		if (! di->displayed && di->item.type < type_line) 
			dbg(1,"warning: item '%s' not displayed\n", item_to_name(di->item.type));
		g_free(l->data);
		l=g_list_next(l);
	}
	g_list_free(h);
	return TRUE;
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
static void xdisplay_free(GHashTable *display_list)
{
	g_hash_table_foreach_remove(display_list, xdisplay_free_list, NULL);
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
static void display_add(struct displaylist *displaylist, struct item *item, int count, struct coord *c, char *label)
{
	struct displayitem *di;
	int len;
	GList *l;
	char *p;

	len=sizeof(*di)+count*sizeof(*c);
	if (label)
		len+=strlen(label)+1;

	p=g_malloc(len);

	di=(struct displayitem *)p;
	di->displayed=0;
	p+=sizeof(*di)+count*sizeof(*c);
	di->item=*item;
	if (label) {
		di->label=p;
		strcpy(di->label, label);
	} else 
		di->label=NULL;
	di->count=count;
	memcpy(di->c, c, count*sizeof(*c));

	l=g_hash_table_lookup(displaylist->dl, GINT_TO_POINTER(item->type));
	l=g_list_prepend(l, di);
	g_hash_table_insert(displaylist->dl, GINT_TO_POINTER(item->type), l);
}


/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
static void label_line(struct graphics *gra, struct graphics_gc *fg, struct graphics_gc *bg, struct graphics_font *font, struct point *p, int count, char *label)
{
	int i,x,y,tl,tlm,th,thm,tlsq,l;
	float lsq;
	double dx,dy;
	struct point p_t;
	struct point pb[5];

	if (gra->meth.get_text_bbox) {
		gra->meth.get_text_bbox(gra->priv, font->priv, label, 0x10000, 0x0, pb, 1);
		tl=(pb[2].x-pb[0].x);
		th=(pb[0].y-pb[1].y);
	} else {
		tl=strlen(label)*4;
		th=8;
	}
	tlm=tl*128;
	thm=th*144;
	tlsq = tlm*tlm;
	for (i = 0 ; i < count-1 ; i++) {
		dx=p[i+1].x-p[i].x;
		dx*=128;
		dy=p[i+1].y-p[i].y;
		dy*=128;
		lsq = dx*dx+dy*dy;
		if (lsq > tlsq) {
			l=(int)sqrtf(lsq);
			x=p[i].x;
			y=p[i].y;
			if (dx < 0) {
				dx=-dx;
				dy=-dy;
				x=p[i+1].x;
				y=p[i+1].y;
			}
			x+=(l-tlm)*dx/l/256;
			y+=(l-tlm)*dy/l/256;
			x-=dy*thm/l/256;
			y+=dx*thm/l/256;
			p_t.x=x;
			p_t.y=y;
#if 0
			dbg(0,"display_text: '%s', %d, %d, %d, %d %d\n", label, x, y, dx*0x10000/l, dy*0x10000/l, l);
#endif
			if (x < gra->r.rl.x && x + tl > gra->r.lu.x && y + tl > gra->r.lu.y && y - tl < gra->r.rl.y) 
				gra->meth.draw_text(gra->priv, fg->priv, bg->priv, font->priv, label, &p_t, dx*0x10000/l, dy*0x10000/l);
		}
	}
}

static void display_draw_arrow(struct point *p, int dx, int dy, int l, struct graphics_gc *gc, struct graphics *gra)
{
	struct point pnt[3];
	pnt[0]=pnt[1]=pnt[2]=*p;
	pnt[0].x+=-dx*l/65536+dy*l/65536;
	pnt[0].y+=-dy*l/65536-dx*l/65536;
	pnt[2].x+=-dx*l/65536-dy*l/65536;
	pnt[2].y+=-dy*l/65536+dx*l/65536;
	gra->meth.draw_lines(gra->priv, gc->priv, pnt, 3);
}

static void display_draw_arrows(struct graphics *gra, struct graphics_gc *gc, struct point *pnt, int count)
{
	int i,dx,dy,l;
	struct point p;
	for (i = 0 ; i < count-1 ; i++) {
		dx=pnt[i+1].x-pnt[i].x;	
		dy=pnt[i+1].y-pnt[i].y;
		l=sqrt(dx*dx+dy*dy);
		if (l) {
			dx=dx*65536/l;
			dy=dy*65536/l;
			p=pnt[i];
			p.x+=dx*15/65536;
			p.y+=dy*15/65536;
			display_draw_arrow(&p, dx, dy, 10, gc, gra);
			p=pnt[i+1];
			p.x-=dx*15/65536;
			p.y-=dy*15/65536;
			display_draw_arrow(&p, dx, dy, 10, gc, gra);
		}
	}
}

static int
intersection(struct point * a1, int adx, int ady, struct point * b1, int bdx, int bdy,
	      struct point * res)
{
	int n, a, b;
	n = bdy * adx - bdx * ady;
	a = bdx * (a1->y - b1->y) - bdy * (a1->x - b1->x);
	b = adx * (a1->y - b1->y) - ady * (a1->x - b1->x);
	if (n < 0) {
		n = -n;
		a = -a;
		b = -b;
	}
#if 0
	if (a < 0 || b < 0)
		return 0;
	if (a > n || b > n)
		return 0;
#endif
	if (n == 0)
		return 0;
	res->x = a1->x + a * adx / n;
	res->y = a1->y + a * ady / n;
	return 1;
}

struct circle {
	int x,y,fowler;
} lw10[]={
{0,10,0},
{4,9,56},
{7,7,128},
{9,4,200},
{10,0,256},
{9,-4,312},
{7,-7,384},
{4,-9,456},
{0,-10,512},
{-4,-9,568},
{-7,-7,640},
{-9,-4,712},
{-10,0,768},
{-9,4,824},
{-7,7,896},
{-4,9,968},
};

static void
draw_circle(struct point *pnt, int diameter, int scale, int start, int len, struct point *res, int *pos, int dir)
{
	struct circle *c;

#if 0
	dbg(0,"diameter=%d start=%d len=%d pos=%d dir=%d\n", diameter, start, len, *pos, dir);
#endif
	int end=start+len;
	int i;
	c=lw10;
	if (len > 0) {
		while (start < 0) {
			start+=1024;
			end+=1024;
		}
		while (end > 0) {
			for (i = 0 ; i < 16 ; i++) {
				if (c[i].fowler > start && c[i].fowler < end) {
					res[*pos].x=pnt->x+c[i].x*diameter/20;
					res[*pos].y=pnt->y+c[i].y*diameter/20;
					(*pos)+=dir;
				}
			}
			end-=1024;
			start-=1024;
		}
	} else {
		while (start > 1024) {
			start-=1024;
			end-=1024;
		}
		while (end < 1024) {
			for (i = 15 ; i >= 0 ; i--) {
					     if (c[i].fowler < start && c[i].fowler > end) {
					res[*pos].x=pnt->x+c[i].x*diameter/20;
					res[*pos].y=pnt->y+c[i].y*diameter/20;
					(*pos)+=dir;
				}
			}
			start+=1024;
			end+=1024;
		}
	}
}


static int
fowler(int dy, int dx)
{
	int adx, ady;		/* Absolute Values of Dx and Dy */
	int code;		/* Angular Region Classification Code */

	adx = (dx < 0) ? -dx : dx;      /* Compute the absolute values. */
	ady = (dy < 0) ? -dy : dy;

	code = (adx < ady) ? 1 : 0;
	if (dx < 0)
		code += 2;
	if (dy < 0)
		code += 4;

	switch (code) {
	case 0:
		return (dx == 0) ? 0 : 128*ady / adx;   /* [  0, 45] */
	case 1:
		return (256 - (128*adx / ady)); /* ( 45, 90] */
	case 3:
		return (256 + (128*adx / ady)); /* ( 90,135) */
	case 2:
		return (512 - (128*ady / adx)); /* [135,180] */
	case 6:
		return (512 + (128*ady / adx)); /* (180,225] */
	case 7:
		return (768 - (128*adx / ady)); /* (225,270) */
	case 5:
		return (768 + (128*adx / ady)); /* [270,315) */
	case 4:
		return (1024 - (128*ady / adx));/* [315,360) */
	}
	return 0;
}
static int
int_sqrt(unsigned int n)
{
	unsigned int h, p= 0, q= 1, r= n;
	while ( q <= n )
		q <<= 2;
	while ( q != 1 ) {
		q >>= 2;
		h = p + q;
		p >>= 1;
		if ( r >= h ) {
			p += q;
			r -= h;
        	}
	}
	return p;
}

struct offset {
	int px,py,nx,ny;
};

static void
calc_offsets(int wi, int l, int dx, int dy, struct offset *res)
{
	int x,y;
	x = (dx * wi) / l;
	y = (dy * wi) / l;
	if (x < 0) {
		res->nx = -x/2;
		res->px = (x-1)/2;
	} else {
		res->nx = -(x+1)/2;
		res->px = x/2;
	}
	if (y < 0) {
		res->ny = -y/2;
		res->py = (y-1)/2;
	} else {
		res->ny = -(y+1)/2;
		res->py = y/2;
	}
}

static void
graphics_draw_polyline_as_polygon(struct graphics *gra, struct graphics_gc *gc, struct point *pnt, int count, int *width, int step)
{
	int maxpoints=200;
	struct point res[maxpoints], pos, poso, neg, nego;
	int i, j, dx, dy, l, dxo, dyo;
	struct offset o,oo;
	int fow, fowo, delta;
	int wi, ppos = maxpoints/2, npos = maxpoints/2;
	int state,prec=5;
	int max_circle_points=20;
	for (i = 0; i < count; i++) {
		wi=*width;
		width+=step;
		if (i < count - 1) {
			dx = (pnt[i + 1].x - pnt[i].x);
			dy = (pnt[i + 1].y - pnt[i].y);
			l = int_sqrt(dx * dx + dy * dy);
			fow=fowler(-dy, dx);
		}
		if (! l) 
			l=1;
#if 0
		dbg(0,"p[%d of %d]=%d,%d wi=%d l=%d\n", i,count-1,pnt[i].x,pnt[i].y, wi, l);
#endif
		calc_offsets(wi, l, dx, dy, &o);
		pos.x = pnt[i].x + o.ny;
		pos.y = pnt[i].y + o.px;
		neg.x = pnt[i].x + o.py;
		neg.y = pnt[i].y + o.nx;
		if (! i)
			state=0;
		else if (i == count-1) 
			state=2;
		else if (npos < max_circle_points || ppos >= maxpoints-max_circle_points)
			state=3;
		else
			state=1;
#if 0
		dbg(0,"state=%d npos=%d ppos=%d\n", state, npos, ppos);
#endif
		switch (state) {
		case 1:
		       if (fowo != fow) {
				poso.x = pnt[i].x + oo.ny;
				poso.y = pnt[i].y + oo.px;
				nego.x = pnt[i].x + oo.py;
				nego.y = pnt[i].y + oo.nx;
				delta=fowo-fow;
				if (delta < 0)
					delta+=1024;
				if (delta < 512) {
					if (intersection(&pos, dx, dy, &poso, dxo, dyo, &res[ppos]))
						ppos++;
					res[--npos] = nego;
					--npos;
					draw_circle(&pnt[i], wi, prec, fowo-512, -delta, res, &npos, -1);
					res[npos] = neg;
				} else {
					res[ppos++] = poso;
					draw_circle(&pnt[i], wi, prec, fowo, 1024-delta, res, &ppos, 1);
					res[ppos++] = pos;
					if (intersection(&neg, dx, dy, &nego, dxo, dyo, &res[npos - 1]))
						npos--;
				}
			}
			break;
		case 2:
		case 3:
			res[--npos] = neg;
			--npos;
			draw_circle(&pnt[i], wi, prec, fow-512, -512, res, &npos, -1);
			res[npos] = pos;
			res[ppos++] = pos;
#if 0
			dbg(0,"npos=%d ppos=%d\n", npos, ppos);
#endif
			dbg_assert(npos > 0);
			dbg_assert(ppos < maxpoints);
			gra->meth.draw_polygon(gra->priv, gc->priv, res+npos, ppos-npos);
#if 0
			for (j = npos ; j < ppos ; j++)
				dbg(0,"res[%d]=%d,%d\n", j-npos, res[j].x, res[j].y);
#endif
			if (state == 2)
				break;
			npos=maxpoints/2;
			ppos=maxpoints/2;
		case 0:
			res[ppos++] = neg;
			draw_circle(&pnt[i], wi, prec, fow+512, 512, res, &ppos, 1);
			res[ppos++] = pos;
			break;
		}
		if (step) {
			wi=*width;
			calc_offsets(wi, l, dx, dy, &oo);
		} else 
			oo=o;
		dxo = -dx;
		dyo = -dy;
		fowo=fow;
	}
}

struct transformation *tg;
enum projection pg;

struct wpoint {
	int x,y,w;
};

static int
clipcode(struct wpoint *p, struct point_rect *r)
{
	int code=0;
	if (p->x < r->lu.x)
		code=1;
	if (p->x > r->rl.x)
		code=2;
	if (p->y < r->lu.y)
		code |=4;
	if (p->y > r->rl.y)
		code |=8;
	return code;
}


static int
clip_line(struct wpoint *p1, struct wpoint *p2, struct point_rect *r)
{
	int code1,code2,ret=1;
	int dx,dy,dw;
	code1=clipcode(p1, r);
	if (code1)
		ret |= 2;
	code2=clipcode(p2, r);
	if (code2)
		ret |= 4;
	dx=p2->x-p1->x;
	dy=p2->y-p1->y;
	dw=p2->w-p1->w;
	while (code1 || code2) {
		if (code1 & code2)
			return 0;
		if (code1 & 1) {
			p1->y+=(r->lu.x-p1->x)*dy/dx;
			p1->w+=(r->lu.x-p1->x)*dw/dx;
			p1->x=r->lu.x;
		} else if (code1 & 2) {
			p1->y+=(r->rl.x-p1->x)*dy/dx;
			p1->w+=(r->rl.x-p1->x)*dw/dx;
			p1->x=r->rl.x;
		} else if (code1 & 4) {
			p1->x+=(r->lu.y-p1->y)*dx/dy;
			p1->w+=(r->lu.y-p1->y)*dw/dy;
			p1->y=r->lu.y;
		} else if (code1 & 8) {
			p1->x+=(r->rl.y-p1->y)*dx/dy;
			p1->w+=(r->rl.y-p1->y)*dw/dy;
			p1->y=r->rl.y;
		}
		code1=clipcode(p1, r);
		if (code1 & code2)
			return 0;
		if (code2 & 1) {
			p2->y+=(r->lu.x-p2->x)*dy/dx;
			p2->w+=(r->lu.x-p2->x)*dw/dx;
			p2->x=r->lu.x;
		} else if (code2 & 2) {
			p2->y+=(r->rl.x-p2->x)*dy/dx;
			p2->w+=(r->rl.x-p2->x)*dw/dx;
			p2->x=r->rl.x;
		} else if (code2 & 4) {
			p2->x+=(r->lu.y-p2->y)*dx/dy;
			p2->w+=(r->lu.y-p2->y)*dw/dy;
			p2->y=r->lu.y;
		} else if (code2 & 8) {
			p2->x+=(r->rl.y-p2->y)*dx/dy;
			p2->w+=(r->rl.y-p2->y)*dw/dy;
			p2->y=r->rl.y;
		}
		code2=clipcode(p2, r);
	}
	return ret;
}

static void
graphics_draw_polyline_clipped(struct graphics *gra, struct graphics_gc *gc, struct point *pa, int count, int *width, int step, int poly)
{
	struct point p[count+1];
	int w[count*step+1];
	struct wpoint p1,p2;
	int i,code,out=0,codeo=0;
	int dx,dy;
	int wmax;
	struct point_rect r=gra->r;

	wmax=width[0];
	if (step) {
		for (i = 1 ; i < count ; i++) {
			if (width[i*step] > wmax)
				wmax=width[i*step];
		}
	}
	if (wmax <= 0)
		return;
	r.lu.x-=wmax;
	r.lu.y-=wmax;
	r.rl.x+=wmax;
	r.rl.y+=wmax;
#if 0
	for (i = 0 ; i < count ; i++) {
		dbg(0,"in[%d]=%d,%d (%d)\n", i, pa[i].x, pa[i].y, width[i*step]);
	}
#endif
	for (i = 0 ; i < count ; i++) {
		if (i) {
			p1.x=pa[i-1].x;
			p1.y=pa[i-1].y;
			p1.w=width[(i-1)*step];
			p2.x=pa[i].x;
			p2.y=pa[i].y;
			p2.w=width[i*step];
			/* 0 = invisible, 1 = completely visible, 3 = start point clipped, 5 = end point clipped, 7 both points clipped */
			code=clip_line(&p1, &p2, &r);
#if 0
			dbg(0,"code=%d\n", code);
#endif
			if (((code == 1 || code == 5) && i == 1) || (code & 2)) {
#if 0
				dbg(0,"start = %d,%d\n", p1.x, p1.y);
#endif
				p[out].x=p1.x;
				p[out].y=p1.y;
				w[out*step]=p1.w;
				out++;
			}
			if (code) {
#if 0
				dbg(0,"end = %d,%d\n", p2.x, p2.y);
#endif
				p[out].x=p2.x;
				p[out].y=p2.y;
				w[out*step]=p2.w;
				out++;
			}
			if (i == count-1 || (code & 4)) {
#if 0
				int j;
				for (j = 0 ; j < out ; j++) {
					dbg(0,"out[%d]=%d,%d (%d)\n", j, p[j].x, p[j].y, w[j*step]);
				}
#endif
				if (out > 1) {
					if (poly) {	
						graphics_draw_polyline_as_polygon(gra, gc, p, out, w, step);
					} else
						gra->meth.draw_lines(gra->priv, gc->priv, p, out);
					out=0;
				}
			}
		}
	}
#if 0
	dbg(0,"done\n");
#endif
}

static int
is_inside(struct point *p, struct point_rect *r, int edge)
{
	switch(edge) {
	case 0:
		return p->x >= r->lu.x;
	case 1:
		return p->x <= r->rl.x;
	case 2:
		return p->y >= r->lu.y;
	case 3:
		return p->y <= r->rl.y;
	}
}

static void
poly_intersection(struct point *p1, struct point *p2, struct point_rect *r, int edge, struct point *ret)
{
	int dx=p2->x-p1->x;
	int dy=p2->y-p1->y;
	switch(edge) {
	case 0:
		ret->y=p1->y+(r->lu.x-p1->x)*dy/dx;
		ret->x=r->lu.x;
		break;
	case 1:
		ret->y=p1->y+(r->rl.x-p1->x)*dy/dx;
		ret->x=r->rl.x;
		break;
	case 2:
		ret->x=p1->x+(r->lu.y-p1->y)*dx/dy;
		ret->y=r->lu.y;
		break;
	case 3:
		ret->x=p1->x+(r->rl.y-p1->y)*dx/dy;
		ret->y=r->rl.y;
		break;
	}
}

static void
graphics_draw_polygon_clipped(struct graphics *gra, struct graphics_gc *gc, struct point *pin, int count_in)
{
	struct point_rect r=gra->r;
	struct point *pout,*p,*s,pi;
	struct point p1[count_in+1];
	struct point p2[count_in+1];
	int count_out,edge=3;
	int i;
#if 0
	r.lu.x+=20;
	r.lu.y+=20;
	r.rl.x-=20;
	r.rl.y-=20;
#endif

	pout=p1;
	for (edge = 0 ; edge < 4 ; edge++) {
		p=pin;
		s=pin+count_in-1;
		count_out=0;
		for (i = 0 ; i < count_in ; i++) {
			if (is_inside(p, &r, edge)) {
				if (! is_inside(s, &r, edge)) {
					poly_intersection(s,p,&r,edge,&pi);
					pout[count_out++]=pi;
				}
				pout[count_out++]=*p;
			} else {
				if (is_inside(s, &r, edge)) {
					poly_intersection(p,s,&r,edge,&pi);
					pout[count_out++]=pi;
				}
			}
			s=p;
			p++;
		}
		count_in=count_out;
		if (pin == p1) {
			pin=p2;
			pout=p1;
		} else {
			pin=p1;
			pout=p2;
		}
	}
	gra->meth.draw_polygon(gra->priv, gc->priv, pin, count_in);
}



/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
static void xdisplay_draw_elements(struct graphics *gra, GHashTable *display_list, struct itemgra *itm)
{
	struct element *e;
	GList *l,*ls,*es,*types;
	enum item_type type;
	struct graphics_gc *gc = NULL;
	struct graphics_image *img=NULL;
	struct point p;
	char path[PATH_MAX];
	struct point pa[16384];
	int width[16384];
	int count;

	es=itm->elements;
	while (es) {
		e=es->data;
		types=itm->type;
		while (types) {
			type=GPOINTER_TO_INT(types->data);
			ls=g_hash_table_lookup(display_list, GINT_TO_POINTER(type));
			l=ls;
			if (gc)
				graphics_gc_destroy(gc);
			if (img)
				graphics_image_free(gra, img);
			gc=NULL;
			img=NULL;
			while (l) {
				struct displayitem *di;
				di=l->data;
				di->displayed=1;
				if (! gc) {
					gc=graphics_gc_new(gra);
					gc->meth.gc_set_foreground(gc->priv, &e->color);
				}
				if (e->type == element_polyline) {
					count=transform(tg, pg, di->c, pa, di->count, 1, e->u.polyline.width, width);
				}
				else
					count=transform(tg, pg, di->c, pa, di->count, 1, 0, NULL);
				switch (e->type) {
				case element_polygon:
#if 0
					{
						int i;
						for (i = 0 ; i < count ; i++) {
							dbg(0,"pa[%d]=%d,%d\n", i, pa[i].x, pa[i].y);
						}
					}
					dbg(0,"element_polygon count=%d\n",count);
#endif
#if 1
					graphics_draw_polygon_clipped(gra, gc, pa, count);
#endif
					break;
				case element_polyline:
#if 0
					if (e->u.polyline.width > 1) {
						graphics_draw_polyline_as_polygon(gra, gc, pa, count, width, 0);
					} else {
#else
					{
						 if (e->u.polyline.width > 1)
		                                             gc->meth.gc_set_linewidth(gc->priv, e->u.polyline.width);

						
#endif
						if (e->u.polyline.width > 0 && e->u.polyline.dash_num > 0)
							graphics_gc_set_dashes(gc, e->u.polyline.width, 
									       e->u.polyline.offset,
									       e->u.polyline.dash_table,
									       e->u.polyline.dash_num);
#if 0
						if (di->label && !strcmp(di->label, "Bahnhofstr.") && di->item.type != type_street_1_city) {
							dbg(0,"0x%x,0x%x %s\n", di->item.id_hi, di->item.id_lo, item_to_name(di->item.type));
#endif
						graphics_draw_polyline_clipped(gra, gc, pa, count, width, 1, e->u.polyline.width > 1);
#if 0
						}
#endif
					}
					break;
				case element_circle:
					if (count) {
						if (e->u.circle.width > 1) 
							gc->meth.gc_set_linewidth(gc->priv, e->u.polyline.width);
						gra->meth.draw_circle(gra->priv, gc->priv, pa, e->u.circle.radius);
						if (di->label && e->text_size) {
							p.x=pa[0].x+3;
							p.y=pa[0].y+10;
							if (! gra->font[e->text_size])
								gra->font[e->text_size]=graphics_font_new(gra, e->text_size*20, 0);
							gra->meth.draw_text(gra->priv, gra->gc[2]->priv, gra->gc[1]->priv, gra->font[e->text_size]->priv, di->label, &p, 0x10000, 0);
						}
					}
					break;
				case element_text:
					if (count && di->label) {
						if (! gra->font[e->text_size])
							gra->font[e->text_size]=graphics_font_new(gra, e->text_size*20, 0);
						label_line(gra, gra->gc[2], gra->gc[1], gra->font[e->text_size], pa, count, di->label);
					}
					break;
				case element_icon:
					if (count) {
						if (!img) {
							if (e->u.icon.src[0] == '/')
								strcpy(path,e->u.icon.src);
							else
								sprintf(path,"%s/xpm/%s", navit_sharedir, e->u.icon.src);
							img=graphics_image_new_scaled_rotated(gra, path, e->u.icon.width, e->u.icon.height, e->u.icon.rotation);
							if (! img)
								dbg(0,"failed to load icon '%s'\n", e->u.icon.src);
						}
						if (img) {
							p.x=pa[0].x - img->hot.x;
							p.y=pa[0].y - img->hot.y;
							gra->meth.draw_image(gra->priv, gra->gc[0]->priv, &p, img->priv);
						}
					}
					break;
				case element_image:
					dbg(1,"image: '%s'\n", di->label);
					if (gra->meth.draw_image_warp)
						gra->meth.draw_image_warp(gra->priv, gra->gc[0]->priv, pa, count, di->label);
					else
						dbg(0,"draw_image_warp not supported by graphics driver drawing '%s'\n", di->label);
					break;
				case element_arrows:
					display_draw_arrows(gra,gc,pa,count);
					break;
				default:
					printf("Unhandled element type %d\n", e->type);
				
				}
				l=g_list_next(l);
			}
			types=g_list_next(types);
		}
		es=g_list_next(es);
	}
	if (gc)
		graphics_gc_destroy(gc);
	if (img)
		graphics_image_free(gra, img);
}

void
graphics_draw_itemgra(struct graphics *gra, struct itemgra *itm, struct transformation *t)
{
	GList *es;
	struct point p;
	struct coord c;
	char *label=NULL;
	struct graphics_gc *gc = NULL;
	struct graphics_image *img;
	char path[PATH_MAX];
	es=itm->elements;
	c.x=0;
	c.y=0;
	while (es) {
		struct element *e=es->data;
		int count=e->coord_count;
		struct point pnt[count+1];
		if (count)
			transform(t, projection_screen, e->coord, pnt, count, 0, 0, NULL);
		else {
			transform(t, projection_screen, &c, pnt, 1, 0, 0, NULL);
			count=1;
		}
		gc=graphics_gc_new(gra);
		gc->meth.gc_set_foreground(gc->priv, &e->color);
		switch (e->type) {
		case element_polyline:
			if (e->u.polyline.width > 1) 
				gc->meth.gc_set_linewidth(gc->priv, e->u.polyline.width);
			if (e->u.polyline.width > 0 && e->u.polyline.dash_num > 0)
				graphics_gc_set_dashes(gc, e->u.polyline.width, 
						       e->u.polyline.offset,
				                       e->u.polyline.dash_table,
				                       e->u.polyline.dash_num);
			gra->meth.draw_lines(gra->priv, gc->priv, pnt, count);
			break;
		case element_circle:
			if (e->u.circle.width > 1) 
				gc->meth.gc_set_linewidth(gc->priv, e->u.polyline.width);
			gra->meth.draw_circle(gra->priv, gc->priv, &pnt[0], e->u.circle.radius);
			if (label && e->text_size) {
				p.x=pnt[0].x+3;
				p.y=pnt[0].y+10;
			if (! gra->font[e->text_size])
				gra->font[e->text_size]=graphics_font_new(gra, e->text_size*20, 0);
				gra->meth.draw_text(gra->priv, gra->gc[2]->priv, gra->gc[1]->priv, gra->font[e->text_size]->priv, label, &p, 0x10000, 0);
			}
			break;
		case element_icon:
			if (e->u.icon.src[0] == '/') 
				strcpy(path,e->u.icon.src);
			else
				sprintf(path,"%s/xpm/%s", navit_sharedir, e->u.icon.src);
			img=graphics_image_new_scaled_rotated(gra, path, e->u.icon.width, e->u.icon.height, e->u.icon.rotation);
			if (! img)
				dbg(0,"failed to load icon '%s'\n", e->u.icon.src);
			else {
				p.x=pnt[0].x - img->hot.x;
				p.y=pnt[0].y - img->hot.y;
				gra->meth.draw_image(gra->priv, gc->priv, &p, img->priv);
				graphics_image_free(gra, img);
			}
			break;
		default:
			dbg(0,"dont know how to draw %d\n", e->type);
		}
		graphics_gc_destroy(gc);
		es=g_list_next(es);
	}
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
static void xdisplay_draw_layer(GHashTable *display_list, struct graphics *gra, struct layer *lay, int order)
{
	GList *itms;
	struct itemgra *itm;

	itms=lay->itemgras;
	while (itms) {
		itm=itms->data;
		if (order >= itm->order.min && order <= itm->order.max) 
			xdisplay_draw_elements(gra, display_list, itm);
		itms=g_list_next(itms);
	}
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
static void xdisplay_draw(GHashTable *display_list, struct graphics *gra, struct layout *l, int order)
{
	GList *lays;
	struct layer *lay;
	
	lays=l->layers;
	while (lays) {
		lay=lays->data;
		xdisplay_draw_layer(display_list, gra, lay, order);
		lays=g_list_next(lays);
	}
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
extern void *route_selection;

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
static void do_draw_map(struct displaylist *displaylist, struct transformation *t, struct map *m, int order)
{
	enum projection pro;
	struct map_rect *mr;
	struct item *item;
	int conv,count,max=16384;
	struct coord ca[max];
	struct attr attr;
	struct map_selection *sel;
	int num=0;

	pro=map_projection(m);
	conv=map_requires_conversion(m);
	sel=transform_get_selection(t, pro, order);
	tg=t;
	pg=pro;
#if 0
	sel=NULL;
#endif
	if (route_selection)
		mr=map_rect_new(m, route_selection);
	else
		mr=map_rect_new(m, sel);
	if (! mr) {
		map_selection_destroy(sel);
		return;
	}
	while ((item=map_rect_get_item(mr))) {
		num++;
#if 0
		if (num < 7599 || num > 7599)
			continue;
#endif
#if 0
		if (item->id_hi != 0xb0031 || item->id_lo != 0x20c9aeea)
			continue;
#endif
		count=item_coord_get(item, ca, item->type < type_line ? 1: max);
		if (item->type >= type_line && count < 2) {
			dbg(1,"poly from map has only %d points\n", count);
			continue;
		}
		if (item->type < type_line) {
			if (! map_selection_contains_point(sel, &ca[0])) {
				dbg(1,"point not visible\n");
				continue;
			}
		} else if (item->type < type_area) {
			if (! map_selection_contains_polyline(sel, ca, count)) {
				dbg(1,"polyline not visible\n");
				continue;
			}
		} else {
			if (! map_selection_contains_polygon(sel, ca, count)) {
				dbg(1,"polygon not visible\n");
				continue;
			}
		}
		if (count == max) 
			dbg(0,"point count overflow\n", count);
		if (item->type >= type_line && count < 2) {
			dbg(1,"poly from transform has only %d points\n", count);
			continue;
		}
		if (!item_attr_get(item, attr_label, &attr))
			attr.u.str=NULL;
		if (conv && attr.u.str && attr.u.str[0]) {
			char *str=map_convert_string(m, attr.u.str);
			display_add(displaylist, item, count, ca, str);
			map_convert_free(str);
		} else
			display_add(displaylist, item, count, ca, attr.u.str);
	}
	map_rect_destroy(mr);
	map_selection_destroy(sel);
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
static void do_draw(struct displaylist *displaylist, struct transformation *t, GList *mapsets, int order)
{
	struct mapset *ms;
	struct map *m;
	struct mapset_handle *h;

	if (! mapsets)
		return;
	ms=mapsets->data;
	h=mapset_open(ms);
	while ((m=mapset_next(h, 1))) {
		do_draw_map(displaylist, t, m, order);
	}
	mapset_close(h);
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
int graphics_ready(struct graphics *this_)
{
	return this_->ready;
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void graphics_displaylist_draw(struct graphics *gra, struct displaylist *displaylist, struct transformation *trans, struct layout *l, int callback)
{
	int order=transform_get_order(trans);
	struct point p;
	tg=trans;
	p.x=0;
	p.y=0;
	// FIXME find a better place to set the background color
	if (l) {
		graphics_gc_set_background(gra->gc[0], &l->color);
		graphics_gc_set_foreground(gra->gc[0], &l->color);
		gra->default_font = g_strdup(l->font);
	}
	graphics_background_gc(gra, gra->gc[0]);
	gra->meth.draw_mode(gra->priv, draw_mode_begin);
	gra->meth.draw_rectangle(gra->priv, gra->gc[0]->priv, &p, 32767, 32767);
	if (l) 
		xdisplay_draw(displaylist->dl, gra, l, order+l->order_delta);
	if (callback)
		callback_list_call_attr_0(gra->cbl, attr_postdraw);
	gra->meth.draw_mode(gra->priv, draw_mode_end);
}

#if 0
/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void graphics_displaylist_move(struct displaylist *displaylist, int dx, int dy)
{
	struct displaylist_handle *dlh;
	struct displayitem *di;
	int i;

	dlh=graphics_displaylist_open(displaylist);
	while ((di=graphics_displaylist_next(dlh))) {
		for (i = 0 ; i < di->count ; i++) {
			di->pnt[i].x+=dx;
			di->pnt[i].y+=dy;
		}
	}
	graphics_displaylist_close(dlh);
}
#endif

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void graphics_draw(struct graphics *gra, struct displaylist *displaylist, GList *mapsets, struct transformation *trans, struct layout *l)
{
	int order=transform_get_order(trans);

	dbg(1,"enter");

#if 0
	printf("scale=%d center=0x%x,0x%x mercator scale=%f\n", scale, co->trans->center.x, co->trans->center.y, transform_scale(co->trans->center.y));
#endif
	
	xdisplay_free(displaylist->dl);
	dbg(1,"order=%d\n", order);


#if 0
	for (i = 0 ; i < data_window_type_end; i++) {
		data_window_begin(co->data_window[i]);	
	}
#endif
	profile(0,NULL);
	if (l)
		order+=l->order_delta;
	if (mapsets)
		do_draw(displaylist, trans, mapsets, order);
//	profile(1,"do_draw");
	graphics_displaylist_draw(gra, displaylist, trans, l, 1);
	profile(1,"xdisplay_draw");
	profile(0,"end");
  
#if 0
	for (i = 0 ; i < data_window_type_end; i++) {
		data_window_end(co->data_window[i]);	
	}
#endif
	gra->ready=1;
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
struct displaylist_handle {
	GList *hl_head,*hl,*l;
};

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
struct displaylist_handle * graphics_displaylist_open(struct displaylist *displaylist)
{
	struct displaylist_handle *ret;

	ret=g_new0(struct displaylist_handle, 1);
	ret->hl_head=ret->hl=g_hash_to_list(displaylist->dl);

	return ret;
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
struct displayitem * graphics_displaylist_next(struct displaylist_handle *dlh)
{
	struct displayitem *ret;
	if (! dlh->l) {
		if (!dlh->hl)
			return NULL;
		dlh->l=dlh->hl->data;
		dlh->hl=g_list_next(dlh->hl);
	}
	ret=dlh->l->data;
	dlh->l=g_list_next(dlh->l);
	return ret;
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void graphics_displaylist_close(struct displaylist_handle *dlh)
{
	g_list_free(dlh->hl_head);
	g_free(dlh);
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
struct displaylist * graphics_displaylist_new(void)
{
	struct displaylist *ret=g_new(struct displaylist, 1);

	ret->dl=g_hash_table_new(NULL,NULL);

	return ret;
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
struct item * graphics_displayitem_get_item(struct displayitem *di)
{
	return &di->item;	
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
char * graphics_displayitem_get_label(struct displayitem *di)
{
	return di->label;
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
static int within_dist_point(struct point *p0, struct point *p1, int dist)
{
	if (p0->x == 32767 || p0->y == 32767 || p1->x == 32767 || p1->y == 32767)
		return 0;
	if (p0->x == -32768 || p0->y == -32768 || p1->x == -32768 || p1->y == -32768)
		return 0;
        if ((p0->x-p1->x)*(p0->x-p1->x) + (p0->y-p1->y)*(p0->y-p1->y) <= dist*dist) {
                return 1;
        }
        return 0;
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
static int within_dist_line(struct point *p, struct point *line_p0, struct point *line_p1, int dist)
{
	int vx,vy,wx,wy;
	int c1,c2;
	struct point line_p;

	vx=line_p1->x-line_p0->x;
	vy=line_p1->y-line_p0->y;
	wx=p->x-line_p0->x;
	wy=p->y-line_p0->y;

	c1=vx*wx+vy*wy;
	if ( c1 <= 0 )
		return within_dist_point(p, line_p0, dist);
	c2=vx*vx+vy*vy;
	if ( c2 <= c1 )
		return within_dist_point(p, line_p1, dist);

	line_p.x=line_p0->x+vx*c1/c2;
	line_p.y=line_p0->y+vy*c1/c2;
	return within_dist_point(p, &line_p, dist);
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
static int within_dist_polyline(struct point *p, struct point *line_pnt, int count, int dist, int close)
{
	int i;
	for (i = 0 ; i < count-1 ; i++) {
		if (within_dist_line(p,line_pnt+i,line_pnt+i+1,dist)) {
			return 1;
		}
	}
	if (close)
		return (within_dist_line(p,line_pnt,line_pnt+count-1,dist));
	return 0;
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
static int within_dist_polygon(struct point *p, struct point *poly_pnt, int count, int dist)
{
	int i, j, c = 0;
        for (i = 0, j = count-1; i < count; j = i++) {
		if ((((poly_pnt[i].y <= p->y) && ( p->y < poly_pnt[j].y )) ||
		((poly_pnt[j].y <= p->y) && ( p->y < poly_pnt[i].y))) &&
		(p->x < (poly_pnt[j].x - poly_pnt[i].x) * (p->y - poly_pnt[i].y) / (poly_pnt[j].y - poly_pnt[i].y) + poly_pnt[i].x)) 
                        c = !c;
        }
	if (! c)
		return within_dist_polyline(p, poly_pnt, count, dist, 1);
        return c;
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
int graphics_displayitem_within_dist(struct displayitem *di, struct point *p, int dist)
{
	struct point pa[16384];
	int count;

	count=transform(tg, pg, di->c, pa, di->count, 1, 0, NULL);
	
	if (di->item.type < type_line) {
		return within_dist_point(p, &pa[0], dist);
	}
	if (di->item.type < type_area) {
		return within_dist_polyline(p, pa, count, dist, 0);
	}
	return within_dist_polygon(p, pa, count, dist);
}
