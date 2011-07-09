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
	fseek(f, offset, SEEK_SET);
	fwrite(b->base, b->size, 1, f);
	fclose(f);
}

void
load_buffer(char *filename, struct buffer *b, long long offset, long long size)
{
	FILE *f;
	long long len;
	int ret;
	if (b->base)
		free(b->base);
	b->malloced=0;
	f=fopen(filename,"rb");
	fseek(f, 0, SEEK_END);
	len=ftell(f);
	if (offset+size > len) {
		size=len-offset;
		ret=1;
	}
	b->size=b->malloced=size;
#if 0
	fprintf(stderr,"reading "LONGLONG_FMT" bytes from %s at "LONGLONG_FMT"\n", b->size, filename, offset);
#endif
	fseek(f, offset, SEEK_SET);
	b->base=malloc(b->size);
	dbg_assert(b->base != NULL);
	fread(b->base, b->size, 1, f);
	fclose(f);
}
