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
#include "config.h"
#include "debug.h"
#include "coord.h"
#include "item.h"
#include "navit.h"
#include "map.h"
#include "route.h"
#include "callback.h"
#include "transform.h"
#include "plugin.h"
#include "vehicle.h"
#include "event.h"
#include "util.h"

struct vehicle_priv {
	int interval;
	int position_set;
	struct callback_list *cbl;
	struct navit *navit;
	struct route *route;
	struct coord_geo geo;
	struct coord last;
	double config_speed;
	double speed;
	double direction;
	struct callback *timer_callback;
	struct event_timeout *timer;
	char *timep;

};

static void
vehicle_demo_destroy(struct vehicle_priv *priv)
{
	if (priv->timer)
		event_remove_timeout(priv->timer);
	callback_destroy(priv->timer_callback);
	g_free(priv->timep);
	g_free(priv);
}

static int
vehicle_demo_position_attr_get(struct vehicle_priv *priv,
			       enum attr_type type, struct attr *attr)
{
	switch (type) {
	case attr_position_speed:
		attr->u.numd = &priv->speed;
		break;
	case attr_position_direction:
		attr->u.numd = &priv->direction;
		break;
	case attr_position_coord_geo:
		attr->u.coord_geo = &priv->geo;
		break;
	case attr_position_time_iso8601:
		g_free(priv->timep);
		priv->timep=current_to_iso8601();
		attr->u.str=priv->timep;
		break;
	default:
		return 0;
	}
	attr->type = type;
	return 1;
}

static int
vehicle_demo_set_attr_do(struct vehicle_priv *priv, struct attr *attr)
{
	switch(attr->type) {
	case attr_navit:
		priv->navit = attr->u.navit;
		break;
	case attr_route:
		priv->route = attr->u.route;
		break;
	case attr_speed:
		priv->config_speed=attr->u.num;
		break;
	case attr_interval:
		priv->interval=attr->u.num;
		if (priv->timer)
			event_remove_timeout(priv->timer);
		priv->timer=event_add_timeout(priv->interval, 1, priv->timer_callback);
		break;
	case attr_position_coord_geo:
		priv->geo=*(attr->u.coord_geo);
		priv->position_set=1;
		dbg(0,"position_set %f %f\n", priv->geo.lat, priv->geo.lng);
		break;
	default:
		dbg(0,"unsupported attribute %s\n",attr_to_name(attr->type));
		return 0;
	}
	return 1;
}

static int
vehicle_demo_set_attr(struct vehicle_priv *priv, struct attr *attr)
{
	return vehicle_demo_set_attr_do(priv, attr);
}

struct vehicle_methods vehicle_demo_methods = {
	vehicle_demo_destroy,
	vehicle_demo_position_attr_get,
	vehicle_demo_set_attr,
};

static void
vehicle_demo_timer(struct vehicle_priv *priv)
{
	struct coord c, c2, pos, ci;
	int slen, len, dx, dy;
	struct route *route=NULL;
	struct map *route_map=NULL;
	struct map_rect *mr=NULL;
	struct item *item=NULL;

	len = (priv->config_speed * priv->interval / 1000)/ 3.6;
	dbg(1, "###### Entering simulation loop\n");
	if (!priv->config_speed)
		return;
	if (priv->route)
		route=priv->route;
	else if (priv->navit) 
		route=navit_get_route(priv->navit);
	if (route)
		route_map=route_get_map(route);
	if (route_map)
		mr=map_rect_new(route_map, NULL);
	if (mr)
		item=map_rect_get_item(mr);
	if (item && item->type == type_route_start)
		item=map_rect_get_item(mr);
	if (item && item_coord_get(item, &pos, 1)) {
		priv->position_set=0;
		dbg(1, "current pos=0x%x,0x%x\n", pos.x, pos.y);
		dbg(1, "last pos=0x%x,0x%x\n", priv->last.x, priv->last.y);
		if (priv->last.x == pos.x && priv->last.y == pos.y) {
			dbg(1, "endless loop\n");
		}
		priv->last = pos;
		while (item && priv->config_speed) {
			if (!item_coord_get(item, &c, 1)) {
				item=map_rect_get_item(mr);
				continue;
			}
			dbg(1, "next pos=0x%x,0x%x\n", c.x, c.y);
			slen = transform_distance(projection_mg, &pos, &c);
			dbg(1, "len=%d slen=%d\n", len, slen);
			if (slen < len) {
				len -= slen;
				pos = c;
			} else {
				if (item_coord_get(item, &c2, 1) || map_rect_get_item(mr)) {
					dx = c.x - pos.x;
					dy = c.y - pos.y;
					ci.x = pos.x + dx * len / slen;
					ci.y = pos.y + dy * len / slen;
					priv->direction =
					    transform_get_angle_delta(&pos, &c, 0);
					priv->speed=priv->config_speed;
				} else {
					ci.x = pos.x;
					ci.y = pos.y;
					priv->speed=0;
					dbg(0,"destination reached\n");
				}
				dbg(1, "ci=0x%x,0x%x\n", ci.x, ci.y);
				transform_to_geo(projection_mg, &ci,
						 &priv->geo);
				callback_list_call_attr_0(priv->cbl, attr_position_coord_geo);
				break;
			}
		}
	} else {
		if (priv->position_set) 
			callback_list_call_attr_0(priv->cbl, attr_position_coord_geo);
	}
	if (mr)
		map_rect_destroy(mr);
}



static struct vehicle_priv *
vehicle_demo_new(struct vehicle_methods
		 *meth, struct callback_list
		 *cbl, struct attr **attrs)
{
	struct vehicle_priv *ret;

	dbg(1, "enter\n");
	ret = g_new0(struct vehicle_priv, 1);
	ret->cbl = cbl;
	ret->interval=1000;
	ret->config_speed=40;
	ret->timer_callback=callback_new_1(callback_cast(vehicle_demo_timer), ret);
	*meth = vehicle_demo_methods;
	while (attrs && *attrs) 
		vehicle_demo_set_attr_do(ret, *attrs++);
	if (!ret->timer)
		ret->timer=event_add_timeout(ret->interval, 1, ret->timer_callback);
	return ret;
}

void
plugin_init(void)
{
	dbg(1, "enter\n");
	plugin_register_vehicle_type("demo", vehicle_demo_new);
}
