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

#include <stdlib.h>
#include <glib.h>
#include <signal.h>
#include "debug.h"
#include "item.h"
#include "xmlconfig.h"
#include "callback.h"
#include "navit.h"
#include "config_.h"
#include "file.h"
#ifdef HAVE_API_WIN32_CE
#include "libc.h"
#endif

struct config {
    NAVIT_OBJECT
} *config;

struct config *
config_get(void) {
    return config;
}

int config_empty_ok;

int configured;

struct attr_iter {
    void *iter;
};

void config_destroy(struct config *this_) {
    attr_list_free(this_->attrs);
    g_free(config);
    exit(0);
}

static void config_terminate(int sig) {
    dbg(lvl_debug,"terminating");
    config_destroy(config);
}

static void config_new_int(void) {
    config=g_new0(struct config, 1);
#ifndef HAVE_API_WIN32_CE
    signal(SIGTERM, config_terminate);
#ifndef HAVE_API_WIN32
#ifndef _MSC_VER
#ifndef __MINGW32__
    signal(SIGPIPE, SIG_IGN);
#endif /* __MINGW32__ */
#endif
#endif
#endif
}

int config_get_attr(struct config *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter) {
    return attr_generic_get_attr(this_->attrs, NULL, type, attr, iter);
}

static int config_set_attr_int(struct config *this_, struct attr *attr) {
    switch (attr->type) {
    case attr_language:
        setenv("LANG",attr->u.str,1);
        return 1;
    case attr_cache_size:
        return file_set_cache_size(attr->u.num);
    default:
        return 0;
    }
}

int config_set_attr(struct config *this_, struct attr *attr) {
    if (config_set_attr_int(this_, attr))
        return navit_object_set_attr((struct navit_object *)this_, attr);
    else
        return 0;
}

int config_add_attr(struct config *this_, struct attr *attr) {
    if (!config) {
        config_new_int();
        this_=config;
    }
    return navit_object_add_attr((struct navit_object *)this_, attr);
}

int config_remove_attr(struct config *this_, struct attr *attr) {
    return navit_object_remove_attr((struct navit_object *)this_, attr);
}

struct attr_iter *
config_attr_iter_new() {
    return navit_object_attr_iter_new();
}

void config_attr_iter_destroy(struct attr_iter *iter) {
    navit_object_attr_iter_destroy(iter);
}


struct config *
config_new(struct attr *parent, struct attr **attrs) {
    if (configured) {
        dbg(lvl_error,"only one config allowed");
        return config;
    }
    if (parent) {
        dbg(lvl_error,"no parent in config allowed");
        return NULL;
    }
    if (!config)
        config_new_int();
    config->func=&config_func;
    navit_object_ref((struct navit_object *)config);
    config->attrs=attr_generic_add_attr_list(config->attrs, attrs);
    while (*attrs) {
        if (!config_set_attr_int(config,*attrs)) {
            dbg(lvl_error,"failed to set attribute '%s'",attr_to_name((*attrs)->type));
            config_destroy(config);
            config=NULL;
            break;
        }
        attrs++;
    }
    configured=1;
    return config;
}

struct object_func config_func = {
    attr_config,
    (object_func_new)config_new,
    (object_func_get_attr)navit_object_get_attr,
    (object_func_iter_new)navit_object_attr_iter_new,
    (object_func_iter_destroy)navit_object_attr_iter_destroy,
    (object_func_set_attr)config_set_attr,
    (object_func_add_attr)config_add_attr,
    (object_func_remove_attr)navit_object_remove_attr,
    (object_func_init)NULL,
    (object_func_destroy)NULL,
    (object_func_dup)NULL,
    (object_func_ref)navit_object_ref,
    (object_func_unref)navit_object_unref,
};
