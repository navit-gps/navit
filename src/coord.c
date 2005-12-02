#include "coord.h"

struct coord *
coord_get(unsigned char **p)
{
	struct coord *ret=(struct coord *)(*p);
	*p += sizeof(*ret);
	return ret;
}
