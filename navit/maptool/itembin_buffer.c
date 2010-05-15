#include <string.h>
#include <stdlib.h>
#include "maptool.h"
#include "debug.h"


static char buffer[400000];
struct item_bin *item_bin=(struct item_bin *)(void *)buffer;

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
