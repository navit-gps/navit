#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <glib.h>
#ifdef HAVE_LIBGPS
#include <gps.h>
#endif
#include "coord.h"
#include "transform.h"
#include "statusbar.h"
#include "vehicle.h"

/* #define INTERPOLATION_TIME 50 */

struct callback {
	void (*func)(struct vehicle *, void *data);
	void *data;
};

struct vehicle {
	GIOChannel *iochan;
	int timer_count;
	int qual;
	int sats;
	double lat,lng;
	double height;
	double dir,speed;
	double time;
	struct coord current_pos;
	struct coord_d curr;
	struct coord_d delta;

	double speed_last;
#ifdef HAVE_LIBGPS
	struct gps_data_t *gps;
#endif
	GList *callbacks;
};

struct vehicle *vehicle_last;

#if INTERPOLATION_TIME
static int
vehicle_timer(gpointer t)
{
	struct vehicle *this=t;	
/*	if (this->timer_count++ < 1000/INTERPOLATION_TIME) { */
		if (this->delta.x || this->delta.y) {
			this->curr.x+=this->delta.x;
			this->curr.y+=this->delta.y;	
			this->current_pos.x=this->curr.x;
			this->current_pos.y=this->curr.y;
			if (this->callback_func)
				(*this->callback_func)(this->callback_data);
		}
/*	} */
	return TRUE; 
}
#endif

static void
vehicle_call_callbacks(struct vehicle *this)
{
	GList *item=g_list_first(this->callbacks);
	while (item) {
		struct callback *cb=item->data;
		(*cb->func)(this, cb->data);
		item=g_list_next(item);
	}
}

struct coord *
vehicle_pos_get(struct vehicle *this)
{
	return &this->current_pos;
}

double 
vehicle_speed_get(struct vehicle *this)
{
	return this->speed;
}

double *
vehicle_dir_get(struct vehicle *this)
{
	return &this->dir;
}

double 
vehicle_height_get(struct vehicle *this)
{
	return this->height;
}

void
vehicle_set_position(struct vehicle *this, struct coord *pos)
{
	this->current_pos=*pos;
	this->curr.x=this->current_pos.x;
	this->curr.y=this->current_pos.y;
	this->delta.x=0;
	this->delta.y=0;
	vehicle_call_callbacks(this);
}

static void
vehicle_parse_gps(struct vehicle *this, char *buffer)
{
	char *p,*item[16];
	double lat,lng,scale,speed;
	int i,debug=0;

	if (debug) {
		printf("GPS %s\n", buffer);
	}
	if (!strncmp(buffer,"$GPGGA",6)) {
		/* $GPGGA,184424.505,4924.2811,N,01107.8846,E,1,05,2.5,408.6,M,,,,0000*0C
			UTC of Fix,Latitude,N/S,Longitude,E/W,Quality,Satelites,HDOP,Altitude,"M"
		*/
		i=0;
		p=buffer;
		while (i < 16) {
			item[i++]=p;
			while (*p && *p != ',') 
				p++;	
			if (! *p) break;
			*p++='\0';
		}

		sscanf(item[2],"%lf",&lat);
		this->lat=floor(lat/100);
		lat-=this->lat*100;
		this->lat+=lat/60;

		sscanf(item[4],"%lf",&lng);
		this->lng=floor(lng/100);
		lng-=this->lng*100;
		this->lng+=lng/60;

		sscanf(item[6],"%d",&this->qual);
		sscanf(item[7],"%d",&this->sats);
		sscanf(item[9],"%lf",&this->height);
		
		transform_mercator(&this->lng, &this->lat, &this->current_pos);
			
		this->curr.x=this->current_pos.x;
		this->curr.y=this->current_pos.y;
		this->timer_count=0;
		vehicle_call_callbacks(this);
	}
	if (!strncmp(buffer,"$GPVTG",6)) {
		/* $GPVTG,143.58,T,,M,0.26,N,0.5,K*6A 
		          Course Over Ground Degrees True,"T",Course Over Ground Degrees Magnetic,"M",
			  Speed in Knots,"N","Speed in KM/H","K",*CHECKSUM */
		
		i=0;
		p=buffer;
		while (i < 16) {
			item[i++]=p;
			while (*p && *p != ',') 
				p++;	
			if (! *p) break;
				*p++='\0';
		}
		sscanf(item[1],"%lf",&this->dir);
		sscanf(item[7],"%lf",&this->speed);

		scale=transform_scale(this->current_pos.y);
		speed=this->speed+(this->speed-this->speed_last)/2;
#ifdef INTERPOLATION_TIME
		this->delta.x=sin(M_PI*this->dir/180)*speed*scale/3600*INTERPOLATION_TIME;
		this->delta.y=cos(M_PI*this->dir/180)*speed*scale/3600*INTERPOLATION_TIME;
#endif
		this->speed_last=this->speed;
	}
}

static gboolean
vehicle_track(GIOChannel *iochan, GIOCondition condition, gpointer t)
{
	struct vehicle *this=t;
	GError *error=NULL;
	char buffer[4096];
	char *str,*tok;
	gsize size;
	
	if (condition == G_IO_IN) {
#ifdef HAVE_LIBGPS
		if (this->gps) {
			vehicle_last=this;
			gps_poll(this->gps);
		} else {
#else
		{
#endif
			g_io_channel_read_chars(iochan, buffer, 4096, &size, &error);
			buffer[size]='\0';
			str=buffer;
			while ((tok=strtok(str, "\n"))) {
				str=NULL;
				vehicle_parse_gps(this, tok);
			}
		}

		return TRUE;
	} 
	return FALSE;
}

#ifdef HAVE_LIBGPS
static void
vehicle_gps_callback(struct gps_data_t *data, char *buf, size_t len, int level)
{
	struct vehicle *this=vehicle_last;
	double scale,speed;
#if INTERPOLATION_TIME
	if (! (data->set & TIME_SET)) {
		return;
	}
	data->set &= ~TIME_SET;
	if (this->time == data->fix.time)
		return;
	this->time=data->fix.time;
#endif
	if (data->set & SPEED_SET) {
		this->speed_last=this->speed;
		this->speed=data->fix.speed*3.6;
		data->set &= ~SPEED_SET;
	}
	if (data->set & TRACK_SET) {
		speed=this->speed+(this->speed-this->speed_last)/2;
		this->dir=data->fix.track;
		scale=transform_scale(this->current_pos.y);
#ifdef INTERPOLATION_TIME
		this->delta.x=sin(M_PI*this->dir/180)*speed*scale/3600*INTERPOLATION_TIME;
		this->delta.y=cos(M_PI*this->dir/180)*speed*scale/3600*INTERPOLATION_TIME;
#endif
		data->set &= ~TRACK_SET;
	}
	if (data->set & LATLON_SET) {
		this->lat=data->fix.latitude;
		this->lng=data->fix.longitude;
		transform_mercator(&this->lng, &this->lat, &this->current_pos);
		this->curr.x=this->current_pos.x;
		this->curr.y=this->current_pos.y;
		this->timer_count=0;
		vehicle_call_callbacks(this);
		data->set &= ~LATLON_SET;
	}
	if (data->set & ALTITUDE_SET) {
		this->height=data->fix.altitude;
		data->set &= ~ALTITUDE_SET;
	}
	if (data->set & SATELLITE_SET) {
		this->sats=data->satellites;
		data->set &= ~SATELLITE_SET;
	}
	if (data->set & STATUS_SET) {
		this->qual=data->status;
		data->set &= ~STATUS_SET;
	}
}
#endif

struct vehicle *
vehicle_new(const char *url)
{
	struct vehicle *this;
	GError *error=NULL;
	int fd=-1;
	char *url_,*colon;
#ifdef HAVE_LIBGPS
	struct gps_data_t *gps=NULL;
#endif

	if (! strncmp(url,"file:",5)) {
		fd=open(url+5,O_RDONLY|O_NDELAY);
		if (fd < 0) {
			g_warning("Failed to open %s", url);
		}
	} else if (! strncmp(url,"gpsd://",7)) {
#ifdef HAVE_LIBGPS
		url_=g_strdup(url);
		colon=index(url_+7,':');
		if (colon) {
			*colon=0;
			gps=gps_open(url_+7,colon+1);
		} else
			gps=gps_open(url+7,NULL);
		g_free(url_);
		if (! gps) {
			g_warning("Failed to connect to %s", url);
		} else {
			gps_query(gps, "w+x\n");
			gps_set_raw_hook(gps, vehicle_gps_callback);
			fd=gps->gps_fd;
		}
#else
		g_warning("No support for gpsd compiled in\n");
#endif
	}
	this=g_new0(struct vehicle,1);
#ifdef HAVE_LIBGPS
	if(gps)
		this->gps=gps;
#endif
	if(fd !=-1) {
		this->iochan=g_io_channel_unix_new(fd);
		g_io_channel_set_encoding(this->iochan, NULL, &error);
		g_io_add_watch(this->iochan, G_IO_IN|G_IO_ERR|G_IO_HUP, vehicle_track, this);
	}
	this->current_pos.x=0x130000;
	this->current_pos.y=0x600000;
	this->curr.x=this->current_pos.x;
	this->curr.y=this->current_pos.y;
	this->delta.x=0;
	this->delta.y=0;
#if INTERPOLATION_TIME
		g_timeout_add(INTERPOLATION_TIME, vehicle_timer, this);
#endif
	
	return this;
}

void *
vehicle_callback_register(struct vehicle *this, void (*func)(struct vehicle *, void *data), void *data)
{
	struct callback *cb;
	cb=g_new(struct callback, 1);
	cb->func=func;
	cb->data=data;
	this->callbacks=g_list_prepend(this->callbacks, cb);
	return cb;
}

void
vehicle_callback_unregister(struct vehicle *this, void *handle)
{
	g_list_remove(this->callbacks, handle);
}

void
vehicle_destroy(struct vehicle *this)
{
	GError *error=NULL;
	GList *item=g_list_first(this->callbacks),*next;


	g_io_channel_shutdown(this->iochan,0,&error);
#ifdef HAVE_LIBGPS
	if (this->gps)
		gps_close(this->gps);
#endif
	while (item) {
		next=g_list_next(item);
		vehicle_callback_unregister(this, item->data);
		item=next;
	}
	g_list_free(this->callbacks);
	g_free(this);
}
