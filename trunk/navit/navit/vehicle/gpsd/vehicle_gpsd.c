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
#include <gps.h>
#include <string.h>
#include <glib.h>
#include <math.h>
#ifdef HAVE_GPSBT
#include <gpsbt.h>
#include <errno.h>
#endif
#include "debug.h"
#include "callback.h"
#include "plugin.h"
#include "coord.h"
#include "item.h"
#include "vehicle.h"
#include "event.h"

static struct vehicle_priv {
	char *source;
	char *gpsd_query;
	struct callback_list *cbl;
	struct callback *cb;
	struct event_watch *evwatch;
	guint retry_interval;
	struct gps_data_t *gps;
	struct coord_geo geo;
	double speed;
	double direction;
	double height;
	double hdop;
	int status;
	int fix_type;
	time_t fix_time;
	int sats;
	int sats_signal;
	int sats_used;
	char *nmea_data;
	char *nmea_data_buf;
	guint retry_timer;
	struct attr ** attrs;
	char fixiso8601[128];
#ifdef HAVE_GPSBT
	gpsbt_t context;
#endif
} *vehicle_last;

#define DEFAULT_RETRY_INTERVAL 10 // seconds
#define MIN_RETRY_INTERVAL 1 // seconds

static void vehicle_gpsd_io(struct vehicle_priv *priv);

static void
vehicle_gpsd_callback(struct gps_data_t *data, char *buf, size_t len,
		      int level)
{
	char *pos,*nmea_data_buf;
        int i=0,sats_signal=0;
       
	struct vehicle_priv *priv = vehicle_last;
	if( len > 0 && buf[0] == '$' ) {
		char buffer[len+2];
		buffer[len+1]='\0';
		memcpy(buffer, buf, len);
		pos=strchr(buffer,'\n');
		if (pos) {
			*++pos='\0';
			if (!priv->nmea_data_buf || strlen(priv->nmea_data_buf) < 65536) {
				nmea_data_buf=g_strconcat(priv->nmea_data_buf ? priv->nmea_data_buf : "", buffer, NULL);
				g_free(priv->nmea_data_buf);
				priv->nmea_data_buf=nmea_data_buf;
			} else {
				dbg(0, "nmea buffer overflow, discarding '%s'\n", buffer);
			}
		}
	}	
	dbg(1,"data->set=0x%x\n", data->set);
	if (data->set & SPEED_SET) {
		priv->speed = data->fix.speed * 3.6;
		if(!isnan(data->fix.speed))
			callback_list_call_attr_0(priv->cbl, attr_position_speed);
		data->set &= ~SPEED_SET;
	}
	if (data->set & TRACK_SET) {
		priv->direction = data->fix.track;
		data->set &= ~TRACK_SET;
	}
	if (data->set & ALTITUDE_SET) {
		priv->height = data->fix.altitude;
		data->set &= ~ALTITUDE_SET;
	}
	if (data->set & SATELLITE_SET) {
                if(data->satellites > 0) {
                        sats_signal=0;
                        for( i=0;i<data->satellites;i++) {
                               if (data->ss[i] > 0)
                                        sats_signal++;
                        }
                }
		if (priv->sats_used != data->satellites_used || priv->sats != data->satellites || priv->sats_signal != sats_signal ) {
			priv->sats_used = data->satellites_used;
			priv->sats = data->satellites;
                        priv->sats_signal = sats_signal;
			callback_list_call_attr_0(priv->cbl, attr_position_sats);
		}
		data->set &= ~SATELLITE_SET;
	}
	if (data->set & STATUS_SET) {
		priv->status = data->status;
		data->set &= ~STATUS_SET;
	}
	if (data->set & MODE_SET) {
		priv->fix_type = data->fix.mode - 1;
		data->set &= ~MODE_SET;
	}
	if (data->set & TIME_SET) {
		priv->fix_time = data->fix.time;
		data->set &= ~TIME_SET;
	}
	if (data->set & PDOP_SET) {
		dbg(1, "pdop : %g\n", data->pdop);
		priv->hdop = data->hdop;
		data->set &= ~PDOP_SET;
	}
	if (data->set & LATLON_SET) {
		priv->geo.lat = data->fix.latitude;
		priv->geo.lng = data->fix.longitude;
		dbg(1,"lat=%f lng=%f\n", priv->geo.lat, priv->geo.lng);
		g_free(priv->nmea_data);
		priv->nmea_data=priv->nmea_data_buf;
		priv->nmea_data_buf=NULL;
		data->set &= ~LATLON_SET;
	}
	// If data->fix.speed is NAN, then the drawing gets jumpy.
	if (! isnan(data->fix.speed) && priv->fix_type > 0) {
		callback_list_call_attr_0(priv->cbl, attr_position_coord_geo);
	}
	dbg(2,"speed ok\n");
}

/**
 * Attempt to open the gps device.
 * Return FALSE if retry not required
 * Return TRUE to try again
 */
static gboolean
vehicle_gpsd_try_open(gpointer *data)
{
	struct vehicle_priv *priv = (struct vehicle_priv *)data;
	char *source = g_strdup(priv->source);
	char *colon = index(source + 7, ':');
	char *port=NULL;
	if (colon) {
		*colon = '\0';
		port=colon+1;
	}
	dbg(0,"Trying to connect to %s:%s\n",source+7,port?port:"default");
	priv->gps = gps_open(source + 7, port);
	g_free(source);

	if (!priv->gps){
		dbg(0,"gps_open failed for '%s'. Retrying in %d seconds. Have you started gpsd?\n", priv->source, priv->retry_interval);
		return TRUE;
	}
	gps_query(priv->gps, priv->gpsd_query);
	gps_set_raw_hook(priv->gps, vehicle_gpsd_callback);
	priv->cb = callback_new_1(callback_cast(vehicle_gpsd_io), priv);
	priv->evwatch = event_add_watch((void *)priv->gps->gps_fd, event_watch_cond_read, priv->cb);
	if (!priv->gps->gps_fd) {
		dbg(0,"Warning: gps_fd is 0, most likely you have used a gps.h incompatible to libgps");
	}
	dbg(0,"Connected to gpsd fd=%d evwatch=%p\n", priv->gps->gps_fd, priv->evwatch);
	return FALSE;
}

/**
 * Open a connection to gpsd. Will re-try the connection if it fails
 */
static void
vehicle_gpsd_open(struct vehicle_priv *priv)
{
#ifdef HAVE_GPSBT
	char errstr[256] = "";
	/* We need to start gpsd (via gpsbt) first. */
	errno = 0;
	memset(&priv->context, 0, sizeof(gpsbt_t));
	if(gpsbt_start(NULL, 0, 0, 0, errstr, sizeof(errstr),
		0, &priv->context) < 0) {
	       dbg(0,"Error connecting to GPS with gpsbt: (%d) %s (%s)\n",
		  errno, strerror(errno), errstr);
	}
	sleep(1);       /* give gpsd time to start */
	dbg(1,"gpsbt_start: completed\n");
#endif
	priv->retry_timer=0;
	if (vehicle_gpsd_try_open((gpointer *)priv)) {
		priv->retry_timer = g_timeout_add(priv->retry_interval*1000, (GSourceFunc)vehicle_gpsd_try_open, (gpointer *)priv);
	}
}

static void
vehicle_gpsd_close(struct vehicle_priv *priv)
{
#ifdef HAVE_GPSBT
	int err;
#endif

	if (priv->retry_timer) {
		g_source_remove(priv->retry_timer);
		priv->retry_timer=0;
	}
	if (priv->evwatch) {
		event_remove_watch(priv->evwatch);
		priv->evwatch = NULL;
	}
	if (priv->cb) {
		callback_destroy(priv->cb);
		priv->cb = NULL;
	}
	if (priv->gps) {
		gps_close(priv->gps);
		priv->gps = NULL;
	}
#ifdef HAVE_GPSBT
	err = gpsbt_stop(&priv->context);
	if (err < 0) {
		dbg(0,"Error %d while gpsbt_stop", err);
	}
	dbg(1,"gpsbt_stop: completed, (%d)",err);
#endif
}

static void
vehicle_gpsd_io(struct vehicle_priv *priv)
{
	dbg(1, "enter\n");
	if (priv->gps) {
	 	vehicle_last = priv;
                if (gps_poll(priv->gps)) {
			g_warning("gps_poll failed\n");
			vehicle_gpsd_close(priv);
			vehicle_gpsd_open(priv);
                }
	}
}

static void
vehicle_gpsd_destroy(struct vehicle_priv *priv)
{
	vehicle_gpsd_close(priv);
	if (priv->source)
		g_free(priv->source);
	if (priv->gpsd_query)
		g_free(priv->gpsd_query);
	g_free(priv);
}

static int
vehicle_gpsd_position_attr_get(struct vehicle_priv *priv,
			       enum attr_type type, struct attr *attr)
{
	struct attr * active=NULL;
	switch (type) {
	case attr_position_fix_type:
		attr->u.num = priv->fix_type;
		break;
	case attr_position_height:
		attr->u.numd = &priv->height;
		break;
	case attr_position_speed:
		attr->u.numd = &priv->speed;
		break;
	case attr_position_direction:
		attr->u.numd = &priv->direction;
		break;
	case attr_position_hdop:
		attr->u.numd = &priv->hdop;
		break;
	case attr_position_qual:
		attr->u.num = priv->sats;
		break;
	case attr_position_sats_signal:
		attr->u.num = priv->sats_signal;
		break;
	case attr_position_sats_used:
		attr->u.num = priv->sats_used;
		break;
	case attr_position_coord_geo:
		attr->u.coord_geo = &priv->geo;
		break;
	case attr_position_nmea:
		attr->u.str=priv->nmea_data;
		if (! attr->u.str)
			return 0;
		break;
	case attr_position_time_iso8601:
		{
		struct tm tm;
		if (!priv->fix_time)
			return 0;
		if (gmtime_r(&priv->fix_time, &tm)) {
			strftime(priv->fixiso8601, sizeof(priv->fixiso8601),
				"%Y-%m-%dT%TZ", &tm);
			attr->u.str=priv->fixiso8601;
		} else
			return 0;
		}
		break;
	case attr_active:
	  active = attr_search(priv->attrs,NULL,attr_active);
	  if(active != NULL) {
		attr->u.num=active->u.num;
	    return 1;
	  } else
	    return 0;
	       break;
	default:
		return 0;
	}
	attr->type = type;
	return 1;
}

struct vehicle_methods vehicle_gpsd_methods = {
	vehicle_gpsd_destroy,
	vehicle_gpsd_position_attr_get,
};

static struct vehicle_priv *
vehicle_gpsd_new_gpsd(struct vehicle_methods
		      *meth, struct callback_list
		      *cbl, struct attr **attrs)
{
	struct vehicle_priv *ret;
	struct attr *source, *query, *retry_int;

	dbg(1, "enter\n");
	source = attr_search(attrs, NULL, attr_source);
	ret = g_new0(struct vehicle_priv, 1);
	ret->source = g_strdup(source->u.str);
	query = attr_search(attrs, NULL, attr_gpsd_query);
	if (query) {
		ret->gpsd_query = g_strconcat(query->u.str, "\n", NULL);
	} else {
		ret->gpsd_query = g_strdup("w+x\n");
	}
	dbg(1,"Format string for gpsd_query: %s\n",ret->gpsd_query);
	retry_int = attr_search(attrs, NULL, attr_retry_interval);
	if (retry_int) {
		ret->retry_interval = retry_int->u.num;
		if (ret->retry_interval < MIN_RETRY_INTERVAL) {
			dbg(0, "Retry interval %d too small, setting to %d\n", ret->retry_interval, MIN_RETRY_INTERVAL);
			ret->retry_interval = MIN_RETRY_INTERVAL;
		}
	} else {
		dbg(1, "Retry interval not defined, setting to %d\n", DEFAULT_RETRY_INTERVAL);
		ret->retry_interval = DEFAULT_RETRY_INTERVAL;
	}
	ret->cbl = cbl;
	*meth = vehicle_gpsd_methods;
	ret->attrs = attrs;
	vehicle_gpsd_open(ret);
	return ret;
}

void
plugin_init(void)
{
	dbg(1, "enter\n");
	plugin_register_vehicle_type("gpsd", vehicle_gpsd_new_gpsd);
}
