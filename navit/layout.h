/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2009 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifndef NAVIT_LAYOUT_H
#define NAVIT_LAYOUT_H

#include "item.h"
#include "color.h"
#include "coord.h"

#ifdef __cplusplus
extern "C" {
#endif

struct poly_hole {
    long long osmid;
    int coord_count;
    struct coord coord[1];
};

struct element {
    enum { element_point, element_polyline, element_polygon, element_circle, element_text, element_icon, element_image, element_arrows } type;
    struct color color;
    int text_size;
    union {
        struct element_point {
            char stub;
        } point;
        struct element_polyline {
            int width;
            int directed;
            int dash_num;
            int offset;
            unsigned char dash_table[4];
        } polyline;
        struct element_polygon {
            char stub;
        } polygon;
        struct element_circle {
            int width;
            int radius;
            struct color background_color;
        } circle;
        struct element_icon {
            char *src;
            int width;
            int height;
            int rotation;
            int x;
            int y;
        } icon;
        struct element_text {
            struct color background_color;
        } text;
    } u;
    int coord_count;
    struct coord *coord;
};


struct itemgra {
    struct range order,sequence_range,speed_range,angle_range;
    GList *type;
    GList *elements;
};

struct layer {
    NAVIT_OBJECT
    struct navit *navit;
    char *name;
    int details;
    GList *itemgras;
    int active;
    struct layer *ref;
};

struct cursor {
    struct attr **attrs;
    struct range *sequence_range;
    char *name;
    int w,h;
    int interval;
};

struct layout {
    NAVIT_OBJECT
    struct navit *navit;
    char *name;
    char* dayname;
    char* nightname;
    char *font;
    struct color color;
    GList *layers;
    GList *cursors;
    int order_delta;
    int active;
};

/* prototypes */
enum attr_type;
struct arrows;
struct attr;
struct attr_iter;
struct circle;
struct cursor;
struct element;
struct icon;
struct image;
struct itemgra;
struct layer;
struct layout;
struct polygon;
struct polyline;
struct text;
struct layout *layout_new(struct attr *parent, struct attr **attrs);
struct attr_iter *layout_attr_iter_new(void);
void layout_attr_iter_destroy(struct attr_iter *iter);
int layout_get_attr(struct layout *layout, enum attr_type type, struct attr *attr, struct attr_iter *iter);
int layout_add_attr(struct layout *layout, struct attr *attr);
struct cursor *layout_get_cursor(struct layout *this_, char *name);
struct cursor *cursor_new(struct attr *parent, struct attr **attrs);
void cursor_destroy(struct cursor *this_);
int cursor_add_attr(struct cursor *this_, struct attr *attr);
struct layer *layer_new(struct attr *parent, struct attr **attrs);
int layer_get_attr(struct layer *layer, enum attr_type type, struct attr *attr, struct attr_iter *iter);
int layer_add_attr(struct layer *layer, struct attr *attr);
int layer_set_attr(struct layer *layer, struct attr *attr);
struct itemgra *itemgra_new(struct attr *parent, struct attr **attrs);
int itemgra_add_attr(struct itemgra *itemgra, struct attr *attr);
struct polygon *polygon_new(struct attr *parent, struct attr **attrs);
struct polyline *polyline_new(struct attr *parent, struct attr **attrs);
struct circle *circle_new(struct attr *parent, struct attr **attrs);
struct text *text_new(struct attr *parent, struct attr **attrs);
struct icon *icon_new(struct attr *parent, struct attr **attrs);
struct image *image_new(struct attr *parent, struct attr **attrs);
struct arrows *arrows_new(struct attr *parent, struct attr **attrs);
int element_add_attr(struct element *e, struct attr *attr);
/* end of prototypes */

#ifdef __cplusplus
}
#endif

#endif
