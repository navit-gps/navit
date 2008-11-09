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
/* prototypes */
enum item_type;
enum map_datum;
enum projection;
struct coord;
struct coord_geo;
struct coord_geo_cart;
struct map_selection;
struct pcoord;
struct point;
struct transformation;
struct transformation *transform_new(void);
void transform_to_geo(enum projection pro, struct coord *c, struct coord_geo *g);
void transform_from_geo(enum projection pro, struct coord_geo *g, struct coord *c);
void transform_from_to(struct coord *cfrom, enum projection from, struct coord *cto, enum projection to);
void transform_geo_to_cart(struct coord_geo *geo, double a, double b, struct coord_geo_cart *cart);
void transform_cart_to_geo(struct coord_geo_cart *cart, double a, double b, struct coord_geo *geo);
void transform_datum(struct coord_geo *from, enum map_datum from_datum, struct coord_geo *to, enum map_datum to_datum);
int transform(struct transformation *t, enum projection pro, struct coord *c, struct point *p, int count, int unique);
void transform_reverse(struct transformation *t, struct point *p, struct coord *c);
enum projection transform_get_projection(struct transformation *this_);
void transform_set_projection(struct transformation *this_, enum projection pro);
struct map_selection *transform_get_selection(struct transformation *this_, enum projection pro, int order);
struct coord *transform_center(struct transformation *this_);
void transform_set_angle(struct transformation *t, int angle);
int transform_get_angle(struct transformation *this_, int angle);
void transform_set_screen_selection(struct transformation *t, struct map_selection *sel);
void transform_set_screen_center(struct transformation *t, struct point *p);
void transform_get_size(struct transformation *t, int *width, int *height);
void transform_setup(struct transformation *t, struct pcoord *c, int scale, int angle);
void transform_setup_source_rect(struct transformation *t);
long transform_get_scale(struct transformation *t);
void transform_set_scale(struct transformation *t, long scale);
int transform_get_order(struct transformation *t);
double transform_scale(int y);
double transform_distance(enum projection pro, struct coord *c1, struct coord *c2);
double transform_polyline_length(enum projection pro, struct coord *c, int count);
int transform_distance_sq(struct coord *c1, struct coord *c2);
int transform_distance_sq_pc(struct pcoord *c1, struct pcoord *c2);
int transform_distance_line_sq(struct coord *l0, struct coord *l1, struct coord *ref, struct coord *lpnt);
int transform_distance_polyline_sq(struct coord *c, int count, struct coord *ref, struct coord *lpnt, int *pos);
void transform_print_deg(double deg);
int transform_get_angle_delta(struct coord *c1, struct coord *c2, int dir);
int transform_within_border(struct transformation *this_, struct point *p, int border);
int transform_within_dist_point(struct coord *ref, struct coord *c, int dist);
int transform_within_dist_line(struct coord *ref, struct coord *c0, struct coord *c1, int dist);
int transform_within_dist_polyline(struct coord *ref, struct coord *c, int count, int close, int dist);
int transform_within_dist_polygon(struct coord *ref, struct coord *c, int count, int dist);
int transform_within_dist_item(struct coord *ref, enum item_type type, struct coord *c, int count, int dist);
void transform_destroy(struct transformation *t);
/* end of prototypes */
#ifdef __cplusplus
}
#endif

#endif
