#include <glib.h>
#include "debug.h"
#include "coord.h"
#include "item.h"
#include "navit.h"
#include "route.h"
#include "callback.h"
#include "transform.h"
#include "plugin.h"
#include "vehicle.h"

struct vehicle_priv {
	int interval;
	struct callback_list *cbl;
	struct navit *navit;
	struct coord_geo geo;
	struct coord last;
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
		dbg(1, "coord %f,%f\n", priv->geo.lat, priv->geo.lng);
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
	struct route_path_coord_handle *h;
	struct coord *c, *pos, ci;
	int slen, len, dx, dy;

	len = (priv->speed * priv->interval / 1000)/ 3.6;
	dbg(1, "###### Entering simulation loop\n");
	if (!priv->navit) {
		dbg(1, "vehicle->navit is not set. Can't simulate\n");
		return 1;
	}
	struct route *vehicle_route = navit_get_route(priv->navit);
	if (!vehicle_route) {
		dbg(1, "navit_get_route NOK\n");
		return 1;
	}

	h = route_path_coord_open(vehicle_route);
	if (!h) {
		dbg(1, "navit_path_coord_open NOK\n");
		return 1;
	}
	pos = route_path_coord_get(h);
	dbg(1, "current pos=%p\n", pos);
	if (pos) {
		dbg(1, "current pos=0x%x,0x%x\n", pos->x, pos->y);
		dbg(1, "last pos=0x%x,0x%x\n", priv->last.x, priv->last.y);
		if (priv->last.x == pos->x && priv->last.y == pos->y) {
			dbg(1, "endless loop\n");
		}
		priv->last = *pos;
		for (;;) {
			c = route_path_coord_get(h);
			dbg(1, "next pos=%p\n", c);
			if (!c)
				break;
			dbg(1, "next pos=0x%x,0x%x\n", c->x, c->y);
			slen = transform_distance(projection_mg, pos, c);
			dbg(1, "len=%d slen=%d\n", len, slen);
			if (slen < len) {
				len -= slen;
				pos = c;
			} else {
				dx = c->x - pos->x;
				dy = c->y - pos->y;
				ci.x = pos->x + dx * len / slen;
				ci.y = pos->y + dy * len / slen;
				priv->direction =
				    transform_get_angle_delta(pos, c, 0);
				dbg(1, "ci=0x%x,0x%x\n", ci.x, ci.y);
				transform_to_geo(projection_mg, &ci,
						 &priv->geo);
				callback_list_call_0(priv->cbl);
				break;
			}
		}
	}
	return 1;
}



static struct vehicle_priv *
vehicle_demo_new(struct vehicle_methods
		 *meth, struct callback_list
		 *cbl, struct attr **attrs)
{
	struct vehicle_priv *ret;
	struct attr *interval,*speed;

	dbg(1, "enter\n");
	ret = g_new0(struct vehicle_priv, 1);
	ret->cbl = cbl;
	ret->interval=1000;
	ret->speed=40;
	if ((speed=attr_search(attrs, NULL, attr_speed)))
		ret->speed=speed->u.num;
	if ((interval=attr_search(attrs, NULL, attr_interval)))
		ret->interval=speed->u.num;
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
