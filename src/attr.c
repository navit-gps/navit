#include <string.h>
#include <glib.h>
#include "debug.h"
#include "item.h"
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

struct attr *
attr_new_from_text(char *name, char *value)
{
	enum attr_type attr;
	struct attr *ret;

	ret=g_new0(struct attr, 1);
	dbg(0,"enter name='%s' value='%s'\n", name, value);
	attr=attr_from_name(name);
	ret->type=attr;
	switch (attr) {
	case attr_item_type:
		ret->u.item_type=item_from_name(value);
		break;
	default:
		dbg(0,"default\n");
		g_free(ret);
		ret=NULL;
	}
	return ret;
}

struct attr *
attr_search(struct attr **attrs, struct attr *last, enum attr_type attr)
{
	dbg(0, "enter attrs=%p\n", attrs);
	while (*attrs) {
		dbg(0,"*attrs=%p\n", *attrs);
		if ((*attrs)->type == attr) {
			return *attrs;
		}
		attrs++;
	}
	exit(0);
	return NULL;
}

void
attr_free(struct attr *attr)
{
	g_free(attr);
}
