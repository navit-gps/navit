/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2011 Navit Team
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
#ifndef NAVIT_GEOM_H
#define NAVIT_GEOM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <glib.h>
#include "config.h"
#include "coord.h"
#include "item.h"
#include "attr.h"


#define sq(x) ((double)(x)*(x))

struct rect {
    struct coord l,h;
};

enum geom_poly_segment_type {
    geom_poly_segment_type_none,
    geom_poly_segment_type_way_inner,
    geom_poly_segment_type_way_outer,
    geom_poly_segment_type_way_left_side,
    geom_poly_segment_type_way_right_side,
    geom_poly_segment_type_way_unknown,

};

struct geom_poly_segment {
    enum geom_poly_segment_type type;
    struct coord *first,*last;
};
/* prototypes */
void geom_coord_copy(struct coord *from, struct coord *to, int count, int reverse);
void geom_coord_revert(struct coord *c, int count);
int geom_line_middle(struct coord *p, int count, struct coord *c);
long long geom_poly_area(struct coord *c, int count);
int geom_poly_centroid(struct coord *c, int count, struct coord *r);
int geom_poly_point_inside(struct coord *cp, int count, struct coord *c);
int geom_poly_closest_point(struct coord *pl, int count, struct coord *p, struct coord *c);
GList *geom_poly_segments_insert(GList *list, struct geom_poly_segment *first, struct geom_poly_segment *second,
                                 struct geom_poly_segment *third);
void geom_poly_segment_destroy(struct geom_poly_segment *seg, void * unused);
GList *geom_poly_segments_remove(GList *list, struct geom_poly_segment *seg);
int geom_poly_segment_compatible(struct geom_poly_segment *s1, struct geom_poly_segment *s2, int dir);
GList *geom_poly_segments_sort(GList *in, enum geom_poly_segment_type type);
int geom_poly_segments_point_inside(GList *in, struct coord *c);
int geom_clip_line_code(struct coord *p1, struct coord *p2, struct rect *r);
int geom_is_inside(struct coord *p, struct rect *r, int edge);
void geom_poly_intersection(struct coord *p1, struct coord *p2, struct rect *r, int edge, struct coord *ret);
void geom_init(void);
/* end of prototypes */
#ifdef __cplusplus
}
#endif

#endif

