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


/** Buffer for temporarily storing an item. */
static char misc_item_buffer[20000000];
/** An item_bin for temporary use. */
struct item_bin *tmp_item_bin=(struct item_bin *)(void *)misc_item_buffer;
/** A node_item for temporary use. */
static struct node_item *tmp_node_item=(struct node_item *)(void *)misc_item_buffer;

struct node_item *
read_node_item(FILE *in) {
    if (fread(tmp_node_item, sizeof(struct node_item), 1, in) != 1)
        return NULL;
    return tmp_node_item;
}

struct item_bin *
read_item(FILE *in) {
    struct item_bin *ib=(struct item_bin *) misc_item_buffer;
    for (;;) {
        switch (item_bin_read(ib, in)) {
        case 0:
            return NULL;
        case 2:
            dbg_assert((ib->len+1)*4 < sizeof(misc_item_buffer));
            bytes_read+=(ib->len+1)*sizeof(int);
            return ib;
        default:
            continue;
        }
    }
}

struct item_bin *
read_item_range(FILE *in, int *min, int *max) {
    struct range r;

    if (fread(&r, sizeof(r), 1, in) != 1)
        return NULL;
    *min=r.min;
    *max=r.max;
    return read_item(in);
}

struct item_bin *
init_item(enum item_type type) {
    struct item_bin *ib=(struct item_bin *) misc_item_buffer;

    item_bin_init(ib, type);
    return ib;
}
