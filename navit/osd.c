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
#include "plugin.h"
#include "item.h"
#include "xmlconfig.h"
#include "color.h"
#include "point.h"
#include "navit.h"
#include "graphics.h"
#include "command.h"
#include "callback.h"
#include "osd.h"


struct osd {
    NAVIT_OBJECT
    struct osd_methods meth;
    struct osd_priv *priv;
};

int osd_set_methods(struct osd_methods *in, int in_size, struct osd_methods *out) {
    return navit_object_set_methods(in, in_size, out, sizeof(struct osd_methods));
}

struct osd *
osd_new(struct attr *parent, struct attr **attrs) {
    struct osd *o;
    struct osd_priv *(*new)(struct navit *nav, struct osd_methods *meth, struct attr **attrs);
    struct attr *type=attr_search(attrs, NULL, attr_type),cbl;

    if (! type)
        return NULL;
    new=plugin_get_category_osd(type->u.str);
    if (! new) {
        dbg(lvl_error, "invalid OSD type '%s'", type->u.str);
        return NULL;
    }
    o=g_new0(struct osd, 1);
    o->attrs=attr_list_dup(attrs);
    cbl.type=attr_callback_list;
    cbl.u.callback_list=callback_list_new();
    o->attrs=attr_generic_prepend_attr(o->attrs, &cbl);

    o->priv=new(parent->u.navit, &o->meth, o->attrs);
    if (o->priv) {
        o->func=&osd_func;
        navit_object_ref((struct navit_object *)o);
    } else {
        attr_list_free(o->attrs);
        g_free(o);
        o=NULL;
    }
    dbg(lvl_debug,"new osd %p",o);
    return o;
}

int osd_get_attr(struct osd *osd, enum attr_type type, struct attr *attr, struct attr_iter *iter) {
    int ret=0;
    if(osd && osd->meth.get_attr)
        /* values for ret: -1: Not possible, 0: Ignored by driver, 1 valid */
        ret=osd->meth.get_attr(osd->priv, type, attr);
    if (ret == -1)
        return 0;
    if (ret > 0)
        return 1;
    return navit_object_get_attr((struct navit_object *)osd, type, attr, iter);
}

int osd_set_attr(struct osd *osd, struct attr* attr) {
    int ret=0;
    if(osd && osd->meth.set_attr)
        /* values for ret: -1: Not possible, 0: Ignored by driver, 1 set and store, 2 set, don't store */
        ret=osd->meth.set_attr(osd->priv, attr);
    if (ret == -1)
        return 0;
    if (ret == 2)
        return 1;
    return navit_object_set_attr((struct navit_object *)osd, attr);
}

static void osd_destroy(struct osd *osd) {
    if (!osd)
        return;
    if (osd->meth.destroy)
        osd->meth.destroy(osd->priv);
    attr_list_free(osd->attrs);
    g_free(osd);
}

void osd_wrap_point(struct point *p, struct navit *nav) {
    if (p->x < 0)
        p->x += navit_get_width(nav);
    if (p->y < 0)
        p->y += navit_get_height(nav);

}

static void osd_evaluate_command(struct osd_item *this, struct navit *nav) {
    struct attr navit;
    navit.type=attr_navit;
    navit.u.navit=nav;
    dbg(lvl_debug, "calling command '%s'", this->command);
    command_evaluate(&navit, this->command);
}

void osd_std_click(struct osd_item *this, struct navit *nav, int pressed, int button, struct point *p) {
    int click_is_outside_item;
    struct point bp = this->p;
    if (!this->command || !this->command[0])
        return;
    osd_wrap_point(&bp, nav);
    click_is_outside_item = p->x < bp.x || p->y < bp.y || p->x > bp.x + this->w || p->y > bp.y + this->h;
    if ((click_is_outside_item || !this->configured) && !this->pressed)
        return;
    if (button != 1)
        return;
    if (!!pressed == !!this->pressed)
        return;
    if (navit_ignore_button(nav))
        return;
    this->pressed = pressed;
    if (pressed && this->command)
        osd_evaluate_command(this, nav);
}

void osd_std_resize(struct osd_item *item) {
    graphics_overlay_resize(item->gr, &item->p, item->w, item->h, 1);
}

/**
 * @brief Calculates the size and position of an OSD item.
 *
 * If the geometry of the OSD item is specified relative to screen dimensions,
 * this function will set its absolute dimensions accordingly. If relative width
 * or relative height is set to 0% (int value is equal to ATTR_REL_RELSHIFT),
 * object width (height) is not changed here, for button and image osds it means
 * to derive values from the underlying image.
 *
 * This method considers padding if the graphics plugin supports it (i.e. its `get_data` method returns
 * a valid pointer if `"padding"` is supplied as its arument): It will offset the origin of the item by
 * the amount of padding in the left and top edges, and will reduce `w` and `h` by the total amount of
 * padding in the respective dimension to obtain the equivalent of 100%.
 *
 * If the graphics driver does not support padding, none of these corrections take place (this is
 * equivalent to 0 padding on all sides).
 *
 * @param item The item whose size and position are to be calculated
 * @param w Available screen width in pixels
 * @param h Available screen height in pixels
 */
void osd_std_calculate_sizes(struct osd_item *item, int w, int h) {
    struct padding *padding = NULL;

    if (item->gr) {
        padding = graphics_get_data(item->gr, "padding");
        if (padding) {
            dbg(lvl_debug, "Got padding=%p for item=%p (item->gr=%p): left=%d top=%d right=%d bottom=%d",
                padding, item, item->gr, padding->left, padding->top, padding->right, padding->bottom);
        } else {
            dbg(lvl_debug, "Got padding=%p for item=%p (item->gr=%p)",
                padding, item, item->gr);
        }
    } else
        dbg(lvl_warning, "cannot get padding for item=%p: item->gr is NULL", item);

    /* reduce w and h by total padding in the respective dimension */
    if (padding) {
        w -= (padding->left + padding->right);
        h -= (padding->top + padding->bottom);
    }

    if(item->rel_w!=ATTR_REL_RELSHIFT)
        item->w=attr_rel2real(item->rel_w, w, 1);
    if(item->w<0)
        item->w=0;
    if(item->rel_h!=ATTR_REL_RELSHIFT)
        item->h=attr_rel2real(item->rel_h, h, 1);
    if(item->h<0)
        item->h=0;
    item->p.x=attr_rel2real(item->rel_x, w, 1);
    item->p.y=attr_rel2real(item->rel_y, h, 1);

    /* add left and top padding to item->p */
    if (padding) {
        item->p.x += padding->left;
        item->p.y += padding->top;
    }
}

/**
 * @brief Recalculates the size and position of an OSD item and
 * triggers a redraw of the item.
 *
 * This is a callback function that can be stored in the `resize_cb` member of an OSD item.
 *
 * @param item The OSD item to resize and redraw
 * @param priv
 * @param w Available screen width in pixels (the width that corresponds to
 * 100%)
 * @param h Available screen height in pixels (the height that corresponds to
 * 100%)
 */
static void osd_std_calculate_sizes_and_redraw(struct osd_item *item, struct osd_priv *priv, int w, int h) {
    struct attr vehicle_attr;

    osd_std_calculate_sizes(item, w, h);

    osd_std_resize(item);
    item->do_draw=1;
    if (item->meth.draw) {
        if (navit_get_attr(item->navit, attr_vehicle, &vehicle_attr, NULL)) {
            item->meth.draw(priv, item->navit, vehicle_attr.u.vehicle);
            item->do_draw=0;
        }
    }
}

static void osd_std_keypress(struct osd_item *item, struct navit *nav, char *key) {
#if 0
    int i;
    dbg(lvl_debug,"key=%s",key);
    for (i = 0 ; i < strlen(key) ; i++) {
        dbg(lvl_debug,"key:0x%02x",key[i]);
    }
    for (i = 0 ; i < strlen(item->accesskey) ; i++) {
        dbg(lvl_debug,"accesskey:0x%02x",item->accesskey[i]);
    }
#endif
    if ( ! graphics_is_disabled(item->gr) && item->accesskey && key && !strcmp(key, item->accesskey))
        osd_evaluate_command(item, nav);
}

/**
 * @brief Configures or unconfigures an OSD item.
 *
 * This method evaluates the result of the last execution of {@code cs}. If it evaluates to true, the
 * item is configured, else it is unconfigured. (A configured item is displayed on the screen and can
 * respond to user input, an unconfigured item is invisible and cannot receive user input.)
 *
 * If an error occurred during evaluation of {@code cs}, the item's configuration state is not changed.
 *
 * @param item The OSD item to reconfigure
 * @param cs The command to evaluate
 */
static void osd_std_reconfigure(struct osd_item *item, struct command_saved *cs) {
    char *err = NULL;	/* Error description */

    dbg(lvl_debug, "enter, item=%p, cs=%p", item, cs);
    if (!command_saved_error(cs)) {
        item->configured = !! command_saved_get_int(cs);
        if (item->gr && !(item->flags & DISABLE_OVERLAY))
            graphics_overlay_disable(item->gr, !item->configured);
    } else {
        err = command_error_to_text(command_saved_error(cs));
        dbg(lvl_error, "Error in saved command: %s, cs=%p.", err, cs);
        g_free(err);
    }
}

void osd_set_std_attr(struct attr **attrs, struct osd_item *item, int flags) {
    struct attr *attr;
    item->flags=flags;
    item->osd_configuration=-1;
    item->color_fg.r = 0xffff;
    item->color_fg.g = 0xffff;
    item->color_fg.b = 0xffff;
    item->color_fg.a = 0xffff;
    item->text_color.r = 0xffff;
    item->text_color.g = 0xffff;
    item->text_color.b = 0xffff;
    item->text_color.a = 0xffff;

    if (flags & TRANSPARENT_BG) {
        item->color_bg.r = 0x0808;
        item->color_bg.g = 0x0808;
        item->color_bg.b = 0xf8f8;
        item->color_bg.a = 0x0000;
    } else {
        item->color_bg.r = 0x0;
        item->color_bg.g = 0x0;
        item->color_bg.b = 0x0;
        item->color_bg.a = 0x5fff;
    }

    attr=attr_search(attrs, NULL, attr_osd_configuration);
    if (attr)
        item->osd_configuration = attr->u.num;

    attr=attr_search(attrs, NULL, attr_enable_expression);
    if (attr) {
        item->enable_cs = command_saved_new(attr->u.str, item->navit, NULL, 0);
    }

    attr = attr_search(attrs, NULL, attr_w);
    if (attr) {
        item->rel_w = attr->u.num;
    }

    attr = attr_search(attrs, NULL, attr_h);
    if (attr) {
        item->rel_h = attr->u.num;
    }

    attr = attr_search(attrs, NULL, attr_x);
    if (attr) {
        item->rel_x = attr->u.num;
    }

    attr = attr_search(attrs, NULL, attr_y);
    if (attr) {
        item->rel_y = attr->u.num;
    }

    attr = attr_search(attrs, NULL, attr_font_size);
    if (attr)
        item->font_size = attr->u.num;

    attr=attr_search(attrs, NULL, attr_background_color);
    if (attr)
        item->color_bg=*attr->u.color;
    attr = attr_search(attrs, NULL, attr_command);
    if (attr)
        item->command = g_strdup(attr->u.str);
    attr=attr_search(attrs, NULL, attr_text_color);
    if (attr)
        item->text_color=*attr->u.color;
    attr=attr_search(attrs, NULL, attr_foreground_color);
    if (attr)
        item->color_fg=*attr->u.color;
    attr=attr_search(attrs, NULL, attr_accesskey);
    if (attr)
        item->accesskey = g_strdup(attr->u.str);
    attr=attr_search(attrs, NULL, attr_font);
    if (attr)
        item->font_name = g_strdup(attr->u.str);

}
void osd_std_config(struct osd_item *item, struct navit *navit) {
    struct attr attr;
    char *err = NULL;	/* Error description */

    dbg(lvl_debug, "enter, item=%p, enable_cs=%p", item, item->enable_cs);
    if (item->enable_cs) {
        item->reconfig_cb = callback_new_1(callback_cast(osd_std_reconfigure), item);
        command_saved_set_cb(item->enable_cs, item->reconfig_cb);

        if (!command_saved_error(item->enable_cs)) {
            item->configured = !! command_saved_get_int(item->enable_cs);
        } else {
            err = command_error_to_text(command_saved_error(item->enable_cs));
            dbg(lvl_error, "Error in saved command: %s, item=%p.", err, item);
            g_free(err);
        }
    } else {
        if (!navit_get_attr(navit, attr_osd_configuration, &attr, NULL))
            attr.u.num=-1;
        item->configured = !!(attr.u.num & item->osd_configuration);
    }

    if (item->gr && !(item->flags & DISABLE_OVERLAY))
        graphics_overlay_disable(item->gr, !item->configured);
}

void osd_set_std_config(struct navit *nav, struct osd_item *item) {
    dbg(lvl_debug, "enter, item=%p", item);
    item->cb = callback_new_attr_2(callback_cast(osd_std_config), attr_osd_configuration, item, nav);
    navit_add_callback(nav, item->cb);
    osd_std_config(item, nav);
}

void osd_set_keypress(struct navit *nav, struct osd_item *item) {
    struct graphics *navit_gr = navit_get_graphics(nav);
    dbg(lvl_info,"accesskey %s",item->accesskey);
    if (item->accesskey) {
        item->keypress_cb=callback_new_attr_2(callback_cast(osd_std_keypress), attr_keypress, item, nav);
        graphics_add_callback(navit_gr, item->keypress_cb);
    }
}

/**
 * @brief Sets up the graphics for an item.
 *
 * This method creates a new graphics overlay for an item and initializes its attributes (colors, font
 * and callbacks for resize and key events).
 *
 * @param nav The navit object
 * @param item The OSD item
 * @param priv The `struct osd_priv` for the OSD item
 */
void osd_set_std_graphic(struct navit *nav, struct osd_item *item, struct osd_priv *priv) {
    struct graphics *navit_gr;
    int w, h;
    struct padding *padding = NULL;

    navit_gr = navit_get_graphics(nav);
    w = navit_get_width(nav);
    h = navit_get_height(nav);

    padding = graphics_get_data(navit_gr, "padding");

    if (padding) {
        dbg(lvl_debug, "Got padding=%p for item=%p: left=%d top=%d right=%d bottom=%d",
            padding, item, padding->left, padding->top, padding->right, padding->bottom);
        w -= (padding->left + padding->right);
        h -= (padding->top + padding->bottom);
    } else
        dbg(lvl_debug, "Padding is NULL");

    osd_std_calculate_sizes(item, w, h);

    if (padding) {
        item->p.x += padding->left;
        item->p.y += padding->top;
    }

    item->gr = graphics_overlay_new(navit_gr, &item->p, item->w, item->h, 1);

    item->graphic_bg = graphics_gc_new(item->gr);
    graphics_gc_set_foreground(item->graphic_bg, &item->color_bg);
    graphics_background_gc(item->gr, item->graphic_bg);

    item->graphic_fg = graphics_gc_new(item->gr);
    graphics_gc_set_foreground(item->graphic_fg, &item->color_fg);

    if (item->flags & ITEM_HAS_TEXT) {
        item->font = graphics_named_font_new(item->gr, item->font_name, item->font_size, 1);
        item->graphic_fg_text = graphics_gc_new(item->gr);
        graphics_gc_set_foreground(item->graphic_fg_text, &item->text_color);
    }

    osd_set_std_config(nav, item);

    item->resize_cb = callback_new_attr_2(callback_cast(osd_std_calculate_sizes_and_redraw), attr_resize, item, priv);
    graphics_add_callback(navit_gr, item->resize_cb);
    osd_set_keypress(nav, item);
}

void osd_fill_with_bgcolor(struct osd_item *item) {
    struct point p[1];
    graphics_draw_mode(item->gr, draw_mode_begin);
    p[0].x=0;
    p[0].y=0;
    graphics_draw_rectangle(item->gr, item->graphic_bg, p, item->w, item->h);
}

struct object_func osd_func = {
    attr_osd,
    (object_func_new)osd_new,
    (object_func_get_attr)osd_get_attr,
    (object_func_iter_new)navit_object_attr_iter_new,
    (object_func_iter_destroy)navit_object_attr_iter_destroy,
    (object_func_set_attr)osd_set_attr,
    (object_func_add_attr)navit_object_add_attr,
    (object_func_remove_attr)navit_object_remove_attr,
    (object_func_init)NULL,
    (object_func_destroy)osd_destroy,
    (object_func_dup)NULL,
    (object_func_ref)navit_object_ref,
    (object_func_unref)navit_object_unref,
};
