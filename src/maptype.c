#include <glib.h>
#include "map.h"
#include "maptype.h"

static struct maptype *maptype_root;

void
maptype_register(char *name, struct map_priv *(*map_new)(struct map_methods *meth, char *data, char **charset, enum projection *pro))
{
	struct maptype *mt;
	mt=g_new(struct maptype, 1);
	mt->name=g_strdup(name);
	mt->map_new=map_new;
	mt->next=maptype_root;
	maptype_root=mt;	
}

struct maptype *
maptype_get(const char *name)
{
	struct maptype *mt=maptype_root;

	while (mt) {
		if (!g_ascii_strcasecmp(mt->name, name))
			return mt;
		mt=mt->next;
	}
	return NULL;
}
