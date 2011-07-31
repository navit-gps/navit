/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifndef NAVIT_OSD_H
#define NAVIT_OSD_H

struct osd_priv;
struct attr;

struct osd_methods {
	void (*osd_destroy)(struct osd_priv *osd);
	void (*set_attr)(struct osd_priv *osd, struct attr* attr);
};

#define osd_draw_cast(x) (void (*)(struct osd_priv *osd, struct navit *navit, struct vehicle *v))(x)

struct osd_item_methods {
	void (*draw)(struct osd_priv *osd, struct navit *navit, struct vehicle *v);
};

struct osd_item {
	struct point p;
	struct osd_item_methods meth;
	int flags, attr_flags, w, h, fg_line_width, font_size, osd_configuration, configured;
	int rel_w, rel_h, rel_x, rel_y;
	struct color color_bg, color_white, text_color;
	struct navit *navit;
	struct graphics *gr;
	struct graphics_gc *graphic_bg, *graphic_fg_white, *graphic_fg_text;
	struct graphics_font *font;
	char *font_name;
	struct callback *cb;
	struct callback *resize_cb;
	struct callback *reconfig_cb;
	struct callback *keypress_cb;
	int pressed;
	char *command;
	struct command_saved *enable_cs;
	char *accesskey;
};

/* prototypes */
struct attr;
struct navit;
struct osd;
struct osd *osd_new(struct attr *parent, struct attr **attrs);
void osd_wrap_point(struct point *p, struct navit *nav);
void osd_std_click(struct osd_item *this, struct navit *nav, int pressed, int button, struct point *p);
void osd_set_std_attr(struct attr **attrs, struct osd_item *item, int flags);
void osd_std_config(struct osd_item *item, struct navit *navit);
void osd_set_std_config(struct navit *nav, struct osd_item *item);
void osd_set_std_graphic(struct navit *nav, struct osd_item *item, struct osd_priv *priv);
void osd_std_resize(struct osd_item *item);
void osd_std_draw(struct osd_item *item);
struct osd* osd_get_osd_by_name(char *name);
int osd_set_attr(struct osd *osd, struct attr* attr);
int osd_get_attr(struct osd *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter);
/* end of prototypes */

#endif

