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

#ifndef NAVIT_MAPSET_H
#define NAVIT_MAPSET_H

#ifdef __cplusplus
extern "C" {
#endif

/* prototypes */
enum attr_type;
struct attr;
struct attr_iter;
struct item;
struct map;

/**
 * @brief A mapset.
 *
 * A mapset is a collection of (one or more) maps. This allows you to combine data from multiple maps, e.g. one map
 * with the road network and another with special POIs.
 */
struct mapset;

struct mapset_handle;
struct mapset_search;
struct mapset *mapset_new(struct attr *parent, struct attr **attrs);
struct mapset *mapset_dup(struct mapset *ms);
struct attr_iter *mapset_attr_iter_new(void * unused);
void mapset_attr_iter_destroy(struct attr_iter *iter);
int mapset_add_attr(struct mapset *ms, struct attr *attr);
int mapset_remove_attr(struct mapset *ms, struct attr *attr);
int mapset_get_attr(struct mapset *ms, enum attr_type type, struct attr *attr, struct attr_iter *iter);
void mapset_destroy(struct mapset *ms);
struct map *mapset_get_map_by_name(struct mapset *ms, const char*map_name);
struct mapset_handle *mapset_open(struct mapset *ms);
struct map *mapset_next(struct mapset_handle *msh, int active);
void mapset_close(struct mapset_handle *msh);
struct mapset_search *mapset_search_new(struct mapset *ms, struct item *item, struct attr *search_attr, int partial);
struct item *mapset_search_get_item(struct mapset_search *this_);
void mapset_search_destroy(struct mapset_search *this_);
struct mapset * mapset_ref(struct mapset* m);
void mapset_unref(struct mapset *m);
/* end of prototypes */
#ifdef __cplusplus
}
#endif
#endif
