#ifndef NAVIT_MAPTYPE_H
#define NAVIT_MAPTYPE_H

struct map_methods;

struct maptype {
	char *name;
	struct map_priv *(*map_new)(struct map_methods *meth, char *data, char **charset, enum projection *pro);
	struct maptype *next;	
};

/* prototypes */
enum projection;
struct map_methods;
struct map_priv;
struct maptype;
void maptype_register(char *name, struct map_priv *(*map_new)(struct map_methods *meth, char *data, char **charset, enum projection *pro));
struct maptype *maptype_get(const char *name);

#endif

