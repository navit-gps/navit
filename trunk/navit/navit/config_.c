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
#include "debug.h"
#include "item.h"
#include "callback.h"
#include "config_.h"

struct config {
	struct attr **attrs;
	struct callback_list *cbl;
} *config;

int config_empty_ok;

struct attr_iter {
	void *iter;
};

void
config_destroy(struct config *this_)
{
	attr_list_free(this_->attrs);
	callback_list_destroy(this_->cbl);
	g_free(config);
	exit(0);
}

int
config_get_attr(struct config *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter)
{
	return attr_generic_get_attr(this_->attrs, NULL, type, attr, iter);
}

static int
config_set_attr_int(struct config *this_, struct attr *attr)
{
	switch (attr->type) {
	case attr_language:
		setenv("LANG",attr->u.str,1);
		return 1;
	default:
		return 0;
	}
}

int
config_set_attr(struct config *this_, struct attr *attr)
{
	return config_set_attr_int(this_, attr);
}

int
config_add_attr(struct config *this_, struct attr *attr)
{
	switch (attr->type) {
	case attr_callback:
		callback_list_add(this_->cbl, attr->u.callback);
	default:	
		this_->attrs=attr_generic_add_attr(this_->attrs, attr);
	}
	callback_list_call_attr_2(this_->cbl, attr->type, attr->u.data, 1);
	return 1;
}

int
config_remove_attr(struct config *this_, struct attr *attr)
{
	switch (attr->type) {
	case attr_callback:
		callback_list_remove(this_->cbl, attr->u.callback);
	default:	
		this_->attrs=attr_generic_remove_attr(this_->attrs, attr);
	}
	callback_list_call_attr_2(this_->cbl, attr->type, attr->u.data, -1);
	return 1;
}

struct attr_iter *
config_attr_iter_new()
{
	return g_new0(struct attr_iter, 1);
}

void
config_attr_iter_destroy(struct attr_iter *iter)
{
	g_free(iter);
}


struct config *
config_new(struct attr *parent, struct attr **attrs)
{
	if (config) {
		dbg(0,"only one config allowed\n");
		return NULL;
	}
	if (parent) {
		dbg(0,"no parent in config allowed\n");
		return NULL;
	}
	config=g_new0(struct config, 1);
	config->attrs=attr_list_dup(attrs);
	config->cbl=callback_list_new();
	while (*attrs) {
		if (!config_set_attr_int(config,*attrs)) {
			dbg(0,"failed to set attribute '%s'\n",attr_to_name((*attrs)->type));
			config_destroy(config);
			config=NULL;
			break;
		}
		attrs++;
	}
	return config;
}
