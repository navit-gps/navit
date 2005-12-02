#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <glib.h>
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
};

void (*callback_func)();
void *callback_data;

int
vehicle_timer(gpointer t)
{
	struct vehicle *this=t;	
	if (this->timer_count++ < 10) {
		this->curr.x+=this->delta.x;
		this->curr.y+=this->delta.y;	
		this->current_pos.x=this->curr.x;
		this->current_pos.y=this->curr.y;
		if (callback_func)
			(*callback_func)(callback_data);
	}
	return TRUE;
}

struct coord *
vehicle_pos_get(void *t)
{
	struct vehicle *this=t;

	return &this->current_pos;
}

double *
vehicle_speed_get(void *t)
{
	struct vehicle *this=t;

	return &this->speed;
}

double *
vehicle_dir_get(void *t)
{
	struct vehicle *this=t;

	return &this->dir;
}

void
vehicle_set_position(void *t, struct coord *pos)
{
	struct vehicle *this=t;

	this->current_pos=*pos;
	this->curr.x=this->current_pos.x;
	this->curr.y=this->current_pos.y;
	this->delta.x=0;
	this->delta.y=0;
	if (callback_func)
		(*callback_func)(callback_data);
}

void
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
		if (callback_func)
			(*callback_func)(callback_data);
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

gboolean
vehicle_track(GIOChannel *iochan, GIOCondition condition, gpointer t)
{
	struct vehicle *this=t;
	GError *error=NULL;
	char buffer[4096];
	char *str,*tok;
	int size;
	
	if (condition == G_IO_IN) {
		g_io_channel_read_chars(iochan, buffer, 4096, &size, &error);
		buffer[size]='\0';
		str=buffer;
		while ((tok=strtok(str, "\n"))) {
			str=NULL;
			vehicle_parse_gps(this, tok);
		}
		return TRUE;
	} 
	return FALSE;
}

void *
vehicle_new(char *file)
{
	struct vehicle *this;
	GError *error=NULL;
	int fd;

	this=g_new(struct vehicle,1);
	fd=open(file,O_RDONLY|O_NDELAY);
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
vehicle_callback(void (*func)(), void *data)
{
	callback_func=func;
	callback_data=data;
}

int
vehicle_destroy(void *t)
{
	struct vehicle *this=t;
	GError *error=NULL;


	g_io_channel_shutdown(this->iochan,0,&error);
	g_free(this);

	return 0;
}
