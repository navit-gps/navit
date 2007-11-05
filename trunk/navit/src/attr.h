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

struct attr {
	enum attr_type type;
	union {
		char *str;
		int num;
		struct item *item;
		enum item_type item_type;
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
