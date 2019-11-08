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
#include "xmlconfig.h"
#include "roadprofile.h"

static void roadprofile_set_attr_do(struct roadprofile *this, struct attr *attr) {
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
roadprofile_new(struct attr *parent, struct attr **attrs) {
    struct roadprofile *this_;
    struct attr **attr;
    this_=g_new0(struct roadprofile, 1);
    this_->func=&roadprofile_func;
    navit_object_ref((struct navit_object *)this_);

    this_->attrs=attr_list_dup(attrs);
    for (attr=attrs; *attr; attr++)
        roadprofile_set_attr_do(this_, *attr);
    return this_;
}

int roadprofile_get_attr(struct roadprofile *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter) {
    return attr_generic_get_attr(this_->attrs, NULL, type, attr, iter);
}

int roadprofile_set_attr(struct roadprofile *this_, struct attr *attr) {
    roadprofile_set_attr_do(this_, attr);
    this_->attrs=attr_generic_set_attr(this_->attrs, attr);
    return 1;
}

int roadprofile_add_attr(struct roadprofile *this_, struct attr *attr) {
    this_->attrs=attr_generic_add_attr(this_->attrs, attr);
    return 1;
}

int roadprofile_remove_attr(struct roadprofile *this_, struct attr *attr) {
    this_->attrs=attr_generic_remove_attr(this_->attrs, attr);
    return 1;
}

struct attr_iter *
roadprofile_attr_iter_new(void * unused) {
    return (struct attr_iter *)g_new0(void *,1);
}

void roadprofile_attr_iter_destroy(struct attr_iter *iter) {
    g_free(iter);
}

static struct roadprofile *roadprofile_dup(struct roadprofile *this_) {
    struct roadprofile *ret=g_new(struct roadprofile, 1);
    *ret=*this_;
    ret->refcount=1;
    ret->attrs=attr_list_dup(this_->attrs);
    return ret;
}

static void roadprofile_destroy(struct roadprofile *this_) {
    attr_list_free(this_->attrs);
    g_free(this_);
}


struct object_func roadprofile_func = {
    attr_roadprofile,
    (object_func_new)roadprofile_new,
    (object_func_get_attr)roadprofile_get_attr,
    (object_func_iter_new)roadprofile_attr_iter_new,
    (object_func_iter_destroy)roadprofile_attr_iter_destroy,
    (object_func_set_attr)roadprofile_set_attr,
    (object_func_add_attr)roadprofile_add_attr,
    (object_func_remove_attr)roadprofile_remove_attr,
    (object_func_init)NULL,
    (object_func_destroy)roadprofile_destroy,
    (object_func_dup)roadprofile_dup,
    (object_func_ref)navit_object_ref,
    (object_func_unref)navit_object_unref,
};
