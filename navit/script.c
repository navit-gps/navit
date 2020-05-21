/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2012 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terscr of the GNU General Public License
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

#include <glib.h>
#include "item.h"
#include "debug.h"
#include "event.h"
#include "callback.h"
#include "command.h"
#include "xmlconfig.h"

struct script {
    NAVIT_OBJECT
    struct attr parent;
    struct callback *cb;
    struct event_timeout *timeout;
    struct command_saved *cs;
};

static void script_run(struct script *scr) {
    struct attr *xml_text=attr_search(scr->attrs, attr_xml_text);
    int error;
    if (!xml_text || !xml_text->u.str) {
        dbg(lvl_error,"no text");
        return;
    }
    dbg(lvl_debug,"running '%s'",xml_text->u.str);
    command_evaluate_to_void(&scr->parent, xml_text->u.str, &error);
}

static int script_set_attr_int(struct script *scr, struct attr *attr) {
    switch (attr->type) {
    case attr_refresh_cond:
        dbg(lvl_debug,"refresh_cond");
        if (scr->cs)
            command_saved_destroy(scr->cs);
        scr->cs=command_saved_attr_new(attr->u.str, &scr->parent, scr->cb, 0);
        return 1;
    case attr_update_period:
        if (scr->timeout)
            event_remove_timeout(scr->timeout);
        scr->timeout=event_add_timeout(attr->u.num, 1, scr->cb);
        return 1;
    default:
        return 0;
    }
}

static struct script *script_new(struct attr *parent, struct attr **attrs) {
    struct script *scr=g_new0(struct script, 1);
    scr->func=&script_func;
    navit_object_ref((struct navit_object *)scr);
    scr->attrs=attr_list_dup(attrs);
    attrs=scr->attrs;
    scr->cb=callback_new_1(callback_cast(script_run), scr);
    scr->parent=*parent;
    while (attrs && *attrs)
        script_set_attr_int(scr, *attrs++);
    dbg(lvl_debug,"return %p",scr);
    return scr;
}

static void script_destroy(struct script *scr) {
    dbg(lvl_debug,"enter %p",scr);
    if (scr->timeout)
        event_remove_timeout(scr->timeout);
    if (scr->cs)
        command_saved_destroy(scr->cs);
    callback_destroy(scr->cb);
    attr_list_free(scr->attrs);
    g_free(scr);
}

struct object_func script_func = {
    attr_script,
    (object_func_new)script_new,
    (object_func_get_attr)navit_object_get_attr,
    (object_func_iter_new)navit_object_attr_iter_new,
    (object_func_iter_destroy)navit_object_attr_iter_destroy,
    (object_func_set_attr)navit_object_set_attr,
    (object_func_add_attr)navit_object_add_attr,
    (object_func_remove_attr)navit_object_remove_attr,
    (object_func_init)NULL,
    (object_func_destroy)script_destroy,
    (object_func_dup)NULL,
    (object_func_ref)navit_object_ref,
    (object_func_unref)navit_object_unref,
};
