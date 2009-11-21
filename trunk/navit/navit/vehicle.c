/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2009 Navit Team
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

#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <time.h>
#include "config.h"
#include "debug.h"
#include "coord.h"
#include "item.h"
#include "log.h"
#include "plugin.h"
#include "transform.h"
#include "util.h"
#include "event.h"
#include "coord.h"
#include "transform.h"
#include "projection.h"
#include "point.h"
#include "graphics.h"
#include "callback.h"
#include "color.h"
#include "layout.h"
#include "vehicle.h"

struct vehicle {
	struct vehicle_methods meth;
	struct vehicle_priv *priv;
	struct callback_list *cbl;
	struct log *nmea_log, *gpx_log;
	struct attr **attrs;

	// cursor
	struct cursor *cursor;
	struct callback *animate_callback;
	struct event_timeout *animate_timer;
	struct point cursor_pnt;
	struct graphics *gra;
	struct graphics_gc *bg;
	struct transformation *trans;
	int angle;
	int speed;
	int sequence;
};

static void vehicle_draw_do(struct vehicle *this_, int lazy);
static void vehicle_log_nmea(struct vehicle *this_, struct log *log);
static void vehicle_log_gpx(struct vehicle *this_, struct log *log);
static void vehicle_log_textfile(struct vehicle *this_, struct log *log);
static void vehicle_log_binfile(struct vehicle *this_, struct log *log);
static int vehicle_add_log(struct vehicle *this_, struct log *log);



/**
 * Creates a new vehicle
 */
struct vehicle *
vehicle_new(struct attr *parent, struct attr **attrs)
{
	struct vehicle *this_;
	struct attr *source;
	struct vehicle_priv *(*vehicletype_new) (struct vehicle_methods *
						 meth,
						 struct callback_list *
						 cbl,
						 struct attr ** attrs);
	char *type, *colon;
	struct pcoord center;

	dbg(1, "enter\n");
	source = attr_search(attrs, NULL, attr_source);
	if (!source) {
		dbg(0, "no source\n");
		return NULL;
	}

	type = g_strdup(source->u.str);
	colon = strchr(type, ':');
	if (colon)
		*colon = '\0';
	dbg(1, "source='%s' type='%s'\n", source->u.str, type);

	vehicletype_new = plugin_get_vehicle_type(type);
	if (!vehicletype_new) {
		dbg(0, "invalid type '%s'\n", type);
		return NULL;
	}
	this_ = g_new0(struct vehicle, 1);
	this_->cbl = callback_list_new();
	this_->priv = vehicletype_new(&this_->meth, this_->cbl, attrs);
	if (!this_->priv) {
		dbg(0, "vehicletype_new failed\n");
		callback_list_destroy(this_->cbl);
		g_free(this_);
		return NULL;
	}
	this_->attrs=attr_list_dup(attrs);

	this_->trans=transform_new();
	center.pro=projection_screen;
	center.x=0;
	center.y=0;
	transform_setup(this_->trans, &center, 16, 0);

	dbg(1, "leave\n");
	return this_;
}

/**
 * Destroys a vehicle
 * 
 * @param this_ The vehicle to destroy
 */
void
vehicle_destroy(struct vehicle *this_)
{
	if (this_->animate_callback) {
		callback_destroy(this_->animate_callback);
		event_remove_timeout(this_->animate_timer);
	}
	transform_destroy(this_->trans);
	this_->meth.destroy(this_->priv);
	callback_list_destroy(this_->cbl);
	attr_list_free(this_->attrs);
	g_free(this_);
}

/**
 * Creates an attribute iterator to be used with vehicles
 */
struct attr_iter *
vehicle_attr_iter_new(void)
{
	return (struct attr_iter *)g_new0(void *,1);
}

/**
 * Destroys a vehicle attribute iterator
 *
 * @param iter a vehicle attr_iter
 */
void
vehicle_attr_iter_destroy(struct attr_iter *iter)
{
	g_free(iter);
}



/**
 * Generic get function
 *
 * @param this_ A vehicle
 * @param type The attribute type to look for
 * @param attr A struct attr to store the attribute
 * @param iter A vehicle attr_iter
 */
int
vehicle_get_attr(struct vehicle *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter)
{
	int ret;
	if (this_->meth.position_attr_get) {
		ret=this_->meth.position_attr_get(this_->priv, type, attr);
		if (ret)
			return ret;
	}
	return attr_generic_get_attr(this_->attrs, NULL, type, attr, iter);
}

/**
 * Generic set function
 *
 * @param this_ A vehicle
 * @param attr
 * @param attrs
 */
int
vehicle_set_attr(struct vehicle *this_, struct attr *attr)
{
	int ret=1;
	if (this_->meth.set_attr)
		ret=this_->meth.set_attr(this_->priv, attr);
	if (ret == 1 && attr->type != attr_navit)
		this_->attrs=attr_generic_set_attr(this_->attrs, attr);
	return ret != 0;
}

/**
 * Generic add function
 *
 * @param this_ A vehicle
 * @param attr A struct attr
 */
int
vehicle_add_attr(struct vehicle *this_, struct attr *attr)
{
	int ret=1;
	switch (attr->type) {
	case attr_callback:
		callback_list_add(this_->cbl, attr->u.callback);
		break;
	case attr_log:
		ret=vehicle_add_log(this_, attr->u.log);
		break;
	// currently supporting oldstyle cursor config.
	case attr_cursor:
		vehicle_set_cursor(this_, attr->u.cursor);
		break;
	default:
		break;
	}
	if (ret)
		this_->attrs=attr_generic_add_attr(this_->attrs, attr);
	return ret;
}

/**
 * @brief Generic remove function.
 *
 * Used to remove a callback from the vehicle.
 * @param this_ A vehicle
 * @param attr
 */
int
vehicle_remove_attr(struct vehicle *this_, struct attr *attr)
{
	switch (attr->type) {
	case attr_callback:
		callback_list_remove(this_->cbl, attr->u.callback);
		break;
	default:
		return 0;
	}
	return 1;
}



/**
 * Sets the cursor of a vehicle.
 *
 * @param this_ A vehicle
 * @param cursor A cursor
 * @author Ralph Sennhauser (10/2009)
 */ 
void
vehicle_set_cursor(struct vehicle *this_, struct cursor *cursor)
{
	if (!cursor) {	// we require this for now.
		return;
	}

	if (this_->animate_callback) {
		event_remove_timeout(this_->animate_timer);
		this_->animate_timer=NULL;		// dangling pointer! prevent double freeing.
		callback_destroy(this_->animate_callback);
		this_->animate_callback=NULL;	// dangling pointer! prevent double freeing.
	}
	if (cursor->interval) {
		this_->animate_callback=callback_new_2(callback_cast(vehicle_draw_do), this_, 0);
		this_->animate_timer=event_add_timeout(cursor->interval, 1, this_->animate_callback);
	}

	if (this_->gra) {
		this_->cursor_pnt.x+=(this_->cursor->w - cursor->w)/2;
		this_->cursor_pnt.y+=(this_->cursor->h - cursor->h)/2;
		graphics_overlay_resize(this_->gra, &this_->cursor_pnt, cursor->w, cursor->h, 65535, 0);
	}

	struct point sc;
	sc.x=cursor->w/2;
	sc.y=cursor->h/2;
	transform_set_screen_center(this_->trans, &sc);

	this_->cursor=cursor;
}

/**
 * Draws a vehicle on top of a graphics.
 *
 * @param this_ The vehicle
 * @param gra The graphics
 * @param pnt Screen coordinates of the vehicle.
 * @param lazy use lazy draw mode.
 * @param angle The angle relative to the map.
 * @param speed The speed of the vehicle.
 */
void
vehicle_draw(struct vehicle *this_, struct graphics *gra, struct point *pnt, int lazy, int angle, int speed)
{
	if (angle < 0)
		angle+=360;
	dbg(1,"enter this=%p gra=%p pnt=%p lazy=%d dir=%d speed=%d\n", this_, gra, pnt, lazy, angle, speed);
	dbg(1,"point %d,%d\n", pnt->x, pnt->y);
	if (!this_->cursor)
		return;
	this_->cursor_pnt=*pnt;
	this_->cursor_pnt.x-=this_->cursor->w/2;
	this_->cursor_pnt.y-=this_->cursor->h/2;
	this_->angle=angle;
	this_->speed=speed;
	if (!this_->gra) {
		struct color c;
		this_->gra=graphics_overlay_new(gra, &this_->cursor_pnt, this_->cursor->w, this_->cursor->h, 65535, 0);
		if (this_->gra) {
			this_->bg=graphics_gc_new(this_->gra);
			c.r=0; c.g=0; c.b=0; c.a=0;
			graphics_gc_set_foreground(this_->bg, &c);
			graphics_background_gc(this_->gra, this_->bg);
		}
	}
	vehicle_draw_do(this_, lazy);
}



static void
vehicle_draw_do(struct vehicle *this_, int lazy)
{
	if (!this_->cursor || !this_->cursor->attrs || !this_->gra)
		return;

	struct point p;
	struct cursor *cursor=this_->cursor;
	int speed=this_->speed;
	int angle=this_->angle;
	int sequence=this_->sequence;

	transform_set_yaw(this_->trans, -this_->angle);
	graphics_draw_mode(this_->gra, draw_mode_begin);
	p.x=0;
	p.y=0;
	graphics_draw_rectangle(this_->gra, this_->bg, &p, cursor->w, cursor->h);
	struct attr **attr=cursor->attrs;
	int match=0;
	while (*attr) {
		if ((*attr)->type == attr_itemgra) {
			struct itemgra *itm=(*attr)->u.itemgra;
			dbg(1,"speed %d-%d %d\n", itm->speed_range.min, itm->speed_range.max, speed);
			if (speed >= itm->speed_range.min && speed <= itm->speed_range.max &&  
			    angle >= itm->angle_range.min && angle <= itm->angle_range.max &&  
			    sequence >= itm->sequence_range.min && sequence <= itm->sequence_range.max) {
				graphics_draw_itemgra(this_->gra, itm, this_->trans);
			}
			if (sequence < itm->sequence_range.max)
				match=1;
		}
		++attr;
	}
	graphics_draw_drag(this_->gra, &this_->cursor_pnt);
	graphics_draw_mode(this_->gra, lazy ? draw_mode_end_lazy : draw_mode_end);
	if (this_->animate_callback) {
		++this_->sequence;
		if (cursor->sequence_range && cursor->sequence_range->max < this_->sequence)
			this_->sequence=cursor->sequence_range->min;
		if (! match && ! cursor->sequence_range)
			this_->sequence=0;
	}
}

static void
vehicle_log_nmea(struct vehicle *this_, struct log *log)
{
	struct attr pos_attr;
	if (!this_->meth.position_attr_get)
		return;
	if (!this_->meth.position_attr_get(this_->priv, attr_position_nmea, &pos_attr))
		return;
	log_write(log, pos_attr.u.str, strlen(pos_attr.u.str), 0);
}

void
vehicle_log_gpx_add_tag(char *tag, char **logstr)
{
	char *ext_start="\t<extensions>\n";
	char *ext_end="\t</extensions>\n";
	char *trkpt_end="</trkpt>";
	char *start=NULL,*end=NULL;
	if (!*logstr) {
		start=g_strdup(ext_start);
		end=g_strdup(ext_end);
	} else {
		char *str=strstr(*logstr, ext_start);
		int len;
		if (str) {
			len=str-*logstr+strlen(ext_start);
			start=g_strdup(*logstr);
			start[len]='\0';
			end=g_strdup(str+strlen(ext_start));
		} else {
			str=strstr(*logstr, trkpt_end);
			len=str-*logstr;
			end=g_strdup_printf("%s%s",ext_end,str);
			str=g_strdup(*logstr);
			str[len]='\0';
			start=g_strdup_printf("%s%s",str,ext_start);
			g_free(str);
		}
	}
	*logstr=g_strdup_printf("%s%s%s",start,tag,end);
	g_free(start);
	g_free(end);
}

static void
vehicle_log_gpx(struct vehicle *this_, struct log *log)
{
	struct attr attr,*attrp;
	enum attr_type *attr_types;
	char *logstr;
	char *extensions="\t<extensions>\n";

	if (!this_->meth.position_attr_get)
		return;
	if (log_get_attr(log, attr_attr_types, &attr, NULL))
		attr_types=attr.u.attr_types;
	else
		attr_types=NULL;
	if (!this_->meth.position_attr_get(this_->priv, attr_position_coord_geo, &attr))
		return;
	logstr=g_strdup_printf("<trkpt lat=\"%f\" lon=\"%f\">\n",attr.u.coord_geo->lat,attr.u.coord_geo->lng);
	if (attr_types && attr_types_contains_default(attr_types, attr_position_time_iso8601, 0)) {
		if (this_->meth.position_attr_get(this_->priv, attr_position_time_iso8601, &attr)) {
			logstr=g_strconcat_printf(logstr,"\t<time>%s</time>\n",attr.u.str);
		} else {
			char *timep = current_to_iso8601();
			logstr=g_strconcat_printf(logstr,"\t<time>%s</time>\n",timep);
			g_free(timep);
		}
	}
	if (attr_types_contains_default(attr_types, attr_position_direction,0) && this_->meth.position_attr_get(this_->priv, attr_position_direction, &attr))
		logstr=g_strconcat_printf(logstr,"\t<course>%.1f</course>\n",*attr.u.numd);
	if (attr_types_contains_default(attr_types, attr_position_speed, 0) && this_->meth.position_attr_get(this_->priv, attr_position_speed, &attr))
		logstr=g_strconcat_printf(logstr,"\t<speed>%.2f</speed>\n",*attr.u.numd);
	if (attr_types_contains_default(attr_types, attr_profilename, 0) && (attrp=attr_search(this_->attrs, NULL, attr_profilename))) {
		logstr=g_strconcat_printf(logstr,"%s\t\t<navit:profilename>%s</navit:profilename>\n",extensions,attrp->u.str);
		extensions="";
	}
	if (attr_types_contains_default(attr_types, attr_position_radius, 0) && this_->meth.position_attr_get(this_->priv, attr_position_radius, &attr)) {
		logstr=g_strconcat_printf(logstr,"%s\t\t<navit:radius>%.2f</navit:radius>\n",extensions,*attr.u.numd);
		extensions="";
	}
	if (!strcmp(extensions,"")) {
		logstr=g_strconcat_printf(logstr,"\t</extensions>\n");
	}
	logstr=g_strconcat_printf(logstr,"</trkpt>\n");
	callback_list_call_attr_1(this_->cbl, attr_log_gpx, &logstr);
	log_write(log, logstr, strlen(logstr), 0);
	g_free(logstr);
}

static void
vehicle_log_textfile(struct vehicle *this_, struct log *log)
{
	struct attr pos_attr;
	char *logstr;
	if (!this_->meth.position_attr_get)
		return;
	if (!this_->meth.position_attr_get(this_->priv, attr_position_coord_geo, &pos_attr))
		return;
	logstr=g_strdup_printf("%f %f type=trackpoint\n", pos_attr.u.coord_geo->lng, pos_attr.u.coord_geo->lat);
	callback_list_call_attr_1(this_->cbl, attr_log_textfile, &logstr);
	log_write(log, logstr, strlen(logstr), 0);
}

static void
vehicle_log_binfile(struct vehicle *this_, struct log *log)
{
	struct attr pos_attr;
	int *buffer;
	int *buffer_new;
	int len,limit=1024,done=0,radius=25;
	struct coord c;
	enum log_flags flags;

	if (!this_->meth.position_attr_get)
		return;
	if (!this_->meth.position_attr_get(this_->priv, attr_position_coord_geo, &pos_attr))
		return;
	transform_from_geo(projection_mg, pos_attr.u.coord_geo, &c);
	if (!c.x || !c.y)
		return;
	while (!done) {
		buffer=log_get_buffer(log, &len);
		if (! buffer || !len) {
			buffer_new=g_malloc(5*sizeof(int));
			buffer_new[0]=2;
			buffer_new[1]=type_track;
			buffer_new[2]=0;
		} else {
			buffer_new=g_malloc((buffer[0]+3)*sizeof(int));
			memcpy(buffer_new, buffer, (buffer[0]+1)*sizeof(int));
		}
		dbg(1,"c=0x%x,0x%x\n",c.x,c.y);
		buffer_new[buffer_new[0]+1]=c.x;
		buffer_new[buffer_new[0]+2]=c.y;
		buffer_new[0]+=2;
		buffer_new[2]+=2;
		if (buffer_new[2] > limit) {
			int count=buffer_new[2]/2;
			struct coord out[count];
			struct coord *in=(struct coord *)(buffer_new+3);
			int count_out=transform_douglas_peucker(in, count, radius, out);
			memcpy(in, out, count_out*2*sizeof(int));
			buffer_new[0]+=(count_out-count)*2;	
			buffer_new[2]+=(count_out-count)*2;	
			flags=log_flag_replace_buffer|log_flag_force_flush|log_flag_truncate;
		} else {
			flags=log_flag_replace_buffer|log_flag_keep_pointer|log_flag_keep_buffer|log_flag_force_flush;
			done=1;
		}
		log_write(log, (char *)buffer_new, (buffer_new[0]+1)*sizeof(int), flags);
	}
}

static int
vehicle_add_log(struct vehicle *this_, struct log *log)
{
	struct callback *cb;
	struct attr type_attr;
	if (!log_get_attr(log, attr_type, &type_attr, NULL))
                return 1;

	if (!strcmp(type_attr.u.str, "nmea")) {
		cb=callback_new_attr_2(callback_cast(vehicle_log_nmea), attr_position_coord_geo, this_, log);
	} else if (!strcmp(type_attr.u.str, "gpx")) {
		char *header = "<?xml version='1.0' encoding='UTF-8'?>\n"
			"<gpx version='1.1' creator='Navit http://navit.sourceforge.net'\n"
			"     xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance'\n"
			"     xmlns:navit='http://www.navit-project.org/schema/navit'\n"
			"     xmlns='http://www.topografix.com/GPX/1/1'\n"
			"     xsi:schemaLocation='http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd'>\n"
			"<trk>\n"
			"<trkseg>\n";
		char *trailer = "</trkseg>\n</trk>\n</gpx>\n";
		log_set_header(log, header, strlen(header));
		log_set_trailer(log, trailer, strlen(trailer));
		cb=callback_new_attr_2(callback_cast(vehicle_log_gpx), attr_position_coord_geo, this_, log);
	} else if (!strcmp(type_attr.u.str, "textfile")) {
		char *header = "type=track\n";
		log_set_header(log, header, strlen(header));
		cb=callback_new_attr_2(callback_cast(vehicle_log_textfile), attr_position_coord_geo, this_, log);
	} else if (!strcmp(type_attr.u.str, "binfile")) {
		cb=callback_new_attr_2(callback_cast(vehicle_log_binfile), attr_position_coord_geo, this_, log);
	} else
		return 1;
	callback_list_add(this_->cbl, cb);
	return 0;
}

