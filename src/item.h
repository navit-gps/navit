

enum item_type {
#define ITEM2(x,y) type_##y=x,
#define ITEM(x) type_##x,
#include "item_def.h"
#undef ITEM2
#undef ITEM
};

#include "attr.h"

#define item_is_equal_id(a,b) ((a).id_hi == (b).id_hi && (a).id_lo == (b).id_lo)
#define item_is_equal(a,b) (item_is_equal_id(a,b) && (a).map == (b).map)

struct coord;

struct item_methods {
	void (*item_coord_rewind)(void *priv_data);
	int (*item_coord_get)(void *priv_data, struct coord *c, int count);
	void (*item_attr_rewind)(void *priv_data);
	int (*item_attr_get)(void *priv_data, enum attr_type attr_type, struct attr *attr);
};

struct item {
	enum item_type type;
	int id_hi;
	int id_lo;
	struct map *map;
	struct item_methods *meth;	
	void *priv_data;
};

/* prototypes */
enum attr_type;
enum item_type;
struct attr;
struct coord;
struct item;
struct item_hash;
void item_coord_rewind(struct item *it);
int item_coord_get(struct item *it, struct coord *c, int count);
void item_attr_rewind(struct item *it);
int item_attr_get(struct item *it, enum attr_type attr_type, struct attr *attr);
struct item *item_new(char *type, int zoom);
enum item_type item_from_name(const char *name);
char *item_to_name(enum item_type item);
struct item_hash *item_hash_new(void);
void item_hash_insert(struct item_hash *h, struct item *item, void *val);
int item_hash_remove(struct item_hash *h, struct item *item);
void *item_hash_lookup(struct item_hash *h, struct item *item);
void item_hash_destroy(struct item_hash *h);
/* end of prototypes */
