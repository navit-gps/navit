/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include <string.h>
#include <glib.h>
#include <gmodule.h>
#include "config.h"
#include "plugin.h"
#include "file.h"
#define PLUGIN_C
#include "plugin.h"
#include "item.h"
#include "debug.h"

struct plugin {
	int active;
	int lazy;
	int ondemand;
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

static int
plugin_get_ondemand(struct plugin *pl)
{
	return pl->ondemand;
}

static void
plugin_set_ondemand(struct plugin *pl, int ondemand)
{
	pl->ondemand=ondemand;
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
plugins_add_path(struct plugins *pls, struct attr **attrs) {
	struct attr *path_attr, *attr;
	struct file_wordexp *we;
	int active=1; // default active
	int lazy=0, ondemand=0;
	int i, count;
	char **array;
	char *name;
	struct plugin *pl;

	if (! (path_attr=attr_search(attrs, NULL, attr_path))) {
		dbg(0,"missing path\n");
		return;
	}
	if ( (attr=attr_search(attrs, NULL, attr_active))) {
		active=attr->u.num;
	}
	if ( (attr=attr_search(attrs, NULL, attr_lazy))) {
		lazy=attr->u.num;
	}
	if ( (attr=attr_search(attrs, NULL, attr_ondemand))) {
		ondemand=attr->u.num;
	}
	dbg(1, "path=\"%s\", active=%d, lazy=%d, ondemand=%d\n",path_attr->u.str, active, lazy, ondemand);

	we=file_wordexp_new(path_attr->u.str);
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
		plugin_set_ondemand(pl, ondemand);
	}
	file_wordexp_destroy(we);
}

void
plugins_init(struct plugins *pls)
{
#ifdef USE_PLUGINS
	struct plugin *pl;
	GList *l;

	l=pls->list;
	while (l) {
		pl=l->data;
		if (! plugin_get_ondemand(pl)) {
			if (plugin_get_active(pl)) 
				if (!plugin_load(pl)) 
					plugin_set_active(pl, 0);
			if (plugin_get_active(pl)) 
				plugin_call_init(pl);
		}
		l=g_list_next(l);
	}
#endif
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
