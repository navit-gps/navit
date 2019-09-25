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

#include <stdlib.h>
#include <glib.h>
#include <stdio.h>
#include <math.h>
#include "config.h"
#include "debug.h"
#include "string.h"
#include "draw_info.h"
#include "point.h"
#include "graphics.h"
#include "projection.h"
#include "item.h"
#include "xmlconfig.h"
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
#include "navit.h"


//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
/**
 * @brief graphics object
 * A graphics object serves as the target for drawing operations.
 * It encapsulates various settings, and a drawing target, such as an image buffer or a window.
 * Currently, in Navit, there is always one main graphics object, which is used to draw the
 * map, and optionally additional graphics objects for overlays.
 * @see graphics_overlay_new()
 * @see struct graphics_gc
 */
struct graphics {
    struct graphics* parent;
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
    int disabled;
    /*
     * Counter for z_order of displayitems;
    */
    int current_z_order;
    GHashTable *image_cache_hash;
    /* for dpi compensation */
    int dpi_factor;
};

struct display_context {
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
struct hash_entry {
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

static void circle_to_points(const struct point *center, int diameter, int scale, int start, int len, struct point *res,
                             int *pos, int dir);
static void graphics_process_selection(struct graphics *gra, struct displaylist *dl);
static void graphics_gc_init(struct graphics *this_);


static int graphics_dpi_scale(struct graphics * gra, int p) {
    int result;
    if(gra == NULL)
        return p;
    result = p * gra->dpi_factor;
    return result;
}
static struct point graphics_dpi_scale_point(struct graphics * gra, struct point *p) {
    struct point result = {-1,-1};
    if(!p)
        return result;
    result.x = graphics_dpi_scale(gra, p->x);
    result.y = graphics_dpi_scale(gra, p->y);
    return result;
}
static int graphics_dpi_unscale(struct graphics * gra, int p) {
    int result;
    if(gra == NULL)
        return p;
    result = p / gra->dpi_factor;
    return result;
}
static struct point graphics_dpi_unscale_point(struct graphics * gra, struct point *p) {
    struct point result = {-1,-1};
    if(!p)
        return result;
    result.x = graphics_dpi_unscale(gra, p->x);
    result.y = graphics_dpi_unscale(gra, p->y);
    return result;
}

static void clear_hash(struct displaylist *dl) {
    int i;
    for (i = 0 ; i < HASH_SIZE ; i++)
        dl->hash_entries[i].type=type_none;
}

static struct hash_entry *get_hash_entry(struct displaylist *dl, enum item_type type) {
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

static struct hash_entry *set_hash_entry(struct displaylist *dl, enum item_type type) {
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

/**
 * @brief Sets a generic attribute of the graphics instance
 *
 * This will only set one of the supported generic graphics attributes (currently {@code gamma},
 * {@code brightness}, {@code contrast} or {@code font_size}) and fail for other attribute types.
 *
 * To set an attribute provided by a graphics plugin, use {@link graphics_set_attr(struct graphics *, struct attr *)}
 * instead.
 *
 * @param gra The graphics instance
 * @param attr The attribute to set
 *
 * @return True if the attribute was set, false if not
 */
static int graphics_set_attr_do(struct graphics *gra, struct attr *attr) {
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

/**
 * @brief Sets an attribute of the graphics instance
 *
 * This method first tries to set one of the private attributes implemented by the current graphics
 * plugin. If this fails, it tries to set one of the generic attributes.
 *
 * If the graphics plugin does not supply a {@code set_attr} method, this method currently does nothing
 * and returns true, even if the attribute is a generic one.
 *
 * @param gra The graphics instance
 * @param attr The attribute to set
 *
 * @return True if the attribute was successfully set, false otherwise.
 */
int graphics_set_attr(struct graphics *gra, struct attr *attr) {
    int ret=1;
    /* FIXME if gra->meth doesn't have a setter, we don't even try the generic attrs - is that what we want? */
    dbg(lvl_debug,"enter");
    if (gra->meth.set_attr)
        ret=gra->meth.set_attr(gra->priv, attr);
    if (!ret)
        ret=graphics_set_attr_do(gra, attr);
    return ret != 0;
}

void graphics_set_rect(struct graphics *gra, struct point_rect *pr) {
    gra->r=*pr;
}

/**
 * @brief unscale coordinates coming from the graphics backend via callback.
 *
 * @param l pointer to callback list
 * @param pcount number of parameters attached to this callback
 * @param p list of parameters
 * @param context context handed over by callback_list_add_patch_function, gra in this case.
 * @return nothing
 */
static void graphics_dpi_patch (struct callback_list *l, enum attr_type type, int pcount, void **p, void * context) {
    /* this is black magic. We scaled all coordinates to the graphics backend
     * to compensate screen dpi. Since the backends communicate back via the callback
     * list, we hook this function to unscale the coordinates coming back to
     * navit before actually calling the callbacks.
     */
    struct graphics * gra;
    gra = (struct graphics *) context;
    if(gra == NULL)
        return;

    if((type == attr_resize) && (pcount >= 2)) {
        int w, h;
        w = GPOINTER_TO_INT(p[0]);
        h = GPOINTER_TO_INT(p[1]);
        dbg(lvl_debug,"scaling attr_resize %d, %d, %d", pcount, w, h);
        p[0] = GINT_TO_POINTER(graphics_dpi_unscale(gra,w));
        p[1] = GINT_TO_POINTER(graphics_dpi_unscale(gra,h));
    }
    if((type == attr_button) && (pcount >=3)) {
        struct point * pnt;
        pnt = (struct point *) p[2];
        dbg(lvl_debug,"scaling attr_button %d, %d, %d", pcount, pnt->x, pnt->y);
        *pnt = graphics_dpi_unscale_point(gra, pnt);
    }
    if((type == attr_motion) && (pcount >=1)) {
        struct point * pnt;
        pnt = (struct point *) p[0];
        dbg(lvl_debug,"scaling attr_motion %d, %d, %d", pcount, pnt->x, pnt->y);
        *pnt = graphics_dpi_unscale_point(gra, pnt);
    }
    /* any more?  attr_keypress doesn't come with coordinates */
}

/**
 * Creates a new graphics object
 * attr type required
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
struct graphics * graphics_new(struct attr *parent, struct attr **attrs) {
    struct graphics *this_;
    struct attr *type_attr, cbl_attr, *real_dpi_attr, *virtual_dpi_attr;
    struct graphics_priv * (*graphicstype_new)(struct navit *nav, struct graphics_methods *meth, struct attr **attrs,
            struct callback_list *cbl);

    if (! (type_attr=attr_search(attrs, NULL, attr_type))) {
        dbg(lvl_error,"Graphics plugin type is not set.");
        return NULL;
    }

    graphicstype_new=plugin_get_category_graphics(type_attr->u.str);
    if (! graphicstype_new) {
        dbg(lvl_error,"Failed to load graphics plugin %s.", type_attr->u.str);
        return NULL;
    }

    this_=g_new0(struct graphics, 1);
    this_->attrs=attr_list_dup(attrs);
    /* start with no scaling */
    this_->dpi_factor=1;
    this_->cbl=callback_list_new();
    cbl_attr.type=attr_callback_list;
    cbl_attr.u.callback_list=this_->cbl;
    callback_list_add_patch_function (this_->cbl, graphics_dpi_patch, (void*) this_);
    this_->attrs=attr_generic_add_attr(this_->attrs, &cbl_attr);
    this_->priv=(*graphicstype_new)(parent->u.navit, &this_->meth, this_->attrs, this_->cbl);
    this_->brightness=0;
    this_->contrast=65536;
    this_->gamma=65536;
    this_->font_size=20;
    this_->image_cache_hash = g_hash_table_new_full(g_str_hash, g_str_equal,g_free,g_free);
    /*get dpi */
    virtual_dpi_attr=attr_search(attrs, NULL, attr_virtual_dpi);
    real_dpi_attr=attr_search(attrs, NULL, attr_real_dpi);
    if(virtual_dpi_attr != NULL) {
        navit_float virtual_dpi, real_dpi=0;
        virtual_dpi=virtual_dpi_attr->u.num;
        if(real_dpi_attr != NULL)
            real_dpi=real_dpi_attr->u.num;
        else
            real_dpi=graphics_get_dpi(this_);
        if((real_dpi != 0) && (virtual_dpi != 0)) {
            this_->dpi_factor = round(real_dpi / virtual_dpi);
            if(this_->dpi_factor < 1)
                this_->dpi_factor = 1;
            dbg(lvl_error,"Using virtual dpi %f, real dpi %f factor %d", virtual_dpi, real_dpi, this_->dpi_factor);
        }
    }
    if(this_->dpi_factor != 1)
        callback_list_call_attr_2(this_->cbl, attr_resize, GINT_TO_POINTER(navit_get_width(parent->u.navit)),
                                  GINT_TO_POINTER(navit_get_height(parent->u.navit)));
    while (*attrs) {
        graphics_set_attr_do(this_,*attrs);
        attrs++;
    }
    return this_;
}

/**
 * @brief Gets an attribute of the graphics instance
 *
 * This function searches the attribute list of the graphics object for an attribute of a given type and
 * stores it in the attr parameter.
 * <p>
 * Searching for attr_any or attr_any_xml is supported.
 * <p>
 * An iterator can be specified to get multiple attributes of the same type:
 * The first call will return the first match from attr; each subsequent call
 * with the same iterator will return the next match. If no more matching
 * attributes are found in either of them, false is returned.
 * <p>
 * Note that currently this will only return the generic attributes which can be set with
 * {@link graphics_set_attr_do(struct graphics *, struct attr *)}. Attributes implemented by a graphics
 * plugin cannot be retrieved with this method.
 *
 * @param this The graphics instance
 * @param type The attribute type to search for
 * @param attr Points to a {@code struct attr} which will receive the attribute
 * @param iter An iterator. This parameter may be NULL.
 *
 * @return True if a matching attribute was found, false if not.
 *
 * @author Martin Schaller (04/2008)
*/
int graphics_get_attr(struct graphics *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter) {
    return attr_generic_get_attr(this_->attrs, NULL, type, attr, iter);
}

/**
 * @brief Create a new graphics overlay.
 * An overlay is a graphics object that is independent of the main graphics object. When
 * drawing everything to a window, the overlay will be shown on top of the main graphics
 * object. Navit uses overlays for OSD elements and for the vehicle on the map.
 * This allows updating OSD elements and the vehicle without redrawing the map.
 *
 * @param parent parent graphics context (should be the main graphics context as returned by
 *        graphics_new)
 * @param p drawing position for the overlay
 * @param w width of overlay
 * @param h height of overlay
 * @param wraparound use wraparound (0/1). If set, position, width and height "wrap around":
 * negative position coordinates wrap around the window, negative width/height specify
 * difference to window width/height.
 * @returns new overlay
 * @author Martin Schaller (04/2008)
*/
struct graphics * graphics_overlay_new(struct graphics *parent, struct point *p, int w, int h, int wraparound) {
    struct graphics *this_;
    struct point_rect pr;
    struct point p_scaled;
    int w_scaled, h_scaled;
    if (!parent->meth.overlay_new)
        return NULL;
    this_=g_new0(struct graphics, 1);
    this_->dpi_factor = parent->dpi_factor;
    p_scaled=graphics_dpi_scale_point(parent,p);
    w_scaled=graphics_dpi_scale(parent,w);
    h_scaled=graphics_dpi_scale(parent,h);
    this_->priv=parent->meth.overlay_new(parent->priv, &this_->meth, &p_scaled, w_scaled, h_scaled, wraparound);
    this_->image_cache_hash = parent->image_cache_hash;
    this_->parent = parent;
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
 * @brief Alters the size, position and wraparound for an overlay
 *
 * @param this_ The overlay's graphics struct
 * @param p The new position of the overlay
 * @param w The new width of the overlay
 * @param h The new height of the overlay
 * @param wraparound The new wraparound of the overlay
 */
void graphics_overlay_resize(struct graphics *this_, struct point *p, int w, int h, int wraparound) {
    struct point p_scaled;
    int w_scaled, h_scaled;
    if (! this_->meth.overlay_resize) {
        return;
    }

    p_scaled = graphics_dpi_scale_point(this_,p);
    w_scaled = graphics_dpi_scale(this_,w);
    h_scaled =graphics_dpi_scale(this_,h);
    this_->meth.overlay_resize(this_->priv, &p_scaled, w_scaled, h_scaled, wraparound);
}

static void graphics_gc_init(struct graphics *this_) {
    struct color background= { COLOR_BACKGROUND_ };
    struct color black= { COLOR_BLACK_ };
    struct color white= { COLOR_WHITE_ };
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
void graphics_init(struct graphics *this_) {
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
void * graphics_get_data(struct graphics *this_, const char *type) {
    return (this_->meth.get_data(this_->priv, type));
}

void graphics_add_callback(struct graphics *this_, struct callback *cb) {
    callback_list_add(this_->cbl, cb);
}

void graphics_remove_callback(struct graphics *this_, struct callback *cb) {
    callback_list_remove(this_->cbl, cb);
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
struct graphics_font * graphics_font_new(struct graphics *gra, int size, int flags) {

    return graphics_named_font_new(gra, gra->default_font, size, flags);
}

struct graphics_font * graphics_named_font_new(struct graphics *gra, char *font, int size, int flags) {
    struct graphics_font *this_;

    this_=g_new0(struct graphics_font,1);
    this_->priv=gra->meth.font_new(gra->priv, &this_->meth, font, graphics_dpi_scale(gra,size), flags);
    return this_;
}

void graphics_font_destroy(struct graphics_font *gra_font) {
    if(!gra_font)
        return;
    gra_font->meth.font_destroy(gra_font->priv);
    g_free(gra_font);
}

/**
 * Destroy graphics
 * Called when navit exits
 * @param gra The graphics instance
 * @returns nothing
 * @author David Tegze (02/2011)
 */
void graphics_free(struct graphics *gra) {
    if (!gra)
        return;

    /* If it's not an overlay, free the image cache. */
    if(!gra->parent) {
        struct graphics_image *img;
        GList *ll, *l;

        /* We can't specify context (pointer to struct graphics) for g_hash_table_new to have it passed to free function
           so we have to free img->priv manually, the rest would be freed by g_hash_table_destroy. GHashTableIter isn't used because it
           broke n800 build at r5107.
        */
        for(ll=l=g_hash_to_list(gra->image_cache_hash); l; l=g_list_next(l)) {
            img=l->data;
            if (img && gra->meth.image_free)
                gra->meth.image_free(gra->priv, img->priv);
        }
        g_list_free(ll);
        g_hash_table_destroy(gra->image_cache_hash);
    }

    attr_list_free(gra->attrs);
    graphics_gc_destroy(gra->gc[0]);
    graphics_gc_destroy(gra->gc[1]);
    graphics_gc_destroy(gra->gc[2]);
    g_free(gra->default_font);
    graphics_font_destroy_all(gra);
    g_free(gra->font);
    gra->meth.graphics_destroy(gra->priv);
    g_free(gra);
}

/**
 * Free all loaded fonts.
 * Used when switching layouts.
 * @param gra The graphics instance
 * @returns nothing
 * @author Sarah Nordstrom (05/2008)
 */
void graphics_font_destroy_all(struct graphics *gra) {
    int i;
    for(i = 0 ; i < gra->font_len; i++) {
        if(!gra->font[i]) continue;
        gra->font[i]->meth.font_destroy(gra->font[i]->priv);
        g_free(gra->font[i]);
        gra->font[i] = NULL;
    }
}

/**
 * Create a new graphics context.
 * @param gra associated graphics object for the new context
 * @returns new graphics context
 * @author Martin Schaller (04/2008)
*/
struct graphics_gc * graphics_gc_new(struct graphics *gra) {
    struct graphics_gc *this_;

    this_=g_new0(struct graphics_gc,1);
    this_->priv=gra->meth.gc_new(gra->priv, &this_->meth);
    this_->gra=gra;
    return this_;
}

/**
 * Destroy a graphics context, freeing associated resources.
 * @param gc context to destroy
 * @author Martin Schaller (04/2008)
*/
void graphics_gc_destroy(struct graphics_gc *gc) {
    if (!gc)
        return;
    gc->meth.gc_destroy(gc->priv);
    g_free(gc);
}

static void graphics_convert_color(struct graphics *gra, struct color *in, struct color *out) {
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
 * Set foreground color.
 * @param gc graphics context to set color for
 * @param c color to set
 * @author Martin Schaller (04/2008)
*/
void graphics_gc_set_foreground(struct graphics_gc *gc, struct color *c) {
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
void graphics_gc_set_background(struct graphics_gc *gc, struct color *c) {
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
void graphics_gc_set_linewidth(struct graphics_gc *gc, int width) {
    gc->meth.gc_set_linewidth(gc->priv, graphics_dpi_scale(gc->gra, width));
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void graphics_gc_set_dashes(struct graphics_gc *gc, int width, int offset, unsigned char dash_list[], int n) {
    if (gc->meth.gc_set_dashes) {
        int a;
        unsigned char * dash_list_scaled = g_alloca(sizeof (unsigned char) * n);
        for(a = 0; a < n; a ++) {
            dash_list_scaled[a]=graphics_dpi_scale(gc->gra,dash_list[a]);
        }
        gc->meth.gc_set_dashes(gc->priv, graphics_dpi_scale(gc->gra,width), graphics_dpi_scale(gc->gra,offset),
                               dash_list_scaled, n);
    }
}

/**
 * @brief Create a new image from file path, optionally scaled to w and h pixels.
 *
 * @param gra the graphics instance
 * @param path path of the image to load
 * @param w width to rescale to, or IMAGE_W_H_UNSET for original width
 * @param h height to rescale to, or IMAGE_W_H_UNSET for original height
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
struct graphics_image * graphics_image_new_scaled(struct graphics *gra, char *path, int w, int h) {
    return graphics_image_new_scaled_rotated(gra, path, w, h, 0);
}

static void image_new_helper(struct graphics *gra, struct graphics_image *this_, char *path, char *name, int width,
                             int height, int rotate, int zip) {
    int i=0;
    int stdsizes[]= {8,12,16,22,24,32,36,48,64,72,96,128,192,256};
    const int numstdsizes=sizeof(stdsizes)/sizeof(int);
    int sz;
    int mode=1;
    int bmstd=0;
    sz=width>0?width:height;
    while (mode<=8) {
        char *new_name=NULL;
        int n;
        switch (mode) {
        case 1:
            /* The best variant both for cpu usage and quality would be prescaled png of a needed size */
            mode++;
            if (width != IMAGE_W_H_UNSET && height != IMAGE_W_H_UNSET) {
                new_name=g_strdup_printf("%s_%d_%d.png", name, width, height);
            }
            break;
        case 2:
            mode++;
            /* Try to load image by the exact name given by user. For example, if she wants to
              scale some prescaled png variant to a new size given as function params, or have
              default png image to be displayed unscaled. */
            new_name=g_strdup(path);
            break;
        case 3:
            mode++;
            /* Next, try uncompressed and compressed svgs as they should give best quality but
               rendering might take more cpu resources when the image is displayed for the first time */
            new_name=g_strdup_printf("%s.svg", name);
            break;
        case 4:
            mode++;
            new_name=g_strdup_printf("%s.svgz", name);
            break;
        case 5:
            mode++;
            i=0;
            /* If we have no size specifiers, try the default png now */
            if(sz<=0) {
                new_name=g_strdup_printf("%s.png", name);
                break;
            }
            /* Find best matching size from standard row */
            for(bmstd=0; bmstd<numstdsizes; bmstd++)
                if(stdsizes[bmstd]>sz)
                    break;
            i=1;
        /* Fall through */
        case 6:
            /* Select best matching image from standard row */
            if(sz>0) {
                /* If size were specified, start with bmstd and then try standard sizes in row
                 * bmstd, bmstd+1, bmstd+2, .. numstdsizes-1, bmstd-1, bmstd-2, .., 0 */
                n=bmstd+i;
                if((bmstd+i)>=numstdsizes)
                    n=numstdsizes-i-1;

                if(++i==numstdsizes)
                    mode++;
            } else {
                /* If no size were specified, start with the smallest standard size and then try following ones */
                n=i++;
                if(i==numstdsizes)
                    mode+=2;
            }
            if(n<0||n>=numstdsizes)
                break;
            new_name=g_strdup_printf("%s_%d_%d.png", name, stdsizes[n],stdsizes[n]);
            break;

        case 7:
            /* Scaling the default prescaled png of unknown size to the needed size will give random quality loss */
            mode++;
            new_name=g_strdup_printf("%s.png", name);
            break;
        case 8:
            /* xpm format is used as a last resort, because its not widely supported and we are moving to svg and png formats */
            mode++;
            new_name=g_strdup_printf("%s.xpm", name);
            break;
        }
        if (! new_name)
            continue;

        this_->width=width;
        this_->height=height;
        dbg(lvl_debug,"Trying to load image '%s' for '%s' at %dx%d", new_name, path, width, height);
        if (zip) {
            unsigned char *start;
            int len;
            if (file_get_contents(new_name, &start, &len)) {
                struct graphics_image_buffer buffer= {"buffer:",graphics_image_type_unknown};
                buffer.start=start;
                buffer.len=len;
                this_->hot = graphics_dpi_scale_point(gra,&this_->hot);
                if(this_->width != IMAGE_W_H_UNSET)
                    this_->width = graphics_dpi_scale(gra,this_->width);
                if(this_->height != IMAGE_W_H_UNSET)
                    this_->height = graphics_dpi_scale(gra,this_->height);
                this_->priv=gra->meth.image_new(gra->priv, &this_->meth, (char *)&buffer, &this_->width, &this_->height, &this_->hot,
                                                rotate);
                this_->hot = graphics_dpi_unscale_point(gra,&this_->hot);
                if(this_->width != IMAGE_W_H_UNSET)
                    this_->width = graphics_dpi_unscale(gra,this_->width);
                if(this_->height != IMAGE_W_H_UNSET)
                    this_->height = graphics_dpi_unscale(gra,this_->height);
                g_free(start);
            }
        } else {
            if (strcmp(new_name,"buffer:")) {
                this_->hot = graphics_dpi_scale_point(gra,&this_->hot);
                if(this_->width != IMAGE_W_H_UNSET)
                    this_->width = graphics_dpi_scale(gra,this_->width);
                if(this_->height != IMAGE_W_H_UNSET)
                    this_->height = graphics_dpi_scale(gra,this_->height);
                this_->priv=gra->meth.image_new(gra->priv, &this_->meth, new_name, &this_->width, &this_->height, &this_->hot, rotate);
                this_->hot = graphics_dpi_unscale_point(gra,&this_->hot);
                if(this_->width != IMAGE_W_H_UNSET)
                    this_->width = graphics_dpi_unscale(gra,this_->width);
                if(this_->height != IMAGE_W_H_UNSET)
                    this_->height = graphics_dpi_unscale(gra,this_->height);
            }
        }
        if (this_->priv) {
            dbg(lvl_info,"Using image '%s' for '%s' at %dx%d", new_name, path, width, height);
            g_free(new_name);
            break;
        }
        g_free(new_name);
    }
}

/**
 * @brief Create a new image from file path, optionally scaled to w and h pixels and rotated.
 *
 * @param gra the graphics instance
 * @param path path of the image to load
 * @param w width to rescale to, or IMAGE_W_H_UNSET for original width
 * @param h height to rescale to, or IMAGE_W_H_UNSET for original height
 * @param rotate angle to rotate the image, in 90 degree steps (not supported by all plugins).
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
struct graphics_image * graphics_image_new_scaled_rotated(struct graphics *gra, char *path, int w, int h, int rotate) {
    struct graphics_image *this_;
    char* hash_key = g_strdup_printf("%s*%d*%d*%d",path,w,h,rotate);
    struct file_wordexp *we;
    int i;
    char **paths;

    if ( g_hash_table_lookup_extended( gra->image_cache_hash, hash_key, NULL, (gpointer)&this_) ) {
        g_free(hash_key);
        dbg(lvl_debug,"Found cached image%sfor '%s'",this_?" ":" miss ",path);
        return this_;
    }

    this_=g_new0(struct graphics_image,1);
    this_->height=h;
    this_->width=w;

    we=file_wordexp_new(path);
    paths=file_wordexp_get_array(we);

    for(i=0; i<file_wordexp_get_count(we) && !this_->priv; i++) {
        char *ext;
        char *s, *name;
        char *pathi=paths[i];
        int len=strlen(pathi);
        int i,k;
        int newwidth=IMAGE_W_H_UNSET, newheight=IMAGE_W_H_UNSET;

        ext=g_utf8_strrchr(pathi,-1,'.');
        i=pathi-ext+len;

        /* Dont allow too long or too short file name extensions*/
        if(ext && ((i>5) || (i<1)))
            ext=NULL;

        /* Search for _w_h name part, begin from char before extension if it exists */
        if(ext)
            s=ext-1;
        else
            s=pathi+len;

        k=1;
        while(s>pathi && g_ascii_isdigit(*s)) {
            if(newheight<0)
                newheight=0;
            newheight+=(*s-'0')*k;
            k*=10;
            s--;
        }

        if(k>1 && s>pathi && *s=='_') {
            k=1;
            s--;
            while(s>pathi && g_ascii_isdigit(*s)) {
                if(newwidth<0)
                    newwidth=0;
                newwidth+=(*s-'0')*k;;
                k*=10;
                s--;
            }
        }

        if(k==1 || s<=pathi || *s!='_') {
            newwidth=IMAGE_W_H_UNSET;
            newheight=IMAGE_W_H_UNSET;
            if(ext)
                s=ext;
            else
                s=pathi+len;

        }

        /* If exact h and w values were given as function parameters, they take precedence over values guessed from the image name */
        if(w!=IMAGE_W_H_UNSET)
            newwidth=w;
        if(h!=IMAGE_W_H_UNSET)
            newheight=h;

        name=g_strndup(pathi,s-pathi);
        image_new_helper(gra, this_, pathi, name, newwidth, newheight, rotate, 0);
        if (!this_->priv && strstr(pathi, ".zip/"))
            image_new_helper(gra, this_, pathi, name, newwidth, newheight, rotate, 1);
        g_free(name);
    }

    file_wordexp_destroy(we);

    if (! this_->priv) {
        dbg(lvl_error,"No image for '%s'", path);
        g_free(this_);
        this_=NULL;
    }

    g_hash_table_insert(gra->image_cache_hash, hash_key,  (gpointer)this_ );

    return this_;
}

/**
 * Create a new image from file path
 * @param gra the graphics instance
 * @param path path of the image to load
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
struct graphics_image * graphics_image_new(struct graphics *gra, char *path) {
    return graphics_image_new_scaled_rotated(gra, path, IMAGE_W_H_UNSET, IMAGE_W_H_UNSET, 0);
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void graphics_image_free(struct graphics *gra, struct graphics_image *img) {
    /* Image is cached inside gra->image_cache_hash. So it would be freed only when graphics is destroyed => Do nothing here. */
}

/**
 * @brief Start or finish a set of drawing operations.
 *
 * graphics_draw_mode(draw_mode_begin) must be invoked before performing any drawing
 * operations; this allows the graphics driver to perform any necessary setup.
 * graphics_draw_mode(draw_mode_end) must be invoked to finish a set of drawing operations;
 * this will typically clean up drawing resources and display the drawing result.
 * @param this_ graphics object that is being drawn to
 * @param mode specify beginning or end of drawing
 * @author Martin Schaller (04/2008)
*/
void graphics_draw_mode(struct graphics *this_, enum draw_mode_num mode) {
    this_->meth.draw_mode(this_->priv, mode);
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void graphics_draw_lines(struct graphics *this_, struct graphics_gc *gc, struct point *p, int count) {
    struct point * p_scaled =  g_alloca(sizeof (struct point)*count);
    int a;
    for(a=0; a < count; a ++)
        p_scaled[a] = graphics_dpi_scale_point(this_,&(p[a]));
    this_->meth.draw_lines(this_->priv, gc->priv, p_scaled, count);
}

/**
 * @brief Draw a circle
 * @param this_ The graphics instance on which to draw
 * @param gc The graphics context
 * @param p The coordinates of the center of the circle
 * @param r The radius of the circle
 *
 * @author Martin Schaller (04/2008)
*/
void graphics_draw_circle(struct graphics *this_, struct graphics_gc *gc, struct point *p, int r) {
    struct point *pnt=g_alloca(sizeof(struct point)*(r*4+64));
    int i=0;
    if(this_->meth.draw_circle) {
        struct point p_scaled;
        p_scaled = graphics_dpi_scale_point(this_,p);
        this_->meth.draw_circle(this_->priv, gc->priv, &p_scaled, graphics_dpi_scale(this_,r));
    } else {
        /* do not scale circle_to_points */
        circle_to_points(p, r, 0, -1, 1026, pnt, &i, 1);
        pnt[i] = pnt[0];
        i++;
        graphics_draw_lines(this_, gc, pnt, i);
    }
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void graphics_draw_rectangle(struct graphics *this_, struct graphics_gc *gc, struct point *p, int w, int h) {
    struct point p_scaled;
    p_scaled = graphics_dpi_scale_point(this_,p);
    this_->meth.draw_rectangle(this_->priv, gc->priv, &p_scaled, graphics_dpi_scale(this_,w), graphics_dpi_scale(this_,h));
}

/**
 * @brief Draw a plain polygon on the display
 *
 * @param gra The graphics instance on which to draw
 * @param gc The graphics context
 * @param[in] pin An array of points forming the polygon
 * @param count_in The number of elements inside @p pin
 */
static void graphics_draw_polygon(struct graphics *gra, struct graphics_gc *gc, struct point *pin, int count_in) {
    if (! gra->meth.draw_polygon) {
        return;
    } else {
        struct point * pin_scaled =  g_alloca(sizeof (struct point)*count_in);
        int a;
        for(a=0; a < count_in; a ++)
            pin_scaled[a] = graphics_dpi_scale_point(gra,&(pin[a]));
        gra->meth.draw_polygon(gra->priv, gc->priv, pin_scaled, count_in);
    }
}

/**
 * @brief Draw a plain polygon with holes on the display
 *
 * @param gra The graphics instance on which to draw
 * @param gc The graphics context
 * @param[in] pin An array of points forming the polygon
 * @param count_in The number of elements inside @p pin
 * @param hole_count The number of hole polygons to cut out
 * @param pcount array of [hole_count] integers giving the number of
 *        points per hole
 * @param holes array of point arrays for the hole polygons
 */
static void graphics_draw_polygon_with_holes(struct graphics *gra, struct graphics_gc *gc, struct point *pin,
        int count_in, int hole_count, int* ccount, struct point **holes) {
    if (! gra->meth.draw_polygon_with_holes) {
        /* TODO: add attr to configure if polygons with holes should be drawn without
         *       the holes if no graphics support for this is present.
         */
        graphics_draw_polygon(gra, gc, pin, count_in);
        return;
    } else {
        struct point * pin_scaled = g_alloca(sizeof (struct point)*count_in);
        struct point ** holes_scaled = g_alloca(sizeof (struct point *)*hole_count);
        int a;
        int b;
        /* scale the outline */
        for(a=0; a < count_in; a ++)
            pin_scaled[a] = graphics_dpi_scale_point(gra,&(pin[a]));
        /*scale the holes */
        for(b=0; b < hole_count; b ++) {
            holes_scaled[b] = g_malloc(sizeof(*(holes_scaled[b])) * ccount[b]);
            for(a=0; a < ccount[b]; a ++)
                holes_scaled[b][a] = graphics_dpi_scale_point(gra,&(holes[b][a]));
        }
        gra->meth.draw_polygon_with_holes(gra->priv, gc->priv, pin_scaled, count_in, hole_count, ccount, holes_scaled);
        /* free the hole arrays */
        for(b=0; b < hole_count; b ++)
            g_free(holes_scaled[b]);
    }
}

void graphics_draw_rectangle_rounded(struct graphics *this_, struct graphics_gc *gc, struct point *plu, int w, int h,
                                     int r, int fill) {
    struct point *p=g_alloca(sizeof(struct point)*(r*4+32));
    struct point pi0= {plu->x+r,plu->y+r};
    struct point pi1= {plu->x+w-r,plu->y+r};
    struct point pi2= {plu->x+w-r,plu->y+h-r};
    struct point pi3= {plu->x+r,plu->y+h-r};
    int i=0;

    circle_to_points(&pi2, r*2, 0, -1, 258, p, &i, 1);
    circle_to_points(&pi1, r*2, 0, 255, 258, p, &i, 1);
    circle_to_points(&pi0, r*2, 0, 511, 258, p, &i, 1);
    circle_to_points(&pi3, r*2, 0, 767, 258, p, &i, 1);
    p[i]=p[0];
    i++;
    if (fill)
        graphics_draw_polygon(this_,gc,p,i);
    else
        graphics_draw_lines(this_,gc,p,i);
}


/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void graphics_draw_text(struct graphics *this_, struct graphics_gc *gc1, struct graphics_gc *gc2,
                        struct graphics_font *font, char *text, struct point *p, int dx, int dy) {
    struct point p_scaled;
    p_scaled = graphics_dpi_scale_point(this_,p);
    this_->meth.draw_text(this_->priv, gc1->priv, gc2 ? gc2->priv : NULL, font->priv, text, &p_scaled, dx, dy);
}


/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void graphics_get_text_bbox(struct graphics *this_, struct graphics_font *font, char *text, int dx, int dy,
                            struct point *ret, int estimate) {
    this_->meth.get_text_bbox(this_->priv, font->priv, text, dx, dy, ret, estimate);
    ret[0]=graphics_dpi_unscale_point(this_,&(ret[0]));
    ret[1]=graphics_dpi_unscale_point(this_,&(ret[1]));
    ret[2]=graphics_dpi_unscale_point(this_,&(ret[2]));
    ret[3]=graphics_dpi_unscale_point(this_,&(ret[3]));
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void graphics_overlay_disable(struct graphics *this_, int disable) {
    this_->disabled = disable;
    if (this_->meth.overlay_disable)
        this_->meth.overlay_disable(this_->priv, disable);
}

int  graphics_is_disabled(struct graphics *this_) {
    return this_->disabled || (this_->parent && this_->parent->disabled);
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
void graphics_draw_image(struct graphics *this_, struct graphics_gc *gc, struct point *p, struct graphics_image *img) {
    struct point p_scaled;
    p_scaled = graphics_dpi_scale_point(this_,p);
    this_->meth.draw_image(this_->priv, gc->priv, &p_scaled, img->priv);
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
static void graphics_draw_image_warp(struct graphics *this_, struct graphics_gc *gc, struct point *p, int count,
                                     struct graphics_image *img) {
    if(this_->meth.draw_image_warp) {
        struct point * p_scaled =  g_alloca(sizeof (struct point)*count);
        int a;
        for(a=0; a < count; a ++)
            p_scaled[a] = graphics_dpi_scale_point(this_,&(p[a]));
        this_->meth.draw_image_warp(this_->priv, gc->priv, p_scaled, count, img->priv);
    } else {
        dbg(lvl_error,"draw_image_warp not supported by graphics driver");
    }
}

//##############################################################################################################
//# Description:
//# Comment:
//# Authors: Martin Schaller (04/2008)
//##############################################################################################################
int graphics_draw_drag(struct graphics *this_, struct point *p) {
    struct point p_scaled;
    if (!this_->meth.draw_drag)
        return 0;
    p_scaled = graphics_dpi_scale_point(this_,p);
    this_->meth.draw_drag(this_->priv, &p_scaled);
    return 1;
}

void graphics_background_gc(struct graphics *this_, struct graphics_gc *gc) {
    this_->meth.background_gc(this_->priv, gc ? gc->priv : NULL);
}


/**
 * @brief Shows the native on-screen keyboard or other input method
 *
 * This method is a wrapper around the respective method of the graphics plugin.
 *
 * The caller should populate the {@code kbd} argument with appropriate {@code mode} and {@code lang}
 * members so the graphics plugin can determine the best matching layout.
 *
 * If an input method is shown, the graphics plugin should try to select the configuration which best
 * matches the specified {@code mode}. For example, if {@code mode} specifies a numeric layout, the
 * graphics plugin should select a numeric keyboard layout (if available), or the equivalent for another
 * input method (such as setting stroke recognition to identify strokes as numbers). Likewise, when an
 * alphanumeric-uppercase mode is requested, it should switch to uppercase input.
 *
 * Implementations should, however, consider that Navit's internal keyboard allows the user to switch
 * modes at will (the only exception being degree mode) and thus must not "lock" the user into a limited
 * layout with no means to switch to a general-purpose one. For example, house number entry in an
 * address search dialog may default to numeric mode, but since some house numbers may contain
 * non-numeric characters, a pure numeric keyboard is suitable only if the user has the option to switch
 * to an alphanumeric layout.
 *
 * When multiple alphanumeric layouts are available, the graphics plugin should use the {@code lang}
 * argument to determine the best layout.
 *
 * When selecting an input method, preference should always be given to the default or last selected
 * input method and configuration if it matches the requested {@code mode} and {@code lang}.
 *
 * If the native input method is going to obstruct parts of Navit's UI, the graphics plugin should set
 * {@code kbd->w} and {@code kbd->h} to the height and width to the appropriate value in pixels. A value
 * of -1 indicates that the input method fills the entire available width or height of the space
 * available to Navit. On windowed platforms, where the on-screen input method and Navit's window may be
 * moved relative to each other as needed and can be displayed alongside each other, the graphics plugin
 * should report 0 for both dimensions.
 *
 * @param this_ The graphics instance
 * @param kbd The keyboard instance
 *
 * @return 1 if the native keyboard is going to be displayed, 0 if not, -1 if the method is not
 * supported by the plugin
 */
int graphics_show_native_keyboard (struct graphics *this_, struct graphics_keyboard *kbd) {
    int ret;
    if (!this_->meth.show_native_keyboard)
        ret = -1;
    else
        ret = this_->meth.show_native_keyboard(kbd);
    dbg(lvl_debug, "return %d", ret);
    return ret;
}


/**
 * @brief Hides the native on-screen keyboard or other input method
 *
 * This method is a wrapper around the respective method of the graphics plugin.
 *
 * A call to this function indicates that Navit no longer needs the input method and is about to reclaim
 * any screen real estate it may have previously reserved for the input method.
 *
 * On platforms that don't support overlapping windows this means that the on-screen input method should
 * be hidden, as it may otherwise obstruct parts of Navit's UI.
 *
 * On windowed platforms, where on-screen input methods can be displayed alongside Navit or moved around
 * as needed, the graphics driver should instead notify the on-screen method that it is no longer
 * expecting user input, allowing the input method to take the appropriate action.
 *
 * The graphics plugin must free any data it has stored in {@code kbd->gra_priv} and reset the pointer
 * to {@code NULL} to indicate it has done so.
 *
 * The caller may free {@code kbd} after this function returns.
 *
 * @param this The graphics instance
 * @param kbd The keyboard instance
 *
 * @return True if the call was successfully passed to the plugin, false if the method is not supported
 * by the plugin
 */
int graphics_hide_native_keyboard (struct graphics *this_, struct graphics_keyboard *kbd) {
    if (!this_->meth.hide_native_keyboard)
        return 0;
    this_->meth.hide_native_keyboard(kbd);
    return 1;
}

#include "attr.h"
#include "popup.h"
#include <stdio.h>

struct displayitem_poly_holes {
    int count;
    int *ccount;
    struct coord ** coords;
};

/**
 * @brief graphics display item structure
 *
 * The graphics item passes the ap items and other items with this structure
 * to the graphics drawing routines. The struct is only a stub. It is allocated
 * including "count -1" struct coord's following c[0], if "holes" not NULL, by a
 * polygon hole structure, and if label != NULL, a series of zero terminated
 * strings followed by another zero for label.
*/
struct displayitem {
    struct displayitem *next;
    struct item item;
    char *label;
    struct displayitem_poly_holes * holes;
    int z_order;
    int count;
    struct coord c[0];
};

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
static void xdisplay_free(struct displaylist *dl) {
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
 * @brief add the holes structure into preallocated area after displayitem
 *
 * @param item to extract holes from
 * @param hole_count precounted number of holes
 * @param p changeable pointer to buffer. Advanced by the size used
 * @returns pointer to newly created holes structure
 */
static inline struct displayitem_poly_holes *  display_add_holes(struct item *item,int hole_count,  char ** p) {
    struct attr attr;
    struct displayitem_poly_holes* holes;
    holes=(struct displayitem_poly_holes *) *p;
    *p+=sizeof(*holes);
    holes->count=0;
    holes->ccount = (int *) *p;
    *p+=hole_count * sizeof(int);
    holes->coords = (struct coord **)*p;
    *p+=hole_count * sizeof(struct coord *);
    item_attr_rewind(item);
    while(item_attr_get(item, attr_poly_hole, &attr)) {
        holes->coords[holes->count] = (struct coord *)*p;
        holes->ccount[holes->count] = attr.u.poly_hole->coord_count;
        memcpy(holes->coords[holes->count], attr.u.poly_hole->coord, holes->ccount[holes->count] * sizeof(struct coord));
        *p += holes->ccount[holes->count] * sizeof(struct coord);
        holes->count ++;
    }
    return holes;
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
static void display_add(struct hash_entry *entry, struct item *item, int count, struct coord *c, char **label,
                        int label_count) {
    struct displayitem *di;
    int len,i;
    char *p;
    struct attr attr;
    int hole_count=0;
    int hole_total_coords=0;
    int holes_length;

    /* calculate number of bytes required */
    /* own length */
    len=sizeof(*di)+count*sizeof(*c);
    /* add length of lables including closing zero */
    if (label && label_count) {
        for (i = 0 ; i < label_count ; i++) {
            if (label[i])
                len+=strlen(label[i])+1;
            else
                len++;
        }
    }
    /* add length for holes */
    item_attr_rewind(item);
    while(item_attr_get(item, attr_poly_hole, &attr)) {
        hole_count ++;
        hole_total_coords += attr.u.poly_hole->coord_count;
    }
    holes_length = sizeof(struct displayitem_poly_holes) + hole_count * sizeof(int) + hole_count * sizeof(
                       struct coord *) + hole_total_coords * sizeof(struct coord);
    if(hole_count > 0)
        dbg(lvl_debug,"got %d holes with %d coords total", hole_count, hole_total_coords);
    len += holes_length;

    p=g_malloc(len);

    di=(struct displayitem *)p;
    p+=sizeof(*di)+count*sizeof(*c);
    di->item=*item;
    di->z_order=0;
    di->holes=NULL;
    if(hole_count > 0) {
        di->holes = display_add_holes(item, hole_count, &p);
    }
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
static void label_line(struct graphics *gra, struct graphics_gc *fg, struct graphics_gc *bg, struct graphics_font *font,
                       struct point *p, int count, char *label) {
    int i,x,y,tl,tlm,th,thm,tlsq,l;
    float lsq;
    double dx,dy;
    struct point p_t;
    struct point pb[5];

    if (gra->meth.get_text_bbox) {
        graphics_get_text_bbox(gra,font,label,0x10000, 0x00, pb, 1);
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
            if (x < gra->r.rl.x && x + tl > gra->r.lu.x && y + tl > gra->r.lu.y && y - tl < gra->r.rl.y)
                graphics_draw_text(gra, fg, bg, font, label, &p_t, dx*0x10000/l, dy*0x10000/l);
        }
    }
}

static void display_draw_arrow(struct point *p, int dx, int dy, int l, struct graphics_gc *gc, struct graphics *gra) {
    struct point pnt[3];
    pnt[0]=pnt[1]=pnt[2]=*p;
    pnt[0].x+=-dx*l/65536+dy*l/65536;
    pnt[0].y+=-dy*l/65536-dx*l/65536;
    pnt[2].x+=-dx*l/65536-dy*l/65536;
    pnt[2].y+=-dy*l/65536+dx*l/65536;
    graphics_draw_lines(gra, gc, pnt, 3);
}

static void display_draw_arrows(struct graphics *gra, struct graphics_gc *gc, struct point *pnt, int count) {
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

static int intersection(struct point * a1, int adx, int ady, struct point * b1, int bdx, int bdy, struct point * res) {
    int n, a, b;
    dbg(lvl_debug,"%d,%d - %d,%d x %d,%d-%d,%d",a1->x,a1->y,a1->x+adx,a1->y+ady,b1->x,b1->y,b1->x+bdx,b1->y+bdy);
    n = bdy * adx - bdx * ady;
    a = bdx * (a1->y - b1->y) - bdy * (a1->x - b1->x);
    b = adx * (a1->y - b1->y) - ady * (a1->x - b1->x);
    dbg(lvl_debug,"a %d b %d n %d",a,b,n);
    if (n < 0) {
        n = -n;
        a = -a;
        b = -b;
    }
    if (n == 0)
        return 0;
    res->x = a1->x + a * adx / n;
    res->y = a1->y + a * ady / n;
    dbg(lvl_debug,"%d,%d",res->x,res->y);
    return 1;
}

struct circle {
    short x,y,fowler;
} circle64[]= {
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

/**
 * @brief Create a set of points on a circle or on a circular arc
 *
 * @param center Center point of the circle
 * @param diameter Diameter of the circle
 * @param scale Unused
 * @param start Position of the first point on the circle (in 1/1024th of the circle), -1 being the bottom of the circle, 511 being the top of the circle
 * @param len Length of the arc on the circle, relative to start (in 1/1024th of the circle), 514 is half a circle, 1028 is a full circle (or 1027 if first and last points are connected with a line)
 * @param[out] res Returned an array of points that will form the resulting circle
 * @param[out] pos Index of the last point filled inside array @p res
 * @param dir Direction of the circle (valid values are 1 (counter-clockwise) or -1 (clockwise), other values may lead to unknown result)
 */
static void circle_to_points(const struct point *center, int diameter, int scale, int start, int len, struct point *res,
                             int *pos, int dir) {
    struct circle *c;
    int count=64;
    int end=start+len;
    int i,step;
    c=circle64;
    if (diameter > 128)
        step=1;
    else if (diameter > 64)
        step=2;
    else if (diameter > 16)
        step=4;
    else if (diameter > 4)
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
                    res[*pos].x=center->x+((c[i].x*diameter+128)>>8);
                    res[*pos].y=center->y+((c[i].y*diameter+128)>>8);
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
                    res[*pos].x=center->x+((c[i].x*diameter+128)>>8);
                    res[*pos].y=center->y+((c[i].y*diameter+128)>>8);
                    (*pos)+=dir;
                }
                i-=step;
            }
            start+=1024;
            end+=1024;
        }
    }
}


static int fowler(int dy, int dx) {
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

struct draw_polyline_shape {
    int wi;
    int step;
    int fow;
    int dx,dy;
    int dxw,dyw;
    int l,lscale;
};
struct draw_polyline_context {
    int prec;
    int ppos,npos;
    struct point *res;
    struct draw_polyline_shape shape;
    struct draw_polyline_shape prev_shape;
};

static void draw_shape_update(struct draw_polyline_shape *shape) {
    shape->dxw = -(shape->dx * shape->wi * shape->lscale) / shape->l;
    shape->dyw = (shape->dy * shape->wi * shape->lscale) / shape->l;
}

static void draw_shape(struct draw_polyline_context *ctx, struct point *pnt, int wi) {
    int dxs,dys,lscales;
    int lscale=16;
    int l;
    struct draw_polyline_shape *shape=&ctx->shape;
    struct draw_polyline_shape *prev=&ctx->prev_shape;

    *prev=*shape;
    if (prev->wi != wi && prev->l) {
        prev->wi=wi;
        draw_shape_update(prev);
    }
    shape->wi=wi;
    shape->dx = (pnt[1].x - pnt[0].x);
    shape->dy = (pnt[1].y - pnt[0].y);
    if (wi > 16)
        shape->step=4;
    else if (wi > 8)
        shape->step=8;
    else
        shape->step=16;
    dxs=shape->dx*shape->dx;
    dys=shape->dy*shape->dy;
    lscales=lscale*lscale;
    if (dxs + dys > lscales)
        l = uint_sqrt(dxs+dys)*lscale;
    else
        l = uint_sqrt((dxs+dys)*lscales);

    shape->fow=fowler(-shape->dy, shape->dx);
    dbg(lvl_debug,"fow=%d",shape->fow);
    if (! l)
        l=1;
    if (wi*lscale > 10000)
        lscale=10000/wi;
    dbg_assert(wi*lscale <= 10000);
    shape->l=l;
    shape->lscale=lscale;
    shape->wi=wi;
    draw_shape_update(shape);
}

static void draw_point(struct draw_polyline_shape *shape, struct point *src, struct point *dst, int pos) {
    if (pos) {
        dst->x=(src->x*2-shape->dyw)/2;
        dst->y=(src->y*2-shape->dxw)/2;
    } else {
        dst->x=(src->x*2+shape->dyw)/2;
        dst->y=(src->y*2+shape->dxw)/2;
    }
}

static void draw_begin(struct draw_polyline_context *ctx, struct point *p) {
    struct draw_polyline_shape *shape=&ctx->shape;
    int i;
    for (i = 0 ; i <= 32 ; i+=shape->step) {
        ctx->res[ctx->ppos].x=(p->x*256+(shape->dyw*circle64[i].y)+(shape->dxw*circle64[i].x))/256;
        ctx->res[ctx->ppos].y=(p->y*256+(shape->dxw*circle64[i].y)-(shape->dyw*circle64[i].x))/256;
        ctx->ppos++;
    }
}

static int draw_middle(struct draw_polyline_context *ctx, struct point *p) {
    int delta=ctx->prev_shape.fow-ctx->shape.fow;
    if (delta > 512)
        delta-=1024;
    if (delta < -512)
        delta+=1024;
    if (delta < 16 && delta > -16) {
        draw_point(&ctx->shape, p, &ctx->res[ctx->npos--], 0);
        draw_point(&ctx->shape, p, &ctx->res[ctx->ppos++], 1);
        return 1;
    }
    dbg(lvl_debug,"delta %d",delta);
    if (delta > 0) {
        struct point pos,poso;
        draw_point(&ctx->shape, p, &pos, 1);
        draw_point(&ctx->prev_shape, p, &poso, 1);
        if (delta >= 256)
            return 0;
        if (intersection(&pos, ctx->shape.dx, ctx->shape.dy, &poso, ctx->prev_shape.dx, ctx->prev_shape.dy,
                         &ctx->res[ctx->ppos])) {
            ctx->ppos++;
            draw_point(&ctx->prev_shape, p, &ctx->res[ctx->npos--], 0);
            draw_point(&ctx->shape, p, &ctx->res[ctx->npos--], 0);
            return 1;
        }
    } else {
        struct point neg,nego;
        draw_point(&ctx->shape, p, &neg, 0);
        draw_point(&ctx->prev_shape, p, &nego, 0);
        if (delta <= -256)
            return 0;
        if (intersection(&neg, ctx->shape.dx, ctx->shape.dy, &nego, ctx->prev_shape.dx, ctx->prev_shape.dy,
                         &ctx->res[ctx->npos])) {
            ctx->npos--;
            draw_point(&ctx->prev_shape, p, &ctx->res[ctx->ppos++], 1);
            draw_point(&ctx->shape, p, &ctx->res[ctx->ppos++], 1);
            return 1;
        }
    }
    return 0;
}

static void draw_end(struct draw_polyline_context *ctx, struct point *p) {
    int i;
    struct draw_polyline_shape *shape=&ctx->prev_shape;
    for (i = 0 ; i <= 32 ; i+=shape->step) {
        ctx->res[ctx->npos].x=(p->x*256+(shape->dyw*circle64[i].y)-(shape->dxw*circle64[i].x))/256;
        ctx->res[ctx->npos].y=(p->y*256+(shape->dxw*circle64[i].y)+(shape->dyw*circle64[i].x))/256;
        ctx->npos--;
    }
}

static void draw_init_ctx(struct draw_polyline_context *ctx, int maxpoints) {
    ctx->prec=1;
    ctx->ppos=maxpoints/2;
    ctx->npos=maxpoints/2-1;
}


static void graphics_draw_polyline_as_polygon(struct graphics *gra, struct graphics_gc *gc,
        struct point *pnt, int count, int *width) {
    int maxpoints=200;
    struct draw_polyline_context ctx;
    int i=0;
    int max_circle_points=20;
    if (count < 2)
        return;
    ctx.shape.l=0;
    ctx.shape.wi=0;
    ctx.res=g_alloca(sizeof(struct point)*maxpoints);
    i=0;
    draw_init_ctx(&ctx, maxpoints);
    draw_shape(&ctx, pnt, *width++);
    draw_begin(&ctx,&pnt[0]);
    for (i = 1 ; i < count -1 ; i++) {
        draw_shape(&ctx, pnt+i, *width++);
        if (ctx.npos < max_circle_points || ctx.ppos >= maxpoints-max_circle_points || !draw_middle(&ctx,&pnt[i])) {
            draw_end(&ctx,&pnt[i]);
            ctx.res[ctx.npos]=ctx.res[ctx.ppos-1];
            graphics_draw_polygon(gra, gc, ctx.res+ctx.npos, ctx.ppos-ctx.npos);
            draw_init_ctx(&ctx, maxpoints);
            draw_begin(&ctx,&pnt[i]);
        }
    }
    draw_shape(&ctx, &pnt[count-2], *width++);
    ctx.prev_shape=ctx.shape;
    draw_end(&ctx,&pnt[count-1]);
    ctx.res[ctx.npos]=ctx.res[ctx.ppos-1];
    graphics_draw_polygon(gra, gc, ctx.res+ctx.npos, ctx.ppos-ctx.npos);
}


struct wpoint {
    int x,y,w;
};

enum relative_pos {
    INSIDE   = 0,
    LEFT_OF  = 1,
    RIGHT_OF = 2,
    ABOVE    = 4,
    BELOW    = 8
};

static int relative_pos(struct wpoint *p, struct point_rect *r) {
    int relative_pos=INSIDE;
    if (p->x < r->lu.x)
        relative_pos=LEFT_OF;
    else if (p->x > r->rl.x)
        relative_pos=RIGHT_OF;
    if (p->y < r->lu.y)
        relative_pos |=ABOVE;
    else if (p->y > r->rl.y)
        relative_pos |=BELOW;
    return relative_pos;
}

static void clip_line_endoint_to_rect_edge(struct wpoint *p, int rel_pos, int dx, int dy, int dw,
        struct point_rect *clip_rect) {
    // We must cast to float to avoid integer
    // overflow (i.e. undefined behaviour) at high
    // zoom levels.
    if (rel_pos & LEFT_OF) {
        p->y+=(((float)clip_rect->lu.x)-p->x)*dy/dx;
        p->w+=(((float)clip_rect->lu.x)-p->x)*dw/dx;
        p->x=clip_rect->lu.x;
    } else if (rel_pos & RIGHT_OF) {
        p->y+=(((float)clip_rect->rl.x)-p->x)*dy/dx;
        p->w+=(((float)clip_rect->rl.x)-p->x)*dw/dx;
        p->x=clip_rect->rl.x;
    } else if (rel_pos & ABOVE) {
        p->x+=(((float)clip_rect->lu.y)-p->y)*dx/dy;
        p->w+=(((float)clip_rect->lu.y)-p->y)*dw/dy;
        p->y=clip_rect->lu.y;
    } else if (rel_pos & BELOW) {
        p->x+=(((float)clip_rect->rl.y)-p->y)*dx/dy;
        p->w+=(((float)clip_rect->rl.y)-p->y)*dw/dy;
        p->y=clip_rect->rl.y;
    }
}

enum clip_result {
    CLIPRES_INVISIBLE     = 0,
    CLIPRES_VISIBLE       = 1,
    CLIPRES_START_CLIPPED = 2,
    CLIPRES_END_CLIPPED   = 4,
};

static int clip_line(struct wpoint *p1, struct wpoint *p2, struct point_rect *clip_rect) {
    int rel_pos1,rel_pos2;
    int ret = CLIPRES_VISIBLE;
    int dx,dy,dw;
    rel_pos1=relative_pos(p1, clip_rect);
    if (rel_pos1!=INSIDE)
        ret |= CLIPRES_START_CLIPPED;
    rel_pos2=relative_pos(p2, clip_rect);
    if (rel_pos2!=INSIDE)
        ret |= CLIPRES_END_CLIPPED;
    dx=p2->x-p1->x;
    dy=p2->y-p1->y;
    dw=p2->w-p1->w;
    while ((rel_pos1!=INSIDE) || (rel_pos2!=INSIDE)) {
        if (rel_pos1 & rel_pos2)
            return CLIPRES_INVISIBLE;
        clip_line_endoint_to_rect_edge(p1, rel_pos1, dx, dy, dw, clip_rect);
        rel_pos1=relative_pos(p1, clip_rect);
        if (rel_pos1 & rel_pos2)
            return CLIPRES_INVISIBLE;
        clip_line_endoint_to_rect_edge(p2, rel_pos2, dx, dy, dw, clip_rect);
        rel_pos2=relative_pos(p2, clip_rect);
    }
    return ret;
}

/**
 * @brief Draw polyline on the display
 *
 * Polylines are a serie of lines connected to each other.
 *
 * @param gra The graphics instance on which to draw
 * @param gc The graphics context
 * @param[in] pin An array of points forming the polygon
 * @param count_in The number of elements inside @p pin
 * @param[in] width An array of width matching the line starting from the corresponding @p pa (if all equal, all lines will have the same width)
 * @param poly A boolean indicating whether the polyline should be closed to form a polygon (only the contour of this polygon will be drawn)
 */
void graphics_draw_polyline_clipped(struct graphics *gra, struct graphics_gc *gc, struct point *pa, int count,
                                    int *width, int poly) {
    struct point *points_to_draw=g_alloca(sizeof(struct point)*(count+1));
    int *w=g_alloca(sizeof(int)*(count+1));
    struct wpoint segment_start,segment_end;
    int i,points_to_draw_cnt=0;
    int clip_result;
    int r_width, r_height;
    struct point_rect r=gra->r;

    r_width=r.rl.x-r.lu.x;
    r_height=r.rl.y-r.lu.y;

    // Expand clipping rect by 1/3 so wide, slanted lines do not
    // partially end before screen border.
    // Ideally we would expand by the line width here, but in 3D
    // mode the width is variable and needs clipping itself, so that
    // would get complicated. Anyway, 1/3 of screen size should be
    // enough...
    r.lu.x-=r_width/3;
    r.lu.y-=r_height/3;
    r.rl.x+=r_width/3;
    r.rl.y+=r_height/3;
    // Iterate over line segments, push them into points_to_draw
    // until we reach a completely invisible segment...
    for (i = 0 ; i < count ; i++) {
        if (i) {
            segment_start.x=pa[i-1].x;
            segment_start.y=pa[i-1].y;
            segment_start.w=width[(i-1)];
            segment_end.x=pa[i].x;
            segment_end.y=pa[i].y;
            segment_end.w=width[i];
            dbg(lvl_debug, "Segment: [%d, %d] - [%d, %d]...", segment_start.x, segment_start.y, segment_end.x, segment_end.y);
            clip_result=clip_line(&segment_start, &segment_end, &r);
            if (clip_result != CLIPRES_INVISIBLE) {
                dbg(lvl_debug, "....clipped to [%d, %d] - [%d, %d]", segment_start.x, segment_start.y, segment_end.x, segment_end.y);
                if ((i == 1) || (clip_result & CLIPRES_START_CLIPPED)) {
                    points_to_draw[points_to_draw_cnt].x=segment_start.x;
                    points_to_draw[points_to_draw_cnt].y=segment_start.y;
                    w[points_to_draw_cnt]=segment_start.w;
                    points_to_draw_cnt++;
                }
                points_to_draw[points_to_draw_cnt].x=segment_end.x;
                points_to_draw[points_to_draw_cnt].y=segment_end.y;
                w[points_to_draw_cnt]=segment_end.w;
                points_to_draw_cnt++;
            }
            if ((i == count-1) || (clip_result & CLIPRES_END_CLIPPED)) {
                // ... then draw the resulting polyline
                if (points_to_draw_cnt > 1) {
                    if (poly) {
                        graphics_draw_polyline_as_polygon(gra, gc, points_to_draw, points_to_draw_cnt, w);
                    } else
                        graphics_draw_lines(gra, gc, points_to_draw, points_to_draw_cnt);
                    points_to_draw_cnt=0;
                }
            }
        }
    }
}

static int is_inside(struct point *p, struct point_rect *r, int edge) {
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

static void poly_intersection(struct point *p1, struct point *p2, struct point_rect *r, int edge, struct point *ret) {
    int dx=p2->x-p1->x;
    int dy=p2->y-p1->y;
    switch(edge) {
    case 0:
        ret->y=p1->y+((float)r->lu.x-p1->x)*dy/dx;
        ret->x=r->lu.x;
        break;
    case 1:
        ret->y=p1->y+((float)r->rl.x-p1->x)*dy/dx;
        ret->x=r->rl.x;
        break;
    case 2:
        ret->x=p1->x+((float)r->lu.y-p1->y)*dx/dy;
        ret->y=r->lu.y;
        break;
    case 3:
        ret->x=p1->x+((float)r->rl.y-p1->y)*dx/dy;
        ret->y=r->rl.y;
        break;
    }
}

/**
 * @brief clip a polygon inside a rectangle
 *
 * This function clippes a given polygon inside a rectangle. It writes the result into provided buffer.
 *
 * @param[in] r rectangle to clip into
 * @param[in] pin point array of input polygon
 * @param[in] count_in number of points in pin
 * @param[out] out preallocated buffer of at least count_in *8 +1 points size
 * @param[out] count_out size of out number of points, number of points used in out at return
 */
static void graphics_clip_polygon(struct point_rect * r, struct point * in, int count_in, struct point *out,
                                  int* count_out) {
    /* set our self a limit for the in stack buffer. To not overflow stack */
    const int limit=10000;
    /* get a temp buffer to store points after one direction clipping.
     * since we are clipping 4 directions, result is always in out at the end*/
    struct point *temp=g_alloca(sizeof(struct point) * (count_in < limit ? count_in*8+1:0));
    struct point *pout;
    struct point *pin;
    int edge;
    int count;

    /* sanity check */
    if((r == NULL) || (in == NULL) || (out == NULL) || (count_out == NULL) || (*count_out < count_in*8+1)) {
        return;
    }

    /* prepare buffers. We have two buffers that we flip over.
     * 1. the output buffer
     * 2. temp
     */
    if (count_in >= limit) {
        /* too big. Allocate a buffer (slower) */
        temp=g_new(struct point, count_in*8+1);
    }
    /* use temp as first buffer. So we get the final result in out*/
    pout = temp;
    /* start with input polygon */
    pin=in;
    /* start with number of points of source polygon*/
    count=count_in;

    /* clip all four directions of a rectangle */
    for (edge = 0 ; edge < 4 ; edge++) {
        int i;
        /* p is first element in current buffer */
        struct point *p=pin;
        /* s is lasst element in current buffer */
        struct point *s=pin+count-1;
        /* nothing written yet */
        *count_out=0;

        /* iterate all points in current buffer */
        for (i = 0 ; i < count ; i++) {
            if (is_inside(p, r, edge)) {
                if (! is_inside(s, r, edge)) {
                    struct point pi;
                    /* current segment crosses border from outside to inside. Add crossing point with border first */
                    poly_intersection(s,p,r,edge,&pi);
                    pout[(*count_out)++]=pi;
                }
                /* add point if inside */
                pout[(*count_out)++]=*p;
            } else {
                if (is_inside(s, r, edge)) {
                    struct point pi;
                    /*current segment crosses border from inside to outside. Add crossing point with border */
                    poly_intersection(p,s,r,edge,&pi);
                    pout[(*count_out)++]=pi;
                }
                /* skip point if outside */
            }
            /* move one coordinate forward */
            s=p;
            p++;
        }
        /* use result of last clipping for next */
        count=*count_out;

        /* switch buffer */
        if(pout == temp) {
            pout=out;
            pin=temp;
        } else {
            pin=out;
            pout=temp;
        }
    }

    /* have clipped poly in out. And number of points now in *count_out */

    /* if we had to allocate the buffer, we need to free it */
    if (count_in >= limit) {
        g_free(temp);
    }
    return;
}

/**
 * @brief Draw a plain polygon on the display
 *
 * @param gra The graphics instance on which to draw
 * @param gc The graphics context
 * @param[in] pin An array of points forming the polygon
 * @param count_in The number of elements inside @p pin
 */
void graphics_draw_polygon_clipped(struct graphics *gra, struct graphics_gc *gc, struct point *pin, int count_in) {
    struct point_rect r=gra->r;
    int limit=10000;
    struct point *pa1=g_alloca(sizeof(struct point) * (count_in < limit ? count_in*8+1:0));
    struct point *clipped;
    int count_out = count_in*8+1;

    /* prepare buffer */
    if (count_in < limit) {
        /* use on stack buffer */
        clipped=pa1;
    } else {
        /* too big. allocate buffer (slower) */
        clipped=g_new(struct point, count_in*8+1);
    }

    graphics_clip_polygon(&r, pin, count_in, clipped, &count_out);
    graphics_draw_polygon(gra, gc, clipped, count_out);

    /* if we had to allocate buffer, free it */
    if (count_in >= limit) {
        g_free(clipped);
    }
}

/**
 * @brief Draw a plain polygon with holes on the display
 *
 * @param gra The graphics instance on which to draw
 * @param gc The graphics context
 * @param[in] pin An array of points forming the polygon
 * @param count_in The number of elements inside @p pin
 * @param hole_count The number of hole polygons to cut out
 * @param pcount array of [hole_count] integers giving the number of
 *        points per hole
 * @param holes array of point arrays for the hole polygons
 */
static void graphics_draw_polygon_with_holes_clipped(struct graphics *gra, struct graphics_gc *gc, struct point *pin,
        int count_in, int hole_count, int* ccount, struct point **holes) {
    int i;
    struct point_rect r=gra->r;
    int limit=10000;
    struct point *pa1;
    struct point *clipped;
    int total_count_in;
    int count_out;
    int count_used;
    int found_hole_count;
    int *found_ccount;
    struct point ** found_holes;
    /* get total node count for polygon plus all holes */
    total_count_in = count_in;
    for(i = 0; i < hole_count; i ++) {
        total_count_in += ccount[i];
    }
    count_out = total_count_in*8+1+hole_count;

    /* prepare buffer */
    pa1=g_alloca(sizeof(struct point) * (total_count_in < limit ? total_count_in*8+1:0));
    if (count_in < limit) {
        /* use on stack buffer */
        clipped=pa1;
    } else {
        /* too big. allocate buffer (slower) */
        clipped=g_new(struct point, count_in*8+1);
    }
    count_used=0;

    /* prepare arrays for new holes */
    found_ccount=g_alloca(sizeof(int)*hole_count);
    found_holes=g_alloca(sizeof(struct point*)*hole_count);
    found_hole_count=0;

    /* clip outer polygon */
    graphics_clip_polygon(&r, pin, count_in, clipped, &count_out);
    count_used += count_out;
    /* clip the holes */
    for (i=0; i < hole_count; i ++) {
        struct point* buffer = clipped + count_used;
        int count = total_count_in*8+1+hole_count - count_used;
        graphics_clip_polygon(&r, holes[i], ccount[i], buffer, &count);
        count_used +=count;
        if(count > 0) {
            /* only if there are points left after clipping */
            found_ccount[found_hole_count]=count;
            found_holes[found_hole_count]=buffer;
            found_hole_count ++;
        }
    }
    /* call drawing function */
    graphics_draw_polygon_with_holes(gra, gc, clipped, count_out, found_hole_count, found_ccount, found_holes);

    /* if we had to allocate buffer, free it */
    if (total_count_in >= limit) {
        g_free(clipped);
    }
}

static void display_context_free(struct display_context *dc) {
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

static struct graphics_font *get_font(struct graphics *gra, int size) {
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

void graphics_draw_text_std(struct graphics *this_, int text_size, char *text, struct point *p) {
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

char *graphics_icon_path(const char *icon) {
    static char *navit_sharedir;
    char *ret=NULL;
    struct file_wordexp *wordexp=NULL;
    dbg(lvl_debug,"enter %s",icon);
    if (strchr(icon, '$')) {
        wordexp=file_wordexp_new(icon);
        if (file_wordexp_get_count(wordexp))
            icon=file_wordexp_get_array(wordexp)[0];
    }
    if (strchr(icon,'/'))
        ret=g_strdup(icon);
    else {
#ifdef HAVE_API_ANDROID
        ret=g_strdup_printf("res/drawable/%s",icon);
#else
        if (! navit_sharedir)
            navit_sharedir = getenv("NAVIT_SHAREDIR");
        ret=g_strdup_printf("%s/icons/%s", navit_sharedir, icon);
#endif
    }
    if (wordexp)
        file_wordexp_destroy(wordexp);
    return ret;
}

static int limit_count(struct coord *c, int count) {
    int i;
    for (i = 1 ; i < count ; i++) {
        if (c[i].x == c[0].x && c[i].y == c[0].y)
            return i+1;
    }
    return count;
}

/**
 * @brief Draw a multi-line text next to a specified point @p pref
 *
 * @param gra The graphics instance on which to draw
 * @param fg The graphics color to use to draw the text
 * @param bg The graphics background color to use to draw the text
 * @param font The font to use to draw the text
 * @param pref The position to draw the text (draw at the right and vertically aligned relatively to this point)
 * @param label The text to draw (may contain '\n' for multiline text, if so lines will be stacked vertically)
 * @param line_spacing The delta between each line (set its value at to least the font text size, to be readable)
 */
static void multiline_label_draw(struct graphics *gra, struct graphics_gc *fg, struct graphics_gc *bg,
                                 struct graphics_font *font, struct point pref, const char *label, int line_spacing) {

    char *input_label=g_strdup(label);
    char *label_lines[10];	/* Max 10 lines of text */
    int label_nblines=0;
    int label_linepos=0;
    char *startline=input_label;
    char *endline=startline;
    while (endline && *endline!='\0') {
        while (*endline!='\0' && *endline!='\n') { /* Search for new line */
            endline=g_utf8_next_char(endline);
        }
        if (*endline=='\0')
            endline=NULL;	/* This means we reached the end of string */
        if (endline) /* Test if we got a new line character ('\n') */
            *endline='\0';	/* Terminate string at line ('\n') and print this line */
        label_lines[label_nblines++]=startline;
        if (endline==NULL)	/* endline is NULL, this was the last line of the multi-line string */
            break;
        endline++;	/* No need for g_utf8_next_char() here, as we know '\n' is a single byte UTF-8 char */
        startline=endline;	/* Start processing next line, by setting startline to its first character */
    }
    if (label_nblines>(sizeof(label_lines)/sizeof(char
                       *))) {	/* Does label_nblines overflows the number of entries in array label_lines? */
        dbg(lvl_warning,"Too many lines (%d) in label \"%s\", truncating to %lu", label_nblines, label,
            sizeof(label_lines)/sizeof(char *));
        label_nblines=sizeof(label_lines)/sizeof(char *);
    }
    /* Horizontally, we position the label next to the specified point (on the right handside) */
    pref.x+=1;
    /* Vertically, we center the text with respect to specified point */
    pref.y-=(label_nblines*line_spacing)/2;

    /* Parse all stored lines, and display them */
    for (label_linepos=0; label_linepos<label_nblines; label_linepos++) {
        graphics_draw_text(gra, fg, bg, font, label_lines[label_linepos],
                           &pref, 0x10000, 0);
        pref.y+=line_spacing;
    }
    g_free(input_label);
}

/**
 * @brief coordnate transfor hole coordinates
 *
 * This function transform a whole set of polygon holes. It therefore allocates memory
 * attached to a displayitem_poly_holes structure and call transform
 *
 * @param trans transformation to be used
 * @param pro projection to be used
 * @param in filled holes structure to transform
 * @param out structure to place result in. Remember to deallocate!
 * @param mindist minimal distance between points
 */
static inline void displayitem_transform_holes(struct transformation *trans, enum projection pro,
        struct displayitem_poly_holes * in, struct displayitem_poly_holes * out, int mindist) {
    if(out == NULL)
        return;
    out->count = 0;
    out->ccount=NULL;
    out->coords=NULL;
    if((in != NULL) && (in->count > 0)) {
        int a;
        /* alloc space for hole conversion. To be freed with displayitem_free_holes later*/
        out->count = in->count;
        out->ccount = g_malloc0(sizeof(*(out->ccount)) * in->count);
        out->coords = g_malloc0(sizeof(*(out->coords)) * in->count);
        for(a = 0; a < in->count; a ++) {
            in->ccount[a]=limit_count(in->coords[a], in->ccount[a]);
            out->coords[a]=g_malloc0(sizeof(*(out->coords[a])) * in->ccount[a]);
            out->ccount[a]=transform(trans, pro, in->coords[a], (struct point *)(out->coords[a]), in->ccount[a], mindist, 0, NULL);
        }
    }
}

/**
 * @brief free hole structure allocated by displayitem_transform_holes
 *
 * @param holes structure to deallocate
 */
static void displayitem_free_holes(struct displayitem_poly_holes * holes) {
    if(holes == NULL)
        return;
    if(holes->count > 0) {
        int a;
        for(a=0; a < holes->count; a ++) {
            g_free(holes->coords[a]);
        }
        g_free(holes->ccount);
        g_free(holes->coords);
    }
}


static inline void displayitem_draw_polygon (struct display_context * dc, struct graphics * gra, struct point * pa,
        int count, struct displayitem_poly_holes * holes) {
    if((holes != NULL) && (holes->count > 0))
        graphics_draw_polygon_with_holes_clipped(gra, dc->gc, pa, count, holes->count, holes->ccount,
                (struct point **)holes->coords);
    else
        graphics_draw_polygon_clipped(gra, dc->gc, pa, count);
}

static inline void displayitem_draw_polyline(struct display_context * dc, struct element * e, struct graphics * gra,
        struct point * pa, int count, int *width) {
    int i;
    graphics_gc_set_linewidth(dc->gc, 1);
    if (e->u.polyline.width > 0 && e->u.polyline.dash_num > 0)
        graphics_gc_set_dashes(dc->gc, e->u.polyline.width, e->u.polyline.offset, e->u.polyline.dash_table,
                               e->u.polyline.dash_num);
    for (i = 0 ; i < count ; i++) {
        if (width[i] < 2)
            width[i]=2;
    }
    graphics_draw_polyline_clipped(gra, dc->gc, pa, count, width, e->u.polyline.width > 1);
}

static inline void displayitem_draw_circle(struct displayitem *di,struct display_context *dc, struct element * e,
        struct graphics * gra, struct point * pa, int count) {
    if (count) {
        if (e->u.circle.width > 1)
            graphics_gc_set_linewidth(dc->gc, e->u.polyline.width);
        graphics_draw_circle(gra, dc->gc, pa, e->u.circle.radius);
        if (di->label && e->text_size) {
            struct graphics_font *font=get_font(gra, e->text_size);
            struct graphics_gc *gc_background=dc->gc_background;
            if (! gc_background && e->u.circle.background_color.a) {
                gc_background=graphics_gc_new(gra);
                graphics_gc_set_foreground(gc_background, &e->u.circle.background_color);
                dc->gc_background=gc_background;
            }
            if (font) {
                struct point p;
                /* Set p to the center of the circle */
                p.x=pa[0].x+(e->u.circle.radius/2);
                p.y=pa[0].y+(e->u.circle.radius/2);
                multiline_label_draw(gra, dc->gc, gc_background, font, p, di->label, e->text_size+1);
            } else
                dbg(lvl_error,"Failed to get font with size %d",e->text_size);
        }
    }
}

static inline void displayitem_draw_text(struct displayitem *di,struct display_context *dc, struct element * e,
        struct graphics * gra,  struct point * pa, int count,  struct displayitem_poly_holes * holes) {
    if (count && di->label) {
        struct graphics_font *font=get_font(gra, e->text_size);
        struct graphics_gc *gc_background=dc->gc_background;
        if (! gc_background && e->u.text.background_color.a) {
            gc_background=graphics_gc_new(gra);
            graphics_gc_set_foreground(gc_background, &e->u.text.background_color);
            dc->gc_background=gc_background;
        }
        if (font) {
            int a;
            label_line(gra, dc->gc, gc_background, font, pa, count, di->label);
            if(holes != NULL) {
                for(a = 0; a < holes->count; a ++)
                    label_line(gra, dc->gc, gc_background, font, (struct point *)holes->coords[a], holes->ccount[a], di->label);
            }
        } else
            dbg(lvl_error,"Failed to get font with size %d",e->text_size);
    }
}

static inline void displayitem_draw_icon(struct displayitem *di,struct display_context *dc, struct element * e,
        struct graphics * gra, struct point * pa, int count) {
    if (count) {
        struct graphics_image *img=dc->img;
        if (!img || item_is_custom_poi(di->item)) {
            char *path;
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
                dbg(lvl_debug,"failed to load icon '%s'", path);
            g_free(path);
        }
        if (img) {
            struct point p;
            if (e->u.icon.x != -1 || e->u.icon.y != -1) {
                p.x=pa[0].x - e->u.icon.x;
                p.y=pa[0].y - e->u.icon.y;
            } else {
                p.x=pa[0].x - img->hot.x;
                p.y=pa[0].y - img->hot.y;
            }
            graphics_draw_image(gra, gra->gc[0], &p, img);
        }
    }
}

static inline void displayitem_draw_image (struct displayitem *di, struct display_context *dc, struct graphics * gra,
        struct point * pa, int count) {
    dbg(lvl_debug,"image: '%s'", di->label);
    struct graphics_image *img=dc->img;
    if (gra->meth.draw_image_warp) {
        img=graphics_image_new_scaled_rotated(gra, di->label, IMAGE_W_H_UNSET, IMAGE_W_H_UNSET, 0);
        if (img)
            graphics_draw_image_warp(gra, gra->gc[0], pa, count, img);
    } else
        dbg(lvl_error,"draw_image_warp not supported by graphics driver drawing '%s'", di->label);
}

/**
 * @brief Draw a displayitem element
 *
 * This function will invoke the appropriate draw primitive depending on the type of the element to draw
 *
 * @brief di The displayitem to draw
 * @brief dummy Unused
 * @brief dc The display_context to use to draw items
 */
static void displayitem_draw(struct displayitem *di, void *dummy, struct display_context *dc) {
    int *width=g_alloca(sizeof(int)*dc->maxlen);
    int limit=0;
    struct point *pa=g_alloca(sizeof(struct point)*dc->maxlen);
    struct graphics *gra=dc->gra;
    struct element *e=dc->e;

    while (di) {
        int count=di->count,mindist=dc->mindist;
        struct displayitem_poly_holes t_holes;
        t_holes.count=0;

        di->z_order=++(gra->current_z_order);

        if (! dc->gc) {
            struct graphics_gc * gc=graphics_gc_new(gra);
            graphics_gc_set_foreground(gc, &e->color);
            dc->gc=gc;
        }

        if (item_type_is_area(dc->type) && (dc->e->type == element_polyline || dc->e->type == element_text))
            limit = 0;

        displayitem_transform_holes(dc->trans, dc->pro, di->holes, &t_holes, mindist);

        if (limit)
            count=limit_count(di->c, count);
        if (dc->type == type_poly_water_tiled)
            mindist=0;
        if (dc->e->type == element_polyline)
            count=transform(dc->trans, dc->pro, di->c, pa, count, mindist, e->u.polyline.width, width);
        else
            count=transform(dc->trans, dc->pro, di->c, pa, count, mindist, 0, NULL);
        switch (e->type) {
        case element_polygon:
            displayitem_draw_polygon(dc, gra, pa, count, &t_holes);
            break;
        case element_polyline:
            displayitem_draw_polyline(dc, e, gra, pa, count, width);
            break;
        case element_circle:
            displayitem_draw_circle(di, dc, e, gra, pa, count);
            break;
        case element_text:
            displayitem_draw_text(di, dc, e, gra, pa,  count, &t_holes);
            break;
        case element_icon:
            displayitem_draw_icon(di, dc, e, gra, pa, count);
            break;
        case element_image:
            displayitem_draw_image (di, dc,  gra, pa, count);
            break;
        case element_arrows:
            display_draw_arrows(gra,dc->gc,pa,count);
            break;
        default:
            dbg(lvl_error, "Unhandled element type %d", e->type);

        }
        /* free space allocated for holes */
        displayitem_free_holes(&t_holes);

        di=di->next;
    }
}
/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
static void xdisplay_draw_elements(struct graphics *gra, struct displaylist *display_list, struct itemgra *itm) {
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

void graphics_draw_itemgra(struct graphics *gra, struct itemgra *itm, struct transformation *t, char *label) {
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
    di->z_order=0;
    di->label=label;
    di->holes=NULL;
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
                dbg(lvl_error,"maximum number of coords reached: %d > %d",e->coord_count,max_coord);
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
static void xdisplay_draw_layer(struct displaylist *display_list, struct graphics *gra, struct layer *lay, int order) {
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
static void xdisplay_draw(struct displaylist *display_list, struct graphics *gra, struct layout *l, int order) {
    GList *lays;
    struct layer *lay;

    gra->current_z_order=0;
    lays=l->layers;
    while (lays) {
        lay=lays->data;
        if (lay->active) {
            if (lay->ref)
                lay=lay->ref;
            xdisplay_draw_layer(display_list, gra, lay, order);
        }
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

static void displaylist_update_layers(struct displaylist *displaylist, GList *layers, int order) {
    while (layers) {
        struct layer *layer=layers->data;
        GList *itemgras;
        if (layer->ref)
            layer=layer->ref;
        itemgras=layer->itemgras;
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

static void displaylist_update_hash(struct displaylist *displaylist) {
    displaylist->max_offset=0;
    clear_hash(displaylist);
    displaylist_update_layers(displaylist, displaylist->layout->layers, displaylist->order);
    dbg(lvl_debug,"max offset %d",displaylist->max_offset);
}


/**
 * @brief Returns selection structure based on displaylist transform, projection and order.
 * Use this function to get map selection if you are going to fetch complete item data from the map based on displayitem reference.
 * @param displaylist
 * @returns Pointer to selection structure
 */
struct map_selection *displaylist_get_selection(struct displaylist *displaylist) {
    return transform_get_selection(displaylist->dc.trans, displaylist->dc.pro, displaylist->order);
}

/**
 * @brief Compare displayitems based on their zorder values.
 * Use with g_list_insert_sorted to sort less shaded items to be before more shaded ones in the result list.
 */
static int displaylist_cmp_zorder(const struct displayitem *a, const struct displayitem *b) {
    if(a->z_order>b->z_order)
        return -1;
    if(a->z_order<b->z_order)
        return 1;
    return 0;
}

/**
 * @brief Returns list of displayitems clicked at given coordinates. The deeper item is in current layout, the deeper it will be in the list.
 * @param displaylist
 * @param p clicked point
 * @param radius radius of clicked area
 * @returns GList of displayitems
 */
GList *displaylist_get_clicked_list(struct displaylist *displaylist, struct point *p, int radius) {
    GList *l=NULL;
    struct displayitem *di;
    struct displaylist_handle *dlh=graphics_displaylist_open(displaylist);

    while ((di=graphics_displaylist_next(dlh))) {
        if (di->z_order>0 && graphics_displayitem_within_dist(displaylist, di, p,radius))
            l=g_list_insert_sorted(l,(gpointer) di, (GCompareFunc) displaylist_cmp_zorder);
    }

    graphics_displaylist_close(dlh);

    return l;
}



static void do_draw(struct displaylist *displaylist, int cancel, int flags) {
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
                displaylist->sel=displaylist_get_selection(displaylist);
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
                    dbg(lvl_error,"point count overflow %d for %s "ITEM_ID_FMT"", count,item_to_name(item->type),ITEM_ID_ARGS(*item));
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
void graphics_displaylist_draw(struct graphics *gra, struct displaylist *displaylist, struct transformation *trans,
                               struct layout *l, int flags) {
    int order=transform_get_order(trans);
    if(displaylist->dc.trans && displaylist->dc.trans!=trans)
        transform_destroy(displaylist->dc.trans);
    if(displaylist->dc.trans!=trans)
        displaylist->dc.trans=transform_dup(trans);
    displaylist->dc.gra=gra;
    displaylist->dc.mindist=flags&512?15:2;
    // FIXME find a better place to set the background color
    if (l) {
        graphics_gc_set_background(gra->gc[0], &l->color);
        graphics_gc_set_foreground(gra->gc[0], &l->color);
        g_free(gra->default_font);
        gra->default_font = g_strdup(l->font);
    }
    graphics_background_gc(gra, gra->gc[0]);
    if (flags & 1)
        callback_list_call_attr_0(gra->cbl, attr_predraw);
    graphics_draw_mode(gra, (flags & 8)?draw_mode_begin_clear:draw_mode_begin);
    if (!(flags & 2))
        graphics_draw_rectangle(gra, gra->gc[0], &gra->r.lu, gra->r.rl.x-gra->r.lu.x, gra->r.rl.y-gra->r.lu.y);
    if (l)	{
        order+=l->order_delta;
        xdisplay_draw(displaylist, gra, l, order>0?order:0);
    }
    if (flags & 1)
        callback_list_call_attr_0(gra->cbl, attr_postdraw);
    if (!(flags & 4))
        graphics_draw_mode(gra, draw_mode_end);
}

static void graphics_load_mapset(struct graphics *gra, struct displaylist *displaylist, struct mapset *mapset,
                                 struct transformation *trans, struct layout *l, int async, struct callback *cb, int flags) {
    int order=transform_get_order(trans);

    dbg(lvl_debug,"enter");
    if (displaylist->busy) {
        if (async == 1)
            return;
        do_draw(displaylist, 1, flags);
    }
    xdisplay_free(displaylist);
    dbg(lvl_debug,"order=%d", order);

    displaylist->dc.gra=gra;
    displaylist->ms=mapset;
    if(displaylist->dc.trans && displaylist->dc.trans!=trans)
        transform_destroy(displaylist->dc.trans);
    if(displaylist->dc.trans!=trans)
        displaylist->dc.trans=transform_dup(trans);
    displaylist->workload=async ? 100 : 0;
    displaylist->cb=cb;
    displaylist->seq++;
    if (l)
        order+=l->order_delta;
    displaylist->order=order>0?order:0;
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
void graphics_draw(struct graphics *gra, struct displaylist *displaylist, struct mapset *mapset,
                   struct transformation *trans, struct layout *l, int async, struct callback *cb, int flags) {
    graphics_load_mapset(gra, displaylist, mapset, trans, l, async, cb, flags);
}

int graphics_draw_cancel(struct graphics *gra, struct displaylist *displaylist) {
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
struct displaylist_handle * graphics_displaylist_open(struct displaylist *displaylist) {
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
struct displayitem * graphics_displaylist_next(struct displaylist_handle *dlh) {
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
void graphics_displaylist_close(struct displaylist_handle *dlh) {
    g_free(dlh);
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
struct displaylist * graphics_displaylist_new(void) {
    struct displaylist *ret=g_new0(struct displaylist, 1);

    ret->dc.maxlen=16384;

    return ret;
}

void graphics_displaylist_destroy(struct displaylist *displaylist) {
    if(displaylist->dc.trans)
        transform_destroy(displaylist->dc.trans);
    g_free(displaylist);

}


/**
 * Get the map item which given displayitem is based on.
 * NOTE: returned structure doesn't contain any attributes or coordinates. type, map, idhi and idlow seem to be the only useable members.
 * @param di pointer to displayitem structure
 * @returns Pointer to struct item
 * @author Martin Schaller (04/2008)
*/
struct item * graphics_displayitem_get_item(struct displayitem *di) {
    return &di->item;
}

/**
 * Get the number of this item as it was last displayed on the screen, dependent of current layout. Items with lower numbers
 * are shaded by items with higher ones when they overlap. Zero means item was not displayed at all. If the item is displayed twice, its topmost
 * occurence is used.
 * @param di pointer to displayitem structure
 * @returns z-order of current item.
*/
int graphics_displayitem_get_z_order(struct displayitem *di) {
    return di->z_order;
}


int graphics_displayitem_get_coord_count(struct displayitem *di) {
    return di->count;
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
char * graphics_displayitem_get_label(struct displayitem *di) {
    return di->label;
}

int graphics_displayitem_get_displayed(struct displayitem *di) {
    return 1;
}

/**
 * FIXME
 * @param <>
 * @returns <>
 * @author Martin Schaller (04/2008)
*/
static int within_dist_point(struct point *p0, struct point *p1, int dist) {
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
static int within_dist_line(struct point *p, struct point *line_p0, struct point *line_p1, int dist) {
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
static int within_dist_polyline(struct point *p, struct point *line_pnt, int count, int dist, int close) {
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
static int within_dist_polygon(struct point *p, struct point *poly_pnt, int count, int dist) {
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
int graphics_displayitem_within_dist(struct displaylist *displaylist, struct displayitem *di, struct point *p,
                                     int dist) {
    struct point *pa=g_alloca(sizeof(struct point)*displaylist->dc.maxlen);
    int count;

    count=transform(displaylist->dc.trans, displaylist->dc.pro, di->c, pa, di->count, 0, 0, NULL);

    if (di->item.type < type_line) {
        return within_dist_point(p, &pa[0], dist);
    }
    if (di->item.type < type_area) {
        return within_dist_polyline(p, pa, count, dist, 0);
    }
    return within_dist_polygon(p, pa, count, dist);
}


static void graphics_process_selection_item(struct displaylist *dl, struct item *item) {
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

void graphics_add_selection(struct graphics *gra, struct item *item, enum item_type type, struct displaylist *dl) {
    struct item *item_dup=g_new(struct item, 1);
    *item_dup=*item;
    item_dup->priv_data=(void *)type;
    gra->selection=g_list_append(gra->selection, item_dup);
    if (dl)
        graphics_process_selection_item(dl, item_dup);
}

void graphics_remove_selection(struct graphics *gra, struct item *item, enum item_type type, struct displaylist *dl) {
    GList *curr;
    int found;

    for (;;) {
        curr=gra->selection;
        found=0;
        while (curr) {
            struct item *sitem=curr->data;
            if (item_is_equal(*item,*sitem)) {
#if 0 /* FIXME */
                if (dl) {
                    struct displayitem di;
                    GHashTable *h;
                    di.item=*sitem;
                    di.label=NULL;
                    di.count=0;
                    di.item.type=type;
                    h=g_hash_table_lookup(dl->dl, GINT_TO_POINTER(di.item.type));
                    if (h)
                        g_hash_table_remove(h, &di);
                }
#endif
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

void graphics_clear_selection(struct graphics *gra, struct displaylist *dl) {
    while (gra->selection) {
        struct item *item=(struct item *)gra->selection->data;
        graphics_remove_selection(gra, item, (enum item_type)item->priv_data,dl);
    }
}

static void graphics_process_selection(struct graphics *gra, struct displaylist *dl) {
    GList *curr;

    curr=gra->selection;
    while (curr) {
        struct item *item=curr->data;
        graphics_process_selection_item(dl, item);
        curr=g_list_next(curr);
    }
}

/**
 * @brief get display resolution in DPI
 * This method returns the native display density in DPI
 * @param gra graphics handle
 * @returns dpi value. May be fraction therefore double.
 */
navit_float graphics_get_dpi(struct graphics *gra) {
    if (!gra->meth.get_dpi)
        return 0;
    return gra->meth.get_dpi(gra->priv);
}
