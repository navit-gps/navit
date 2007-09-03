#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <glib.h>
#ifdef HAVE_LIBGPS
#include <gps.h>
#endif
#include "debug.h"
#include "coord.h"
#include "callback.h"
#include "transform.h"
#include "projection.h"
#include "statusbar.h"
#include "vehicle.h"

int vfd;

static void disable_watch(struct vehicle *this);
static void enable_watch(struct vehicle *this);

 /* #define INTERPOLATION_TIME 50 */

struct callback {
	void (*func)(struct vehicle *, void *data);
	void *data;
};

struct vehicle {
	char *url;
	GIOChannel *iochan;
	guint watch;
	int is_file;
	int is_pipe;
	int timer_count;
	int qual;
	int sats;
	struct coord_geo geo;
	double height;
	double dir,speed;
	double time;
	struct coord current_pos;
	struct coord_d curr;
	struct coord_d delta;

	double speed_last;
	int fd;
	FILE *file;
#ifdef HAVE_LIBGPS
	struct gps_data_t *gps;
#endif
#define BUFFER_SIZE 256
	char buffer[BUFFER_SIZE];
	int buffer_pos;

	struct callback_list *cbl;
	struct vehicle *child;
	struct callback *child_cb;

	int magic;
	int is_udp;
	int interval;
	struct sockaddr_in rem;
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
				(*this->callback_func)(this, this->callback_data);
		}
/*	} */
	return TRUE; 
}
#endif

enum projection
vehicle_projection(struct vehicle *this)
{
	return projection_mg;
}

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
vehicle_height_get(struct vehicle *this)
{
	return &this->height;
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
	callback_list_call_1(this->cbl, this);
}

static int
enable_watch_timer(gpointer t)
{
	struct vehicle *this=t;
	enable_watch(this);
	
	return FALSE;
}

static void
vehicle_parse_gps(struct vehicle *this, char *buffer)
{
	char *p,*item[16];
	double lat,lng,scale,speed;
	int i,bcsum;
	int len=strlen(buffer);
	unsigned char csum=0;

	dbg(1, "buffer='%s' ", buffer);
	write(vfd, buffer, len);
	write(vfd, "\n", 1);
	for (;;) {
		if (len < 4) {
			dbg(0, "too short\n");
			return;
		}
		if (buffer[len-1] == '\r' || buffer[len-1] == '\n')
			buffer[--len]='\0';
		else
			break;
	}
	if (buffer[0] != '$') {
		dbg(0, "no leading $\n");
		return;
	}
	if (buffer[len-3] != '*') {
		dbg(0, "no *XX\n");
		return;
	}
	for (i = 1 ; i < len-3 ; i++) {
		csum ^= (unsigned char)(buffer[i]);
	}
	if (!sscanf(buffer+len-2, "%x", &bcsum)) {
		dbg(0, "no checksum\n");
		return;
	}
	if (bcsum != csum) {
		dbg(0, "wrong checksum\n");
		return;
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
		this->geo.lat=floor(lat/100);
		lat-=this->geo.lat*100;
		this->geo.lat+=lat/60;

		sscanf(item[4],"%lf",&lng);
		this->geo.lng=floor(lng/100);
		lng-=this->geo.lng*100;
		this->geo.lng+=lng/60;

		sscanf(item[6],"%d",&this->qual);
		sscanf(item[7],"%d",&this->sats);
		sscanf(item[9],"%lf",&this->height);
		
		transform_from_geo(projection_mg, &this->geo, &this->current_pos);
			
		this->curr.x=this->current_pos.x;
		this->curr.y=this->current_pos.y;
		this->timer_count=0;
		callback_list_call_1(this->cbl, this);
		if (this->is_file) {
			disable_watch(this);
			g_timeout_add(1000, enable_watch_timer, this);
		}
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
	if (!strncmp(buffer,"$GPRMC",6)) {
		/* $GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A */
		/* Time,Active/Void,lat,N/S,long,W/E,speed in knots,track angle,date,magnetic variation */
		i=0;
		p=buffer;
		while (i < 16) {
			item[i++]=p;
			while (*p && *p != ',') 
				p++;	
			if (! *p) break;
			*p++='\0';
		}
		sscanf(item[8],"%lf",&this->dir);
		sscanf(item[7],"%lf",&this->speed);
		this->speed *= 1.852;
		scale=transform_scale(this->current_pos.y);
		speed=this->speed+(this->speed-this->speed_last)/2;
#ifdef INTERPOLATION_TIME
		this->delta.x=sin(M_PI*this->dir/180)*speed*scale/3600*INTERPOLATION_TIME;
		this->delta.y=cos(M_PI*this->dir/180)*speed*scale/3600*INTERPOLATION_TIME;
#endif
		this->speed_last=this->speed;
	}
}

#ifdef HAVE_LIBGPS
static void
vehicle_gps_callback(struct gps_data_t *data, char *buf, size_t len, int level)
{
	// If data->fix.speed is NAN, then the drawing gets jumpy. 
	if(isnan(data->fix.speed)){
		return;
	}

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
		this->geo.lat=data->fix.latitude;
		this->geo.lng=data->fix.longitude;
		transform_from_geo(projection_mg, &this->geo, &this->current_pos);
		this->curr.x=this->current_pos.x;
		this->curr.y=this->current_pos.y;
		this->timer_count=0;
		callback_list_call_1(this->cbl, this);
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


static void
vehicle_close(struct vehicle *this)
{
	GError *error=NULL;


	g_io_channel_shutdown(this->iochan,0,&error);
#ifdef HAVE_LIBGPS
	if (this->gps)
		gps_close(this->gps);
#endif
	if (this->file)
		pclose(this->file);
	if (this->fd != -1)
		close(this->fd);
}

struct packet {
	int magic __attribute__ ((packed));
	unsigned char type;
	union {
		struct {
			int x __attribute__ ((packed));
			int y __attribute__ ((packed));
			unsigned char speed;
			unsigned char dir;
		} pos;
	} u;
} __attribute__ ((packed)) ;

static void
vehicle_udp_recv(struct vehicle *this)
{
	struct packet pkt;
	int size;

	dbg(2,"enter this=%p\n",this);
	size=recv(this->fd, &pkt, 15, 0);
	if (pkt.magic == this->magic) {
		dbg(3,"magic 0x%x size=%d\n", pkt.magic, size);
		this->current_pos.x=pkt.u.pos.x;
		this->current_pos.y=pkt.u.pos.y;
		this->speed=pkt.u.pos.speed;
		this->dir=pkt.u.pos.dir*2;
		callback_list_call_1(this->cbl, this);
	}
}

static void
vehicle_udp_update(struct vehicle *this, struct vehicle *child)
{
	struct coord *pos=&child->current_pos;
	struct packet pkt;
	int speed=child->speed;
	int dir=child->dir/2;
	if (speed > 255)
		speed=255;
	pkt.magic=this->magic;
	pkt.type=1;
	pkt.u.pos.x=pos->x;
	pkt.u.pos.y=pos->y;
	pkt.u.pos.speed=speed;
	pkt.u.pos.dir=dir;
	sendto(this->fd, &pkt, 15, 0, (struct sockaddr *)&this->rem, sizeof(this->rem));
	this->current_pos=child->current_pos;
	this->speed=child->speed;
	this->dir=child->dir;
	callback_list_call_1(this->cbl, this);
}

static int
vehicle_udp_query(void *data)
{
	struct vehicle *this=(struct vehicle *)data;
	struct packet pkt;
	dbg(2,"enter this=%p\n", this);
	pkt.magic=this->magic;
	pkt.type=2;
	sendto(this->fd, &pkt, 5, 0, (struct sockaddr *)&this->rem, sizeof(this->rem));
	dbg(2,"ret=TRUE\n");
	return TRUE;
}

static int
vehicle_udp_open(struct vehicle *this)
{
	char *host,*child,*url,*colon;
	int port;
	url=g_strdup(this->url);
	colon=index(url+6,':');
	struct sockaddr_in lcl;

	if (! colon || sscanf(colon+1,"%d/%i/%d", &port, &this->magic, &this->interval) != 3) {
		g_warning("Wrong syntax in %s\n", this->url);
		return 0;
	}
	host=url+6;
	*colon='\0';
	this->fd=socket(PF_INET, SOCK_DGRAM, 0);
	this->is_udp=1;
	memset(&lcl, 0, sizeof(lcl));
	lcl.sin_family = AF_INET;

	this->rem.sin_family = AF_INET;
	inet_aton(host, &this->rem.sin_addr);
	this->rem.sin_port=htons(port);

	bind(this->fd, (struct sockaddr *)&lcl, sizeof(lcl));
	child=index(colon+1,' ');
	if (child) {
		child++;
		if (!this->child) {
			dbg(3,"child=%s\n", child);
			this->child=vehicle_new(child);
			this->child_cb=callback_new_1(callback_cast(vehicle_udp_update), this);
			vehicle_callback_add(this->child, this->child_cb);
		}
	} else {
		vehicle_udp_query(this);
		g_timeout_add(this->interval*1000, vehicle_udp_query, this);
	}
	g_free(url);
	return 0;
}

static int
vehicle_open(struct vehicle *this)
{
	struct termios tio;
	struct stat st;
	int fd=0;

#ifdef HAVE_LIBGPS
	struct gps_data_t *gps=NULL;
	char *url_,*colon;
#endif
	if (! strncmp(this->url,"file:",5)) {
		fd=open(this->url+5,O_RDONLY|O_NDELAY);
		if (fd < 0) {
			g_warning("Failed to open %s", this->url);
			return 0;
		}
		stat(this->url+5, &st);
		if (S_ISREG (st.st_mode)) {
			this->is_file=1;
		} else {
			tcgetattr(fd, &tio);
			cfmakeraw(&tio);
			cfsetispeed(&tio, B4800);
			cfsetospeed(&tio, B4800);
			tio.c_cc[VMIN]=16;
			tio.c_cc[VTIME]=1;
			tcsetattr(fd, TCSANOW, &tio);
		}
		this->fd=fd;
	} else if (! strncmp(this->url,"pipe:",5)) {
		this->file=popen(this->url+5, "r");
		this->is_pipe=1;
		if (! this->file) {
			g_warning("Failed to open %s", this->url);
			return 0;
		}
		fd=fileno(this->file);
	} else if (! strncmp(this->url,"gpsd://",7)) {
#ifdef HAVE_LIBGPS
		url_=g_strdup(this->url);
		colon=index(url_+7,':');
		if (colon) {
			*colon=0;
			gps=gps_open(url_+7,colon+1);
		} else
			gps=gps_open(this->url+7,NULL);
		g_free(url_);
		if (! gps) {
			g_warning("Failed to connect to %s", this->url);
			return 0;
		}
		gps_query(gps, "w+x\n");
		gps_set_raw_hook(gps, vehicle_gps_callback);
		fd=gps->gps_fd;
		this->gps=gps;
#else
		g_warning("No support for gpsd compiled in\n");
		return 0;
#endif
	} else if (! strncmp(this->url,"udp://",6)) {
		vehicle_udp_open(this);
		fd=this->fd;
	}
	this->iochan=g_io_channel_unix_new(fd);
	enable_watch(this);
	return 1;
}


static gboolean
vehicle_track(GIOChannel *iochan, GIOCondition condition, gpointer t)
{
	struct vehicle *this=t;
	char *str,*tok;
	gsize size;

	dbg(1,"enter condition=%d\n", condition);
	if (condition == G_IO_IN) {
#ifdef HAVE_LIBGPS
		if (this->gps) {
			vehicle_last=this;
			gps_poll(this->gps);
		} else {
#else
		{
#endif
			if (this->is_udp) {
				vehicle_udp_recv(this);
				return TRUE;
			}
			size=read(g_io_channel_unix_get_fd(iochan), this->buffer+this->buffer_pos, BUFFER_SIZE-this->buffer_pos-1);
			if (size <= 0) {
				vehicle_close(this);
				vehicle_open(this);
				return TRUE;
			}
			this->buffer_pos+=size;
			this->buffer[this->buffer_pos]='\0';
			dbg(1,"size=%d pos=%d buffer='%s'\n", size, this->buffer_pos, this->buffer);
			str=this->buffer;
			while ((tok=index(str, '\n'))) {
				*tok++='\0';
				dbg(1,"line='%s'\n", str);
				vehicle_parse_gps(this, str);
				str=tok;
			}
			if (str != this->buffer) {
				size=this->buffer+this->buffer_pos-str;
				memmove(this->buffer, str, size+1);
				this->buffer_pos=size;
				dbg(1,"now pos=%d buffer='%s'\n", this->buffer_pos, this->buffer);
			} else if (this->buffer_pos == BUFFER_SIZE-1) {
				dbg(0,"overflow\n");
				this->buffer_pos=0;
			}
			
		}

		return TRUE;
	} 
	return FALSE;
}

static void
enable_watch(struct vehicle *this)
{
	this->watch=g_io_add_watch(this->iochan, G_IO_IN|G_IO_ERR|G_IO_HUP, vehicle_track, this);
}

static void
disable_watch(struct vehicle *this)
{
	g_source_remove(this->watch);
}

struct vehicle *
vehicle_new(const char *url)
{
	struct vehicle *this;
	this=g_new0(struct vehicle,1);

	this->cbl=callback_list_new();
	this->url=g_strdup(url);
	this->fd=-1;

	if (! vfd) {
		vfd=open("vlog.txt", O_RDWR|O_APPEND|O_CREAT, 0644);
	}
	vehicle_open(this);
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

void
vehicle_callback_add(struct vehicle *this, struct callback *cb)
{
	callback_list_add(this->cbl, cb);
}

void
vehicle_callback_remove(struct vehicle *this, struct callback *cb)
{
	callback_list_remove(this->cbl, cb);
}


void
vehicle_destroy(struct vehicle *this)
{
	vehicle_close(this);
	callback_list_destroy(this->cbl);
	g_free(this->url);
	g_free(this);
}
