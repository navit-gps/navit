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
#include "plugin.h"

#define _(STRING)    gettext(STRING)

struct suffix {
	char *fullname;
	char *abbrev;
	int sex;
} suffixes[]= {
	{"weg",NULL,1},
	{"platz","pl.",1},
	{"ring",NULL,1},
	{"allee",NULL,2},
	{"gasse",NULL,2},
	{"straße","str.",2},
	{"strasse",NULL,2},
};

struct navigation {
	struct map *map;
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
	int turn_around_limit;
	int distance_turn;
	int distance_last;
	int announce[route_item_last-route_item_first+1][3];
};


struct navigation_command {
	struct navigation_itm *itm;
	struct navigation_command *next;
	int delta;
};

struct navigation *
navigation_new(struct attr **attrs)
{
	int i,j;
	struct navigation *ret=g_new0(struct navigation, 1);
	ret->hash=item_hash_new();
	ret->callback=callback_list_new();
	ret->callback_speech=callback_list_new();
	ret->level_last=-2;
	ret->distance_last=-2;
	ret->distance_turn=50;
	ret->turn_around_limit=3;

	for (j = 0 ; j <= route_item_last-route_item_first ; j++) {
		for (i = 0 ; i < 3 ; i++) {
			ret->announce[j][i]=-1;
		}
	}

	return ret;	
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
	struct item item;
	int angle_start;
	int angle_end;
	int time;
	int length;
	int dest_time;
	int dest_length;
	int told;
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
	if (is_length) 
		return g_strdup_printf(ngettext("one kilometer","%d kilometers", dist/1000), dist/1000);
	else
		return g_strdup_printf(ngettext("in one kilometer","in %d kilometers", dist/1000), dist/1000);
}

static void
navigation_destroy_itms_cmds(struct navigation *this_, struct navigation_itm *end)
{
	struct navigation_itm *itm;
	struct navigation_command *cmd;
	dbg(2,"enter this_=%p this_->first=%p this_->cmd_first=%p end=%p\n", this_, this_->first, this_->cmd_first, end);
	if (this_->cmd_first)
		dbg(2,"this_->cmd_first->itm=%p\n", this_->cmd_first->itm);
	while (this_->first && this_->first != end) {
		itm=this_->first;
		dbg(3,"destroying %p\n", itm);
		item_hash_remove(this_->hash, &itm->item);
		this_->first=itm->next;
		if (this_->first)
			this_->first->prev=NULL;
		if (this_->cmd_first && this_->cmd_first->itm == itm->next) {
			cmd=this_->cmd_first;
			this_->cmd_first=cmd->next;
			g_free(cmd);
		}
		map_convert_free(itm->name1);
		map_convert_free(itm->name2);
		g_free(itm);
	}
	if (! this_->first)
		this_->last=NULL;
	if (! this_->first && end) 
		dbg(0,"end wrong\n");
	dbg(2,"ret this_->first=%p this_->cmd_first=%p\n",this_->first, this_->cmd_first);
}

static void
navigation_itm_update(struct navigation_itm *itm, struct item *ritem)
{
	struct attr length, time;
	if (! item_attr_get(ritem, attr_length, &length)) {
		dbg(0,"no length\n");
		return;
	}
	if (! item_attr_get(ritem, attr_time, &time)) {
		dbg(0,"no time\n");
		return;
	}
	dbg(1,"length=%d time=%d\n", length.u.num, time.u.num);
	itm->length=length.u.num;
	itm->time=time.u.num;
}

static struct navigation_itm *
navigation_itm_new(struct navigation *this_, struct item *ritem)
{
	struct navigation_itm *ret=g_new0(struct navigation_itm, 1);
	int l,i=0;
	struct item *sitem;
	struct attr street_item;
	struct map_rect *mr;
	struct attr attr;
	struct coord c[5];

	if (ritem) {
		ret->told=0;
		if (! item_attr_get(ritem, attr_street_item, &street_item)) {
			dbg(0,"no street item\n");
			return NULL;
		}
		sitem=street_item.u.item;
		ret->item=*sitem;
		item_hash_insert(this_->hash, sitem, ret);
		mr=map_rect_new(sitem->map, NULL);
		sitem=map_rect_get_item_byid(mr, sitem->id_hi, sitem->id_lo);
		if (item_attr_get(sitem, attr_street_name, &attr))
			ret->name1=map_convert_string(sitem->map,attr.u.str);
		if (item_attr_get(sitem, attr_street_name_systematic, &attr))
			ret->name2=map_convert_string(sitem->map,attr.u.str);
		navigation_itm_update(ret, ritem);
		l=-1;
		while (item_coord_get(ritem, &c[i], 1)) {
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
		ret->angle_start=road_angle(&c[0], &c[1], 0);
		ret->angle_end=road_angle(&c[l-1], &c[l], 0);
		dbg(1,"i=%d start %d end %d '%s' '%s'\n", i, ret->angle_start, ret->angle_end, ret->name1, ret->name2);
		map_rect_destroy(mr);
	}
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
	if (new->item.type == old->item.type || (new->item.type != type_ramp && old->item.type != type_ramp)) {
		if (is_same_street2(old, new)) {
			dbg(1, "maneuver_required: is_same_street: no\n");
			return 0;
		}
	} else
		dbg(1, "maneuver_required: old or new is ramp\n");
#if 0
	if (old->crossings_end == 2) {
		dbg(1, "maneuver_required: only 2 connections: no\n");
		return 0;
	}
#endif
	if (new->item.type == type_highway_land || new->item.type == type_highway_city || old->item.type == type_highway_land || old->item.type == type_highway_city) {
		dbg(1, "maneuver_required: highway changed name\n");
		return 1;
	}
	*delta=new->angle_start-old->angle_end;
	if (*delta < -180)
		*delta+=360;
	if (*delta > 180)
		*delta-=360;
	dbg(1,"delta=%d-%d=%d\n", new->angle_start, old->angle_end, *delta);
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
	dbg(1,"enter this_=%p itm=%p delta=%d\n", this_, itm, delta);
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
	int delta;
	itm=this_->first;
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

static int
contains_suffix(char *name, char *suffix)
{
	if (!suffix)
		return 0;
	if (strlen(name) < strlen(suffix))
		return 0;
	return !strcasecmp(name+strlen(name)-strlen(suffix), suffix);
}

static char *
replace_suffix(char *name, char *search, char *replace)
{
	int len=strlen(name)-strlen(search);
	char *ret=g_malloc(len+strlen(replace)+1);
	strncpy(ret, name, len);
	strcpy(ret+len, replace);

	return ret;
}

static char *
navigation_item_destination(struct navigation_itm *itm, struct navigation_itm *next, char *prefix)
{
	char *ret=NULL,*name1,*sep,*name2;
	int i,sex;
	if (! prefix)
		prefix="";
	if(!itm->name1 && !itm->name2 && itm->item.type == type_ramp) {
		dbg(1,">> Next is ramp %lx current is %lx \n", itm->item.type, next->item.type);
			 
		if(next->item.type == type_ramp)
			return NULL;
		if(itm->item.type == type_highway_city || itm->item.type == type_highway_land )
			return g_strdup_printf("%s%s",prefix,_("exit"));				 
		else
			return g_strdup_printf("%s%s",prefix,_("ramp"));
		
	}
	if (!itm->name1 && !itm->name2)
		return NULL;
	if (itm->name1) {
		sex=-1;
		name1=NULL;
		for (i = 0 ; i < sizeof(suffixes)/sizeof(suffixes[0]) ; i++) {
			if (contains_suffix(itm->name1,suffixes[i].fullname)) {
				sex=suffixes[i].sex;
				name1=g_strdup(itm->name1);
				break;
			}
			if (contains_suffix(itm->name1,suffixes[i].abbrev)) {
				sex=suffixes[i].sex;
				name1=replace_suffix(itm->name1, suffixes[i].abbrev, suffixes[i].fullname);
				break;
			}
		}
		if (itm->name2) {
			name2=itm->name2;
			sep=" ";
		} else {
			name2="";
			sep="";
		}
		switch (sex) {
		case -1:
			/* TRANSLATORS: Arguments: 1: Prefix (Space if required) 2: Street Name 3: Separator (Space if required), 4: Systematic Street Name */
			ret=g_strdup_printf(_("%sinto the street %s%s%s"),prefix,itm->name1, sep, name2);
			break;
		case 1:
			/* TRANSLATORS: Arguments: 1: Prefix (Space if required) 2: Street Name 3: Separator (Space if required), 4: Systematic Street Name. Male form. The stuff after | doesn't have to be included */
			ret=g_strdup_printf(_("%sinto the %s%s%s|male form"),prefix,name1, sep, name2);
			break;
		case 2:
			/* TRANSLATORS: Arguments: 1: Prefix (Space if required) 2: Street Name 3: Separator (Space if required), 4: Systematic Street Name. Female form. The stuff after | doesn't have to be included */
			ret=g_strdup_printf(_("%sinto the %s%s%s|female form"),prefix,name1, sep, name2);
			break;
		case 3:
			/* TRANSLATORS: Arguments: 1: Prefix (Space if required) 2: Street Name 3: Separator (Space if required), 4: Systematic Street Name. Neutral form. The stuff after | doesn't have to be included */
			ret=g_strdup_printf(_("%sinto the %s%s%s|neutral form"),prefix,name1, sep, name2);
			break;
		}
		g_free(name1);
			
	} else
		/* TRANSLATORS: gives the name of the next road to turn into (into the E17) */
		ret=g_strdup_printf(_("into the %s"),itm->name2);
	name1=ret;
	while (*name1) {
		switch (*name1) {
		case '|':
			*name1='\0';
			break;
		case '/':
			*name1++=' ';
			break;
		default:
			name1++;
		}
	}
	return ret;
}


static char *
show_maneuver(struct navigation *nav, struct navigation_itm *itm, struct navigation_command *cmd, enum attr_type type)
{
	/* TRANSLATORS: right, as in 'Turn right' */
	char *dir=_("right"),*strength="";
	int distance=itm->dest_length-cmd->itm->dest_length;
	char *d,*ret;
	int delta=cmd->delta;
	int level;
	level=1;
	if (delta < 0) {
		/* TRANSLATORS: left, as in 'Turn left' */
		dir=_("left");
		delta=-delta;
	}
	if (delta < 45) {
		/* TRANSLATORS: Don't forget the ending space */
		strength=_("easily ");
	} else if (delta < 105) {
		strength="";
	} else if (delta < 165) {
		/* TRANSLATORS: Don't forget the ending space */
		strength=_("strongly ");
	} else {
		dbg(1,"delta=%d\n", delta);
		/* TRANSLATORS: Don't forget the ending space */
		strength=_("unknown ");
	}
	if (type != attr_navigation_long_exact) 
		distance=round_distance(distance);
	if (type == attr_navigation_speech) {
		if (nav->turn_around && nav->turn_around == nav->turn_around_limit) 
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
		int tellstreetname = 0;
		char *destination = NULL; 
 
		if(type == attr_navigation_speech) { // In voice mode
			// In Voice Mode only tell the street name in level 1 or in level 0 if level 1
			// was skipped

			if (level == 1) { // we are close to the intersection
				cmd->itm->told = 1; // remeber to be checked when we turn
				tellstreetname = 1; // Ok so we tell the name of the street 
			}

			if (level == 0) {
				if(cmd->itm->told == 0) // we are write at the intersection
					tellstreetname = 1; 
				else
					cmd->itm->told = 0;  // reset just in case we come to the same street again
			}

		}
		else
		     tellstreetname = 1;

		if(tellstreetname) 
			destination=navigation_item_destination(cmd->itm, itm, " ");
		/* TRANSLATORS: The first argument is strength, the second direction, the third distance and the fourth destination Example: 'Turn 'slightly' 'left' in '100 m' 'onto baker street' */
		ret=g_strdup_printf(_("Turn %1$s%2$s %3$s%4$s"), strength, dir, d, destination ? destination:"");
		g_free(destination);
	} else
		ret=g_strdup_printf(_("You have reached your destination %s"), d);
	g_free(d);
	return ret;
}

static void
navigation_call_callbacks(struct navigation *this_, int force_speech)
{
	int distance, level = 0;
	void *p=this_;
	if (!this_->cmd_first)
		return;
	callback_list_call(this_->callback, 1, &p);
	dbg(1,"force_speech=%d turn_around=%d turn_around_limit=%d\n", force_speech, this_->turn_around, this_->turn_around_limit);
	distance=round_distance(this_->first->dest_length-this_->cmd_first->itm->dest_length);
	if (this_->turn_around_limit && this_->turn_around == this_->turn_around_limit) {
		dbg(1,"distance=%d distance_turn=%d\n", distance, this_->distance_turn);
		while (distance > this_->distance_turn) {
			this_->level_last=4;
			level=4;
			force_speech=1;
			if (this_->distance_turn >= 500)
				this_->distance_turn*=2;
			else
				this_->distance_turn=500;
		}
	} else if (!this_->turn_around_limit || this_->turn_around == -this_->turn_around_limit+1) {
		this_->distance_turn=50;
		level=navigation_get_announce_level(this_, this_->first->item.type, distance);
		if (level < this_->level_last) {
			dbg(1,"level %d < %d\n", level, this_->level_last);
			this_->level_last=level;
			force_speech=1;
		}
		if (!item_is_equal(this_->cmd_first->itm->item, this_->item_last)) {
			dbg(1,"item different\n");
			this_->item_last=this_->cmd_first->itm->item;
			force_speech=1;
		}
	}
	if (force_speech) {
		this_->level_last=level;
		dbg(1,"distance=%d level=%d type=0x%x\n", distance, level, this_->first->item.type);
		callback_list_call(this_->callback_speech, 1, &p);
	}
}

void
navigation_update(struct navigation *this_, struct route *route)
{
	struct map *map;
	struct map_rect *mr;
	struct item *ritem,*sitem;
	struct attr street_item;
	struct navigation_itm *itm;
	int incr=0;

	if (! route)
		return;
	map=route_get_map(route);
	if (! map)
		return;
	mr=map_rect_new(map, NULL);
	if (! mr)
		return;
	dbg(1,"enter\n");
	ritem=map_rect_get_item(mr);
	if (ritem) {
		if (!item_attr_get(ritem, attr_street_item, &street_item)) {
			ritem=map_rect_get_item(mr);
			if (! ritem) {
				return;
			}
			if (!item_attr_get(ritem, attr_street_item, &street_item)) {
				dbg(0,"no street item\n");
			}	
		}
		sitem=street_item.u.item;
		dbg(1,"sitem=%p\n", sitem);
		itm=item_hash_lookup(this_->hash, sitem);
		dbg(2,"itm for item with id (0x%x,0x%x) is %p\n", sitem->id_hi, sitem->id_lo, itm);
		navigation_destroy_itms_cmds(this_, itm);
		if (itm) {
			incr=1;
			navigation_itm_update(itm, ritem);
		} else {
			dbg(1,"not on track\n");
			do {
				dbg(1,"item\n");
				navigation_itm_new(this_, ritem);
				ritem=map_rect_get_item(mr);
			} while (ritem);
			itm=navigation_itm_new(this_, NULL);
			make_maneuvers(this_);
		}
		calculate_dest_distance(this_, incr);
		dbg(2,"destination distance old=%d new=%d\n", this_->distance_last, this_->first->dest_length);
		if (this_->first->dest_length > this_->distance_last && this_->distance_last >= 0) 
			this_->turn_around++;
		else
			this_->turn_around--;
		if (this_->turn_around > this_->turn_around_limit)
			this_->turn_around=this_->turn_around_limit;
		else if (this_->turn_around < -this_->turn_around_limit+1)
			this_->turn_around=-this_->turn_around_limit+1;
		dbg(2,"turn_around=%d\n", this_->turn_around);
		this_->distance_last=this_->first->dest_length;
		profile(0,"end");
		navigation_call_callbacks(this_, FALSE);
	} else
		navigation_destroy_itms_cmds(this_, NULL);
	map_rect_destroy(mr);
	
#if 0
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
#endif
}

void
navigation_flush(struct navigation *this_)
{
	navigation_destroy_itms_cmds(this_, NULL);
}


void
navigation_destroy(struct navigation *this_)
{
	navigation_flush(this_);
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

struct map *
navigation_get_map(struct navigation *this_)
{
	struct attr type_attr={attr_type, {"navigation"}};
	struct attr navigation_attr={attr_navigation, .u.navigation=this_};
        struct attr data_attr={attr_data, {""}};
        struct attr *attrs_navigation[]={&type_attr, &navigation_attr, &data_attr, NULL};

	if (! this_->map)
		this_->map=map_new(attrs_navigation);
	return this_->map;
}

struct map_priv {
	struct navigation *navigation;
};

struct map_rect_priv {
	struct navigation *nav;
	struct navigation_command *cmd;
	struct navigation_command *cmd_next;
	struct navigation_itm *itm;
	struct navigation_itm *itm_next;
	struct item item;
};

static int
navigation_map_item_coord_get(void *priv_data, struct coord *c, int count)
{
	return 0;
}

static int
navigation_map_item_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr)
{
	struct map_rect_priv *this_=priv_data;
	attr->type=attr_type;
	switch(attr_type) {
	case attr_navigation_short:
	case attr_navigation_long:
	case attr_navigation_long_exact:
	case attr_navigation_speech:
		attr->u.str=show_maneuver(this_->nav, this_->itm, this_->cmd, attr_type);
		return 1;
	case attr_length:
		attr->u.num=this_->itm->dest_length-this_->cmd->itm->dest_length;
		return 1;
	case attr_time:
		attr->u.num=this_->itm->dest_time-this_->cmd->itm->dest_time;
		return 1;
	case attr_destination_length:
		attr->u.num=this_->itm->dest_length;
		return 1;
	case attr_destination_time:
		attr->u.num=this_->itm->dest_time;
		return 1;
	default:
		attr->type=attr_none;
		return 0;
	}
}

static struct item_methods navigation_map_item_methods = {
	NULL,
	navigation_map_item_coord_get,
	NULL,
	navigation_map_item_attr_get,
};


static void
navigation_map_destroy(struct map_priv *priv)
{
	g_free(priv);
}

static struct map_rect_priv *
navigation_map_rect_new(struct map_priv *priv, struct map_selection *sel)
{
	struct navigation *nav=priv->navigation;
	struct map_rect_priv *ret=g_new0(struct map_rect_priv, 1);
	ret->nav=nav;
	ret->cmd_next=nav->cmd_first;
	ret->itm_next=nav->first;
	ret->item.meth=&navigation_map_item_methods;
	ret->item.priv_data=ret;
	return ret;
}

static void
navigation_map_rect_destroy(struct map_rect_priv *priv)
{
	g_free(priv);
}

static struct item *
navigation_map_get_item(struct map_rect_priv *priv)
{
	struct item *ret;
	int delta;
	if (!priv->cmd_next)
		return NULL;
	ret=&priv->item;	
	priv->cmd=priv->cmd_next;
	priv->itm=priv->itm_next;
	priv->itm_next=priv->cmd->itm;
	priv->cmd_next=priv->cmd->next;

	delta=priv->cmd->delta;	
	dbg(1,"delta=%d\n", delta);
	if (delta < 0) {
		delta=-delta;
		if (delta < 45)
			ret->type=type_nav_left_1;
		else if (delta < 105)
			ret->type=type_nav_left_2;
		else if (delta < 165) 
			ret->type=type_nav_left_3;
		else
			ret->type=type_none;
	} else {
		if (delta < 45)
			ret->type=type_nav_right_1;
		else if (delta < 105)
			ret->type=type_nav_right_2;
		else if (delta < 165) 
			ret->type=type_nav_right_3;
		else
			ret->type=type_none;
	}
	dbg(1,"type=%d\n", ret->type);
	return ret;
}

static struct item *
navigation_map_get_item_byid(struct map_rect_priv *priv, int id_hi, int id_lo)
{
	dbg(0,"stub");
	return NULL;
}

static struct map_methods navigation_map_meth = {
	projection_mg,
	NULL,
	navigation_map_destroy,
	navigation_map_rect_new,
	navigation_map_rect_destroy,
	navigation_map_get_item,
	navigation_map_get_item_byid,
	NULL,
	NULL,
	NULL,
};

static struct map_priv *
navigation_map_new(struct map_methods *meth, struct attr **attrs)
{
	struct map_priv *ret;
	struct attr *navigation_attr;

	navigation_attr=attr_search(attrs, NULL, attr_navigation);
	if (! navigation_attr)
		return NULL;
	ret=g_new0(struct map_priv, 1);
	*meth=navigation_map_meth;
	ret->navigation=navigation_attr->u.navigation;

	return ret;
}


void
navigation_init(void)
{
	plugin_register_map_type("navigation", navigation_map_new);
}
