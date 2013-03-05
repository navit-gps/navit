/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2012 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terpo of the GNU General Public License
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
#include "xmlconfig.h"

struct profile_option
{
	NAVIT_OBJECT
};

struct profile_option *
profile_option_new(struct attr *parent, struct attr **attrs)
{
	struct profile_option *po=g_new0(struct profile_option, 1);
	po->func=&profile_option_func;
	navit_object_ref((struct navit_object *)po);
	po->attrs=attr_list_dup(attrs);
	return po;
}

void
profile_option_destroy(struct profile_option *po)
{
	attr_list_free(po->attrs);
	g_free(po);
}

struct object_func profile_option_func = {
	attr_profile_option,
	(object_func_new)profile_option_new,
	(object_func_get_attr)NULL,
	(object_func_iter_new)NULL,
	(object_func_iter_destroy)NULL,
	(object_func_set_attr)NULL,
	(object_func_add_attr)NULL,
	(object_func_remove_attr)NULL,
	(object_func_init)NULL,
	(object_func_destroy)profile_option_destroy,
	(object_func_dup)NULL,
	(object_func_ref)navit_object_ref,
	(object_func_unref)navit_object_unref,
};
