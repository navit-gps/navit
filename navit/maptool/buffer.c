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
#include "navit_lfs.h"
#include <stdlib.h>
#include "maptool.h"
#include "debug.h"

void
save_buffer(char *filename, struct buffer *b, long long offset)
{
	FILE *f;
	f=fopen(filename,"rb+");
	if (! f)
		f=fopen(filename,"wb+");
		
	dbg_assert(f != NULL);
	fseeko(f, offset, SEEK_SET);
	fwrite(b->base, b->size, 1, f);
	fclose(f);
}

void
load_buffer(char *filename, struct buffer *b, long long offset, long long size)
{
	FILE *f;
	long long len;
	dbg_assert(size>=0);
	dbg_assert(offset>=0);
	if (b->base)
		free(b->base);
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
#if 0
	fprintf(stderr,"reading "LONGLONG_FMT" bytes from %s of "LONGLONG_FMT" bytes at "LONGLONG_FMT"\n", b->size, filename, len, offset);
#endif
	fseeko(f, offset, SEEK_SET);
	b->base=malloc(b->size);
	dbg_assert(b->base != NULL);
	fread(b->base, b->size, 1, f);
	fclose(f);
}

long long
sizeof_buffer(char *filename)
{
	long long ret;
	FILE *f=fopen(filename,"rb");
	fseeko(f, 0, SEEK_END);
	ret=ftello(f);
	fclose(f);
	return ret;
}
