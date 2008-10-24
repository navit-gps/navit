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
#include "config.h"
#ifdef USE_PLUGINS
#ifdef HAVE_GMODULE
#include <gmodule.h>
#else
#include <dlfcn.h>
#endif
#endif
#include "plugin.h"
#include "file.h"
#define PLUGIN_C
#include "plugin.h"
#include "item.h"
#include "debug.h"

#ifndef HAVE_GMODULE
typedef void * GModule;
#define G_MODULE_BIND_LOCAL 1
#define G_MODULE_BIND_LAZY 2
static int
g_module_supported(void)
{
	return 1;
}

static void *
g_module_open(char *name, int flags)
{
	return dlopen(name,
		(flags & G_MODULE_BIND_LAZY ? RTLD_LAZY : RTLD_NOW) |
		(flags & G_MODULE_BIND_LOCAL ? RTLD_LOCAL : RTLD_GLOBAL));
}

static char *
g_module_error(void)
{
	return dlerror();
}

static int
g_module_symbol(GModule *handle, char *symbol, gpointer *addr)
{
	*addr=dlsym(handle, symbol);
	return (*addr != NULL);
}

static void
g_module_close(GModule *handle)
{
	dlclose(handle);
}

#endif

struct plugin {
	int active;
	int lazy;
	int ondemand;
	char *name;
#ifdef USE_PLUGINS
	GModule *mod;
#endif
	void (*init)(void);
};

struct plugins {
	GHashTable *hash;
	GList *list;
} *pls;

struct plugin *
plugin_new(char *plugin)
{
#ifdef USE_PLUGINS
	struct plugin *ret;
	if (! g_module_supported()) {
		return NULL;
	}
	ret=g_new0(struct plugin, 1);
	ret->name=g_strdup(plugin);
	return ret;
#else
	return NULL;
#endif
}

int
plugin_load(struct plugin *pl)
{
#ifdef USE_PLUGINS
	gpointer init;

	GModule *mod;

	if (pl->mod) {
		dbg(0,"can't load '%s', already loaded\n", pl->name);
		return 0;
	}
	mod=g_module_open(pl->name, G_MODULE_BIND_LOCAL | (pl->lazy ? G_MODULE_BIND_LAZY : 0));
	if (! mod) {
		dbg(0,"can't load '%s', Error '%s'\n", pl->name, g_module_error());
		return 0;
	}
	if (!g_module_symbol(mod, "plugin_init", &init)) {
		dbg(0,"can't load '%s', plugin_init not found\n", pl->name);
		g_module_close(mod);
		return 0;
	} else {
		pl->mod=mod;
		pl->init=init;
	}
	return 1;
#else
	return 0;
#endif
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

#ifdef USE_PLUGINS
static int
plugin_get_ondemand(struct plugin *pl)
{
	return pl->ondemand;
}
#endif

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
#ifdef USE_PLUGINS
	g_module_close(pl->mod);
	pl->mod=NULL;
#endif
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
	pls=ret;
	return ret;
}

void
plugins_add_path(struct plugins *pls, struct attr **attrs) {
#ifdef USE_PLUGINS 
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
				dbg(0,"failed to create plugin '%s'\n", name);
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
#endif
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
plugin_get_type(enum plugin_type type, const char *type_name, const char *name)
{
	dbg(1, "type=\"%s\", name=\"%s\"\n", type_name, name);
	GList *l,*lpls;
	struct name_val *nv;
	struct plugin *pl;
	char *mod_name, *filename=NULL, *corename=NULL;
	l=plugin_types[type];
	while (l) {
		nv=l->data;
		if (!g_ascii_strcasecmp(nv->name, name))
			return nv->val;
		l=g_list_next(l);
	}
	if (!pls)
		return NULL;
	lpls=pls->list;
	if(!g_ascii_strcasecmp(type_name, "map"))
		type_name="data";
	filename=g_strjoin("", "lib", type_name, "_", name, NULL);
	corename=g_strjoin("", "lib", type_name, "_", "core", NULL);
	while (lpls) {
		pl=lpls->data;
		if ((mod_name=g_strrstr(pl->name, "/")))
			mod_name++;
		else
			mod_name=pl->name;
		if (!g_ascii_strncasecmp(mod_name, filename, strlen(filename)) || !g_ascii_strncasecmp(mod_name, corename, strlen(filename))) {
			dbg(0, "Loading module \"%s\"\n",pl->name) ;
			if (plugin_get_active(pl)) 
				if (!plugin_load(pl)) 
					plugin_set_active(pl, 0);
			if (plugin_get_active(pl)) 
				plugin_call_init(pl);
			l=plugin_types[type];
			while (l) {
				nv=l->data;
				if (!g_ascii_strcasecmp(nv->name, name)) {
					g_free(filename);
					g_free(corename);
					return nv->val;
				}
				l=g_list_next(l);
			}
		}
		lpls=g_list_next(lpls);
	}
	g_free(filename);
	g_free(corename);
	return NULL;
}
