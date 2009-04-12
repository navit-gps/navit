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

void
vehicleprofile_set_attr_do(struct vehicleprofile *this_, struct attr *attr)
{
	switch (attr->type) {
	case attr_flags:
		this_->flags=attr->u.num;
		break;
	case attr_flags_forward_mask:
		this_->flags_forward_mask=attr->u.num;
		break;
	case attr_flags_reverse_mask:
		this_->flags_forward_mask=attr->u.num;
		break;
	case attr_maxspeed_handling:
		this_->maxspeed_handling=attr->u.num;
		break;
	case attr_route_mode:
		this_->mode=attr->u.num;
		break;
	default:
		break;
	}
}

struct vehicleprofile *
vehicleprofile_new(struct attr *parent, struct attr **attrs)
{
	struct vehicleprofile *this_;
	struct attr **attr, *type_attr;
	if (! (type_attr=attr_search(attrs, NULL, attr_name))) {
		return NULL;
	}
	this_=g_new0(struct vehicleprofile, 1);
	this_->attrs=attr_list_dup(attrs);
	this_->roadprofile_hash=g_hash_table_new(NULL, NULL);
	for (attr=attrs;*attr; attr++)
		vehicleprofile_set_attr_do(this_, *attr);
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
	vehicleprofile_set_attr_do(this_, attr);
	this_->attrs=attr_generic_set_attr(this_->attrs, attr);
	return 1;
}

int
vehicleprofile_add_attr(struct vehicleprofile *this_, struct attr *attr)
{
	this_->attrs=attr_generic_add_attr(this_->attrs, attr);
	struct attr item_types_attr;
	switch (attr->type) {
	case attr_roadprofile:
		if (roadprofile_get_attr(attr->u.roadprofile, attr_item_types, &item_types_attr)) {
			enum item_type *types=item_types_attr.u.item_types;
			while (*types != type_none) {
				g_hash_table_insert(this_->roadprofile_hash, (void *)(long)(*types), attr->u.roadprofile);
				types++;
			}
			dbg(0,"ok\n");
		}
		break;
	default:
		break;
	}
	return 1;
}

int
vehicleprofile_remove_attr(struct vehicleprofile *this_, struct attr *attr)
{
	this_->attrs=attr_generic_remove_attr(this_->attrs, attr);
	return 1;
}

struct roadprofile_data *
vehicleprofile_get_roadprofile(struct vehicleprofile *this_, enum item_type type)
{
	return g_hash_table_lookup(this_->roadprofile_hash, (void *)(long)type);
}
