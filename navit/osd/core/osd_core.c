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

#ifdef _MSC_VER
#define _USE_MATH_DEFINES 1
#endif /* _MSC_VER */
#include <math.h>
#include <stdio.h>
#include <glib.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "item.h"
#include "point.h"
#include "coord.h"
#include "graphics.h"
#include "transform.h"
#include "route.h"
#include "navit.h"
#include "plugin.h"
#include "debug.h"
#include "callback.h"
#include "color.h"
#include "vehicle.h"
#include "navigation.h"
#include "track.h"
#include "map.h"
#include "file.h"
#include "attr.h"
#include "command.h"
#include "navit_nls.h"
#include "messages.h"
#include "vehicleprofile.h"
#include "roadprofile.h"
#include "osd.h"
#include "speech.h"
#include "event.h"
#include "mapset.h"

#ifdef _MSC_VER
static double round(double x)
{
   if (x >= 0.0)
      return floor(x + 0.5);
   else
      return ceil(x - 0.5);
}
#endif /* MSC_VER */

struct odometer;

static void osd_odometer_reset(struct odometer *this);
static void osd_cmd_odometer_reset(struct navit *this, char *function, struct attr **in, struct attr ***out, int *valid);
static void osd_odometer_draw(struct odometer *this, struct navit *nav, struct vehicle *v);

struct compass {
	struct osd_item osd_item;
	int width;
	struct graphics_gc *green;
};

static void
transform_rotate(struct point *center, int angle, struct point *p,
		 int count)
{
	int i, x, y;
	double dx, dy;
	for (i = 0; i < count; i++) {
		dx = sin(M_PI * angle / 180.0);
		dy = cos(M_PI * angle / 180.0);
		x = dy * p->x - dx * p->y;
		y = dx * p->x + dy * p->y;

		p->x = center->x + x;
		p->y = center->y + y;
		p++;
	}
}

static void
handle(struct graphics *gr, struct graphics_gc *gc, struct point *p, int r,
       int dir)
{
	struct point ph[3];
	int l = r * 0.4;

	ph[0].x = 0;
	ph[0].y = r;
	ph[1].x = 0;
	ph[1].y = -r;
	transform_rotate(p, dir, ph, 2);
	graphics_draw_lines(gr, gc, ph, 2);
	ph[0].x = -l;
	ph[0].y = -r + l;
	ph[1].x = 0;
	ph[1].y = -r;
	ph[2].x = l;
	ph[2].y = -r + l;
	transform_rotate(p, dir, ph, 3);
	graphics_draw_lines(gr, gc, ph, 3);
}

/**
 * * Format distance, choosing the unit (m or km) and precision depending on distance
 * *
 * * @param distance distance in meters
 * * @param sep separator character to be inserted between distance value and unit
 * * @returns a pointer to a string containing the formatted distance
 * */
static char *
format_distance(double distance, char *sep)
{
	if (distance >= 100000)
		return g_strdup_printf("%.0f%skm", distance / 1000, sep);
	else if (distance >= 10000)
		return g_strdup_printf("%.1f%skm", distance / 1000, sep);
	else if (distance >= 300)
		return g_strdup_printf("%.0f%sm", round(distance / 25) * 25, sep);
	else if (distance >= 50)
		return g_strdup_printf("%.0f%sm", round(distance / 10) * 10, sep);
	else if (distance >= 10)
		return g_strdup_printf("%.0f%sm", distance, sep);
	else
		return g_strdup_printf("%.1f%sm", distance, sep);
}

/**
 * * Format time (duration)
 * *
 * * @param tm pointer to a tm structure specifying the time
 * * @param days days
 * * @returns a pointer to a string containing the formatted time
 * */
static char * 
format_time(struct tm *tm, int days)
{
	if (days)
		return g_strdup_printf("%d+%02d:%02d", days, tm->tm_hour, tm->tm_min);
	else
		return g_strdup_printf("%02d:%02d", tm->tm_hour, tm->tm_min);
}

/**
 * * Format speed in km/h
 * *
 * * @param speed speed in km/h
 * * @param sep separator character to be inserted between speed value and unit
 * * @returns a pointer to a string containing the formatted speed
 * */
static char * 
format_speed(double speed, char *sep, char *format)
{
	if (!format || !strcmp(format,"named"))
		return g_strdup_printf("%.0f%skm/h", speed, sep);
	else if (!strcmp(format,"value") || !strcmp(format,"unit")) {
		if (!strcmp(format,"value"))
			return g_strdup_printf("%.0f", speed);
		else 
			return g_strdup_printf("km/h");
	} 
	return g_strdup_printf("");
}

/*static char *
format_float(double num)
{
	return g_strdup_printf("%f", num);
}*/

static char *
format_float_0(double num)
{
	return g_strdup_printf("%.0f", num);
}


static int odometers_saved = 0;
static GList* odometer_list = NULL;

static struct command_table commands[] = {
	{"odometer_reset",command_cast(osd_cmd_odometer_reset)},
};

struct odometer {
	struct osd_item osd_item;
	int width;
	struct graphics_gc *orange;
	struct graphics_gc *white;
	struct callback *click_cb;
	char *text;                 //text of label attribute for this osd
	char *name;                 //unique name of the odometer (needed for handling multiple odometers persistently)
	struct color idle_color;    //text color when counter is idle

	int bDisableReset;         
	int bAutoStart;         
	int bActive;                //counting or not
	int autosave_period;        //autosave period in seconds
	double sum_dist;            //sum of distance ofprevious intervals in meters
	int sum_time;               //sum of time of previous intervals in seconds (needed for avg spd calculation)
	int time_all;
	time_t last_click_time;     //time of last click (for double click handling)
	time_t last_start_time;     //time of last start of counting
	struct coord last_coord;
};

static void
osd_cmd_odometer_reset(struct navit *this, char *function, struct attr **in, struct attr ***out, int *valid)
{
	if (in && in[0] && ATTR_IS_STRING(in[0]->type) && in[0]->u.str) {
          GList* list = odometer_list;
          while(list) {
            if(!strcmp(((struct odometer*)(list->data))->name,in[0]->u.str)) {
              osd_odometer_reset(list->data);
	      osd_odometer_draw(list->data,this,NULL);
            }
            list = g_list_next(list);
          }
	} 
}

static char* 
str_replace(char*output, char*input, char*pattern, char*replacement)
{
  char *pos;
  char *pos2;
  if (!output || !input || !pattern || !replacement) {
    return NULL;
  }
  if(!strcmp(pattern,"")) {
    return input;
  }

  pos  = &input[0];
  pos2 = &input[0];
  output[0] = 0;
  while ( (pos2=strstr(pos,pattern)) ) {
    strncat(output,pos,pos2-pos);
    strcat(output,replacement);
    pos = pos2 + strlen(pattern);
  }
  strcat(output,pos);
  return NULL;
}

/*
 * save current odometer state to string
 */
static char *osd_odometer_to_string(struct odometer* this_)
{
  return g_strdup_printf("odometer %s %lf %d %d\n",this_->name,this_->sum_dist,this_->time_all,this_->bActive);
}

/*
 * load current odometer state from string
 */
static void osd_odometer_from_string(struct odometer* this_, char*str)
{
  char*  tok;
  char*  name_str;
  char*  sum_dist_str;
  char*  sum_time_str;
  char*  active_str;
  tok = strtok(str, " ");
  if( !tok || strcmp("odometer",tok)) {
    return;
  }
  name_str = g_strdup(strtok(NULL, " "));
  if(!name_str) {
    return;
  }
  sum_dist_str = g_strdup(strtok(NULL, " "));
  if(!sum_dist_str) {
    return;
  }
  sum_time_str = g_strdup(strtok(NULL, " "));
  if(!sum_time_str) {
    return;
  }
  active_str = g_strdup(strtok(NULL, " "));
  if(!active_str) {
    return;
  }
  this_->name = name_str;
  this_->sum_dist = atof(sum_dist_str); 
  this_->sum_time = atoi(sum_time_str); 
  this_->bActive = atoi(active_str); 
  this_->last_coord.x = -1;
}

static void
osd_odometer_draw(struct odometer *this, struct navit *nav,
		 struct vehicle *v)
{
  struct coord curr_coord;
  struct graphics_gc *curr_color;

  char *dist_buffer=0;
  char *spd_buffer=0;
  char *time_buffer = 0;
  struct point p, bbox[4];
  struct attr position_attr,vehicle_attr;
  enum projection pro;
  struct vehicle* curr_vehicle = v;
  double spd = 0;

  int remainder;
  int days;
  int hours;
  int mins;
  int secs;

  char buffer [256+1]="";
  char buffer2[256+1]="";

  if(nav) {
    navit_get_attr(nav, attr_vehicle, &vehicle_attr, NULL);
  }
  if (vehicle_attr.u.vehicle) {
    curr_vehicle = vehicle_attr.u.vehicle;
  }

  if(0==curr_vehicle)
    return;

  osd_std_draw(&this->osd_item);
  if(this->bActive) {
    vehicle_get_attr(curr_vehicle, attr_position_coord_geo,&position_attr, NULL);
    pro = projection_mg;//position_attr.u.pcoord->pro;
    transform_from_geo(pro, position_attr.u.coord_geo, &curr_coord);

    if (this->last_coord.x != -1 ) {
        //we have valid previous position
        double dCurrDist = 0;
        dCurrDist = transform_distance(pro, &curr_coord, &this->last_coord);
	if(0<curr_coord.x && 0<this->last_coord.x) {
	        this->sum_dist += dCurrDist;
	}
        this->time_all = time(0)-this->last_click_time+this->sum_time;
        spd = 3.6*(double)this->sum_dist/(double)this->time_all;
    }
    this->last_coord = curr_coord;
  }

  dist_buffer = format_distance(this->sum_dist,"");
  spd_buffer = format_speed(spd,"","");
  remainder = this->time_all;
  days  = remainder  / (24*60*60);
  remainder = remainder  % (24*60*60);
  hours = remainder  / (60*60);
  remainder = remainder  % (60*60);
  mins  = remainder  / (60);
  remainder = remainder  % (60);
  secs  = remainder;
  if(0<days) {
    time_buffer = g_strdup_printf("%02dd %02d:%02d:%02d",days,hours,mins,secs);
  }
  else {
    time_buffer = g_strdup_printf("%02d:%02d:%02d",hours,mins,secs);
  }

  buffer [0] = 0;
  buffer2[0] = 0;
  if(this->text) {
    str_replace(buffer,this->text,"${avg_spd}",spd_buffer);
    str_replace(buffer2,buffer,"${distance}",dist_buffer);
    str_replace(buffer,buffer2,"${time}",time_buffer);
  }
  g_free(time_buffer);

  graphics_get_text_bbox(this->osd_item.gr, this->osd_item.font, buffer, 0x10000, 0, bbox, 0);
  p.x=(this->osd_item.w-bbox[2].x)/2;
  p.y = this->osd_item.h-this->osd_item.h/10;
  curr_color = this->bActive?this->white:this->orange;
  graphics_draw_text(this->osd_item.gr, curr_color, NULL, this->osd_item.font, buffer, &p, 0x10000, 0);
  g_free(dist_buffer);
  g_free(spd_buffer);
  graphics_draw_mode(this->osd_item.gr, draw_mode_end);
}


static void
osd_odometer_reset(struct odometer *this)
{
  if(!this->bDisableReset) {
    this->bActive = 0;
    this->sum_dist = 0;
    this->sum_time = 0;
    this->last_start_time = 0;
    this->last_coord.x = -1;
    this->last_coord.y = -1;
  }
}

static void
osd_odometer_click(struct odometer *this, struct navit *nav, int pressed, int button, struct point *p)
{
  struct point bp = this->osd_item.p;
  osd_wrap_point(&bp, nav);
  if ((p->x < bp.x || p->y < bp.y || p->x > bp.x + this->osd_item.w || p->y > bp.y + this->osd_item.h || !this->osd_item.configured ) && !this->osd_item.pressed)
    return;
  if (button != 1)
    return;
  if (navit_ignore_button(nav))
    return;
  if (!!pressed == !!this->osd_item.pressed)
    return;
  if (pressed) { //single click handling
    if(this->bActive) { //being stopped
      this->last_coord.x = -1;
      this->last_coord.y = -1;
      this->sum_time += time(0)-this->last_click_time;
    }

  this->bActive ^= 1;  //toggle active flag

  if (this->last_click_time == time(0)) { //double click handling
    osd_odometer_reset(this);
  }

  this->last_click_time = time(0);

  osd_odometer_draw(this, nav,NULL);
  }
}


static int 
osd_odometer_save(struct navit* nav) 
{
		//save odometers that are persistent(ie have name)
		FILE*f;
		GList* list = odometer_list;
		char* fn = g_strdup_printf("%s/odometer.txt",navit_get_user_data_directory(TRUE));
		f = fopen(fn,"w+");
		g_free(fn);
		if(!f) {
			return TRUE;
		}
		while (list) {
			if( ((struct odometer*)(list->data))->name) {
				char*odo_str = osd_odometer_to_string(list->data);
				fprintf(f,"%s",odo_str);
				
			}
			list = g_list_next(list);
		}
		fclose(f);
	return TRUE;
}


static void
osd_odometer_init(struct odometer *this, struct navit *nav)
{
	osd_set_std_graphic(nav, &this->osd_item, (struct osd_priv *)this);

	this->orange = graphics_gc_new(this->osd_item.gr);
	graphics_gc_set_foreground(this->orange, &this->idle_color);
	graphics_gc_set_linewidth(this->orange, this->width);

	this->white = graphics_gc_new(this->osd_item.gr);
	graphics_gc_set_foreground(this->white, &this->osd_item.text_color);
	graphics_gc_set_linewidth(this->white, this->width);

	graphics_gc_set_linewidth(this->osd_item.graphic_fg_white, this->width);

	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_odometer_draw), attr_position_coord_geo, this));

	navit_add_callback(nav, this->click_cb = callback_new_attr_1(callback_cast (osd_odometer_click), attr_button, this));
	
	if(this->autosave_period>0) {
		event_add_timeout(this->autosave_period*1000, 1, callback_new_1(callback_cast(osd_odometer_save), NULL));
	}

	if(this->bAutoStart) {
		this->bActive = 1;
	}
	osd_odometer_draw(this, nav, NULL);
}

static void 
osd_odometer_destroy(struct navit* nav)
{
	GList* list;
	char* fn;
	if(!odometers_saved) {
		odometers_saved = 1;
		osd_odometer_save(NULL);
	}
}

static struct osd_priv *
osd_odometer_new(struct navit *nav, struct osd_methods *meth,
		struct attr **attrs)
{
	FILE* f;
	char* fn;

	struct odometer *this = g_new0(struct odometer, 1);
	struct attr *attr;
	struct color orange_color={0xffff,0xa5a5,0x0000,0xffff};
	this->osd_item.p.x = 120;
	this->osd_item.p.y = 20;
	this->osd_item.w = 60;
	this->osd_item.h = 80;
	this->osd_item.navit = nav;
	this->osd_item.font_size = 200;
	this->osd_item.meth.draw = osd_draw_cast(osd_odometer_draw);

	this->bActive = 0; //do not count on init
	this->sum_dist = 0;
	this->last_click_time = time(0);
	this->last_coord.x = -1;
	this->last_coord.y = -1;

	attr = attr_search(attrs, NULL, attr_label);
	//FIXME find some way to free text!!!!
	if (attr) {
		this->text = g_strdup(attr->u.str);
        }
	else
		this->text = NULL;

	attr = attr_search(attrs, NULL, attr_name);
	//FIXME find some way to free text!!!!
	if (attr) {
		this->name = g_strdup(attr->u.str);
        }
	else
		this->name = NULL;

	attr = attr_search(attrs, NULL, attr_disable_reset);
	if (attr)
		this->bDisableReset = attr->u.num;
	else
		this->bDisableReset = 0;

	attr = attr_search(attrs, NULL, attr_autostart);
	if (attr)
		this->bAutoStart = attr->u.num;
	else
		this->bAutoStart = 0;
	attr = attr_search(attrs, NULL, attr_autosave_period);
	if (attr)
		this->autosave_period = attr->u.num;
	else
		this->autosave_period = -1;  //disabled by default

	osd_set_std_attr(attrs, &this->osd_item, 2);
	attr = attr_search(attrs, NULL, attr_width);
	this->width=attr ? attr->u.num : 2;
	attr = attr_search(attrs, NULL, attr_idle_color);
	this->idle_color=attr ? *attr->u.color : orange_color; // text idle_color defaults to orange

	this->last_coord.x = -1;
	this->last_coord.y = -1;
	this->sum_dist = 0.0;

	//load state from file
	fn = g_strdup_printf("%s/odometer.txt",navit_get_user_data_directory(FALSE));
	f = fopen(fn,"r+");

	if(f) {
		g_free(fn);

		while(!feof(f)) {
			char str[128];
			char *line;
			if(fgets(str,128,f)) 
			{
				char *tok;
				line = g_strdup(str);
				tok = strtok(str," ");
				if(!strcmp(tok,"odometer")) {
					tok = strtok(NULL," ");
					if(this->name && tok && !strcmp(this->name,tok)) {
						osd_odometer_from_string(this,line);
					}
				}
				g_free(line);
			}
		}
		fclose(f);
	}

	navit_command_add_table(nav, commands, sizeof(commands)/sizeof(struct command_table));
	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_odometer_init), attr_graphics_ready, this));
	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_odometer_destroy), attr_destroy, nav));
	odometer_list = g_list_append(odometer_list, this);

	return (struct osd_priv *) this;
}




struct stopwatch {
	struct osd_item osd_item;
	int width;
	struct graphics_gc *orange,*white;
	struct callback *click_cb;
	struct color idle_color;    //text color when counter is idle

	int bDisableReset;
	int bActive;                //counting or not
	time_t current_base_time;   //base time of currently measured time interval
	time_t sum_time;            //sum of previous time intervals (except current intervals)
	time_t last_click_time;     //time of last click (for double click handling)
};

static void 
osd_stopwatch_draw(struct stopwatch *this, struct navit *nav,
		struct vehicle *v)
{

	struct graphics_gc *curr_color;
	char buffer[32]="00:00:00";
	struct point p;
	struct point bbox[4];
	time_t total_sec,total_min,total_hours,total_days;
	total_sec = this->sum_time;

	osd_std_draw(&this->osd_item);

	if(this->bActive) {
		total_sec += time(0)-this->current_base_time;
		}

	total_min = total_sec/60;
	total_hours = total_min/60;
	total_days = total_hours/24;

	if (total_days==0) {
		g_snprintf(buffer,32,"%02d:%02d:%02d", (int)total_hours%24, (int)total_min%60, (int)total_sec%60);
	} else {
		g_snprintf(buffer,32,"%02dd %02d:%02d:%02d",
		(int)total_days, (int)total_hours%24, (int)total_min%60, (int)total_sec%60); 
	}

	graphics_get_text_bbox(this->osd_item.gr, this->osd_item.font, buffer, 0x10000, 0, bbox, 0);
	p.x=(this->osd_item.w-bbox[2].x)/2;
	p.y = this->osd_item.h-this->osd_item.h/10;

	curr_color = this->bActive?this->white:this->orange;
	graphics_draw_text(this->osd_item.gr, curr_color, NULL, this->osd_item.font, buffer, &p, 0x10000, 0);
	graphics_draw_mode(this->osd_item.gr, draw_mode_end);
}


static void
osd_stopwatch_click(struct stopwatch *this, struct navit *nav, int pressed, int button, struct point *p)
{
	struct point bp = this->osd_item.p;
	osd_wrap_point(&bp, nav);
  if ((p->x < bp.x || p->y < bp.y || p->x > bp.x + this->osd_item.w || p->y > bp.y + this->osd_item.h || !this->osd_item.configured ) && !this->osd_item.pressed)
	return;
  if (button != 1)
    return;
  if (navit_ignore_button(nav))
    return;
  if (!!pressed == !!this->osd_item.pressed)
    return;

	if (pressed) { //single click handling

		if(this->bActive) {
		this->sum_time += time(0)-this->current_base_time;
		this->current_base_time = 0;
		} else {
			this->current_base_time = time(0);
		}

		this->bActive ^= 1;  //toggle active flag

		if (this->last_click_time == time(0) && !this->bDisableReset) { //double click handling
		this->bActive = 0;
		this->current_base_time = 0;
		this->sum_time = 0;
		}

	this->last_click_time = time(0);
	}

	osd_stopwatch_draw(this, nav,NULL);
}


static void
osd_stopwatch_init(struct stopwatch *this, struct navit *nav)
{
	osd_set_std_graphic(nav, &this->osd_item, (struct osd_priv *)this);

	this->orange = graphics_gc_new(this->osd_item.gr);
	graphics_gc_set_foreground(this->orange, &this->idle_color);
	graphics_gc_set_linewidth(this->orange, this->width);

	this->white = graphics_gc_new(this->osd_item.gr);
	graphics_gc_set_foreground(this->white, &this->osd_item.text_color);
	graphics_gc_set_linewidth(this->white, this->width);


	graphics_gc_set_linewidth(this->osd_item.graphic_fg_white, this->width);

	event_add_timeout(500, 1, callback_new_1(callback_cast(osd_stopwatch_draw), this));

	navit_add_callback(nav, this->click_cb = callback_new_attr_1(callback_cast (osd_stopwatch_click), attr_button, this));

	osd_stopwatch_draw(this, nav, NULL);
}

static struct osd_priv *
osd_stopwatch_new(struct navit *nav, struct osd_methods *meth,
		struct attr **attrs)
{
	struct stopwatch *this = g_new0(struct stopwatch, 1);
	struct attr *attr;
	struct color orange_color={0xffff,0xa5a5,0x0000,0xffff};

	this->osd_item.p.x = 120;
	this->osd_item.p.y = 20;
	this->osd_item.w = 60;
	this->osd_item.h = 80;
	this->osd_item.navit = nav;
	this->osd_item.font_size = 200;
	this->osd_item.meth.draw = osd_draw_cast(osd_stopwatch_draw);

	this->bActive = 0; //do not count on init
	this->current_base_time = 0;
	this->sum_time = 0;
	this->last_click_time = 0;

	osd_set_std_attr(attrs, &this->osd_item, 2);
	attr = attr_search(attrs, NULL, attr_width);
	this->width=attr ? attr->u.num : 2;
	attr = attr_search(attrs, NULL, attr_idle_color);
	this->idle_color=attr ? *attr->u.color : orange_color; // text idle_color defaults to orange
	attr = attr_search(attrs, NULL, attr_disable_reset);
	if (attr)
		this->bDisableReset = attr->u.num;
	else
		this->bDisableReset = 0;

	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_stopwatch_init), attr_graphics_ready, this));
	return (struct osd_priv *) this;
}


static void
osd_compass_draw(struct compass *this, struct navit *nav,
		 struct vehicle *v)
{
	struct point p,bbox[4];
	struct attr attr_dir, destination_attr, position_attr;
	double dir, vdir = 0;
	char *buffer;
	struct coord c1, c2;
	enum projection pro;

	osd_std_draw(&this->osd_item);
	p.x = this->osd_item.w/2;
	p.y = this->osd_item.w/2;
	graphics_draw_circle(this->osd_item.gr,
			     this->osd_item.graphic_fg_white, &p, this->osd_item.w*5/6);
	if (v) {
		if (vehicle_get_attr(v, attr_position_direction, &attr_dir, NULL)) {
			vdir = *attr_dir.u.numd;
			handle(this->osd_item.gr, this->osd_item.graphic_fg_white, &p, this->osd_item.w/3, -vdir);
		}

		if (navit_get_attr(nav, attr_destination, &destination_attr, NULL)
		    && vehicle_get_attr(v, attr_position_coord_geo,&position_attr, NULL)) {
			pro = destination_attr.u.pcoord->pro;
			transform_from_geo(pro, position_attr.u.coord_geo, &c1);
			c2.x = destination_attr.u.pcoord->x;
			c2.y = destination_attr.u.pcoord->y;
			dir = atan2(c2.x - c1.x, c2.y - c1.y) * 180.0 / M_PI;
			dir -= vdir;
			handle(this->osd_item.gr, this->green, &p, this->osd_item.w/3, dir);
			buffer=format_distance(transform_distance(pro, &c1, &c2),"");
			graphics_get_text_bbox(this->osd_item.gr, this->osd_item.font, buffer, 0x10000, 0, bbox, 0);
			p.x=(this->osd_item.w-bbox[2].x)/2;
			p.y = this->osd_item.h-this->osd_item.h/10;
			graphics_draw_text(this->osd_item.gr, this->green, NULL, this->osd_item.font, buffer, &p, 0x10000, 0);
			g_free(buffer);
		}
	}
	graphics_draw_mode(this->osd_item.gr, draw_mode_end);
}



static void
osd_compass_init(struct compass *this, struct navit *nav)
{
	struct color c;

	osd_set_std_graphic(nav, &this->osd_item, (struct osd_priv *)this);

	this->green = graphics_gc_new(this->osd_item.gr);
	c.r = 0;
	c.g = 65535;
	c.b = 0;
	c.a = 65535;
	graphics_gc_set_foreground(this->green, &c);
	graphics_gc_set_linewidth(this->green, this->width);
	graphics_gc_set_linewidth(this->osd_item.graphic_fg_white, this->width);

	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_compass_draw), attr_position_coord_geo, this));

	osd_compass_draw(this, nav, NULL);
}

static struct osd_priv *
osd_compass_new(struct navit *nav, struct osd_methods *meth,
		struct attr **attrs)
{
	struct compass *this = g_new0(struct compass, 1);
	struct attr *attr;
	this->osd_item.p.x = 20;
	this->osd_item.p.y = 20;
	this->osd_item.w = 60;
	this->osd_item.h = 80;
	this->osd_item.navit = nav;
	this->osd_item.font_size = 200;
	this->osd_item.meth.draw = osd_draw_cast(osd_compass_draw);
	osd_set_std_attr(attrs, &this->osd_item, 2);
	attr = attr_search(attrs, NULL, attr_width);
	this->width=attr ? attr->u.num : 2;
	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_compass_init), attr_graphics_ready, this));
	return (struct osd_priv *) this;
}

struct osd_button {
	int use_overlay;
	struct osd_item item;
	struct callback *draw_cb,*navit_init_cb;
	struct graphics_image *img;
	char *src;
};

static void
osd_button_draw(struct osd_button *this, struct navit *nav)
{
	struct point bp = this->item.p;
	if (!this->item.configured)
		return;
	osd_wrap_point(&bp, nav);
	graphics_draw_image(this->item.gr, this->item.graphic_bg, &bp, this->img);
}

static void
osd_button_init(struct osd_button *this, struct navit *nav)
{
	struct graphics *gra = navit_get_graphics(nav);
	dbg(1, "enter\n");
	this->img = graphics_image_new(gra, this->src);
	if (!this->img) {
		dbg(1, "failed to load '%s'\n", this->src);
		return;
	}
	if (!this->item.w)
		this->item.w=this->img->width;
	if (!this->item.h)
		this->item.h=this->img->height;
	if (this->use_overlay) {
		struct graphics_image *img;
		struct point p;
		osd_set_std_graphic(nav, &this->item, (struct osd_priv *)this);
		img=graphics_image_new(this->item.gr, this->src);
		p.x=(this->item.w-this->img->width)/2;
		p.y=(this->item.h-this->img->height)/2;
		osd_std_draw(&this->item);
		graphics_draw_image(this->item.gr, this->item.graphic_bg, &p, img);
		graphics_draw_mode(this->item.gr, draw_mode_end);
		graphics_image_free(this->item.gr, img);
	} else {
		osd_set_std_config(nav, &this->item);
		this->item.gr=gra;
		this->item.graphic_bg=graphics_gc_new(this->item.gr);
		graphics_add_callback(gra, this->draw_cb=callback_new_attr_2(callback_cast(osd_button_draw), attr_postdraw, this, nav));
	}
	navit_add_callback(nav, this->navit_init_cb = callback_new_attr_1(callback_cast (osd_std_click), attr_button, &this->item));
	osd_button_draw(this,nav);
}

static struct osd_priv *
osd_button_new(struct navit *nav, struct osd_methods *meth,
	       struct attr **attrs)
{
	struct osd_button *this = g_new0(struct osd_button, 1);
	struct attr *attr;

	this->item.navit = nav;
	this->item.meth.draw = osd_draw_cast(osd_button_draw);

	osd_set_std_attr(attrs, &this->item, 1|16);

	attr=attr_search(attrs, NULL, attr_use_overlay);
	if (attr)
		this->use_overlay=attr->u.num;
	if (!this->item.command) {
		dbg(0, "no command\n");
		goto error;
	}
	attr = attr_search(attrs, NULL, attr_src);
	if (!attr) {
		dbg(0, "no src\n");
		goto error;
	}

	this->src = graphics_icon_path(attr->u.str);

	navit_add_callback(nav, this->navit_init_cb = callback_new_attr_1(callback_cast (osd_button_init), attr_graphics_ready, this));

	return (struct osd_priv *) this;
      error:
	g_free(this);
	return NULL;
}

struct osd_image {
	int use_overlay;
	struct osd_item item;
	struct callback *draw_cb,*navit_init_cb;
	struct graphics_image *img;
	char *src;
};

static void
osd_image_draw(struct osd_image *this, struct navit *nav)
{
	struct point bp = this->item.p;
	osd_wrap_point(&bp, nav);
	graphics_draw_image(this->item.gr, this->item.graphic_bg, &bp, this->img);
}

static void
osd_image_init(struct osd_image *this, struct navit *nav)
{
	struct graphics *gra = navit_get_graphics(nav);
	dbg(1, "enter\n");
	this->img = graphics_image_new(gra, this->src);
	if (!this->img) {
		dbg(1, "failed to load '%s'\n", this->src);
		return;
	}
	if (!this->item.w)
		this->item.w=this->img->width;
	if (!this->item.h)
		this->item.h=this->img->height;
	if (this->use_overlay) {
		struct graphics_image *img;
		struct point p;
		osd_set_std_graphic(nav, &this->item, (struct osd_priv *)this);
		img=graphics_image_new(this->item.gr, this->src);
		p.x=(this->item.w-this->img->width)/2;
		p.y=(this->item.h-this->img->height)/2;
		osd_std_draw(&this->item);
		graphics_draw_image(this->item.gr, this->item.graphic_bg, &p, img);
		graphics_draw_mode(this->item.gr, draw_mode_end);
		graphics_image_free(this->item.gr, img);
	} else {
		this->item.configured=1;
		this->item.gr=gra;
		this->item.graphic_bg=graphics_gc_new(this->item.gr);
		graphics_add_callback(gra, this->draw_cb=callback_new_attr_2(callback_cast(osd_button_draw), attr_postdraw, this, nav));
	}
	osd_image_draw(this,nav);
}

static struct osd_priv *
osd_image_new(struct navit *nav, struct osd_methods *meth,
	       struct attr **attrs)
{
	struct osd_image *this = g_new0(struct osd_image, 1);
	struct attr *attr;

	this->item.navit = nav;
	this->item.meth.draw = osd_draw_cast(osd_image_draw);

	osd_set_std_attr(attrs, &this->item, 1);

	attr=attr_search(attrs, NULL, attr_use_overlay);
	if (attr)
		this->use_overlay=attr->u.num;
	attr = attr_search(attrs, NULL, attr_src);
	if (!attr) {
		dbg(0, "no src\n");
		goto error;
	}

	this->src = graphics_icon_path(attr->u.str);

	navit_add_callback(nav, this->navit_init_cb = callback_new_attr_1(callback_cast (osd_image_init), attr_graphics_ready, this));

	return (struct osd_priv *) this;
      error:
	g_free(this);
	return NULL;
}

struct nav_next_turn {
	struct osd_item osd_item;
	char *test_text;
	char *icon_src;
	int icon_h, icon_w, active;
	char *last_name;
	int level;
};

static void
osd_nav_next_turn_draw(struct nav_next_turn *this, struct navit *navit,
		       struct vehicle *v)
{
	struct point p;
	int do_draw = 0;
	struct navigation *nav = NULL;
	struct map *map = NULL;
	struct map_rect *mr = NULL;
	struct item *item = NULL;
	struct graphics_image *gr_image;
	char *image;
	char *name = "unknown";
	int level = this->level;

	if (navit)
		nav = navit_get_navigation(navit);
	if (nav)
		map = navigation_get_map(nav);
	if (map)
		mr = map_rect_new(map, NULL);
	if (mr)
		while ((item = map_rect_get_item(mr))
		       && (item->type == type_nav_position || item->type == type_nav_none || level-- > 0));
	if (item) {
		name = item_to_name(item->type);
		dbg(1, "name=%s\n", name);
		if (this->active != 1 || this->last_name != name) {
			this->active = 1;
			this->last_name = name;
			do_draw = 1;
		}
	} else {
		if (this->active != 0) {
			this->active = 0;
			do_draw = 1;
		}
	}
	if (mr)
		map_rect_destroy(mr);

	if (do_draw) {
		osd_std_draw(&this->osd_item);
		if (this->active) {
			image = g_strdup_printf(this->icon_src, name);
			dbg(1, "image=%s\n", image);
			gr_image =
			    graphics_image_new_scaled(this->osd_item.gr,
						      image, this->icon_w,
						      this->icon_h);
			if (!gr_image) {
				dbg(0,"failed to load %s in %dx%d\n",image,this->icon_w,this->icon_h);
				g_free(image);
				image = graphics_icon_path("unknown.xpm");
				gr_image =
				    graphics_image_new_scaled(this->
							      osd_item.gr,
							      image,
							      this->icon_w,
							      this->
							      icon_h);
			}
			dbg(1, "gr_image=%p\n", gr_image);
			if (gr_image) {
				p.x =
				    (this->osd_item.w -
				     gr_image->width) / 2;
				p.y =
				    (this->osd_item.h -
				     gr_image->height) / 2;
				graphics_draw_image(this->osd_item.gr,
						    this->osd_item.
						    graphic_fg_white, &p,
						    gr_image);
				graphics_image_free(this->osd_item.gr,
						    gr_image);
			}
			g_free(image);
		}
		graphics_draw_mode(this->osd_item.gr, draw_mode_end);
	}
}

static void
osd_nav_next_turn_init(struct nav_next_turn *this, struct navit *nav)
{
	osd_set_std_graphic(nav, &this->osd_item, (struct osd_priv *)this);
	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_nav_next_turn_draw), attr_position_coord_geo, this));
	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_std_click), attr_button, &this->osd_item));
	osd_nav_next_turn_draw(this, nav, NULL);
}

static struct osd_priv *
osd_nav_next_turn_new(struct navit *nav, struct osd_methods *meth,
		      struct attr **attrs)
{
	struct nav_next_turn *this = g_new0(struct nav_next_turn, 1);
	struct attr *attr;

	this->osd_item.p.x = 20;
	this->osd_item.p.y = -80;
	this->osd_item.w = 70;
	this->osd_item.navit = nav;
	this->osd_item.h = 70;
	this->osd_item.font_size = 200;
	this->osd_item.meth.draw = osd_draw_cast(osd_nav_next_turn_draw);
	osd_set_std_attr(attrs, &this->osd_item, 0);

	this->icon_w = -1;
	this->icon_h = -1;
	this->active = -1;
	this->level  = 0;

	attr = attr_search(attrs, NULL, attr_icon_w);
	if (attr)
		this->icon_w = attr->u.num;

	attr = attr_search(attrs, NULL, attr_icon_h);
	if (attr)
		this->icon_h = attr->u.num;

	attr = attr_search(attrs, NULL, attr_icon_src);
	if (attr) {
		struct file_wordexp *we;
		char **array;
		we = file_wordexp_new(attr->u.str);
		array = file_wordexp_get_array(we);
		this->icon_src = graphics_icon_path(array[0]);
		file_wordexp_destroy(we);
	} else {
		this->icon_src = graphics_icon_path("%s_wh.svg");
	}
	
	attr = attr_search(attrs, NULL, attr_level);
	if (attr)
		this->level=attr->u.num;

	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_nav_next_turn_init), attr_graphics_ready, this));
	return (struct osd_priv *) this;
}

struct nav_toggle_announcer
{
	int w,h;
	struct callback *navit_init_cb;
	struct osd_item item;
	char *icon_src;
	int icon_h, icon_w, active, last_state;
};

static void
osd_nav_toggle_announcer_draw(struct nav_toggle_announcer *this, struct navit *navit, struct vehicle *v)
{
	struct point p;
	int do_draw = 0;
	struct graphics_image *gr_image;
	char *path;
	char *gui_sound_off = "gui_sound_off";
	char *gui_sound_on = "gui_sound";
    struct attr attr, speechattr;

    if (this->last_state == -1)
    {
        if (!navit_get_attr(navit, attr_speech, &speechattr, NULL))
            if (!speech_get_attr(speechattr.u.speech, attr_active, &attr, NULL))
                attr.u.num = 1;
        this->active = attr.u.num;
    } else
        this->active = !this->active;

    if(this->active != this->last_state)
    {
        this->last_state = this->active;
        do_draw = 1;
    }

	if (do_draw)
    {
		graphics_draw_mode(this->item.gr, draw_mode_begin);
		p.x = 0;
		p.y = 0;
		graphics_draw_rectangle(this->item.gr, this->item.graphic_bg, &p, this->item.w, this->item.h);

		if (this->active)
            path = g_strdup_printf(this->icon_src, gui_sound_on);
        else
            path = g_strdup_printf(this->icon_src, gui_sound_off);
        
        gr_image = graphics_image_new_scaled(this->item.gr, path, this->icon_w, this->icon_h);
        if (!gr_image)
        {
            g_free(path);
            path = graphics_icon_path("unknown.xpm");
            gr_image = graphics_image_new_scaled(this->item.gr, path, this->icon_w, this->icon_h);
        }
        
        dbg(1, "gr_image=%p\n", gr_image);
        
        if (gr_image)
        {
            p.x = (this->item.w - gr_image->width) / 2;
            p.y = (this->item.h - gr_image->height) / 2;
            graphics_draw_image(this->item.gr, this->item.graphic_fg_white, &p, gr_image);
            graphics_image_free(this->item.gr, gr_image);
        }
        
        g_free(path);
		graphics_draw_mode(this->item.gr, draw_mode_end);
	}
}

static void
osd_nav_toggle_announcer_init(struct nav_toggle_announcer *this, struct navit *nav)
{
	osd_set_std_graphic(nav, &this->item, (struct osd_priv *)this);
	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_nav_toggle_announcer_draw), attr_speech, this));
    navit_add_callback(nav, this->navit_init_cb = callback_new_attr_1(callback_cast(osd_std_click), attr_button, &this->item));
	osd_nav_toggle_announcer_draw(this, nav, NULL);
}

static struct osd_priv *
osd_nav_toggle_announcer_new(struct navit *nav, struct osd_methods *meth, struct attr **attrs)
{
	struct nav_toggle_announcer *this = g_new0(struct nav_toggle_announcer, 1);
    struct attr *attr;
    char *command = "announcer_toggle()";

	this->item.w = 48;
	this->item.h = 48;
	this->item.p.x = -64;
	this->item.navit = nav;
	this->item.p.y = 76;
	this->item.meth.draw = osd_draw_cast(osd_nav_toggle_announcer_draw);

	osd_set_std_attr(attrs, &this->item, 0);

	this->icon_w = -1;
	this->icon_h = -1;
    this->last_state = -1;

    attr = attr_search(attrs, NULL, attr_icon_src);
	if (attr) {
		struct file_wordexp *we;
		char **array;
		we = file_wordexp_new(attr->u.str);
		array = file_wordexp_get_array(we);
		this->icon_src = g_strdup(array[0]);
		file_wordexp_destroy(we);
	} else
		this->icon_src = graphics_icon_path("%s_32.xpm");

    this->item.command = g_strdup(command);

	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_nav_toggle_announcer_init), attr_graphics_ready, this));
	return (struct osd_priv *) this;
}

enum osd_speed_warner_eAnnounceState {eNoWarn=0,eWarningTold=1};
enum camera_t {CAM_FIXED=1, CAM_TRAFFIC_LAMP, CAM_RED, CAM_SECTION, CAM_MOBILE, CAM_RAIL, CAM_TRAFFIPAX};
char*camera_t_strs[] = {"None","Fix","Traffic lamp","Red detect","Section","Mobile","Rail","Traffipax(non persistent)"};
char*camdir_t_strs[] = {"All dir.","UNI-dir","BI-dir"};
enum cam_dir_t {CAMDIR_ALL=0, CAMDIR_ONE, CAMDIR_TWO};

struct osd_speed_cam_entry {
	double lon;
	double lat;
	enum camera_t cam_type;
	int speed_limit;
	enum cam_dir_t cam_dir;
	int direction;
};

struct osd_speed_cam {
  struct osd_item item;
  int width;
  struct graphics_gc *white,*orange;
  struct graphics_gc *red;
  struct color idle_color; 

  int announce_on;
  enum osd_speed_warner_eAnnounceState announce_state;
  char *text;                 //text of label attribute for this osd
};

static double 
angle_diff(firstAngle,secondAngle)
{
        double difference = secondAngle - firstAngle;
        while (difference < -180) difference += 360;
        while (difference > 180) difference -= 360;
        return difference;
}

static void
osd_speed_cam_draw(struct osd_speed_cam *this_, struct navit *navit, struct vehicle *v)
{
  struct attr position_attr,vehicle_attr;
  struct point p, bbox[4];
  struct attr speed_attr;
  struct vehicle* curr_vehicle = v;
  struct coord curr_coord;
  struct coord cam_coord;
  struct mapset* ms;

  double dCurrDist = -1;
  int dir_idx = -1;
  int dir = -1;
  int spd = -1;
  int idx = -1;
  double speed = -1;
  int bFound = 0;

  int dst=2000;
  int dstsq=dst*dst;
  struct map_selection sel;
  struct map_rect *mr;
  struct mapset_handle *msh;
  struct map *map;
  struct item *item;

  struct attr attr_dir;
  struct graphics_gc *curr_color;
  int ret_attr = 0;

  if(navit) {
    navit_get_attr(navit, attr_vehicle, &vehicle_attr, NULL);
  }
  else {
    return;
  }
  if (vehicle_attr.u.vehicle) {
    curr_vehicle = vehicle_attr.u.vehicle;
  }

  if(0==curr_vehicle)
    return;

  if(!(ms=navit_get_mapset(navit))) {
    return;
  }

  ret_attr = vehicle_get_attr(curr_vehicle, attr_position_coord_geo,&position_attr, NULL);
  if(0==ret_attr) {
    return;
  }

  transform_from_geo(projection_mg, position_attr.u.coord_geo, &curr_coord);

  sel.next=NULL;
  sel.order=18;
  sel.range.min=type_tec_common;
  sel.range.max=type_tec_common;
  sel.u.c_rect.lu.x=curr_coord.x-dst;
  sel.u.c_rect.lu.y=curr_coord.y+dst;
  sel.u.c_rect.rl.x=curr_coord.x+dst;
  sel.u.c_rect.rl.y=curr_coord.y-dst;
  
  msh=mapset_open(ms);
  while ((map=mapset_next(msh, 1))) {
    mr=map_rect_new(map, &sel);
    if (!mr)
      continue;
    while ((item=map_rect_get_item(mr))) {
      struct coord cn;
      if (item->type == type_tec_common && item_coord_get(item, &cn, 1)) {
        int dist=transform_distance_sq(&cn, &curr_coord);
        if (dist < dstsq) {  
  		  struct attr tec_attr;
          bFound = 1;
          dstsq=dist;
          dCurrDist = sqrt(dist);
          cam_coord = cn;
          idx = -1;
          if(item_attr_get(item,attr_tec_type,&tec_attr)) {
            idx = tec_attr.u.num;
          }
          dir_idx = -1;
          if(item_attr_get(item,attr_tec_dirtype,&tec_attr)) {
            dir_idx = tec_attr.u.num;
          }
          dir= 0;
          if(item_attr_get(item,attr_tec_direction,&tec_attr)) {
            dir = tec_attr.u.num;
          }
          spd= 0;
          if(item_attr_get(item,attr_maxspeed,&tec_attr)) {
            spd = tec_attr.u.num;
          }
        }
      }
    }
    map_rect_destroy(mr);
  }
  mapset_close(msh);

  osd_std_draw(&this_->item);
  if(bFound) {
    dCurrDist = transform_distance(projection_mg, &curr_coord, &cam_coord);
    ret_attr = vehicle_get_attr(curr_vehicle,attr_position_speed,&speed_attr, NULL);
    if(0==ret_attr) {
      return;
    }
    speed = *speed_attr.u.numd;
    if(dCurrDist <= speed*750.0/130.0) {  //at speed 130 distance limit is 750m
      if(this_->announce_state==eNoWarn && this_->announce_on) {
        this_->announce_state=eWarningTold; //warning told
        navit_say(navit, _("Look out! Camera!"));
      }
    }
    else {
      this_->announce_state=eNoWarn;
    }
  
    if(this_->text && 0<=idx ) {
      char buffer [256]="";
      char buffer2[256]="";
	  char dir_str[16];
	  char spd_str[16];
      buffer [0] = 0;
      buffer2[0] = 0; 
  
      str_replace(buffer,this_->text,"${distance}",format_distance(dCurrDist,""));
      str_replace(buffer2,buffer,"${camera_type}",(idx<=CAM_TRAFFIPAX)?camera_t_strs[idx]:"");
      str_replace(buffer,buffer2,"${camera_dir}",(0<=dir_idx && dir_idx<=CAMDIR_TWO)?camdir_t_strs[dir_idx]:"");
      sprintf(dir_str,"%d",dir);
      sprintf(spd_str,"%d",spd);
      str_replace(buffer2,buffer,"${direction}",dir_str);
      str_replace(buffer,buffer2,"${speed_limit}",spd_str);
  
      graphics_get_text_bbox(this_->item.gr, this_->item.font, buffer, 0x10000, 0, bbox, 0);
      p.x=(this_->item.w-bbox[2].x)/2;
      p.y = this_->item.h-this_->item.h/10;
      curr_color = this_->orange;
      //tolerance is +-20 degrees
      if(
        dir_idx==CAMDIR_ONE && 
        dCurrDist <= speed*750.0/130.0 && 
        vehicle_get_attr(v, attr_position_direction, &attr_dir, NULL) && 
        fabs(angle_diff(dir,*attr_dir.u.numd))<=20 ) {
          curr_color = this_->red;
      }
      //tolerance is +-20 degrees in both directions
      else if(
        dir_idx==CAMDIR_TWO && 
        dCurrDist <= speed*750.0/130.0 && 
        vehicle_get_attr(v, attr_position_direction, &attr_dir, NULL) && 
        (fabs(angle_diff(dir,*attr_dir.u.numd))<=20 || fabs(angle_diff(dir+180,*attr_dir.u.numd))<=20 )) {
          curr_color = this_->red;
      }
      else if(dCurrDist <= speed*750.0/130.0) { 
        curr_color = this_->red;
      }
      graphics_draw_text(this_->item.gr, curr_color, NULL, this_->item.font, buffer, &p, 0x10000, 0);
    }
  }
  graphics_draw_mode(this_->item.gr, draw_mode_end);
}

static void
osd_speed_cam_init(struct osd_speed_cam *this, struct navit *nav)
{
  struct color red_color={0xffff,0x0000,0x0000,0xffff};
  osd_set_std_graphic(nav, &this->item, (struct osd_priv *)this);

  this->red = graphics_gc_new(this->item.gr);
  graphics_gc_set_foreground(this->red, &red_color);
  graphics_gc_set_linewidth(this->red, this->width);

  this->orange = graphics_gc_new(this->item.gr);
  graphics_gc_set_foreground(this->orange, &this->idle_color);
  graphics_gc_set_linewidth(this->orange, this->width);

  this->white = graphics_gc_new(this->item.gr);
  graphics_gc_set_foreground(this->white, &this->item.text_color);
  graphics_gc_set_linewidth(this->white, this->width);


  graphics_gc_set_linewidth(this->item.graphic_fg_white, this->width);

  navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_speed_cam_draw), attr_position_coord_geo, this));

}

static struct osd_priv *
osd_speed_cam_new(struct navit *nav, struct osd_methods *meth, struct attr **attrs)
{

  struct color default_color={0xffff,0xa5a5,0x0000,0xffff};

  struct osd_speed_cam *this = g_new0(struct osd_speed_cam, 1);
  struct attr *attr;
  this->item.p.x = 120;
  this->item.p.y = 20;
  this->item.w = 60;
  this->item.h = 80;
  this->item.navit = nav;
  this->item.font_size = 200;
  this->item.meth.draw = osd_draw_cast(osd_speed_cam_draw);

  osd_set_std_attr(attrs, &this->item, 2);
  attr = attr_search(attrs, NULL, attr_width);
  this->width=attr ? attr->u.num : 2;
  attr = attr_search(attrs, NULL, attr_idle_color);
  this->idle_color=attr ? *attr->u.color : default_color; // text idle_color defaults to orange

  attr = attr_search(attrs, NULL, attr_label);
  if (attr) {
    this->text = g_strdup(attr->u.str);
  }
  else
    this->text = NULL;

  attr = attr_search(attrs, NULL, attr_announce_on);
  if (attr) {
    this->announce_on = attr->u.num;
  }
  else {
    this->announce_on = 1;    //announce by default
  }

  navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_speed_cam_init), attr_graphics_ready, this));
  return (struct osd_priv *) this;
}

struct osd_speed_warner {
	struct osd_item item;
	struct graphics_gc *red;
	struct graphics_gc *green;
	struct graphics_gc *grey;
	struct graphics_gc *black;
	struct graphics_gc *white;
	int width;
	int active;
	int d;
	double speed_exceed_limit_offset;
	double speed_exceed_limit_percent;
	int announce_on;
	enum osd_speed_warner_eAnnounceState announce_state;
	int bTextOnly;
};

static void
osd_speed_warner_draw(struct osd_speed_warner *this, struct navit *navit, struct vehicle *v)
{
    struct point p,bbox[4];
    char text[16]="0";

    struct tracking *tracking = NULL;
    struct graphics_gc *osd_color=this->grey;


    osd_std_draw(&this->item);
    p.x=this->item.w/2-this->d/4;
    p.y=this->item.h/2-this->d/4;
    p.x=this->item.w/2;
    p.y=this->item.h/2;
	//graphics_draw_circle(this->item.gr, this->white, &p, this->d/2);

    if (navit) {
        tracking = navit_get_tracking(navit);
    }
    if (tracking) {

        struct attr maxspeed_attr,speed_attr;
        int *flags;
        double routespeed = -1;
        double tracking_speed = -1;
	int osm_data = 0;
        struct item *item;
        item=tracking_get_current_item(tracking);

        flags=tracking_get_current_flags(tracking);
        if (flags && (*flags & AF_SPEED_LIMIT) && tracking_get_attr(tracking, attr_maxspeed, &maxspeed_attr, NULL)) {
            routespeed = maxspeed_attr.u.num;
	    osm_data = 1;
        }
        if (routespeed == -1) {
            struct vehicleprofile *prof=navit_get_vehicleprofile(navit);
            struct roadprofile *rprof=NULL;
            if (prof && item)
                rprof=vehicleprofile_get_roadprofile(prof, item->type);
            if (rprof) {
                if(rprof->maxspeed!=0)
                    routespeed=rprof->maxspeed;
            }
        }
        tracking_get_attr(tracking, attr_position_speed, &speed_attr, NULL);
        tracking_speed = *speed_attr.u.numd;
        if( -1 != tracking_speed && -1 != routespeed ) {
            g_snprintf(text,16,"%s%.0lf",osm_data ? "" : "~",routespeed);
            if( this->speed_exceed_limit_offset+routespeed<tracking_speed &&
                (100.0+this->speed_exceed_limit_percent)/100.0*routespeed<tracking_speed ) {
                if(this->announce_state==eNoWarn && this->announce_on) {
                    this->announce_state=eWarningTold; //warning told
                    navit_say(navit,_("Please decrease your speed"));
                }
            }
            if( tracking_speed <= routespeed ) {
                this->announce_state=eNoWarn; //no warning
                osd_color = this->green;
            }
            else {
                osd_color = this->red;
            }
        } else {
            osd_color = this->grey;
        }
    } else {
        //when tracking is not available display grey
        osd_color = this->grey;
    }
    if(0==this->bTextOnly) {
      graphics_draw_circle(this->item.gr, osd_color, &p, this->d-this->width*2 );
    }

	graphics_get_text_bbox(this->item.gr, this->item.font, text, 0x10000, 0, bbox, 0);
	p.x=(this->item.w-bbox[2].x)/2;
	p.y=(this->item.h+bbox[2].y)/2-bbox[2].y;
	graphics_draw_text(this->item.gr, osd_color, NULL, this->item.font, text, &p, 0x10000, 0);
	graphics_draw_mode(this->item.gr, draw_mode_end);
}

static void
osd_speed_warner_init(struct osd_speed_warner *this, struct navit *nav)
{
	struct color white_color={0xffff,0xffff,0xffff,0x0000};
	struct color red_color={0xffff,0,0,0xffff};
	struct color green_color={0,0xffff,0,0xffff};
	struct color grey_color={0x8888,0x8888,0x8888,0x8888};
	struct color black_color={0x1111,0x1111,0x1111,0x9999};

	osd_set_std_graphic(nav, &this->item, (struct osd_priv *)this);
	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_speed_warner_draw), attr_position_coord_geo, this));



	this->white=graphics_gc_new(this->item.gr);
	graphics_gc_set_foreground(this->white, &white_color);

	graphics_gc_set_linewidth(this->white, this->d/2-2 /*-this->width*/ );

	this->red=graphics_gc_new(this->item.gr);
	graphics_gc_set_foreground(this->red, &red_color);
	graphics_gc_set_linewidth(this->red, this->width);

	this->green=graphics_gc_new(this->item.gr);
	graphics_gc_set_foreground(this->green, &green_color);
	graphics_gc_set_linewidth(this->green, this->width-2);

	this->grey=graphics_gc_new(this->item.gr);
	graphics_gc_set_foreground(this->grey, &grey_color);
	graphics_gc_set_linewidth(this->grey, this->width);

	this->black=graphics_gc_new(this->item.gr);
	graphics_gc_set_foreground(this->black, &black_color);
	graphics_gc_set_linewidth(this->black, this->width);

	osd_speed_warner_draw(this, nav, NULL);
}

static struct osd_priv *
osd_speed_warner_new(struct navit *nav, struct osd_methods *meth, struct attr **attrs)
{
	struct osd_speed_warner *this=g_new0(struct osd_speed_warner, 1);
	struct attr *attr;
	this->item.p.x=-80;
	this->item.p.y=20;
	this->item.w=60;
	this->item.navit = nav;
	this->item.h=60;
	this->active=-1;
	this->item.meth.draw = osd_draw_cast(osd_speed_warner_draw);

	attr = attr_search(attrs, NULL, attr_speed_exceed_limit_offset);
	if (attr) {
		this->speed_exceed_limit_offset = attr->u.num;
	} else
		this->speed_exceed_limit_offset = 15;    //by default 15 km/h

	attr = attr_search(attrs, NULL, attr_speed_exceed_limit_percent);
	if (attr) {
		this->speed_exceed_limit_percent = attr->u.num;
    } else
		this->speed_exceed_limit_percent = 10;    //by default factor of 1.1

        this->bTextOnly = 0;	//by default display graphics also
	attr = attr_search(attrs, NULL, attr_label);
	if (attr) {
          if (!strcmp("text_only",attr->u.str)) {
            this->bTextOnly = 1;
          }
        }

	attr = attr_search(attrs, NULL, attr_announce_on);
	if (attr)
		this->announce_on = attr->u.num;
	else
		this->announce_on = 1;    //announce by default
	osd_set_std_attr(attrs, &this->item, 2);
	this->d=this->item.w;
	if (this->item.h < this->d)
		this->d=this->item.h;
	this->width=this->d/10;
	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_speed_warner_init), attr_graphics_ready, this));
	return (struct osd_priv *) this;
}

struct osd_text_item {
    int static_text;
    char *text;
    void *prev;
    void *next;
    enum attr_type section;
    enum attr_type attr_typ;
    void *root;
    int offset;
    char *format;
};

struct osd_text {
	struct osd_item osd_item;
	int active;
	char *text;
	int align;
	char *last;
	struct osd_text_item *items;
};


/**
 * @brief Format a text attribute
 *
 * Returns the formatted current value of an attribute as a string
 * 
 * @param attr Pointer to an attr structure specifying the attribute to be formatted
 * @param format Pointer to a string specifying how to format the attribute. Allowed format strings depend on the attribute; this member can be NULL.
 * @returns Pointer to a string containing the formatted value
 */
static char *
osd_text_format_attr(struct attr *attr, char *format)
{
	struct tm tm, text_tm, text_tm0;
	time_t textt;
	int days=0;
	char buffer[1024];

	switch (attr->type) {
	case attr_position_speed:
		return format_speed(*attr->u.numd,"",format);
	case attr_position_height:
	case attr_position_direction:
		return format_float_0(*attr->u.numd);
	case attr_position_magnetic_direction:
		return g_strdup_printf("%d",attr->u.num);
	case attr_position_coord_geo:
		if ((!format) || (!strcmp(format,"pos_degminsec")))
		{ 
			coord_format(attr->u.coord_geo->lat,attr->u.coord_geo->lng,DEGREES_MINUTES_SECONDS,buffer,sizeof(buffer));
			return g_strdup(buffer);
		}
		else if (!strcmp(format,"pos_degmin")) 
		{
			coord_format(attr->u.coord_geo->lat,attr->u.coord_geo->lng,DEGREES_MINUTES,buffer,sizeof(buffer));
			return g_strdup(buffer);
		}
		else if (!strcmp(format,"pos_deg")) 
		{
			coord_format(attr->u.coord_geo->lat,attr->u.coord_geo->lng,DEGREES_DECIMAL,buffer,sizeof(buffer));
			return g_strdup(buffer);
		}
		else if (!strcmp(format,"lat_degminsec")) 
		{
			coord_format(attr->u.coord_geo->lat,360,DEGREES_MINUTES_SECONDS,buffer,sizeof(buffer));
			return g_strdup(buffer);
		}
		else if (!strcmp(format,"lat_degmin")) 
		{
			coord_format(attr->u.coord_geo->lat,360,DEGREES_MINUTES,buffer,sizeof(buffer));
			return g_strdup(buffer);
		}
		else if (!strcmp(format,"lat_deg")) 
		{
			coord_format(attr->u.coord_geo->lat,360,DEGREES_DECIMAL,buffer,sizeof(buffer));
			return g_strdup(buffer);
		}
		else if (!strcmp(format,"lng_degminsec")) 
		{
			coord_format(360,attr->u.coord_geo->lng,DEGREES_MINUTES_SECONDS,buffer,sizeof(buffer));
			return g_strdup(buffer);
		}
		else if (!strcmp(format,"lng_degmin")) 
		{
			coord_format(360,attr->u.coord_geo->lng,DEGREES_MINUTES,buffer,sizeof(buffer));
			return g_strdup(buffer);
		}
		else if (!strcmp(format,"lng_deg")) 
		{
			coord_format(360,attr->u.coord_geo->lng,DEGREES_DECIMAL,buffer,sizeof(buffer));
			return g_strdup(buffer);
		}
		else
		{ // fall back to pos_degminsec
			coord_format(attr->u.coord_geo->lat,attr->u.coord_geo->lng,DEGREES_MINUTES_SECONDS,buffer,sizeof(buffer));
			return g_strdup(buffer);
		}
	case attr_destination_time:
		if (!format || (strcmp(format,"arrival") && strcmp(format,"remaining")))
			break;
		textt = time(NULL);
		tm = *localtime(&textt);
		if (!strcmp(format,"remaining")) {
			textt-=tm.tm_hour*3600+tm.tm_min*60+tm.tm_sec;
			tm = *localtime(&textt);
		}
		textt += attr->u.num / 10;
		text_tm = *localtime(&textt);
		if (tm.tm_year != text_tm.tm_year || tm.tm_mon != text_tm.tm_mon || tm.tm_mday != text_tm.tm_mday) {
			text_tm0 = text_tm;
			text_tm0.tm_sec = 0;
			text_tm0.tm_min = 0;
			text_tm0.tm_hour = 0;
			tm.tm_sec = 0;
			tm.tm_min = 0;
			tm.tm_hour = 0;
			days = (mktime(&text_tm0) - mktime(&tm) + 43200) / 86400;
			}
		return format_time(&text_tm, days);
	case attr_length:
	case attr_destination_length:
		if (!format)
			break;
		if (!strcmp(format,"named"))
			return format_distance(attr->u.num,"");
		if (!strcmp(format,"value") || !strcmp(format,"unit")) {
			char *ret,*tmp=format_distance(attr->u.num," ");
			char *pos=strchr(tmp,' ');
			if (! pos)
				return tmp;
			*pos++='\0';
			if (!strcmp(format,"value"))
				return tmp;
			ret=g_strdup(pos);
			g_free(tmp);
			return ret;
		}
	case attr_position_time_iso8601:
		if ((!format) || (!strcmp(format,"iso8601")))
		{ 
			break;
		}
		else 
		{
			if (strstr(format, "local;") == format)
			{
				textt = iso8601_to_secs(attr->u.str);
				memcpy ((void *) &tm, (void *) localtime(&textt), sizeof(tm));
				strftime(buffer, sizeof(buffer), (char *)(format + 6), &tm);
			}
			else if ((sscanf(format, "%*c%2d:%2d;", &(text_tm.tm_hour), &(text_tm.tm_min)) == 2) && (strchr("+-", format[0])))
			{
				if (strchr("-", format[0]))
				{
					textt = iso8601_to_secs(attr->u.str) - text_tm.tm_hour * 3600 - text_tm.tm_min * 60;
				}
				else
				{
					textt = iso8601_to_secs(attr->u.str) + text_tm.tm_hour * 3600 + text_tm.tm_min * 60;
				}
				memcpy ((void *) &tm, (void *) gmtime(&textt), sizeof(tm));
				strftime(buffer, sizeof(buffer), &format[strcspn(format, ";") + 1], &tm);
			}
			else
			{
				sscanf(attr->u.str, "%4d-%2d-%2dT%2d:%2d:%2d", &(tm.tm_year), &(tm.tm_mon), &(tm.tm_mday), &(tm.tm_hour), &(tm.tm_min), &(tm.tm_sec));
				// the tm structure definition is kinda weird and needs some extra voodoo
				tm.tm_year-=1900; tm.tm_mon--;
				// get weekday and day of the year
				mktime(&tm);
				strftime(buffer, sizeof(buffer), format, &tm);
			}
			return g_strdup(buffer);
		}
	default:
		break;
	}
	return attr_to_text(attr, NULL, 1);
}

/**
 * Parse a string of the form key.subkey or key[index].subkey into its components, where subkey can itself have its own index and further subkeys
 *
 * @param in String to parse (the part before subkey will be modified by the function); upon returning this pointer will point to a string containing key
 * @param index Pointer to an address that will receive a pointer to a string containing index or a null pointer if key does not have an index
 * @returns If the function succeeds, a pointer to a string containing subkey, i.e. everything following the first period, or a pointer to an empty string if there is nothing left to parse. If the function fails(index with missing closed bracket or passing a null pointer as index argument when an index was encountered), the return value is NULL
 */
static char *
osd_text_split(char *in, char **index)
{
	char *pos;
	int len;
	if (index)
		*index=NULL;
	len=strcspn(in,"[.");
	in+=len;
	switch (in[0]) {
	case '\0':
		return in;
	case '.':
		*in++='\0';
		return in;
	case '[':
		if (!index)
			return NULL;
		*in++='\0';
		*index=in;
		pos=strchr(in,']');
		if (pos) {
			*pos++='\0';
			if (*pos == '.') {
				*pos++='\0';
			}
			return pos;
		}
		return NULL;
	}
	return NULL;
}

static void
osd_text_draw(struct osd_text *this, struct navit *navit, struct vehicle *v)
{
	struct point p, p2[4];
	char *str,*last,*next,*value,*absbegin;
	int do_draw = 0;
	struct attr attr, vehicle_attr, maxspeed_attr;
	struct navigation *nav = NULL;
	struct tracking *tracking = NULL;
	struct route *route = NULL;
	struct map *nav_map = NULL;
	struct map_rect *nav_mr = NULL;
	struct item *item;
	struct osd_text_item *oti;
	int offset,lines;
	int height=this->osd_item.font_size*13/256;
	int yspacing=height/2;
	int xspacing=height/4;

	vehicle_attr.u.vehicle=NULL;
	oti=this->items;
	str=NULL;

	while (oti) {

		item=NULL;
		value=NULL;

		if (oti->static_text) {
			value=g_strdup(oti->text);
		} else if (oti->section == attr_navigation) {
			if (navit && !nav)
				nav = navit_get_navigation(navit);
			if (nav && !nav_map)
				nav_map = navigation_get_map(nav);
			if (nav_map )
				nav_mr = map_rect_new(nav_map, NULL);
			if (nav_mr)
				item = map_rect_get_item(nav_mr);

			offset=oti->offset;
			while (item) {
				if (item->type == type_nav_none)
					item=map_rect_get_item(nav_mr);
				else if (!offset)
					break;
				else {
					offset--;
					item=map_rect_get_item(nav_mr);
				}
			}

			if (item) {
				dbg(1,"name %s\n", item_to_name(item->type));
				dbg(1,"type %s\n", attr_to_name(oti->attr_typ));
				if (item_attr_get(item, oti->attr_typ, &attr))
					value=osd_text_format_attr(&attr, oti->format);
			}
			if (nav_mr)
				map_rect_destroy(nav_mr);
		} else if (oti->section == attr_vehicle) {
			if (navit && !vehicle_attr.u.vehicle) {
				navit_get_attr(navit, attr_vehicle, &vehicle_attr, NULL);
			}
			if (vehicle_attr.u.vehicle) {
				if (vehicle_get_attr(vehicle_attr.u.vehicle, oti->attr_typ, &attr, NULL)) {
					value=osd_text_format_attr(&attr, oti->format);
				}
			}
		} else if (oti->section == attr_tracking) {
			if (navit) {
				tracking = navit_get_tracking(navit);
				route = navit_get_route(navit);
			}
			if (tracking) {
				item=tracking_get_current_item(tracking);
				if (item && (oti->attr_typ == attr_speed)) {
					double routespeed = -1;
					int *flags=tracking_get_current_flags(tracking);
					if (flags && (*flags & AF_SPEED_LIMIT) && tracking_get_attr(tracking, attr_maxspeed, &maxspeed_attr, NULL)) {
						routespeed = maxspeed_attr.u.num;
					}

					if (routespeed == -1) {
						struct vehicleprofile *prof=navit_get_vehicleprofile(navit);
						struct roadprofile *rprof=NULL;
						if (prof)
							rprof=vehicleprofile_get_roadprofile(prof, item->type);
						if (rprof) {
							routespeed=rprof->speed;
						}
					}

					value = format_speed(routespeed,"", oti->format);
				} else if (item) {
					if (tracking_get_attr(tracking, oti->attr_typ, &attr, NULL))
						value=osd_text_format_attr(&attr, oti->format);
				}
			}

		} else if (oti->section == attr_navit) {
			if (oti->attr_typ == attr_message) {
				struct message *msg;
				int len,offset;
				char *tmp;

				msg = navit_get_messages(navit);
				len = 0;
				while (msg) {
					len+= strlen(msg->text) + 2;

					msg = msg->next;
				}

				value = g_malloc(len +1);

				msg = navit_get_messages(navit);
				offset = 0;
				while (msg) {
					tmp = g_stpcpy((value+offset), msg->text);
					g_stpcpy(tmp, "\\n");
					offset += strlen(msg->text) + 2;

					msg = msg->next;
				}

				value[len] = '\0';
			}
		}

		next=g_strdup_printf("%s%s",str ? str:"",value ? value:" ");
		if (value)
			g_free(value);
		if (str)
			g_free(str);
		str=next;
		oti=oti->next;
	}

	if ( this->last && str && !strcmp(this->last, str) ) {
		do_draw=0;
	} else {
		do_draw=1;
		if (this->last)
			g_free(this->last);
		this->last = g_strdup(str);
	}

	absbegin=str;
	if (do_draw) {
		osd_std_draw(&this->osd_item);
	}
	if (do_draw && str) {
		lines=0;
		next=str;
		last=str;
		while ((next=strstr(next, "\\n"))) {
			last = next;
			lines++;
			next++;
		}

		while (*last) {
			if (! g_ascii_isspace(*last)) {
				lines++;
				break;
			}
			last++;
		}

		dbg(1,"this->align=%d\n", this->align);
		switch (this->align & 51) {
		case 1:
			p.y=0;
			break;
		case 2:
			p.y=(this->osd_item.h-lines*(height+yspacing)-yspacing);
			break;
		case 16: // Grow from top to bottom
			p.y = 0;
			if (lines != 0) {
				this->osd_item.h = (lines-1) * (height+yspacing) + height;
			} else {
				this->osd_item.h = 0;
			}

			if (do_draw) {
				osd_std_resize(&this->osd_item);
			}
		default:
			p.y=(this->osd_item.h-lines*(height+yspacing)-yspacing)/2;
		}

		while (str) {
			next=strstr(str, "\\n");
			if (next) {
				*next='\0';
				next+=2;
			}
			graphics_get_text_bbox(this->osd_item.gr,
					       this->osd_item.font,
					       str, 0x10000,
					       0x0, p2, 0);
			switch (this->align & 12) {
			case 4:
				p.x=xspacing;
				break;
			case 8:
				p.x=this->osd_item.w-(p2[2].x-p2[0].x)-xspacing;
				break;
			default:
				p.x = ((p2[0].x - p2[2].x) / 2) + (this->osd_item.w / 2);
			}
			p.y += height+yspacing;
			graphics_draw_text(this->osd_item.gr,
					   this->osd_item.graphic_fg_text,
					   NULL, this->osd_item.font,
					   str, &p, 0x10000,
					   0);
			str=next;
		}
	}
	if(do_draw) {
		graphics_draw_mode(this->osd_item.gr, draw_mode_end);
	}
	g_free(absbegin);

}

/**
 * @brief Create a new osd_text_item and insert it into a linked list
 * 
 * @param parent Pointer to the preceding osd_text_item structure in the list. If NULL, the new osd_text_item becomes the root element of a new list.
 * @returns A pointer to the new osd_text_item element.
 */
struct osd_text_item *
oti_new(struct osd_text_item * parent)
{
    struct osd_text_item *this;
    this=g_new0(struct osd_text_item, 1);
    this->prev=parent;

    if(!parent) {
        this->root=this;
    } else {
        parent->next=this;
        this->root=parent->root;
    }

    return this;
}

/**
 * @brief Prepare a text type OSD element
 *
 * Parses the label string (as specified in the XML file) for a text type OSD element into attributes and static text. 
 * 
 * @param this Pointer to an osd_text structure representing the OSD element. The items member of this structure will receive a pointer to a list of osd_text_item structures.
 * @param nav Pointer to a navit structure
 * @returns nothing
 */
static void
osd_text_prepare(struct osd_text *this, struct navit *nav)
{
	char *absbegin,*str,*start,*end,*key,*subkey,*index;
	struct osd_text_item *oti;

	oti=NULL;
	str=g_strdup(this->text);
	absbegin=str;

	while ((start=strstr(str, "${"))) {

		*start='\0';
		start+=2;

		// find plain text before
		if (start!=str) {
			oti = oti_new(oti);
			oti->static_text=1;
			oti->text=g_strdup(str);

		}

		end=strstr(start,"}");
		if (! end)
			break;

		*end++='\0';
		key=start;

		subkey=osd_text_split(key,NULL);

	    oti = oti_new(oti);
		oti->section=attr_from_name(key);

		if (( oti->section == attr_navigation ||
				oti->section == attr_tracking) && subkey) {
		    key=osd_text_split(subkey,&index);

			if (index)
				oti->offset=atoi(index);

			subkey=osd_text_split(key,&index);

			if (!strcmp(key,"route_speed")) {
				oti->attr_typ=attr_speed;
			} else {
				oti->attr_typ=attr_from_name(key);
			}
			oti->format = g_strdup(index);

		} else if ((oti->section == attr_vehicle || oti->section == attr_navit) && subkey) {
			key=osd_text_split(subkey,&index);
			if (!strcmp(subkey,"messages")) {
				oti->attr_typ=attr_message;
			} else {
				oti->attr_typ=attr_from_name(subkey);
			}
			oti->format = g_strdup(index);
		}

		switch(oti->attr_typ) {
			default:
				navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_text_draw), attr_position_coord_geo, this));
				break;
		}

		str=(end);
	}

	if(*str!='\0'){
		oti = oti_new(oti);
		oti->static_text=1;
		oti->text=g_strdup(str);
	}

	if (oti)
		this->items=oti->root;
	else
		this->items=NULL;

	g_free(absbegin);

}

static void
osd_text_init(struct osd_text *this, struct navit *nav)
{

	osd_set_std_graphic(nav, &this->osd_item, (struct osd_priv *)this);
	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_std_click), attr_button, &this->osd_item));
	osd_text_prepare(this,nav);
	osd_text_draw(this, nav, NULL);

}

static struct osd_priv *
osd_text_new(struct navit *nav, struct osd_methods *meth,
	    struct attr **attrs)
{
	struct osd_text *this = g_new0(struct osd_text, 1);
	struct attr *attr;

	this->osd_item.p.x = -80;
	this->osd_item.p.y = 20;
	this->osd_item.w = 60;
	this->osd_item.h = 20;
	this->osd_item.navit = nav;
	this->osd_item.font_size = 200;
	this->osd_item.meth.draw = osd_draw_cast(osd_text_draw);
	osd_set_std_attr(attrs, &this->osd_item, 2);

	this->active = -1;
	this->last = NULL;

	attr = attr_search(attrs, NULL, attr_label);
	if (attr)
		this->text = g_strdup(attr->u.str);
	else
		this->text = NULL;
	attr = attr_search(attrs, NULL, attr_align);
	if (attr)
		this->align=attr->u.num;

	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_text_init), attr_graphics_ready, this));
	return (struct osd_priv *) this;
}

struct gps_status {
	struct osd_item osd_item;
	char *icon_src;
	int icon_h, icon_w, active;
	int strength;
};

static void
osd_gps_status_draw(struct gps_status *this, struct navit *navit,
		       struct vehicle *v)
{
	struct point p;
	int do_draw = 0;
	struct graphics_image *gr_image;
	char *image;
	struct attr attr, vehicle_attr;
	int strength=-1;

	if (navit && navit_get_attr(navit, attr_vehicle, &vehicle_attr, NULL)) {
		if (vehicle_get_attr(vehicle_attr.u.vehicle, attr_position_fix_type, &attr, NULL)) {
			switch(attr.u.num) {
			case 1:
			case 2:
				strength=2;
				if (vehicle_get_attr(vehicle_attr.u.vehicle, attr_position_sats_used, &attr, NULL)) {
					dbg(1,"num=%d\n", attr.u.num);
					if (attr.u.num >= 3) 
						strength=attr.u.num-1;
					if (strength > 5)
						strength=5;
					if (strength > 3) {
						if (vehicle_get_attr(vehicle_attr.u.vehicle, attr_position_hdop, &attr, NULL)) {
							if (*attr.u.numd > 2.0 && strength > 4)
								strength=4;
							if (*attr.u.numd > 4.0 && strength > 3)
								strength=3;
						}
					}
				}
				break;
			default:
				strength=1;
			}
		}
	}	
	if (this->strength != strength) {
		this->strength=strength;
		do_draw=1;
	}
	if (do_draw) {
		osd_std_draw(&this->osd_item);
		if (this->active) {
			image = g_strdup_printf(this->icon_src, strength);
			gr_image = graphics_image_new_scaled(this->osd_item.gr, image, this->icon_w, this->icon_h);
			if (gr_image) {
				p.x = (this->osd_item.w - gr_image->width) / 2;
				p.y = (this->osd_item.h - gr_image->height) / 2;
				graphics_draw_image(this->osd_item.gr, this->osd_item.  graphic_fg_white, &p, gr_image);
				graphics_image_free(this->osd_item.gr, gr_image);
			}
			g_free(image);
		}
		graphics_draw_mode(this->osd_item.gr, draw_mode_end);
	}
}

static void
osd_gps_status_init(struct gps_status *this, struct navit *nav)
{
	osd_set_std_graphic(nav, &this->osd_item, (struct osd_priv *)this);
	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_gps_status_draw), attr_position_coord_geo, this));
	osd_gps_status_draw(this, nav, NULL);
}

static struct osd_priv *
osd_gps_status_new(struct navit *nav, struct osd_methods *meth,
		      struct attr **attrs)
{
	struct gps_status *this = g_new0(struct gps_status, 1);
	struct attr *attr;

	this->osd_item.p.x = 20;
	this->osd_item.p.y = -80;
	this->osd_item.w = 60;
	this->osd_item.navit = nav;
	this->osd_item.h = 40;
	this->osd_item.font_size = 200;
	this->osd_item.meth.draw = osd_draw_cast(osd_gps_status_draw);
	osd_set_std_attr(attrs, &this->osd_item, 0);

	this->icon_w = -1;
	this->icon_h = -1;
	this->active = -1;
	this->strength = -2;

	attr = attr_search(attrs, NULL, attr_icon_w);
	if (attr)
		this->icon_w = attr->u.num;

	attr = attr_search(attrs, NULL, attr_icon_h);
	if (attr)
		this->icon_h = attr->u.num;

	attr = attr_search(attrs, NULL, attr_icon_src);
	if (attr) {
		struct file_wordexp *we;
		char **array;
		we = file_wordexp_new(attr->u.str);
		array = file_wordexp_get_array(we);
		this->icon_src = g_strdup(array[0]);
		file_wordexp_destroy(we);
	} else
		this->icon_src = graphics_icon_path("gui_strength_%d_32_32.png");

	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_gps_status_init), attr_graphics_ready, this));
	return (struct osd_priv *) this;
}


struct volume {
	struct osd_item osd_item;
	char *icon_src;
	int icon_h, icon_w, active;
	int strength;
	struct callback *click_cb;
};

static void
osd_volume_draw(struct volume *this, struct navit *navit)
{
	struct point p;
	struct graphics_image *gr_image;
	char *image;

	osd_std_draw(&this->osd_item);
	if (this->active) {
		image = g_strdup_printf(this->icon_src, this->strength);
		gr_image = graphics_image_new_scaled(this->osd_item.gr, image, this->icon_w, this->icon_h);
		if (gr_image) {
			p.x = (this->osd_item.w - gr_image->width) / 2;
			p.y = (this->osd_item.h - gr_image->height) / 2;
			graphics_draw_image(this->osd_item.gr, this->osd_item.  graphic_fg_white, &p, gr_image);
			graphics_image_free(this->osd_item.gr, gr_image);
		}
		g_free(image);
	}
	graphics_draw_mode(this->osd_item.gr, draw_mode_end);
}

static void
osd_volume_click(struct volume *this, struct navit *nav, int pressed, int button, struct point *p)
{
	struct point bp = this->osd_item.p;
	osd_wrap_point(&bp, nav);
	if ((p->x < bp.x || p->y < bp.y || p->x > bp.x + this->osd_item.w || p->y > bp.y + this->osd_item.h) && !this->osd_item.pressed)
		return;
	navit_ignore_button(nav);
	if (pressed) {
		if (p->y - bp.y < this->osd_item.h/2)
			this->strength++;
		else
			this->strength--;
		if (this->strength < 0)
			this->strength=0;
		if (this->strength > 5)
			this->strength=5;
		osd_volume_draw(this, nav);
	}
}
static void
osd_volume_init(struct volume *this, struct navit *nav)
{
	osd_set_std_graphic(nav, &this->osd_item, (struct osd_priv *)this);
	navit_add_callback(nav, this->click_cb = callback_new_attr_1(callback_cast (osd_volume_click), attr_button, this));
	osd_volume_draw(this, nav);
}

static struct osd_priv *
osd_volume_new(struct navit *nav, struct osd_methods *meth,
		      struct attr **attrs)
{
	struct volume *this = g_new0(struct volume, 1);
	struct attr *attr;

	this->osd_item.p.x = 20;
	this->osd_item.p.y = -80;
	this->osd_item.w = 60;
	this->osd_item.navit = nav;
	this->osd_item.h = 40;
	this->osd_item.font_size = 200;
	this->osd_item.meth.draw = osd_draw_cast(osd_volume_draw);
	osd_set_std_attr(attrs, &this->osd_item, 0);

	this->icon_w = -1;
	this->icon_h = -1;
	this->active = -1;
	this->strength = -1;

	attr = attr_search(attrs, NULL, attr_icon_w);
	if (attr)
		this->icon_w = attr->u.num;

	attr = attr_search(attrs, NULL, attr_icon_h);
	if (attr)
		this->icon_h = attr->u.num;

	attr = attr_search(attrs, NULL, attr_icon_src);
	if (attr) {
		struct file_wordexp *we;
		char **array;
		we = file_wordexp_new(attr->u.str);
		array = file_wordexp_get_array(we);
		this->icon_src = g_strdup(array[0]);
		file_wordexp_destroy(we);
	} else
		this->icon_src = graphics_icon_path("gui_strength_%d_32_32.png");

	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_volume_init), attr_graphics_ready, this));
	return (struct osd_priv *) this;
}

struct osd_scale {
	int use_overlay;
	struct osd_item item;
	struct callback *draw_cb,*navit_init_cb;
	struct graphics_gc *black;
};

static void
osd_scale_draw(struct osd_scale *this, struct navit *nav)
{
	struct point bp,bp1,bp2;
	struct point p[10],bbox[4];
	struct coord c[2];
	struct attr transformation;
	int len;
	double dist,exp,base,man;
	char *text;
	int w=this->item.w*9/10;
	int o=(this->item.w-w)/2;

	if (!navit_get_attr(nav, attr_transformation, &transformation, NULL))
		return;
	if (this->use_overlay) {
		graphics_draw_mode(this->item.gr, draw_mode_begin);
		bp.x=0;
		bp.y=0;
		graphics_draw_rectangle(this->item.gr, this->item.graphic_bg, &bp, this->item.w, this->item.h);
	} else {
		bp=this->item.p;
		osd_wrap_point(&bp, nav);
	}
	bp1=bp;
	bp1.y+=this->item.h/2;
	bp1.x+=o;
	bp2=bp1;
	bp2.x+=w;
	p[0]=bp1;
	p[1]=bp2;
	transform_reverse(transformation.u.transformation, &p[0], &c[0]);
	transform_reverse(transformation.u.transformation, &p[1], &c[1]);
	dist=transform_distance(transform_get_projection(transformation.u.transformation), &c[0], &c[1]);
	exp=floor(log10(dist));
	base=pow(10,exp);
	man=dist/base;
	if (man >= 5)
		man=5;
	else if (man >= 2)
		man=2;
	else
		man=1;
	len=this->item.w-man*base/dist*w;
	p[0].x+=len/2;
	p[1].x-=len/2;
	p[2]=p[0];
	p[3]=p[0];
	p[2].y-=this->item.h/10;
	p[3].y+=this->item.h/10;
	p[4]=p[1];
	p[5]=p[1];
	p[4].y-=this->item.h/10;
	p[5].y+=this->item.h/10;
	p[6]=p[2];
	p[6].x-=2;
	p[6].y-=2;
	p[7]=p[0];
	p[7].y-=2;
	p[8]=p[4];
	p[8].x-=2;
	p[8].y-=2;
	graphics_draw_rectangle(this->item.gr, this->item.graphic_fg_white, p+6, 4,this->item.h/5+4);
	graphics_draw_rectangle(this->item.gr, this->item.graphic_fg_white, p+7, p[1].x-p[0].x, 4);
	graphics_draw_rectangle(this->item.gr, this->item.graphic_fg_white, p+8, 4,this->item.h/5+4);
	graphics_draw_lines(this->item.gr, this->black, p, 2);
	graphics_draw_lines(this->item.gr, this->black, p+2, 2);
	graphics_draw_lines(this->item.gr, this->black, p+4, 2);
	text=format_distance(man*base, "");
	graphics_get_text_bbox(this->item.gr, this->item.font, text, 0x10000, 0, bbox, 0);
	p[0].x=(this->item.w-bbox[2].x)/2+bp.x;
	p[0].y=bp.y+this->item.h-this->item.h/10;
	graphics_draw_text(this->item.gr, this->black, this->item.graphic_fg_white, this->item.font, text, &p[0], 0x10000, 0);
	g_free(text);
	if (this->use_overlay)
		graphics_draw_mode(this->item.gr, draw_mode_end);
}

static void
osd_scale_init(struct osd_scale *this, struct navit *nav)
{
	struct color color_white={0xffff,0xffff,0xffff,0x0000};
	struct color color_black={0x0000,0x0000,0x0000,0x0000};
	struct graphics *gra = navit_get_graphics(nav);
	dbg(1, "enter\n");
	if (this->use_overlay) {
		osd_set_std_graphic(nav, &this->item, (struct osd_priv *)this);
	} else {
		this->item.configured=1;
		this->item.gr=gra;
		this->item.font = graphics_font_new(this->item.gr, this->item.font_size, 1);
		this->item.graphic_fg_white=graphics_gc_new(this->item.gr);
		this->item.color_white=color_white;
		graphics_gc_set_foreground(this->item.graphic_fg_white, &this->item.color_white);
	}
	this->black=graphics_gc_new(this->item.gr);
	graphics_gc_set_foreground(this->black, &color_black);
	graphics_add_callback(gra, this->draw_cb=callback_new_attr_2(callback_cast(osd_scale_draw), attr_postdraw, this, nav));
	if (navit_get_ready(nav) == 3)
		osd_scale_draw(this, nav);
}

static struct osd_priv *
osd_scale_new(struct navit *nav, struct osd_methods *meth,
	       struct attr **attrs)
{
	struct osd_scale *this = g_new0(struct osd_scale, 1);
	struct attr *attr;

	this->item.navit = nav;
	this->item.meth.draw = osd_draw_cast(osd_scale_draw);

	osd_set_std_attr(attrs, &this->item, 3);

	attr=attr_search(attrs, NULL, attr_use_overlay);
	if (attr)
		this->use_overlay=attr->u.num;

	navit_add_callback(nav, this->navit_init_cb = callback_new_attr_1(callback_cast (osd_scale_init), attr_graphics_ready, this));

	return (struct osd_priv *) this;
}

struct auxmap {
	struct osd_item osd_item;
	struct displaylist *displaylist;
	struct transformation *ntrans;
	struct transformation *trans;
	struct layout *layout;
	struct callback *postdraw_cb;
	struct graphics_gc *red;
	struct navit *nav;
};

static void
osd_auxmap_draw(struct auxmap *this)
{
	int d=10;
	struct point p;
	struct attr mapset;

	if (!this->osd_item.configured)
		return;
	if (!navit_get_attr(this->nav, attr_mapset, &mapset, NULL) || !mapset.u.mapset)
		return;
	p.x=this->osd_item.w/2;
	p.y=this->osd_item.h/2;
	transform_set_center(this->trans, transform_get_center(this->ntrans));
	transform_set_scale(this->trans, 64);
	transform_set_yaw(this->trans, transform_get_yaw(this->ntrans));
	transform_setup_source_rect(this->trans);
	transform_set_projection(this->trans, transform_get_projection(this->ntrans));
#if 0
	graphics_displaylist_draw(this->osd_item.gr, this->displaylist, this->trans, this->layout, 4);
#endif
	graphics_draw(this->osd_item.gr, this->displaylist, mapset.u.mapset, this->trans, this->layout, 0, NULL, 1);
	graphics_draw_circle(this->osd_item.gr, this->red, &p, d);
	graphics_draw_mode(this->osd_item.gr, draw_mode_end);

}

static void
osd_auxmap_init(struct auxmap *this, struct navit *nav)
{
	struct graphics *gra;
	struct attr attr;
	struct map_selection sel;
	struct color red={0xffff,0x0,0x0,0xffff};

	this->nav=nav;
	if (! navit_get_attr(nav, attr_graphics, &attr, NULL))
		return;
	gra=attr.u.graphics;
	graphics_add_callback(gra, callback_new_attr_1(callback_cast(osd_auxmap_draw), attr_postdraw, this));
	if (! navit_get_attr(nav, attr_transformation, &attr, NULL))
		return;
	this->ntrans=attr.u.transformation;
	if (! navit_get_attr(nav, attr_displaylist, &attr, NULL))
		return;
	this->displaylist=attr.u.displaylist;
	if (! navit_get_attr(nav, attr_layout, &attr, NULL))
		return;
	this->layout=attr.u.layout;
	osd_set_std_graphic(nav, &this->osd_item, NULL);
	graphics_init(this->osd_item.gr);
	this->red=graphics_gc_new(gra);
	graphics_gc_set_foreground(this->red,&red);
	graphics_gc_set_linewidth(this->red,3);
	this->trans=transform_new();
	memset(&sel, 0, sizeof(sel));
	sel.u.p_rect.rl.x=this->osd_item.w;
	sel.u.p_rect.rl.y=this->osd_item.h;
	transform_set_screen_selection(this->trans, &sel);
        graphics_set_rect(this->osd_item.gr, &sel.u.p_rect);
#if 0
	osd_auxmap_draw(this, nav);
#endif
}

static struct osd_priv *
osd_auxmap_new(struct navit *nav, struct osd_methods *meth, struct attr **attrs)
{
	struct auxmap *this = g_new0(struct auxmap, 1);

	this->osd_item.p.x = 20;
	this->osd_item.p.y = -80;
	this->osd_item.w = 60;
	this->osd_item.h = 40;
	this->osd_item.font_size = 200;
	osd_set_std_attr(attrs, &this->osd_item, 0);

	navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_auxmap_init), attr_navit, this));
	return (struct osd_priv *) this;
}


void
plugin_init(void)
{
	plugin_register_osd_type("compass", osd_compass_new);
	plugin_register_osd_type("navigation_next_turn", osd_nav_next_turn_new);
	plugin_register_osd_type("button", osd_button_new);
    	plugin_register_osd_type("toggle_announcer", osd_nav_toggle_announcer_new);
    	plugin_register_osd_type("speed_warner", osd_speed_warner_new);
    	plugin_register_osd_type("speed_cam", osd_speed_cam_new);
    	plugin_register_osd_type("text", osd_text_new);
    	plugin_register_osd_type("gps_status", osd_gps_status_new);
    	plugin_register_osd_type("volume", osd_volume_new);
    	plugin_register_osd_type("scale", osd_scale_new);
	plugin_register_osd_type("image", osd_image_new);
	plugin_register_osd_type("stopwatch", osd_stopwatch_new);
	plugin_register_osd_type("odometer", osd_odometer_new);
	plugin_register_osd_type("auxmap", osd_auxmap_new);
}
