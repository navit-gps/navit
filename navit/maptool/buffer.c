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
	fprintf(stderr,"reading "LONGLONG_FMT" bytes from %s at "LONGLONG_FMT"\n", b->size, filename, offset);
	fseek(f, offset, SEEK_SET);
	b->base=malloc(b->size);
	dbg_assert(b->base != NULL);
	fread(b->base, b->size, 1, f);
	fclose(f);
}
