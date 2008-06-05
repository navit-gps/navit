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

#include <glib.h>
#include "projection.h"
#include "map.h"
#include "maptype.h"

static struct maptype *maptype_root;

void
maptype_register(char *name, struct map_priv *(*map_new)(struct map_methods *meth, char *data, char **charset, enum projection *pro))
{
	struct maptype *mt;
	mt=g_new(struct maptype, 1);
	mt->name=g_strdup(name);
	mt->map_new=map_new;
	mt->next=maptype_root;
	maptype_root=mt;	
}

struct maptype *
maptype_get(const char *name)
{
	struct maptype *mt=maptype_root;

	while (mt) {
		if (!g_ascii_strcasecmp(mt->name, name))
			return mt;
		mt=mt->next;
	}
	return NULL;
}
