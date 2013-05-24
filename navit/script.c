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
#include "xmlconfig.h"

struct script
{
	NAVIT_OBJECT
	struct attr parent;
	struct callback *cb;
	struct event_timeout *timeout;
};

static void
script_run(struct script *scr)
{
	struct attr *xml_text=attr_search(scr->attrs, NULL, attr_xml_text);
	int error;
	if (!xml_text || !xml_text->u.str) {
		dbg(0,"no text\n");
		return;
	}
	dbg(0,"running '%s'\n",xml_text->u.str);
	command_evaluate_to_void(&scr->parent, xml_text->u.str, &error);
}

static int
script_set_attr_int(struct script *scr, struct attr *attr)
{
	switch (attr->type) {
	case attr_update_period:
		if (scr->timeout)
			event_remove_timeout(scr->timeout);
		scr->timeout=event_add_timeout(attr->u.num, 1, scr->cb);
		return 1;
	default:
		return 0;
	}
}

struct script *
script_new(struct attr *parent, struct attr **attrs)
{
	struct script *scr=g_new0(struct script, 1);
	scr->func=&script_func;
	navit_object_ref((struct navit_object *)scr);
	scr->attrs=attr_list_dup(attrs);
	attrs=scr->attrs;
	scr->cb=callback_new_1(callback_cast(script_run), scr);
	attr_dup_content(parent, &scr->parent);
	while (attrs && *attrs) 
		script_set_attr_int(scr, *attrs++);
	dbg(0,"return %p\n",scr);
	return scr;
}

void
script_destroy(struct script *scr)
{
	dbg(0,"enter %p\n",scr);
	if (scr->timeout)
		event_remove_timeout(scr->timeout);
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
