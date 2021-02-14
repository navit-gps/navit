/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
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

#ifndef NAVIT_TRANSFORM_H
#define NAVIT_TRANSFORM_H

#ifdef __cplusplus
extern "C" {
#endif
#include "coord.h"

/* prototypes */
enum attr_type;
enum item_type;
enum map_datum;
enum projection;
struct attr;
struct attr_iter;
struct coord;
struct coord_geo;
struct coord_geo_cart;
struct map_selection;
struct pcoord;
struct point;
struct transformation;
struct transformation *transform_new(struct pcoord *center, int scale, int yaw);
int transform_get_hog(struct transformation *this_);
void transform_set_hog(struct transformation *this_, int hog);
int transform_get_attr(struct transformation *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter);
int transform_set_attr(struct transformation *this_, struct attr *attr);
int transformation_get_order_base(struct transformation *this_);
void transform_set_order_base(struct transformation *this_, int order_base);
struct transformation *transform_dup(struct transformation *t);
void transform_to_geo(enum projection pro, const struct coord *c, struct coord_geo *g);
void transform_from_geo(enum projection pro, const struct coord_geo *g, struct coord *c);
void transform_from_to_count(struct coord *cfrom, enum projection from, struct coord *cto, enum projection to, int count);
void transform_from_to(struct coord *cfrom, enum projection from, struct coord *cto, enum projection to);
void transform_geo_to_cart(struct coord_geo *geo, navit_float a, navit_float b, struct coord_geo_cart *cart);
void transform_cart_to_geo(struct coord_geo_cart *cart, navit_float a, navit_float b, struct coord_geo *geo);
void transform_utm_to_geo(const double UTMEasting, const double UTMNorthing, int ZoneNumber, int NorthernHemisphere, struct coord_geo *geo);
void transform_datum(struct coord_geo *from, enum map_datum from_datum, struct coord_geo *to, enum map_datum to_datum);
int transform(struct transformation *t, enum projection pro, struct coord *c, struct point *p, int count, int mindist, int width, int *width_return);
int transform_reverse(struct transformation *t, struct point *p, struct coord *c);
double transform_pixels_to_map_distance(struct transformation *transformation, int pixels);
enum projection transform_get_projection(struct transformation *this_);
void transform_set_projection(struct transformation *this_, enum projection pro);
struct map_selection *transform_get_selection(struct transformation *this_, enum projection pro, int order);
struct coord *transform_center(struct transformation *this_);
struct coord *transform_get_center(struct transformation *this_);
void transform_set_center(struct transformation *this_, struct coord *c);
void transform_set_yaw(struct transformation *t, int yaw);
int transform_get_yaw(struct transformation *this_);
void transform_set_pitch(struct transformation *this_, int pitch);
int transform_get_pitch(struct transformation *this_);
void transform_set_roll(struct transformation *this_, int roll);
int transform_get_roll(struct transformation *this_);
void transform_set_distance(struct transformation *this_, int distance);
int transform_get_distance(struct transformation *this_);
void transform_set_scales(struct transformation *this_, int xscale, int yscale, int wscale);
void transform_set_screen_selection(struct transformation *t, struct map_selection *sel);
void transform_set_screen_center(struct transformation *t, struct point *p);
void transform_get_size(struct transformation *t, int *width, int *height);
void transform_setup(struct transformation *t, struct pcoord *c, int scale, int yaw);
void transform_setup_source_rect(struct transformation *t);
long transform_get_scale(struct transformation *t);
void transform_set_scale(struct transformation *t, long scale);
int transform_get_order(struct transformation *t);
double transform_scale(int y);
double transform_distance(enum projection pro, struct coord *c1, struct coord *c2);
void transform_project(enum projection pro, struct coord *c, int distance, int angle, struct coord *res);
double transform_polyline_length(enum projection pro, struct coord *c, int count);
int transform_distance_sq(struct coord *c1, struct coord *c2);
navit_float transform_distance_sq_float(struct coord *c1, struct coord *c2);
int transform_distance_sq_pc(struct pcoord *c1, struct pcoord *c2);
int transform_distance_line_sq(struct coord *l0, struct coord *l1, struct coord *ref, struct coord *lpnt);
navit_float transform_distance_line_sq_float(struct coord *l0, struct coord *l1, struct coord *ref, struct coord *lpnt);
int transform_distance_polyline_sq(struct coord *c, int count, struct coord *ref, struct coord *lpnt, int *pos);
int transform_douglas_peucker(struct coord *in, int count, int dist_sq, struct coord *out);
int transform_douglas_peucker_float(struct coord *in, int count, navit_float dist_sq, struct coord *out);
void transform_print_deg(double deg);
int transform_get_angle_delta(struct coord *c1, struct coord *c2, int dir);
int transform_within_border(struct transformation *this_, struct point *p, int border);
int transform_within_dist_point(struct coord *ref, struct coord *c, int dist);
int transform_within_dist_line(struct coord *ref, struct coord *c0, struct coord *c1, int dist);
int transform_within_dist_polyline(struct coord *ref, struct coord *c, int count, int close, int dist);
int transform_within_dist_polygon(struct coord *ref, struct coord *c, int count, int dist);
int transform_within_dist_item(struct coord *ref, enum item_type type, struct coord *c, int count, int dist);
void transform_copy(struct transformation *src, struct transformation *dst);
void transform_destroy(struct transformation *t);
/* end of prototypes */
#ifdef __cplusplus
}
#endif

#endif
