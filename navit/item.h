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

#ifndef NAVIT_ITEM_H
#define NAVIT_ITEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

enum item_type {
#define ITEM2(x,y) type_##y=x,
#define ITEM(x) type_##x,
#include "item_def.h"
#undef ITEM2
#undef ITEM
};

#define route_item_first type_street_0
#define route_item_last type_street_parking_lane
extern int default_flags[];

#include "attr.h"


/* NOTE: we treat districts as towns for now, since
   a) navit does not implement district search yet
   b) OSM "place=suburb" maps to type_district in maptool. with the OSM USA maps,
      there are many "suburbs" that users will consider towns (not districts/counties);
      we want navit's town search to find them
*/
#define item_type_is_area(type) ((type) >= type_area)
#define item_is_town(item) ((item).type >= type_town_label && (item).type <= type_district_label_1e7)
#define item_is_district(item) ((item).type >= type_district_label && (item).type <= type_district_label_1e7)
#define item_is_poly_place(item) ((item).type >= type_poly_place1 && (item).type <= type_poly_place6)
#define item_is_point(item) ((item).type < type_line)
#define item_is_custom_poi(item) ((item).type >= type_poi_customg && (item).type < type_line)
#define item_is_street(item) (((item).type >= type_street_nopass && (item).type <= type_roundabout) \
                               ||  (item).type == type_street_service \
                               || ((item).type >= type_street_pedestrian && (item).type <= type_track_grass) \
                               ||  (item).type == type_living_street  \
                               ||  (item).type == type_street_construction  \
                               ||  (item).type == type_path \
                               ||  (item).type == type_street_parking_lane \
                               ||  (item).type == type_footway )

#define item_is_equal_id(a,b) ((a).id_hi == (b).id_hi && (a).id_lo == (b).id_lo)
#define item_is_equal(a,b) (item_is_equal_id(a,b) && (a).map == (b).map)

struct coord;

enum change_mode {
    change_mode_delete,
    change_mode_modify,
    change_mode_append,
    change_mode_prepend,
};

struct item_methods {
    void (*item_coord_rewind)(void *priv_data);
    int (*item_coord_get)(void *priv_data, struct coord *c, int count);
    void (*item_attr_rewind)(void *priv_data);
    int (*item_attr_get)(void *priv_data, enum attr_type attr_type, struct attr *attr);
    int (*item_coord_is_node)(void *priv_data);
    int (*item_attr_set)(void *priv_data, struct attr *attr, enum change_mode mode);
    int (*item_coord_set)(void *priv_data, struct coord *c, int count, enum change_mode mode);
    int (*item_type_set)(void *priv_data, enum item_type type);
    int (*item_coords_left)(void *priv_data);
};

struct item_id {
    int id_hi;
    int id_lo;
};

#define ITEM_ID_FMT "(0x%x,0x%x)"
#define ITEM_ID_ARGS(x) (x).id_hi,(x).id_lo

/**
 * Represents an object on a map, such as a POI, a building, a way or a boundary.
 */
struct item {
    enum item_type type; /**< Type of the item.*/
    int id_hi;  /**< First part of the ID of the item (item IDs have two parts).*/
    int id_lo; /**< Second part of the ID of the item.*/
    struct map *map; /**< The map this items belongs to.*/
    struct item_methods *meth; /**< Methods to manipulate this item.*/
    void *priv_data; /**< Private item data, only used by the map plugin which supplied this item.*/
};

extern struct item_range {
    enum item_type min,max;
} item_range_all;

extern struct item busy_item;

/* prototypes */
enum attr_type;
enum change_mode;
enum item_type;
enum projection;
struct attr;
struct coord;
struct item;
struct item_hash;
struct item_range;
struct map;
struct map_selection;
void item_create_hash(void);
void item_destroy_hash(void);
int *item_get_default_flags(enum item_type type);
void item_coord_rewind(struct item *it);
int item_coords_left(struct item * it);
int item_coord_get(struct item *it, struct coord *c, int count);
int item_coord_set(struct item *it, struct coord *c, int count, enum change_mode mode);
int item_coord_get_within_selection(struct item *it, struct coord *c, int count, struct map_selection *sel);
int item_coord_get_within_range(struct item *i, struct coord *c, int max, struct coord *start, struct coord *end);
int item_coord_get_pro(struct item *it, struct coord *c, int count, enum projection to);
int item_coord_is_node(struct item *it);
void item_attr_rewind(struct item *it);
int item_attr_get(struct item *it, enum attr_type attr_type, struct attr *attr);
int item_attr_set(struct item *it, struct attr *attr, enum change_mode mode);
int item_type_set(struct item *it, enum item_type type);
struct item *item_new(char *type, int zoom);
enum item_type item_from_name(const char *name);
char *item_to_name(enum item_type item);
unsigned int item_id_hash(const void *key);
int item_id_equal(const void *a, const void *b);
void item_id_from_ptr(struct item *item, void *id);
struct item_hash *item_hash_new(void);
void item_hash_insert(struct item_hash *h, struct item *item, void *val);
int item_hash_remove(struct item_hash *h, struct item *item);
void *item_hash_lookup(struct item_hash *h, struct item *item);
void item_hash_destroy(struct item_hash *h);
int item_range_intersects_range(struct item_range *range1, struct item_range *range2);
int item_range_contains_item(struct item_range *range, enum item_type type);
void item_dump_attr(struct item *item, struct map *map, FILE *out);
void item_dump_filedesc(struct item *item, struct map *map, FILE *out);
void item_cleanup(void);

/* end of prototypes */


#ifdef __cplusplus
}
/* __cplusplus */
#endif

/* NAVIT_ITEM_H */
#endif
