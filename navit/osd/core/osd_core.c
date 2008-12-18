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
#include "navit_nls.h"

struct osd_item {
	struct point p;
	int w, h, fg_line_width, font_size;
	struct color color_bg, color_white;
	struct graphics *gr;
	struct graphics_gc *graphic_bg, *graphic_fg_white;
	struct graphics_font *font;
};

struct compass {
	struct osd_item osd_item;
	struct graphics_gc *green;
};

static void
osd_set_std_attr(struct attr **attrs, struct osd_item *item)
{
	struct attr *attr;

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

	item->color_white.r = 0xffff;
	item->color_white.g = 0xffff;
	item->color_white.b = 0xffff;
	item->color_white.a = 0xffff;
	item->color_bg.r = 0x0;
	item->color_bg.g = 0x0;
	item->color_bg.b = 0x0;
	item->color_bg.a = 0x5fff;
}

static void
osd_set_std_graphic(struct navit *nav, struct osd_item *item)
{
	struct graphics *navit_gr;

	navit_gr = navit_get_graphics(nav);
	item->gr =
	    graphics_overlay_new(navit_gr, &item->p, item->w, item->h,
				 65535, 1);

	item->graphic_bg = graphics_gc_new(item->gr);
	graphics_gc_set_foreground(item->graphic_bg, &item->color_bg);
	graphics_background_gc(item->gr, item->graphic_bg);

	item->graphic_fg_white = graphics_gc_new(item->gr);
	graphics_gc_set_foreground(item->graphic_fg_white,
				   &item->color_white);

	item->font = graphics_font_new(item->gr, item->font_size, 1);

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

static void
format_distance(char *buffer, double distance)
{
	if (distance >= 100000)
		sprintf(buffer, "%.0fkm", distance / 1000);
	else if (distance >= 10000)
		sprintf(buffer, "%.1fkm", distance / 1000);
	else if (distance >= 300)
		sprintf(buffer, "%.0fm", round(distance / 25) * 25);
	else if (distance >= 50)
		sprintf(buffer, "%.0fm", round(distance / 10) * 10);
	else if (distance >= 10)
		sprintf(buffer, "%.0fm", distance);
	else
		sprintf(buffer, "%.1fm", distance);
}

#if 0
static void
format_speed(char *buffer, double speed)
{
	printf(buffer, "%.0f", speed);
}
#endif

static void
osd_compass_draw(struct compass *this, struct navit *nav,
		 struct vehicle *v)
{
	struct point p;
	struct attr attr_dir, destination_attr, position_attr;
	double dir, vdir = 0;
	char buffer[16];
	struct coord c1, c2;
	enum projection pro;

	graphics_draw_mode(this->osd_item.gr, draw_mode_begin);
	p.x = 0;
	p.y = 0;
	graphics_draw_rectangle(this->osd_item.gr,
				this->osd_item.graphic_bg, &p,
				this->osd_item.w, this->osd_item.h);
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
			format_distance(buffer,
					transform_distance(pro, &c1, &c2));
			p.x = 8;
			p.y = 72;
			graphics_draw_text(this->osd_item.gr, this->green,
					   NULL, this->osd_item.font,
					   buffer, &p, 0x10000, 0);
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

	navit_add_callback(nav,
			   callback_new_attr_1(callback_cast
					       (osd_compass_draw),
					       attr_position_coord_geo,
					       this));

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
	osd_set_std_attr(attrs, &this->osd_item);
	navit_add_callback(nav,
			   callback_new_attr_1(callback_cast
					       (osd_compass_init),
					       attr_navit, this));
	return (struct osd_priv *) this;
}

struct osd_eta {
	struct osd_item osd_item;
	int active;
	char *test_text;
	char last_eta[16];
};

static void
osd_eta_draw(struct osd_eta *this, struct navit *navit, struct vehicle *v)
{
	struct point p, p2[4];
	char eta[16];
	int days = 0, do_draw = 0;
	time_t etat;
	struct tm tm, eta_tm, eta_tm0;
	struct attr attr;
	struct navigation *nav = NULL;
	struct map *map = NULL;
	struct map_rect *mr = NULL;
	struct item *item = NULL;


	eta[0] = '\0';

	if (navit)
		nav = navit_get_navigation(navit);
	if (nav)
		map = navigation_get_map(nav);
	if (map)
		mr = map_rect_new(map, NULL);

	if (mr)
		item = map_rect_get_item(mr);

	if (item) {
		if (item_attr_get(item, attr_destination_time, &attr)) {
			etat = time(NULL);
			tm = *localtime(&etat);
			etat += attr.u.num / 10;
			eta_tm = *localtime(&etat);
			if (tm.tm_year != eta_tm.tm_year
			    || tm.tm_mon != eta_tm.tm_mon
			    || tm.tm_mday != eta_tm.tm_mday) {
				eta_tm0 = eta_tm;
				eta_tm0.tm_sec = 0;
				eta_tm0.tm_min = 0;
				eta_tm0.tm_hour = 0;
				tm.tm_sec = 0;
				tm.tm_min = 0;
				tm.tm_hour = 0;
				days =
				    (mktime(&eta_tm0) - mktime(&tm) +
				     43200) / 86400;
			}
			if (days)
				sprintf(eta, "%d+%02d:%02d", days,
					eta_tm.tm_hour, eta_tm.tm_min);
			else
				sprintf(eta, "%02d:%02d", eta_tm.tm_hour,
					eta_tm.tm_min);
		}
		if (this->active != 1 || strcmp(this->last_eta, eta)) {
			this->active = 1;
			strcpy(this->last_eta, eta);
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

	if (do_draw || this->test_text) {
		graphics_draw_mode(this->osd_item.gr, draw_mode_begin);
		p.x = 0;
		p.y = 0;
		graphics_draw_rectangle(this->osd_item.gr,
					this->osd_item.graphic_bg, &p,
					32767, 32767);
		if (this->active) {
			if (*eta) {
				graphics_get_text_bbox(this->osd_item.gr,
						       this->osd_item.font,
						       eta, 0x10000, 0x0,
						       p2, 0);
				p.y =
				    ((p2[0].y - p2[2].y) / 2) +
				    (this->osd_item.h / 2);
				p.x =
				    ((p2[0].x - p2[2].x) / 2) +
				    (this->osd_item.w / 2);
				graphics_draw_text(this->osd_item.gr,
						   this->osd_item.
						   graphic_fg_white, NULL,
						   this->osd_item.font,
						   eta, &p, 0x10000, 0);
			}
		} else if (this->test_text) {
			graphics_get_text_bbox(this->osd_item.gr,
					       this->osd_item.font,
					       this->test_text, 0x10000,
					       0x0, p2, 0);
			p.y =
			    ((p2[0].y - p2[2].y) / 2) +
			    (this->osd_item.h / 2);
			p.x =
			    ((p2[0].x - p2[2].x) / 2) +
			    (this->osd_item.w / 2);
			graphics_draw_text(this->osd_item.gr,
					   this->osd_item.graphic_fg_white,
					   NULL, this->osd_item.font,
					   this->test_text, &p, 0x10000,
					   0);
		}
		graphics_draw_mode(this->osd_item.gr, draw_mode_end);
	}

}

static void
osd_eta_init(struct osd_eta *this, struct navit *nav)
{

	osd_set_std_graphic(nav, &this->osd_item);
	navit_add_callback(nav,
			   callback_new_attr_1(callback_cast(osd_eta_draw),
					       attr_position_coord_geo,
					       this));
	osd_eta_draw(this, nav, NULL);

}

static struct osd_priv *
osd_eta_new(struct navit *nav, struct osd_methods *meth,
	    struct attr **attrs)
{
	struct osd_eta *this = g_new0(struct osd_eta, 1);
	struct attr *attr;

	this->osd_item.p.x = -80;
	this->osd_item.p.y = 20;
	this->osd_item.w = 60;
	this->osd_item.h = 20;
	this->osd_item.font_size = 200;
	osd_set_std_attr(attrs, &this->osd_item);

	this->active = -1;

	attr = attr_search(attrs, NULL, attr_label);
	if (attr)
		this->test_text = g_strdup(attr->u.str);
	else
		this->test_text = NULL;

	navit_add_callback(nav,
			   callback_new_attr_1(callback_cast(osd_eta_init),
					       attr_navit, this));
	return (struct osd_priv *) this;
}

struct osd_speed {
	struct osd_item osd_item;
	int active;
	char *test_text;
	double last_speed;
};

static void
osd_speed_draw(struct osd_speed *this, struct navit *nav)
{
	struct point p, p2[4];
	struct attr attr;
	double speed = 0;
	int speed_max = 0;
	int do_draw = 0;
	char buffer[16];
	struct route *r=NULL;
	int *speedlist=NULL;
	struct tracking *tr=NULL;
	struct item *item=NULL;
	struct vehicle *v=NULL;


	if (nav) {
		if (navit_get_attr(nav, attr_vehicle, &attr, NULL)) {
			v = attr.u.vehicle;
		}
		r = navit_get_route(nav);
		tr = navit_get_tracking(nav);
	}

	if (r)
		speedlist = route_get_speedlist(r);

	if (tr)
		item = tracking_get_current_item(tr);

	if (item && speedlist) {
		speed_max = speedlist[item->type - route_item_first];
	}

	if (v) {
		if (vehicle_get_attr(v, attr_position_speed, &attr, NULL)) {
			speed = *attr.u.numd;
		}
		if (this->active != 1 || this->last_speed != speed) {
			this->last_speed = speed;
			this->active = 1;
			do_draw = 1;
		}
	} else {
		if (this->active != 0) {
			this->active = 0;
			do_draw = 1;
		}
	}

	if (do_draw) {
		graphics_draw_mode(this->osd_item.gr, draw_mode_begin);
		p.x = 0;
		p.y = 0;
		graphics_draw_rectangle(this->osd_item.gr,
					this->osd_item.graphic_bg, &p,
					32767, 32767);
		if (this->active) {
			sprintf(buffer, "%.0f/%i", speed, speed_max);
			graphics_get_text_bbox(this->osd_item.gr,
					       this->osd_item.font, buffer,
					       0x10000, 0x0, p2, 0);
			p.y =
			    ((p2[0].y - p2[2].y) / 2) +
			    (this->osd_item.h / 2);
			graphics_draw_text(this->osd_item.gr,
					   this->osd_item.graphic_fg_white,
					   NULL, this->osd_item.font,
					   buffer, &p, 0x10000, 0);
		}
		graphics_draw_mode(this->osd_item.gr, draw_mode_end);
	}
}

static void
osd_speed_init(struct osd_speed *this, struct navit *nav)
{
	struct attr attr, attr_cb;
	struct vehicle *v = NULL;

	osd_set_std_graphic(nav, &this->osd_item);

	if (nav) {
		if (navit_get_attr(nav, attr_vehicle, &attr, NULL)) {
			v = attr.u.vehicle;
		}
	}

	if (v) {
		attr_cb.type = attr_callback;
		attr_cb.u.callback =
		    callback_new_attr_2(callback_cast(osd_speed_draw),
					attr_position_speed, this, nav);
		if (!vehicle_add_attr(v, &attr_cb)) {
			dbg(0, "failed register callback\n");
		}
	}
	osd_speed_draw(this, nav);
}

static struct osd_priv *
osd_speed_new(struct navit *nav, struct osd_methods *meth,
	      struct attr **attrs)
{
	struct osd_speed *this = g_new0(struct osd_speed, 1);
	struct attr *attr;

	this->osd_item.p.x = 0;
	this->osd_item.p.y = -40;
	this->osd_item.w = 150;
	this->osd_item.h = 40;
	this->osd_item.font_size = 200;
	osd_set_std_attr(attrs, &this->osd_item);

	this->active = -1;

	attr = attr_search(attrs, NULL, attr_label);
	if (attr)
		this->test_text = g_strdup(attr->u.str);
	else
		this->test_text = NULL;

	navit_add_callback(nav,
			   callback_new_attr_1(callback_cast
					       (osd_speed_init),
					       attr_navit, this));
	return (struct osd_priv *) this;
}

struct osd_sats {
	struct osd_item osd_item;
	int active;
	char *test_text;
	int last_sats, last_sats_used, last_sats_signal;
};

static void
osd_sats_draw(struct osd_sats *this, struct vehicle *v)
{
	struct point p, p2[4];
	struct attr attr;
	int satelites = 0, satelites_used = 0, do_draw =
	    0, satelites_signal = 0;
	char buffer[16];

	if (v) {
		if (vehicle_get_attr(v, attr_position_qual, &attr, NULL)) {
			satelites = attr.u.num;
		}
		if (vehicle_get_attr
		    (v, attr_position_sats_used, &attr, NULL)) {
			satelites_used = attr.u.num;
		}
		if (vehicle_get_attr
		    (v, attr_position_sats_signal, &attr, NULL)) {
			satelites_signal = attr.u.num;
		}
		if (this->active != 1 || this->last_sats != satelites
		    || this->last_sats_used != satelites_used
		    || this->last_sats_signal != satelites_signal) {
			this->last_sats = satelites;
			this->last_sats_used = satelites_used;
			this->last_sats_signal = satelites_signal;
			this->active = 1;
			do_draw = 1;
		}
	} else {
		if (this->active != 0) {
			this->active = 0;
			do_draw = 1;
		}
	}

	if (do_draw) {
		graphics_draw_mode(this->osd_item.gr, draw_mode_begin);
		p.x = 0;
		p.y = 0;
		graphics_draw_rectangle(this->osd_item.gr,
					this->osd_item.graphic_bg, &p,
					32767, 32767);
		if (this->active) {
			sprintf(buffer, "%i/%i/%i", satelites_used,
				satelites_signal, satelites);
			graphics_get_text_bbox(this->osd_item.gr,
					       this->osd_item.font, buffer,
					       0x10000, 0x0, p2, 0);
			p.y =
			    ((p2[0].y - p2[2].y) / 2) +
			    (this->osd_item.h / 2);
			graphics_draw_text(this->osd_item.gr,
					   this->osd_item.graphic_fg_white,
					   NULL, this->osd_item.font,
					   buffer, &p, 0x10000, 0);
		}
		graphics_draw_mode(this->osd_item.gr, draw_mode_end);
	}
}

static void
osd_sats_init(struct osd_sats *this, struct navit *nav)
{
	struct attr attr, attr_cb;
	struct vehicle *v = NULL;

	osd_set_std_graphic(nav, &this->osd_item);

	if (nav) {
		if (navit_get_attr(nav, attr_vehicle, &attr, NULL)) {
			v = attr.u.vehicle;
		}
	}

	if (v) {
		attr_cb.type = attr_callback;
		attr_cb.u.callback =
		    callback_new_attr_2(callback_cast(osd_sats_draw),
					attr_position_sats, this, v);
		if (!vehicle_add_attr(v, &attr_cb)) {
			dbg(0, "failed register callback\n");
		}
	}
	osd_sats_draw(this, v);
}

static struct osd_priv *
osd_sats_new(struct navit *nav, struct osd_methods *meth,
	     struct attr **attrs)
{
	struct osd_sats *this = g_new0(struct osd_sats, 1);
	struct attr *attr;

	this->osd_item.p.x = 0;
	this->osd_item.p.y = -40;
	this->osd_item.w = 150;
	this->osd_item.h = 40;
	this->osd_item.font_size = 200;
	osd_set_std_attr(attrs, &this->osd_item);

	this->active = -1;

	attr = attr_search(attrs, NULL, attr_label);
	if (attr)
		this->test_text = g_strdup(attr->u.str);
	else
		this->test_text = NULL;

	navit_add_callback(nav,
			   callback_new_attr_1(callback_cast
					       (osd_sats_init), attr_navit,
					       this));
	return (struct osd_priv *) this;
}

struct osd_nav_distance_to_target {
	struct osd_item osd_item;
	int active;
	char *test_text;
	char last_distance[16];
};


static void
osd_nav_distance_to_target_draw(struct osd_nav_distance_to_target *this,
				struct navit *navit, struct vehicle *v)
{
	struct point p, p2[4];
	char distance[16];
	int do_draw = 0;
	struct attr attr;
	struct navigation *nav = NULL;
	struct map *map = NULL;
	struct map_rect *mr = NULL;
	struct item *item = NULL;


	distance[0] = '\0';

	if (navit)
		nav = navit_get_navigation(navit);
	if (nav)
		map = navigation_get_map(nav);
	if (map)
		mr = map_rect_new(map, NULL);

	if (mr)
		item = map_rect_get_item(mr);

	if (item) {
		if (item_attr_get(item, attr_destination_length, &attr)) {
			format_distance(distance, attr.u.num);
		}
		if (this->active != 1
		    || strcmp(this->last_distance, distance)) {
			this->active = 1;
			strcpy(this->last_distance, distance);
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


	if (do_draw || this->test_text) {
		graphics_draw_mode(this->osd_item.gr, draw_mode_begin);
		p.x = 0;
		p.y = 0;
		graphics_draw_rectangle(this->osd_item.gr,
					this->osd_item.graphic_bg, &p,
					32767, 32767);
		if (this->active || this->test_text) {
			if (this->test_text) {
				graphics_get_text_bbox(this->osd_item.gr,
						       this->osd_item.font,
						       this->test_text,
						       0x10000, 0x0, p2,
						       0);
				p.x =
				    ((p2[0].x - p2[2].x) / 2) +
				    (this->osd_item.w / 2);
				p.y =
				    ((p2[0].y - p2[2].y) / 2) +
				    (this->osd_item.h / 2);
				graphics_draw_text(this->osd_item.gr,
						   this->osd_item.
						   graphic_fg_white, NULL,
						   this->osd_item.font,
						   this->test_text, &p,
						   0x10000, 0);
			} else if (*distance) {
				graphics_get_text_bbox(this->osd_item.gr,
						       this->osd_item.font,
						       distance, 0x10000,
						       0x0, p2, 0);
				p.x =
				    ((p2[0].x - p2[2].x) / 2) +
				    (this->osd_item.w / 2);
				p.y =
				    ((p2[0].y - p2[2].y) / 2) +
				    (this->osd_item.h / 2);
				graphics_draw_text(this->osd_item.gr,
						   this->osd_item.
						   graphic_fg_white, NULL,
						   this->osd_item.font,
						   distance, &p, 0x10000,
						   0);
			}
		}
		graphics_draw_mode(this->osd_item.gr, draw_mode_end);
	}
}


static void
osd_nav_distance_to_target_init(struct osd_nav_distance_to_target *this,
				struct navit *nav)
{

	osd_set_std_graphic(nav, &this->osd_item);

	navit_add_callback(nav,
			   callback_new_attr_1(callback_cast
					       (osd_nav_distance_to_target_draw),
					       attr_position_coord_geo,
					       this));

	osd_nav_distance_to_target_draw(this, nav, NULL);

}

static struct osd_priv *
osd_nav_distance_to_target_new(struct navit *nav, struct osd_methods *meth,
			       struct attr **attrs)
{
	struct osd_nav_distance_to_target *this =
	    g_new0(struct osd_nav_distance_to_target, 1);
	struct attr *attr;

	this->osd_item.p.x = -80;
	this->osd_item.p.y = 40;
	this->osd_item.w = 60;
	this->osd_item.h = 20;
	this->osd_item.font_size = 200;
	osd_set_std_attr(attrs, &this->osd_item);

	this->active = -1;

	attr = attr_search(attrs, NULL, attr_label);
	if (attr)
		this->test_text = g_strdup(attr->u.str);
	else
		this->test_text = NULL;

	navit_add_callback(nav,
			   callback_new_attr_1(callback_cast
					       (osd_nav_distance_to_target_init),
					       attr_navit, this));
	return (struct osd_priv *) this;
}

struct osd_nav_distance_to_next {
	struct osd_item osd_item;
	int active;
	char last_distance[16];
	char *test_text;
};

static void
osd_nav_distance_to_next_draw(struct osd_nav_distance_to_next *this,
			      struct navit *navit, struct vehicle *v)
{
	struct point p, p2[4];
	char distance[16];
	int do_draw = 0;
	struct attr attr;
	struct navigation *nav = NULL;
	struct map *map = NULL;
	struct map_rect *mr = NULL;
	struct item *item = NULL;

	distance[0] = '\0';

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
		if (item_attr_get(item, attr_length, &attr)) {
			format_distance(distance, attr.u.num);
		}
		if (this->active != 1
		    || strcmp(this->last_distance, distance)) {
			this->active = 1;
			strcpy(this->last_distance, distance);
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

	if (do_draw || this->test_text) {
		graphics_draw_mode(this->osd_item.gr, draw_mode_begin);
		p.x = 0;
		p.y = 0;
		graphics_draw_rectangle(this->osd_item.gr,
					this->osd_item.graphic_bg, &p,
					32767, 32767);
		if (this->active || this->test_text) {
			if (this->test_text) {
				graphics_get_text_bbox(this->osd_item.gr,
						       this->osd_item.font,
						       this->test_text,
						       0x10000, 0x0, p2,
						       0);
				p.x =
				    ((p2[0].x - p2[2].x) / 2) +
				    (this->osd_item.w / 2);
				p.y =
				    ((p2[0].y - p2[2].y) / 2) +
				    (this->osd_item.h / 2);
				graphics_draw_text(this->osd_item.gr,
						   this->osd_item.
						   graphic_fg_white, NULL,
						   this->osd_item.font,
						   this->test_text, &p,
						   0x10000, 0);
			} else if (*distance) {
				graphics_get_text_bbox(this->osd_item.gr,
						       this->osd_item.font,
						       distance, 0x10000,
						       0x0, p2, 0);
				p.x =
				    ((p2[0].x - p2[2].x) / 2) +
				    (this->osd_item.w / 2);
				p.y =
				    ((p2[0].y - p2[2].y) / 2) +
				    (this->osd_item.h / 2);
				graphics_draw_text(this->osd_item.gr,
						   this->osd_item.
						   graphic_fg_white, NULL,
						   this->osd_item.font,
						   distance, &p, 0x10000,
						   0);
			}
		}
		graphics_draw_mode(this->osd_item.gr, draw_mode_end);
	}
}

static void
osd_nav_distance_to_next_init(struct osd_nav_distance_to_next *this,
			      struct navit *nav)
{
	osd_set_std_graphic(nav, &this->osd_item);
	navit_add_callback(nav,
			   callback_new_attr_1(callback_cast
					       (osd_nav_distance_to_next_draw),
					       attr_position_coord_geo,
					       this));
	osd_nav_distance_to_next_draw(this, nav, NULL);
}

static struct osd_priv *
osd_nav_distance_to_next_new(struct navit *nav, struct osd_methods *meth,
			     struct attr **attrs)
{
	struct osd_nav_distance_to_next *this =
	    g_new0(struct osd_nav_distance_to_next, 1);
	struct attr *attr;

	this->osd_item.p.x = 20;
	this->osd_item.p.y = -40;
	this->osd_item.w = 60;
	this->osd_item.h = 20;
	this->osd_item.font_size = 200;
	osd_set_std_attr(attrs, &this->osd_item);
	this->active = -1;

	attr = attr_search(attrs, NULL, attr_label);
	if (attr)
		this->test_text = g_strdup(attr->u.str);
	else
		this->test_text = NULL;

	navit_add_callback(nav,
			   callback_new_attr_1(callback_cast
					       (osd_nav_distance_to_next_init),
					       attr_navit, this));
	return (struct osd_priv *) this;
}

struct osd_street_name {
	struct osd_item osd_item;
	int active;
	struct item item;
	char *test_text;
};

static void
osd_street_name_draw(struct osd_street_name *this, struct navit *navit,
		     struct vehicle *v)
{
	struct point p, p2[4];
	char distance[16];
	int do_draw = 0;
	struct attr attr_name1, attr_name2;
	char *name1 = NULL, *name2 = NULL;
	struct tracking *tr = NULL;
	struct map_rect *mr = NULL;
	struct item *item = NULL;
	char *name = NULL;

	distance[0] = '\0';
	if (navit)
		tr = navit_get_tracking(navit);

	if (tr)
		item = tracking_get_current_item(tr);

	dbg(1, "navit=%p tr=%p item=%p\n", navit, tr, item);
	if (item) {
		if (!item_is_equal(*item, this->item)) {
			do_draw = 1;
			this->item = *item;
			mr = map_rect_new(item->map, NULL);
			item =
			    map_rect_get_item_byid(mr, item->id_hi,
						   item->id_lo);
			if (item_attr_get
			    (item, attr_street_name, &attr_name1))
				name1 =
				    map_convert_string(item->map,
						       attr_name1.u.str);
			if (item_attr_get
			    (item, attr_street_name_systematic,
			     &attr_name2))
				name2 =
				    map_convert_string(item->map,
						       attr_name2.u.str);
			printf("name1=%s name2=%s\n", name1, name2);
			map_rect_destroy(mr);
			if (name1 && name2)
				name =
				    g_strdup_printf("%s/%s", name2, name1);
			else
				name = g_strdup(name1 ? name1 : name2);
			map_convert_free(name1);
			map_convert_free(name2);
			this->active = 1;
		}
	} else {
		if (this->item.map || this->active)
			do_draw = 1;
		this->active = 0;
		memset(&this->item, 0, sizeof(this->item));
		name = NULL;
	}

	if (do_draw) {
		dbg(1, "name=%s\n", name);
		graphics_draw_mode(this->osd_item.gr, draw_mode_begin);
		p.x = 0;
		p.y = 0;
		graphics_draw_rectangle(this->osd_item.gr,
					this->osd_item.graphic_bg, &p,
					32767, 32767);
		if (this->test_text) {
			graphics_get_text_bbox(this->osd_item.gr,
					       this->osd_item.font,
					       this->test_text, 0x10000,
					       0x0, p2, 0);
			p.y =
			    ((p2[0].y - p2[2].y) / 2) +
			    (this->osd_item.h / 2);
			graphics_draw_text(this->osd_item.gr,
					   this->osd_item.graphic_fg_white,
					   NULL, this->osd_item.font,
					   this->test_text, &p, 0x10000,
					   0);
		} else if (name) {
			graphics_get_text_bbox(this->osd_item.gr,
					       this->osd_item.font, name,
					       0x10000, 0x0, p2, 0);
			p.y =
			    ((p2[0].y - p2[2].y) / 2) +
			    (this->osd_item.h / 2);
			graphics_draw_text(this->osd_item.gr,
					   this->osd_item.graphic_fg_white,
					   NULL, this->osd_item.font, name,
					   &p, 0x10000, 0);
		}
		graphics_draw_mode(this->osd_item.gr, draw_mode_end);
	}
	g_free(name);
}

static void
osd_street_name_init(struct osd_street_name *this, struct navit *nav)
{
	osd_set_std_graphic(nav, &this->osd_item);
	navit_add_callback(nav,
			   callback_new_attr_1(callback_cast
					       (osd_street_name_draw),
					       attr_position_coord_geo,
					       this));
	osd_street_name_draw(this, nav, NULL);
}

static struct osd_priv *
osd_street_name_new(struct navit *nav, struct osd_methods *meth,
		    struct attr **attrs)
{
	struct osd_street_name *this = g_new0(struct osd_street_name, 1);
	struct attr *attr;

	this->osd_item.p.x = 0;
	this->osd_item.p.y = -40;
	this->osd_item.w = 150;
	this->osd_item.h = 40;
	this->osd_item.font_size = 200;
	osd_set_std_attr(attrs, &this->osd_item);

	attr = attr_search(attrs, NULL, attr_label);
	if (attr)
		this->test_text = g_strdup(attr->u.str);
	else
		this->test_text = NULL;

	this->active = -1;

	navit_add_callback(nav,
			   callback_new_attr_1(callback_cast
					       (osd_street_name_init),
					       attr_navit, this));
	return (struct osd_priv *) this;
}

struct osd_button {
	struct point p;
	struct navit *nav;
	struct graphics *gra;
	struct graphics_gc *gc;
	struct callback *navit_init_cb;
	struct callback *draw_cb;
	struct graphics_image *img;
	int pressed;
	char *src;
	char *command;
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
osd_button_click(struct osd_button *this, struct navit *nav, int pressed,
		 int button, struct point *p)
{
	struct point bp = this->p;
	wrap_point(&bp, this->nav);
	if ((p->x < bp.x || p->y < bp.y || p->x > bp.x + this->img->width
	     || p->y > bp.y + this->img->height) && !this->pressed)
		return;
	navit_ignore_button(nav);
	this->pressed = pressed;
	if (pressed) {
		dbg(0, "calling command '%s'\n", this->command);
		navit_command_call(nav, this->command);
	}
}

static void
osd_button_draw(struct osd_button *this)
{
	struct point bp = this->p;
	wrap_point(&bp, this->nav);
	graphics_draw_image(this->gra, this->gc, &bp, this->img);
}

static void
osd_button_init(struct osd_button *this, struct navit *nav)
{
	struct graphics *gra = navit_get_graphics(nav);
	dbg(1, "enter\n");
	this->nav = nav;
	this->gra = gra;
	this->gc = graphics_gc_new(gra);
	this->img = graphics_image_new(gra, this->src);
	if (!this->img) {
		dbg(0, "failed to load '%s'\n", this->src);
	} else {
		navit_add_callback(nav, this->navit_init_cb =
				   callback_new_attr_1(callback_cast
						       (osd_button_click),
						       attr_button, this));
		graphics_add_callback(gra, this->draw_cb =
				      callback_new_attr_1(callback_cast
							  (osd_button_draw),
							  attr_postdraw,
							  this));
	}
}

static struct osd_priv *
osd_button_new(struct navit *nav, struct osd_methods *meth,
	       struct attr **attrs)
{
	struct osd_button *this = g_new0(struct osd_button, 1);
	struct attr *attr;
	attr = attr_search(attrs, NULL, attr_x);
	if (attr)
		this->p.x = attr->u.num;
	attr = attr_search(attrs, NULL, attr_y);
	if (attr)
		this->p.y = attr->u.num;
	attr = attr_search(attrs, NULL, attr_command);
	if (!attr) {
		dbg(0, "no command\n");
		goto error;
	}
	this->command = g_strdup(attr->u.str);
	attr = attr_search(attrs, NULL, attr_src);
	if (!attr) {
		dbg(0, "no src\n");
		goto error;
	}
	this->src =
	    g_strjoin(NULL, getenv("NAVIT_SHAREDIR"), "/xpm/", attr->u.str,
		      NULL);
	navit_add_callback(nav, this->navit_init_cb =
			   callback_new_attr_1(callback_cast
					       (osd_button_init),
					       attr_navit, this));

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
		graphics_draw_mode(this->osd_item.gr, draw_mode_begin);
		p.x = 0;
		p.y = 0;
		graphics_draw_rectangle(this->osd_item.gr,
					this->osd_item.graphic_bg, &p,
					this->osd_item.w,
					this->osd_item.h);
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
	navit_add_callback(nav,
			   callback_new_attr_1(callback_cast
					       (osd_nav_next_turn_draw),
					       attr_position_coord_geo,
					       this));
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
	osd_set_std_attr(attrs, &this->osd_item);

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

	navit_add_callback(nav,
			   callback_new_attr_1(callback_cast
					       (osd_nav_next_turn_init),
					       attr_navit, this));
	return (struct osd_priv *) this;
}

struct nav_next_street_name {
	struct osd_item osd_item;
	int active;
	char *last_street_name;
	char *test_text;
};

static void
osd_nav_next_street_name_draw(struct nav_next_street_name *this,
			      struct navit *navit)
{

	int do_draw = 0;
	char *name_next = NULL;
	struct navigation *nav = NULL;
	struct map *map = NULL;
	struct map_rect *mr = NULL;
	struct item *item = NULL;
	struct attr attr;
	struct point p, p2[4];

	if (navit)
		nav = navit_get_navigation(navit);

	if (nav)
		map = navigation_get_map(nav);

	if (map)
		mr = map_rect_new(map, NULL);

	if (mr)
		while ((item = map_rect_get_item(mr))
		       && item->type == type_nav_position);

	if (item && item_attr_get(item, attr_street_name, &attr)) {
		if (item->type == type_nav_destination) {
			name_next = _("reaching destination");
		} else {
			name_next = attr.u.str;
		}

		if (this->active != 1
		    || strcmp(this->last_street_name, name_next)) {
			this->active = 1;
			if (this->last_street_name)
				g_free(this->last_street_name);
			this->last_street_name = g_strdup(name_next);
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

	if (do_draw || this->test_text) {
		graphics_draw_mode(this->osd_item.gr, draw_mode_begin);
		p.x = 0;
		p.y = 0;

		graphics_draw_rectangle(this->osd_item.gr,
					this->osd_item.graphic_bg, &p, 32767,
					32767);
		if (this->active) {
			if (name_next) {
				graphics_get_text_bbox(this->osd_item.gr,
						       this->osd_item.font,
						       name_next, 0x10000,
						       0x0, p2, 0);
				p.y =
				    ((p2[0].y - p2[2].y) / 2) +
				    (this->osd_item.h / 2);
				graphics_draw_text(this->osd_item.gr,
						   this->osd_item.
						   graphic_fg_white, NULL,
						   this->osd_item.font,
						   name_next, &p, 0x10000,
						   0);
			}
		} else if (this->test_text) {
			graphics_get_text_bbox(this->osd_item.gr,
					       this->osd_item.font,
					       this->test_text, 0x10000,
					       0x0, p2, 0);
			p.y =
			    ((p2[0].y - p2[2].y) / 2) + (this->osd_item.h / 2);
			graphics_draw_text(this->osd_item.gr,
					   this->osd_item.graphic_fg_white,
					   NULL, this->osd_item.font,
					   this->test_text, &p, 0x10000,
					   0);
		}
		graphics_draw_mode(this->osd_item.gr, draw_mode_end);
	}

}

static void
osd_nav_next_street_name_init(struct nav_next_street_name *this,
			      struct navit *nav)
{

	osd_set_std_graphic(nav, &this->osd_item);
	navit_add_callback(nav,
			   callback_new_attr_1(callback_cast
					       (osd_nav_next_street_name_draw),
					       attr_position_coord_geo,
					       this));
	osd_nav_next_street_name_draw(this, nav);
}

static struct osd_priv *
osd_nav_next_street_name_new(struct navit *nav, struct osd_methods *meth,
			     struct attr **attrs)
{

	struct nav_next_street_name *this =
	    g_new0(struct nav_next_street_name, 1);
	struct attr *attr;

	this->osd_item.p.x = 0;
	this->osd_item.p.y = -40;
	this->osd_item.w = 150;
	this->osd_item.h = 40;
	this->osd_item.font_size = 200;
	osd_set_std_attr(attrs, &this->osd_item);

	attr = attr_search(attrs, NULL, attr_label);
	if (attr)
		this->test_text = g_strdup(attr->u.str);
	else
		this->test_text = NULL;

	this->active = -1;
	this->last_street_name = NULL;

	navit_add_callback(nav,
			   callback_new_attr_1(callback_cast
					       (osd_nav_next_street_name_init),
					       attr_navit, this));
	return (struct osd_priv *) this;
}

void
plugin_init(void)
{
	plugin_register_osd_type("compass", osd_compass_new);
	plugin_register_osd_type("eta", osd_eta_new);
	plugin_register_osd_type("navigation_distance_to_target",
				 osd_nav_distance_to_target_new);
	plugin_register_osd_type("navigation_distance_to_next",
				 osd_nav_distance_to_next_new);
	plugin_register_osd_type("navigation_next_turn",
				 osd_nav_next_turn_new);
	plugin_register_osd_type("street_name", osd_street_name_new);
	plugin_register_osd_type("navigation_next_street_name",
				 osd_nav_next_street_name_new);
	plugin_register_osd_type("button", osd_button_new);
	plugin_register_osd_type("vehicle_gps_satnum", osd_sats_new);
	plugin_register_osd_type("vehicle_speed", osd_speed_new);

/*
        plugin_register_osd_type("position_max_speed", osd_position_max_speed_new);
        plugin_register_osd_type("vehicle_pos", osd_vehicle_pos_new);
*/
}
