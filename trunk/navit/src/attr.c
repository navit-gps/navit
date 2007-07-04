#include <string.h>
#include "attr.h"

struct attr_name {
	enum attr_type attr;
	char *name;
};


static struct attr_name attr_names[]={
#define ATTR2(x,y) ATTR(x)
#define ATTR(x) { attr_##x, #x },
#include "attr_def.h"
#undef ATTR2
#undef ATTR
};

enum attr_type
attr_from_name(char *name)
{
	int i;

	for (i=0 ; i < sizeof(attr_names)/sizeof(struct attr_name) ; i++) {
		if (! strcmp(attr_names[i].name, name))
			return attr_names[i].attr;
	}
	return attr_none;
}

char *
attr_to_name(enum attr_type attr)
{
	int i;

	for (i=0 ; i < sizeof(attr_names)/sizeof(struct attr_name) ; i++) {
		if (attr_names[i].attr == attr)
			return attr_names[i].name;
	}
	return NULL; 
}
