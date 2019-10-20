/*
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

struct node_item *
read_node_item(FILE *in) {
#define ITEM_COUNT 1*1024*1024
    /* we read in bigger chunks as file IO in small chunks is slow as hell */
    static FILE * last_in = NULL;
    static long last_pos = 0;
    static int in_count;
    static int out_count;
    static struct node_item item_buffer[sizeof(struct node_item) * ITEM_COUNT];

    struct node_item * retval = NULL;

    if((last_in != in) || (last_pos != ftell(in))) {
        if((out_count - in_count) > 0)
            fprintf(stderr, "change file. Still %d items\n", out_count - in_count);
        /* got new file. flush buffer. */
        in_count=0;
        out_count=0;
        last_in=in;
    }

    /* check if we need to really read from file */
    if ((in_count - out_count) > 0) {
        /* no, return item from buffer */
        retval=&(item_buffer[out_count]);
    } else {
        out_count=0;
        in_count=fread(item_buffer, sizeof(struct node_item), ITEM_COUNT, in);
        //fprintf(stderr, "read %d items\n", in_count);
        if(in_count < 1) {
            /* buffer still empty after read */
            return NULL;
        }
        /* yes, try to read full buffer at once */
        retval=&item_buffer[0];
    }
    out_count ++;
    last_pos=ftell(in);

    return retval;
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
