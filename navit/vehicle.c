/*
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

/** @file vehicle.c
 * @defgroup vehicle-plugins vehicle plugins
 * @ingroup plugins
 * @brief Generic components of the vehicle object.
 *
 * This file implements the generic vehicle interface, i.e. everything which is
 * not specific to a single data source.
 *
 * @author Navit Team
 * @date 2005-2014
 */

#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <time.h>
#include <math.h> /* for sqrt from coord.h */
#include "config.h"
#include "debug.h"
#include "coord.h"
#include "item.h"
#include "xmlconfig.h"
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
#include "navit_nls.h"


struct vehicle {
    NAVIT_OBJECT
    struct vehicle_methods meth;
    struct vehicle_priv *priv;
    struct callback_list *cbl;
    struct log *nmea_log, *gpx_log;
    char *gpx_desc;

    // cursor
    struct cursor *cursor;
    int cursor_fixed;
    struct callback *animate_callback;
    struct event_timeout *animate_timer;
    struct point cursor_pnt;
    int need_resize;
    int real_w;
    int real_h;
    struct graphics *gra;
    struct graphics_gc *bg;
    struct transformation *trans;
    int angle;
    int speed;
    int sequence;
    GHashTable *log_to_cb;
};

struct object_func vehicle_func;

static void vehicle_set_default_name(struct vehicle *this);
static void vehicle_draw_do(struct vehicle *this_);
static void vehicle_log_nmea(struct vehicle *this_, struct log *log);
static void vehicle_log_gpx(struct vehicle *this_, struct log *log);
static void vehicle_log_textfile(struct vehicle *this_, struct log *log);
static void vehicle_log_binfile(struct vehicle *this_, struct log *log);
static int vehicle_add_log(struct vehicle *this_, struct log *log);



/**
 * @brief Creates a new vehicle
 *
 * @param parent
 * @param attrs Points to a null-terminated array of pointers to the attributes
 * for the new vehicle type.
 *
 * @return The newly created vehicle object
 */
struct vehicle *
vehicle_new(struct attr *parent, struct attr **attrs) {
    struct vehicle *this_;
    struct attr *source;
    struct vehicle_priv *(*vehicletype_new) (struct vehicle_methods *
            meth,
            struct callback_list *
            cbl,
            struct attr ** attrs);
    char *type, *colon;
    struct pcoord center;

    dbg(lvl_debug, "enter");
    source = attr_search(attrs, NULL, attr_source);
    if (!source) {
        dbg(lvl_error, "incomplete vehicle definition: missing attribute 'source'");
        return NULL;
    }

    type = g_strdup(source->u.str);
    colon = strchr(type, ':');
    if (colon)
        *colon = '\0';
    dbg(lvl_debug, "source='%s' type='%s'", source->u.str, type);

    vehicletype_new = plugin_get_category_vehicle(type);
    if (!vehicletype_new) {
        dbg(lvl_error, "invalid source '%s': unknown type '%s'", source->u.str, type);
        g_free(type);
        return NULL;
    }
    g_free(type);
    this_ = g_new0(struct vehicle, 1);
    this_->func=&vehicle_func;
    navit_object_ref((struct navit_object *)this_);
    this_->cbl = callback_list_new();
    this_->priv = vehicletype_new(&this_->meth, this_->cbl, attrs);
    if (!this_->priv) {
        dbg(lvl_error, "vehicletype_new failed");
        callback_list_destroy(this_->cbl);
        g_free(this_);
        return NULL;
    }
    this_->attrs=attr_list_dup(attrs);

    center.pro=projection_screen;
    center.x=0;
    center.y=0;
    this_->trans=transform_new(&center, 16, 0);
    vehicle_set_default_name(this_);

    dbg(lvl_debug, "leave");
    this_->log_to_cb=g_hash_table_new(NULL,NULL);
    return this_;
}

/**
 * @brief Destroys a vehicle
 *
 * @param this_ The vehicle to destroy
 */
void vehicle_destroy(struct vehicle *this_) {
    dbg(lvl_debug,"enter");
    if (this_->animate_callback) {
        callback_destroy(this_->animate_callback);
        event_remove_timeout(this_->animate_timer);
    }
    transform_destroy(this_->trans);
    this_->meth.destroy(this_->priv);
    callback_list_destroy(this_->cbl);
    attr_list_free(this_->attrs);
    if (this_->bg)
        graphics_gc_destroy(this_->bg);
    if (this_->gra)
        graphics_free(this_->gra);
    g_free(this_);
}

/**
 * Creates an attribute iterator to be used with vehicles
 */
struct attr_iter *
vehicle_attr_iter_new(void * unused) {
    return (struct attr_iter *)g_new0(void *,1);
}

/**
 * Destroys a vehicle attribute iterator
 *
 * @param iter a vehicle attr_iter
 */
void vehicle_attr_iter_destroy(struct attr_iter *iter) {
    g_free(iter);
}



/**
 * Generic get function
 *
 * @param this_ Pointer to a vehicle structure
 * @param type The attribute type to look for
 * @param attr Pointer to a {@code struct attr} to store the attribute
 * @param iter A vehicle attr_iter. This is only used for generic attributes; for attributes specific to the vehicle object it is ignored.
 * @return True for success, false for failure
 */
int vehicle_get_attr(struct vehicle *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter) {
    int ret;
    if (type == attr_log_gpx_desc) {
        attr->u.str = this_->gpx_desc;
        return 1;
    }
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
 * @param attr The attribute to set
 * @return False on success, true on failure
 */
int vehicle_set_attr(struct vehicle *this_, struct attr *attr) {
    int ret=1;
    if (attr->type == attr_log_gpx_desc) {
        g_free(this_->gpx_desc);
        this_->gpx_desc = g_strdup(attr->u.str);
    } else if (this_->meth.set_attr)
        ret=this_->meth.set_attr(this_->priv, attr);
    /* attr_profilename probably is never used by vehicle itself but it's used to control the
      routing engine. So any vehicle should allow to set and read it. */
    if(attr->type == attr_profilename)
        ret=1;
    if (ret == 1 && attr->type != attr_navit && attr->type != attr_pdl_gps_update)
        this_->attrs=attr_generic_set_attr(this_->attrs, attr);
    return ret != 0;
}

/**
 * Generic add function
 *
 * @param this_ A vehicle
 * @param attr The attribute to add
 *
 * @return true if the attribute was added, false if not.
 */
int vehicle_add_attr(struct vehicle *this_, struct attr *attr) {
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
        this_->cursor_fixed=1;
        vehicle_set_cursor(this_, attr->u.cursor, 1);
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
int vehicle_remove_attr(struct vehicle *this_, struct attr *attr) {
    struct callback *cb;
    switch (attr->type) {
    case attr_callback:
        callback_list_remove(this_->cbl, attr->u.callback);
        break;
    case attr_log:
        cb=g_hash_table_lookup(this_->log_to_cb, attr->u.log);
        if (!cb)
            return 0;
        g_hash_table_remove(this_->log_to_cb, attr->u.log);
        callback_list_remove(this_->cbl, cb);
        break;
    default:
        this_->attrs=attr_generic_remove_attr(this_->attrs, attr);
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
void vehicle_set_cursor(struct vehicle *this_, struct cursor *cursor, int overwrite) {
    dbg(lvl_debug,"enter this_=%p cursot=%p overwrit=%d, this_->cursor_fixed=%d, this_->gra=%p", this_, cursor, overwrite,
        this_->cursor_fixed, this_->gra);
    if (this_->cursor_fixed && !overwrite)
        return;
    if (this_->animate_callback) {
        event_remove_timeout(this_->animate_timer);
        this_->animate_timer=NULL;		// dangling pointer! prevent double freeing.
        callback_destroy(this_->animate_callback);
        this_->animate_callback=NULL;	// dangling pointer! prevent double freeing.
    }
    if (cursor && cursor->interval) {
        this_->animate_callback=callback_new_2(callback_cast(vehicle_draw_do), this_, 0);
        this_->animate_timer=event_add_timeout(cursor->interval, 1, this_->animate_callback);
    }
    /* we changed the cursor, so the overlay (if existing) may need a resize */
    this_->need_resize=1;
    this_->cursor=cursor;

    /* if the graphics was already created, but a NULL cursor was set, we need to disable
     * otherwise stale overlay
     */
    if(this_->gra) {
        if(this_->cursor)
            graphics_overlay_disable(this_->gra, 0);
        else
            graphics_overlay_disable(this_->gra, 1);
    }
    /* vehicle_draw will care for the graphics */
}

/**
 * Draws a vehicle on top of a graphics.
 *
 * @param this_ The vehicle
 * @param gra The graphics
 * @param pnt Screen coordinates of the vehicle.
 * @param angle The angle relative to the map.
 * @param speed The speed of the vehicle.
 */
void vehicle_draw(struct vehicle *this_, struct graphics *gra, struct point *pnt, int angle, int speed) {
    struct point sc;
    if (angle < 0)
        angle+=360;
    dbg(lvl_debug,"enter this=%p gra=%p pnt=%p dir=%d speed=%d", this_, gra, pnt, angle, speed);
    dbg(lvl_debug,"point %d,%d", pnt->x, pnt->y);
    this_->cursor_pnt=*pnt;
    this_->angle=angle;
    this_->speed=speed;
    if (!this_->cursor)
        return;

    if((this_->need_resize) || !(this_->gra)) {
        /* recalculate real size of the required overlay */
        navit_float radius;

        /* get the radius of the out circle. Pythagoras greets */
        radius = navit_sqrt((this_->cursor->w * this_->cursor->w) + (this_->cursor->h * this_->cursor->h));
        /* since we rotate the rectangle around the center to indicate direction, the overlay needs to be at least the
         * radius of the out circle big. The +1 compensates the rounding error.
         */
        this_->real_w = (int)radius +1;
        this_->real_h = (int)radius +1;

        /* set transform center to the middle of the cursor */
        sc.x = this_->real_w/2;
        sc.y = this_->real_h/2;
        transform_set_screen_center(this_->trans, &sc);
    }

    /* move the cursor point from te center to the top left*/
    this_->cursor_pnt.x-=(this_->real_w/2);
    this_->cursor_pnt.y-=(this_->real_h/2);
    dbg(lvl_debug,"real point %d,%d real size %d,%d", this_->cursor_pnt.x, this_->cursor_pnt.y, this_->real_w,
        this_->real_h);

    if (!this_->gra) {
        struct color c;
        this_->need_resize=0;
        this_->gra=graphics_overlay_new(gra, &this_->cursor_pnt, this_->real_w, this_->real_h, 0);
        if (this_->gra) {
            graphics_init(this_->gra);
            this_->bg=graphics_gc_new(this_->gra);
            c.r=0;
            c.g=0;
            c.b=0;
            c.a=0;
            graphics_gc_set_foreground(this_->bg, &c);
            graphics_background_gc(this_->gra, this_->bg);
        }
    } else if (this_->need_resize) {
        /* seems the cursor was changed. Need to resize */
        this_->need_resize=0;
        graphics_overlay_resize(this_->gra, &this_->cursor_pnt, this_->real_w, this_->real_h, 0);
    }

    vehicle_draw_do(this_);
}

int vehicle_get_cursor_data(struct vehicle *this, struct point *pnt, int *angle, int *speed) {
    *pnt=this->cursor_pnt;
    *angle=this->angle;
    *speed=this->speed;
    return 1;
}

static void vehicle_set_default_name(struct vehicle *this_) {
    struct attr default_name;
    if (!attr_search(this_->attrs, NULL, attr_name)) {
        default_name.type=attr_name;
        // Safe cast: attr_generic_set_attr does not modify its parameter.
        default_name.u.str=(char*)_("Unnamed vehicle");
        this_->attrs=attr_generic_set_attr(this_->attrs, &default_name);
        dbg(lvl_error, "Incomplete vehicle definition: missing attribute 'name'. Default name set.");
    }
}

static void vehicle_draw_do(struct vehicle *this_) {
    struct point p;
    struct cursor *cursor=this_->cursor;
    int speed=this_->speed;
    int angle=this_->angle;
    int sequence=this_->sequence;
    struct attr **attr;
    char *label=NULL;
    int match=0;

    if (!this_->cursor || !this_->cursor->attrs || !this_->gra)
        return;

    attr=this_->attrs;
    while (attr && *attr) {
        if ((*attr)->type == attr_name)
            label=(*attr)->u.str;
        attr++;
    }
    transform_set_yaw(this_->trans, -this_->angle);
    graphics_draw_mode(this_->gra, draw_mode_begin);
    p.x=0;
    p.y=0;
    /* clear old content by overwriting with an rectangle */
    graphics_draw_rectangle(this_->gra, this_->bg, &p, this_->real_w, this_->real_h);
    attr=cursor->attrs;
    while (*attr) {
        if ((*attr)->type == attr_itemgra) {
            struct itemgra *itm=(*attr)->u.itemgra;
            dbg(lvl_debug,"speed %d-%d %d", itm->speed_range.min, itm->speed_range.max, speed);
            if (speed >= itm->speed_range.min && speed <= itm->speed_range.max &&
                    angle >= itm->angle_range.min && angle <= itm->angle_range.max &&
                    sequence >= itm->sequence_range.min && sequence <= itm->sequence_range.max) {
                graphics_draw_itemgra(this_->gra, itm, this_->trans, label);
            }
            if (sequence < itm->sequence_range.max)
                match=1;
        }
        ++attr;
    }
    graphics_draw_drag(this_->gra, &this_->cursor_pnt);
    graphics_draw_mode(this_->gra, draw_mode_end);
    if (this_->animate_callback) {
        ++this_->sequence;
        if (cursor->sequence_range && cursor->sequence_range->max < this_->sequence)
            this_->sequence=cursor->sequence_range->min;
        if (! match && ! cursor->sequence_range)
            this_->sequence=0;
    }
}

/**
 * @brief Writes to an NMEA log.
 *
 * @param this_ The vehicle supplying data
 * @param log The log to write to
 */
static void vehicle_log_nmea(struct vehicle *this_, struct log *log) {
    struct attr pos_attr;
    if (!this_->meth.position_attr_get)
        return;
    if (!this_->meth.position_attr_get(this_->priv, attr_position_nmea, &pos_attr))
        return;
    log_write(log, pos_attr.u.str, strlen(pos_attr.u.str), 0);
}

/**
 * Add a tag to the extensions section of a GPX trackpoint.
 *
 * @param tag The tag to add
 * @param logstr Pointer to a pointer to a string to be inserted into the log.
 * When calling this function, {@code *logstr} must point to the substring into which the new tag is
 * to be inserted. If {@code *logstr} is NULL, a new string will be created for the extensions section.
 * Upon returning, {@code *logstr} will point to the new string with the additional tag inserted.
 */
void vehicle_log_gpx_add_tag(char *tag, char **logstr) {
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

/**
 * @brief Writes a trackpoint to a GPX log.
 *
 * @param this_ The vehicle supplying data
 * @param log The log to write to
 */
static void vehicle_log_gpx(struct vehicle *this_, struct log *log) {
    struct attr attr,*attrp, fix_attr;
    enum attr_type *attr_types;
    char *logstr;
    char *extensions="\t<extensions>\n";

    if (!this_->meth.position_attr_get)
        return;
    if (log_get_attr(log, attr_attr_types, &attr, NULL))
        attr_types=attr.u.attr_types;
    else
        attr_types=NULL;
    if (this_->meth.position_attr_get(this_->priv, attr_position_fix_type, &fix_attr)) {
        if ( fix_attr.u.num == 0 )
            return;
    }
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
    if (this_->gpx_desc) {
        logstr=g_strconcat_printf(logstr,"\t<desc>%s</desc>\n",this_->gpx_desc);
        g_free(this_->gpx_desc);
        this_->gpx_desc = NULL;
    }
    if (attr_types_contains_default(attr_types, attr_position_height,0)
            && this_->meth.position_attr_get(this_->priv, attr_position_height, &attr))
        logstr=g_strconcat_printf(logstr,"\t<ele>%.6f</ele>\n",*attr.u.numd);
    // <magvar> magnetic variation in degrees; we might use position_magnetic_direction and position_direction to figure it out
    // <geoidheight> Height (in meters) of geoid (mean sea level) above WGS84 earth ellipsoid. As defined in NMEA GGA message (field 11, which vehicle_wince.c ignores)
    // <name> GPS name (arbitrary)
    // <cmt> comment
    // <src> Source of data
    // <link> Link to additional information (URL)
    // <sym> Text of GPS symbol name
    // <type> Type (classification)
    // <fix> Type of GPS fix {'none'|'2d'|'3d'|'dgps'|'pps'}, leave out if unknown. Similar to position_fix_type but more detailed.
    if (attr_types_contains_default(attr_types, attr_position_sats_used,0)
            && this_->meth.position_attr_get(this_->priv, attr_position_sats_used, &attr))
        logstr=g_strconcat_printf(logstr,"\t<sat>%d</sat>\n",attr.u.num);
    if (attr_types_contains_default(attr_types, attr_position_hdop,0)
            && this_->meth.position_attr_get(this_->priv, attr_position_hdop, &attr))
        logstr=g_strconcat_printf(logstr,"\t<hdop>%.6f</hdop>\n",*attr.u.numd);
    // <vdop>, <pdop> Vertical and position dilution of precision, no corresponding attribute
    if (attr_types_contains_default(attr_types, attr_position_direction,0)
            && this_->meth.position_attr_get(this_->priv, attr_position_direction, &attr))
        logstr=g_strconcat_printf(logstr,"\t<course>%.1f</course>\n",*attr.u.numd);
    if (attr_types_contains_default(attr_types, attr_position_speed, 0)
            && this_->meth.position_attr_get(this_->priv, attr_position_speed, &attr))
        logstr=g_strconcat_printf(logstr,"\t<speed>%.2f</speed>\n",(*attr.u.numd / 3.6));
    if (attr_types_contains_default(attr_types, attr_profilename, 0)
            && (attrp=attr_search(this_->attrs, NULL, attr_profilename))) {
        logstr=g_strconcat_printf(logstr,"%s\t\t<navit:profilename>%s</navit:profilename>\n",extensions,attrp->u.str);
        extensions="";
    }
    if (attr_types_contains_default(attr_types, attr_position_radius, 0)
            && this_->meth.position_attr_get(this_->priv, attr_position_radius, &attr)) {
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

/**
 * @brief Writes to a text log.
 *
 * @param this_ The vehicle supplying data
 * @param log The log to write to
 */
static void vehicle_log_textfile(struct vehicle *this_, struct log *log) {
    struct attr pos_attr,fix_attr;
    char *logstr;
    if (!this_->meth.position_attr_get)
        return;
    if (this_->meth.position_attr_get(this_->priv, attr_position_fix_type, &fix_attr)) {
        if (fix_attr.u.num == 0)
            return;
    }
    if (!this_->meth.position_attr_get(this_->priv, attr_position_coord_geo, &pos_attr))
        return;
    logstr=g_strdup_printf("%f %f type=trackpoint\n", pos_attr.u.coord_geo->lng, pos_attr.u.coord_geo->lat);
    callback_list_call_attr_1(this_->cbl, attr_log_textfile, &logstr);
    log_write(log, logstr, strlen(logstr), 0);
}

/**
 * @brief Writes to a binary log.
 *
 * @param this_ The vehicle supplying data
 * @param log The log to write to
 */
static void vehicle_log_binfile(struct vehicle *this_, struct log *log) {
    struct attr pos_attr, fix_attr;
    int *buffer;
    int *buffer_new;
    int len,limit=1024,done=0,radius=25;
    struct coord c;
    enum log_flags flags;

    if (!this_->meth.position_attr_get)
        return;
    if (this_->meth.position_attr_get(this_->priv, attr_position_fix_type, &fix_attr)) {
        if (fix_attr.u.num == 0)
            return;
    }
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
        dbg(lvl_debug,"c=0x%x,0x%x",c.x,c.y);
        buffer_new[buffer_new[0]+1]=c.x;
        buffer_new[buffer_new[0]+2]=c.y;
        buffer_new[0]+=2;
        buffer_new[2]+=2;
        if (buffer_new[2] > limit) {
            int count=buffer_new[2]/2;
            struct coord *out=g_alloca(sizeof(struct coord)*(count));
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

/**
 * @brief Registers a new log to receive data.
 *
 * @param this_ The vehicle supplying data
 * @param log The log to write to
 *
 * @return False if the log is of an unknown type, true otherwise (including when {@code attr_type} is missing).
 */
static int vehicle_add_log(struct vehicle *this_, struct log *log) {
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
        return 0;
    g_hash_table_insert(this_->log_to_cb, log, cb);
    callback_list_add(this_->cbl, cb);
    return 1;
}

struct object_func vehicle_func = {
    attr_vehicle,
    (object_func_new)vehicle_new,
    (object_func_get_attr)vehicle_get_attr,
    (object_func_iter_new)vehicle_attr_iter_new,
    (object_func_iter_destroy)vehicle_attr_iter_destroy,
    (object_func_set_attr)vehicle_set_attr,
    (object_func_add_attr)vehicle_add_attr,
    (object_func_remove_attr)vehicle_remove_attr,
    (object_func_init)NULL,
    (object_func_destroy)vehicle_destroy,
    (object_func_dup)NULL,
    (object_func_ref)navit_object_ref,
    (object_func_unref)navit_object_unref,
};
