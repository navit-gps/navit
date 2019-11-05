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
#include <string.h>
#include "debug.h"
#include "item.h"
#include "xmlconfig.h"
#include "roadprofile.h"
#include "vehicleprofile.h"
#include "callback.h"

static void vehicleprofile_set_attr_do(struct vehicleprofile *this_, struct attr *attr) {
    dbg(lvl_debug,"%s:%ld", attr_to_name(attr->type), attr->u.num);
    switch (attr->type) {
    case attr_flags:
        this_->flags=attr->u.num;
        break;
    case attr_flags_forward_mask:
        this_->flags_forward_mask=attr->u.num;
        break;
    case attr_flags_reverse_mask:
        this_->flags_reverse_mask=attr->u.num;
        break;
    case attr_maxspeed_handling:
        this_->maxspeed_handling=attr->u.num;
        break;
    case attr_route_mode:
        this_->mode=attr->u.num;
        break;
    case attr_name:
        if(this_->name)
            g_free(this_->name);
        /* previously used strdupn not available on win32 */
        this_->name = g_strdup(attr->u.str);
        break;
    case attr_route_depth:
        if(this_->route_depth)
            g_free(this_->route_depth);
        this_->route_depth = g_strdup(attr->u.str);
        break;
    case attr_vehicle_axle_weight:
        this_->axle_weight=attr->u.num;
        break;
    case attr_vehicle_dangerous_goods:
        this_->dangerous_goods=attr->u.num;
        break;
    case attr_vehicle_height:
        this_->height=attr->u.num;
        break;
    case attr_vehicle_length:
        this_->length=attr->u.num;
        break;
    case attr_vehicle_weight:
        this_->weight=attr->u.num;
        break;
    case attr_vehicle_width:
        this_->width=attr->u.num;
        break;
    case attr_static_speed:
        this_->static_speed=attr->u.num;
        break;
    case attr_static_distance:
        this_->static_distance=attr->u.num;
        break;
    case attr_through_traffic_penalty:
        this_->through_traffic_penalty=attr->u.num;
        break;
    case attr_turn_around_penalty:
        this_->turn_around_penalty=attr->u.num;
        break;
    case attr_turn_around_penalty2:
        this_->turn_around_penalty2=attr->u.num;
        break;
    default:
        break;
    }
}

static void vehicleprofile_free_hash_item(gpointer key, gpointer value, gpointer user_data) {
    struct navit_object *obj=value;
    obj->func->unref(obj);
}

static void vehicleprofile_free_hash(struct vehicleprofile *this_) {
    if (this_->roadprofile_hash) {
        g_hash_table_foreach(this_->roadprofile_hash, vehicleprofile_free_hash_item, NULL);
        g_hash_table_destroy(this_->roadprofile_hash);
    }
}

static void vehicleprofile_clear(struct vehicleprofile *this_) {
    this_->mode=0;
    this_->flags_forward_mask=0;
    this_->flags_reverse_mask=0;
    this_->flags=0;
    this_->maxspeed_handling = maxspeed_enforce;
    this_->static_speed=0;
    this_->static_distance=0;
    g_free(this_->name);
    this_->name=NULL;
    g_free(this_->route_depth);
    this_->route_depth=NULL;
    this_->dangerous_goods=0;
    this_->length=-1;
    this_->width=-1;
    this_->height=-1;
    this_->weight=-1;
    this_->axle_weight=-1;
    this_->through_traffic_penalty=9000;
    vehicleprofile_free_hash(this_);
    this_->roadprofile_hash=g_hash_table_new(NULL, NULL);
}

static void vehicleprofile_apply_roadprofile(struct vehicleprofile *this_, struct navit_object *rp, int is_option) {
    struct attr item_types_attr;
    if (rp->func->get_attr(rp, attr_item_types, &item_types_attr, NULL)) {
        enum item_type *types=item_types_attr.u.item_types;
        while (*types != type_none) {
            struct navit_object *oldrp;
            /* Maptool won't place any access flags for roads which don't have default access flags set. Warn user. */
            if(!item_get_default_flags(*types))
                dbg(lvl_error,
                    "On '%s' roads used in '%s' vehicleprofile access restrictions are ignored. You might even be directed to drive in wrong direction on a one-way road. "
                    "Please define default access flags for above road type to item.c and rebuild the map.\n", item_to_name(*types),
                    this_->name);
            oldrp=g_hash_table_lookup(this_->roadprofile_hash, (void *)(long)(*types));
            if (is_option && oldrp) {
                struct navit_object *newrp;
                struct attr_iter *iter=rp->func->iter_new(NULL);
                struct attr attr;
                dbg(lvl_debug,"patching roadprofile");
                newrp=oldrp->func->dup(oldrp);
                while (rp->func->get_attr(rp, attr_any, &attr, iter))
                    newrp->func->set_attr(newrp, &attr);
                oldrp->func->iter_destroy(iter);
                oldrp->func->unref(oldrp);
                g_hash_table_insert(this_->roadprofile_hash, (void *)(long)(*types), newrp);
            } else {
                if (oldrp)
                    oldrp->func->unref(oldrp);
                g_hash_table_insert(this_->roadprofile_hash, (void *)(long)(*types), rp->func->ref(rp));
            }
            types++;
        }
    }
}

static void vehicleprofile_apply_attrs(struct vehicleprofile *this_, struct navit_object *obj, int is_option) {
    struct attr attr;
    struct attr_iter *iter=obj->func->iter_new(NULL);
    while (obj->func->get_attr(obj, attr_any, &attr, iter)) {
        dbg(lvl_debug,"%s",attr_to_name(attr.type));
        if (attr.type == attr_roadprofile)
            vehicleprofile_apply_roadprofile(this_, attr.u.navit_object, is_option);
        else if (attr.type != attr_profile_option)
            vehicleprofile_set_attr_do(this_, &attr);
    }
    obj->func->iter_destroy(iter);
}

static void vehicleprofile_debug_roadprofile(gpointer key, gpointer value, gpointer user_data) {
    struct roadprofile *rp=value;
    dbg(lvl_debug,"type %s avg %d weight %d max %d",item_to_name((int)(long)key),rp->speed,rp->route_weight,rp->maxspeed);
}

static void vehicleprofile_update(struct vehicleprofile *this_) {
    struct attr_iter *iter=vehicleprofile_attr_iter_new(NULL);
    struct attr profile_option;
    dbg(lvl_debug,"enter");
    vehicleprofile_clear(this_);
    vehicleprofile_apply_attrs(this_, (struct navit_object *)this_, 0);
    while (vehicleprofile_get_attr(this_, attr_profile_option, &profile_option, iter)) {
        struct attr active, name;
        if (!profile_option.u.navit_object->func->get_attr(profile_option.u.navit_object, attr_active, &active, NULL))
            active.u.num=0;
        if (profile_option.u.navit_object->func->get_attr(profile_option.u.navit_object, attr_name, &name, NULL))
            dbg(lvl_debug,"%p %s %ld",profile_option.u.navit_object,name.u.str,active.u.num);
        if (active.u.num)
            vehicleprofile_apply_attrs(this_, profile_option.u.navit_object, 1);
    }
    vehicleprofile_attr_iter_destroy(iter);
    dbg(lvl_debug,"result l %d w %d h %d wg %d awg %d pen %d",this_->length,this_->width,this_->height,this_->weight,
        this_->axle_weight,this_->through_traffic_penalty);
    dbg(lvl_debug,"m %d fwd 0x%x rev 0x%x flags 0x%x max %d stsp %d stdst %d dg %d",this_->mode,this_->flags_forward_mask,
        this_->flags_reverse_mask, this_->flags, this_->maxspeed_handling, this_->static_speed, this_->static_distance,
        this_->dangerous_goods);
    g_hash_table_foreach(this_->roadprofile_hash, vehicleprofile_debug_roadprofile, NULL);

}


struct vehicleprofile *
vehicleprofile_new(struct attr *parent, struct attr **attrs) {
    struct vehicleprofile *this_;
    struct attr **attr, *type_attr;
    if (! (type_attr=attr_search(attrs, NULL, attr_name))) {
        return NULL;
    }
    this_=g_new0(struct vehicleprofile, 1);
    this_->func=&vehicleprofile_func;
    navit_object_ref((struct navit_object *)this_);
    this_->attrs=attr_list_dup(attrs);
    this_->active_callback.type=attr_callback;
    this_->active_callback.u.callback=callback_new_attr_1(callback_cast(vehicleprofile_update), attr_active, this_);
    vehicleprofile_clear(this_);
    for (attr=attrs; *attr; attr++)
        vehicleprofile_set_attr_do(this_, *attr);
    return this_;
}

struct attr_iter *
vehicleprofile_attr_iter_new(void* unused) {
    return (struct attr_iter *)g_new0(void *,1);
}

void vehicleprofile_attr_iter_destroy(struct attr_iter *iter) {
    g_free(iter);
}


int vehicleprofile_get_attr(struct vehicleprofile *this_, enum attr_type type, struct attr *attr,
                            struct attr_iter *iter) {
    return attr_generic_get_attr(this_->attrs, NULL, type, attr, iter);
}

int vehicleprofile_set_attr(struct vehicleprofile *this_, struct attr *attr) {
    vehicleprofile_set_attr_do(this_, attr);
    this_->attrs=attr_generic_set_attr(this_->attrs, attr);
    return 1;
}

int vehicleprofile_add_attr(struct vehicleprofile *this_, struct attr *attr) {
    this_->attrs=attr_generic_add_attr(this_->attrs, attr);
    switch (attr->type) {
    case attr_roadprofile:
        vehicleprofile_apply_roadprofile(this_, attr->u.navit_object, 0);
        break;
    case attr_profile_option:
        attr->u.navit_object->func->add_attr(attr->u.navit_object, &this_->active_callback);
        break;
    default:
        break;
    }
    return 1;
}

int vehicleprofile_remove_attr(struct vehicleprofile *this_, struct attr *attr) {
    this_->attrs=attr_generic_remove_attr(this_->attrs, attr);
    return 1;
}

struct roadprofile *
vehicleprofile_get_roadprofile(struct vehicleprofile *this_, enum item_type type) {
    return g_hash_table_lookup(this_->roadprofile_hash, (void *)(long)type);
}

char *vehicleprofile_get_name(struct vehicleprofile *this_) {
    return this_->name;
}

static int vehicleprofile_init(struct vehicleprofile *this_) {
    vehicleprofile_update(this_);
    return 0;
}

struct object_func vehicleprofile_func = {
    attr_vehicleprofile,
    (object_func_new)vehicleprofile_new,
    (object_func_get_attr)vehicleprofile_get_attr,
    (object_func_iter_new)vehicleprofile_attr_iter_new,
    (object_func_iter_destroy)vehicleprofile_attr_iter_destroy,
    (object_func_set_attr)vehicleprofile_set_attr,
    (object_func_add_attr)vehicleprofile_add_attr,
    (object_func_remove_attr)vehicleprofile_remove_attr,
    (object_func_init)vehicleprofile_init,
    (object_func_destroy)NULL,
    (object_func_dup)NULL,
    (object_func_ref)navit_object_ref,
    (object_func_unref)navit_object_unref,
};
