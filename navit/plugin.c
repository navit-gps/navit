/*
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
#ifdef HAVE_API_WIN32_BASE
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#endif
#endif
#include "plugin.h"
#include "file.h"
#define PLUGIN_C
#include "plugin.h"
#include "item.h"
#include "debug.h"

/**
 * @defgroup plugins
 * @brief A interface to handle all plugins inside navit
 *
 * @{
 */

#ifdef USE_PLUGINS
#ifndef HAVE_GMODULE
typedef void * GModule;
#define G_MODULE_BIND_LOCAL 1
#define G_MODULE_BIND_LAZY 2
static int g_module_supported(void) {
    return 1;
}

#ifdef HAVE_API_WIN32_BASE

static DWORD last_error;
static char errormsg[64];

static void *g_module_open(char *name, int flags) {
    HINSTANCE handle;
    int len=MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, name, -1, 0, 0);
    wchar_t filename[len];
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, name, -1, filename, len) ;

    handle = LoadLibraryW (filename);
    if (!handle)
        last_error=GetLastError();
    return handle;
}

static char *g_module_error(void) {
    sprintf(errormsg,"dll error %d",(int)last_error);
    return errormsg;
}

static int g_module_symbol(GModule *handle, char *symbol, gpointer *addr) {
#ifdef HAVE_API_WIN32_CE
    int len=MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, symbol, -1, 0, 0);
    wchar_t wsymbol[len+1];
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, symbol, -1, wsymbol, len) ;
    *addr=GetProcAddress ((HANDLE)handle, wsymbol);
#else
    *addr=GetProcAddress ((HANDLE)handle, symbol);
#endif
    if (*addr)
        return 1;
    last_error=GetLastError();
    return 0;
}

static void g_module_close(GModule *handle) {
    FreeLibrary((HANDLE)handle);
}

#else
static void *g_module_open(char *name, int flags) {
    return dlopen(name,
                  (flags & G_MODULE_BIND_LAZY ? RTLD_LAZY : RTLD_NOW) |
                  (flags & G_MODULE_BIND_LOCAL ? RTLD_LOCAL : RTLD_GLOBAL));
}

static char *g_module_error(void) {
    return dlerror();
}

static int g_module_symbol(GModule *handle, char *symbol, gpointer *addr) {
    *addr=dlsym(handle, symbol);
    return (*addr != NULL);
}

static void g_module_close(GModule *handle) {
    dlclose(handle);
}
#endif
#endif
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

static struct plugin *plugin_new_from_path(char *plugin) {
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

int plugin_load(struct plugin *pl) {
#ifdef USE_PLUGINS
    gpointer init;

    GModule *mod;

    if (pl->mod) {
        dbg(lvl_debug,"'%s' already loaded, returning", pl->name);
        return 1;
    }
    mod=g_module_open(pl->name, G_MODULE_BIND_LOCAL | (pl->lazy ? G_MODULE_BIND_LAZY : 0));
    if (! mod) {
        dbg(lvl_error,"can't load '%s', Error '%s'", pl->name, g_module_error());
        return 0;
    }
    if (!g_module_symbol(mod, "plugin_init", &init)) {
        dbg(lvl_error,"can't load '%s', plugin_init not found", pl->name);
        g_module_close(mod);
        return 0;
    } else {
        dbg(lvl_debug, "loaded module %s", pl->name);
        pl->mod=mod;
        pl->init=init;
    }
    return 1;
#else
    return 0;
#endif
}

char *plugin_get_name(struct plugin *pl) {
    return pl->name;
}

int plugin_get_active(struct plugin *pl) {
    return pl->active;
}

void plugin_set_active(struct plugin *pl, int active) {
    pl->active=active;
}

void plugin_set_lazy(struct plugin *pl, int lazy) {
    pl->lazy=lazy;
}

#ifdef USE_PLUGINS
static int plugin_get_ondemand(struct plugin *pl) {
    return pl->ondemand;
}
#endif

static void plugin_set_ondemand(struct plugin *pl, int ondemand) {
    pl->ondemand=ondemand;
}

void plugin_call_init(struct plugin *pl) {
    pl->init();
}

void plugin_unload(struct plugin *pl) {
#ifdef USE_PLUGINS
    g_module_close(pl->mod);
    pl->mod=NULL;
#endif
}

void plugin_destroy(struct plugin *pl) {
    g_free(pl);
}

struct plugins *
plugins_new(void) {
    struct plugins *ret=g_new0(struct plugins, 1);
    ret->hash=g_hash_table_new(g_str_hash, g_str_equal);
    pls=ret;
    return ret;
}

struct plugin *
plugin_new(struct attr *parent, struct attr **attrs) {
#ifdef USE_PLUGINS
    struct attr *path_attr, *attr;
    struct file_wordexp *we;
    int active=1; // default active
    int lazy=0, ondemand=0;
    int i, count;
    char **array;
    char *name;
    struct plugin *pl=NULL;
    struct plugins *pls=NULL;

    if (parent)
        pls=parent->u.plugins;

    if (! (path_attr=attr_search(attrs, NULL, attr_path))) {
        dbg(lvl_error,"missing path");
        return NULL;
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
    dbg(lvl_debug, "path=\"%s\", active=%d, lazy=%d, ondemand=%d",path_attr->u.str, active, lazy, ondemand);

    we=file_wordexp_new(path_attr->u.str);
    count=file_wordexp_get_count(we);
    array=file_wordexp_get_array(we);
    dbg(lvl_info,"expanded to %d words",count);
    if (count != 1 || file_exists(array[0])) {
        for (i = 0 ; i < count ; i++) {
            name=array[i];
            dbg(lvl_info,"found plugin module file [%d]: '%s'", i, name);
            if (! (pls && (pl=g_hash_table_lookup(pls->hash, name)))) {
                pl=plugin_new_from_path(name);
                if (! pl) {
                    dbg(lvl_error,"failed to create plugin from file '%s'", name);
                    continue;
                }
                if (pls) {
                    g_hash_table_insert(pls->hash, plugin_get_name(pl), pl);
                    pls->list=g_list_append(pls->list, pl);
                }
            } else {
                if (pls) {
                    pls->list=g_list_remove(pls->list, pl);
                    pls->list=g_list_append(pls->list, pl);
                }
            }
            plugin_set_active(pl, active);
            plugin_set_lazy(pl, lazy);
            plugin_set_ondemand(pl, ondemand);
            if (!pls && active) {
                if (!plugin_load(pl))
                    plugin_set_active(pl, 0);
                else
                    plugin_call_init(pl);
            }
        }
    }
    file_wordexp_destroy(we);
    return pl;
#else
    return 0;
#endif
}

void plugins_init(struct plugins *pls) {
#ifdef USE_PLUGINS
    struct plugin *pl;
    GList *l;

    l=pls->list;
    if (l) {
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
    } else {
        dbg(lvl_error, "Warning: No plugins found. Is Navit installed correctly?");
    }
#endif
}

void plugins_destroy(struct plugins *pls) {
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

static void *find_by_name(enum plugin_category category, const char *name) {
    GList *name_list=plugin_categories[category];
    while (name_list) {
        struct name_val *nv=name_list->data;
        if (!g_ascii_strcasecmp(nv->name, name))
            return nv->val;
        name_list=g_list_next(name_list);
    }
    return NULL;
}

void *plugin_get_category(enum plugin_category category, const char *category_name, const char *name) {
    GList *plugin_list;
    struct plugin *pl;
    char *mod_name, *filename=NULL, *corename=NULL;
    void *result=NULL;

    dbg(lvl_debug, "category=\"%s\", name=\"%s\"", category_name, name);

    if ((result=find_by_name(category, name))) {
        return result;
    }
    if (!pls)
        return NULL;
    plugin_list=pls->list;
    filename=g_strjoin("", "lib", category_name, "_", name, NULL);
    corename=g_strjoin("", "lib", category_name, "_", "core", NULL);
    while (plugin_list) {
        pl=plugin_list->data;
        if ((mod_name=g_strrstr(pl->name, "/")))
            mod_name++;
        else
            mod_name=pl->name;
        if (!g_ascii_strncasecmp(mod_name, filename, strlen(filename))
                || !g_ascii_strncasecmp(mod_name, corename, strlen(corename))) {
            dbg(lvl_debug, "Loading module \"%s\"",pl->name) ;
            if (plugin_get_active(pl))
                if (!plugin_load(pl))
                    plugin_set_active(pl, 0);
            if (plugin_get_active(pl))
                plugin_call_init(pl);
            if ((result=find_by_name(category, name))) {
                g_free(filename);
                g_free(corename);
                return result;
            }
        }
        plugin_list=g_list_next(plugin_list);
    }
    g_free(filename);
    g_free(corename);
    return NULL;
}
