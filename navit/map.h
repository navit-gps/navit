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

/** @file
 *
 * @brief Contains exported functions / structures for map.c
 *
 * This file contains code that works together with map.c and that is exported
 * to other modules.
 */

#ifndef NAVIT_MAP_H
#define NAVIT_MAP_H

#ifdef __cplusplus
extern "C" {
#endif

struct map_priv;
struct attr;
#include "coord.h"
#include "point.h"
#include "layer.h"
#include "debug.h"

#define WORLD_BOUNDINGBOX_MIN_X -20000000
#define WORLD_BOUNDINGBOX_MAX_X  20000000
#define WORLD_BOUNDINGBOX_MIN_Y -20000000
#define WORLD_BOUNDINGBOX_MAX_Y  20000000

/**
 * @brief Used to select data from a map
 *
 * A map selection is a map query, used to select data from a map. It builds a coordinate rectangle on the map and
 * selects a range of item types to query the map for.
 *
 * Multiple rectangular areas and/or non-contiguous ranges of item types can be specified by concatenating multiple map
 * selections in a linked list.
 *
 * Note that passing NULL instead of a pointer to such a struct often means "get me everything".
 */
struct map_selection {
	struct map_selection *next;     /**< Linked-List pointer */
	union {
		struct coord_rect c_rect;   /**< For building the rectangle based on coordinates */
		struct point_rect p_rect;   /**< For building the rectangle based on points */
	} u;
	int order;		    	/**< Holds the order */
	struct item_range range;	/**< Range of items which should be delivered */
};

/**
 * @brief Holds all functions a map plugin has to implement to be usable
 *
 * This structure holds pointers to a map plugin's functions, which Navit's core will call
 * to communicate with the plugin. For further information look into map.c, which has
 * functions with the same names acting more or less as "wrappers" around the functions here.
 * Especially the arguments (and their meaning) of each function will be described there.
 *
 * Note that there is no separate method to remove an item from a map. This is instead accomplished by
 * retrieving the item, then setting its type to `type_none` via its `item_type_set` method (provided
 * the item and its map support removal).
 */
struct map_methods {
	enum projection pro;        /**< The projection used for that type of map */
	char *charset;              /**< The charset this map uses - e.g. "iso8859-1" or "utf-8". Please specify this in a form so that g_convert() can handle it. */
	void 			(*map_destroy)(struct map_priv *priv);  /**< Function used to destroy ("close") a map. */
	struct map_rect_priv *  (*map_rect_new)(struct map_priv *map, struct map_selection *sel); /**< Function to create a new map rect on the map. */
	void			(*map_rect_destroy)(struct map_rect_priv *mr); /**< Function to destroy a map rect */
	struct item *		(*map_rect_get_item)(struct map_rect_priv *mr); /**< Function to return the next item from a map rect */
	struct item *		(*map_rect_get_item_byid)(struct map_rect_priv *mr, int id_hi, int id_lo); /**< Function to get an item with a specific ID from a map rect, can be NULL */
	struct map_search_priv *(*map_search_new)(struct map_priv *map, struct item *item, struct attr *search, int partial); /**< Function to start a new search on the map, can be NULL */
	void			(*map_search_destroy)(struct map_search_priv *ms); /**< Function to destroy a map search struct, ignored if `map_search_new` is NULL */
	struct item *		(*map_search_get_item)(struct map_search_priv *ms); /**< Function to get the next item of a search on the map, ignored if `map_search_new` is NULL */
	struct item *		(*map_rect_create_item)(struct map_rect_priv *mr, enum item_type type); /**< Function to create a new item in the map */
	int			(*map_get_attr)(struct map_priv *priv, enum attr_type type, struct attr *attr); /**< Function to get a map attribute, can be NULL */
    int			(*map_set_attr)(struct map_priv *priv, struct attr *attr); /**< Function to set a map attribute, can be NULL */
};

/**
 * @brief Checks if a coordinate is within a map selection
 *
 * Checks if a coordinate is within a map selection. Note that since a selection of NULL
 * means "select everything", with sel = NULL this will always return true. If there are
 * more than one selection in a linked-list, it is sufficient if only one of the selections
 * contains the coordinate.
 *
 * @param sel The selection to check if the point is within
 * @param c Coordinate to check if it is within the selection
 * @return True if the coordinate is within one of the selections, False otherwise
 */
static inline int
map_selection_contains_point(struct map_selection *sel, struct coord *c)
{
	struct map_selection *curr=sel;
        while (curr) {
                struct coord_rect *r=&curr->u.c_rect;
                if (c->x >= r->lu.x && c->x <= r->rl.x &&
                    c->y <= r->lu.y && c->y >= r->rl.y)
                        return 1;
                curr=curr->next;
        }
        return sel ? 0:1;
}

/**
 * @brief Checks if a polyline is within a map selection
 *
 * @sa Please refer to map_selection_contains_point()
 *
 * @param sel The selection to check if the polyline is within
 * @param c Coordinates of the polyline to check if it is within the selection
 * @param count Number of coordinates in c
 * @return True if the polyline is within one of the selections, False otherwise
 */
static inline int
map_selection_contains_polyline(struct map_selection *sel, struct coord *c, int count)
{
	int i,x_mi,x_ma,y_mi,y_ma;
	struct map_selection *curr;

	if (! sel)
		return 1;
	for (i = 0 ; i < count-1 ; i++) {
		x_mi=c[i].x;
		if (c[i+1].x < x_mi)
			x_mi=c[i+1].x;
		x_ma=c[i].x;
		if (c[i+1].x > x_ma)
			x_ma=c[i+1].x;
		y_mi=c[i].y;
		if (c[i+1].y < y_mi)
			y_mi=c[i+1].y;
		y_ma=c[i].y;
		if (c[i+1].y > y_ma)
			y_ma=c[i+1].y;
		curr=sel;
		while (curr) {
			struct coord_rect *sr=&curr->u.c_rect;
			if (x_mi <= sr->rl.x && x_ma >= sr->lu.x &&
			    y_ma >= sr->rl.y && y_mi <= sr->lu.y)
				return 1;
			curr=curr->next;
		}
	}
	return 0;
}

/**
 * @brief Checks if a rectangle is within a map selection
 *
 * @sa Please refer to map_selection_contains_point()
 *
 * @param sel The selection to check if the rectangle is within
 * @param r Rectangle to be checked for
 * @return True if the rectangle is within one of the selections, False otherwise
 */
static inline int
map_selection_contains_rect(struct map_selection *sel, struct coord_rect *r)
{
	struct map_selection *curr;

	dbg_assert(r->lu.x <= r->rl.x);
	dbg_assert(r->lu.y >= r->rl.y);

	if (! sel)
		return 1;
	curr=sel;
	while (curr) {
		struct coord_rect *sr=&curr->u.c_rect;
		dbg_assert(sr->lu.x <= sr->rl.x);
		dbg_assert(sr->lu.y >= sr->rl.y);
		if (r->lu.x <= sr->rl.x && r->rl.x >= sr->lu.x &&
		    r->lu.y >= sr->rl.y && r->rl.y <= sr->lu.y)
			return 1;
		curr=curr->next;
	}
	return 0;
}


/**
 * @brief Checks if a polygon is within a map selection
 *
 * @sa Please refer to map_selection_contains_point()
 *
 * @param sel The selection to check if the polygon  is within
 * @param c Pointer to coordinates of the polygon
 * @param count Number of coordinates in c
 * @return True if the polygon is within one of the selections, False otherwise
 */
static inline int
map_selection_contains_polygon(struct map_selection *sel, struct coord *c, int count)
{
	struct coord_rect r;
	int i;

	if (! sel)
		return 1;
	if (! count)
		return 0;
	r.lu=c[0];
	r.rl=c[0];
	for (i = 1 ; i < count ; i++) {
		if (c[i].x < r.lu.x)
			r.lu.x=c[i].x;
		if (c[i].x > r.rl.x)
			r.rl.x=c[i].x;
		if (c[i].y < r.rl.y)
			r.rl.y=c[i].y;
		if (c[i].y > r.lu.y)
			r.lu.y=c[i].y;
	}
	return map_selection_contains_rect(sel, &r);
}

/* prototypes */
enum attr_type;
enum projection;
struct attr;
struct attr_iter;
struct callback;
struct item;

/**
 * @brief Holds information about a map
 *
 * A map holds data used for screen display, search and routing. Maps can come in different forms, such as stored in
 * files or created on the fly in memory. Flat-file maps typically hold the data commonly found in a road map, such as
 * roads, land uses, buildings and POIs. In-memory maps are used internally for data which changes at runtime, such as
 * information on the currently active route or traffic reports.
 *
 * To read data from a map (or add data if the map driver supports it), you need to obtain a map rectangle by calling
 * the map’s `map_rect_new` method. By passing a map selection it is possible to restrict the items retrieved from the
 * map to a certain region or to certain item types.
 */
struct map;

/**
 * @brief Implementation-specific map data.
 *
 * This struct is defined individually by each map driver and not ment to be accessed outside the map driver.
 */
struct map_priv;

/**
 * @brief Describes an extract of a map
 *
 * A map rectangle is the result of a map query. It can be obtained from the map by calling `map_rect_new` and passing
 * a map selection. The resulting map rectangle can be queried for items. Contrary to its name, a map rectangle does
 * not necessarily correspond to a rectangle: depending on the map selection it was created from, it can hold data from
 * multiple rectangular areas (which may or may not overlap) or from the entire map.
 *
 * Map rectangles are not guaranteed to be thread-safe, i.e. a map rectangle should never be accessed by more than one
 * thread without proper external synchronization. It is recommended that each thread obtain a separate map rectangle.
 *
 * Map implementations must ensure, however, that accesses to different map rectangles of the same map by different
 * threads are properly synchronized. Most importantly, they must ensure that threads reading from one map rectangle
 * receive consistent data while another thread is writing to another (which may also happen when reloading data from a
 * stored map).
 */
struct map_rect;

/**
 * @brief Holds information about a search on a map
 *
 * This structure holds information about a search performed on a map. This can be
 * used as "handle" to retrieve items from a search.
 */
struct map_search;

struct map_selection;
struct pcoord;
struct map *map_new(struct attr *parent, struct attr **attrs);
struct map *map_ref(struct map* m);
void map_unref(struct map* m);
int map_get_attr(struct map *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter);
int map_set_attr(struct map *this_, struct attr *attr);
void map_add_callback(struct map *this_, struct callback *cb);
void map_remove_callback(struct map *this_, struct callback *cb);
int map_requires_conversion(struct map *this_);
char *map_convert_string_tmp(struct map *this_, char *str);
char *map_convert_string(struct map *this_, char *str);
char *map_convert_dup(char *str);
void map_convert_free(char *str);
enum projection map_projection(struct map *this_);
void map_set_projection(struct map *this_, enum projection pro);
void map_destroy(struct map *m);
struct map_rect *map_rect_new(struct map *m, struct map_selection *sel);
struct item *map_rect_get_item(struct map_rect *mr);
struct item *map_rect_get_item_byid(struct map_rect *mr, int id_hi, int id_lo);
struct item *map_rect_create_item(struct map_rect *mr, enum item_type type_);
void map_rect_destroy(struct map_rect *mr);
struct map_search *map_search_new(struct map *m, struct item *item, struct attr *search_attr, int partial);
struct item *map_search_get_item(struct map_search *this_);
void map_search_destroy(struct map_search *this_);
struct map_selection *map_selection_rect_new(struct pcoord *center, int distance, int order);
struct map_selection *map_selection_dup_pro(struct map_selection *sel, enum projection from, enum projection to);
struct map_selection *map_selection_dup(struct map_selection *sel);
void map_selection_destroy(struct map_selection *sel);
int map_selection_contains_item_rect(struct map_selection *sel, struct item *item);
int map_selection_contains_item_range(struct map_selection *sel, int follow, struct item_range *range, int count);
int map_selection_contains_item(struct map_selection *sel, int follow, enum item_type type);
int map_priv_is(struct map *map, struct map_priv *priv);
void map_dump_filedesc(struct map *map, FILE *out);
void map_dump_file(struct map *map, const char *file);
void map_dump(struct map *map);
void map_destroy_do(struct map *m);
struct maps * maps_new(struct attr *parent, struct attr **attrs);
/* end of prototypes */

#ifdef __cplusplus
}
#endif
#endif
