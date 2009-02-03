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

#include <math.h>
#include <stdio.h>
#include <glib.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "item.h"
#include "point.h"
#include "coord.h"
#include "graphics.h"
#include "transform.h"
#include "route.h"
#include "navit.h"
#include "plugin.h"
#include "debug.h"
#include "callback.h"
#include "color.h"
#include "vehicle.h"
#include "navigation.h"
#include "track.h"
#include "map.h"
#include "file.h"
#include "attr.h"
#include "command.h"
#include "navit_nls.h"

struct osd_item {
	struct point p;
	int flags, attr_flags, w, h, fg_line_width, font_size, osd_configuration, configured;
	struct color color_bg, color_white, text_color;
	struct graphics *gr;
	struct graphics_gc *graphic_bg, *graphic_fg_white, *graphic_fg_text;
	struct graphics_font *font;
	struct callback *cb;
	int pressed;
	char *command;
};

struct compass {
	struct osd_item osd_item;
	struct graphics_gc *green;
};

static void
wrap_point(struct point *p, struct navit *nav)
{
	if (p->x < 0)
		p->x += navit_get_width(nav);
	if (p->y < 0)
		p->y += navit_get_height(nav);

}

static void
osd_std_click(struct osd_item *this, struct navit *nav, int pressed, int button, struct point *p)
{
	struct point bp = this->p;
	wrap_point(&bp, nav);
	if ((p->x < bp.x || p->y < bp.y || p->x > bp.x + this->w || p->y > bp.y + this->h) && !this->pressed)
		return;
	navit_ignore_button(nav);
	this->pressed = pressed;
	if (pressed) {
		struct attr navit;
		navit.type=attr_navit;
		navit.u.navit=nav;
		dbg(0, "calling command '%s'\n", this->command);
		command_evaluate_to_void(&navit, this->command);
	}
}

static void
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

static void
osd_std_config(struct osd_item *item, struct navit *navit)
{
	struct attr osd_configuration;
	dbg(0,"enter\n");
	if (!navit_get_attr(navit, attr_osd_configuration, &osd_configuration, NULL))
		return;
	item->configured = !!(osd_configuration.u.num & item->osd_configuration);
	graphics_overlay_disable(item->gr, !item->configured);
}

static void
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

static void
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


static void
transform_rotate(struct point *center, int angle, struct point *p,
		 int count)
{
	int i, x, y;
	double dx, dy;
	for (i = 0; i < count; i++) {
		dx = sin(M_PI * angle / 180.0);
		dy = cos(M_PI * angle / 180.0);
		x = dy * p->x - dx * p->y;
		y = dx * p->x + dy * p->y;

		p->x = center->x + x;
		p->y = center->y + y;
		p++;
	}
}

static void
handle(struct graphics *gr, struct graphics_gc *gc, struct point *p, int r,
       int dir)
{
	struct point ph[3];
	int l = r * 0.4;

	ph[0].x = 0;
	ph[0].y = r;
	ph[1].x = 0;
	ph[1].y = -r;
	transform_rotate(p, dir, ph, 2);
	graphics_draw_lines(gr, gc, ph, 2);
	ph[0].x = -l;
	ph[0].y = -r + l;
	ph[1].x = 0;
	ph[1].y = -r;
	ph[2].x = l;
	ph[2].y = -r + l;
	transform_rotate(p, dir, ph, 3);
	graphics_draw_lines(gr, gc, ph, 3);
}

static char *
format_distance(double distance, char *sep)
{
	if (distance >= 100000)
		return g_strdup_printf("%.0f%skm", distance / 1000, sep);
	else if (distance >= 10000)
		return g_strdup_printf("%.1f%skm", distance / 1000, sep);
	else if (distance >= 300)
		return g_strdup_printf("%.0f%sm", round(distance / 25) * 25, sep);
	else if (distance >= 50)
		return g_strdup_printf("%.0f%sm", round(distance / 10) * 10, sep);
	else if (distance >= 10)
		return g_strdup_printf("%.0f%sm", distance, sep);
	else
		return g_strdup_printf("%.1f%sm", distance, sep);
}

static char * 
format_time(struct tm *tm, int days)
{
	if (days)
		return g_strdup_printf("%d+%02d:%02d", days, tm->tm_hour, tm->tm_min);
	else
		return g_strdup_printf("%02d:%02d", tm->tm_hour, tm->tm_min);
}

static char * 
format_speed(double speed, char *sep)
{
	return g_strdup_printf("%.0f%skm/h", speed, sep);
}

static void
osd_compass_draw(struct compass *this, struct navit *nav,
		 struct vehicle *v)
{
	struct point p;
	struct attr attr_dir, destination_attr, position_attr;
	double dir, vdir = 0;
	char *buffer;
	struct coord c1, c2;
	enum projection pro;

	osd_std_draw(&this->osd_item);
	p.x = 30;
	p.y = 30;
	graphics_draw_circle(this->osd_item.gr,
			     this->osd_item.graphic_fg_white, &p, 50);
	if (v) {
		if (vehicle_get_attr
		    (v, attr_position_direction, &attr_dir, NULL)) {
			vdir = *attr_dir.u.numd;
			handle(this->osd_item.gr,
			       this->osd_item.graphic_fg_white, &p, 20,
			       -vdir);
		}

		if (navit_get_attr
		    (nav, attr_destination, &destination_attr, NULL)
		    && vehicle_get_attr(v, attr_position_coord_geo,
					&position_attr, NULL)) {
			pro = destination_attr.u.pcoord->pro;
			transform_from_geo(pro, position_attr.u.coord_geo,
					   &c1);
			c2.x = destination_attr.u.pcoord->x;
			c2.y = destination_attr.u.pcoord->y;
			dir =
			    atan2(c2.x - c1.x, c2.y - c1.y) * 180.0 / M_PI;
			dir -= vdir;
			handle(this->osd_item.gr, this->green, &p, 20,
			       dir);
			buffer=format_distance(transform_distance(pro, &c1, &c2),"");
			p.x = 8;
			p.y = 72;
			graphics_draw_text(this->osd_item.gr, this->green,
					   NULL, this->osd_item.font,
					   buffer, &p, 0x10000, 0);
			g_free(buffer);
		}
	}
	graphics_draw_mode(this->osd_item.gr, draw_mode_end);
}



static void
osd_compass_init(struct compass *this, struct navit *nav)
{
	struct color c;

	osd_set_std_graphic(nav, &this->osd_item);

	this->green = graphics_gc_new(this->osd_item.gr);
	c.r = 0;
	c.g = 65535;
	c.b = 0;
	c.a = 65535;
	graphics_gc_set_foreground(this->green, &c);
	graphics_gc_set_linewidth(this->green, 2);
	graphics_gc_set_linewidth(this->osd_item.graphic_fg_white, 2);

	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_compass_draw), attr_position_coord_geo, this));

	osd_compass_draw(this, nav, NULL);
}

static struct osd_priv *
osd_compass_new(struct navit *nav, struct osd_methods *meth,
		struct attr **attrs)
{
	struct compass *this = g_new0(struct compass, 1);
	this->osd_item.p.x = 20;
	this->osd_item.p.y = 20;
	this->osd_item.w = 60;
	this->osd_item.h = 80;
	this->osd_item.font_size = 200;
	osd_set_std_attr(attrs, &this->osd_item, 2);
	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_compass_init), attr_navit, this));
	return (struct osd_priv *) this;
}

struct osd_button {
	int use_overlay;
	struct osd_item item;
	struct callback *draw_cb,*navit_init_cb;
	struct graphics_image *img;
	char *src;
};

static void
osd_button_draw(struct osd_button *this, struct navit *nav)
{
	struct point bp = this->item.p;
	wrap_point(&bp, nav);
	graphics_draw_image(this->item.gr, this->item.graphic_bg, &bp, this->img);
}

static void
osd_button_init(struct osd_button *this, struct navit *nav)
{
	struct graphics *gra = navit_get_graphics(nav);
	dbg(1, "enter\n");
	this->img = graphics_image_new(gra, this->src);
	if (!this->img) {
		dbg(0, "failed to load '%s'\n", this->src);
		return;
	}
	if (!this->item.w)
		this->item.w=this->img->width;
	if (!this->item.h)
		this->item.h=this->img->height;
	if (this->use_overlay) {
		struct graphics_image *img;
		struct point p;
		osd_set_std_graphic(nav, &this->item);
		img=graphics_image_new(this->item.gr, this->src);
		p.x=(this->item.w-this->img->width)/2;
		p.y=(this->item.h-this->img->height)/2;
		osd_std_draw(&this->item);
		graphics_draw_image(this->item.gr, this->item.graphic_bg, &p, img);
		graphics_draw_mode(this->item.gr, draw_mode_end);
		graphics_image_free(this->item.gr, img);
	} else {
		this->item.gr=gra;
		this->item.graphic_bg=graphics_gc_new(this->item.gr);
		graphics_add_callback(gra, this->draw_cb=callback_new_attr_2(callback_cast(osd_button_draw), attr_postdraw, this, nav));
	}
	navit_add_callback(nav, this->navit_init_cb = callback_new_attr_1(callback_cast (osd_std_click), attr_button, &this->item));
}

static struct osd_priv *
osd_button_new(struct navit *nav, struct osd_methods *meth,
	       struct attr **attrs)
{
	struct osd_button *this = g_new0(struct osd_button, 1);
	struct attr *attr;

	osd_set_std_attr(attrs, &this->item, 1);

	attr=attr_search(attrs, NULL, attr_use_overlay);
	if (attr)
		this->use_overlay=attr->u.num;
	if (!this->item.command) {
		dbg(0, "no command\n");
		goto error;
	}
	attr = attr_search(attrs, NULL, attr_src);
	if (!attr) {
		dbg(0, "no src\n");
		goto error;
	}
	this->src = g_strjoin(NULL, getenv("NAVIT_SHAREDIR"), "/xpm/", attr->u.str, NULL);
	navit_add_callback(nav, this->navit_init_cb = callback_new_attr_1(callback_cast (osd_button_init), attr_navit, this));

	return (struct osd_priv *) this;
      error:
	g_free(this);
	return NULL;
}

struct nav_next_turn {
	struct osd_item osd_item;
	char *test_text;
	char *icon_src;
	int icon_h, icon_w, active;
	char *last_name;
};

static void
osd_nav_next_turn_draw(struct nav_next_turn *this, struct navit *navit,
		       struct vehicle *v)
{
	struct point p;
	int do_draw = 0;
	struct navigation *nav = NULL;
	struct map *map = NULL;
	struct map_rect *mr = NULL;
	struct item *item = NULL;
	struct graphics_image *gr_image;
	char *image;
	char *name = "unknown";


	if (navit)
		nav = navit_get_navigation(navit);
	if (nav)
		map = navigation_get_map(nav);
	if (map)
		mr = map_rect_new(map, NULL);
	if (mr)
		while ((item = map_rect_get_item(mr))
		       && (item->type == type_nav_position || item->type == type_nav_none));
	if (item) {
		name = item_to_name(item->type);
		dbg(0, "name=%s\n", name);
		if (this->active != 1 || this->last_name != name) {
			this->active = 1;
			this->last_name = name;
			do_draw = 1;
		}
	} else {
		if (this->active != 0) {
			this->active = 0;
			do_draw = 1;
		}
	}
	if (mr)
		map_rect_destroy(mr);

	if (do_draw) {
		osd_std_draw(&this->osd_item);
		if (this->active) {
			image = g_strdup_printf(this->icon_src, name);
			dbg(0, "image=%s\n", image);
			gr_image =
			    graphics_image_new_scaled(this->osd_item.gr,
						      image, this->icon_w,
						      this->icon_h);
			if (!gr_image) {
				g_free(image);
				image =
				    g_strjoin(NULL,
					      getenv("NAVIT_SHAREDIR"),
					      "/xpm/unknown.xpm", NULL);
				gr_image =
				    graphics_image_new_scaled(this->
							      osd_item.gr,
							      image,
							      this->icon_w,
							      this->
							      icon_h);
			}
			dbg(1, "gr_image=%p\n", gr_image);
			if (gr_image) {
				p.x =
				    (this->osd_item.w -
				     gr_image->width) / 2;
				p.y =
				    (this->osd_item.h -
				     gr_image->height) / 2;
				graphics_draw_image(this->osd_item.gr,
						    this->osd_item.
						    graphic_fg_white, &p,
						    gr_image);
				graphics_image_free(this->osd_item.gr,
						    gr_image);
			}
			g_free(image);
		}
		graphics_draw_mode(this->osd_item.gr, draw_mode_end);
	}
}

static void
osd_nav_next_turn_init(struct nav_next_turn *this, struct navit *nav)
{
	osd_set_std_graphic(nav, &this->osd_item);
	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_nav_next_turn_draw), attr_position_coord_geo, this));
	osd_nav_next_turn_draw(this, nav, NULL);
}

static struct osd_priv *
osd_nav_next_turn_new(struct navit *nav, struct osd_methods *meth,
		      struct attr **attrs)
{
	struct nav_next_turn *this = g_new0(struct nav_next_turn, 1);
	struct attr *attr;

	this->osd_item.p.x = 20;
	this->osd_item.p.y = -80;
	this->osd_item.w = 60;
	this->osd_item.h = 40;
	this->osd_item.font_size = 200;
	osd_set_std_attr(attrs, &this->osd_item, 0);

	this->icon_w = -1;
	this->icon_h = -1;
	this->active = -1;

	attr = attr_search(attrs, NULL, attr_icon_w);
	if (attr)
		this->icon_w = attr->u.num;

	attr = attr_search(attrs, NULL, attr_icon_h);
	if (attr)
		this->icon_h = attr->u.num;

	attr = attr_search(attrs, NULL, attr_icon_src);
	if (attr) {
		struct file_wordexp *we;
		char **array;
		we = file_wordexp_new(attr->u.str);
		array = file_wordexp_get_array(we);
		this->icon_src = g_strdup(array[0]);
		file_wordexp_destroy(we);
	} else
		this->icon_src =
		    g_strjoin(NULL, getenv("NAVIT_SHAREDIR"),
			      "/xpm/%s_32.xpm", NULL);

	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_nav_next_turn_init), attr_navit, this));
	return (struct osd_priv *) this;
}

struct nav_toggle_announcer
{
	int w,h;
	struct callback *navit_init_cb;
	struct osd_item item;
	char *icon_src;
	int icon_h, icon_w, active, last_state;
};

static void
osd_nav_toggle_announcer_draw(struct nav_toggle_announcer *this, struct navit *navit, struct vehicle *v)
{
	struct point p;
	int do_draw = 0;
	struct graphics_image *gr_image;
	char *path;
	char *gui_sound_off = "gui_sound_off";
	char *gui_sound_on = "gui_sound";
    struct attr attr, speechattr;

    if (this->last_state == -1)
    {
        if (!navit_get_attr(navit, attr_speech, &speechattr, NULL))
            if (!speech_get_attr(speechattr.u.speech, attr_active, &attr, NULL))
                attr.u.num = 1;
        this->active = attr.u.num;
    } else
        this->active = !this->active;

    if(this->active != this->last_state)
    {
        this->last_state = this->active;
        do_draw = 1;
    }

	if (do_draw)
    {
		graphics_draw_mode(this->item.gr, draw_mode_begin);
		p.x = 0;
		p.y = 0;
		graphics_draw_rectangle(this->item.gr, this->item.graphic_bg, &p, this->item.w, this->item.h);

		if (this->active)
            path = g_strdup_printf(this->icon_src, gui_sound_on);
        else
            path = g_strdup_printf(this->icon_src, gui_sound_off);
        
        gr_image = graphics_image_new_scaled(this->item.gr, path, this->icon_w, this->icon_h);
        if (!gr_image)
        {
            g_free(path);
            path = g_strjoin(NULL, getenv("NAVIT_SHAREDIR"), "/xpm/unknown.xpm", NULL);
            gr_image = graphics_image_new_scaled(this->item.gr, path, this->icon_w, this->icon_h);
        }
        
        dbg(1, "gr_image=%p\n", gr_image);
        
        if (gr_image)
        {
            p.x = (this->item.w - gr_image->width) / 2;
            p.y = (this->item.h - gr_image->height) / 2;
            graphics_draw_image(this->item.gr, this->item.graphic_fg_white, &p, gr_image);
            graphics_image_free(this->item.gr, gr_image);
        }
        
        g_free(path);
		graphics_draw_mode(this->item.gr, draw_mode_end);
	}
}

static void
osd_nav_toggle_announcer_init(struct nav_toggle_announcer *this, struct navit *nav)
{
	osd_set_std_graphic(nav, &this->item);
	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_nav_toggle_announcer_draw), attr_speech, this));
    navit_add_callback(nav, this->navit_init_cb = callback_new_attr_1(callback_cast(osd_std_click), attr_button, &this->item));
	osd_nav_toggle_announcer_draw(this, nav, NULL);
}

static struct osd_priv *
osd_nav_toggle_announcer_new(struct navit *nav, struct osd_methods *meth, struct attr **attrs)
{
	struct nav_toggle_announcer *this = g_new0(struct nav_toggle_announcer, 1);
    struct attr *attr;
    char *command = "navit_announcer_toggle";

	this->item.w = 48;
	this->item.h = 48;
    this->item.p.x = -64;
    this->item.p.y = 76;

	osd_set_std_attr(attrs, &this->item, 0);

	this->icon_w = -1;
	this->icon_h = -1;
    this->last_state = -1;

    attr = attr_search(attrs, NULL, attr_icon_src);
	if (attr) {
		struct file_wordexp *we;
		char **array;
		we = file_wordexp_new(attr->u.str);
		array = file_wordexp_get_array(we);
		this->icon_src = g_strdup(array[0]);
		file_wordexp_destroy(we);
	} else
		this->icon_src =
		    g_strjoin(NULL, getenv("NAVIT_SHAREDIR"),
			      "/xpm/%s_32.xpm", NULL);

    this->item.command = g_strdup(command);

	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_nav_toggle_announcer_init), attr_navit, this));
	return (struct osd_priv *) this;
}

struct osd_speed_warner {
	struct osd_item item;
	struct graphics_gc *red;
	int width;
	int active;
	int d;
};

static void
osd_speed_warner_draw(struct osd_speed_warner *this, struct navit *navit, struct vehicle *v)
{
	struct point p[4];
	char *text="60";

	osd_std_draw(&this->item);
	p[0].x=this->item.w/2-this->d/4;
	p[0].y=this->item.h/2-this->d/4;
	graphics_draw_rectangle(this->item.gr, this->item.graphic_fg_white, p, this->d/2, this->d/2);
	p[0].x=this->item.w/2;
	p[0].y=this->item.h/2;
	graphics_draw_circle(this->item.gr, this->item.graphic_fg_white, p, this->d/2);
	graphics_draw_circle(this->item.gr, this->red, p, this->d-this->width*2);
	graphics_get_text_bbox(this->item.gr, this->item.font, text, 0x10000, 0, p, 0);
	p[0].x=(this->item.w-p[2].x)/2;
	p[0].y=(this->item.h+p[2].y)/2-p[2].y;
	graphics_draw_text(this->item.gr, this->item.graphic_fg_text, NULL, this->item.font, text, p, 0x10000, 0);
	graphics_draw_mode(this->item.gr, draw_mode_end);
}

static void
osd_speed_warner_init(struct osd_speed_warner *this, struct navit *nav)
{
	osd_set_std_graphic(nav, &this->item);
	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_speed_warner_draw), attr_position_coord_geo, this));
	this->red=graphics_gc_new(this->item.gr);
	graphics_gc_set_foreground(this->red, &(struct color ){0xffff,0,0,0xffff});
	graphics_gc_set_linewidth(this->red, this->width);
	graphics_gc_set_linewidth(this->item.graphic_fg_white, this->d/4+2);
	osd_speed_warner_draw(this, nav, NULL);
}

static struct osd_priv *
osd_speed_warner_new(struct navit *nav, struct osd_methods *meth, struct attr **attrs)
{
	struct osd_speed_warner *this=g_new0(struct osd_speed_warner, 1);
	this->item.p.x=-80;
	this->item.p.y=20;
	this->item.w=60;
	this->item.h=60;
	this->active=-1;
	osd_set_std_attr(attrs, &this->item, 2);
	this->d=this->item.w;
	if (this->item.h < this->d)
		this->d=this->item.h;
	this->width=this->d/10;
	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_speed_warner_init), attr_navit, this));
	return (struct osd_priv *) this;
}

struct osd_text {
	struct osd_item osd_item;
	int active;
	char *text;
	int align;
	char last_text[16];
};

static char *
osd_text_format_attr(struct attr *attr, char *format)
{
	struct tm tm, text_tm, text_tm0;
	time_t textt;
	int days=0;

	switch (attr->type) {
	case attr_position_speed:
		return format_speed(*attr->u.numd,"");
	case attr_destination_time:
		if (!format || (strcmp(format,"arrival") && strcmp(format,"remaining")))
			break;
		textt = time(NULL);
		tm = *localtime(&textt);
		if (!strcmp(format,"remaining")) {
			textt-=tm.tm_hour*3600+tm.tm_min*60+tm.tm_sec;
			tm = *localtime(&textt);
		}
		textt += attr->u.num / 10;
		text_tm = *localtime(&textt);
		if (tm.tm_year != text_tm.tm_year || tm.tm_mon != text_tm.tm_mon || tm.tm_mday != text_tm.tm_mday) {
			text_tm0 = text_tm;
			text_tm0.tm_sec = 0;
			text_tm0.tm_min = 0;
			text_tm0.tm_hour = 0;
			tm.tm_sec = 0;
			tm.tm_min = 0;
			tm.tm_hour = 0;
			days = (mktime(&text_tm0) - mktime(&tm) + 43200) / 86400;
       		}
		return format_time(&text_tm, days);
	case attr_length:
	case attr_destination_length:
		if (!format)
			break;
		if (!strcmp(format,"named"))
			return format_distance(attr->u.num,"");
		if (!strcmp(format,"value") || !strcmp(format,"unit")) {
			char *ret,*tmp=format_distance(attr->u.num," ");
			char *pos=strchr(tmp,' ');
			if (! pos)
				return tmp;
			*pos++='\0';
			if (!strcmp(format,"value"))
				return tmp;
			ret=g_strdup(pos);
			g_free(tmp);
			return ret;
		}
	default:
		break;
	}
	return attr_to_text(attr, NULL, 1);
}

static char *
osd_text_split(char *in, char **index)
{
	char *pos;
	int len;
	if (index)
		*index=NULL;
	len=strcspn(in,"[.");
	in+=len;
	switch (in[0]) {
	case '\0':
		return in;
	case '.':
		*in++='\0';
		return in;
	case '[':
		if (!index)
			return NULL;
		*in++='\0';
		*index=in;
		pos=strchr(in,']');
		if (pos) {
			*pos++='\0';
			if (*pos == '.') {
				*pos++='\0';
			}
			return pos;
		}
		return NULL;
	}
	return NULL;
}

static void
osd_text_draw(struct osd_text *this, struct navit *navit, struct vehicle *v)
{
	struct point p, p2[4];
	char *str,*next,*start,*end,*key,*subkey,*index,*value;
	int do_draw = 0;
	struct attr attr, vehicle_attr;
	struct navigation *nav = NULL;
	struct tracking *tracking = NULL;
	struct route *route = NULL;
	struct map *nav_map = NULL;
	struct map_rect *nav_mr = NULL;
	struct item *item;
	int offset,lines;
	int *speedlist = NULL;
	int height=this->osd_item.font_size*13/256;
	int yspacing=height/2;
	int xspacing=height/4;
	enum attr_type attr_type;

	vehicle_attr.u.vehicle=NULL;
	do_draw=1;
	str=g_strdup(this->text);
	while ((start=strstr(str, "${"))) {
		item=NULL;
		end=strstr(str,"}");
		if (! end)
			break;
		*end++='\0';
		value=NULL;
		key=start+2;
		subkey=osd_text_split(key,NULL);
		if (!strcmp(key,"navigation") && subkey) {
			if (navit && !nav)
				nav = navit_get_navigation(navit);
			if (nav && !nav_map)
				nav_map = navigation_get_map(nav);
			if (nav_map) 
				key=osd_text_split(subkey,&index);
			if (nav_map && !strcmp(subkey,"item")) {
				if (nav_map)
					nav_mr = map_rect_new(nav_map, NULL);
				if (nav_mr) {
					item = map_rect_get_item(nav_mr);
				}
				offset=0;
				if (index)
					offset=atoi(index);
				while (item) {
					if (item->type == type_nav_none) 
						item=map_rect_get_item(nav_mr);
					else if (!offset)
						break;
					else {
						offset--;
						item=map_rect_get_item(nav_mr);
					}
				}
				if (item) {
					dbg(0,"name %s\n", item_to_name(item->type));
					subkey=osd_text_split(key,&index);
					attr_type=attr_from_name(key);
					dbg(0,"type %s\n", attr_to_name(attr_type));
					if (item_attr_get(item, attr_type, &attr)) 
						value=osd_text_format_attr(&attr, index);
					else
						dbg(0,"failed\n");
				}
			}
		} else if (!strcmp(key,"tracking") && subkey) {
			if (navit) {
				tracking = navit_get_tracking(navit);
				route = navit_get_route(navit);
			}
			if (tracking) {
				key=osd_text_split(subkey,&index);
				if (!strcmp(subkey, "item") && key) {
					item=tracking_get_current_item(tracking);
					if (item && !strcmp(key,"route_speed")) {
						if (route && (speedlist=route_get_speedlist(route))) {
							if (item->type >= route_item_first && item->type <= route_item_last) {
								double routespeed=speedlist[item->type-route_item_first];
								value=format_speed(routespeed,"");
							}
						}
					}
				}
			}
		} else if (!strcmp(key,"vehicle") && subkey) {
			if (navit && !vehicle_attr.u.vehicle) {
				navit_get_attr(navit, attr_vehicle, &vehicle_attr, NULL);
			}
			if (vehicle_attr.u.vehicle) {
				key=osd_text_split(subkey,&index);
				attr_type=attr_from_name(subkey);
				if (vehicle_get_attr(vehicle_attr.u.vehicle, attr_type, &attr, NULL)) {
					value=osd_text_format_attr(&attr, index);
				}
			}
				
		}
		*start='\0';
		next=g_strdup_printf("%s%s%s",str,value ? value:" ",end);
		g_free(value);
		g_free(str);
		str=next;
	}
	lines=1;
	next=str;
	while ((next=strstr(next, "\\n"))) {
		lines++;
		next++;
	}
	dbg(0,"this->align=%d\n", this->align);
	switch (this->align & 3) {
	case 1:
		p.y=0;
		break;
	case 2:
		p.y=(this->osd_item.h-lines*(height+yspacing)-yspacing);
		break;
	default:
		p.y=(this->osd_item.h-lines*(height+yspacing)-yspacing)/2;
	}
	if (do_draw) {
		osd_std_draw(&this->osd_item);
		while (str) {
			next=strstr(str, "\\n");
			if (next) {
				*next='\0';
				next+=2;
			}
			graphics_get_text_bbox(this->osd_item.gr,
					       this->osd_item.font,
					       str, 0x10000,
					       0x0, p2, 0);
			switch (this->align & 12) {
			case 4:
				p.x=xspacing;
				break;
			case 8:
				p.x=this->osd_item.w-(p2[2].x-p2[0].x)-xspacing;
				break;
			default:
				p.x = ((p2[0].x - p2[2].x) / 2) + (this->osd_item.w / 2);
			}
			p.y += height+yspacing;
			graphics_draw_text(this->osd_item.gr,
					   this->osd_item.graphic_fg_text,
					   NULL, this->osd_item.font,
					   str, &p, 0x10000,
					   0);
			str=next;
		}
		graphics_draw_mode(this->osd_item.gr, draw_mode_end);
	}

}

static void
osd_text_init(struct osd_text *this, struct navit *nav)
{

	osd_set_std_graphic(nav, &this->osd_item);
	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_text_draw), attr_position_coord_geo, this));
	osd_text_draw(this, nav, NULL);

}

static struct osd_priv *
osd_text_new(struct navit *nav, struct osd_methods *meth,
	    struct attr **attrs)
{
	struct osd_text *this = g_new0(struct osd_text, 1);
	struct attr *attr;

	this->osd_item.p.x = -80;
	this->osd_item.p.y = 20;
	this->osd_item.w = 60;
	this->osd_item.h = 20;
	this->osd_item.font_size = 200;
	osd_set_std_attr(attrs, &this->osd_item, 2);

	this->active = -1;

	attr = attr_search(attrs, NULL, attr_label);
	if (attr)
		this->text = g_strdup(attr->u.str);
	else
		this->text = NULL;
	attr = attr_search(attrs, NULL, attr_align);
	if (attr)
		this->align=attr->u.num;

	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_text_init), attr_navit, this));
	return (struct osd_priv *) this;
}

struct gps_status {
	struct osd_item osd_item;
	char *icon_src;
	int icon_h, icon_w, active;
	int strength;
};

static void
osd_gps_status_draw(struct gps_status *this, struct navit *navit,
		       struct vehicle *v)
{
	struct point p;
	int do_draw = 0;
	struct graphics_image *gr_image;
	char *image;
	struct attr attr, vehicle_attr;
	int strength=-1;

	if (navit && navit_get_attr(navit, attr_vehicle, &vehicle_attr, NULL)) {
		if (vehicle_get_attr(vehicle_attr.u.vehicle, attr_position_fix_type, &attr, NULL)) {
			switch(attr.u.num) {
			case 1:
			case 2:
				strength=2;
				if (vehicle_get_attr(vehicle_attr.u.vehicle, attr_position_sats_used, &attr, NULL)) {
					dbg(0,"num=%d\n", attr.u.num);
					if (attr.u.num >= 3) 
						strength=attr.u.num-1;
					if (strength > 5)
						strength=5;
					if (strength > 3) {
						if (vehicle_get_attr(vehicle_attr.u.vehicle, attr_position_hdop, &attr, NULL)) {
							if (*attr.u.numd > 2.0 && strength > 4)
								strength=4;
							if (*attr.u.numd > 4.0 && strength > 3)
								strength=3;
						}
					}
				}
				break;
			default:
				strength=1;
			}
		}
	}	
	if (this->strength != strength) {
		this->strength=strength;
		do_draw=1;
	}
	if (do_draw) {
		osd_std_draw(&this->osd_item);
		if (this->active) {
			image = g_strdup_printf(this->icon_src, strength);
			gr_image = graphics_image_new_scaled(this->osd_item.gr, image, this->icon_w, this->icon_h);
			if (gr_image) {
				p.x = (this->osd_item.w - gr_image->width) / 2;
				p.y = (this->osd_item.h - gr_image->height) / 2;
				graphics_draw_image(this->osd_item.gr, this->osd_item.  graphic_fg_white, &p, gr_image);
				graphics_image_free(this->osd_item.gr, gr_image);
			}
			g_free(image);
		}
		graphics_draw_mode(this->osd_item.gr, draw_mode_end);
	}
}

static void
osd_gps_status_init(struct gps_status *this, struct navit *nav)
{
	osd_set_std_graphic(nav, &this->osd_item);
	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_gps_status_draw), attr_position_coord_geo, this));
	osd_gps_status_draw(this, nav, NULL);
}

static struct osd_priv *
osd_gps_status_new(struct navit *nav, struct osd_methods *meth,
		      struct attr **attrs)
{
	struct gps_status *this = g_new0(struct gps_status, 1);
	struct attr *attr;

	this->osd_item.p.x = 20;
	this->osd_item.p.y = -80;
	this->osd_item.w = 60;
	this->osd_item.h = 40;
	this->osd_item.font_size = 200;
	osd_set_std_attr(attrs, &this->osd_item, 0);

	this->icon_w = -1;
	this->icon_h = -1;
	this->active = -1;
	this->strength = -1;

	attr = attr_search(attrs, NULL, attr_icon_w);
	if (attr)
		this->icon_w = attr->u.num;

	attr = attr_search(attrs, NULL, attr_icon_h);
	if (attr)
		this->icon_h = attr->u.num;

	attr = attr_search(attrs, NULL, attr_icon_src);
	if (attr) {
		struct file_wordexp *we;
		char **array;
		we = file_wordexp_new(attr->u.str);
		array = file_wordexp_get_array(we);
		this->icon_src = g_strdup(array[0]);
		file_wordexp_destroy(we);
	} else
		this->icon_src = g_strjoin(NULL, getenv("NAVIT_SHAREDIR"), "/xpm/gui_strength_%d_32_32.png", NULL);

	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_gps_status_init), attr_navit, this));
	return (struct osd_priv *) this;
}


struct volume {
	struct osd_item osd_item;
	char *icon_src;
	int icon_h, icon_w, active;
	int strength;
	struct callback *click_cb;
};

static void
osd_volume_draw(struct volume *this, struct navit *navit)
{
	struct point p;
	struct graphics_image *gr_image;
	char *image;

	osd_std_draw(&this->osd_item);
	if (this->active) {
		image = g_strdup_printf(this->icon_src, this->strength);
		gr_image = graphics_image_new_scaled(this->osd_item.gr, image, this->icon_w, this->icon_h);
		if (gr_image) {
			p.x = (this->osd_item.w - gr_image->width) / 2;
			p.y = (this->osd_item.h - gr_image->height) / 2;
			graphics_draw_image(this->osd_item.gr, this->osd_item.  graphic_fg_white, &p, gr_image);
			graphics_image_free(this->osd_item.gr, gr_image);
		}
		g_free(image);
	}
	graphics_draw_mode(this->osd_item.gr, draw_mode_end);
}

static void
osd_volume_click(struct volume *this, struct navit *nav, int pressed, int button, struct point *p)
{
	struct point bp = this->osd_item.p;
	wrap_point(&bp, nav);
	if ((p->x < bp.x || p->y < bp.y || p->x > bp.x + this->osd_item.w || p->y > bp.y + this->osd_item.h) && !this->osd_item.pressed)
		return;
	navit_ignore_button(nav);
	if (pressed) {
		if (p->y - bp.y < this->osd_item.h/2)
			this->strength++;
		else
			this->strength--;
		if (this->strength < 0)
			this->strength=0;
		if (this->strength > 5)
			this->strength=5;
		osd_volume_draw(this, nav);
	}
}
static void
osd_volume_init(struct volume *this, struct navit *nav)
{
	osd_set_std_graphic(nav, &this->osd_item);
	navit_add_callback(nav, this->click_cb = callback_new_attr_1(callback_cast (osd_volume_click), attr_button, this));
	osd_volume_draw(this, nav);
}

static struct osd_priv *
osd_volume_new(struct navit *nav, struct osd_methods *meth,
		      struct attr **attrs)
{
	struct volume *this = g_new0(struct volume, 1);
	struct attr *attr;

	this->osd_item.p.x = 20;
	this->osd_item.p.y = -80;
	this->osd_item.w = 60;
	this->osd_item.h = 40;
	this->osd_item.font_size = 200;
	osd_set_std_attr(attrs, &this->osd_item, 0);

	this->icon_w = -1;
	this->icon_h = -1;
	this->active = -1;
	this->strength = -1;

	attr = attr_search(attrs, NULL, attr_icon_w);
	if (attr)
		this->icon_w = attr->u.num;

	attr = attr_search(attrs, NULL, attr_icon_h);
	if (attr)
		this->icon_h = attr->u.num;

	attr = attr_search(attrs, NULL, attr_icon_src);
	if (attr) {
		struct file_wordexp *we;
		char **array;
		we = file_wordexp_new(attr->u.str);
		array = file_wordexp_get_array(we);
		this->icon_src = g_strdup(array[0]);
		file_wordexp_destroy(we);
	} else
		this->icon_src = g_strjoin(NULL, getenv("NAVIT_SHAREDIR"), "/xpm/gui_strength_%d_32_32.png", NULL);

	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_volume_init), attr_navit, this));
	return (struct osd_priv *) this;
}


void
plugin_init(void)
{
	plugin_register_osd_type("compass", osd_compass_new);
	plugin_register_osd_type("navigation_next_turn", osd_nav_next_turn_new);
	plugin_register_osd_type("button", osd_button_new);
    	plugin_register_osd_type("toggle_announcer", osd_nav_toggle_announcer_new);
    	plugin_register_osd_type("speed_warner", osd_speed_warner_new);
    	plugin_register_osd_type("text", osd_text_new);
    	plugin_register_osd_type("gps_status", osd_gps_status_new);
    	plugin_register_osd_type("volume", osd_volume_new);
}
