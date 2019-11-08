/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2009 Navit Team
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
#include <string.h>
#include "item.h"
#include "attr.h"
#include "xmlconfig.h"
#include "layout.h"
#include "coord.h"
#include "debug.h"
#include "navit.h"

/**
 * @brief Create a new layout object and attach it to a navit parent
 *
 * @param parent The parent for this layout (a navit attr)
 * @param attrs An array of attributes that for this layout
 * @return The newly created layout object
 */
struct layout *
layout_new(struct attr *parent, struct attr **attrs) {
    struct layout *l;
    struct navit *navit;
    struct color def_color = {COLOR_BACKGROUND_};
    int def_underground_alpha = UNDERGROUND_ALPHA_;
    struct attr *name_attr,*color_attr,*order_delta_attr,*font_attr,*day_attr,*night_attr,*active_attr,
               *underground_alpha_attr,*icon_attr;

    if (! (name_attr=attr_search(attrs, NULL, attr_name)))
        return NULL;
    navit = parent->u.navit;
    if (navit_get_layout_by_name(navit, name_attr->u.str)) {
        dbg(lvl_warning, "Another layout with name '%s' has already been parsed. Discarding subsequent duplicate.",
            name_attr->u.str);
        return NULL;
    }

    l = g_new0(struct layout, 1);
    l->func=&layout_func;
    navit_object_ref((struct navit_object *)l);
    l->name = g_strdup(name_attr->u.str);
    if ((font_attr=attr_search(attrs, NULL, attr_font))) {
        l->font = g_strdup(font_attr->u.str);
    }
    if ((day_attr=attr_search(attrs, NULL, attr_daylayout))) {
        l->dayname = g_strdup(day_attr->u.str);
    }
    if ((night_attr=attr_search(attrs, NULL, attr_nightlayout))) {
        l->nightname = g_strdup(night_attr->u.str);
    }
    if ((color_attr=attr_search(attrs, NULL, attr_color)))
        l->color = *color_attr->u.color;
    else
        l->color = def_color;
    if ((underground_alpha_attr=attr_search(attrs, NULL, attr_underground_alpha))) {
        int a = underground_alpha_attr->u.num;
        /* for convenience, the alpha value is just 8 bit as usual if using
        * corresponding attr. therefore we need to shift that up */
        l->underground_alpha = (a << 8) | a;
    } else
        l->underground_alpha = def_underground_alpha;
    if ((icon_attr=attr_search(attrs, NULL, attr_icon_w)))
        l->icon_w = icon_attr->u.num;
    else
        l->icon_w = -1;
    if ((icon_attr=attr_search(attrs, NULL, attr_icon_h)))
        l->icon_h = icon_attr->u.num;
    else
        l->icon_h = -1;
    if ((order_delta_attr=attr_search(attrs, NULL, attr_order_delta)))
        l->order_delta=order_delta_attr->u.num;
    if ((active_attr=attr_search(attrs, NULL, attr_active)))
        l->active = active_attr->u.num;
    l->navit=navit;
    return l;
}

struct attr_iter {
    GList *last;
};


struct attr_iter *
layout_attr_iter_new(void* unused) {
    return g_new0(struct attr_iter, 1);
}

void layout_attr_iter_destroy(struct attr_iter *iter) {
    g_free(iter);
}

int layout_get_attr(struct layout *layout, enum attr_type type, struct attr *attr, struct attr_iter *iter) {
    GList *cursor,*layer;
    attr->type=type;
    switch (type) {
    case attr_name:
        attr->u.str=layout->name;
        return 1;
    case attr_cursor:
        cursor=layout->cursors;
        while (cursor) {
            if (!iter || iter->last == g_list_previous(cursor)) {
                attr->u.cursor=cursor->data;
                if (iter)
                    iter->last=cursor;
                return 1;
            }
            cursor=g_list_next(cursor);
        }
        break;
    case attr_layer:
        layer=layout->layers;
        while (layer) {
            if (!iter || iter->last == g_list_previous(layer)) {
                attr->u.layer=layer->data;
                if (iter)
                    iter->last=layer;
                return 1;
            }
            layer=g_list_next(layer);
        }
        break;
    case attr_active:
        attr->u.num=layout->active;
        return 1;
    case attr_nightlayout:
        attr->u.str=layout->nightname;
        return 1;
    case attr_daylayout:
        attr->u.str=layout->dayname;
        return 1;
    default:
        break;
    }
    return 0;
}


int layout_add_attr(struct layout *layout, struct attr *attr) {
    switch (attr->type) {
    case attr_cursor:
        layout->cursors = g_list_append(layout->cursors, attr->u.cursor);
        break;
    case attr_layer:
        layout->layers = g_list_append(layout->layers, attr->u.layer);
        break;
    default:
        return 0;
    }
    layout->attrs=attr_generic_add_attr(layout->attrs, attr);
    return 1;
}

/**
 * Searchs the layout for a cursor with the given name.
 *
 * @param layout The layout
 * @param name The name
 * @returns A pointer to cursor with the given name or the name default or NULL.
 * @author Ralph Sennhauser (10/2009)
*/
struct cursor *
layout_get_cursor(struct layout *this_, char *name) {
    GList *c;
    struct cursor *d=NULL;

    c=g_list_first(this_->cursors);
    while (c) {
        if (! strcmp(((struct cursor *)c->data)->name, name))
            return c->data;
        if (! strcmp(((struct cursor *)c->data)->name, "default"))
            d=c->data;
        c=g_list_next(c);
    }
    return d;
}



struct cursor *
cursor_new(struct attr *parent, struct attr **attrs) {
    struct attr *w, *h, *name, *interval, *sequence_range;
    struct cursor *this;

    w=attr_search(attrs, NULL, attr_w);
    h=attr_search(attrs, NULL, attr_h);
    if (! w || ! h)
        return NULL;

    this=g_new0(struct cursor,1);
    this->w=w->u.num;
    this->h=h->u.num;
    name=attr_search(attrs, NULL, attr_name);
    if (name)
        this->name=g_strdup(name->u.str);
    else
        this->name=g_strdup("default");
    interval=attr_search(attrs, NULL, attr_interval);
    if (interval)
        this->interval=interval->u.num;
    sequence_range=attr_search(attrs, NULL, attr_sequence_range);
    if (sequence_range) {
        struct range *r=g_new0(struct range,1);
        r->min=sequence_range->u.range.min;
        r->max=sequence_range->u.range.max;
        this->sequence_range=r;
    } else {
        this->sequence_range=NULL;
    }
    dbg(lvl_info,"ret=%p", this);
    return this;
}

void cursor_destroy(struct cursor *this_) {
    if (this_->sequence_range)
        g_free(this_->sequence_range);
    if (this_->name) {
        g_free(this_->name);
    }
    g_free(this_);
}

int cursor_add_attr(struct cursor *this_, struct attr *attr) {
    switch (attr->type) {
    case attr_itemgra:
        this_->attrs=attr_generic_add_attr(this_->attrs, attr);
        return 1;
    default:
        break;
    }
    return 0;
}

static int layer_set_attr_do(struct layer *l, struct attr *attr, int init) {
    struct attr_iter *iter;
    struct navit_object *obj;
    struct attr layer;
    switch (attr->type) {
    case attr_active:
        l->active = attr->u.num;
        return 1;
    case attr_details:
        l->details = attr->u.num;
        return 1;
    case attr_name:
        g_free(l->name);
        l->name = g_strdup(attr->u.str);
        return 1;
    case attr_ref:
        navit_object_unref((struct navit_object *)l->ref);
        l->ref=NULL;
        obj=(struct navit_object *)l->navit;
        if (obj==NULL) {
            dbg(lvl_error, "Invalid layer reference '%s': Only layers inside a layout can use references.", attr->u.str);
            return 0;
        }
        iter=obj->func->iter_new(obj);
        while (obj->func->get_attr(obj, attr_layer, &layer, iter)) {
            if (!strcmp(layer.u.layer->name, attr->u.str)) {
                l->ref=(struct layer*)navit_object_ref(layer.u.navit_object);
                break;
            }
        }
        if (l->ref==NULL) {
            dbg(lvl_error, "Ignoring reference to unknown layer '%s' in layer '%s'.", attr->u.str, l->name);
        }
        obj->func->iter_destroy(iter);
        return 0;
    default:
        return 0;
    }
}



struct layer * layer_new(struct attr *parent, struct attr **attrs) {
    struct layer *l;

    l = g_new0(struct layer, 1);
    if (parent->type == attr_layout)
        l->navit=parent->u.layout->navit;
    l->func=&layer_func;
    navit_object_ref((struct navit_object *)l);
    l->active=1;
    for (; *attrs; attrs++) {
        layer_set_attr_do(l, *attrs, 1);
    }
    if (l->name==NULL) {
        dbg(lvl_error, "Ignoring layer without name.");
        g_free(l);
        return NULL;
    }
    return l;
}

int layer_get_attr(struct layer *layer, enum attr_type type, struct attr *attr, struct attr_iter *iter) {
    attr->type=type;
    switch(type) {
    case attr_active:
        attr->u.num=layer->active;
        return 1;
    case attr_details:
        attr->u.num=layer->details;
        return 1;
    case attr_name:
        if (layer->name) {
            attr->u.str=layer->name;
            return 1;
        }
        break;
    default:
        return 0;
    }
    return 0;
}

int layer_add_attr(struct layer *layer, struct attr *attr) {
    switch (attr->type) {
    case attr_itemgra:
        layer->itemgras = g_list_append(layer->itemgras, attr->u.itemgra);
        return 1;
    default:
        return 0;
    }
}

int layer_set_attr(struct layer *layer, struct attr *attr) {
    return layer_set_attr_do(layer, attr, 0);
}

static void layer_destroy(struct layer *layer) {
    attr_list_free(layer->attrs);
    g_free(layer->name);
    g_free(layer);
}

struct itemgra * itemgra_new(struct attr *parent, struct attr **attrs) {
    struct itemgra *itm;
    struct attr *order, *item_types, *speed_range, *angle_range, *sequence_range;
    enum item_type *type;
    struct range defrange;

    itm = g_new0(struct itemgra, 1);
    order=attr_search(attrs, NULL, attr_order);
    item_types=attr_search(attrs, NULL, attr_item_types);
    speed_range=attr_search(attrs, NULL, attr_speed_range);
    angle_range=attr_search(attrs, NULL, attr_angle_range);
    sequence_range=attr_search(attrs, NULL, attr_sequence_range);
    defrange.min=0;
    defrange.max=32767;
    if (order)
        itm->order=order->u.range;
    else
        itm->order=defrange;
    if (speed_range)
        itm->speed_range=speed_range->u.range;
    else
        itm->speed_range=defrange;
    if (angle_range)
        itm->angle_range=angle_range->u.range;
    else
        itm->angle_range=defrange;
    if (sequence_range)
        itm->sequence_range=sequence_range->u.range;
    else
        itm->sequence_range=defrange;
    if (item_types) {
        type=item_types->u.item_types;
        while (type && *type != type_none) {
            itm->type=g_list_append(itm->type, GINT_TO_POINTER(*type));
            type++;
        }
    }
    return itm;
}
int itemgra_add_attr(struct itemgra *itemgra, struct attr *attr) {
    switch (attr->type) {
    case attr_polygon:
    case attr_polyline:
    case attr_circle:
    case attr_text:
    case attr_icon:
    case attr_image:
    case attr_arrows:
        itemgra->elements = g_list_append(itemgra->elements, attr->u.element);
        return 1;
    default:
        dbg(lvl_error,"unknown: %s", attr_to_name(attr->type));
        return 0;
    }
}

static void element_set_oneway(struct element *e, struct attr **attrs) {
    struct attr *oneway;
    oneway=attr_search(attrs, NULL, attr_oneway);
    if (oneway)
        e->oneway=oneway->u.num;
}

static void element_set_color(struct element *e, struct attr **attrs) {
    struct attr *color;
    color=attr_search(attrs, NULL, attr_color);
    if (color)
        e->color=*color->u.color;
}


static void element_set_background_color(struct color *c, struct attr **attrs) {
    struct attr *color;
    color=attr_search(attrs, NULL, attr_background_color);
    if (color)
        *c=*color->u.color;
}


static void element_set_text_size(struct element *e, struct attr **attrs) {
    struct attr *text_size;
    text_size=attr_search(attrs, NULL, attr_text_size);
    if (text_size)
        e->text_size=text_size->u.num;
}

static void element_set_arrows_width(struct element *e, struct attr **attrs) {
    struct attr *width;
    width=attr_search(attrs, NULL, attr_width);
    if (width)
        e->u.arrows.width=width->u.num;
    else
        e->u.arrows.width=10;
}

static void element_set_polyline_width(struct element *e, struct attr **attrs) {
    struct attr *width;
    width=attr_search(attrs, NULL, attr_width);
    if (width)
        e->u.polyline.width=width->u.num;
}

static void element_set_polyline_directed(struct element *e, struct attr **attrs) {
    struct attr *directed;
    directed=attr_search(attrs, NULL, attr_directed);
    if (directed)
        e->u.polyline.directed=directed->u.num;
}

static void element_set_polyline_dash(struct element *e, struct attr **attrs) {
    struct attr *dash;
    int i;

    dash=attr_search(attrs, NULL, attr_dash);
    if (dash) {
        for (i=0; i<4; i++) {
            if (!dash->u.dash[i])
                break;
            e->u.polyline.dash_table[i] = dash->u.dash[i];
        }
        e->u.polyline.dash_num=i;
    }
}

static void element_set_polyline_offset(struct element *e, struct attr **attrs) {
    struct attr *offset;
    offset=attr_search(attrs, NULL, attr_offset);
    if (offset)
        e->u.polyline.offset=offset->u.num;
}

static void element_set_circle_width(struct element *e, struct attr **attrs) {
    struct attr *width;
    width=attr_search(attrs, NULL, attr_width);
    if (width)
        e->u.circle.width=width->u.num;
}

static void element_set_circle_radius(struct element *e, struct attr **attrs) {
    struct attr *radius;
    radius=attr_search(attrs, NULL, attr_radius);
    if (radius)
        e->u.circle.radius=radius->u.num;
}

struct polygon *
polygon_new(struct attr *parent, struct attr **attrs) {
    struct element *e;
    e = g_new0(struct element, 1);
    e->type=element_polygon;
    element_set_color(e, attrs);
    element_set_oneway(e, attrs);

    return (struct polygon *)e;
}

struct polyline *
polyline_new(struct attr *parent, struct attr **attrs) {
    struct element *e;

    e = g_new0(struct element, 1);
    e->type=element_polyline;
    element_set_color(e, attrs);
    element_set_oneway(e, attrs);
    element_set_polyline_width(e, attrs);
    element_set_polyline_directed(e, attrs);
    element_set_polyline_dash(e, attrs);
    element_set_polyline_offset(e, attrs);
    return (struct polyline *)e;
}

struct circle *
circle_new(struct attr *parent, struct attr **attrs) {
    struct element *e;
    struct color color_black = {COLOR_BLACK_};
    struct color color_white = {COLOR_WHITE_};

    e = g_new0(struct element, 1);
    e->type=element_circle;
    e->color = color_black;
    e->u.circle.background_color = color_white;
    element_set_color(e, attrs);
    element_set_background_color(&e->u.circle.background_color, attrs);
    element_set_oneway(e, attrs);
    element_set_text_size(e, attrs);
    element_set_circle_width(e, attrs);
    element_set_circle_radius(e, attrs);

    return (struct circle *)e;
}

struct text *
text_new(struct attr *parent, struct attr **attrs) {
    struct element *e;
    struct color color_black = {COLOR_BLACK_};
    struct color color_white = {COLOR_WHITE_};

    e = g_new0(struct element, 1);
    e->type=element_text;
    element_set_text_size(e, attrs);
    e->color = color_black;
    e->u.text.background_color = color_white;
    element_set_color(e, attrs);
    element_set_background_color(&e->u.text.background_color, attrs);
    element_set_oneway(e, attrs);

    return (struct text *)e;
}

struct icon *
icon_new(struct attr *parent, struct attr **attrs) {
    struct element *e;
    struct attr *src,*w,*h,*rotation,*x,*y;
    src=attr_search(attrs, NULL, attr_src);
    if (! src)
        return NULL;

    e = g_malloc0(sizeof(*e)+strlen(src->u.str)+1);
    e->type=element_icon;
    e->u.icon.src=(char *)(e+1);
    if ((w=attr_search(attrs, NULL, attr_w)))
        e->u.icon.width=w->u.num;
    else
        e->u.icon.width=-1;
    if ((h=attr_search(attrs, NULL, attr_h)))
        e->u.icon.height=h->u.num;
    else
        e->u.icon.height=-1;
    if ((x=attr_search(attrs, NULL, attr_x)))
        e->u.icon.x=x->u.num;
    else
        e->u.icon.x=-1;
    if ((y=attr_search(attrs, NULL, attr_y)))
        e->u.icon.y=y->u.num;
    else
        e->u.icon.y=-1;
    if ((rotation=attr_search(attrs, NULL, attr_rotation)))
        e->u.icon.rotation=rotation->u.num;
    strcpy(e->u.icon.src,src->u.str);

    return (struct icon *)e;
}

struct image *
image_new(struct attr *parent, struct attr **attrs) {
    struct element *e;

    e = g_malloc0(sizeof(*e));
    e->type=element_image;

    return (struct image *)e;
}

struct arrows *
arrows_new(struct attr *parent, struct attr **attrs) {
    struct element *e;
    e = g_malloc0(sizeof(*e));
    e->type=element_arrows;
    element_set_color(e, attrs);
    element_set_oneway(e, attrs);
    element_set_arrows_width(e, attrs);
    return (struct arrows *)e;
}

int element_add_attr(struct element *e, struct attr *attr) {
    switch (attr->type) {
    case attr_coord:
        e->coord=g_realloc(e->coord,(e->coord_count+1)*sizeof(struct coord));
        e->coord[e->coord_count++]=*attr->u.coord;
        return 1;
    default:
        return 0;
    }
}

struct object_func layout_func = {
    attr_layout,
    (object_func_new)layout_new,
    (object_func_get_attr)layout_get_attr,
    (object_func_iter_new)layout_attr_iter_new,
    (object_func_iter_destroy)layout_attr_iter_destroy,
    (object_func_set_attr)NULL,
    (object_func_add_attr)layout_add_attr,
    (object_func_remove_attr)NULL,
    (object_func_init)NULL,
    (object_func_destroy)NULL,
    (object_func_dup)NULL,
    (object_func_ref)navit_object_ref,
    (object_func_unref)navit_object_unref,
};


struct object_func layer_func = {
    attr_layer,
    (object_func_new)layer_new,
    (object_func_get_attr)layer_get_attr,
    (object_func_iter_new)NULL,
    (object_func_iter_destroy)NULL,
    (object_func_set_attr)layer_set_attr,
    (object_func_add_attr)layer_add_attr,
    (object_func_remove_attr)NULL,
    (object_func_init)NULL,
    (object_func_destroy)layer_destroy,
    (object_func_dup)NULL,
    (object_func_ref)navit_object_ref,
    (object_func_unref)navit_object_unref,
};
