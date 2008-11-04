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
};


struct displaylist {
	GHashTable *dl;
};

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
struct graphics * graphics_overlay_new(struct graphics *parent, struct point *p, int w, int h)
{
	struct graphics *this_;
	this_=g_new0(struct graphics, 1);
	this_->priv=parent->meth.overlay_new(parent->priv, &this_->meth, p, w, h);
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
	this_->meth.background_gc(this_->priv, this_->gc[0]->priv);
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
	this_->priv=gra->meth.image_new(gra->priv, &this_->meth, path, &this_->width, &this_->height, &this_->hot);
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
void graphics_get_text_bbox(struct graphics *this_, struct graphics_font *font, char *text, int dx, int dy, struct point *ret)
{
	this_->meth.get_text_bbox(this_->priv, font->priv, text, dx, dy, ret);
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
	struct point pnt[0];
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
void display_add(struct displaylist *displaylist, struct item *item, int count, struct point *pnt, char *label)
{
	struct displayitem *di;
	int len;
	GList *l;
	char *p;

	len=sizeof(*di)+count*sizeof(*pnt);
	if (label)
		len+=strlen(label)+1;

	p=g_malloc(len);

	di=(struct displayitem *)p;
	di->displayed=0;
	p+=sizeof(*di)+count*sizeof(*pnt);
	di->item=*item;
	if (label) {
		di->label=p;
		strcpy(di->label, label);
	} else 
		di->label=NULL;
	di->count=count;
	memcpy(di->pnt, pnt, count*sizeof(*pnt));

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
	int i,x,y,tl;
	double dx,dy,l;
	struct point p_t;

	tl=strlen(label)*400;
	for (i = 0 ; i < count-1 ; i++) {
		dx=p[i+1].x-p[i].x;
		dx*=100;
		dy=p[i+1].y-p[i].y;
		dy*=100;
		l=(int)sqrt((float)(dx*dx+dy*dy));
		if (l > tl) {
			x=p[i].x;
			y=p[i].y;
			if (dx < 0) {
				dx=-dx;
				dy=-dy;
				x=p[i+1].x;
				y=p[i+1].y;
			}
			x+=(l-tl)*dx/l/200;
			y+=(l-tl)*dy/l/200;
			x-=dy*45/l/10;
			y+=dx*45/l/10;
			p_t.x=x;
			p_t.y=y;
	#if 0
			printf("display_text: '%s', %d, %d, %d, %d %d\n", label, x, y, dx*0x10000/l, dy*0x10000/l, l);
	#endif
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

static void display_draw_arrows(struct displayitem *di, struct graphics_gc *gc, struct graphics *gra)
{
	int i,dx,dy,l;
	struct point p;
	for (i = 0 ; i < di->count-1 ; i++) {
		dx=di->pnt[i+1].x-di->pnt[i].x;	
		dy=di->pnt[i+1].y-di->pnt[i].y;
		l=sqrt(dx*dx+dy*dy);
		if (l) {
			dx=dx*65536/l;
			dy=dy*65536/l;
			p=di->pnt[i];
			p.x+=dx*15/65536;
			p.y+=dy*15/65536;
			display_draw_arrow(&p, dx, dy, 10, gc, gra);
			p=di->pnt[i+1];
			p.x-=dx*15/65536;
			p.y-=dy*15/65536;
			display_draw_arrow(&p, dx, dy, 10, gc, gra);
		}
	}
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
	struct graphics_image *img;
	struct point p;
	char path[PATH_MAX];

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
				switch (e->type) {
				case element_polygon:
					gra->meth.draw_polygon(gra->priv, gc->priv, di->pnt, di->count);
					break;
				case element_polyline:
					if (e->u.polyline.width > 1) 
						gc->meth.gc_set_linewidth(gc->priv, e->u.polyline.width);
					if (e->u.polyline.width > 0 && e->u.polyline.dash_num > 0)
						graphics_gc_set_dashes(gc, e->u.polyline.width, 
								       e->u.polyline.offset,
						                       e->u.polyline.dash_table,
						                       e->u.polyline.dash_num);
					gra->meth.draw_lines(gra->priv, gc->priv, di->pnt, di->count);
					break;
				case element_circle:
					if (e->u.circle.width > 1) 
						gc->meth.gc_set_linewidth(gc->priv, e->u.polyline.width);
					gra->meth.draw_circle(gra->priv, gc->priv, &di->pnt[0], e->u.circle.radius);
					if (di->label && e->text_size) {
						p.x=di->pnt[0].x+3;
						p.y=di->pnt[0].y+10;
						if (! gra->font[e->text_size])
							gra->font[e->text_size]=graphics_font_new(gra, e->text_size*20, 0);
						gra->meth.draw_text(gra->priv, gra->gc[2]->priv, gra->gc[1]->priv, gra->font[e->text_size]->priv, di->label, &p, 0x10000, 0);
					}
					break;
				case element_text:
					if (di->label) {
						if (! gra->font[e->text_size])
							gra->font[e->text_size]=graphics_font_new(gra, e->text_size*20, 0);
						label_line(gra, gra->gc[2], gra->gc[1], gra->font[e->text_size], di->pnt, di->count, di->label);
					}
					break;
				case element_icon:
					if (!img) {
						sprintf(path,"%s/xpm/%s", navit_sharedir, e->u.icon.src);
						img=graphics_image_new_scaled(gra, path, e->u.icon.width, e->u.icon.height);
						if (! img)
							dbg(0,"failed to load icon '%s'\n", e->u.icon.src);
					}
					if (img) {
						p.x=di->pnt[0].x - img->hot.x;
						p.y=di->pnt[0].y - img->hot.y;
						gra->meth.draw_image(gra->priv, gra->gc[0]->priv, &p, img->priv);
						graphics_image_free(gra, img);
						img = NULL;
					}
					break;
				case element_image:
					dbg(1,"image: '%s'\n", di->label);
					if (gra->meth.draw_image_warp)
						gra->meth.draw_image_warp(gra->priv, gra->gc[0]->priv, di->pnt, di->count, di->label);
					else
						dbg(0,"draw_image_warp not supported by graphics driver drawing '%s'\n", di->label);
					break;
				case element_arrows:
					display_draw_arrows(di,gc,gra);
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
	struct point pnt[max];
	struct coord ca[max];
	struct attr attr;
	struct map_selection *sel;

	pro=map_projection(m);
	conv=map_requires_conversion(m);
	sel=transform_get_selection(t, pro, order);
	if (route_selection)
		mr=map_rect_new(m, route_selection);
	else
		mr=map_rect_new(m, sel);
	if (! mr) {
		map_selection_destroy(sel);
		return;
	}
	while ((item=map_rect_get_item(mr))) {
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
		count=transform(t, pro, ca, pnt, count, 1);
		if (item->type >= type_line && count < 2) {
			dbg(1,"poly from transform has only %d points\n", count);
			continue;
		}
		if (!item_attr_get(item, attr_label, &attr))
			attr.u.str=NULL;
		if (conv && attr.u.str && attr.u.str[0]) {
			char *str=map_convert_string(m, attr.u.str);
			display_add(displaylist, item, count, pnt, str);
			map_convert_free(str);
		} else
			display_add(displaylist, item, count, pnt, attr.u.str);
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
	p.x=0;
	p.y=0;
	// FIXME find a better place to set the background color
	graphics_gc_set_background(gra->gc[0], &l->color);
	graphics_gc_set_foreground(gra->gc[0], &l->color);
	gra->default_font = g_strdup(l->font);
	gra->meth.background_gc(gra->priv, gra->gc[0]->priv);
	gra->meth.draw_mode(gra->priv, draw_mode_begin);
	gra->meth.draw_rectangle(gra->priv, gra->gc[0]->priv, &p, 32767, 32767);
	xdisplay_draw(displaylist->dl, gra, l, order+l->order_delta);
	if (callback)
		callback_list_call_attr_0(gra->cbl, attr_postdraw);
	gra->meth.draw_mode(gra->priv, draw_mode_end);
}

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
	order+=l->order_delta;
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
	if (di->item.type < type_line) {
		return within_dist_point(p, &di->pnt[0], dist);
	}
	if (di->item.type < type_area) {
		return within_dist_polyline(p, di->pnt, di->count, dist, 0);
	}
	return within_dist_polygon(p, di->pnt, di->count, dist);
}
