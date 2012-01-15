/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2011 Navit Team
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
#include <string.h>
#include <stdlib.h>
#include "maptool.h"
#include "debug.h"


static char buffer[800000];
struct item_bin *item_bin=(struct item_bin *)(void *)buffer;
static char buffer_relation_area[800000];
struct item_bin *item_bin_relation_area=(struct item_bin *)(void *)buffer_relation_area;
static struct node_item *node_item=(struct node_item *)(void *)buffer;

struct node_item *
read_node_item(FILE *in)
{
	if (fread(node_item, sizeof(struct node_item), 1, in) != 1)
		return NULL;
	return node_item;
}

struct item_bin *
read_item(FILE *in)
{
	struct item_bin *ib=(struct item_bin *) buffer;
	for (;;) {
		switch (item_bin_read(ib, in)) {
		case 0:
			return NULL;
		case 2:
			dbg_assert((ib->len+1)*4 < sizeof(buffer));
			bytes_read+=(ib->len+1)*sizeof(int);
			return ib;
		default:
			continue;
		}
	}
}

struct item_bin *
read_item_range(FILE *in, int *min, int *max)
{
	struct range r;

	if (fread(&r, sizeof(r), 1, in) != 1)
		return NULL;
	*min=r.min;
	*max=r.max;
	return read_item(in);
}

struct item_bin *
init_item(enum item_type type)
{
	struct item_bin *ib=(struct item_bin *) buffer;

	item_bin_init(ib, type);
	return ib;
}
