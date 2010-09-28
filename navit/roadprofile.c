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
#include "debug.h"
#include "item.h"
#include "roadprofile.h"

static void
roadprofile_set_attr_do(struct roadprofile *this, struct attr *attr)
{
	switch (attr->type) {
	case attr_speed:
		this->speed=attr->u.num;
		break;
	case attr_maxspeed:
		this->maxspeed=attr->u.num; 
		break; 
	case attr_route_weight:
		this->route_weight=attr->u.num;
		break;
	default:
		break;
	}
}

struct roadprofile *
roadprofile_new(struct attr *parent, struct attr **attrs)
{
	struct roadprofile *this_;
	struct attr **attr;
	this_=g_new0(struct roadprofile, 1);
	this_->attrs=attr_list_dup(attrs);
	this_->maxspeed=0;
	for (attr=attrs;*attr; attr++) 
		roadprofile_set_attr_do(this_, *attr);
	return this_;
}

int
roadprofile_get_attr(struct roadprofile *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter)
{
	return attr_generic_get_attr(this_->attrs, NULL, type, attr, iter);
}

int
roadprofile_set_attr(struct roadprofile *this_, struct attr *attr)
{
	roadprofile_set_attr_do(this_, attr);
	this_->attrs=attr_generic_set_attr(this_->attrs, attr);
	return 1;
}

int
roadprofile_add_attr(struct roadprofile *this_, struct attr *attr)
{
	this_->attrs=attr_generic_add_attr(this_->attrs, attr);
	return 1;
}

int
roadprofile_remove_attr(struct roadprofile *this_, struct attr *attr)
{
	this_->attrs=attr_generic_remove_attr(this_->attrs, attr);
	return 1;
}

