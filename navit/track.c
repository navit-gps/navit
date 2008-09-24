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
#include "plugin.h"

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
	struct route *rt;
	struct map *map;
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
	struct coord curr[2], curr_in, curr_out;
	int curr_angle;
	struct coord last[2], last_in, last_out;
};


int angle_factor=10;
int connected_pref=10;
int nostop_pref=10;
int offroad_limit_pref=5000;
int route_pref=300;


struct coord *
tracking_get_pos(struct tracking *tr)
{
	return &tr->curr_out;
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
tracking_angle_delta(int vehicle_angle, int street_angle, int flags)
{
	int full=180;
	int ret;
	if (flags) {
		full=360;
		if (flags & 2) {
			street_angle=(street_angle+180)%360;
			if (flags & 1)
				return 360*360;
		}
	}
	ret=tracking_angle_abs_diff(vehicle_angle, street_angle, full);
	
	return ret*ret;
}

static int
tracking_is_connected(struct coord *c1, struct coord *c2)
{
	if (c1[0].x == c2[0].x && c1[0].y == c2[0].y)
		return 0;
	if (c1[0].x == c2[1].x && c1[0].y == c2[1].y)
		return 0;
	if (c1[1].x == c2[0].x && c1[1].y == c2[0].y)
		return 0;
	if (c1[1].x == c2[1].x && c1[1].y == c2[1].y)
		return 0;
	return connected_pref;
}

static int
tracking_is_no_stop(struct coord *c1, struct coord *c2)
{
	if (c1->x == c2->x && c1->y == c2->y)
		return nostop_pref;
	return 0;
}

static int
tracking_is_on_route(struct route *rt, struct item *item)
{
	if (! rt)
		return 0;
	if (route_pos_contains(rt, item))
		return 0;
	if (route_contains(rt, item))
		return 0;
	return route_pref;
}

static int
tracking_value(struct tracking *tr, struct tracking_line *t, int offset, struct coord *lpnt, int min, int flags)
{
	int value=0;
	struct street_data *sd=t->street;
	dbg(2, "%d: (0x%x,0x%x)-(0x%x,0x%x)\n", offset, sd->c[offset].x, sd->c[offset].y, sd->c[offset+1].x, sd->c[offset+1].y);
	if (flags & 1) 
		value+=transform_distance_line_sq(&sd->c[offset], &sd->c[offset+1], &tr->curr_in, lpnt);
	if (value >= min)
		return value;
	if (flags & 2) 
		value += tracking_angle_delta(tr->curr_angle, t->angle[offset], sd->flags)*angle_factor>>4;
	if (value >= min)
		return value;
	if (flags & 4) 
		value += tracking_is_connected(tr->last, &sd->c[offset]);
	if (flags & 8) 
		value += tracking_is_no_stop(lpnt, &tr->last_out);
	if (value >= min)
		return value;
	if (flags & 16)
		value += tracking_is_on_route(tr->rt, &sd->item);
	return value;
}



int
tracking_update(struct tracking *tr, struct coord *c, int angle)
{
	struct tracking_line *t;
	int i,value,min;
	struct coord lpnt;
#if 0
	int min,dist;
	int debug=0;
#endif
	dbg(1,"enter(%p,%p,%d)\n", tr, c, angle);
	dbg(1,"c=0x%x,0x%x\n", c->x, c->y);

	if (c->x == tr->curr_in.x && c->y == tr->curr_in.y) {
		if (tr->curr_out.x && tr->curr_out.y)
			*c=tr->curr_out;
		return 0;
	}
	tr->last_in=tr->curr_in;
	tr->last_out=tr->curr_out;
	tr->last[0]=tr->curr[0];
	tr->last[1]=tr->curr[1];
	tr->curr_in=*c;
	tr->curr_angle=angle;
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
	min=INT_MAX/2;
	while (t) {
		struct street_data *sd=t->street;
		if ((sd->flags & 3) == 3) {
			t=t->next;
			continue;
		}
		for (i = 0; i < sd->count-1 ; i++) {
			value=tracking_value(tr,t,i,&lpnt,min,-1);
			if (value < min) {
				tr->curr_line=t;
				tr->pos=i;
				tr->curr[0]=sd->c[i];
				tr->curr[1]=sd->c[i+1];
				dbg(1,"lpnt.x=0x%x,lpnt.y=0x%x pos=%d %d+%d+%d+%d=%d\n", lpnt.x, lpnt.y, i, 
					transform_distance_line_sq(&sd->c[i], &sd->c[i+1], c, &lpnt),
					tracking_angle_delta(angle, t->angle[i], 0)*angle_factor,
					tracking_is_connected(tr->last, &sd->c[i]) ? connected_pref : 0,
					lpnt.x == tr->last_out.x && lpnt.y == tr->last_out.y ? nostop_pref : 0,
					value
				);
				tr->curr_out=lpnt;
				min=value;
			}
		}
		t=t->next;
	}
	dbg(1,"tr->curr_line=%p min=%d\n", tr->curr_line, min);
	if (!tr->curr_line || min > offroad_limit_pref)
		return 0;
	dbg(1,"found 0x%x,0x%x\n", tr->curr_out.x, tr->curr_out.y);
	*c=tr->curr_out;
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
tracking_set_route(struct tracking *this, struct route *rt)
{
	this->rt=rt;
}

void
tracking_destroy(struct tracking *tr)
{
	tracking_free_lines(tr);
	g_free(tr);
}

struct map *
tracking_get_map(struct tracking *this_)
{
	if (! this_->map)
		this_->map=map_new((struct attr*[]){
			&(struct attr){attr_type,{"tracking"}},
			&(struct attr){attr_trackingo,.u.tracking=this_},
			&(struct attr){attr_data,{""}},
			&(struct attr){attr_description,{"Tracking"}},
			NULL});
	return this_->map;
}


struct map_priv {
	struct tracking *tracking;
};

struct map_rect_priv {
	struct tracking *tracking;
	struct item item;
	struct tracking_line *curr,*next;
	int coord;
	enum attr_type attr_next;
	int ccount;
	int debug_idx;
	char *str;
};

static int
tracking_map_item_coord_get(void *priv_data, struct coord *c, int count)
{
	struct map_rect_priv *this=priv_data;
	int ret=0;
	dbg(1,"enter\n");
	while (this->ccount < 2 && count > 0) {
		*c=this->curr->street->c[this->ccount+this->coord];
		dbg(1,"coord %d 0x%x,0x%x\n",this->ccount,c->x,c->y);
		this->ccount++;
		ret++;
		c++;
		count--;
	}
	return ret;
}

static int
tracking_map_item_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr)
{
	struct map_rect_priv *this_=priv_data;
	attr->type=attr_type;
	struct coord lpnt,*c;
	struct tracking *tr=this_->tracking;
	int value;

	if (this_->str) {
		g_free(this_->str);
		this_->str=NULL;
	}

	switch(attr_type) {
	case attr_debug:
		switch(this_->debug_idx) {
		case 0:
                        this_->debug_idx++;
			this_->str=attr->u.str=g_strdup_printf("overall: %d",tracking_value(tr, this_->curr, this_->coord, &lpnt, INT_MAX/2, -1));
                        return 1;
		case 1:
			this_->debug_idx++;
			c=&this_->curr->street->c[this_->coord];
			value=tracking_value(tr, this_->curr, this_->coord, &lpnt, INT_MAX/2, 1);
                        this_->str=attr->u.str=g_strdup_printf("distance: (0x%x,0x%x) from (0x%x,0x%x)-(0x%x,0x%x) at (0x%x,0x%x) %d",
				tr->curr_in.x, tr->curr_in.y,
				c[0].x, c[0].y, c[1].x, c[1].y,
				lpnt.x, lpnt.y, value);
			return 1;
		case 2:
			this_->debug_idx++;
                        this_->str=attr->u.str=g_strdup_printf("angle: %d to %d (flags %d) %d",
				tr->curr_angle, this_->curr->angle[this_->coord], this_->curr->street->flags & 3,
				tracking_value(tr, this_->curr, this_->coord, &lpnt, INT_MAX/2, 2));
			return 1;
		case 3:
			this_->debug_idx++;
                        this_->str=attr->u.str=g_strdup_printf("connected: %d", tracking_value(tr, this_->curr, this_->coord, &lpnt, INT_MAX/2, 4));
			return 1;
		case 4:
			this_->debug_idx++;
                        this_->str=attr->u.str=g_strdup_printf("no_stop: %d", tracking_value(tr, this_->curr, this_->coord, &lpnt, INT_MAX/2, 8));
			return 1;
		case 5:
			this_->debug_idx++;
                        this_->str=attr->u.str=g_strdup_printf("route: %d", tracking_value(tr, this_->curr, this_->coord, &lpnt, INT_MAX/2, 16));
			return 1;
		case 6:
			this_->debug_idx++;
                        this_->str=attr->u.str=g_strdup_printf("line %p", this_->curr);
			return 1;
		default:
			this_->attr_next=attr_none;
			return 0;
		}
	case attr_any:
		while (this_->attr_next != attr_none) {
			if (tracking_map_item_attr_get(priv_data, this_->attr_next, attr))
				return 1;
		}
		return 0;
	default:
		attr->type=attr_none;
		return 0;
	}
}

static struct item_methods tracking_map_item_methods = {
	NULL,
	tracking_map_item_coord_get,
	NULL,
	tracking_map_item_attr_get,
};


static void
tracking_map_destroy(struct map_priv *priv)
{
	g_free(priv);
}

static void
tracking_map_rect_init(struct map_rect_priv *priv)
{
	priv->next=priv->tracking->lines;
	priv->curr=NULL;
	priv->coord=0;
	priv->item.id_lo=0;
	priv->item.id_hi=0;
}

static struct map_rect_priv *
tracking_map_rect_new(struct map_priv *priv, struct map_selection *sel)
{
	struct tracking *tracking=priv->tracking;
	struct map_rect_priv *ret=g_new0(struct map_rect_priv, 1);
	ret->tracking=tracking;
	tracking_map_rect_init(ret);
	ret->item.meth=&tracking_map_item_methods;
	ret->item.priv_data=ret;
	ret->item.type=type_tracking_100;
	return ret;
}

static void
tracking_map_rect_destroy(struct map_rect_priv *priv)
{
	g_free(priv);
}

static struct item *
tracking_map_get_item(struct map_rect_priv *priv)
{
	struct item *ret=&priv->item;
	int value;
	struct coord lpnt;

	if (!priv->next)
		return NULL;
	if (! priv->curr || priv->coord + 2 >= priv->curr->street->count) {
		priv->curr=priv->next;
		priv->next=priv->curr->next;
		priv->coord=0;
		priv->item.id_lo=0;
		priv->item.id_hi++;
	} else {
		priv->coord++;
		priv->item.id_lo++;
	}
	value=tracking_value(priv->tracking, priv->curr, priv->coord, &lpnt, INT_MAX/2, -1);
	if (value < 64) 
		priv->item.type=type_tracking_100;
	else if (value < 128)
		priv->item.type=type_tracking_90;
	else if (value < 256)
		priv->item.type=type_tracking_80;
	else if (value < 512)
		priv->item.type=type_tracking_70;
	else if (value < 1024)
		priv->item.type=type_tracking_60;
	else if (value < 2048)
		priv->item.type=type_tracking_50;
	else if (value < 4096)
		priv->item.type=type_tracking_40;
	else if (value < 8192)
		priv->item.type=type_tracking_30;
	else if (value < 16384)
		priv->item.type=type_tracking_20;
	else if (value < 32768)
		priv->item.type=type_tracking_10;
	else
		priv->item.type=type_tracking_0;
	dbg(1,"item %d %d points\n", priv->coord, priv->curr->street->count);
	priv->ccount=0;
	priv->attr_next=attr_debug;
	priv->debug_idx=0;
	return ret;
}

static struct item *
tracking_map_get_item_byid(struct map_rect_priv *priv, int id_hi, int id_lo)
{
	struct item *ret;
	tracking_map_rect_init(priv);
	while ((ret=tracking_map_get_item(priv))) {
		if (ret->id_hi == id_hi && ret->id_lo == id_lo) 
			return ret;
	}
	return NULL;
}

static struct map_methods tracking_map_meth = {
	projection_mg,
	"utf-8",
	tracking_map_destroy,
	tracking_map_rect_new,
	tracking_map_rect_destroy,
	tracking_map_get_item,
	tracking_map_get_item_byid,
	NULL,
	NULL,
	NULL,
};

static struct map_priv *
tracking_map_new(struct map_methods *meth, struct attr **attrs)
{
	struct map_priv *ret;
	struct attr *tracking_attr;

	tracking_attr=attr_search(attrs, NULL, attr_trackingo);
	if (! tracking_attr)
		return NULL;
	ret=g_new0(struct map_priv, 1);
	*meth=tracking_map_meth;
	ret->tracking=tracking_attr->u.tracking;

	return ret;
}


void
tracking_init(void)
{
	plugin_register_map_type("tracking", tracking_map_new);
}
