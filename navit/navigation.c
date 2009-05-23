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
#include <ctype.h>
#include <glib.h>
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
#include "navit_nls.h"

#define DEBUG

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
	struct route *route;
	struct map *map;
	struct item_hash *hash;
	struct navigation_itm *first;
	struct navigation_itm *last;
	struct navigation_command *cmd_first;
	struct navigation_command *cmd_last;
	struct callback_list *callback_speech;
	struct callback_list *callback;
	struct navit *navit;
	int level_last;
	struct item item_last;
	int turn_around;
	int turn_around_limit;
	int distance_turn;
	int distance_last;
	struct callback *route_cb;
	int announce[route_item_last-route_item_first+1][3];
};


struct navigation_command {
	struct navigation_itm *itm;
	struct navigation_command *next;
	struct navigation_command *prev;
	int delta;
	int roundabout_delta;
	int length;
};

static void navigation_flush(struct navigation *this_);

/**
 * @brief Calculates the delta between two angles
 * @param angle1 The first angle
 * @param angle2 The second angle
 * @return The difference between the angles: -179..-1=angle2 is left of angle1,0=same,1..179=angle2 is right of angle1,180=angle1 is opposite of angle2
 */ 

static int
angle_delta(int angle1, int angle2)
{
	int delta=angle2-angle1;
	if (delta <= -180)
		delta+=360;
	if (delta > 180)
		delta-=360;
	return delta;
}

static int
angle_median(int angle1, int angle2)
{
	int delta=angle_delta(angle1, angle2);
	int ret=angle1+delta/2;
	if (ret < 0)
		ret+=360;
	if (ret > 360)
		ret-=360;
	return ret;
}

static int
angle_opposite(int angle)
{
	return ((angle+180)%360);
}

int
navigation_get_attr(struct navigation *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter)
{
	dbg(0,"enter %s\n", attr_to_name(type));
	switch (type) {
	case attr_map:
		attr->u.map=this_->map;
		break;
	default:
		return 0;	
	}
	attr->type=type;
	return 1;
}


struct navigation *
navigation_new(struct attr *parent, struct attr **attrs)
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
	ret->navit=parent->u.navit;

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

/**
 * @brief Holds a way that one could possibly drive from a navigation item
 */
struct navigation_way {
	struct navigation_way *next;		/**< Pointer to a linked-list of all navigation_ways from this navigation item */ 
	short dir;			/**< The direction -1 or 1 of the way */
	short angle2;			/**< The angle one has to steer to drive from the old item to this street */
	int flags;			/**< The flags of the way */
	struct item item;		/**< The item of the way */
	char *name1;
	char *name2;
};

struct navigation_itm {
	char *name1;
	char *name2;
	struct item item;
	int direction;
	int angle_start;
	int angle_end;
	struct coord start,end;
	int time;
	int length;
	int dest_time;
	int dest_length;
	int told;							/**< Indicates if this item's announcement has been told earlier and should not be told again*/
	int streetname_told;				/**< Indicates if this item's streetname has been told in speech navigation*/
	int dest_count;
	int flags;
	struct navigation_itm *next;
	struct navigation_itm *prev;
	struct navigation_way *ways;		/**< Pointer to all ways one could drive from here */
};

/* 0=N,90=E */
static int
road_angle(struct coord *c1, struct coord *c2, int dir)
{
	int ret=transform_get_angle_delta(c1, c2, dir);
	dbg(1, "road_angle(0x%x,0x%x - 0x%x,0x%x)=%d\n", c1->x, c1->y, c2->x, c2->y, ret);
	return ret;
}

static char
*get_count_str(int n) 
{
	switch (n) {
	case 0:
		return _("zeroth"); // Not shure if this exists, neither if it will ever be needed
	case 1:
		return _("first");
	case 2:
		return _("second");
	case 3:
		return _("third");
	case 4:
		return _("fourth");
	case 5:
		return _("fifth");
	case 6:
		return _("sixth");
	default: 
		return NULL;
	}
}

static char
*get_exit_count_str(int n) 
{
	switch (n) {
	case 0:
		return _("zeroth exit"); // Not shure if this exists, neither if it will ever be needed
	case 1:
		return _("first exit");
	case 2:
		return _("second exit");
	case 3:
		return _("third exit");
	case 4:
		return _("fourth exit");
	case 5:
		return _("fifth exit");
	case 6:
		return _("sixth exit");
	default: 
		return NULL;
	}
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


/**
 * @brief This calculates the angle with which an item starts or ends
 *
 * This function can be used to get the angle an item (from a route graph map)
 * starts or ends with. Note that the angle will point towards the inner of
 * the item.
 *
 * This is meant to be used with items from a route graph map
 * With other items this will probably not be optimal...
 *
 * @param w The way which should be calculated
 */ 
static void
calculate_angle(struct navigation_way *w)
{
	struct coord cbuf[2];
	struct item *ritem; // the "real" item
	struct coord c;
	struct map_rect *mr;
	struct attr attr;

	w->angle2=361;
	mr = map_rect_new(w->item.map, NULL);
	if (!mr)
		return;

	ritem = map_rect_get_item_byid(mr, w->item.id_hi, w->item.id_lo);
	if (!ritem) {
		dbg(1,"Item from segment not found on map!\n");
		map_rect_destroy(mr);
		return;
	}

	if (ritem->type < type_line || ritem->type >= type_area) {
		map_rect_destroy(mr);
		return;
	}
	if (item_attr_get(ritem, attr_flags, &attr))
		w->flags=attr.u.num;
	else
		w->flags=0;
	if (item_attr_get(ritem, attr_street_name, &attr))
		w->name1=map_convert_string(ritem->map,attr.u.str);
	else
		w->name1=NULL;
	if (item_attr_get(ritem, attr_street_name_systematic, &attr))
		w->name2=map_convert_string(ritem->map,attr.u.str);
	else
		w->name2=NULL;
		
	if (w->dir < 0) {
		if (item_coord_get(ritem, cbuf, 2) != 2) {
			dbg(1,"Using calculate_angle() with a less-than-two-coords-item?\n");
			map_rect_destroy(mr);
			return;
		}
			
		while (item_coord_get(ritem, &c, 1)) {
			cbuf[0] = cbuf[1];
			cbuf[1] = c;
		}
		
	} else {
		if (item_coord_get(ritem, cbuf, 2) != 2) {
			dbg(1,"Using calculate_angle() with a less-than-two-coords-item?\n");
			map_rect_destroy(mr);
			return;
		}
		c = cbuf[0];
		cbuf[0] = cbuf[1];
		cbuf[1] = c;
	}

	map_rect_destroy(mr);

	w->angle2=road_angle(&cbuf[1],&cbuf[0],0);
}

/**
 * @brief Returns the time (in seconds) one will drive between two navigation items
 *
 * This function returns the time needed to drive between two items, including both of them,
 * in seconds.
 *
 * @param from The first item
 * @param to The last item
 * @return The travel time in seconds, or -1 on error
 */
static int
navigation_time(struct navigation_itm *from, struct navigation_itm *to)
{
	struct navigation_itm *cur;
	int time;

	time = 0;
	cur = from;
	while (cur) {
		time += cur->time;

		if (cur == to) {
			break;
		}
		cur = cur->next;
	}

	if (!cur) {
		return -1;
	}

	return time;
}

/**
 * @brief Clears the ways one can drive from itm
 *
 * @param itm The item that should have its ways cleared
 */
static void
navigation_itm_ways_clear(struct navigation_itm *itm)
{
	struct navigation_way *c,*n;

	c = itm->ways;
	while (c) {
		n = c->next;
		map_convert_free(c->name1);
		map_convert_free(c->name2);
		g_free(c);
		c = n;
	}

	itm->ways = NULL;
}

/**
 * @brief Updates the ways one can drive from itm
 *
 * This updates the list of possible ways to drive to from itm. The item "itm" is on
 * and the next navigation item are excluded.
 *
 * @param itm The item that should be updated
 * @param graph_map The route graph's map that these items are on 
 */
static void
navigation_itm_ways_update(struct navigation_itm *itm, struct map *graph_map) 
{
	struct map_selection coord_sel;
	struct map_rect *g_rect; // Contains a map rectangle from the route graph's map
	struct item *i,*sitem;
	struct attr sitem_attr,direction_attr;
	struct navigation_way *w,*l;

	navigation_itm_ways_clear(itm);

	// These values cause the code in route.c to get us only the route graph point and connected segments
	coord_sel.next = NULL;
	coord_sel.u.c_rect.lu = itm->start;
	coord_sel.u.c_rect.rl = itm->start;
	// the selection's order is ignored
	
	g_rect = map_rect_new(graph_map, &coord_sel);
	
	i = map_rect_get_item(g_rect);
	if (!i || i->type != type_rg_point) { // probably offroad? 
		return ;
	}

	w = NULL;
	
	while (1) {
		i = map_rect_get_item(g_rect);

		if (!i) {
			break;
		}
		
		if (i->type != type_rg_segment) {
			continue;
		}
		
		if (!item_attr_get(i,attr_street_item,&sitem_attr)) {
			dbg(1, "Got no street item for route graph item in entering_straight()\n");
			continue;
		}		

		if (!item_attr_get(i,attr_direction,&direction_attr)) {
			continue;
		}

		sitem = sitem_attr.u.item;
		if (item_is_equal(itm->item,*sitem) || ((itm->prev) && item_is_equal(itm->prev->item,*sitem))) {
			continue;
		}

		l = w;
		w = g_new(struct navigation_way, 1);
		w->dir = direction_attr.u.num;
		w->item = *sitem;
		w->next = l;
		calculate_angle(w);
	}

	map_rect_destroy(g_rect);
	
	itm->ways = w;
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
			if (cmd->next) {
				cmd->next->prev = NULL;
			}
			g_free(cmd);
		}
		map_convert_free(itm->name1);
		map_convert_free(itm->name2);
		navigation_itm_ways_clear(itm);
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

/**
 * @brief This check if an item is part of a roundabout
 *
 * @param itm The item to be checked
 * @return True if the item is part of a roundabout
 */ 
static int
check_roundabout(struct navigation_itm *itm, struct map *graph_map)
{
	struct map_selection coord_sel;
	struct map_rect *g_rect; // Contains a map rectangle from the route graph's map
	struct item *i,*sitem;
	struct attr sitem_attr,flags_attr;

	// These values cause the code in route.c to get us only the route graph point and connected segments
	coord_sel.next = NULL;
	coord_sel.u.c_rect.lu = itm->start;
	coord_sel.u.c_rect.rl = itm->start;
	// the selection's order is ignored
	
	g_rect = map_rect_new(graph_map, &coord_sel);
	
	i = map_rect_get_item(g_rect);
	if (!i || i->type != type_rg_point) { // probably offroad? 
		return 0;
	}

	while (1) {
		i = map_rect_get_item(g_rect);

		if (!i) {
			break;
		}
		
		if (i->type != type_rg_segment) {
			continue;
		}
		
		if (!item_attr_get(i,attr_street_item,&sitem_attr)) {
			continue;
		}		

		sitem = sitem_attr.u.item;
		if (item_is_equal(itm->item,*sitem)) {
			if (item_attr_get(i,attr_flags,&flags_attr) && (flags_attr.u.num & AF_ROUNDABOUT)) {
				map_rect_destroy(g_rect);
				return 1;
			}
		}
	}

	map_rect_destroy(g_rect);
	return 0;
}

static struct navigation_itm *
navigation_itm_new(struct navigation *this_, struct item *ritem)
{
	struct navigation_itm *ret=g_new0(struct navigation_itm, 1);
	int i=0;
	struct item *sitem;
	struct map *graph_map = NULL;
	struct attr street_item,direction,route_attr;
	struct map_rect *mr;
	struct attr attr;
	struct coord c[5];

	if (ritem) {
		ret->streetname_told=0;
		if (! item_attr_get(ritem, attr_street_item, &street_item)) {
			dbg(0,"no street item\n");
			return NULL;
		}
		if (item_attr_get(ritem, attr_direction, &direction))
			ret->direction=direction.u.num;
		else
			ret->direction=0;

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

		while (item_coord_get(ritem, &c[i], 1)) {
			dbg(1, "coord %d 0x%x 0x%x\n", i, c[i].x ,c[i].y);

			if (i < 4) 
				i++;
			else {
				c[2]=c[3];
				c[3]=c[4];
			}
		}
		dbg(1,"count=%d\n", i);
		i--;

		ret->angle_start=road_angle(&c[0], &c[1], 0);
		ret->angle_end=road_angle(&c[i-1], &c[i], 0);

		ret->start=c[0];
		ret->end=c[i];

		item_attr_get(ritem, attr_route, &route_attr);
		graph_map = route_get_graph_map(route_attr.u.route);
		if (check_roundabout(ret, graph_map)) {
			ret->flags |= AF_ROUNDABOUT;
		}

		dbg(1,"i=%d start %d end %d '%s' '%s'\n", i, ret->angle_start, ret->angle_end, ret->name1, ret->name2);
		map_rect_destroy(mr);
	} else {
		if (this_->last)
			ret->start=ret->end=this_->last->end;
	}
	if (! this_->first)
		this_->first=ret;
	if (this_->last) {
		this_->last->next=ret;
		ret->prev=this_->last;
		if (graph_map) {
			navigation_itm_ways_update(ret,graph_map);
		}
	}
	dbg(1,"ret=%p\n", ret);
	this_->last=ret;
	return ret;
}

/**
 * @brief Counts how many times a driver could turn right/left 
 *
 * This function counts how many times the driver theoretically could
 * turn right/left between two navigation items, not counting the final
 * turn itself.
 *
 * @param from The navigation item which should form the start
 * @param to The navigation item which should form the end
 * @param direction Set to < 0 to count turns to the left >= 0 for turns to the right
 * @return The number of possibilities to turn or -1 on error
 */
static int
count_possible_turns(struct navigation_itm *from, struct navigation_itm *to, int direction)
{
	int count;
	struct navigation_itm *curr;
	struct navigation_way *w;

	count = 0;
	curr = from->next;
	while (curr && (curr != to)) {
		w = curr->ways;

		while (w) {
			if (direction < 0) {
				if (angle_delta(curr->prev->angle_end, w->angle2) < 0) {
					count++;
					break;
				}
			} else {
				if (angle_delta(curr->prev->angle_end, w->angle2) > 0) {
					count++;
					break;
				}				
			}
			w = w->next;
		}
		curr = curr->next;
	}

	if (!curr) { // from does not lead to to?
		return -1;
	}

	return count;
}

/**
 * @brief Calculates distance and time to the destination
 *
 * This function calculates the distance and the time to the destination of a
 * navigation. If incr is set, this is only calculated for the first navigation
 * item, which is a lot faster than re-calculation the whole destination, but works
 * only if the rest of the navigation already has been calculated.
 *
 * @param this_ The navigation whose destination / time should be calculated
 * @param incr Set this to true to only calculate the first item. See description.
 */
static void
calculate_dest_distance(struct navigation *this_, int incr)
{
	int len=0, time=0, count=0;
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
		itm->dest_count=next->dest_count+1;
		itm->dest_time=next->dest_time+itm->time;
		dbg(2, "new values: time=%d lenght=%d\n", itm->dest_length, itm->dest_time);
		return;
	}
	while (itm) {
		len+=itm->length;
		time+=itm->time;
		itm->dest_length=len;
		itm->dest_time=time;
		itm->dest_count=count++;
		itm=itm->prev;
	}
	dbg(1,"len %d time %d\n", len, time);
}

/**
 * @brief Checks if two navigation items are on the same street
 *
 * This function checks if two navigation items are on the same street. It returns
 * true if either their name or their "systematic name" (e.g. "A6" or "B256") are the
 * same.
 *
 * @param old The first item to be checked
 * @param new The second item to be checked
 * @return True if both old and new are on the same street
 */
static int
is_same_street2(char *old_name1, char *old_name2, char *new_name1, char *new_name2)
{
	if (old_name1 && new_name1 && !strcmp(old_name1, new_name1)) {
		dbg(1,"is_same_street: '%s' '%s' vs '%s' '%s' yes (1.)\n", old_name2, new_name2, old_name1, new_name1);
		return 1;
	}
	if (old_name2 && new_name2 && !strcmp(old_name2, new_name2)) {
		dbg(1,"is_same_street: '%s' '%s' vs '%s' '%s' yes (2.)\n", old_name2, new_name2, old_name1, new_name1);
		return 1;
	}
	dbg(1,"is_same_street: '%s' '%s' vs '%s' '%s' no\n", old_name2, new_name2, old_name1, new_name1);
	return 0;
}

#if 0
/**
 * @brief Checks if two navigation items are on the same street
 *
 * This function checks if two navigation items are on the same street. It returns
 * true if the first part of their "systematic name" is equal. If the "systematic name" is
 * for example "A352/E3" (a german highway which at the same time is part of the international
 * E-road network), it would only search for "A352" in the second item's systematic name.
 *
 * @param old The first item to be checked
 * @param new The second item to be checked
 * @return True if the "systematic name" of both items matches. See description.
 */
static int
is_same_street_systematic(struct navigation_itm *old, struct navigation_itm *new)
{
	int slashold,slashnew;
	if (!old->name2 || !new->name2)
		return 1;
	slashold=strcspn(old->name2, "/");
	slashnew=strcspn(new->name2, "/");
	if (slashold != slashnew || strncmp(old->name2, new->name2, slashold))
		return 0;
	return 1;
}


/**
 * @brief Check if there are multiple possibilities to drive from old
 *
 * This function checks, if there are multiple streets connected to the exit of "old".
 * Sometimes it happens that an item on a map is just segmented, without any other streets
 * being connected there, and it is not useful if navit creates a maneuver there.
 *
 * @param new The navigation item we're driving to
 * @return True if there are multiple streets
 */
static int 
maneuver_multiple_streets(struct navigation_itm *new)
{
	if (new->ways) {
		return 1;
	} else {
		return 0;
	}
}


/**
 * @brief Check if the new item is entered "straight"
 *
 * This function checks if the new item is entered "straight" from the old item, i.e. if there
 * is no other street one could take from the old item on with less steering.
 *
 * @param new The navigation item we're driving to
 * @param diff The absolute angle one needs to steer to drive to this item
 * @return True if the new item is entered "straight"
 */
static int 
maneuver_straight(struct navigation_itm *new, int diff)
{
	int curr_diff;
	struct navigation_way *w;

	w = new->ways;
	dbg(1,"diff=%d\n", diff);
	while (w) {
		curr_diff=abs(angle_delta(new->prev->angle_end, w->angle2));
		dbg(1,"curr_diff=%d\n", curr_diff);
		if (curr_diff < diff) {
			return 0;
		}
		w = w->next;
	}
	return 1;
}
#endif

static int maneuver_category(enum item_type type)
{
	switch (type) {
	case type_street_0:
		return 1;
	case type_street_1_city:
		return 2;
	case type_street_2_city:
		return 3;
	case type_street_3_city:
		return 4;
	case type_street_4_city:
		return 5;
	case type_highway_city:
		return 7;
	case type_street_1_land:
		return 2;
	case type_street_2_land:
		return 3;
	case type_street_3_land:
		return 4;
	case type_street_4_land:
		return 5;
	case type_street_n_lanes:
		return 6;
	case type_highway_land:
		return 7;
	case type_ramp:
		return 0;
	case type_roundabout:
		return 0;
	case type_ferry:
		return 0;
	default:
		return 0;
	}
	
	
}

static int
is_way_allowed(struct navigation_way *way)
{
	if (way->dir > 0) {
		if (way->flags & AF_ONEWAYREV)
			return 0;
	} else {
		if (way->flags & AF_ONEWAY)
			return 0;
	}
	return 1;
}

/**
 * @brief Checks if navit has to create a maneuver to drive from old to new
 *
 * This function checks if it has to create a "maneuver" - i.e. guide the user - to drive 
 * from "old" to "new".
 *
 * @param old The old navigation item, where we're coming from
 * @param new The new navigation item, where we're going to
 * @param delta The angle the user has to steer to navigate from old to new
 * @param reason A text string explaining how the return value resulted
 * @return True if navit should guide the user, false otherwise
 */
static int
maneuver_required2(struct navigation_itm *old, struct navigation_itm *new, int *delta, char **reason)
{
	int ret=0,d,dw,dlim;
	char *r=NULL;
	struct navigation_way *w;
	int cat,ncat,wcat,maxcat,left=-180,right=180,is_unambigous=0,is_same_street;

	dbg(1,"enter %p %p %p\n",old, new, delta);
	d=angle_delta(old->angle_end, new->angle_start);
	if (!new->ways) {
		/* No announcement necessary */
		r="no: Only one possibility";
	} else if (!new->ways->next && new->ways->item.type == type_ramp && !is_way_allowed(new->ways)) {
		/* If the other way is only a ramp and it is one-way in the wrong direction, no announcement necessary */
		r="no: Only ramp";
	}
	if (! r) {
		if ((old->flags & AF_ROUNDABOUT) && ! (new->flags & AF_ROUNDABOUT)) {
			r="yes: leaving roundabout";
			ret=1;
		} else 	if (!(old->flags & AF_ROUNDABOUT) && (new->flags & AF_ROUNDABOUT)) 
			r="no: entering roundabout";
		else if ((old->flags & AF_ROUNDABOUT) && (new->flags & AF_ROUNDABOUT)) 
			r="no: staying in roundabout";
	}
	if (!r && abs(d) > 75) {
		/* always make an announcement if you have to make a sharp turn */
		r="yes: delta over 75";
		ret=1;
	}
	cat=maneuver_category(old->item.type);
	ncat=maneuver_category(new->item.type);
	if (!r) {
		/* Check whether the street keeps its name */
		is_same_street=is_same_street2(old->name1, old->name2, new->name1, new->name2);
		w = new->ways;
		maxcat=-1;
		while (w) {
			dw=angle_delta(old->angle_end, w->angle2);
			if (dw < 0) {
				if (dw > left)
					left=dw;
			} else {
				if (dw < right)
					right=dw;
			}
			wcat=maneuver_category(w->item.type);
			/* If any other street has the same name but isn't a highway (a highway might split up temporarily), then
			   we can't use the same name criterium  */
			if (is_same_street && is_same_street2(old->name1, old->name2, w->name1, w->name2) && (cat != 7 || wcat != 7) && is_way_allowed(w))
				is_same_street=0;
			/* Mark if the street has a higher or the same category */
			if (wcat > maxcat)
				maxcat=wcat;
			w = w->next;
		}
		/* get the delta limit for checking for other streets. It is lower if the street has no other
		   streets of the same or higher category */
		if (ncat < cat)
			dlim=80;
		else
			dlim=120;
		/* if the street is really straight, the others might be closer to straight */
		if (abs(d) < 20)
			dlim/=2;
		if ((maxcat == ncat && maxcat == cat) || (ncat == 0 && cat == 0)) 
			dlim=abs(d)*620/256;
		else if (maxcat < ncat && maxcat < cat)
			dlim=abs(d)*128/256;
		if (left < -dlim && right > dlim) 
			is_unambigous=1;
		if (!is_same_street && is_unambigous < 1) {
			ret=1;
			r="yes: not same street or ambigous";
		} else
			r="no: same street and unambigous";
#ifdef DEBUG
		r=g_strdup_printf("yes: d %d left %d right %d dlim=%d cat old:%d new:%d max:%d unambigous=%d same_street=%d", d, left, right, dlim, cat, ncat, maxcat, is_unambigous, is_same_street);
#endif
	}
	*delta=d;
	if (reason)
		*reason=r;
	return ret;
	

#if 0
	if (new->item.type == old->item.type || (new->item.type != type_ramp && old->item.type != type_ramp)) {
		if (is_same_street2(old, new)) {
			if (! entering_straight(new, abs(*delta))) {
				dbg(1, "maneuver_required: Not driving straight: yes\n");
				if (reason)
					*reason="yes: Not driving straight";
				return 1;
			}

			if (check_multiple_streets(new)) {
				if (entering_straight(new,abs(*delta)*2)) {
					if (reason)
						*reason="no: delta < ext_limit for same name";
					return 0;
				}
				if (reason)	
					*reason="yes: delta > ext_limit for same name";
				return 1;
			} else {
				dbg(1, "maneuver_required: Staying on the same street: no\n");
				if (reason)
					*reason="no: Staying on same street";
				return 0;
			}
		}
	} else
		dbg(1, "maneuver_required: old or new is ramp\n");
#if 0
	if (old->item.type == type_ramp && (new->item.type == type_highway_city || new->item.type == type_highway_land)) {
		dbg(1, "no_maneuver_required: old is ramp new is highway\n");
		if (reason)
			*reason="no: old is ramp new is highway";
		return 0;
	}
#endif
#if 0
	if (old->crossings_end == 2) {
		dbg(1, "maneuver_required: only 2 connections: no\n");
		return 0;
	}
#endif
	dbg(1,"delta=%d-%d=%d\n", new->angle_start, old->angle_end, *delta);
	if ((new->item.type == type_highway_land || new->item.type == type_highway_city || old->item.type == type_highway_land || old->item.type == type_highway_city) && (!is_same_street_systematic(old, new) || (old->name2 != NULL && new->name2 == NULL))) {
		dbg(1, "maneuver_required: highway changed name\n");
		if (reason)
			*reason="yes: highway changed name";
		return 1;
	}
	if (abs(*delta) < straight_limit) {
		if (! entering_straight(new,abs(*delta))) {
			if (reason)
				*reason="yes: not straight";
			dbg(1, "maneuver_required: not driving straight: yes\n");
			return 1;
		}

		dbg(1, "maneuver_required: delta(%d) < %d: no\n", *delta, straight_limit);
		if (reason)
			*reason="no: delta < limit";
		return 0;
	}
	if (abs(*delta) < ext_straight_limit) {
		if (entering_straight(new,abs(*delta)*2)) {
			if (reason)
				*reason="no: delta < ext_limit";
			return 0;
		}
	}

	if (! check_multiple_streets(new)) {
		dbg(1, "maneuver_required: only one possibility: no\n");
		if (reason)
			*reason="no: only one possibility";
		return 0;
	}

	dbg(1, "maneuver_required: delta=%d: yes\n", *delta);
	if (reason)
		*reason="yes: delta >= limit";
	return 1;
#endif
}

static struct navigation_command *
command_new(struct navigation *this_, struct navigation_itm *itm, int delta)
{
	struct navigation_command *ret=g_new0(struct navigation_command, 1);
	dbg(1,"enter this_=%p itm=%p delta=%d\n", this_, itm, delta);
	ret->delta=delta;
	ret->itm=itm;
	if (itm && itm->prev && itm->prev->ways && !(itm->flags & AF_ROUNDABOUT) && (itm->prev->flags & AF_ROUNDABOUT)) {
		int len=0;
		int angle=0;
		int entry_angle;
		struct navigation_itm *itm2=itm->prev;
		int exit_angle=angle_median(itm->prev->angle_end, itm->ways->angle2);
		dbg(1,"exit %d median from %d,%d\n", exit_angle,itm->prev->angle_end, itm->ways->angle2);
		while (itm2 && (itm2->flags & AF_ROUNDABOUT)) {
			len+=itm2->length;
			angle=itm2->angle_end;
			itm2=itm2->prev;
		}
		if (itm2 && itm2->next && itm2->next->ways) {
			itm2=itm2->next;
			entry_angle=angle_median(angle_opposite(itm2->angle_start), itm2->ways->angle2);
			dbg(1,"entry %d median from %d(%d),%d\n", entry_angle,angle_opposite(itm2->angle_start), itm2->angle_start, itm2->ways->angle2);
		} else {
			entry_angle=angle_opposite(angle);
		}
		dbg(0,"entry %d exit %d\n", entry_angle, exit_angle);
		ret->roundabout_delta=angle_delta(entry_angle, exit_angle);
		ret->length=len;
		
	}
	if (this_->cmd_last) {
		this_->cmd_last->next=ret;
		ret->prev = this_->cmd_last;
	}
	this_->cmd_last=ret;

	if (!this_->cmd_first)
		this_->cmd_first=ret;
	return ret;
}

static void
make_maneuvers(struct navigation *this_, struct route *route)
{
	struct navigation_itm *itm, *last=NULL, *last_itm=NULL;
	int delta;
	itm=this_->first;
	this_->cmd_last=NULL;
	this_->cmd_first=NULL;
	while (itm) {
		if (last) {
			if (maneuver_required2(last_itm, itm,&delta,NULL)) {
				command_new(this_, itm, delta);
			}
		} else
			last=itm;
		last_itm=itm;
		itm=itm->next;
	}
	command_new(this_, last_itm, 0);
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
	if (isupper(name[len])) {
		ret[len]=toupper(ret[len]);
	}

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
			return g_strdup_printf("%s%s",prefix,_("exit"));	/* %FIXME Can this even be reached? */			 
		else
			return g_strdup_printf("%s%s",prefix,_("into the ramp"));
		
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
		ret=g_strdup_printf(_("%sinto the %s"),prefix,itm->name2);
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
show_maneuver(struct navigation *nav, struct navigation_itm *itm, struct navigation_command *cmd, enum attr_type type, int connect)
{
	/* TRANSLATORS: right, as in 'Turn right' */
	char *dir=_("right"),*strength="";
	int distance=itm->dest_length-cmd->itm->dest_length;
	char *d,*ret=NULL;
	int delta=cmd->delta;
	int level;
	int strength_needed;
	int skip_roads;
	int count_roundabout;
	struct navigation_itm *cur;
	struct navigation_way *w;
	
	if (connect) {
		level = -2; // level = -2 means "connect to another maneuver via 'then ...'"
	} else {
		level=1;
	}

	w = itm->next->ways;
	strength_needed = 0;
	if (angle_delta(itm->next->angle_start,itm->angle_end) < 0) {
		while (w) {
			if (angle_delta(w->angle2,itm->angle_end) < 0) {
				strength_needed = 1;
				break;
			}
			w = w->next;
		}
	} else {
		while (w) {
			if (angle_delta(w->angle2,itm->angle_end) > 0) {
				strength_needed = 1;
				break;
			}
			w = w->next;
		}
	}

	if (delta < 0) {
		/* TRANSLATORS: left, as in 'Turn left' */
		dir=_("left");
		delta=-delta;
	}

	if (strength_needed) {
		if (delta < 45) {
			/* TRANSLATORS: Don't forget the ending space */
			strength=_("easily ");
		} else if (delta < 105) {
			strength="";
		} else if (delta < 165) {
			/* TRANSLATORS: Don't forget the ending space */
			strength=_("strongly ");
		} else if (delta < 180) {
			/* TRANSLATORS: Don't forget the ending space */
			strength=_("really strongly ");
		} else {
			dbg(1,"delta=%d\n", delta);
			/* TRANSLATORS: Don't forget the ending space */
			strength=_("unknown ");
		}
	}
	if (type != attr_navigation_long_exact) 
		distance=round_distance(distance);
	if (type == attr_navigation_speech) {
		if (nav->turn_around && nav->turn_around == nav->turn_around_limit) 
			return g_strdup(_("When possible, please turn around"));
		if (!connect) {
			level=navigation_get_announce_level(nav, itm->item.type, distance-cmd->length);
		}
		dbg(1,"distance=%d level=%d type=0x%x\n", distance, level, itm->item.type);
	}

	if (cmd->itm->prev->flags & AF_ROUNDABOUT) {
		switch (level) {
		case 2:
			return g_strdup(_("Enter the roundabout soon"));
		case 1:
			d = get_distance(distance, type, 1);
			/* TRANSLATORS: %s is the distance to the roundabout */
			ret = g_strdup_printf(_("In %s, enter the roundabout"), d);
			g_free(d);
			return ret;
		case -2:
		case 0:
			cur = cmd->itm->prev;
			count_roundabout = 0;
			while (cur && (cur->flags & AF_ROUNDABOUT)) {
				if (cur->next->ways && is_way_allowed(cur->next->ways)) { // If the next segment has no exit or the exit isn't allowed, don't count it
					count_roundabout++;
				}
				cur = cur->prev;
			}
			switch (level) {
			case 0:
				ret = g_strdup_printf(_("Leave the roundabout at the %s"), get_exit_count_str(count_roundabout));
				break;
			case -2:
				ret = g_strdup_printf(_("then leave the roundabout at the %s"), get_exit_count_str(count_roundabout));
				break;
			}
			return ret;
		}
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
		skip_roads = count_possible_turns(nav->first,cmd->itm,cmd->delta);
		if (skip_roads > 0) {
			if (get_count_str(skip_roads+1)) {
				/* TRANSLATORS: First argument is the how manieth street to take, second the direction */ 
				ret = g_strdup_printf(_("Take the %1$s road to the %2$s"), get_count_str(skip_roads+1), dir);
				return ret;
			} else {
				d = g_strdup_printf(_("after %i roads"), skip_roads);
			}
		} else {
			d=g_strdup(_("now"));
		}
		break;
	case -2:
		skip_roads = count_possible_turns(cmd->prev->itm,cmd->itm,cmd->delta);
		if (skip_roads > 0) {
			/* TRANSLATORS: First argument is the how manieth street to take, second the direction */ 
			if (get_count_str(skip_roads+1)) {
				ret = g_strdup_printf(_("then take the %1$s road to the %2$s"), get_count_str(skip_roads+1), dir);
				return ret;
			} else {
				d = g_strdup_printf(_("after %i roads"), skip_roads);
			}

		} else {
			d = g_strdup("");
		}
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
				cmd->itm->streetname_told = 1; // remeber to be checked when we turn
				tellstreetname = 1; // Ok so we tell the name of the street 
			}

			if (level == 0) {
				if(cmd->itm->streetname_told == 0) // we are right at the intersection
					tellstreetname = 1; 
				else
					cmd->itm->streetname_told = 0;  // reset just in case we come to the same street again
			}

		}
		else
		     tellstreetname = 1;

		if(tellstreetname) 
			destination=navigation_item_destination(cmd->itm, itm, " ");
		if (level != -2) {
			/* TRANSLATORS: The first argument is strength, the second direction, the third distance and the fourth destination Example: 'Turn 'slightly' 'left' in '100 m' 'onto baker street' */
			ret=g_strdup_printf(_("Turn %1$s%2$s %3$s%4$s"), strength, dir, d, destination ? destination:"");
		} else {
			/* TRANSLATORS: First argument is strength, second direction, third how many roads to skip, fourth destination */
			ret=g_strdup_printf(_("then turn %1$s%2$s %3$s%4$s"), strength, dir, d, destination ? destination:"");
		}
		g_free(destination);
	} else {
		if (!connect) {
			ret=g_strdup_printf(_("You have reached your destination %s"), d);
		} else {
			ret=g_strdup_printf(_("then you have reached your destination."));
		}
	}
	g_free(d);
	return ret;
}

/**
 * @brief Creates announcements for maneuvers, plus maneuvers immediately following the next maneuver
 *
 * This function does create an announcement for the current maneuver and for maneuvers
 * immediately following that maneuver, if these are too close and we're in speech navigation.
 *
 * @return An announcement that should be made
 */
static char *
show_next_maneuvers(struct navigation *nav, struct navigation_itm *itm, struct navigation_command *cmd, enum attr_type type)
{
	struct navigation_command *cur,*prev;
	int distance=itm->dest_length-cmd->itm->dest_length;
	int level, dist, i, time;
	int speech_time,time2nav;
	char *ret,*old,*buf,*next;

	if (type != attr_navigation_speech) {
		return show_maneuver(nav, itm, cmd, type, 0); // We accumulate maneuvers only in speech navigation
	}

	level=navigation_get_announce_level(nav, itm->item.type, distance-cmd->length);

	if (level > 1) {
		return show_maneuver(nav, itm, cmd, type, 0); // We accumulate maneuvers only if they are close
	}

	if (cmd->itm->told) {
		return g_strdup("");
	}

	ret = show_maneuver(nav, itm, cmd, type, 0);
	time2nav = navigation_time(itm,cmd->itm->prev);
	old = NULL;

	cur = cmd->next;
	prev = cmd;
	i = 0;
	while (cur && cur->itm) {
		// We don't merge more than 3 announcements...
		if (i > 1) { // if you change this, please also change the value below, that is used to terminate the loop
			break;
		}
		
		next = show_maneuver(nav,prev->itm, cur, type, 0);
		speech_time = navit_speech_estimate(nav->navit,next);
		g_free(next);

		if (speech_time == -1) { // user didn't set cps
			speech_time = 30; // assume 3 seconds
		}

		dist = prev->itm->dest_length - cur->itm->dest_length;
		time = navigation_time(prev->itm,cur->itm->prev);

		if (time >= (speech_time + 30)) { // 3 seconds for understanding what has been said
			break;
		}

		old = ret;
		buf = show_maneuver(nav, prev->itm, cur, type, 1);
		ret = g_strdup_printf("%s, %s", old, buf);
		g_free(buf);
		if (navit_speech_estimate(nav->navit,ret) > time2nav) {
			g_free(ret);
			ret = old;
			i = 2; // This will terminate the loop
		} else {
			g_free(old);
		}

		// If the two maneuvers are *really* close, we shouldn't tell the second one again, because TTS won't be fast enough
		if (time <= speech_time) {
			cur->itm->told = 1;
		}

		prev = cur;
		cur = cur->next;
		i++;
	}

	return ret;
}

static void
navigation_call_callbacks(struct navigation *this_, int force_speech)
{
	int distance, level = 0, level2;
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
		distance-=this_->cmd_first->length;
		level=navigation_get_announce_level(this_, this_->first->item.type, distance);
		if (this_->cmd_first->itm->prev) {
			level2=navigation_get_announce_level(this_, this_->cmd_first->itm->prev->item.type, distance);
			if (level2 > level)
				level=level2;
		}
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

static void
navigation_update(struct navigation *this_, struct route *route, struct attr *attr)
{
	struct map *map;
	struct map_rect *mr;
	struct item *ritem;			/* Holds an item from the route map */
	struct item *sitem;			/* Holds the corresponding item from the actual map */
	struct attr street_item,street_direction;
	struct navigation_itm *itm;
	int mode=0, incr=0, first=1;
	if (attr->type != attr_route_status)
		return;

	dbg(1,"enter %d\n", mode);
	if (attr->u.num == route_status_no_destination || attr->u.num == route_status_not_found || attr->u.num == route_status_path_done_new) 
		navigation_flush(this_);
	if (attr->u.num != route_status_path_done_new && attr->u.num != route_status_path_done_incremental)
		return;
		
	if (! this_->route)
		return;
	map=route_get_map(this_->route);
	if (! map)
		return;
	mr=map_rect_new(map, NULL);
	if (! mr)
		return;
	dbg(1,"enter\n");
	while ((ritem=map_rect_get_item(mr))) {
		if (ritem->type != type_street_route)
			continue;
		if (first && item_attr_get(ritem, attr_street_item, &street_item)) {
			first=0;
			if (!item_attr_get(ritem, attr_direction, &street_direction))
				street_direction.u.num=0;
			sitem=street_item.u.item;
			dbg(1,"sitem=%p\n", sitem);
			itm=item_hash_lookup(this_->hash, sitem);
			dbg(2,"itm for item with id (0x%x,0x%x) is %p\n", sitem->id_hi, sitem->id_lo, itm);
			if (itm && itm->direction != street_direction.u.num) {
				dbg(2,"wrong direction\n");
				itm=NULL;
			}
			navigation_destroy_itms_cmds(this_, itm);
			if (itm) {
				navigation_itm_update(itm, ritem);
				break;
			}
			dbg(1,"not on track\n");
		}
		navigation_itm_new(this_, ritem);
	}
	if (first) 
		navigation_destroy_itms_cmds(this_, NULL);
	else {
		if (! ritem) {
			navigation_itm_new(this_, NULL);
			make_maneuvers(this_,this_->route);
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
	}
	map_rect_destroy(mr);
}

static void
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
	if (! this_->map)
		this_->map=map_new(NULL, (struct attr*[]){
			&(struct attr){attr_type,{"navigation"}},
			&(struct attr){attr_navigation,.u.navigation=this_},
			&(struct attr){attr_data,{""}},
			&(struct attr){attr_description,{"Navigation"}},
			NULL});
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
	struct navigation_itm *cmd_itm;
	struct navigation_itm *cmd_itm_next;
	struct item item;
	enum attr_type attr_next;
	int ccount;
	int debug_idx;
	struct navigation_way *ways;
	int show_all;
	char *str;
};

static int
navigation_map_item_coord_get(void *priv_data, struct coord *c, int count)
{
	struct map_rect_priv *this=priv_data;
	if (this->ccount || ! count)
		return 0;
	*c=this->itm->start;
	this->ccount=1;
	return 1;
}

static int
navigation_map_item_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr)
{
	struct map_rect_priv *this_=priv_data;
	attr->type=attr_type;
	struct navigation_command *cmd=this_->cmd;
	struct navigation_itm *itm=this_->itm;
	struct navigation_itm *prev=itm->prev;

	if (this_->str) {
		g_free(this_->str);
		this_->str=NULL;
	}

	if (cmd) {
		if (cmd->itm != itm)
			cmd=NULL;	
	}
	switch(attr_type) {
	case attr_navigation_short:
		this_->attr_next=attr_navigation_long;
		if (cmd) {
			this_->str=attr->u.str=show_next_maneuvers(this_->nav, this_->cmd_itm, cmd, attr_type);
			return 1;
		}
		return 0;
	case attr_navigation_long:
		this_->attr_next=attr_navigation_long_exact;
		if (cmd) {
			this_->str=attr->u.str=show_next_maneuvers(this_->nav, this_->cmd_itm, cmd, attr_type);
			return 1;
		}
		return 0;
	case attr_navigation_long_exact:
		this_->attr_next=attr_navigation_speech;
		if (cmd) {
			this_->str=attr->u.str=show_next_maneuvers(this_->nav, this_->cmd_itm, cmd, attr_type);
			return 1;
		}
		return 0;
	case attr_navigation_speech:
		this_->attr_next=attr_length;
		if (cmd) {
			this_->str=attr->u.str=show_next_maneuvers(this_->nav, this_->cmd_itm, this_->cmd, attr_type);
			return 1;
		}
		return 0;
	case attr_length:
		this_->attr_next=attr_time;
		if (cmd) {
			attr->u.num=this_->cmd_itm->dest_length-cmd->itm->dest_length;
			return 1;
		}
		return 0;
	case attr_time:
		this_->attr_next=attr_destination_length;
		if (cmd) {
			attr->u.num=this_->cmd_itm->dest_time-cmd->itm->dest_time;
			return 1;
		}
		return 0;
	case attr_destination_length:
		attr->u.num=itm->dest_length;
		this_->attr_next=attr_destination_time;
		return 1;
	case attr_destination_time:
		attr->u.num=itm->dest_time;
		this_->attr_next=attr_street_name;
		return 1;
	case attr_street_name:
		attr->u.str=itm->name1;
		this_->attr_next=attr_street_name_systematic;
		if (attr->u.str)
			return 1;
		return 0;
	case attr_street_name_systematic:
		attr->u.str=itm->name2;
		this_->attr_next=attr_debug;
		if (attr->u.str)
			return 1;
		return 0;
	case attr_debug:
		switch(this_->debug_idx) {
		case 0:
			this_->debug_idx++;
			this_->str=attr->u.str=g_strdup_printf("angle:%d (- %d)", itm->angle_start, itm->angle_end);
			return 1;
		case 1:
			this_->debug_idx++;
			this_->str=attr->u.str=g_strdup_printf("item type:%s", item_to_name(itm->item.type));
			return 1;
		case 2:
			this_->debug_idx++;
			if (cmd) {
				this_->str=attr->u.str=g_strdup_printf("delta:%d", cmd->delta);
				return 1;
			}
		case 3:
			this_->debug_idx++;
			if (prev) {
				this_->str=attr->u.str=g_strdup_printf("prev street_name:%s", prev->name1);
				return 1;
			}
		case 4:
			this_->debug_idx++;
			if (prev) {
				this_->str=attr->u.str=g_strdup_printf("prev street_name_systematic:%s", prev->name2);
				return 1;
			}
		case 5:
			this_->debug_idx++;
			if (prev) {
				this_->str=attr->u.str=g_strdup_printf("prev angle:(%d -) %d", prev->angle_start, prev->angle_end);
				return 1;
			}
		case 6:
			this_->debug_idx++;
			this_->ways=itm->ways;
			if (prev) {
				this_->str=attr->u.str=g_strdup_printf("prev item type:%s", item_to_name(prev->item.type));
				return 1;
			}
		case 7:
			if (this_->ways && prev) {
				this_->str=attr->u.str=g_strdup_printf("other item angle:%d delta:%d flags:%d dir:%d type:%s id:(0x%x,0x%x)", this_->ways->angle2, angle_delta(prev->angle_end, this_->ways->angle2), this_->ways->flags, this_->ways->dir, item_to_name(this_->ways->item.type), this_->ways->item.id_hi, this_->ways->item.id_lo);
				this_->ways=this_->ways->next;
				return 1;
			}
			this_->debug_idx++;
		case 8:
			this_->debug_idx++;
			if (prev) {
				int delta=0;
				char *reason=NULL;
				maneuver_required2(prev, itm, &delta, &reason);
				this_->str=attr->u.str=g_strdup_printf("reason:%s",reason);
				return 1;
			}
			
		default:
			this_->attr_next=attr_none;
			return 0;
		}
	case attr_any:
		while (this_->attr_next != attr_none) {
			if (navigation_map_item_attr_get(priv_data, this_->attr_next, attr))
				return 1;
		}
		return 0;
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

static void
navigation_map_rect_init(struct map_rect_priv *priv)
{
	priv->cmd_next=priv->nav->cmd_first;
	priv->cmd_itm_next=priv->itm_next=priv->nav->first;
}

static struct map_rect_priv *
navigation_map_rect_new(struct map_priv *priv, struct map_selection *sel)
{
	struct navigation *nav=priv->navigation;
	struct map_rect_priv *ret=g_new0(struct map_rect_priv, 1);
	ret->nav=nav;
	navigation_map_rect_init(ret);
	ret->item.meth=&navigation_map_item_methods;
	ret->item.priv_data=ret;
#ifdef DEBUG
	ret->show_all=1;
#endif
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
	struct item *ret=&priv->item;
	int delta;
	if (!priv->itm_next)
		return NULL;
	priv->itm=priv->itm_next;
	priv->cmd=priv->cmd_next;
	priv->cmd_itm=priv->cmd_itm_next;
	if (!priv->cmd)
		return NULL;
	if (!priv->show_all && priv->itm->prev != NULL) 
		priv->itm=priv->cmd->itm;
	priv->itm_next=priv->itm->next;
	if (priv->itm->prev)
		ret->type=type_nav_none;
	else
		ret->type=type_nav_position;
	if (priv->cmd->itm == priv->itm) {
		priv->cmd_itm_next=priv->cmd->itm;
		priv->cmd_next=priv->cmd->next;
		if (priv->cmd_itm_next && !priv->cmd_itm_next->next)
			ret->type=type_nav_destination;
		else {
			if (priv->itm && priv->itm->prev && !(priv->itm->flags & AF_ROUNDABOUT) && (priv->itm->prev->flags & AF_ROUNDABOUT)) {
			
				switch (((180+22)-priv->cmd->roundabout_delta)/45) {
				case 0:
				case 1:
					ret->type=type_nav_roundabout_r1;
					break;
				case 2:
					ret->type=type_nav_roundabout_r2;
					break;
				case 3:
					ret->type=type_nav_roundabout_r3;
					break;
				case 4:
					ret->type=type_nav_roundabout_r4;
					break;
				case 5:
					ret->type=type_nav_roundabout_r5;
					break;
				case 6:
					ret->type=type_nav_roundabout_r6;
					break;
				case 7:
					ret->type=type_nav_roundabout_r7;
					break;
				case 8:
					ret->type=type_nav_roundabout_r8;
					break;
				}
			} else {
				delta=priv->cmd->delta;	
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
			}
		}
	}
	priv->ccount=0;
	priv->debug_idx=0;
	priv->attr_next=attr_navigation_short;

	ret->id_lo=priv->itm->dest_count;
	dbg(1,"type=%d\n", ret->type);
	return ret;
}

static struct item *
navigation_map_get_item_byid(struct map_rect_priv *priv, int id_hi, int id_lo)
{
	struct item *ret;
	navigation_map_rect_init(priv);
	while ((ret=navigation_map_get_item(priv))) {
		if (ret->id_hi == id_hi && ret->id_lo == id_lo) 
			return ret;
	}
	return NULL;
}

static struct map_methods navigation_map_meth = {
	projection_mg,
	"utf-8",
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
navigation_set_route(struct navigation *this_, struct route *route)
{
	struct attr callback;
	this_->route=route;
	this_->route_cb=callback_new_attr_1(callback_cast(navigation_update), attr_route_status, this_);
	callback.type=attr_callback;
	callback.u.callback=this_->route_cb;
	route_add_attr(route, &callback);
}

void
navigation_init(void)
{
	plugin_register_map_type("navigation", navigation_map_new);
}
