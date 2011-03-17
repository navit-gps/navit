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
#include "config.h"
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
#include "event.h"


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
	int font_len;
	struct graphics_font **font;
	struct graphics_gc *gc[3];
	struct attr **attrs;
	struct callback_list *cbl;
	struct point_rect r;
	int gamma,brightness,contrast;
	int colormgmt;
	int font_size;
	GList *selection;
};

struct display_context
{
	struct graphics *gra;
	struct element *e;
	struct graphics_gc *gc;
	struct graphics_gc *gc_background;
	struct graphics_image *img;
	enum projection pro;
	int mindist;
	struct transformation *trans;
	enum item_type type;
	int maxlen;
};

#define HASH_SIZE 1024
struct hash_entry
{
	enum item_type type;
	struct displayitem *di;
};


struct displaylist {
	int busy;
	int workload;
	struct callback *cb;
	struct layout *layout, *layout_hashed;
	struct display_context dc;
	int order, order_hashed, max_offset;
	struct mapset *ms;
	struct mapset_handle *msh;
	struct map *m;
	int conv;
	struct map_selection *sel;
	struct map_rect *mr;
	struct callback *idle_cb;
	struct event_idle *idle_ev;
	unsigned int seq;
	struct hash_entry hash_entries[HASH_SIZE];
};


struct displaylist_icon_cache {
	unsigned int seq;

};

static void draw_circle(struct point *pnt, int diameter, int scale, int start, int len, struct point *res, int *pos, int dir);
static void graphics_process_selection(struct graphics *gra, struct displaylist *dl);
static void graphics_gc_init(struct graphics *this_);

static void
clear_hash(struct displaylist *dl)
{
	int i;
	for (i = 0 ; i < HASH_SIZE ; i++)
		dl->hash_entries[i].type=type_none;
}

static struct hash_entry *
get_hash_entry(struct displaylist *dl, enum item_type type)
{
	int hashidx=(type*2654435761UL) & (HASH_SIZE-1);
	int offset=dl->max_offset;
	do {
		if (!dl->hash_entries[hashidx].type)
			return NULL;
		if (dl->hash_entries[hashidx].type == type)
			return &dl->hash_entries[hashidx];
		hashidx=(hashidx+1)&(HASH_SIZE-1);
	} while (offset-- > 0);
	return NULL;
}

static struct hash_entry *
set_hash_entry(struct displaylist *dl, enum item_type type)
{
	int hashidx=(type*2654435761UL) & (HASH_SIZE-1);
	int offset=0;
	for (;;) {
		if (!dl->hash_entries[hashidx].type) {
			dl->hash_entries[hashidx].type=type;
			if (dl->max_offset < offset)
				dl->max_offset=offset;
			return &dl->hash_entries[hashidx];
		}
		if (dl->hash_entries[hashidx].type == type)
			return &dl->hash_entries[hashidx];
		hashidx=(hashidx+1)&(HASH_SIZE-1);
		offset++;
	}
	return NULL;
}

static int
graphics_set_attr_do(struct graphics *gra, struct attr *attr)
{
	switch (attr->type) {
	case attr_gamma:
		gra->gamma=attr->u.num;
		break;
	case attr_brightness:
		gra->brightness=attr->u.num;
		break;
	case attr_contrast:
		gra->contrast=attr->u.num;
		break;
	case attr_font_size:
		gra->font_size=attr->u.num;
		return 1;
	default:
		return 0;
	}
	gra->colormgmt=(gra->gamma != 65536 || gra->brightness != 0 || gra->contrast != 65536);
	graphics_gc_init(gra);
	return 1;
}

int
graphics_set_attr(struct graphics *gra, struct attr *attr)
{
	int ret=1;
	dbg(0,"enter\n");
	if (gra->meth.set_attr)
		ret=gra->meth.set_attr(gra->priv, attr);
	if (!ret)
		ret=graphics_set_attr_do(gra, attr);
        return ret != 0;
}

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
	this_->brightness=0;
	this_->contrast=65536;
	this_->gamma=65536;
	this_->font_size=20;
	while (*attrs) {
		graphics_set_attr_do(this_,*attrs);
		attrs++;
	}
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
	struct point_rect pr;
	if (!parent->meth.overlay_new)
		return NULL;
	this_=g_new0(struct graphics, 1);
	this_->priv=parent->meth.overlay_new(parent->priv, &this_->meth, p, w, h, alpha, wraparound);
	pr.lu.x=0;
	pr.lu.y=0;
	pr.rl.x=w;
	pr.rl.y=h;
	this_->font_size=20;
	graphics_set_rect(this_, &pr);
	if (!this_->priv) {
		g_free(this_);
		this_=NULL;
	}
	return this_;
}

/**
 * @brief Alters the size, position, alpha and wraparound for an overlay
 *
 * @param this_ The overlay's graphics struct
 * @param p The new position of the overlay
 * @param w The new width of the overlay
 * @param h The new height of the overlay
 * @param alpha The new alpha of the overlay
 * @param wraparound The new wraparound of the overlay
 */
void
graphics_overlay_resize(struct graphics *this_, struct point *p, int w, int h, int alpha, int wraparound)
{
	if (! this_->meth.overlay_resize) {
		return;
	}

	this_->meth.overlay_resize(this_->priv, p, w, h, alpha, wraparound);
}

static void
graphics_gc_init(struct graphics *this_)
{
	struct color background={ COLOR_BACKGROUND_ };
	struct color black={ COLOR_BLACK_ };
	struct color white={ COLOR_WHITE_ };
	if (!this_->gc[0] || !this_->gc[1] || !this_->gc[2])
		return;
	graphics_gc_set_background(this_->gc[0], &background );
	graphics_gc_set_foreground(this_->gc[0], &background );
	graphics_gc_set_background(this_->gc[1], &black );
	graphics_gc_set_foreground(this_->gc[1], &white );
	graphics_gc_set_background(this_->gc[2], &white );
	graphics_gc_set_foreground(this_->gc[2], &black );
}



/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void graphics_init(struct graphics *this_)
{
	if (this_->gc[0])
		return;
	this_->gc[0]=graphics_gc_new(this_);
	this_->gc[1]=graphics_gc_new(this_);
	this_->gc[2]=graphics_gc_new(this_);
	graphics_gc_init(this_);
	graphics_background_gc(this_, this_->gc[0]);
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void * graphics_get_data(struct graphics *this_, const char *type)
{
	return (this_->meth.get_data(this_->priv, type));
}

void graphics_add_callback(struct graphics *this_, struct callback *cb)
{
	callback_list_add(this_->cbl, cb);
}

void graphics_remove_callback(struct graphics *this_, struct callback *cb)
{
	callback_list_remove(this_->cbl, cb);
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

struct graphics_font * graphics_named_font_new(struct graphics *gra, char *font, int size, int flags)
{
	struct graphics_font *this_;

	this_=g_new0(struct graphics_font,1);
	this_->priv=gra->meth.font_new(gra->priv, &this_->meth, font, size, flags);
	return this_;
}


/**
 * Destroy graphics
 * Called when navit exits
 * @param gra The graphics instance
 * @returns nothing
 * @author David Tegze (02/2011)
 */
void graphics_free(struct graphics *gra)
{
	gra->meth.graphics_destroy(gra->priv);
	g_free(gra->default_font);
	graphics_font_destroy_all(gra);
	g_free(gra);
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
	for(i = 0 ; i < gra->font_len; i++) {
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
	this_->gra=gra;
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

static void
graphics_convert_color(struct graphics *gra, struct color *in, struct color *out)
{
	*out=*in;
	if (gra->brightness) {
		out->r+=gra->brightness;
		out->g+=gra->brightness;
		out->b+=gra->brightness;
	}
	if (gra->contrast != 65536) {
		out->r=out->r*gra->contrast/65536;
		out->g=out->g*gra->contrast/65536;
		out->b=out->b*gra->contrast/65536;
	}
	if (out->r < 0)
		out->r=0;
	if (out->r > 65535)
		out->r=65535;
	if (out->g < 0)
		out->g=0;
	if (out->g > 65535)
		out->g=65535;
	if (out->b < 0)
		out->b=0;
	if (out->b > 65535)
		out->b=65535;
	if (gra->gamma != 65536) {
		out->r=pow(out->r/65535.0,gra->gamma/65536.0)*65535.0;
		out->g=pow(out->g/65535.0,gra->gamma/65536.0)*65535.0;
		out->b=pow(out->b/65535.0,gra->gamma/65536.0)*65535.0;
	}
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void graphics_gc_set_foreground(struct graphics_gc *gc, struct color *c)
{
	struct color cn;
	if (gc->gra->colormgmt) {
		graphics_convert_color(gc->gra, c, &cn);
		c=&cn;
	}
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
	struct color cn;
	if (gc->gra->colormgmt) {
		graphics_convert_color(gc->gra, c, &cn);
		c=&cn;
	}
	gc->meth.gc_set_background(gc->priv, c);
}


/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void graphics_gc_set_stipple(struct graphics_gc *gc, struct graphics_image *img)
{
	gc->meth.gc_set_stipple(gc->priv, img ? img->priv : NULL);
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
	struct point *pnt=g_alloca(sizeof(struct point)*(r*4+64));
	int i=0;

	if(this_->meth.draw_circle)
		this_->meth.draw_circle(this_->priv, gc->priv, p, r);
	else
	{
		draw_circle(p, r, 0, -1, 1026, pnt, &i, 1);
		pnt[i] = pnt[0];
		i++;
		this_->meth.draw_lines(this_->priv, gc->priv, pnt, i);
	}
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

void graphics_draw_rectangle_rounded(struct graphics *this_, struct graphics_gc *gc, struct point *plu, int w, int h, int r, int fill)
{
	struct point *p=g_alloca(sizeof(struct point)*(r*4+32));
	struct point pi0={plu->x+r,plu->y+r};
	struct point pi1={plu->x+w-r,plu->y+r};
	struct point pi2={plu->x+w-r,plu->y+h-r};
	struct point pi3={plu->x+r,plu->y+h-r};
	int i=0;

	draw_circle(&pi2, r*2, 0, -1, 258, p, &i, 1);
	draw_circle(&pi1, r*2, 0, 255, 258, p, &i, 1);
	draw_circle(&pi0, r*2, 0, 511, 258, p, &i, 1);
	draw_circle(&pi3, r*2, 0, 767, 258, p, &i, 1);
	p[i]=p[0];
	i++;
	if (fill)
		this_->meth.draw_polygon(this_->priv, gc->priv, p, i);
	else
		this_->meth.draw_lines(this_->priv, gc->priv, p, i);
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
	struct displayitem *next;
	struct item item;
	char *label;
	int count;
	struct coord c[0];
};

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
static void xdisplay_free(struct displaylist *dl)
{
	int i;
	for (i = 0 ; i < HASH_SIZE ; i++) {
		struct displayitem *di=dl->hash_entries[i].di;
		while (di) {
			struct displayitem *next=di->next;
			g_free(di);
			di=next;
		}
		dl->hash_entries[i].di=NULL;
	}
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
static void display_add(struct hash_entry *entry, struct item *item, int count, struct coord *c, char **label, int label_count)
{
	struct displayitem *di;
	int len,i;
	char *p;

	len=sizeof(*di)+count*sizeof(*c);
	if (label && label_count) {
		for (i = 0 ; i < label_count ; i++) {
			if (label[i])
				len+=strlen(label[i])+1;
			else
				len++;
		}
	}
	p=g_malloc(len);

	di=(struct displayitem *)p;
	p+=sizeof(*di)+count*sizeof(*c);
	di->item=*item;
	if (label && label_count) {
		di->label=p;
		for (i = 0 ; i < label_count ; i++) {
			if (label[i]) {
				strcpy(p, label[i]);
				p+=strlen(label[i])+1;
			} else
				*p++='\0';
		}
	} else
		di->label=NULL;
	di->count=count;
	memcpy(di->c, c, count*sizeof(*c));
	di->next=entry->di;
	entry->di=di;
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
	tlm=tl*32;
	thm=th*36;
	tlsq = tlm*tlm;
	for (i = 0 ; i < count-1 ; i++) {
		dx=p[i+1].x-p[i].x;
		dx*=32;
		dy=p[i+1].y-p[i].y;
		dy*=32;
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
			x+=(l-tlm)*dx/l/64;
			y+=(l-tlm)*dy/l/64;
			x-=dy*thm/l/64;
			y+=dx*thm/l/64;
			p_t.x=x;
			p_t.y=y;
#if 0
			dbg(0,"display_text: '%s', %d, %d, %d, %d %d\n", label, x, y, dx*0x10000/l, dy*0x10000/l, l);
#endif
			if (x < gra->r.rl.x && x + tl > gra->r.lu.x && y + tl > gra->r.lu.y && y - tl < gra->r.rl.y)
				gra->meth.draw_text(gra->priv, fg->priv, bg?bg->priv:NULL, font->priv, label, &p_t, dx*0x10000/l, dy*0x10000/l);
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
	short x,y,fowler;
} circle64[]={
{0,128,0},
{13,127,13},
{25,126,25},
{37,122,38},
{49,118,53},
{60,113,67},
{71,106,85},
{81,99,104},
{91,91,128},
{99,81,152},
{106,71,171},
{113,60,189},
{118,49,203},
{122,37,218},
{126,25,231},
{127,13,243},
{128,0,256},
{127,-13,269},
{126,-25,281},
{122,-37,294},
{118,-49,309},
{113,-60,323},
{106,-71,341},
{99,-81,360},
{91,-91,384},
{81,-99,408},
{71,-106,427},
{60,-113,445},
{49,-118,459},
{37,-122,474},
{25,-126,487},
{13,-127,499},
{0,-128,512},
{-13,-127,525},
{-25,-126,537},
{-37,-122,550},
{-49,-118,565},
{-60,-113,579},
{-71,-106,597},
{-81,-99,616},
{-91,-91,640},
{-99,-81,664},
{-106,-71,683},
{-113,-60,701},
{-118,-49,715},
{-122,-37,730},
{-126,-25,743},
{-127,-13,755},
{-128,0,768},
{-127,13,781},
{-126,25,793},
{-122,37,806},
{-118,49,821},
{-113,60,835},
{-106,71,853},
{-99,81,872},
{-91,91,896},
{-81,99,920},
{-71,106,939},
{-60,113,957},
{-49,118,971},
{-37,122,986},
{-25,126,999},
{-13,127,1011},
};

static void
draw_circle(struct point *pnt, int diameter, int scale, int start, int len, struct point *res, int *pos, int dir)
{
	struct circle *c;

#if 0
	dbg(0,"diameter=%d start=%d len=%d pos=%d dir=%d\n", diameter, start, len, *pos, dir);
#endif
	int count=64;
	int end=start+len;
	int i,step;
	c=circle64;
	if (diameter > 128)
		step=1;
	else if (diameter > 64)
		step=2;
	else if (diameter > 24)
		step=4;
	else if (diameter > 8)
		step=8;
	else
		step=16;
	if (len > 0) {
		while (start < 0) {
			start+=1024;
			end+=1024;
		}
		while (end > 0) {
			i=0;
			while (i < count && c[i].fowler <= start)
				i+=step;
			while (i < count && c[i].fowler < end) {
				if (1< *pos || 0<dir) {
					res[*pos].x=pnt->x+((c[i].x*diameter+128)>>8);
					res[*pos].y=pnt->y+((c[i].y*diameter+128)>>8);
					(*pos)+=dir;
				}
				i+=step;
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
			i=count-1;
			while (i >= 0 && c[i].fowler >= start)
				i-=step;
			while (i >= 0 && c[i].fowler > end) {
				if (1< *pos || 0<dir) {
					res[*pos].x=pnt->x+((c[i].x*diameter+128)>>8);
					res[*pos].y=pnt->y+((c[i].y*diameter+128)>>8);
					(*pos)+=dir;
				}
				i-=step;
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

	/* avoid q rollover */
	if(n >= (1<<(sizeof(n)*8-2))) {
		q = 1<<(sizeof(n)*8-2);
	} else {
		while ( q <= n ) {
			q <<= 2;
		}
		q >>= 2;
	}

	while ( q != 0 ) {
		h = p + q;
		p >>= 1;
		if ( r >= h ) {
			p += q;
			r -= h;
		}
		q >>= 2;
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
	struct point *res=g_alloca(sizeof(struct point)*maxpoints);
	struct point pos, poso, neg, nego;
	int i, dx=0, dy=0, l=0, dxo=0, dyo=0;
	struct offset o,oo={};
	int fow=0, fowo=0, delta;
	int wi, ppos = maxpoints/2, npos = maxpoints/2;
	int state,prec=5;
	int max_circle_points=20;
	int lscale=16;
	i=0;
	for (;;) {
		wi=*width;
		width+=step;
		if (i < count - 1) {
			int dxs,dys,lscales;

			dx = (pnt[i + 1].x - pnt[i].x);
			dy = (pnt[i + 1].y - pnt[i].y);
#if 0
			l = int_sqrt(dx * dx * lscale * lscale + dy * dy * lscale * lscale);
#else
			dxs=dx*dx;
                       dys=dy*dy;
                       lscales=lscale*lscale;
                       if (dxs + dys > lscales)
                               l = int_sqrt(dxs+dys)*lscale;
                       else
                               l = int_sqrt((dxs+dys)*lscales);
#endif
			fow=fowler(-dy, dx);
		}
		if (! l)
			l=1;
		if (wi*lscale > 10000)
			lscale=10000/wi;
		dbg_assert(wi*lscale <= 10000);
		calc_offsets(wi*lscale, l, dx, dy, &o);
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
			dbg_assert(npos > 0);
			dbg_assert(ppos < maxpoints);
			gra->meth.draw_polygon(gra->priv, gc->priv, res+npos, ppos-npos);
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
		i++;
		if (i >= count)
			break;
		if (step) {
			wi=*width;
			calc_offsets(wi*lscale, l, dx, dy, &oo);
		} else
			oo=o;
		dxo = -dx;
		dyo = -dy;
		fowo=fow;
	}
}


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
	struct point *p=g_alloca(sizeof(struct point)*(count+1));
	int *w=g_alloca(sizeof(int)*(count*step+1));
	struct wpoint p1,p2;
	int i,code,out=0;
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
			if (((code == 1 || code == 5) && i == 1) || (code & 2)) {
				p[out].x=p1.x;
				p[out].y=p1.y;
				w[out*step]=p1.w;
				out++;
			}
			if (code) {
				p[out].x=p2.x;
				p[out].y=p2.y;
				w[out*step]=p2.w;
				out++;
			}
			if (i == count-1 || (code & 4)) {
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
	default:
		return 0;
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
	struct point *pout,*p,*s,pi,*p1,*p2;
	int limit=10000;
	struct point *pa1=g_alloca(sizeof(struct point) * (count_in < limit ? count_in*8+1:0));
	struct point *pa2=g_alloca(sizeof(struct point) * (count_in < limit ? count_in*8+1:0));
	int count_out,edge=3;
	int i;
#if 0
	r.lu.x+=20;
	r.lu.y+=20;
	r.rl.x-=20;
	r.rl.y-=20;
#endif
	if (count_in < limit) {
		p1=pa1;
		p2=pa2;
	} else {
		p1=g_new(struct point, count_in*8+1);
		p2=g_new(struct point, count_in*8+1);
	}

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
	if (count_in >= limit) {
		g_free(p1);
		g_free(p2);
	}
}


static void
display_context_free(struct display_context *dc)
{
	if (dc->gc)
		graphics_gc_destroy(dc->gc);
	if (dc->gc_background)
		graphics_gc_destroy(dc->gc_background);
	if (dc->img)
		graphics_image_free(dc->gra, dc->img);
	dc->gc=NULL;
	dc->gc_background=NULL;
	dc->img=NULL;
}

static struct graphics_font *
get_font(struct graphics *gra, int size)
{
	if (size > 64)
		size=64;
	if (size >= gra->font_len) {
		gra->font=g_renew(struct graphics_font *, gra->font, size+1);
		while (gra->font_len <= size)
			gra->font[gra->font_len++]=NULL;
	}
	if (! gra->font[size])
		gra->font[size]=graphics_font_new(gra, size*gra->font_size, 0);
	return gra->font[size];
}

void graphics_draw_text_std(struct graphics *this_, int text_size, char *text, struct point *p)
{
	struct graphics_font *font=get_font(this_, text_size);
	struct point bbox[4];
	int i;

	graphics_get_text_bbox(this_, font, text, 0x10000, 0, bbox, 0);
	for (i = 0 ; i < 4 ; i++) {
		bbox[i].x+=p->x;
		bbox[i].y+=p->y;
	}
	graphics_draw_rectangle(this_, this_->gc[2], &bbox[1], bbox[2].x-bbox[0].x, bbox[0].y-bbox[1].y+5);
	graphics_draw_text(this_, this_->gc[1], this_->gc[2], font, text, p, 0x10000, 0);
}

char *
graphics_icon_path(char *icon)
{
	static char *navit_sharedir;
	char *ret=NULL;
	struct file_wordexp *wordexp=NULL;
	dbg(1,"enter %s\n",icon);
	if (strchr(icon, '$')) {
		wordexp=file_wordexp_new(icon);
		if (file_wordexp_get_count(wordexp))
			icon=file_wordexp_get_array(wordexp)[0];
	}
	if (icon[0] == '/')
		ret=g_strdup(icon);
	else {
#ifdef HAVE_API_ANDROID
		// get resources for the correct screen density
		//
		// this part not needed, android unpacks only the correct version into res/drawable dir!
		// dbg(1,"android icon_path %s\n",icon);
		// static char *android_density;
		// android_density = getenv("ANDROID_DENSITY");
		// ret=g_strdup_printf("res/drawable-%s/%s",android_density ,icon);
		ret=g_strdup_printf("res/drawable/%s" ,icon);
#else
		if (! navit_sharedir)
			navit_sharedir = getenv("NAVIT_SHAREDIR");
		ret=g_strdup_printf("%s/xpm/%s", navit_sharedir, icon);
#endif
	}
	if (wordexp)
		file_wordexp_destroy(wordexp);
	return ret;
}

static int
limit_count(struct coord *c, int count)
{
	int i;
	for (i = 1 ; i < count ; i++) {
		if (c[i].x == c[0].x && c[i].y == c[0].y)
			return i+1;
	}
	return count;
}


static void
displayitem_draw(struct displayitem *di, void *dummy, struct display_context *dc)
{
	int *width=g_alloca(sizeof(int)*dc->maxlen);
	struct point *pa=g_alloca(sizeof(struct point)*dc->maxlen);
	struct graphics *gra=dc->gra;
	struct graphics_gc *gc=dc->gc;
	struct element *e=dc->e;
	struct graphics_image *img=dc->img;
	struct point p;
	char *path;

	while (di) {
	int i,count=di->count,mindist=dc->mindist;

	if (! gc) {
		gc=graphics_gc_new(gra);
		graphics_gc_set_foreground(gc, &e->color);
		dc->gc=gc;
	}
	if (item_type_is_area(dc->type) && (dc->e->type == element_polyline || dc->e->type == element_text))
		count=limit_count(di->c, count);
	if (dc->type == type_poly_water_tiled)
		mindist=0;
	if (dc->e->type == element_polyline)
		count=transform(dc->trans, dc->pro, di->c, pa, count, mindist, e->u.polyline.width, width);
	else
		count=transform(dc->trans, dc->pro, di->c, pa, count, mindist, 0, NULL);
	switch (e->type) {
	case element_polygon:
		graphics_draw_polygon_clipped(gra, gc, pa, count);
		break;
	case element_polyline:
		{
			gc->meth.gc_set_linewidth(gc->priv, 1);
			if (e->u.polyline.width > 0 && e->u.polyline.dash_num > 0)
				graphics_gc_set_dashes(gc, e->u.polyline.width,
						       e->u.polyline.offset,
						       e->u.polyline.dash_table,
						       e->u.polyline.dash_num);
			for (i = 0 ; i < count ; i++) {
				if (width[i] < 2)
					width[i]=2;
			}
			graphics_draw_polyline_clipped(gra, gc, pa, count, width, 1, e->u.polyline.width > 1);
		}
		break;
	case element_circle:
		if (count) {
			if (e->u.circle.width > 1)
				gc->meth.gc_set_linewidth(gc->priv, e->u.polyline.width);
			graphics_draw_circle(gra, gc, pa, e->u.circle.radius);
			if (di->label && e->text_size) {
				struct graphics_font *font=get_font(gra, e->text_size);
				struct graphics_gc *gc_background=dc->gc_background;
				if (! gc_background && e->u.circle.background_color.a) {
					gc_background=graphics_gc_new(gra);
					graphics_gc_set_foreground(gc_background, &e->u.circle.background_color);
					dc->gc_background=gc_background;
				}
				p.x=pa[0].x+3;
				p.y=pa[0].y+10;
				if (font)
					gra->meth.draw_text(gra->priv, gc->priv, gc_background?gc_background->priv:NULL, font->priv, di->label, &p, 0x10000, 0);
				else
					dbg(0,"Failed to get font with size %d\n",e->text_size);
			}
		}
		break;
	case element_text:
		if (count && di->label) {
			struct graphics_font *font=get_font(gra, e->text_size);
			struct graphics_gc *gc_background=dc->gc_background;
			if (! gc_background && e->u.text.background_color.a) {
				gc_background=graphics_gc_new(gra);
				graphics_gc_set_foreground(gc_background, &e->u.text.background_color);
				dc->gc_background=gc_background;
			}
			if (font)
				label_line(gra, gc, gc_background, font, pa, count, di->label);
			else
				dbg(0,"Failed to get font with size %d\n",e->text_size);
		}
		break;
	case element_icon:
		if (count) {
			if (!img || item_is_custom_poi(di->item)) {
				if (item_is_custom_poi(di->item)) {
					char *icon;
					char *src;
					if (img)
						graphics_image_free(dc->gra, img);
					src=e->u.icon.src;
					if (!src || !src[0])
						src="%s";
					icon=g_strdup_printf(src,di->label+strlen(di->label)+1);
					path=graphics_icon_path(icon);
					g_free(icon);
				} else
					path=graphics_icon_path(e->u.icon.src);
				img=graphics_image_new_scaled_rotated(gra, path, e->u.icon.width, e->u.icon.height, e->u.icon.rotation);
				if (img)
					dc->img=img;
				else
					dbg(0,"failed to load icon '%s'\n", path);
				g_free(path);
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
	di=di->next;
	}
}
/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
static void xdisplay_draw_elements(struct graphics *gra, struct displaylist *display_list, struct itemgra *itm)
{
	struct element *e;
	GList *es,*types;
	struct display_context *dc=&display_list->dc;
	struct hash_entry *entry;

	es=itm->elements;
	while (es) {
		e=es->data;
		dc->e=e;
		types=itm->type;
		while (types) {
			dc->type=GPOINTER_TO_INT(types->data);
			entry=get_hash_entry(display_list, dc->type);
			if (entry && entry->di) {
				displayitem_draw(entry->di, NULL, dc);
				display_context_free(dc);
			}
			types=g_list_next(types);
		}
		es=g_list_next(es);
	}
}

void
graphics_draw_itemgra(struct graphics *gra, struct itemgra *itm, struct transformation *t, char *label)
{
	GList *es;
	struct display_context dc;
	int max_coord=32;
	char *buffer=g_alloca(sizeof(struct displayitem)+max_coord*sizeof(struct coord));
	struct displayitem *di=(struct displayitem *)buffer;
	es=itm->elements;
	di->item.type=type_none;
	di->item.id_hi=0;
	di->item.id_lo=0;
	di->item.map=NULL;
	di->label=label;
	dc.gra=gra;
	dc.gc=NULL;
	dc.gc_background=NULL;
	dc.img=NULL;
	dc.pro=projection_screen;
	dc.mindist=0;
	dc.trans=t;
	dc.type=type_none;
	dc.maxlen=max_coord;
	while (es) {
		struct element *e=es->data;
		if (e->coord_count) {
			if (e->coord_count > max_coord) {
				dbg(0,"maximum number of coords reached: %d > %d\n",e->coord_count,max_coord);
				di->count=max_coord;
			} else
				di->count=e->coord_count;
			memcpy(di->c, e->coord, di->count*sizeof(struct coord));
		} else {
			di->c[0].x=0;
			di->c[0].y=0;
			di->count=1;
		}
		dc.e=e;
		di->next=NULL;
		displayitem_draw(di, NULL, &dc);
		display_context_free(&dc);
		es=g_list_next(es);
	}
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
static void xdisplay_draw_layer(struct displaylist *display_list, struct graphics *gra, struct layer *lay, int order)
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
static void xdisplay_draw(struct displaylist *display_list, struct graphics *gra, struct layout *l, int order)
{
	GList *lays;
	struct layer *lay;

	lays=l->layers;
	while (lays) {
		lay=lays->data;
		if (lay->active)
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

static void
displaylist_update_layers(struct displaylist *displaylist, GList *layers, int order)
{
	while (layers) {
		struct layer *layer=layers->data;
		GList *itemgras=layer->itemgras;
		while (itemgras) {
			struct itemgra *itemgra=itemgras->data;
			GList *types=itemgra->type;
			if (itemgra->order.min <= order && itemgra->order.max >= order) {
				while (types) {
					enum item_type type=(enum item_type) types->data;
					set_hash_entry(displaylist, type);
					types=g_list_next(types);
				}
			}
			itemgras=g_list_next(itemgras);
		}
		layers=g_list_next(layers);
	}
}

static void
displaylist_update_hash(struct displaylist *displaylist)
{
	displaylist->max_offset=0;
	clear_hash(displaylist);
	displaylist_update_layers(displaylist, displaylist->layout->layers, displaylist->order);
	dbg(1,"max offset %d\n",displaylist->max_offset);
}



static void
do_draw(struct displaylist *displaylist, int cancel, int flags)
{
	struct item *item;
	int count,max=displaylist->dc.maxlen,workload=0;
	struct coord *ca=g_alloca(sizeof(struct coord)*max);
	struct attr attr,attr2;
	enum projection pro;

	if (displaylist->order != displaylist->order_hashed || displaylist->layout != displaylist->layout_hashed) {
		displaylist_update_hash(displaylist);
		displaylist->order_hashed=displaylist->order;
		displaylist->layout_hashed=displaylist->layout;
	}
	profile(0,NULL);
	pro=transform_get_projection(displaylist->dc.trans);
	while (!cancel) {
		if (!displaylist->msh)
			displaylist->msh=mapset_open(displaylist->ms);
		if (!displaylist->m) {
			displaylist->m=mapset_next(displaylist->msh, 1);
			if (!displaylist->m) {
				mapset_close(displaylist->msh);
				displaylist->msh=NULL;
				break;
			}
			displaylist->dc.pro=map_projection(displaylist->m);
			displaylist->conv=map_requires_conversion(displaylist->m);
			if (route_selection)
				displaylist->sel=route_selection;
			else
				displaylist->sel=transform_get_selection(displaylist->dc.trans, displaylist->dc.pro, displaylist->order);
			displaylist->mr=map_rect_new(displaylist->m, displaylist->sel);
		}
		if (displaylist->mr) {
			while ((item=map_rect_get_item(displaylist->mr))) {
				int label_count=0;
				char *labels[2];
				struct hash_entry *entry;
				if (item == &busy_item) {
					if (displaylist->workload)
						return;
					else
						continue;
				}
				entry=get_hash_entry(displaylist, item->type);
				if (!entry)
					continue;
				count=item_coord_get_within_selection(item, ca, item->type < type_line ? 1: max, displaylist->sel);
				if (! count)
					continue;
				if (displaylist->dc.pro != pro)
					transform_from_to_count(ca, displaylist->dc.pro, ca, pro, count);
				if (count == max) {
					dbg(0,"point count overflow %d for %s "ITEM_ID_FMT"\n", count,item_to_name(item->type),ITEM_ID_ARGS(*item));
					displaylist->dc.maxlen=max*2;
				}
				if (item_is_custom_poi(*item)) {
					if (item_attr_get(item, attr_icon_src, &attr2))
						labels[1]=map_convert_string(displaylist->m, attr2.u.str);
					else
						labels[1]=NULL;
					label_count=2;
				} else {
					labels[1]=NULL;
					label_count=0;
				}
				if (item_attr_get(item, attr_label, &attr)) {
					labels[0]=attr.u.str;
					if (!label_count)
						label_count=2;
				} else
					labels[0]=NULL;
				if (displaylist->conv && label_count) {
					labels[0]=map_convert_string(displaylist->m, labels[0]);
					display_add(entry, item, count, ca, labels, label_count);
					map_convert_free(labels[0]);
				} else
					display_add(entry, item, count, ca, labels, label_count);
				if (labels[1])
					map_convert_free(labels[1]);
				workload++;
				if (workload == displaylist->workload)
					return;
			}
			map_rect_destroy(displaylist->mr);
		}
		if (!route_selection)
			map_selection_destroy(displaylist->sel);
		displaylist->mr=NULL;
		displaylist->sel=NULL;
		displaylist->m=NULL;
	}
	profile(1,"process_selection\n");
	if (displaylist->idle_ev)
		event_remove_idle(displaylist->idle_ev);
	displaylist->idle_ev=NULL;
	callback_destroy(displaylist->idle_cb);
	displaylist->idle_cb=NULL;
	displaylist->busy=0;
	graphics_process_selection(displaylist->dc.gra, displaylist);
	profile(1,"draw\n");
	if (! cancel)
		graphics_displaylist_draw(displaylist->dc.gra, displaylist, displaylist->dc.trans, displaylist->layout, flags);
	map_rect_destroy(displaylist->mr);
	if (!route_selection)
		map_selection_destroy(displaylist->sel);
	mapset_close(displaylist->msh);
	displaylist->mr=NULL;
	displaylist->sel=NULL;
	displaylist->m=NULL;
	displaylist->msh=NULL;
	profile(1,"callback\n");
	callback_call_1(displaylist->cb, cancel);
	profile(0,"end\n");
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void graphics_displaylist_draw(struct graphics *gra, struct displaylist *displaylist, struct transformation *trans, struct layout *l, int flags)
{
	int order=transform_get_order(trans);
	displaylist->dc.trans=trans;
	displaylist->dc.gra=gra;
	displaylist->dc.mindist=transform_get_scale(trans)/2;
	// FIXME find a better place to set the background color
	if (l) {
		graphics_gc_set_background(gra->gc[0], &l->color);
		graphics_gc_set_foreground(gra->gc[0], &l->color);
		gra->default_font = g_strdup(l->font);
	}
	graphics_background_gc(gra, gra->gc[0]);
	if (flags & 1)
		callback_list_call_attr_0(gra->cbl, attr_predraw);
	gra->meth.draw_mode(gra->priv, (flags & 8)?draw_mode_begin_clear:draw_mode_begin);
	if (!(flags & 2))
		gra->meth.draw_rectangle(gra->priv, gra->gc[0]->priv, &gra->r.lu, gra->r.rl.x-gra->r.lu.x, gra->r.rl.y-gra->r.lu.y);
	if (l)
		xdisplay_draw(displaylist, gra, l, order+l->order_delta);
	if (flags & 1)
		callback_list_call_attr_0(gra->cbl, attr_postdraw);
	if (!(flags & 4))
		gra->meth.draw_mode(gra->priv, draw_mode_end);
}

static void graphics_load_mapset(struct graphics *gra, struct displaylist *displaylist, struct mapset *mapset, struct transformation *trans, struct layout *l, int async, struct callback *cb, int flags)
{
	int order=transform_get_order(trans);

	dbg(1,"enter");
	if (displaylist->busy) {
		if (async == 1)
			return;
		do_draw(displaylist, 1, flags);
	}
	xdisplay_free(displaylist);
	dbg(1,"order=%d\n", order);

	displaylist->dc.gra=gra;
	displaylist->ms=mapset;
	displaylist->dc.trans=trans;
	displaylist->workload=async ? 100 : 0;
	displaylist->cb=cb;
	displaylist->seq++;
	if (l)
		order+=l->order_delta;
	displaylist->order=order;
	displaylist->busy=1;
	displaylist->layout=l;
	if (async) {
		if (! displaylist->idle_cb)
			displaylist->idle_cb=callback_new_3(callback_cast(do_draw), displaylist, 0, flags);
		displaylist->idle_ev=event_add_idle(50, displaylist->idle_cb);
	} else
		do_draw(displaylist, 0, flags);
}
/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void graphics_draw(struct graphics *gra, struct displaylist *displaylist, struct mapset *mapset, struct transformation *trans, struct layout *l, int async, struct callback *cb, int flags)
{
	graphics_load_mapset(gra, displaylist, mapset, trans, l, async, cb, flags);
}

int
graphics_draw_cancel(struct graphics *gra, struct displaylist *displaylist)
{
	if (!displaylist->busy)
		return 0;
	do_draw(displaylist, 1, 0);
	return 1;
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
struct displaylist_handle {
	struct displaylist *dl;
	struct displayitem *di;
	int hashidx;
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
	ret->dl=displaylist;

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
	if (!dlh)
		return NULL;
	for (;;) {
		if (dlh->di) {
			ret=dlh->di;
			dlh->di=ret->next;
			break;
		}
		if (dlh->hashidx == HASH_SIZE) {
			ret=NULL;
			break;
		}
		if (dlh->dl->hash_entries[dlh->hashidx].type)
			dlh->di=dlh->dl->hash_entries[dlh->hashidx].di;
		dlh->hashidx++;
	}
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
	struct displaylist *ret=g_new0(struct displaylist, 1);

	ret->dc.maxlen=16384;

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

int
graphics_displayitem_get_coord_count(struct displayitem *di)
{
	return di->count;
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

int
graphics_displayitem_get_displayed(struct displayitem *di)
{
	return 1;
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

	if (line_p0->x < line_p1->x) {
		if (p->x < line_p0->x - dist)
			return 0;
		if (p->x > line_p1->x + dist)
			return 0;
	} else {
		if (p->x < line_p1->x - dist)
			return 0;
		if (p->x > line_p0->x + dist)
			return 0;
	}
	if (line_p0->y < line_p1->y) {
		if (p->y < line_p0->y - dist)
			return 0;
		if (p->y > line_p1->y + dist)
			return 0;
	} else {
		if (p->y < line_p1->y - dist)
			return 0;
		if (p->y > line_p0->y + dist)
			return 0;
	}

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
int graphics_displayitem_within_dist(struct displaylist *displaylist, struct displayitem *di, struct point *p, int dist)
{
	struct point *pa=g_alloca(sizeof(struct point)*displaylist->dc.maxlen);
	int count;

	count=transform(displaylist->dc.trans, displaylist->dc.pro, di->c, pa, di->count, 1, 0, NULL);

	if (di->item.type < type_line) {
		return within_dist_point(p, &pa[0], dist);
	}
	if (di->item.type < type_area) {
		return within_dist_polyline(p, pa, count, dist, 0);
	}
	return within_dist_polygon(p, pa, count, dist);
}


static void
graphics_process_selection_item(struct displaylist *dl, struct item *item)
{
#if 0 /* FIXME */
	struct displayitem di,*di_res;
	GHashTable *h;
	int count,max=dl->dc.maxlen;
	struct coord ca[max];
	struct attr attr;
	struct map_rect *mr;

	di.item=*item;
	di.label=NULL;
	di.count=0;
	h=g_hash_table_lookup(dl->dl, GINT_TO_POINTER(di.item.type));
	if (h) {
		di_res=g_hash_table_lookup(h, &di);
		if (di_res) {
			di.item.type=(enum item_type)item->priv_data;
			display_add(dl, &di.item, di_res->count, di_res->c, NULL, 0);
			return;
		}
	}
	mr=map_rect_new(item->map, NULL);
	item=map_rect_get_item_byid(mr, item->id_hi, item->id_lo);
	count=item_coord_get(item, ca, item->type < type_line ? 1: max);
	if (!item_attr_get(item, attr_label, &attr))
		attr.u.str=NULL;
	if (dl->conv && attr.u.str && attr.u.str[0]) {
		char *str=map_convert_string(item->map, attr.u.str);
		display_add(dl, item, count, ca, &str, 1);
		map_convert_free(str);
	} else
		display_add(dl, item, count, ca, &attr.u.str, 1);
	map_rect_destroy(mr);
#endif
}

void
graphics_add_selection(struct graphics *gra, struct item *item, enum item_type type, struct displaylist *dl)
{
	struct item *item_dup=g_new(struct item, 1);
	*item_dup=*item;
	item_dup->priv_data=(void *)type;
	gra->selection=g_list_append(gra->selection, item_dup);
	if (dl)
		graphics_process_selection_item(dl, item_dup);
}

void
graphics_remove_selection(struct graphics *gra, struct item *item, enum item_type type, struct displaylist *dl)
{
	GList *curr;
	int found;

	for (;;) {
		curr=gra->selection;
		found=0;
		while (curr) {
			struct item *sitem=curr->data;
			if (item_is_equal(*item,*sitem)) {
				if (dl) {
					struct displayitem di;
					/* Unused Variable
					GHashTable *h; */
					di.item=*sitem;
					di.label=NULL;
					di.count=0;
					di.item.type=type;
#if 0 /* FIXME */
					h=g_hash_table_lookup(dl->dl, GINT_TO_POINTER(di.item.type));
					if (h)
						g_hash_table_remove(h, &di);
#endif
				}
				g_free(sitem);
				gra->selection=g_list_remove(gra->selection, curr->data);
				found=1;
				break;
			}
		}
		if (!found)
			return;
	}
}

void
graphics_clear_selection(struct graphics *gra, struct displaylist *dl)
{
	while (gra->selection) {
		struct item *item=(struct item *)gra->selection->data;
		graphics_remove_selection(gra, item, (enum item_type)item->priv_data,dl);
	}
}

static void
graphics_process_selection(struct graphics *gra, struct displaylist *dl)
{
	GList *curr;

	curr=gra->selection;
	while (curr) {
		struct item *item=curr->data;
		graphics_process_selection_item(dl, item);
		curr=g_list_next(curr);
	}
}
