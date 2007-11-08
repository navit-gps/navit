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
#include "navit.h"
#include "item.h"
#include "log.h"
#include "vehicle.h"
#include "item.h"
#include "route.h"

static void disable_watch(struct vehicle *this_);
static void enable_watch(struct vehicle *this_);

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
	int status; // Do we have a fix? 0=no 1=yes, without DGPS 2=yes, with DGPS
	int sats,sats_used;
	struct coord_geo geo;
	double height;
	double dir,speed;
	double time;
	double pdop;
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
	struct log *nmea_log, *gpx_log, *textfile_log;

	struct navit *navit;
};

// FIXME : this_ is an ugly hack (dixit cp15 ;) )
struct vehicle *vehicle_last;

#if INTERPOLATION_TIME
static int
vehicle_timer(gpointer t)
{
	struct vehicle *this_=t;	
/*	if (this_->timer_count++ < 1000/INTERPOLATION_TIME) { */
		if (this_->delta.x || this_->delta.y) {
			this_->curr.x+=this_->delta.x;
			this_->curr.y+=this_->delta.y;	
			this_->current_pos.x=this_->curr.x;
			this_->current_pos.y=this_->curr.y;
			if (this_->callback_func)
				(*this_->callback_func)(this_, this_->callback_data);
		}
/*	} */
	return TRUE; 
}
#endif

enum projection
vehicle_projection(struct vehicle *this_)
{
	return projection_mg;
}

struct coord *
vehicle_pos_get(struct vehicle *this_)
{
	return &this_->current_pos;
}

double *
vehicle_speed_get(struct vehicle *this_)
{
	return &this_->speed;
}

double *
vehicle_height_get(struct vehicle *this_)
{
	return &this_->height;
}
double *
vehicle_dir_get(struct vehicle *this_)
{
	return &this_->dir;
}

int *
vehicle_status_get(struct vehicle *this_)
{
	return &this_->status;
}

int *
vehicle_sats_get(struct vehicle *this_)
{
	return &this_->sats;
}

int *
vehicle_sats_used_get(struct vehicle *this_)
{
	return &this_->sats_used;
}

double *
vehicle_pdop_get(struct vehicle *this_)
{
	return &this_->pdop;
}

void
vehicle_set_position(struct vehicle *this_, struct coord *pos)
{
	this_->current_pos=*pos;
	this_->curr.x=this_->current_pos.x;
	this_->curr.y=this_->current_pos.y;
	this_->delta.x=0;
	this_->delta.y=0;
	callback_list_call_1(this_->cbl, this_);
}

static int
enable_watch_timer(gpointer t)
{
	struct vehicle *this_=t;
	enable_watch(this_);
	
	return FALSE;
}

// FIXME Should this_ function be static ?
void
vehicle_set_navit(struct vehicle *this_,struct navit *nav) {
	dbg(0,"vehicle_set_navit called\n");
	this_->navit=nav;
}

static void
vehicle_parse_gps(struct vehicle *this_, char *buffer)
{
	char *p,*item[16];
	double lat,lng,scale,speed;
	int i,bcsum;
	int len=strlen(buffer);
	unsigned char csum=0;

	dbg(1, "buffer='%s' ", buffer);
	if (this_->nmea_log) {
		log_write(this_->nmea_log, buffer, len);
		log_write(this_->nmea_log, "\n", 1);
	}
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
		this_->geo.lat=floor(lat/100);
		lat-=this_->geo.lat*100;
		this_->geo.lat+=lat/60;

		sscanf(item[4],"%lf",&lng);
		this_->geo.lng=floor(lng/100);
		lng-=this_->geo.lng*100;
		this_->geo.lng+=lng/60;

		sscanf(item[6],"%d",&this_->status);
		sscanf(item[7],"%d",&this_->sats);
		sscanf(item[9],"%lf",&this_->height);
	
		if (this_->gpx_log) {
			char buffer[256];
			sprintf(buffer,"<trkpt lat=\"%f\" lon=\"%f\" />\n",this_->geo.lat,this_->geo.lng);
			log_write(this_->gpx_log, buffer, strlen(buffer));
			
		}
		if (this_->textfile_log) {
			char buffer[256];
			sprintf(buffer,"%f %f type=trackpoint\n",this_->geo.lng,this_->geo.lat);
			log_write(this_->textfile_log, buffer, strlen(buffer));
		}
		transform_from_geo(projection_mg, &this_->geo, &this_->current_pos);
			
		this_->curr.x=this_->current_pos.x;
		this_->curr.y=this_->current_pos.y;
		this_->timer_count=0;
		callback_list_call_1(this_->cbl, this_);
		if (this_->is_file) {
			disable_watch(this_);
			g_timeout_add(1000, enable_watch_timer, this_);
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
		sscanf(item[1],"%lf",&this_->dir);
		sscanf(item[7],"%lf",&this_->speed);

		scale=transform_scale(this_->current_pos.y);
		speed=this_->speed+(this_->speed-this_->speed_last)/2;
#ifdef INTERPOLATION_TIME
		this_->delta.x=sin(M_PI*this_->dir/180)*speed*scale/3600*INTERPOLATION_TIME;
		this_->delta.y=cos(M_PI*this_->dir/180)*speed*scale/3600*INTERPOLATION_TIME;
#endif
		this_->speed_last=this_->speed;
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
		sscanf(item[8],"%lf",&this_->dir);
		sscanf(item[7],"%lf",&this_->speed);
		this_->speed *= 1.852;
		scale=transform_scale(this_->current_pos.y);
		speed=this_->speed+(this_->speed-this_->speed_last)/2;
#ifdef INTERPOLATION_TIME
		this_->delta.x=sin(M_PI*this_->dir/180)*speed*scale/3600*INTERPOLATION_TIME;
		this_->delta.y=cos(M_PI*this_->dir/180)*speed*scale/3600*INTERPOLATION_TIME;
#endif
		this_->speed_last=this_->speed;
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

	struct vehicle *this_=vehicle_last;
	double scale,speed;
#if INTERPOLATION_TIME
	if (! (data->set & TIME_SET)) {
		return;
	}
	data->set &= ~TIME_SET;
	if (this_->time == data->fix.time)
		return;
	this_->time=data->fix.time;
#endif
	if (data->set & SPEED_SET) {
		this_->speed_last=this_->speed;
		this_->speed=data->fix.speed*3.6;
		data->set &= ~SPEED_SET;
	}
	if (data->set & TRACK_SET) {
		speed=this_->speed+(this_->speed-this_->speed_last)/2;
		this_->dir=data->fix.track;
		scale=transform_scale(this_->current_pos.y);
#ifdef INTERPOLATION_TIME
		this_->delta.x=sin(M_PI*this_->dir/180)*speed*scale/3600*INTERPOLATION_TIME;
		this_->delta.y=cos(M_PI*this_->dir/180)*speed*scale/3600*INTERPOLATION_TIME;
#endif
		data->set &= ~TRACK_SET;
	}
	if (data->set & LATLON_SET) {
		this_->geo.lat=data->fix.latitude;
		this_->geo.lng=data->fix.longitude;
		transform_from_geo(projection_mg, &this_->geo, &this_->current_pos);
		this_->curr.x=this_->current_pos.x;
		this_->curr.y=this_->current_pos.y;
		this_->timer_count=0;
		callback_list_call_1(this_->cbl, this_);
		data->set &= ~LATLON_SET;
	}
	if (data->set & ALTITUDE_SET) {
		this_->height=data->fix.altitude;
		data->set &= ~ALTITUDE_SET;
	}
	if (data->set & SATELLITE_SET) {
		this_->sats=data->satellites;
		data->set &= ~SATELLITE_SET;
		/* FIXME : the USED_SET check does not work yet. */
		this_->sats_used=data->satellites_used;
	}
	if (data->set & STATUS_SET) {
		this_->status=data->status;
		data->set &= ~STATUS_SET;
	}
	if(data->set & PDOP_SET){
		printf("pdop : %d\n",data->pdop);
	}
}
#endif


static void
vehicle_close(struct vehicle *this_)
{
	GError *error=NULL;


	g_io_channel_shutdown(this_->iochan,0,&error);
#ifdef HAVE_LIBGPS
	if (this_->gps)
		gps_close(this_->gps);
#endif
	if (this_->file)
		pclose(this_->file);
	if (this_->fd != -1)
		close(this_->fd);
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
vehicle_udp_recv(struct vehicle *this_)
{
	struct packet pkt;
	int size;

	dbg(2,"enter this_=%p\n",this_);
	size=recv(this_->fd, &pkt, 15, 0);
	if (pkt.magic == this_->magic) {
		dbg(3,"magic 0x%x size=%d\n", pkt.magic, size);
		this_->current_pos.x=pkt.u.pos.x;
		this_->current_pos.y=pkt.u.pos.y;
		this_->speed=pkt.u.pos.speed;
		this_->dir=pkt.u.pos.dir*2;
		callback_list_call_1(this_->cbl, this_);
	}
}

static void
vehicle_udp_update(struct vehicle *this_, struct vehicle *child)
{
	struct coord *pos=&child->current_pos;
	struct packet pkt;
	int speed=child->speed;
	int dir=child->dir/2;
	if (speed > 255)
		speed=255;
	pkt.magic=this_->magic;
	pkt.type=1;
	pkt.u.pos.x=pos->x;
	pkt.u.pos.y=pos->y;
	pkt.u.pos.speed=speed;
	pkt.u.pos.dir=dir;
	sendto(this_->fd, &pkt, 15, 0, (struct sockaddr *)&this_->rem, sizeof(this_->rem));
	this_->current_pos=child->current_pos;
	this_->speed=child->speed;
	this_->dir=child->dir;
	callback_list_call_1(this_->cbl, this_);
}

static int
vehicle_udp_query(void *data)
{
	struct vehicle *this_=(struct vehicle *)data;
	struct packet pkt;
	dbg(2,"enter this_=%p\n", this_);
	pkt.magic=this_->magic;
	pkt.type=2;
	sendto(this_->fd, &pkt, 5, 0, (struct sockaddr *)&this_->rem, sizeof(this_->rem));
	dbg(2,"ret=TRUE\n");
	return TRUE;
}

static int
vehicle_udp_open(struct vehicle *this_)
{
	char *host,*child,*url,*colon;
	int port;
	url=g_strdup(this_->url);
	colon=index(url+6,':');
	struct sockaddr_in lcl;

	if (! colon || sscanf(colon+1,"%d/%i/%d", &port, &this_->magic, &this_->interval) != 3) {
		g_warning("Wrong syntax in %s\n", this_->url);
		return 0;
	}
	host=url+6;
	*colon='\0';
	this_->fd=socket(PF_INET, SOCK_DGRAM, 0);
	this_->is_udp=1;
	memset(&lcl, 0, sizeof(lcl));
	lcl.sin_family = AF_INET;

	this_->rem.sin_family = AF_INET;
	inet_aton(host, &this_->rem.sin_addr);
	this_->rem.sin_port=htons(port);

	bind(this_->fd, (struct sockaddr *)&lcl, sizeof(lcl));
	child=index(colon+1,' ');
	if (child) {
		child++;
		if (!this_->child) {
			dbg(3,"child=%s\n", child);
			this_->child=vehicle_new(child);
			this_->child_cb=callback_new_1(callback_cast(vehicle_udp_update), this_);
			vehicle_callback_add(this_->child, this_->child_cb);
		}
	} else {
		vehicle_udp_query(this_);
		g_timeout_add(this_->interval*1000, vehicle_udp_query, this_);
	}
	g_free(url);
	return 0;
}

static int
vehicle_demo_timer (struct vehicle *this)
{
	struct route_path_coord_handle *h;
	struct coord *c;
 	dbg(1,"###### Entering simulation loop\n");
	if(!this->navit){
		dbg(1,"vehicle->navit is not set. Can't simulate\n");
		return 1;
	}

	// <cp15> Then check whether the route is set, if not return TRUE
	struct route * vehicle_route=navit_get_route(this->navit);
	if(!vehicle_route){
		dbg(1,"navit_get_route NOK\n");
		return 1;
	}

	h=route_path_coord_open(vehicle_route);
	if (!h) {
		dbg(1,"navit_path_coord_open NOK\n");
		return 1;
	}
	c=route_path_coord_get(h);
	dbg(1,"current pos=%p\n", c);
	if (c) 
		dbg(1,"current pos=0x%x,0x%x\n", c->x, c->y);
	c=route_path_coord_get(h);
	dbg(1,"next pos=%p\n", c);
	if (c) {
		dbg(1,"next pos=0x%x,0x%x\n", c->x, c->y);
		vehicle_set_position(this,c);
	}
	return 1;
}

static int
vehicle_open(struct vehicle *this_)
{
	struct termios tio;
	struct stat st;
	int fd=0;

#ifdef HAVE_LIBGPS
	struct gps_data_t *gps=NULL;
	char *url_,*colon;
#endif
	if (! strncmp(this_->url,"file:",5)) {
		fd=open(this_->url+5,O_RDONLY|O_NDELAY);
		if (fd < 0) {
			g_warning("Failed to open %s", this_->url);
			return 0;
		}
		stat(this_->url+5, &st);
		if (S_ISREG (st.st_mode)) {
			this_->is_file=1;
		} else {
			tcgetattr(fd, &tio);
			cfmakeraw(&tio);
			cfsetispeed(&tio, B4800);
			cfsetospeed(&tio, B4800);
			tio.c_cc[VMIN]=16;
			tio.c_cc[VTIME]=1;
			tcsetattr(fd, TCSANOW, &tio);
		}
		this_->fd=fd;
	} else if (! strncmp(this_->url,"pipe:",5)) {
		this_->file=popen(this_->url+5, "r");
		this_->is_pipe=1;
		if (! this_->file) {
			g_warning("Failed to open %s", this_->url);
			return 0;
		}
		fd=fileno(this_->file);
	} else if (! strncmp(this_->url,"gpsd://",7)) {
#ifdef HAVE_LIBGPS
		url_=g_strdup(this_->url);
		colon=index(url_+7,':');
		if (colon) {
			*colon=0;
			gps=gps_open(url_+7,colon+1);
		} else
			gps=gps_open(this_->url+7,NULL);
		g_free(url_);
		if (! gps) {
			g_warning("Failed to connect to %s", this_->url);
			return 0;
		}
		gps_query(gps, "w+x\n");
		gps_set_raw_hook(gps, vehicle_gps_callback);
		fd=gps->gps_fd;
		this_->gps=gps;
#else
		g_warning("No support for gpsd compiled in\n");
		return 0;
#endif
	} else if (! strncmp(this_->url,"udp://",6)) {
		vehicle_udp_open(this_);
		fd=this_->fd;
	} else if (! strncmp(this_->url,"demo://",7)) {
		dbg(0,"Creating a demo vehicle\n");
		g_timeout_add(1000, vehicle_demo_timer, this_);
	}
	this_->iochan=g_io_channel_unix_new(fd);
	enable_watch(this_);
	return 1;
}


static gboolean
vehicle_track(GIOChannel *iochan, GIOCondition condition, gpointer t)
{
	struct vehicle *this_=t;
	char *str,*tok;
	gsize size;

	dbg(1,"enter condition=%d\n", condition);
	if (condition == G_IO_IN) {
#ifdef HAVE_LIBGPS
		if (this_->gps) {
			vehicle_last=this_;
			gps_poll(this_->gps);
		} else {
#else
		{
#endif
			if (this_->is_udp) {
				vehicle_udp_recv(this_);
				return TRUE;
			}
			size=read(g_io_channel_unix_get_fd(iochan), this_->buffer+this_->buffer_pos, BUFFER_SIZE-this_->buffer_pos-1);
			if (size <= 0) {
				vehicle_close(this_);
				vehicle_open(this_);
				return TRUE;
			}
			this_->buffer_pos+=size;
			this_->buffer[this_->buffer_pos]='\0';
			dbg(1,"size=%d pos=%d buffer='%s'\n", size, this_->buffer_pos, this_->buffer);
			str=this_->buffer;
			while ((tok=index(str, '\n'))) {
				*tok++='\0';
				dbg(1,"line='%s'\n", str);
				vehicle_parse_gps(this_, str);
				str=tok;
			}
			if (str != this_->buffer) {
				size=this_->buffer+this_->buffer_pos-str;
				memmove(this_->buffer, str, size+1);
				this_->buffer_pos=size;
				dbg(1,"now pos=%d buffer='%s'\n", this_->buffer_pos, this_->buffer);
			} else if (this_->buffer_pos == BUFFER_SIZE-1) {
				dbg(0,"overflow\n");
				this_->buffer_pos=0;
			}
			
		}

		return TRUE;
	} 
	return FALSE;
}

static void
enable_watch(struct vehicle *this_)
{
	this_->watch=g_io_add_watch(this_->iochan, G_IO_IN|G_IO_ERR|G_IO_HUP, vehicle_track, this_);
}

static void
disable_watch(struct vehicle *this_)
{
	g_source_remove(this_->watch);
}

struct vehicle *
vehicle_new(const char *url)
{
	struct vehicle *this_;
	this_=g_new0(struct vehicle,1);

	this_->cbl=callback_list_new();
	this_->url=g_strdup(url);
	this_->fd=-1;

	vehicle_open(this_);
	this_->current_pos.x=0x130000;
	this_->current_pos.y=0x600000;
	this_->curr.x=this_->current_pos.x;
	this_->curr.y=this_->current_pos.y;
	this_->delta.x=0;
	this_->delta.y=0;
#if INTERPOLATION_TIME
	g_timeout_add(INTERPOLATION_TIME, vehicle_timer, this_);
#endif
	
	return this_;
}

void
vehicle_callback_add(struct vehicle *this_, struct callback *cb)
{
	callback_list_add(this_->cbl, cb);
}

void
vehicle_callback_remove(struct vehicle *this_, struct callback *cb)
{
	callback_list_remove(this_->cbl, cb);
}


int
vehicle_add_log(struct vehicle *this_, struct log *log, struct attr **attrs)
{
	struct attr *type;
	type=attr_search(attrs, NULL, attr_type);
	if (! type) 
		return 1;
	if (!strcmp(type->u.str,"nmea")) {
		this_->nmea_log=log;
		if (this_->child) 
			this_->child->nmea_log=log;
		
	} else if (!strcmp(type->u.str,"gpx")) {
		char *header="<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<gpx version=\"1.0\" creator=\"Navit http://navit.sourceforge.net\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.topografix.com/GPX/1/0\" xsi:schemaLocation=\"http://www.topografix.com/GPX/1/0 http://www.topografix.com/GPX/1/0/gpx.xsd\">\n<trk>\n<trkseg>\n";
		char *trailer="</trkseg>\n</trk>\n</gpx>\n";
		this_->gpx_log=log;
		if (this_->child)
			this_->child->gpx_log=log;
		log_set_header(log,header,strlen(header));
		log_set_trailer(log,trailer,strlen(trailer));
	} else if (!strcmp(type->u.str,"textfile")) {
		char *header="type=track\n";
		this_->textfile_log=log;
		if (this_->child)
			this_->child->textfile_log=log;
		log_set_header(log,header,strlen(header));
	} else
		return 1;
	return 0;
}

void
vehicle_destroy(struct vehicle *this_)
{
	vehicle_close(this_);
	callback_list_destroy(this_->cbl);
	g_free(this_->url);
	g_free(this_);
}
