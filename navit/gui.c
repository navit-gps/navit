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
#include "callback.h"
#include "gui.h"
#include "menu.h"
#include "data_window.h"
#include "item.h"
#include "plugin.h"

struct gui {
    struct gui_methods meth;
    struct gui_priv *priv;
    struct attr **attrs;
    struct attr parent;
};

struct gui *
gui_new(struct attr *parent, struct attr **attrs) {
    struct gui *this_;
    struct attr *type_attr;
    struct gui_priv *(*guitype_new)(struct navit *nav, struct gui_methods *meth, struct attr **attrs, struct gui *gui);
    struct attr cbl;
    if (! (type_attr=attr_search(attrs, attr_type))) {
        return NULL;
    }

    guitype_new=plugin_get_category_gui(type_attr->u.str);
    if (! guitype_new)
        return NULL;

    this_=g_new0(struct gui, 1);
    this_->attrs=attr_list_dup(attrs);
    cbl.type=attr_callback_list;
    cbl.u.callback_list=callback_list_new();
    this_->attrs=attr_generic_add_attr(this_->attrs, &cbl);
    this_->priv=guitype_new(parent->u.navit, &this_->meth, this_->attrs, this_);
    this_->parent=*parent;
    return this_;
}

int gui_get_attr(struct gui *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter) {
    int ret;
    if (this_->meth.get_attr) {
        ret=this_->meth.get_attr(this_->priv, type, attr);
        if (ret)
            return ret;
    }
    if (type == this_->parent.type) {
        *attr=this_->parent;
        return 1;
    }
    return attr_generic_get_attr(this_->attrs, NULL, type, attr, iter);
}


int gui_set_attr(struct gui *this_, struct attr *attr) {
    int ret=1;
    if (this_->meth.set_attr)
        ret=this_->meth.set_attr(this_->priv, attr);
    if (ret == 1)
        this_->attrs=attr_generic_set_attr(this_->attrs, attr);
    return ret != 0;
}

int gui_add_attr(struct gui *this_, struct attr *attr) {
    int ret=0;
    if (this_->meth.add_attr)
        ret=this_->meth.add_attr(this_->priv, attr);
    return ret;
}

struct menu *
gui_menubar_new(struct gui *gui) {
    struct menu *this_;
    if (! gui->meth.menubar_new)
        return NULL;
    this_=g_new0(struct menu, 1);
    this_->priv=gui->meth.menubar_new(gui->priv, &this_->meth);
    if (! this_->priv) {
        g_free(this_);
        return NULL;
    }
    return this_;
}

struct menu *
gui_popup_new(struct gui *gui) {
    struct menu *this_;
    if (!gui || ! gui->meth.popup_new)
        return NULL;
    this_=g_new0(struct menu, 1);
    this_->priv=gui->meth.popup_new(gui->priv, &this_->meth);
    if (! this_->priv) {
        g_free(this_);
        return NULL;
    }
    return this_;
}

struct datawindow *
gui_datawindow_new(struct gui *gui, const char *name, struct callback *click, struct callback *close) {
    struct datawindow *this_;
    if (! gui->meth.datawindow_new)
        return NULL;
    this_=g_new0(struct datawindow, 1);
    this_->priv=gui->meth.datawindow_new(gui->priv, name, click, close, &this_->meth);
    if (! this_->priv) {
        g_free(this_);
        return NULL;
    }
    return this_;
}

int gui_add_bookmark(struct gui *gui, struct pcoord *c, char *description) {
    int ret;
    dbg(lvl_info,"enter");
    if (! gui->meth.add_bookmark)
        return 0;
    ret=gui->meth.add_bookmark(gui->priv, c, description);

    dbg(lvl_info,"ret=%d", ret);
    return ret;
}

int gui_set_graphics(struct gui *this_, struct graphics *gra) {
    if (! this_->meth.set_graphics) {
        dbg(lvl_error, "cannot set graphics, method 'set_graphics' not available");
        return 1;
    }
    return this_->meth.set_graphics(this_->priv, gra);
}

void gui_disable_suspend(struct gui *this_) {
    if (this_->meth.disable_suspend)
        this_->meth.disable_suspend(this_->priv);
}

int gui_has_main_loop(struct gui *this_) {
    if (! this_->meth.run_main_loop)
        return 0;
    return 1;
}

int gui_run_main_loop(struct gui *this_) {
    if (! gui_has_main_loop(this_))
        return 1;
    return this_->meth.run_main_loop(this_->priv);
}

