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

/** @file
 * 
 * @brief Contains code used for loading more than one map
 *
 * The code in this file introduces "mapsets", which are collections of several maps.
 * This enables navit to operate on more than one map at once. See map.c / map.h to learn
 * how maps are handled.
 */

#include <string.h>
#include <glib.h>
#include <glib/gprintf.h>
#include "debug.h"
#include "item.h"
#include "mapset.h"
#include "projection.h"
#include "map.h"

/**
 * @brief A mapset
 *
 * This structure holds a complete mapset
 */
struct mapset {
	GList *maps; /**< Linked list of all the maps in the mapset */
};

/**
 * @brief Creates a new, empty mapset
 *
 * @return The new mapset 
 */
struct mapset *mapset_new(struct attr *parent, struct attr **attrs)
{
	struct mapset *ms;

	ms=g_new0(struct mapset, 1);

	return ms;
}


/**
 * @brief Adds a map to a mapset
 *
 * @param ms The mapset to add the map to
 * @param m The map to be added
 */
int
mapset_add_attr(struct mapset *ms, struct attr *attr)
{
	switch (attr->type) {
	case attr_map:
		ms->maps=g_list_append(ms->maps, attr->u.map);
		return 1;
	default:
		return 0;
	}
}

#if 0
static void mapset_maps_free(struct mapset *ms)
{
	/* todo */
}
#endif

/**
 * @brief Destroys a mapset. 
 *
 * This destroys a mapset. Please note that it does not touch the contained maps
 * in any way.
 *
 * @param ms The mapset to be destroyed
 */
void mapset_destroy(struct mapset *ms)
{
	g_free(ms);
}

/**
 * @brief Handle for a mapset in use
 *
 * This struct is used for a mapset that is in use. With this it is possible to iterate
 * all maps in a mapset.
 */
struct mapset_handle {
	GList *l;	/**< Pointer to the current (next) map */
};

/**
 * @brief Returns a new handle for a mapset
 *
 * This returns a new handle for an existing mapset. The new handle points to the first
 * map in the set.
 *
 * @param ms The mapset to get a handle of
 * @return The new mapset handle
 */
struct mapset_handle *
mapset_open(struct mapset *ms)
{
	struct mapset_handle *ret;

	ret=g_new(struct mapset_handle, 1);
	ret->l=ms->maps;

	return ret;
}

/**
 * @brief Gets the next map from a mapset handle
 *
 * If you set active to true, this function will not return any maps that
 * have the attr_active attribute associated with them and set to false.
 *
 * @param msh The mapset handle to get the next map of
 * @param active Set to true to only get active maps (See description)
 * @return The next map
 */
struct map * mapset_next(struct mapset_handle *msh, int active)
{
	struct map *ret;
	struct attr active_attr;

	for (;;) {
		if (!msh->l)
			return NULL;
		ret=msh->l->data;
		msh->l=g_list_next(msh->l);
		if (!active)
			return ret;			
		if (!map_get_attr(ret, attr_active, &active_attr, NULL))
			return ret;
		if (active_attr.u.num)
			return ret;
	}
}

/**
 * @brief Closes a mapset handle after it is no longer used
 *
 * @param msh Mapset handle to be closed
 */
void 
mapset_close(struct mapset_handle *msh)
{
	g_free(msh);
}

/**
 * @brief Holds information about a search in a mapset
 *
 * This struct holds information about a search (e.g. for a street) in a mapset. 
 *
 * @sa For a more detailed description see the documentation of mapset_search_new().
 */
struct mapset_search {
	GList *map;					/**< The list of maps to be searched within */
	struct map_search *ms;		/**< A map search struct for the map currently active */
	struct item *item;			/**< "Superior" item. */
	struct attr *search_attr;	/**< Attribute to be searched for. */
	int partial;				/**< Indicates if one would like to have partial matches */
};

/**
 * @brief Starts a search on a mapset
 *
 * This function starts a search on a mapset. What attributes one can search for depends on the
 * map plugin. See the description of map_search_new() in map.c for details.
 *
 * If you enable partial matches bear in mind that the search matches only the begin of the
 * strings - a search for a street named "street" would match to "streetfoo", but not to
 * "somestreet". Search is case insensitive.
 *
 * The item passed to this function specifies a "superior item" to "search within" - e.g. a town 
 * in which we want to search for a street, or a country in which to search for a town.
 *
 * @param ms The mapset that should be searched
 * @param item Specifies a superior item to "search within" (see description)
 * @param search_attr Attribute specifying what to search for. See description.
 * @param partial Set this to true to also have partial matches. See description.
 * @return A new mapset search struct for this search
 */
struct mapset_search *
mapset_search_new(struct mapset *ms, struct item *item, struct attr *search_attr, int partial)
{
	struct mapset_search *this;
	dbg(1,"enter(%p,%p,%p,%d)\n", ms, item, search_attr, partial);
	this=g_new0(struct mapset_search,1);
	this->map=ms->maps;
	this->item=item;
	this->search_attr=search_attr;
	this->partial=partial;
	this->ms=map_search_new(this->map->data, item, search_attr, partial);
	return this;
}

/**
 * @brief Returns the next found item from a mapset search
 *
 * This function returns the next item from a mapset search or NULL if there are no more items found.
 * It automatically iterates through all the maps in the mapset. Please note that maps which have the
 * attr_active attribute associated with them and set to false are not searched.
 *
 * @param this The mapset search to return an item from
 * @return The next found item or NULL if there are no more items found
 */
struct item *
mapset_search_get_item(struct mapset_search *this)
{
	struct item *ret=NULL;
	struct attr active_attr;

	while (!this->ms || !(ret=map_search_get_item(this->ms))) { /* The current map has no more items to be returned */
		if (this->search_attr->type >= attr_country_all && this->search_attr->type <= attr_country_name)
			break;
		for (;;) {
			this->map=g_list_next(this->map);
			if (! this->map)
				break;
			if (!map_get_attr(this->map->data, attr_active, &active_attr, NULL))
				break;
			if (active_attr.u.num)
				break;
		}
		if (! this->map)
			break;
		map_search_destroy(this->ms);
		this->ms=map_search_new(this->map->data, this->item, this->search_attr, this->partial);
	}
	return ret;
}

/**
 * @brief Destroys a mapset search
 *
 * @param this The mapset search to be destroyed
 */
void
mapset_search_destroy(struct mapset_search *this)
{
	if (this) {
		map_search_destroy(this->ms);
		g_free(this);
	}
}
