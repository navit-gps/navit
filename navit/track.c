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
#include "item.h"
#include "attr.h"
#include "track.h"
#include "debug.h"
#include "transform.h"
#include "coord.h"
#include "route.h"
#include "projection.h"
#include "map.h"
#include "mapset.h"

struct tracking_line
{
	struct street_data *street;
#if 0
	long segid;
	int linenum;
	struct coord c[2];
	struct coord lpnt;
	int value;
	int dir;
#endif
	struct tracking_line *next;
	int angle[0];
};


struct tracking {
	struct mapset *ms;
#if 0
	struct transformation t;
#endif
	struct coord last_updated;
	struct tracking_line *lines;
#if 0
	struct tracking_line **last_ptr;
#endif
	struct tracking_line *curr_line;
	int pos;
	struct coord curr[2];
	struct coord last_in;
	struct coord last_out;
};


int angle_factor=30;
int connected_pref=-10;
int nostop_pref=10;
int offroad_limit_pref=5000;


struct coord *
tracking_get_pos(struct tracking *tr)
{
	return &tr->last_out;
}

int
tracking_get_segment_pos(struct tracking *tr)
{
	return tr->pos;
}

struct street_data *
tracking_get_street_data(struct tracking *tr)
{
	return tr->curr_line->street;
}

int
tracking_get_current_attr(struct tracking *_this, enum attr_type type, struct attr *attr)
{
	struct item *item;
	struct map_rect *mr;
	int result=0;
	if (! _this->curr_line || ! _this->curr_line->street)
		return 0;
	item=&_this->curr_line->street->item;
	mr=map_rect_new(item->map,NULL);
	item=map_rect_get_item_byid(mr, item->id_hi, item->id_lo);
	if (item_attr_get(item, type, attr))
		result=1;
	map_rect_destroy(mr);
	return result;
}

struct item *
tracking_get_current_item(struct tracking *_this)
{
	if (! _this->curr_line || ! _this->curr_line->street)
		return NULL;
	return &_this->curr_line->street->item;
}

static void
tracking_get_angles(struct tracking_line *tl)
{
	int i;
	struct street_data *sd=tl->street;
	for (i = 0 ; i < sd->count-1 ; i++) 
		tl->angle[i]=transform_get_angle_delta(&sd->c[i], &sd->c[i+1], 0);
}

static void
tracking_doupdate_lines(struct tracking *tr, struct coord *cc)
{
	int max_dist=1000;
	struct map_selection *sel=route_rect(18, cc, cc, 0, max_dist);
	struct mapset_handle *h;
	struct map *m;
	struct map_rect *mr;
	struct item *item;
	struct street_data *street;
	struct tracking_line *tl;
#if 0
	struct coord c;
#endif

	dbg(1,"enter\n");
        h=mapset_open(tr->ms);
        while ((m=mapset_next(h,1))) {
		mr=map_rect_new(m, sel);
		if (! mr)
			continue;
		while ((item=map_rect_get_item(mr))) {
			if (item->type >= type_street_0 && item->type <= type_ferry) {
				street=street_get_data(item);
				tl=g_malloc(sizeof(struct tracking_line)+(street->count-1)*sizeof(int));
				tl->street=street;
				tracking_get_angles(tl);
				tl->next=tr->lines;
				tr->lines=tl;
			}
		}
		map_rect_destroy(mr);
	}
	mapset_close(h);
	map_selection_destroy(sel);
	dbg(1, "exit\n");
#if 0

	struct transformation t;

	tr->last_ptr=&tr->lines;
	transform_setup_source_rect_limit(&t,c,1000);
	transform_setup_source_rect_limit(&tr->t,c,1000);


	profile_timer(NULL);
	street_get_block(tr->ma,&t,tst_callback,tr);
	profile_timer("end");
#endif
}


static void
tracking_free_lines(struct tracking *tr)
{
	struct tracking_line *tl=tr->lines,*next;
	dbg(1,"enter(tr=%p)\n", tr);

	while (tl) {
		next=tl->next;
		street_data_free(tl->street);
		g_free(tl);
		tl=next;
	}
	tr->lines=NULL;
}

static int
tracking_angle_abs_diff(int a1, int a2, int full)
{
	int ret;

	if (a2 > a1)
		ret=(a2-a1)%full;
	else
		ret=(a1-a2)%full;
	if (ret > full/2)
		ret=full-ret;
	return ret;
}

static int
tracking_angle_delta(int vehicle_angle, int street_angle, int dir)
{
	int full=180;
	int ret;
	if (dir) {
		full=360;
		if (dir < 0)
			street_angle=(street_angle+180)%360;
	}
	ret=tracking_angle_abs_diff(vehicle_angle, street_angle, full);
	
	return ret*ret;
}

static int
tracking_is_connected(struct coord *c1, struct coord *c2)
{
	if (c1[0].x == c2[0].x && c1[0].y == c2[0].y)
		return 1;
	if (c1[0].x == c2[1].x && c1[0].y == c2[1].y)
		return 1;
	if (c1[1].x == c2[0].x && c1[1].y == c2[0].y)
		return 1;
	if (c1[1].x == c2[1].x && c1[1].y == c2[1].y)
		return 1;
	return 0;
}

int
tracking_update(struct tracking *tr, struct coord *c, int angle)
{
	struct tracking_line *t;
	int i,value,min=0;
	struct coord lpnt;
#if 0
	int min,dist;
	int debug=0;
#endif
	dbg(1,"enter(%p,%p,%d)\n", tr, c, angle);
	dbg(1,"c=0x%x,0x%x\n", c->x, c->y);

	if (c->x == tr->last_in.x && c->y == tr->last_in.y) {
		if (tr->last_out.x && tr->last_out.y)
			*c=tr->last_out;
		return 0;
	}
	tr->last_in=*c;
	if (!tr->lines || transform_distance_sq(&tr->last_updated, c) > 250000) {
		dbg(1, "update\n");
		tracking_free_lines(tr);
		tracking_doupdate_lines(tr, c);
		tr->last_updated=*c;
		dbg(1,"update end\n");
	}
		
	t=tr->lines;
	if (! t)
		return 0;
	tr->curr_line=NULL;
	while (t) {
		struct street_data *sd=t->street;
		int dir = 0;
		switch(sd->flags & AF_ONEWAYMASK) {
		case 0:
			dir=0;
			break;
		case 1:
			dir=1;
			break;
		case 2:
			dir=-1;
			break;
		case 3:
			t=t->next;
			continue;
		}
		for (i = 0; i < sd->count-1 ; i++) {
			dbg(2, "%d: (0x%x,0x%x)-(0x%x,0x%x)\n", i, sd->c[i].x, sd->c[i].y, sd->c[i+1].x, sd->c[i+1].y);
			value=transform_distance_line_sq(&sd->c[i], &sd->c[i+1], c, &lpnt);
			if (value < INT_MAX/2) {
				value += tracking_angle_delta(angle, t->angle[i], dir)*angle_factor>>4;
				if (tracking_is_connected(tr->curr, &sd->c[i]))
					value += connected_pref;
				if (lpnt.x == tr->last_out.x && lpnt.y == tr->last_out.y)
					value += nostop_pref;
				if (! tr->curr_line || value < min) {
					tr->curr_line=t;
					tr->pos=i;
					tr->curr[0]=sd->c[i];
					tr->curr[1]=sd->c[i+1];
					dbg(1,"lpnt.x=0x%x,lpnt.y=0x%x pos=%d %d+%d+%d+%d=%d\n", lpnt.x, lpnt.y, i, 
						transform_distance_line_sq(&sd->c[i], &sd->c[i+1], c, &lpnt),
						tracking_angle_delta(angle, t->angle[i], 0)*angle_factor,
						tracking_is_connected(tr->curr, &sd->c[i]) ? connected_pref : 0,
						lpnt.x == tr->last_out.x && lpnt.y == tr->last_out.y ? nostop_pref : 0,
						value
					);
					tr->last_out=lpnt;
					min=value;
				}

			 }
		}
		t=t->next;
	}
	dbg(1,"tr->curr_line=%p min=%d\n", tr->curr_line, min);
	if (!tr->curr_line || min > offroad_limit_pref)
		return 0;
	dbg(1,"found 0x%x,0x%x\n", tr->last_out.x, tr->last_out.y);
	*c=tr->last_out;
	return 1;	
}

struct tracking *
tracking_new(struct mapset *ms)
{
	struct tracking *this=g_new0(struct tracking, 1);
	this->ms=ms;

	return this;
}

void
tracking_set_mapset(struct tracking *this, struct mapset *ms)
{
	this->ms=ms;
}

void
tracking_destroy(struct tracking *tr)
{
	tracking_free_lines(tr);
	g_free(tr);
}
