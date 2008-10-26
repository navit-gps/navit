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

/* prototypes */
struct attr;
struct item;
struct map;
struct mapset;
struct mapset_handle;
struct mapset_search;
struct mapset *mapset_new(struct attr *parent, struct attr **attrs);
int mapset_add_attr(struct mapset *ms, struct attr *attr);
void mapset_destroy(struct mapset *ms);
struct mapset_handle *mapset_open(struct mapset *ms);
struct map *mapset_next(struct mapset_handle *msh, int active);
void mapset_close(struct mapset_handle *msh);
struct mapset_search *mapset_search_new(struct mapset *ms, struct item *item, struct attr *search_attr, int partial);
struct item *mapset_search_get_item(struct mapset_search *this);
void mapset_search_destroy(struct mapset_search *this);

#endif

