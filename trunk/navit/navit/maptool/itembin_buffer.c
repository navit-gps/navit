#include <string.h>
#include <stdlib.h>
#include "maptool.h"
#include "debug.h"

struct item_bin *
read_item(FILE *in)
{
	struct item_bin *ib=(struct item_bin *) buffer;
	int r,s;
	r=fread(ib, sizeof(*ib), 1, in);
	if (r != 1)
		return NULL;
	bytes_read+=r;
	dbg_assert((ib->len+1)*4 < sizeof(buffer));
	s=(ib->len+1)*4-sizeof(*ib);
	r=fread(ib+1, s, 1, in);
	if (r != 1)
		return NULL;
	bytes_read+=r;
	return ib;
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
