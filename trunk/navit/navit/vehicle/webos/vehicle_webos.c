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
#include <sys/time.h>
#include <PDL.h>
#include <SDL.h>
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
	struct callback_list *cbl;
	struct callback *event_cb;
	double track, speed, altitude, radius;
	time_t fix_time;
	struct coord_geo geo;
	struct attr ** attrs;
	char fixiso8601[128];
	int pdk_version;
	struct event_timeout *ev_timeout;
	struct callback *timeout_cb;
	char has_fix;
};

static void
vehicle_webos_callback(PDL_ServiceParameters *params, void *user)
{
	PDL_Location *location;
	SDL_Event event;
	SDL_UserEvent userevent;
	int err;

	err = PDL_GetParamInt(params, "errorCode");
	if (err != 0) {
		dbg(0,"Location Callback errorCode %d\n", err);
		return /*PDL_EOTHER*/;
	}

	location = g_new0 (PDL_Location, 1);

	location->altitude = PDL_GetParamDouble(params, "altitude");
	location->velocity = PDL_GetParamDouble(params, "velocity");
	location->heading = PDL_GetParamDouble(params, "heading");
	location->horizontalAccuracy = PDL_GetParamDouble(params, "horizAccuracy");
	location->latitude = PDL_GetParamDouble(params, "latitude");
	location->longitude = PDL_GetParamDouble(params, "longitude");

	userevent.type = SDL_USEREVENT;
	userevent.code = PDL_GPS_UPDATE;
	userevent.data1 = location;
	userevent.data2 = NULL;

	event.type = SDL_USEREVENT;
	event.user = userevent;

	SDL_PushEvent(&event);

	return /*PDL_NOERROR*/;
}

static void
vehicle_webos_gps_update(struct vehicle_priv *priv, PDL_Location *location)
{
	struct timeval tv;
	gettimeofday(&tv,NULL);

	event_remove_timeout(priv->ev_timeout);
	priv->ev_timeout = NULL;

	int new_timeout = (priv->fix_time == 0 ? 1 : (int)difftime(tv.tv_sec, priv->fix_time)) * 2000;

	priv->fix_time = tv.tv_sec;
	priv->has_fix = 1;
	priv->geo.lat = location->latitude;
	/* workaround for webOS GPS bug following */
	priv->geo.lng = (priv->pdk_version == 200 && location->longitude >= -1 && location->longitude <= 1) ? 
		-location->longitude : location->longitude;

	dbg(2,"Location: %f %f %f %.12g %.12g +-%fm\n",
			location->altitude,
			location->velocity,
			location->heading,
			priv->geo.lat,
			priv->geo.lng,
			location->horizontalAccuracy);

	if (location->altitude != -1)
		priv->altitude = location->altitude;
	if (location->velocity != -1)
		priv->speed = location->velocity * 3.6;
	if (location->heading != -1)
		priv->track = location->heading;
	if (location->horizontalAccuracy != -1)
		priv->radius = location->horizontalAccuracy;

	if (priv->pdk_version <= 100)
		g_free(location);

	callback_list_call_attr_0(priv->cbl, attr_position_coord_geo);

	dbg(1, "add timeout(%i)\n", new_timeout);
	priv->ev_timeout = event_add_timeout(new_timeout, 0, priv->timeout_cb);
}

static void
vehicle_webos_timeout_callback(struct vehicle_priv *priv)
{
	dbg(1, "GPS timeout triggered cb(%p)\n", priv->timeout_cb);

	priv->fix_time = 0;
	priv->has_fix = 0;

	callback_list_call_attr_0(priv->cbl, attr_position_fix_type);
}

static void
vehicle_webos_close(struct vehicle_priv *priv)
{
	event_remove_timeout(priv->ev_timeout);
	priv->ev_timeout = NULL;

	callback_destroy(priv->timeout_cb);

	if (priv->pdk_version <= 100)
		PDL_UnregisterServiceCallback((PDL_ServiceCallbackFunc)vehicle_webos_callback);
	else
		PDL_EnableLocationTracking(PDL_FALSE);
}

static int
vehicle_webos_open(struct vehicle_priv *priv)
{
	PDL_Err err;
	
	priv->pdk_version = PDL_GetPDKVersion();	
	dbg(1,"pdk_version(%d)\n", priv->pdk_version);

	if (priv->pdk_version <= 100) {
		err = PDL_ServiceCallWithCallback("palm://com.palm.location/startTracking",
				"{subscribe:true}",
				(PDL_ServiceCallbackFunc)vehicle_webos_callback,
				priv,
				PDL_FALSE);
		if (err != PDL_NOERROR) {
			dbg(0,"PDL_ServiceCallWithCallback failed with (%d): (%s)\n", err, PDL_GetError());
			vehicle_webos_close(priv);
			return 0;
		}
	}
	else {
		dbg(1,"Calling PDL_EnableLocationTracking(PDL_TRUE)\n");
		err = PDL_EnableLocationTracking(PDL_TRUE);
		if (err != PDL_NOERROR) {
			dbg(0,"PDL_EnableLocationTracking failed with (%d): (%s)\n", err, PDL_GetError());
			vehicle_webos_close(priv);
			return 0;
		}
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
			if (priv->fix_time) {
				struct tm tm;
				if (gmtime_r(&priv->fix_time, &tm)) {
					strftime(priv->fixiso8601, sizeof(priv->fixiso8601),
							"%Y-%m-%dT%TZ", &tm);
					attr->u.str=priv->fixiso8601;
				}
				else {
					priv->fix_time = 0;
					return 0;
				}
				dbg(1,"Fix Time: %d %s\n", priv->fix_time, priv->fixiso8601);
			}
			else {
				dbg(1,"Fix Time: %d\n", priv->fix_time);
				return 0;
			}

			break;
		case attr_position_fix_type:
			if (!priv->has_fix || priv->radius == 0.0)
				attr->u.num = 0;	// strength = 1
			else if (priv->radius > 20.0)
				attr->u.num = 1;	// strength >= 2
			else
				attr->u.num = 2;	// strength >= 2

			break;
		case attr_position_sats_used:
			if (!priv->has_fix)
				attr->u.num = 0;
			else if (priv->radius <= 6.0 )
				attr->u.num = 6;	// strength = 5
			else if (priv->radius <= 10.0 )
				attr->u.num = 5;	// strength = 4
			else if (priv->radius <= 15.0 )
				attr->u.num = 4;	// strength = 3
			else
				return 0;

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
		case attr_pdl_gps_update:
			vehicle_webos_gps_update(priv, (PDL_Location *)attr->u.data);
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

	priv->timeout_cb = callback_new_1(callback_cast(vehicle_webos_timeout_callback), priv);

	*meth = vehicle_webos_methods;
	while (*attrs) {
		vehicle_webos_set_attr_do(priv, *attrs, 1);
		attrs++;
	}

	vehicle_webos_open(priv);
	
	return priv;
}

void
plugin_init(void)
{
	dbg(1, "enter\n");
	plugin_register_vehicle_type("webos", vehicle_webos_new);
}

