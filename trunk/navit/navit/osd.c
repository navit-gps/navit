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
#include "color.h"
#include "point.h"
#include "navit.h"
#include "graphics.h"
#include "command.h"
#include "callback.h"
#include "osd.h"


struct osd {
	struct osd_methods meth;
	struct osd_priv *priv;
};

struct osd *
osd_new(struct attr *parent, struct attr **attrs)
{
        struct osd *o;
        struct osd_priv *(*new)(struct navit *nav, struct osd_methods *meth, struct attr **attrs);
	struct attr *type=attr_search(attrs, NULL, attr_type);

	if (! type)
		return NULL;
        new=plugin_get_osd_type(type->u.str);
        if (! new)
                return NULL;
        o=g_new0(struct osd, 1);
        o->priv=new(parent->u.navit, &o->meth, attrs);
        return o;
}

void
osd_wrap_point(struct point *p, struct navit *nav)
{
	if (p->x < 0)
		p->x += navit_get_width(nav);
	if (p->y < 0)
		p->y += navit_get_height(nav);

}

void
osd_std_click(struct osd_item *this, struct navit *nav, int pressed, int button, struct point *p)
{
	struct point bp = this->p;
	osd_wrap_point(&bp, nav);
	if ((p->x < bp.x || p->y < bp.y || p->x > bp.x + this->w || p->y > bp.y + this->h || !this->configured) && !this->pressed)
		return;
	if (navit_ignore_button(nav))
		return;
	this->pressed = pressed;
	if (pressed) {
		struct attr navit;
		navit.type=attr_navit;
		navit.u.navit=nav;
		dbg(0, "calling command '%s'\n", this->command);
		command_evaluate(&navit, this->command);
	}
}

void
osd_set_std_attr(struct attr **attrs, struct osd_item *item, int flags)
{
	struct attr *attr;

	item->flags=flags;
	item->osd_configuration=-1;
	item->color_white.r = 0xffff;
	item->color_white.g = 0xffff;
	item->color_white.b = 0xffff;
	item->color_white.a = 0xffff;
	item->text_color.r = 0xffff;
	item->text_color.g = 0xffff;
	item->text_color.b = 0xffff;
	item->text_color.a = 0xffff;
	if (flags & 1) {
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

	attr = attr_search(attrs, NULL, attr_w);
	if (attr)
		item->w = attr->u.num;

	attr = attr_search(attrs, NULL, attr_h);
	if (attr)
		item->h = attr->u.num;

	attr = attr_search(attrs, NULL, attr_x);
	if (attr)
		item->p.x = attr->u.num;

	attr = attr_search(attrs, NULL, attr_y);
	if (attr)
		item->p.y = attr->u.num;

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
        attr=attr_search(attrs, NULL, attr_flags);
        if (attr)
                item->attr_flags=attr->u.num;

}

void
osd_std_config(struct osd_item *item, struct navit *navit)
{
	struct attr osd_configuration;
	dbg(1,"enter\n");
	if (!navit_get_attr(navit, attr_osd_configuration, &osd_configuration, NULL))
		osd_configuration.u.num=-1;
	item->configured = !!(osd_configuration.u.num & item->osd_configuration);
	graphics_overlay_disable(item->gr, !item->configured);
}

void
osd_set_std_graphic(struct navit *nav, struct osd_item *item)
{
	struct graphics *navit_gr;

	navit_gr = navit_get_graphics(nav);
	item->gr = graphics_overlay_new(navit_gr, &item->p, item->w, item->h, 65535, 1);

	item->graphic_bg = graphics_gc_new(item->gr);
	graphics_gc_set_foreground(item->graphic_bg, &item->color_bg);
	graphics_background_gc(item->gr, item->graphic_bg);

	item->graphic_fg_white = graphics_gc_new(item->gr);
	graphics_gc_set_foreground(item->graphic_fg_white, &item->color_white);

	if (item->flags & 2) {
		item->font = graphics_font_new(item->gr, item->font_size, 1);
		item->graphic_fg_text = graphics_gc_new(item->gr);
		graphics_gc_set_foreground(item->graphic_fg_text, &item->text_color);
	}

	item->cb = callback_new_attr_2(callback_cast(osd_std_config), attr_osd_configuration, item, nav);
	navit_add_callback(nav, item->cb);
	osd_std_config(item, nav);
}

void
osd_std_resize(struct osd_item *item)
{
	graphics_overlay_resize(item->gr, &item->p, item->w, item->h, 65535, 1);
}

void
osd_std_draw(struct osd_item *item)
{
	struct point p[2];
	int flags=item->attr_flags;
	graphics_draw_mode(item->gr, draw_mode_begin);
	p[0].x=0;
	p[0].y=0;
	graphics_draw_rectangle(item->gr, item->graphic_bg, p, item->w, item->h);
	p[1].x=item->w-1;
	p[1].y=0;
	if (flags & 1) 
		graphics_draw_lines(item->gr, item->graphic_fg_text, p, 2);
	p[0].x=item->w-1;
	p[0].y=item->h-1;
	if (flags & 2) 
		graphics_draw_lines(item->gr, item->graphic_fg_text, p, 2);
	p[1].x=0;
	p[1].y=item->h-1;
	if (flags & 4) 
		graphics_draw_lines(item->gr, item->graphic_fg_text, p, 2);
	p[0].x=0;
	p[0].y=0;
	if (flags & 8) 
		graphics_draw_lines(item->gr, item->graphic_fg_text, p, 2);
}
