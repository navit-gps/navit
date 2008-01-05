#ifndef NAVIT_ATTR_H
#define NAVIT_ATTR_H

#ifdef __cplusplus
extern "C" {
#endif
#ifndef ATTR_H
#define ATTR_H

enum item_type;

enum attr_type {
#define ATTR2(x,y) attr_##y=x,
#define ATTR(x) attr_##x,
#include "attr_def.h"
#undef ATTR2
#undef ATTR
};

#define AF_ONEWAY	(1<<0)
#define AF_ONEWAYREV	(1<<1)
#define AF_NOPASS	(AF_ONEWAY|AF_ONEWAYREV)
#define AF_ONEWAYMASK	(AF_ONEWAY|AF_ONEWAYREV)
#define AF_SEGMENTED	(1<<2)


struct attr {
	enum attr_type type;
	union {
		char *str;
		int num;
		struct item *item;
		enum item_type item_type;
		double * numd;
		struct color *color;
		struct coord_geo *coord_geo;
		struct navit *navit;
		struct callback *callback;
		struct log *log;
		struct route *route;
		struct navigation *navigation;
	} u;
};

/* prototypes */
enum attr_type;
struct attr;
enum attr_type attr_from_name(const char *name);
char *attr_to_name(enum attr_type attr);
struct attr *attr_new_from_text(const char *name, const char *value);
struct attr *attr_search(struct attr **attrs, struct attr *last, enum attr_type attr);
int attr_data_size(struct attr *attr);
void *attr_data_get(struct attr *attr);
void attr_data_set(struct attr *attr, void *data);
void attr_free(struct attr *attr);
/* end of prototypes */
#endif
#ifdef __cplusplus
}
#endif

#endif
