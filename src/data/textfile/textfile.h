#include <stdio.h>
#include "attr.h"
#include "coord.h"
struct map_priv {
	int id;
	char *filename;
};

#define SIZE 512

struct map_rect_priv {
	struct map_selection *sel;

	FILE *f;
	long pos;
	char line[SIZE];
	int attr_pos;
	enum attr_type attr_last;
	char attrs[SIZE];
	char attr[SIZE];
	char attr_name[SIZE];
	struct coord c;
	int eoc;
	struct map_priv *m;
	struct item item;
};

