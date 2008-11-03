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
 * @brief Contains code that makes navit able to load maps
 *
 * This file contains the code that makes navit able to load maps. Because
 * navit is able to open maps in different formats, this code does not handle
 * any map format itself. This is done by map plugins which register to this
 * code by calling plugin_register_map_type().
 *
 * When opening a new map, the map plugin will return a pointer to a map_priv
 * struct, which can be defined by the map plugin and contains whatever private
 * data the map plugin needs to access the map. This pointer will also be used
 * as a "handle" to access the map opened.
 *
 * A common task is to create a "map rect". A map rect is a rectangular part of
 * the map, that one can for example retrieve items from. It is not possible to
 * retrieve items directly from the complete map. Creating a map rect returns a
 * pointer to a map_rect_priv, which contains private data for the map rect and
 * will be used as "handle" for this map rect.
 */

#include <glib.h>
#include <string.h>
#include "debug.h"
#include "coord.h"
#include "projection.h"
#include "map.h"
#include "maptype.h"
#include "transform.h"
#include "item.h"
#include "plugin.h"
#include "callback.h"
#include "country.h"

/**
 * @brief Holds information about a map
 *
 * This structure holds information about a map.
 */
struct map {
	struct map_methods meth;			/**< Structure with pointers to the map plugin's functions */
	struct map_priv *priv;				/**< Private data of the map, only known to the map plugin */
	struct attr **attrs;				/**< Attributes of this map */
	struct callback_list *attr_cbl;		/**< List of callbacks that are called when attributes change */
};

/**
 * @brief Describes a rectangular extract of a map
 *
 * This structure describes a rectangular extract of a map.
 */
struct map_rect {
	struct map *m;				/**< The map this extract is from */
	struct map_rect_priv *priv; /**< Private data of this map rect, only known to the map plugin */
};

/**
 * @brief Opens a new map
 *
 * This function opens a new map based on the attributes passed. This function
 * takes the attribute "attr_type" to determine which type of map to open and passes
 * all attributes to the map plugin's function that was specified in the
 * plugin_register_new_map_type()-call.
 *
 * Note that every plugin should accept an attribute of type "attr_data" to be passed
 * with the filename of the map to be opened as value.
 *
 * @param attrs Attributes specifying which map to open, see description
 * @return The opened map or NULL on failure
 */
struct map *
map_new(struct attr *parent, struct attr **attrs)
{
	struct map *m;
	struct map_priv *(*maptype_new)(struct map_methods *meth, struct attr **attrs);
	struct attr *type=attr_search(attrs, NULL, attr_type);

	if (! type) {
		dbg(0,"missing type\n");
		return NULL;
	}
	maptype_new=plugin_get_map_type(type->u.str);
	if (! maptype_new) {
		dbg(0,"invalid type '%s'\n", type->u.str);
		return NULL;
	}

	m=g_new0(struct map, 1);
	m->attrs=attr_list_dup(attrs);
	m->priv=maptype_new(&m->meth, attrs);
	if (! m->priv) {
		g_free(m);
		m=NULL;
	}
	if (m)
		m->attr_cbl=callback_list_new();
	return m;
}

/**
 * @brief Gets an attribute from a map
 *
 * @param this_ The map the attribute should be read from
 * @param type The type of the attribute to be read
 * @param attr Pointer to an attrib-structure where the attribute should be written to
 * @param iter (NOT IMPLEMENTED) Used to iterate through all attributes of a type. Set this to NULL to get the first attribute, set this to an attr_iter to get the next attribute
 * @return True if the attribute type was found, false if not
 */
int
map_get_attr(struct map *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter)
{
	return attr_generic_get_attr(this_->attrs, NULL, type, attr, iter);
}

/**
 * @brief Sets an attribute of a map
 *
 * This sets an attribute of a map, overwriting an attribute of the same type if it
 * already exists. This function also calls all the callbacks that are registred
 * to be called when attributes change.
 *
 * @param this_ The map to set the attribute of
 * @param attr The attribute to set
 * @return True if the attr could be set, false otherwise
 */
int
map_set_attr(struct map *this_, struct attr *attr)
{
	this_->attrs=attr_generic_set_attr(this_->attrs, attr);
	callback_list_call_attr_2(this_->attr_cbl, attr->type, this_, attr);
	return 1;
}

/**
 * @brief Registers a new callback for attribute-change
 *
 * This function registers a new callback function that should be called if the attributes
 * of the map change.
 *
 * @param this_ The map to associate the callback with
 * @param cb The callback to add
 */
void
map_add_callback(struct map *this_, struct callback *cb)
{
	callback_list_add(this_->attr_cbl, cb);
}

/**
 * @brief Removes a callback from the list of attribute-change callbacks
 *
 * This function removes one callback from the list of callbacks functions that should be called
 * when attributes of the map change.
 *
 * @param this_ The map to remove the callback from
 * @param cb The callback to remove
 */
void
map_remove_callback(struct map *this_, struct callback *cb)
{
	callback_list_remove(this_->attr_cbl, cb);
}


/**
 * @brief Checks if strings from a map have to be converted
 *
 * @param this_ Map to be checked for the need to convert strings
 * @return True if strings from the map have to be converted, false otherwise
 */
int
map_requires_conversion(struct map *this_)
{
	return (this_->meth.charset != NULL && strcmp(this_->meth.charset, "utf-8"));
}

/**
 * @brief Converts a string from a map
 *
 * @param this_ The map the string to be converted is from
 * @param str The string to be converted
 * @return The converted string. It has to be map_convert_free()d after use.
 */
char *
map_convert_string(struct map *this_, char *str)
{
	return g_convert(str, -1,"utf-8",this_->meth.charset,NULL,NULL,NULL);
}

/**
 * @brief Frees the memory allocated for a converted string
 *
 * @param str The string to be freed
 */
void
map_convert_free(char *str)
{
	g_free(str);
}

/**
 * @brief Returns the projection of a map
 *
 * @param this_ The map to return the projection of
 * @return The projection of the map
 */
enum projection
map_projection(struct map *this_)
{
	return this_->meth.pro;
}

/**
 * @brief Sets the projection of a map
 *
 * @param this_ The map to set the projection of
 * @param pro The projection to be set
 */
void
map_set_projection(struct map *this_, enum projection pro)
{
	this_->meth.pro=pro;
}

/**
 * @brief Destroys an opened map
 *
 * @param m The map to be destroyed
 */
void
map_destroy(struct map *m)
{
	m->meth.map_destroy(m->priv);
	attr_list_free(m->attrs);
	g_free(m);
}

/**
 * @brief Creates a new map rect
 *
 * This creates a new map rect, which can be used to retrieve items from a map. If
 * sel is a linked-list of selections, all of them will be used. If you pass NULL as
 * sel, this means "get me the whole map".
 *
 * @param m The map to build the rect on
 * @param sel Map selection to choose the rectangle - may be NULL, see description
 * @return A new map rect
 */
struct map_rect *
map_rect_new(struct map *m, struct map_selection *sel)
{
	struct map_rect *mr;

#if 0
	printf("map_rect_new 0x%x,0x%x-0x%x,0x%x\n", r->lu.x, r->lu.y, r->rl.x, r->rl.y);
#endif
	mr=g_new0(struct map_rect, 1);
	mr->m=m;
	mr->priv=m->meth.map_rect_new(m->priv, sel);
	if (! mr->priv) {
		g_free(mr);
		mr=NULL;
	}

	return mr;
}

/**
 * @brief Gets the next item from a map rect
 *
 * Returns an item from a map rect and advances the "item pointer" one step further,
 * so that at the next call the next item is returned. Returns NULL if there are no more items.
 *
 * @param mr The map rect to return an item from
 * @return An item from the map rect
 */
struct item *
map_rect_get_item(struct map_rect *mr)
{
	struct item *ret;
	dbg_assert(mr != NULL);
	dbg_assert(mr->m != NULL);
	dbg_assert(mr->m->meth.map_rect_get_item != NULL);
	ret=mr->m->meth.map_rect_get_item(mr->priv);
	if (ret)
		ret->map=mr->m;
	return ret;
}

/**
 * @brief Returns the item specified by the ID
 *
 * @param mr The map rect to search for the item
 * @param id_hi High part of the ID to be found
 * @param id_lo Low part of the ID to be found
 * @return The item with the specified ID or NULL if not found
 */
struct item *
map_rect_get_item_byid(struct map_rect *mr, int id_hi, int id_lo)
{
	struct item *ret=NULL;
	dbg_assert(mr != NULL);
	dbg_assert(mr->m != NULL);
	if (mr->m->meth.map_rect_get_item_byid)
		ret=mr->m->meth.map_rect_get_item_byid(mr->priv, id_hi, id_lo);
	if (ret)
		ret->map=mr->m;
	return ret;
}

/**
 * @brief Destroys a map rect
 *
 * @param mr The map rect to be destroyed
 */
void
map_rect_destroy(struct map_rect *mr)
{
	mr->m->meth.map_rect_destroy(mr->priv);
	g_free(mr);
}

/**
 * @brief Holds information about a search on a map
 *
 * This structure holds information about a search performed on a map. This can be
 * used as "handle" to retrieve items from a search.
 */
struct map_search {
        struct map *m;
        struct attr search_attr;
        void *priv;
};

/**
 * @brief Starts a search on a map
 *
 * This function starts a search on a map. What attributes one can search for depends on the
 * map plugin.
 *
 * The OSM/binfile plugin currently supports: attr_town_name, attr_street_name
 * The MG plugin currently supports: ttr_town_postal, attr_town_name, attr_street_name
 *
 * If you enable partial matches bear in mind that the search matches only the begin of the
 * strings - a search for a street named "street" would match to "streetfoo", but not to
 * "somestreet". Search is case insensitive.
 *
 * The item passed to this function specifies a "superior item" to "search within" - e.g. a town 
 * in which we want to search for a street, or a country in which to search for a town.
 *
 * Please also note that the search for countries is not handled by map plugins but by navit internally -
 * have a look into country.c for details. Because of that every map plugin has to accept a country item
 * to be passed as "superior item".
 * 
 * Note: If you change something here, please make shure to also update the documentation of mapset_search_new()
 * in mapset.c!
 *
 * @param m The map that should be searched
 * @param item Specifies a superior item to "search within" (see description)
 * @param search_attr Attribute specifying what to search for. See description.
 * @param partial Set this to true to also have partial matches. See description.
 * @return A new map search struct for this search
 */
struct map_search *
map_search_new(struct map *m, struct item *item, struct attr *search_attr, int partial)
{
	struct map_search *this_;
	dbg(1,"enter(%p,%p,%p,%d)\n", m, item, search_attr, partial);
	dbg(1,"0x%x 0x%x 0x%x\n", attr_country_all, search_attr->type, attr_country_name);
	this_=g_new0(struct map_search,1);
	this_->m=m;
	this_->search_attr=*search_attr;
	if (search_attr->type >= attr_country_all && search_attr->type <= attr_country_name)
		this_->priv=country_search_new(&this_->search_attr, partial);
	else {
		if (m->meth.map_search_new) {
			if (m->meth.charset)
				this_->search_attr.u.str=g_convert(this_->search_attr.u.str, -1,m->meth.charset,"utf-8",NULL,NULL,NULL);
			this_->priv=m->meth.map_search_new(m->priv, item, &this_->search_attr, partial);
			if (! this_->priv) {
				g_free(this_);
				this_=NULL;
			}
		} else {
			g_free(this_);
			this_=NULL;
		}
	}
	return this_;
}

/**
 * @brief Returns an item from a map search
 *
 * This returns an item of the result of a search on a map and advances the "item pointer" one step,
 * so that at the next call the next item will be returned. If there are no more items in the result
 * NULL is returned.
 *
 * @param this_ Map search struct of the search
 * @return One item of the result
 */
struct item *
map_search_get_item(struct map_search *this_)
{
	struct item *ret;

	if (! this_)
		return NULL;
	if (this_->search_attr.type >= attr_country_all && this_->search_attr.type <= attr_country_name)
		return country_search_get_item(this_->priv);
	ret=this_->m->meth.map_search_get_item(this_->priv);
	if (ret)
		ret->map=this_->m;
	return ret;
}

/**
 * @brief Destroys a map search struct
 *
 * @param this_ The map search struct to be destroyed
 */
void
map_search_destroy(struct map_search *this_)
{
	if (! this_)
		return;
	if (this_->search_attr.type >= attr_country_all && this_->search_attr.type <= attr_country_name)
		country_search_destroy(this_->priv);
	else {
		if (this_->m->meth.charset)
				g_free(this_->search_attr.u.str);
		this_->m->meth.map_search_destroy(this_->priv);
	}
	g_free(this_);
}

/**
 * @brief Creates a new rectangular map selection
 *
 * @param center Coordinates of the center of the new rectangle
 * @param distance Distance of the rectangle's borders from the center
 * @param order Desired order of the new selection
 * @return The new map selection
 */
struct map_selection *
map_selection_rect_new(struct pcoord *center, int distance, int order)
{
	int i;
	struct map_selection *ret=g_new0(struct map_selection, 1);
	for (i = 0 ; i < layer_end ; i++) {
		ret->order[i]=order;
	}
	ret->u.c_rect.lu.x=center->x-distance;
	ret->u.c_rect.lu.y=center->y+distance;
	ret->u.c_rect.rl.x=center->x+distance;
	ret->u.c_rect.rl.y=center->y-distance;
	return ret;
}

/**
 * @brief Duplicates a map selection, transforming coordinates
 *
 * This duplicates a map selection and at the same time transforms the internal
 * coordinates of the selection from one projection to another.
 *
 * @param sel The map selection to be duplicated
 * @param from The projection used for the selection at the moment
 * @param to The projection that should be used for the duplicated selection
 * @return A duplicated, transformed map selection
 */
struct map_selection *
map_selection_dup_pro(struct map_selection *sel, enum projection from, enum projection to)
{
	struct map_selection *next,**last;
	struct map_selection *ret=NULL;
	last=&ret;
	while (sel) {
		next = g_new(struct map_selection, 1);
		*next=*sel;
		if (from != projection_none || to != projection_none) {
			transform_from_to(&sel->u.c_rect.lu, from, &next->u.c_rect.lu, to);
			transform_from_to(&sel->u.c_rect.rl, from, &next->u.c_rect.rl, to);
		}
		*last=next;
		last=&next->next;
		sel = sel->next;
	}
	return ret;
}

/**
 * @brief Duplicates a map selection
 *
 * @param sel The map selection to duplicate
 * @return The duplicated map selection
 */
struct map_selection *
map_selection_dup(struct map_selection *sel)
{
	return map_selection_dup_pro(sel, projection_none, projection_none);
}

/**
 * @brief Destroys a map selection
 *
 * @param sel The map selection to be destroyed
 */
void
map_selection_destroy(struct map_selection *sel)
{
	struct map_selection *next;
	while (sel) {
		next = sel->next;
		g_free(sel);
		sel = next;
	}
}

/**
 * @brief Checks if a selection contains a rectangle containing an item
 *
 * This function checks if a selection contains a rectangle which exactly contains
 * an item. The rectangle is automatically built around the given item.
 *
 * @param sel The selection to be checked
 * @param item The item that the rectangle should be built around
 * @return True if the rectangle is within the selection, false otherwise
 */
int
map_selection_contains_item_rect(struct map_selection *sel, struct item *item)
{
	struct coord c;
	struct coord_rect r;
	int count=0;
	while (item_coord_get(item, &c, 1)) {
		if (! count) {
			r.lu=c;
			r.rl=c;
		} else
			coord_rect_extend(&r, &c);
		count++;
	}
	if (! count)
		return 0;
	return map_selection_contains_rect(sel, &r);

}

/**
 * @brief Checks if a pointer points to the private data of a map
 *
 * @param map The map whose private data should be checked.
 * @param priv The private data that should be checked.
 * @return True if priv is the private data of map
 */
int
map_priv_is(struct map *map, struct map_priv *priv)
{
	return (map->priv == priv);
}

void
map_dump_filedesc(struct map *map, FILE *out)
{
	struct map_rect *mr=map_rect_new(map, NULL);
	struct item *item;
	int i,count,max=16384;
	struct coord ca[max];
	struct attr attr;

	while ((item = map_rect_get_item(mr))) {
		count=item_coord_get(item, ca, item->type < type_line ? 1: max);
		if (item->type < type_line) 
			fprintf(out,"mg:0x%x 0x%x ", ca[0].x, ca[0].y);
		fprintf(out,"%s", item_to_name(item->type));
		while (item_attr_get(item, attr_any, &attr)) 
			fprintf(out," %s='%s'", attr_to_name(attr.type), attr_to_text(&attr, map, 1));
		fprintf(out,"\n");
		if (item->type >= type_line)
			for (i = 0 ; i < count ; i++)
				fprintf(out,"mg:0x%x 0x%x\n", ca[i].x, ca[i].y);
	}
	map_rect_destroy(mr);
}

void
map_dump_file(struct map *map, char *file)
{
	FILE *f;
	f=fopen(file,"w");
	map_dump_filedesc(map, f);
	fclose(f);
}

void
map_dump(struct map *map)
{
	map_dump_filedesc(map, stdout);
}
