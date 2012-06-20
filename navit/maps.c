/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2012 Navit Team
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

#include <glib.h>
#include "item.h"
#include "debug.h"
#include "file.h"
#include "map.h"
#include "mapset.h"
#include "xmlconfig.h"

struct maps;

struct maps *
maps_new(struct attr *parent, struct attr **attrs)
{
	struct attr *data,**attrs_dup;
	if (!parent) {
		dbg(0,"No parent\n");
		return NULL;
	}
	if (parent->type != attr_mapset) {
		dbg(0,"Parent must be mapset\n");
		return NULL;
	}
	dbg(0,"enter\n");
	attrs_dup=attr_list_dup(attrs);
	data=attr_search(attrs_dup, NULL, attr_data);
	if (data) {
		struct file_wordexp *wexp=file_wordexp_new(data->u.str);
		int i,count=file_wordexp_get_count(wexp);
		char **array=file_wordexp_get_array(wexp);
		for (i = 0 ; i < count ; i++) {
			struct attr map;
			g_free(data->u.str);
			data->u.str=g_strdup(array[i]);
			map.type=attr_map;
			map.u.map=map_new(parent, attrs_dup);
			if (map.u.map) {
				mapset_add_attr(parent->u.mapset, &map);
				map_unref(map.u.map);
			}
			
		}
		file_wordexp_destroy(wexp);	
	} else {
		dbg(0,"no data attribute\n");
	}
	attr_list_free(attrs_dup);
	return NULL;
}

struct object_func maps_func = {
	attr_maps,
	(object_func_new)maps_new,
	(object_func_get_attr)NULL,
	(object_func_iter_new)NULL,
	(object_func_iter_destroy)NULL,
	(object_func_set_attr)NULL,
	(object_func_add_attr)NULL,
	(object_func_remove_attr)NULL,
	(object_func_init)NULL,
	(object_func_destroy)NULL,
	(object_func_dup)NULL,
	(object_func_ref)NULL,
	(object_func_unref)NULL,
};
