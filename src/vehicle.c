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

struct vehicle {
	GIOChannel *iochan;
	int timer_count;
	int qual;
	int sats;
	double lat,lng;
	double height;
	double dir,speed;
	struct coord current_pos;
	struct coord_d curr;
	struct coord_d delta;

	double speed_last;
#ifdef HAVE_LIBGPS
	struct gps_data_t *gps;
#endif
	void (*callback_func)(void *data);
	void *callback_data;
};

struct vehicle *vehicle_last;

#if 0
static int
vehicle_timer(gpointer t)
{
	struct vehicle *this=t;	
	if (this->timer_count++ < 10) {
		this->curr.x+=this->delta.x;
		this->curr.y+=this->delta.y;	
		this->current_pos.x=this->curr.x;
		this->current_pos.y=this->curr.y;
		if (this->callback_func)
			(*this->callback_func)(this->callback_data);
	}
	return TRUE;
}
#endif

struct coord *
vehicle_pos_get(struct vehicle *this)
{
	return &this->current_pos;
}

double *
vehicle_speed_get(struct vehicle *this)
{
	return &this->speed;
}

double *
vehicle_dir_get(struct vehicle *this)
{
	return &this->dir;
}

void
vehicle_set_position(struct vehicle *this, struct coord *pos)
{
	this->current_pos=*pos;
	this->curr.x=this->current_pos.x;
	this->curr.y=this->current_pos.y;
	this->delta.x=0;
	this->delta.y=0;
	if (this->callback_func)
		(*this->callback_func)(this->callback_data);
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
		if (this->callback_func)
			(*this->callback_func)(this->callback_data);
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
		this->delta.x=sin(M_PI*this->dir/180)*speed*scale/36;
		this->delta.y=cos(M_PI*this->dir/180)*speed*scale/36;
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
	if (data->set & SPEED_SET) {
		this->speed_last=this->speed;
		this->speed=data->fix.speed*1.852;
		data->set &= ~SPEED_SET;
	}
	if (data->set & TRACK_SET) {
		speed=this->speed+(this->speed-this->speed_last)/2;
		this->dir=data->fix.track;
		scale=transform_scale(this->current_pos.y);
		this->delta.x=sin(M_PI*this->dir/180)*speed*scale/36;
		this->delta.y=cos(M_PI*this->dir/180)*speed*scale/36;
		data->set &= ~TRACK_SET;
	}
	if (data->set & LATLON_SET) {
		this->lat=data->fix.latitude;
		this->lng=data->fix.longitude;
		transform_mercator(&this->lng, &this->lat, &this->current_pos);
		this->curr.x=this->current_pos.x;
		this->curr.y=this->current_pos.y;
		if (this->callback_func)
			(*this->callback_func)(this->callback_data);
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
			return NULL;
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
			return NULL;
		}
		gps_query(gps, "w+x\n");
		gps_set_raw_hook(gps, vehicle_gps_callback);
		fd=gps->gps_fd;
#else
		g_warning("No support for gpsd compiled in\n");
		return NULL;
#endif
	}
	this=g_new(struct vehicle,1);
#ifdef HAVE_LIBGPS
	this->gps=gps;
#endif
	this->iochan=g_io_channel_unix_new(fd);
	g_io_channel_set_encoding(this->iochan, NULL, &error);
	g_io_add_watch(this->iochan, G_IO_IN|G_IO_ERR|G_IO_HUP, vehicle_track, this);
#if 0
	g_timeout_add(100, vehicle_timer, this);
#endif
	this->current_pos.x=0x130000;
	this->current_pos.y=0x600000;
	
	return this;
}

void
vehicle_callback(struct vehicle *this, void (*func)(void *data), void *data)
{
	this->callback_func=func;
	this->callback_data=data;
}

void
vehicle_destroy(struct vehicle *this)
{
	GError *error=NULL;


	g_io_channel_shutdown(this->iochan,0,&error);
#ifdef HAVE_LIBGPS
	if (this->gps)
		gps_close(this->gps);
#endif
	g_free(this);
}
