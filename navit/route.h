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
 * @brief Contains exported code for route.c
 *
 * This file contains code that works together with route.c and that is exported to
 * other modules.
 */

#ifndef NAVIT_ROUTE_H
#define NAVIT_ROUTE_H

#ifdef __cplusplus
extern "C" {
#endif
enum route_path_flags {
	route_path_flag_none=0,
	route_path_flag_cancel=1,
	route_path_flag_async=2,
	route_path_flag_no_rebuild=4,
};

enum route_status {
	route_status_no_destination=0,
	route_status_destination_set=1,
	route_status_not_found=1|2,
	route_status_building_path=1|4,
	route_status_building_graph=1|4|8,
	route_status_path_done_new=1|16,
	route_status_path_done_incremental=1|32,
};

struct route_crossing {
	long segid;
	int dir;
};

struct route_crossings {
	int count;
	struct route_crossing crossing[0];
};

/**
 * @brief Information about a street
 *
 * This contains information about a certain street
 */
struct street_data {
	struct item item;	/**< The map item for this street */
	int count;			/**< Number of coordinates this street has */
	int flags;
	int maxspeed;		/**< Maximum speed allowed on this street. */
	struct coord c[0];	/**< Pointer to the coordinates of this street.
						 *   DO NOT INSERT FIELDS AFTER THIS. */
};

/* prototypes */
enum attr_type;
enum projection;
struct attr;
struct attr_iter;
struct coord;
struct item;
struct map;
struct map_selection;
struct mapset;
struct pcoord;
struct route;
struct route_info;
struct street_data;
struct tracking;
struct vehicleprofile;
struct route *route_new(struct attr *parent, struct attr **attrs);
struct route *route_dup(struct route *orig);
void route_set_mapset(struct route *this_, struct mapset *ms);
void route_set_profile(struct route *this_, struct vehicleprofile *prof);
struct mapset *route_get_mapset(struct route *this_);
struct route_info *route_get_pos(struct route *this_);
struct route_info *route_get_dst(struct route *this_);
int route_get_path_set(struct route *this_);
int route_contains(struct route *this_, struct item *item);
int route_destination_reached(struct route *this_);
void route_set_position(struct route *this_, struct pcoord *pos);
void route_set_position_from_tracking(struct route *this_, struct tracking *tracking, enum projection pro);
struct map_selection *route_rect(int order, struct coord *c1, struct coord *c2, int rel, int abs);
void route_set_destinations(struct route *this_, struct pcoord *dst, int count, int async);
int route_get_destinations(struct route *this_, struct pcoord *pc, int count);
int route_get_destination_count(struct route *this_);
void route_get_distances(struct route *this_, struct coord *c, int count, int *distances);
void route_set_destination(struct route *this_, struct pcoord *dst, int async);
void route_append_destination(struct route *this_, struct pcoord *dst, int async);
void route_remove_nth_waypoint(struct route *this_, int n);
void route_remove_waypoint(struct route *this_);
char* route_get_destination_description(struct route *this_, int n);
struct coord route_get_coord_dist(struct route *this_, int dist);
struct street_data *street_get_data(struct item *item);
struct street_data *street_data_dup(struct street_data *orig);
void street_data_free(struct street_data *sd);
void route_info_free(struct route_info *inf);
struct street_data *route_info_street(struct route_info *rinf);
struct map *route_get_map(struct route *this_);
struct map *route_get_graph_map(struct route *this_);
enum route_path_flags route_get_flags(struct route *this_);
int route_has_graph(struct route *this_);
void route_set_projection(struct route *this_, enum projection pro);
void route_set_destinations(struct route *this_, struct pcoord *dst, int count, int async);
int route_set_attr(struct route *this_, struct attr *attr);
int route_add_attr(struct route *this_, struct attr *attr);
int route_remove_attr(struct route *this_, struct attr *attr);
struct attr_iter * route_attr_iter_new(void);
void route_attr_iter_destroy(struct attr_iter *iter);
int route_get_attr(struct route *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter);
void route_init(void);
void route_destroy(struct route *this_);
/* end of prototypes */
#ifdef __cplusplus
}
#endif

#endif

