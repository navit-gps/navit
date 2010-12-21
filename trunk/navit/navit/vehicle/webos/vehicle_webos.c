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

#include <config.h>
#include <string.h>
#include <glib.h>
#include <math.h>
#include <errno.h>
#include <time.h>
#include <PDL.h>
#include "debug.h"
#include "callback.h"
#include "plugin.h"
#include "coord.h"
#include "item.h"
#include "vehicle.h"
#include "event.h"

static char *vehicle_webos_prefix="webos:";

struct vehicle_priv {
	char *source;
	char *address;
	struct callback_list *cbl, *event_cbl;
	struct callback *event_cb;
	double time, track, speed, altitude, radius;
	time_t fix_time;
	struct coord_geo geo;
	struct attr ** attrs;
	char fixiso8601[128];
};

static void
vehicle_webos_callback_event(struct callback_list *cbl, enum attr_type type)
{
	callback_list_call_attr_0(cbl, type);
}

static void
vehicle_webos_callback(PDL_ServiceParameters *params, void *user)
{
	struct vehicle_priv *priv=user;
	int err;

	err = PDL_GetParamInt(params, "errorCode");
	if (err != 0) {
		dbg(0,"Location Callback errorCode %d\n", err);
		return /*PDL_EOTHER*/;
	}

	double altitude = PDL_GetParamDouble(params, "altitude");
	double speed = PDL_GetParamDouble(params, "velocity") * 3.6; /* multiply with 3.6 to get kph */
	double track = PDL_GetParamDouble(params, "heading");
	double horizAccuracy = PDL_GetParamDouble(params, "horizAccuracy");
	priv->geo.lat = PDL_GetParamDouble(params, "latitude");
	priv->geo.lng = PDL_GetParamDouble(params, "longitude");
	double time = PDL_GetParamDouble(params, "timestamp") / 1000;

	dbg(2,"Location: %f %f %f %.12g %.12g +-%fm %f\n",
			altitude,
			speed,
			track,
			priv->geo.lat,
			priv->geo.lng,
			horizAccuracy,
			time);

	if (altitude != -1)
		priv->altitude = altitude;
	if (speed != -1)
		priv->speed = speed;
	if (track != -1)
		priv->track = track;
	if (horizAccuracy != -1)
		priv->radius = horizAccuracy;
	if (time != priv->time) {
		dbg(2,"NEW Time: %f\n", time);
		priv->time = time;
		priv->fix_time = 0;
		event_call_callback(priv->event_cbl);
	}

	return /*PDL_NOERROR*/;
}

static void
vehicle_webos_close(struct vehicle_priv *priv)
{
	PDL_UnregisterServiceCallback((PDL_ServiceCallbackFunc)vehicle_webos_callback);
	callback_list_destroy(priv->event_cbl);
}

static int
vehicle_webos_open(struct vehicle_priv *priv)
{
	PDL_Err err;

	err = PDL_ServiceCallWithCallback("palm://com.palm.location/startTracking",
			"{subscribe:true}",
			(PDL_ServiceCallbackFunc)vehicle_webos_callback,
			priv,
			PDL_FALSE);
	if (err != PDL_NOERROR) {
		dbg(0,"PDL_ServiceCallWithCallback failed\n");
		vehicle_webos_close(priv);
		return 0;
	}

	return 1;
}


static void
vehicle_webos_destroy(struct vehicle_priv *priv)
{
	vehicle_webos_close(priv);
	if (priv->source)
		g_free(priv->source);
	g_free(priv);
}

static int
vehicle_webos_position_attr_get(struct vehicle_priv *priv,
		enum attr_type type, struct attr *attr)
{
	switch (type) {
		case attr_position_height:
			dbg(1,"Altitude: %f\n", priv->altitude);
			attr->u.numd = &priv->altitude;
			break;
		case attr_position_speed:
			dbg(1,"Speed: %f\n", priv->speed);
			attr->u.numd = &priv->speed;
			break;
		case attr_position_direction:
			dbg(1,"Direction: %f\n", priv->track);
			attr->u.numd = &priv->track;
			break;
		case attr_position_coord_geo:
			dbg(1,"Coord: %.12g %.12g\n", priv->geo.lat, priv->geo.lng);
			attr->u.coord_geo = &priv->geo;
			break;
		case attr_position_radius:
			dbg(1,"Radius: %f\n", priv->radius);
			attr->u.numd = &priv->radius;
			break;
		case attr_position_time_iso8601:
			if (!priv->time)
				return 0;

			if (!priv->fix_time) {
				struct tm tm;
				priv->fix_time = priv->time;
				if (gmtime_r(&priv->fix_time, &tm))
					strftime(priv->fixiso8601, sizeof(priv->fixiso8601),
							"%Y-%m-%dT%TZ", &tm);
				else {
					priv->fix_time = 0;
					return 0;
				}
			}
			dbg(1,"Fix Time: %d %s\n", priv->fix_time, priv->fixiso8601);
			attr->u.str=priv->fixiso8601;

			break;
		default:
			return 0;
	}
	attr->type = type;
	return 1;
}

static int
vehicle_webos_set_attr_do(struct vehicle_priv *priv, struct attr *attr, int init)
{
	switch (attr->type) {
		case attr_source:
			if (strncmp(vehicle_webos_prefix,attr->u.str,strlen(vehicle_webos_prefix))) {
				dbg(1,"source must start with '%s'\n", vehicle_webos_prefix);
				return 0;
			}
			g_free(priv->source);
			priv->source=g_strdup(attr->u.str);
			priv->address=priv->source+strlen(vehicle_webos_prefix);
			if (!priv->address[0])
				priv->address=NULL;
			if (!init) {
				vehicle_webos_close(priv);
				vehicle_webos_open(priv);
			}
			return 1;
		case attr_profilename:
			return 1;
		default:
			return 0;
	}
}

static int
vehicle_webos_set_attr(struct vehicle_priv *priv, struct attr *attr)
{
	return vehicle_webos_set_attr_do(priv, attr, 0);
}

struct vehicle_methods vehicle_webos_methods = {
	vehicle_webos_destroy,
	vehicle_webos_position_attr_get,
	vehicle_webos_set_attr,
};

static struct vehicle_priv *
vehicle_webos_new(struct vehicle_methods
		*meth, struct callback_list
		*cbl, struct attr **attrs)
{
	struct vehicle_priv *priv;


	priv = g_new0(struct vehicle_priv, 1);
	priv->attrs = attrs;
	priv->cbl = cbl;
	*meth = vehicle_webos_methods;
	while (*attrs) {
		vehicle_webos_set_attr_do(priv, *attrs, 1);
		attrs++;
	}
	priv->event_cbl = callback_list_new();
	priv->event_cb = callback_new_2(callback_cast(vehicle_webos_callback_event),
			cbl, attr_position_coord_geo);
	callback_list_add(priv->event_cbl, priv->event_cb);

	vehicle_webos_open(priv);
	
	return priv;
}

void
plugin_init(void)
{
	dbg(1, "enter\n");
	plugin_register_vehicle_type("webos", vehicle_webos_new);
}

