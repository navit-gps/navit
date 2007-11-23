#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include <libintl.h>
#include "debug.h"
#include "profile.h"
#include "navigation.h"
#include "coord.h"
#include "item.h"
#include "route.h"
#include "transform.h"
#include "mapset.h"
#include "projection.h"
#include "map.h"
#include "navit.h"
#include "callback.h"

#define _(STRING)    gettext(STRING)

struct navigation {
	struct mapset *ms;
	struct item_hash *hash;
	struct navigation_itm *first;
	struct navigation_itm *last;
	struct navigation_command *cmd_first;
	struct navigation_command *cmd_last;
	struct callback_list *callback_speech;
	struct callback_list *callback;
	int level_last;
	struct item item_last;
	int turn_around;
	int distance_turn;
	int distance_last;
	int announce[route_item_last-route_item_first+1][3];
};


struct navigation_command {
	struct navigation_itm *itm;
	struct navigation_command *next;
	int delta;
};

struct navigation_list {
	struct navigation *nav;
	struct navigation_command *cmd;
	struct navigation_command *cmd_next;
	struct navigation_itm *itm;
	struct navigation_itm *itm_next;
	struct item item;
#if 0
	char *str;
#endif
};

struct navigation *
navigation_new(struct mapset *ms)
{
	int i,j;
	struct navigation *ret=g_new0(struct navigation, 1);
	ret->ms=ms;
	ret->hash=item_hash_new();
	ret->callback=callback_list_new();
	ret->callback_speech=callback_list_new();
	ret->level_last=-2;
	ret->distance_last=-2;
	ret->distance_turn=50;

	for (j = 0 ; j <= route_item_last-route_item_first ; j++) {
		for (i = 0 ; i < 3 ; i++) {
			ret->announce[j][i]=-1;
		}
	}

	return ret;	
}

void
navigation_set_mapset(struct navigation *this_, struct mapset *ms)
{
	this_->ms=ms;
}

int
navigation_set_announce(struct navigation *this_, enum item_type type, int *level)
{
	int i;
	if (type < route_item_first || type > route_item_last) {
		dbg(0,"street type %d out of range [%d,%d]", type, route_item_first, route_item_last);
		return 0;
	}
	for (i = 0 ; i < 3 ; i++) 
		this_->announce[type-route_item_first][i]=level[i];
	return 1;
}

static int
navigation_get_announce_level(struct navigation *this_, enum item_type type, int dist)
{
	int i;

	if (type < route_item_first || type > route_item_last)
		return -1;
	for (i = 0 ; i < 3 ; i++) {
		if (dist <= this_->announce[type-route_item_first][i])
			return i;
	}
	return i;
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

static int
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
get_distance(int dist, enum attr_type type, int is_length)
{
	if (type == attr_navigation_long) {
		if (is_length)
			return g_strdup_printf(_("%d m"), dist);
		else
			return g_strdup_printf(_("in %d m"), dist);
	}
	if (dist < 1000) {
		if (is_length)
			return g_strdup_printf(_("%d meters"), dist);
		else
			return g_strdup_printf(_("in %d meters"), dist);
	}
	if (dist < 5000) {
		int rem=(dist/100)%10;
		if (rem) {
			if (is_length)
				return g_strdup_printf(_("%d.%d kilometer"), dist/1000, rem);
			else
				return g_strdup_printf(_("in %d.%d kilometers"), dist/1000, rem);
		}
	}
	switch (dist) {
	case 1000:
		if (is_length) 
			return g_strdup_printf(_("one kilometer"));
		else
			return g_strdup_printf(_("in one kilometer"));
	case 2000:
		if (is_length) 
			return g_strdup_printf(_("two kilometers"));
		else
			return g_strdup_printf(_("in two kilometers"));
	case 3000:
		if (is_length) 
			return g_strdup_printf(_("three kilometers"));
		else
			return g_strdup_printf(_("in three kilometers"));
	case 4000:
		if (is_length) 
			return g_strdup_printf(_("four kilometers"));
		else
			return g_strdup_printf(_("in four kilometers"));
	default:
		if (is_length) 
			return g_strdup_printf(_("%d kilometers"), dist/1000);
		else
			return g_strdup_printf(_("in %d kilometers"), dist/1000);
	}
}

static void
navigation_destroy_itms_cmds(struct navigation *this_, struct navigation_itm *end)
{
	struct navigation_itm *itm;
	struct navigation_command *cmd;
	dbg(2,"enter this_=%p end=%p\n", this_, end);
	while (this_->first && this_->first != end) {
		itm=this_->first;
		dbg(3,"destroying %p\n", itm);
		item_hash_remove(this_->hash, &itm->item);
		this_->first=itm->next;
		if (this_->first)
			this_->first->prev=NULL;
		if (this_->cmd_first && this_->cmd_first->itm == itm) {
			cmd=this_->cmd_first;
			this_->cmd_first=cmd->next;
			g_free(cmd);
		}
		g_free(itm);
	}
	if (! this_->first)
		this_->last=NULL;
	if (! this_->first && end) 
		dbg(0,"end wrong\n");
	dbg(2,"ret\n");
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
		ret->item=*item;
		item_hash_insert(this_->hash, item, ret);
		mr=map_rect_new(item->map, NULL);
		item=map_rect_get_item_byid(mr, item->id_hi, item->id_lo);
		if (item_attr_get(item, attr_street_name, &attr))
			ret->name1=g_strdup(attr.u.str);
		if (item_attr_get(item, attr_street_name_systematic, &attr))
			ret->name2=g_strdup(attr.u.str);
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
	dbg(1,"ret=%p\n", ret);
	this_->last=ret;
	return ret;
}

static void
calculate_dest_distance(struct navigation *this_, int incr)
{
	int len=0, time=0;
	struct navigation_itm *next,*itm=this_->last;
	dbg(1, "enter this_=%p, incr=%d\n", this_, incr);
	if (incr) {
		if (itm)
			dbg(2, "old values: (%p) time=%d lenght=%d\n", itm, itm->dest_length, itm->dest_time);
		else
			dbg(2, "old values: itm is null\n");
		itm=this_->first;
		next=itm->next;
		dbg(2, "itm values: time=%d lenght=%d\n", itm->length, itm->time);
		dbg(2, "next values: (%p) time=%d lenght=%d\n", next, next->dest_length, next->dest_time);
		itm->dest_length=next->dest_length+itm->length;
		itm->dest_time=next->dest_time+itm->time;
		dbg(2, "new values: time=%d lenght=%d\n", itm->dest_length, itm->dest_time);
		return;
	}
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
show_maneuver(struct navigation *nav, struct navigation_itm *itm, struct navigation_command *cmd, enum attr_type type)
{
	char *dir=_("right"),*strength="";
	int distance=itm->dest_length-cmd->itm->dest_length;
	char *d,*ret;
	int delta=cmd->delta;
	int level,i,pos[3];
	char *arg[3],*str[3];
	level=1;
	if (delta < 0) {
		dir=_("left");
		delta=-delta;
	}
	if (delta < 45) {
		strength=_("easily ");
	} else if (delta < 105) {
		strength="";
	} else if (delta < 165) {
		strength=_("strongly ");
	} else {
		dbg(1,"delta=%d\n", delta);
		strength=_("unknown ");
	}
	if (type != attr_navigation_long_exact) 
		distance=round_distance(distance);
	if (type == attr_navigation_speech) {
		if (nav->turn_around) 
			return g_strdup(_("When possible, please turn around"));
		level=navigation_get_announce_level(nav, itm->item.type, distance);
		dbg(1,"distance=%d level=%d type=0x%x\n", distance, level, itm->item.type);
	}
	switch(level) {
	case 3:
		d=get_distance(distance, type, 1);
		ret=g_strdup_printf(_("Follow the road for the next %s"), d);
		g_free(d);
		return ret;
	case 2:
		d=g_strdup(_("soon"));
		break;
	case 1:
		d=get_distance(distance, type, 0);
		break;
	case 0:
		d=g_strdup(_("now"));
		break;
	default:
		d=g_strdup(_("error"));
	}
	if (cmd->itm->next) {
		for (i = 0 ; i < 3 ; i++) 
			arg[i]=NULL;
		pos[0]=atoi(_("strength_pos"));
		str[0]=strength;
		pos[1]=atoi(_("direction_pos"));
		str[1]=dir;
		pos[2]=atoi(_("distance_pos"));
		str[2]=d;
		
		for (i = 0 ; i < 3 ; i++) {
			if (pos[i] > 0 && pos[i] < 4) {
				arg[pos[i]-1]=str[i];
			} else
				arg[i]=str[i];
		}
		
		ret=g_strdup_printf(_("Turn %s%s %s"), arg[0], arg[1], arg[2]);
	}
	else
		ret=g_strdup_printf(_("You have reached your destination %s"), d);
	g_free(d);
	return ret;
}

static int
navigation_list_item_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr)
{
	struct navigation_list *this_=priv_data;
	switch(attr_type) {
	case attr_navigation_short:
	case attr_navigation_long:
	case attr_navigation_long_exact:
	case attr_navigation_speech:
		attr->u.str=show_maneuver(this_->nav, this_->itm, this_->cmd, attr_type);
		return 1;
	default:
		return 0;
	}
}

struct item_methods navigation_list_item_methods = {
	NULL,
	NULL,
	NULL,
	navigation_list_item_attr_get,
};



struct navigation_list *
navigation_list_new(struct navigation *this_)
{
	struct navigation_list *ret=g_new0(struct navigation_list, 1);
	ret->nav=this_;
	ret->cmd_next=this_->cmd_first;
	ret->itm_next=this_->first;
	ret->item.meth=&navigation_list_item_methods;
	ret->item.priv_data=ret;
	return ret;
}

struct item *
navigation_list_get_item(struct navigation_list *this_)
{
	if (!this_->cmd_next)
		return NULL;
	this_->cmd=this_->cmd_next;
	this_->itm=this_->itm_next;
#if 0
	this_->str=show_maneuver(this_->nav, this_->itm, this_->cmd, mode);
#endif
	this_->itm_next=this_->cmd->itm;
	this_->cmd_next=this_->cmd->next;

	return &this_->item;
}


void
navigation_list_destroy(struct navigation_list *this_)
{
	g_free(this_);
}

static void
navigation_call_callbacks(struct navigation *this_, int force_speech)
{
	int distance, level = 0;
	void *p=this_;
	callback_list_call(this_->callback, 1, &p);
	distance=round_distance(this_->first->dest_length-this_->cmd_first->itm->dest_length);
	if (this_->turn_around) {
		if (distance > this_->distance_turn) {
			this_->level_last=4;
			level=4;
			force_speech=1;
			if (this_->distance_turn > 500)
				this_->distance_turn*=2;
			else
				this_->distance_turn=500;
		}
	} else {
		this_->distance_turn=50;
		level=navigation_get_announce_level(this_, this_->first->item.type, distance);
		if (level < this_->level_last) {
			dbg(0,"level %d < %d\n", level, this_->level_last);
			this_->level_last=level;
			force_speech=1;
		}
		if (!item_is_equal(this_->cmd_first->itm->item, this_->item_last)) {
			dbg(0,"item different\n");
			this_->item_last=this_->cmd_first->itm->item;
			force_speech=1;
		}
	}
	if (force_speech) {
		this_->level_last=level;
		dbg(0,"distance=%d level=%d type=0x%x\n", distance, level, this_->first->item.type);
		callback_list_call(this_->callback_speech, 1, &p);
	}
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
	int incr;

	profile(0,NULL);
	pos=route_get_pos(route);
	dst=route_get_dst(route);
	if (! pos || ! dst)
		return;
	speedlist=route_get_speedlist(route);
	len=route_info_length(pos, dst, 0);
	dbg(2,"len pos,dst = %d\n", len);
	if (len == -1) {
		len=route_info_length(pos, NULL, 0);
		dbg(2,"len pos = %d\n", len);
		end_flag=1;
	}
	sd=route_info_street(pos);
	itm=item_hash_lookup(this_->hash, &sd->item);
	dbg(2,"itm for item with id (0x%x,0x%x) is %p\n", sd->item.id_hi, sd->item.id_lo, itm);
	navigation_destroy_itms_cmds(this_, itm);
	if (itm) 
		incr=1;
	else {
		itm=navigation_itm_new(this_, &sd->item, route_info_point(pos, -1));
		incr=0;
	}
	itm->length=len;
	itm->time=route_time(speedlist, &sd->item, len);
	dbg(2,"%p time = %d\n", itm, itm->time);
	if (!incr) {
		printf("not on track\n");
		rph=route_path_open(route);
		if (rph) {
			while((s=route_path_get_segment(rph))) {
				itm=navigation_itm_new(this_, route_path_segment_get_item(s),route_path_segment_get_start(s));
				itm->time=route_path_segment_get_time(s);
				itm->length=route_path_segment_get_length(s);
			}
			route_path_close(rph);
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
		make_maneuvers(this_);
	}
	calculate_dest_distance(this_, incr);
	dbg(2,"destination distance old=%d new=%d\n", this_->distance_last, this_->first->dest_length);
	if (this_->first->dest_length > this_->distance_last && this_->distance_last >= 0) 
		this_->turn_around=1;
	else
		this_->turn_around=0;
	dbg(2,"turn_around=%d\n", this_->turn_around);
	this_->distance_last=this_->first->dest_length;
	profile(0,"end");
	navigation_call_callbacks(this_, FALSE);
}

void
navigation_destroy(struct navigation *this_)
{
	item_hash_destroy(this_->hash);
	callback_list_destroy(this_->callback);
	callback_list_destroy(this_->callback_speech);
	g_free(this_);
}

int
navigation_register_callback(struct navigation *this_, enum attr_type type, struct callback *cb)
{
	if (type == attr_navigation_speech)
		callback_list_add(this_->callback_speech, cb);
	else
		callback_list_add(this_->callback, cb);
	return 1;
}

void
navigation_unregister_callback(struct navigation *this_, enum attr_type type, struct callback *cb)
{
	if (type == attr_navigation_speech)
		callback_list_remove_destroy(this_->callback_speech, cb);
	else
		callback_list_remove_destroy(this_->callback, cb);
}
