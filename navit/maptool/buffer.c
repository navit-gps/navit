/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2018 Navit Team
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
#include "navit_lfs.h"
#include <stdlib.h>
#include "maptool.h"
#include "debug.h"

/**
 * @brief Saves a buffer to a file
 *
 * This function saves a buffer to a file.
 *
 * @param filename The name of the while to where the buffer is saved to.
 * @param b Buffer which is saved to file.
 * @param offset
 */
void save_buffer(char *filename, struct buffer *b, long long offset) {
    FILE *f;
    f=fopen(filename,"rb+");
    if (! f)
        f=fopen(filename,"wb+");

    dbg_assert(f != NULL);
    dbg_assert(fseeko(f, offset, SEEK_SET)==0);
    dbg_assert(fwrite(b->base, b->size, 1, f)==1);
    fclose(f);
}
/**
 * @brief Loads a buffer from a file
 *
 * This function loads a buffer from a file.
 *
 * @param filename The name of the while to where the buffer is loaded from.
 * @param b Buffer in which file is loaded.
 * @param offset
 * @return indicator if operation suceeded
 */
int load_buffer(char *filename, struct buffer *b, long long offset, long long size) {
    FILE *f;
    long long len;
    dbg_assert(size>=0);
    dbg_assert(offset>=0);
    g_free(b->base);
    b->malloced=0;
    f=fopen(filename,"rb");
    fseeko(f, 0, SEEK_END);
    len=ftello(f);
    dbg_assert(len>=0);
    if (offset+size > len) {
        size=len-offset;
    }
    b->size=b->malloced=size;
    dbg_assert(b->size>0);

    fseeko(f, offset, SEEK_SET);
    b->base=g_malloc(b->size);
    if (fread(b->base, b->size, 1, f) == 0) {
        dbg(lvl_warning, "fread failed");
        fclose(f);
        return 0;
    }
    fclose(f);
    return 1;
}
/**
 * @brief Determines size of buffer for file
 *
 * This function determines the size of the buffer required to read a file.
 *
 * @param  filename Name of file for which the required size of the buffer is determined
 * @return required size of buffer
 */
long long sizeof_buffer(char *filename) {
    long long ret;
    FILE *f=fopen(filename,"rb");
    fseeko(f, 0, SEEK_END);
    ret=ftello(f);
    fclose(f);
    return ret;
}
