#include <glib.h>
#include <glib/gprintf.h>
#include "debug.h"
#include "attr.h"
#include "mapset.h"
#include "map.h"

struct mapset {
	GList *maps;
};

struct mapset *mapset_new(void)
{
	struct mapset *ms;

	ms=g_new0(struct mapset, 1);

	return ms;
}

void mapset_add(struct mapset *ms, struct map *m)
{
	ms->maps=g_list_append(ms->maps, m);
}

#if 0
static void mapset_maps_free(struct mapset *ms)
{
	/* todo */
}
#endif

void mapset_destroy(struct mapset *ms)
{
	g_free(ms);
}

struct mapset_handle {
	GList *l;
};

struct mapset_handle *
mapset_open(struct mapset *ms)
{
	struct mapset_handle *ret;

	ret=g_new(struct mapset_handle, 1);
	ret->l=ms->maps;

	return ret;
}

struct map * mapset_next(struct mapset_handle *msh, int active)
{
	struct map *ret;

	for (;;) {
		if (!msh->l)
			return NULL;
		ret=msh->l->data;
		msh->l=g_list_next(msh->l);
		if (!active || map_get_active(ret))
			return ret;
	}
}

void 
mapset_close(struct mapset_handle *msh)
{
	g_free(msh);
}

struct mapset_search {
	GList *map;
	struct map_search *ms;
	struct item *item;
	struct attr *search_attr;
	int partial;
};

struct mapset_search *
mapset_search_new(struct mapset *ms, struct item *item, struct attr *search_attr, int partial)
{
	struct mapset_search *this;
	dbg(1,"enter(%p,%p,%p,%d)\n", ms, item, search_attr, partial);
	this=g_new0(struct mapset_search,1);
	this->map=ms->maps;
	this->item=item;
	this->search_attr=search_attr;
	this->partial=partial;
	this->ms=map_search_new(this->map->data, item, search_attr, partial);
	return this;
}

struct item *
mapset_search_get_item(struct mapset_search *this)
{
	struct item *ret;
	while (!(ret=map_search_get_item(this->ms))) {
		if (this->search_attr->type >= attr_country_all && this->search_attr->type <= attr_country_name)
			break;
		this->map=g_list_next(this->map);
		if (! this->map)
			break;
		map_search_destroy(this->ms);
		this->ms=map_search_new(this->map->data, this->item, this->search_attr, this->partial);
	}
	return ret;
}

void
mapset_search_destroy(struct mapset_search *this)
{
	map_search_destroy(this->ms);
	g_free(this);
}
