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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <glib.h>
#include "debug.h"
#include "coord.h"
#include "transform.h"
#include "projection.h"
#include "point.h"
#include "graphics.h"
#include "vehicle.h"
#include "navit.h"
#include "callback.h"
#include "color.h"
#include "cursor.h"
#include "compass.h"
#include "item.h"
#include "layout.h"
#include "event.h"
/* #include "track.h" */


struct cursor {
	int w,h;
	struct graphics *gra;
	struct callback *animate_callback;
	struct event_timeout *animate_timer;	
	struct point cursor_pnt;
	struct graphics_gc *bg;
	struct attr **attrs;
	struct transformation *trans;
	int sequence;
	int angle;
	int speed;
};



static void
cursor_draw_do(struct cursor *this_, int lazy)
{
	struct point p;
	int speed=this_->speed;
	int angle=this_->angle;

	if (!this_->attrs || !this_->gra)
		return;
	if (!this_->gra)
		return;
	transform_set_angle(this_->trans, -this_->angle);
	graphics_draw_mode(this_->gra, draw_mode_begin);
	p.x=0;
	p.y=0;
	graphics_draw_rectangle(this_->gra, this_->bg, &p, this_->w, this_->h);
	for (;;) {
		struct attr **attr=this_->attrs;
		int sequence=this_->sequence;
		int match=0;
		while (*attr) {
			if ((*attr)->type == attr_itemgra) {
				struct itemgra *itm=(*attr)->u.itemgra;
				dbg(1,"speed %d-%d %d\n", itm->speed_range.min, itm->speed_range.max, speed);
				if (speed >= itm->speed_range.min && speed <= itm->speed_range.max &&  
				    angle >= itm->angle_range.min && angle <= itm->angle_range.max &&  
				    sequence >= itm->sequence_range.min && sequence <= itm->sequence_range.max) {
					graphics_draw_itemgra(this_->gra, itm, this_->trans);
					match=1;
				}
			}
			attr++;
		}
		if (match) {
			break;
		} else {
			if (this_->sequence) 
				this_->sequence=0;
			else
				break;
		}
	}
	graphics_draw_drag(this_->gra, &this_->cursor_pnt);
	graphics_draw_mode(this_->gra, lazy ? draw_mode_end_lazy : draw_mode_end);
}

void
cursor_draw(struct cursor *this_, struct graphics *gra, struct point *pnt, int lazy, int angle, int speed)
{
	if (angle < 0)
		angle+=360;
	dbg(1,"enter this=%p gra=%p pnt=%p lazy=%d dir=%d speed=%d\n", this_, gra, pnt, lazy, angle, speed);
	dbg(1,"point %d,%d\n", pnt->x, pnt->y);
	this_->cursor_pnt=*pnt;
	this_->cursor_pnt.x-=this_->w/2;
	this_->cursor_pnt.y-=this_->h/2;
	this_->angle=angle;
	this_->speed=speed;
	if (!this_->gra) {
		struct color c;
		this_->gra=graphics_overlay_new(gra, &this_->cursor_pnt, this_->w, this_->h, 65535);
		this_->bg=graphics_gc_new(this_->gra);
		c.r=0; c.g=0; c.b=0; c.a=0;
		graphics_gc_set_foreground(this_->bg, &c);
		graphics_background_gc(this_->gra, this_->bg);
	}
	cursor_draw_do(this_, lazy);
}

static void
cursor_animate(struct cursor * this)
{
	this->sequence++;
	cursor_draw_do(this, 0);
}

int
cursor_add_attr(struct cursor *this_, struct attr *attr)
{
        int ret=1;
        switch (attr->type) {
        case attr_itemgra:
                break;
	default:
		ret=0;
        }
        if (ret)
                this_->attrs=attr_generic_add_attr(this_->attrs, attr);
        return ret;
}

struct cursor *
cursor_new(struct attr *parent, struct attr **attrs)
{
	struct cursor *this=g_new0(struct cursor,1);
	struct attr *w,*h, *interval;
	struct pcoord center;
	struct point scenter;
	w=attr_search(attrs, NULL, attr_w);
	h=attr_search(attrs, NULL, attr_h);
	if (! w || ! h)
		return NULL;

	this->w=w->u.num;
	this->h=h->u.num;
	this->trans=transform_new();
	center.pro=projection_screen;
	center.x=0;
	center.y=0;
	transform_setup(this->trans, &center, 16, 0);
	scenter.x=this->w/2;
	scenter.y=this->h/2;
	transform_set_screen_center(this->trans, &scenter);
	interval=attr_search(attrs, NULL, attr_interval);
	if (interval) {
		this->animate_callback=callback_new_1(callback_cast(cursor_animate), this);
		this->animate_timer=event_add_timeout(interval->u.num, 1, this->animate_callback);
	}
	dbg(2,"ret=%p\n", this);
	return this;
}

void
cursor_destroy(struct cursor *this_)
{
	if (this_->animate_callback) {
		callback_destroy(this_->animate_callback);
		event_remove_timeout(this_->animate_timer);
	}
	transform_destroy(this_->trans);
	g_free(this_);
}
