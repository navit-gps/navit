#include <glib.h>
#include <gmodule.h>
#include "plugin.h"
#include "file.h"
#define PLUGIN_C
#include "plugin.h"

struct plugin {
	int active;
	int lazy;
	char *name;
	GModule *mod;
	void (*init)(void);
};

struct plugins {
	GHashTable *hash;
	GList *list;
};

struct plugin *
plugin_new(char *plugin)
{
	struct plugin *ret;
	if (! g_module_supported()) {
		return NULL;
	}
	ret=g_new0(struct plugin, 1);
	ret->name=g_strdup(plugin);
	return ret;
	
}

int
plugin_load(struct plugin *pl)
{
	gpointer init;

	GModule *mod;

	if (pl->mod) {
		g_warning("can't load '%s', already loaded\n", pl->name);
		return 0;
	}
	mod=g_module_open(pl->name, G_MODULE_BIND_LOCAL | (pl->lazy ? G_MODULE_BIND_LAZY : 0));
	if (! mod) {
		g_warning("can't load '%s', Error '%s'\n", pl->name, g_module_error());
		return 0;
	}
	if (!g_module_symbol(mod, "plugin_init", &init)) {
		g_warning("can't load '%s', plugin_init not found\n", pl->name);
		g_module_close(mod);
		return 0;
	} else {
		pl->mod=mod;
		pl->init=init;
	}
	return 1;
}

char *
plugin_get_name(struct plugin *pl)
{
	return pl->name;
}

int
plugin_get_active(struct plugin *pl)
{
	return pl->active;
}

void
plugin_set_active(struct plugin *pl, int active)
{
	pl->active=active;
}

void
plugin_set_lazy(struct plugin *pl, int lazy)
{
	pl->lazy=lazy;
}

void
plugin_call_init(struct plugin *pl)
{
	pl->init();
}

void
plugin_unload(struct plugin *pl)
{
	g_module_close(pl->mod);
	pl->mod=NULL;
}

void
plugin_destroy(struct plugin *pl)
{
	g_free(pl);
}

struct plugins *
plugins_new(void)
{
	struct plugins *ret=g_new0(struct plugins, 1);
	ret->hash=g_hash_table_new(g_str_hash, g_str_equal);
	return ret;
}

void
plugins_add_path(struct plugins *pls, const char *path, int active, int lazy)
{
	struct file_wordexp *we;
	int i, count;
	char **array;
	char *name;
	struct plugin *pl;

	we=file_wordexp_new(path);
	count=file_wordexp_get_count(we);
	array=file_wordexp_get_array(we);	
	for (i = 0 ; i < count ; i++) {
		name=array[i];
		if (! (pl=g_hash_table_lookup(pls->hash, name))) {
			pl=plugin_new(name);
			if (! pl) {
				g_warning("failed to create plugin '%s'\n", name);
				continue;
			}
			g_hash_table_insert(pls->hash, plugin_get_name(pl), pl);
			pls->list=g_list_append(pls->list, pl);
		} else {
			pls->list=g_list_remove(pls->list, pl);
			pls->list=g_list_append(pls->list, pl);
		}
		plugin_set_active(pl, active);
		plugin_set_lazy(pl, lazy);
	}
	file_wordexp_destroy(we);
}

void
plugins_init(struct plugins *pls)
{
	struct plugin *pl;
	GList *l;

	l=pls->list;
	while (l) {
		pl=l->data;
		if (plugin_get_active(pl)) 
			if (!plugin_load(pl)) 
				plugin_set_active(pl, 0);
		l=g_list_next(l);
	}
	l=pls->list;
	while (l) {
		pl=l->data;
		if (plugin_get_active(pl)) 
			plugin_call_init(pl);
		l=g_list_next(l);
	}
}

void
plugins_destroy(struct plugins *pls)
{
	GList *l;
	struct plugin *pl;

	l=pls->list;
	while (l) {
		pl=l->data;
		plugin_unload(pl);
		plugin_destroy(pl);
	}
	g_list_free(pls->list);
	g_hash_table_destroy(pls->hash);
	g_free(pls);
}

void *
plugin_get_type(enum plugin_type type, const char *name)
{
	GList *l;
	struct name_val *nv;
	l=plugin_types[type];
	while (l) {
		nv=l->data;
	 	if (!g_ascii_strcasecmp(nv->name, name))
			return nv->val;
		l=g_list_next(l);
	}
	return NULL;
}
