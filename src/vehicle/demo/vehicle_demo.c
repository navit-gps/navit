#include <glib.h>
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

struct vehicle_priv {
	int interval;
	int position_set;
	struct callback_list *cbl;
	struct navit *navit;
	struct coord_geo geo;
	struct coord last;
	double config_speed;
	double speed;
	double direction;
};

static void
vehicle_demo_destroy(struct vehicle_priv *priv)
{
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
	default:
		return 0;
	}
	attr->type = type;
	return 1;
}

static int
vehicle_demo_set_attr(struct vehicle_priv *priv, struct attr *attr,
		      struct attr **attrs)
{
	if (attr->type == attr_navit) {
		priv->navit = attr->u.navit;
		return 1;
	}
	return 0;
}

struct vehicle_methods vehicle_demo_methods = {
	vehicle_demo_destroy,
	vehicle_demo_position_attr_get,
	vehicle_demo_set_attr,
};

static int
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
	if (priv->navit) 
		route=navit_get_route(priv->navit);
	if (route)
		route_map=route_get_map(route);
	if (route_map)
		mr=map_rect_new(route_map, NULL);
	if (mr) 
		item=map_rect_get_item(mr);	
	if (mr && item_coord_get(item, &pos, 1)) {
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
				callback_list_call_0(priv->cbl);
				break;
			}
		}
	} else {
		if (priv->position_set) 
			callback_list_call_0(priv->cbl);
	}
	if (mr)
		map_rect_destroy(mr);
	return 1;
}



static struct vehicle_priv *
vehicle_demo_new(struct vehicle_methods
		 *meth, struct callback_list
		 *cbl, struct attr **attrs)
{
	struct vehicle_priv *ret;
	struct attr *interval,*speed,*position_coord_geo;

	dbg(1, "enter\n");
	ret = g_new0(struct vehicle_priv, 1);
	ret->cbl = cbl;
	ret->interval=1000;
	ret->config_speed=40;
	if ((speed=attr_search(attrs, NULL, attr_speed))) {
		ret->config_speed=speed->u.num;
	}
	if ((interval=attr_search(attrs, NULL, attr_interval)))
		ret->interval=speed->u.num;
	if ((position_coord_geo=attr_search(attrs, NULL, attr_position_coord_geo))) {
		ret->geo=*(position_coord_geo->u.coord_geo);
		ret->position_set=1;
		dbg(0,"position_set %f %f\n", ret->geo.lat, ret->geo.lng);
	}
	*meth = vehicle_demo_methods;
	g_timeout_add(ret->interval, (GSourceFunc) vehicle_demo_timer, ret);
	return ret;
}

void
plugin_init(void)
{
	dbg(1, "enter\n");
	plugin_register_vehicle_type("demo", vehicle_demo_new);
}
