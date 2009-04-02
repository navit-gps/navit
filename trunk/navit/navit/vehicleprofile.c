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

#include <glib.h>
#include <string.h>
#include "debug.h"
#include "item.h"
#include "vehicleprofile.h"

struct vehicleprofile {
	struct attr **attrs;
};

struct vehicleprofile *
vehicleprofile_new(struct attr *parent, struct attr **attrs)
{
	struct vehicleprofile *this_;
	struct attr *type_attr;
	if (! (type_attr=attr_search(attrs, NULL, attr_name))) {
		return NULL;
	}
	this_=g_new0(struct vehicleprofile, 1);
	this_->attrs=attr_list_dup(attrs);
	return this_;
}

int
vehicleprofile_get_attr(struct vehicleprofile *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter)
{
	return attr_generic_get_attr(this_->attrs, NULL, type, attr, iter);
}

int
vehicleprofile_set_attr(struct vehicleprofile *this_, struct attr *attr)
{
	this_->attrs=attr_generic_set_attr(this_->attrs, attr);
	return 1;
}

int
vehicleprofile_add_attr(struct vehicleprofile *this_, struct attr *attr)
{
	this_->attrs=attr_generic_add_attr(this_->attrs, attr);
	return 1;
}

int
vehicleprofile_remove_attr(struct vehicleprofile *this_, struct attr *attr)
{
	this_->attrs=attr_generic_remove_attr(this_->attrs, attr);
	return 1;
}

