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

#ifndef NAVIT_MAPTYPE_H
#define NAVIT_MAPTYPE_H

struct map_methods;
enum projection;

struct maptype {
	char *name;
	struct map_priv *(*map_new)(struct map_methods *meth, char *data, char **charset, enum projection *pro);
	struct maptype *next;
};

/* prototypes */
struct map_methods;
struct map_priv;
struct maptype;
void maptype_register(char *name, struct map_priv *(*map_new)(struct map_methods *meth, char *data, char **charset, enum projection *pro));
struct maptype *maptype_get(const char *name);

#endif

