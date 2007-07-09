#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include "debug.h"
#include "navigation.h"
#include "coord.h"
#include "item.h"
#include "route.h"
#include "transform.h"
#include "mapset.h"
#include "map.h"
#include "navit.h"
#include "callback.h"


struct navigation {
	struct mapset *ms;
	struct navigation_itm *first;
	struct navigation_itm *last;
	struct navigation_command *cmd_first;
	struct navigation_command *cmd_last;
	struct callback_list *callback_speech;
	struct callback_list *callback;
};

struct navigation_command {
	struct navigation_itm *itm;
	struct navigation_command *next;
	int delta;
};

struct navigation_list {
	struct navigation *nav;
	struct navigation_command *cmd;
	struct navigation_itm *itm;
	char *str;
};

struct street_data {
	struct item item;
	int count;
	int limit;
	struct coord c[0];
};


struct navigation *
navigation_new(struct mapset *ms)
{
	struct navigation *ret=g_new(struct navigation, 1);
	ret->ms=ms;
	ret->callback=callback_list_new();
	ret->callback_speech=callback_list_new();

	return ret;	
}

void
navigation_set_mapset(struct navigation *this_, struct mapset *ms)
{
	this_->ms=ms;
}

struct navigation_itm {
	char *name1;
	char *name2;
	struct coord start;
	struct item item;
	int angle_start;
	int angle_end;
	int time;
	int length;
	int dest_time;
	int dest_length;
	struct navigation_itm *next;
	struct navigation_itm *prev;
};

/* 0=N,90=E */
static int
road_angle(struct coord *c1, struct coord *c2, int dir)
{
	int ret=transform_get_angle_delta(c1, c2, dir);
	dbg(1, "road_angle(0x%x,0x%x - 0x%x,0x%x)=%d\n", c1->x, c1->y, c2->x, c2->y, ret);
	return ret;
}

int
round_distance(int dist)
{
	if (dist < 100) {
		dist=(dist+5)/10;
		return dist*10;
	}
	if (dist < 250) {
		dist=(dist+13)/25;
		return dist*25;
	}
	if (dist < 500) {
		dist=(dist+25)/50;
		return dist*50;
	}
	if (dist < 1000) {
		dist=(dist+50)/100;
		return dist*100;
	}
	if (dist < 5000) {
		dist=(dist+50)/100;
		return dist*100;
	}
	if (dist < 100000) {
		dist=(dist+500)/1000;
		return dist*1000;
	}
	dist=(dist+5000)/10000;
	return dist*10000;
}

static char *
get_distance(char *prefix, int dist, enum navigation_mode mode)
{
	if (mode == navigation_mode_long) 
		return g_strdup_printf("%d m", dist);
	if (dist < 1000) 
		return g_strdup_printf(gettext("%s %d metern"), prefix, dist);
	if (dist < 5000) {
		int rem=(dist/100)%10;
		if (rem)
			return g_strdup_printf(gettext("%s %d,%d kilometern"), prefix, dist/1000, rem);
	}
	if ( dist == 1000) 
		return g_strdup_printf(gettext("%s einem kilometer"), prefix);
	return g_strdup_printf(gettext("%s %d kilometer"), prefix, dist/1000);
}

static struct navigation_itm *
navigation_itm_new(struct navigation *this_, struct item *item, struct coord *start)
{
	struct navigation_itm *ret=g_new0(struct navigation_itm, 1);
	int l,i=0,a1,a2,dir=0;
	struct map_rect *mr;
	struct attr attr;
	struct coord c[5];

	if (item) {
		mr=map_rect_new(item->map, NULL);
		item=map_rect_get_item_byid(mr, item->id_hi, item->id_lo);
		if (item_attr_get(item, attr_street_name, &attr))
			ret->name1=g_strdup(attr.u.str);
		if (item_attr_get(item, attr_street_name_systematic, &attr))
			ret->name2=g_strdup(attr.u.str);
		ret->item=*item;
		l=-1;
		while (item_coord_get(item, &c[i], 1)) {
			dbg(1, "coord %d 0x%x 0x%x\n", i, c[i].x ,c[i].y);
			l=i;
			if (i < 4) 
				i++;
			else {
				c[2]=c[3];
				c[3]=c[4];
			}
		}
		dbg(1,"count=%d\n", l);
		if (l == 4)
			l=3;
		if (start->x != c[0].x || start->y != c[0].y)
			dir=-1;
		a1=road_angle(&c[0], &c[1], dir);
		a2=road_angle(&c[l-1], &c[l], dir);
		if (dir >= 0) {
			ret->angle_start=a1;
			ret->angle_end=a2;
		} else {
			ret->angle_start=a2;
			ret->angle_end=a1;
		}
		dbg(1,"i=%d a1 %d a2 %d '%s' '%s'\n", i, a1, a2, ret->name1, ret->name2);
		map_rect_destroy(mr);
	}
	if (start)
		ret->start=*start;
	if (! this_->first)
		this_->first=ret;
	if (this_->last) {
		this_->last->next=ret;
		ret->prev=this_->last;
	}
	this_->last=ret;
	return ret;
}

static void
calculate_dest_distance(struct navigation *this_)
{
	int len=0, time=0;
	struct navigation_itm *itm=this_->last;
	while (itm) {
		len+=itm->length;
		time+=itm->time;
		itm->dest_length=len;
		itm->dest_time=time;
		itm=itm->prev;
	}
	dbg(1,"len %d time %d\n", len, time);
}

static int
is_same_street2(struct navigation_itm *old, struct navigation_itm *new)
{
	if (old->name1 && new->name1 && !strcmp(old->name1, new->name1)) {
		dbg(1,"is_same_street: '%s' '%s' vs '%s' '%s' yes (1.)\n", old->name2, new->name2, old->name1, new->name1);
		return 1;
	}
	if (old->name2 && new->name2 && !strcmp(old->name2, new->name2)) {
		dbg(1,"is_same_street: '%s' '%s' vs '%s' '%s' yes (2.)\n", old->name2, new->name2, old->name1, new->name1);
		return 1;
	}
	dbg(1,"is_same_street: '%s' '%s' vs '%s' '%s' no\n", old->name2, new->name2, old->name1, new->name1);
	return 0;
}

static int
maneuver_required2(struct navigation_itm *old, struct navigation_itm *new, int *delta)
{
	dbg(1,"enter %p %p %p\n",old, new, delta);
	if (new->item.type == type_ramp && old && (old->item.type == type_highway_land || old->item.type == type_highway_city)) {
		dbg(1, "maneuver_required: new is ramp from highway: yes\n");
		return 1;
	}
	if (is_same_street2(old, new)) {
		dbg(1, "maneuver_required: is_same_street: no\n");
		return 0;
	}
#if 0
	if (old->crossings_end == 2) {
		dbg(1, "maneuver_required: only 2 connections: no\n");
		return 0;
	}
#endif
	*delta=new->angle_start-old->angle_end;
	if (*delta < -180)
		*delta+=360;
	if (*delta > 180)
		*delta-=360;
	if (*delta < 20 && *delta >-20) {
		dbg(1, "maneuver_required: delta(%d) < 20: no\n", *delta);
		return 0;
	}
	dbg(1, "maneuver_required: delta=%d: yes\n", *delta);
	return 1;
}

static struct navigation_command *
command_new(struct navigation *this_, struct navigation_itm *itm, int delta)
{
	struct navigation_command *ret=g_new0(struct navigation_command, 1);
	ret->delta=delta;
	ret->itm=itm;
	if (this_->cmd_last)
		this_->cmd_last->next=ret;
	this_->cmd_last=ret;
	if (!this_->cmd_first)
		this_->cmd_first=ret;
	return ret;
}

static void
make_maneuvers(struct navigation *this_)
{
	struct navigation_itm *itm, *last=NULL, *last_itm=NULL;
	itm=this_->first;
	int delta;
	this_->cmd_last=NULL;
	this_->cmd_first=NULL;
	while (itm) {
		if (last) {
			if (maneuver_required2(last_itm, itm, &delta)) {
				command_new(this_, itm, delta);
			}
		} else
			last=itm;
		last_itm=itm;
		itm=itm->next;
	}
}

static char *
show_maneuver(struct navigation_itm *itm, struct navigation_command *cmd, enum navigation_mode mode)
{
	char *dir="rechts",*strength="";
	int distance=itm->dest_length-cmd->itm->dest_length;
	char *d,*ret;
	int delta=cmd->delta;
	if (delta < 0) {
		dir="links";
		delta=-delta;
	}
	if (delta < 45) {
		strength="leicht ";
	} else if (delta < 105) {
		strength="";
	} else if (delta < 165) {
		strength="scharf ";
	} else {
		dbg(0,"delta=%d\n", delta);
		strength="unbekannt ";
	}
	d=get_distance("In", round_distance(distance), mode);
	if (cmd->itm->next)
		ret=g_strdup_printf("%s %s%s abbiegen", d, strength, dir);
	else
		ret=g_strdup_printf("%s haben Sie ihr Ziel erreicht", d);
	g_free(d);
	return ret;
}

struct navigation_list *
navigation_list_new(struct navigation *this_)
{
	struct navigation_list *ret=g_new0(struct navigation_list, 1);
	ret->nav=this_;
	ret->cmd=this_->cmd_first;
	ret->itm=this_->first;
	return ret;
}

char *
navigation_list_get(struct navigation_list *this_, enum navigation_mode mode)
{
	if (!this_->cmd)
		return NULL;
	g_free(this_->str);
	this_->str=show_maneuver(this_->itm, this_->cmd, mode);
	this_->itm=this_->cmd->itm;
	this_->cmd=this_->cmd->next;

	return this_->str;
}


void
navigation_list_destroy(struct navigation_list *this_)
{
	g_free(this_->str);
	g_free(this_);
}

void
navigation_update(struct navigation *this_, struct route *route)
{
	struct route_path_handle *rph;
	struct route_path_segment *s;
	struct navigation_itm *itm;
	struct route_info *pos,*dst;
	struct street_data *sd;
	int *speedlist;
	int len,end_flag=0;
	void *p;


	pos=route_get_pos(route);
	dst=route_get_dst(route);
	if (! pos || ! dst)
		return;
	speedlist=route_get_speedlist(route);
	this_->first=this_->last=NULL;
	len=route_info_length(pos, dst, 0);
	if (len == -1) {
		len=route_info_length(pos, NULL, 0);
		end_flag=1;
	}
	sd=route_info_street(pos);
	itm=navigation_itm_new(this_, &sd->item, route_info_point(pos, -1));
	itm->length=len;
	itm->time=route_time(speedlist, &sd->item, len);
	rph=route_path_open(route);
	while((s=route_path_get_segment(rph))) {
		itm=navigation_itm_new(this_, route_path_segment_get_item(s),route_path_segment_get_start(s));
		itm->time=route_path_segment_get_time(s);
		itm->length=route_path_segment_get_length(s);
	}
	if (end_flag) {
		len=route_info_length(NULL, dst, 0);
		dbg(1, "end %d\n", len);
		sd=route_info_street(dst);
		itm=navigation_itm_new(this_, &sd->item, route_info_point(pos, 2));
		itm->length=len;
		itm->time=route_time(speedlist, &sd->item, len);
	}
	itm=navigation_itm_new(this_, NULL, NULL);
	route_path_close(rph);
	calculate_dest_distance(this_);
	make_maneuvers(this_);
	p=this_;
	callback_list_call(this_->callback, 1, &p);
}

void
navigation_destroy(struct navigation *this_)
{
	callback_list_destroy(this_->callback);
	callback_list_destroy(this_->callback_speech);
	g_free(this_);
}

int
navigation_register_callback(struct navigation *this_, enum navigation_mode mode, struct callback *cb)
{
	callback_list_add(this_->callback, cb);
	return 1;
}

void
navigation_unregister_callback(struct navigation *this_, struct callback *cb)
{
	callback_list_remove_destroy(this_->callback, cb);
}
