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
#include <gypsy/gypsy-device.h>
#include <gypsy/gypsy-control.h>
#include <gypsy/gypsy-course.h>
#include <gypsy/gypsy-position.h>
#include <gypsy/gypsy-satellite.h>
#include <string.h>
#include <glib.h>
#include <math.h>
#include "debug.h"
#include "callback.h"
#include "plugin.h"
#include "coord.h"
#include "item.h"
#include "vehicle.h"

static struct vehicle_priv {
	char *source;
	GypsyControl *control;
	GypsyPosition *position;
	GypsyDevice *device;
	GypsyCourse *course;
	GypsySatellite *satellite;
	char *path;
	struct callback_list *cbl;
	guint retry_interval;
	struct coord_geo geo;
	double speed;
	double direction;
	double height;
	int status;
	int sats;
	int sats_used;
	guint retry_timer;
	struct attr ** attrs;
} *vehicle_last;

#define DEFAULT_RETRY_INTERVAL 10 // seconds
#define MIN_RETRY_INTERVAL 1 // seconds

static void
vehicle_gypsy_fixstatus_changed(GypsyDevice *device,
		gint fixstatus,
		gpointer userdata)
{
	struct vehicle_priv *priv = vehicle_last;

	if (fixstatus==GYPSY_DEVICE_FIX_STATUS_3D || 
			fixstatus==GYPSY_DEVICE_FIX_STATUS_2D)
		priv->status=1; // from gpsd: 1=fix ; 2=DGPS fix
	else
		priv->status=0;

	callback_list_call_0(priv->cbl);
}

static void 
vehicle_gypsy_position_changed(GypsyPosition *position, 
		GypsyPositionFields fields_set, int timestamp, 
		double latitude, double longitude, double altitude, 
		gpointer userdata)
{
	struct vehicle_priv *priv = vehicle_last;
	int cb = FALSE;

	if (fields_set & GYPSY_POSITION_FIELDS_LATITUDE)
	{
		cb = TRUE;
		priv->geo.lat = latitude;
	}
	if (fields_set & GYPSY_POSITION_FIELDS_LONGITUDE)
	{
		cb = TRUE;
		priv->geo.lng = longitude;
	}
	if (fields_set & GYPSY_POSITION_FIELDS_ALTITUDE)
	{
		cb = TRUE;
		priv->height = altitude;
	}

	if (cb)
		callback_list_call_0(priv->cbl);
}

static void 
vehicle_gypsy_satellite_changed(GypsySatellite *satellite, 
		GPtrArray *satellites,
		gpointer userdata)
{
	struct vehicle_priv *priv = vehicle_last;

	int i, sats, used=0;
	
	sats = satellites->len;
	for (i = 0; i < sats; i++) {
		GypsySatelliteDetails *details = satellites->pdata[i];
		if (details->in_use)
			used++;
	}

	priv->sats_used = used;
	priv->sats = sats;
	
	callback_list_call_0(priv->cbl);
}

static void 
vehicle_gypsy_course_changed (GypsyCourse *course, 
		GypsyCourseFields fields,
		int timestamp,
		double speed,
		double direction,
		double climb,
		gpointer userdata)
{
	struct vehicle_priv *priv = vehicle_last;
	int cb = FALSE;

	if (fields & GYPSY_COURSE_FIELDS_SPEED)
	{
		priv->speed = speed;
		cb = TRUE;
	}
	if (fields & GYPSY_COURSE_FIELDS_DIRECTION)
	{
		priv->direction = direction;
		cb = TRUE;
	}

	if (cb)
		callback_list_call_0(priv->cbl);
}

/**
 * Attempt to open the gps device.
 * Return FALSE if retry not required
 * Return TRUE to try again
 */
static gboolean
vehicle_gypsy_try_open(gpointer *data)
{
	struct vehicle_priv *priv = (struct vehicle_priv *)data;
	char *source = g_strdup(priv->source);

	GError *error = NULL;
	
	g_type_init();
	priv->control = gypsy_control_get_default(); 
	priv->path = gypsy_control_create(priv->control, source+8, &error); 
	if (priv->path == NULL) { 
		g_warning ("Error creating gypsy conrtol path for %s: %s", source+8, error->message); 
		return TRUE; 
	}
	
	priv->position = gypsy_position_new(priv->path);
	g_signal_connect(priv->position, "position-changed", G_CALLBACK (vehicle_gypsy_position_changed), NULL);

	priv->satellite = gypsy_satellite_new(priv->path);
	g_signal_connect(priv->satellite, "satellites-changed", G_CALLBACK (vehicle_gypsy_satellite_changed), NULL);

	priv->course = gypsy_course_new(priv->path);
	g_signal_connect(priv->course, "course-changed", G_CALLBACK (vehicle_gypsy_course_changed), NULL);

	priv->device = gypsy_device_new(priv->path);
	g_signal_connect(priv->device, "fix-status-changed", G_CALLBACK (vehicle_gypsy_fixstatus_changed), NULL);

	gypsy_device_start(priv->device, &error);
	if (error != NULL) { 
		g_warning ("Error starting gypsy for %s: %s", source+8, error->message); 
		return TRUE;
	}

	vehicle_last = priv;
	dbg(0,"gypsy connected to %d\n", source+8);
	g_free(source);
	return FALSE;
}

/**
 * Open a connection to gypsy. Will re-try the connection if it fails
 */
static void
vehicle_gypsy_open(struct vehicle_priv *priv)
{
	priv->retry_timer=0;
	if (vehicle_gypsy_try_open((gpointer *)priv)) {
		priv->retry_timer = g_timeout_add(priv->retry_interval*1000, (GSourceFunc)vehicle_gypsy_try_open, (gpointer *)priv);
	}
}

static void
vehicle_gypsy_close(struct vehicle_priv *priv)
{
	if (priv->retry_timer) {
		g_source_remove(priv->retry_timer);
		priv->retry_timer=0;
	}
	if (priv->path)
		g_free(priv->path);
	
	if (priv->position)
		g_free(priv->position);
	
	if (priv->satellite)
		g_free(priv->satellite);

	if (priv->course)
		g_free(priv->course);

	if (priv->device)
		g_free(priv->device);
	
	if (priv->control)
		g_object_unref(G_OBJECT (priv->control));
}

static void
vehicle_gypsy_destroy(struct vehicle_priv *priv)
{
	vehicle_gypsy_close(priv);
	if (priv->source)
		g_free(priv->source);
	g_free(priv);
}

static int
vehicle_gypsy_position_attr_get(struct vehicle_priv *priv,
			       enum attr_type type, struct attr *attr)
{
	struct attr * active=NULL;
	switch (type) {
	case attr_position_height:
		attr->u.numd = &priv->height;
		break;
	case attr_position_speed:
		attr->u.numd = &priv->speed;
		break;
	case attr_position_direction:
		attr->u.numd = &priv->direction;
		break;
	case attr_position_qual:
		attr->u.num = priv->sats;
		break;
	case attr_position_sats_used:
		attr->u.num = priv->sats_used;
		break;
	case attr_position_coord_geo:
		attr->u.coord_geo = &priv->geo;
		break;
	case attr_active:
	  active = attr_search(priv->attrs,NULL,attr_active);
	  if(active != NULL && active->u.num == 1)
	    return 1;
	  else
	    return 0;
	       break;

	default:
		return 0;
	}
	attr->type = type;
	return 1;
}

struct vehicle_methods vehicle_gypsy_methods = {
	vehicle_gypsy_destroy,
	vehicle_gypsy_position_attr_get,
};

static struct vehicle_priv *
vehicle_gypsy_new_gypsy(struct vehicle_methods
		      *meth, struct callback_list
		      *cbl, struct attr **attrs)
{
	struct vehicle_priv *ret;
	struct attr *source, *retry_int;

	dbg(1, "enter\n");
	source = attr_search(attrs, NULL, attr_source);
	ret = g_new0(struct vehicle_priv, 1);
	ret->source = g_strdup(source->u.str);
	ret->attrs = attrs;
	retry_int = attr_search(attrs, NULL, attr_retry_interval);
	if (retry_int) {
		ret->retry_interval = retry_int->u.num;
		if (ret->retry_interval < MIN_RETRY_INTERVAL) {
			dbg(0, "Retry interval %d too small, setting to %d\n", ret->retry_interval, MIN_RETRY_INTERVAL);
			ret->retry_interval = MIN_RETRY_INTERVAL;
		}
	} else {
		dbg(0, "Retry interval not defined, setting to %d\n", DEFAULT_RETRY_INTERVAL);
		ret->retry_interval = DEFAULT_RETRY_INTERVAL;
	}
	ret->cbl = cbl;
	*meth = vehicle_gypsy_methods;
	vehicle_gypsy_open(ret);
	return ret;
}

void
plugin_init(void)
{
	dbg(1, "enter\n");
	plugin_register_vehicle_type("gypsy", vehicle_gypsy_new_gypsy);
}
