#ifndef ATTR_H
#define ATTR_H

enum attr_type {
#define ATTR2(x,y) attr_##x=y,
#define ATTR(x) attr_##x,
#include "attr_def.h"
#undef ATTR2
#undef ATTR
};

struct attr {
	enum attr_type type;
	union {
		char *str;
		int num;
		struct item *item;
	} u;
};

enum attr_type attr_from_name(char *name);
char * attr_to_name(enum attr_type attr);

#endif
