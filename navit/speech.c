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
#include "speech.h"
#include "plugin.h"
#include "xmlconfig.h"

struct speech {
    NAVIT_OBJECT;
    struct speech_priv *priv;
    struct speech_methods meth;
};


struct speech *
speech_new(struct attr *parent, struct attr **attrs) {
    struct speech *this_;
    struct speech_priv *(*speech_new)(struct speech_methods *meth, struct attr **attrs, struct attr *parent);
    struct attr *attr;

    attr=attr_search(attrs, NULL, attr_type);
    if (! attr) {
        dbg(lvl_error,"type missing");
        return NULL;
    }
    dbg(lvl_debug,"type='%s'", attr->u.str);
    speech_new=plugin_get_category_speech(attr->u.str);
    dbg(lvl_debug,"new=%p", speech_new);
    if (! speech_new) {
        dbg(lvl_error,"wrong type '%s'", attr->u.str);
        return NULL;
    }
    this_=(struct speech *)navit_object_new(attrs, &speech_func, sizeof(struct speech));
    this_->priv=speech_new(&this_->meth, this_->attrs, parent);
    dbg(lvl_debug, "say=%p", this_->meth.say);
    dbg(lvl_debug,"priv=%p", this_->priv);
    if (! this_->priv) {
        speech_destroy(this_);
        return NULL;
    }
    dbg(lvl_debug,"return %p", this_);

    return this_;
}

void speech_destroy(struct speech *this_) {
    if (this_->priv)
        this_->meth.destroy(this_->priv);
    navit_object_destroy((struct navit_object *)this_);
}

int speech_say(struct speech *this_, const char *text) {
    dbg(lvl_debug, "this_=%p text='%s' calling %p", this_, text, this_->meth.say);
    return (this_->meth.say)(this_->priv, text);
}

struct attr active=ATTR_INT(active, 1);
struct attr *speech_default_attrs[]= {
    &active,
    NULL,
};

/**
 * @brief Gets an attribute from a speech plugin
 *
 * @param this_ The speech plugin the attribute should be read from
 * @param type The type of the attribute to be read
 * @param attr Pointer to an attrib-structure where the attribute should be written to
 * @param iter (NOT IMPLEMENTED) Used to iterate through all attributes of a type. Set this to NULL to get the first attribute, set this to an attr_iter to get the next attribute
 * @return True if the attribute type was found, false if not
 */

int speech_get_attr(struct speech *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter) {
    return attr_generic_get_attr(this_->attrs, speech_default_attrs, type, attr, iter);
}

/**
 * @brief Tries to estimate how long it will take to speak a certain string
 *
 * This function tries to estimate how long it will take to speak a certain string
 * passed in str. It relies on the "characters per second"-value passed from the
 * configuration.
 *
 * @param this_ The speech whose speed should be used
 * @param str The string that should be estimated
 * @return Time in tenth of seconds or -1 on error
 */
int speech_estimate_duration(struct speech *this_, char *str) {
    int count;
    struct attr cps_attr;

    if (!speech_get_attr(this_,attr_cps,&cps_attr,NULL)) {
        return -1;
    }

    count = strlen(str);

    return (count * 10) / cps_attr.u.num;
}

/**
 * @brief Sets an attribute from an speech plugin
 *
 * This sets an attribute of a speech plugin, overwriting an attribute of the same type if it
 * already exists. This function also calls all the callbacks that are registred
 * to be called when attributes change.
 *
 * @param this_ The speech plugin to set the attribute of
 * @param attr The attribute to set
 * @return True if the attr could be set, false otherwise
 */

int speech_set_attr(struct speech *this_, struct attr *attr) {
    this_->attrs=attr_generic_set_attr(this_->attrs, attr);
    //callback_list_call_attr_2(this_->attr_cbl, attr->type, this_, attr);
    return 1;
}

struct object_func speech_func = {
    attr_speech,
    (object_func_new)speech_new,
    (object_func_get_attr)speech_get_attr,
    (object_func_iter_new)navit_object_attr_iter_new,
    (object_func_iter_destroy)navit_object_attr_iter_destroy,
    (object_func_set_attr)speech_set_attr,
    (object_func_add_attr)navit_object_add_attr,
    (object_func_remove_attr)navit_object_remove_attr,
    (object_func_init)NULL,
    (object_func_destroy)speech_destroy,
    (object_func_dup)NULL,
    (object_func_ref)navit_object_ref,
    (object_func_unref)navit_object_unref,
};

