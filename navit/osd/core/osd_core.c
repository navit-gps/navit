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

#include "config.h"
#ifdef _MSC_VER
#define _USE_MATH_DEFINES 1
#endif /* _MSC_VER */
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <glib.h>
#include <time.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <string.h>
#include "item.h"
#include "xmlconfig.h"
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
#include "util.h"

#ifdef HAVE_API_WIN32_CE
#include "libc.h"
#endif

#ifdef _MSC_VER
static double round(double x) {
    if (x >= 0.0)
        return floor(x + 0.5);
    else
        return ceil(x - 0.5);
}
#endif /* MSC_VER */

struct osd_priv_common {
    struct osd_item osd_item;
    struct osd_priv *data;
    int (*spec_set_attr_func)(struct osd_priv_common *opc, struct attr* attr);
};

struct odometer;

int set_std_osd_attr(struct osd_priv *priv, struct attr*the_attr);
static void osd_odometer_reset(struct osd_priv_common *opc, int flags);
static void osd_cmd_odometer_reset(struct navit *this, char *function, struct attr **in, struct attr ***out,
                                   int *valid);
static void osd_odometer_draw(struct osd_priv_common *opc, struct navit *nav, struct vehicle *v);
static struct osd_text_item * oti_new(struct osd_text_item * parent);
int osd_button_set_attr(struct osd_priv_common *opc, struct attr* attr);

static int b_commandtable_added = 0;


struct compass {
    int width;	/*!< Width of the compass in pixels */
    struct color destination_dir_color;	/*!< Color definition of the destination direction arrow */
    struct color north_color;	/*!< Color definition of north handle of the compass */
    struct graphics_gc *destination_dir_gc;	/*!< graphics_gc context used to draw the destination direction arrow */
    struct graphics_gc *north_gc; /*!< graphics_gc context used to draw the north handle of the compass */
    struct callback *click_cb;	/*!< A callback to execute when clicking on the compass */
};

/**
 * @brief Rotate a group of points around a @p center
 * @param center The coordinates of the center of the rotation to apply
 * @param angle The angle of the rotation
 * @param[in,out] p An array of points to rotate
 * @param count The number of points stored inside @p p
 */
static void transform_rotate(struct point *center, int angle, struct point *p,
                             int count) {
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

/**
 * @brief Move a group of points in a direction (adding @p dx and @p dy to their x and y coordinates)
 * @param dx The shift to perform to the x axis
 * @param dy The shift to perform to the y axis
 * @param[in,out] p An array of points to move
 * @param count The number of points stored inside @p p
 */
static void transform_move(int dx, int dy, struct point *p,
                           int count) {
    int i;
    for (i = 0; i < count; i++) {
        p->x += dx;
        p->y += dy;
        p++;
    }
}

/**
 * @brief Draw a compass handle of length @p r, centered at point @p p, pointing to direction @p dir
 *
 * @param gr The graphics instance on which to draw
 * @param gc_n The color to use for the north half of the compass
 * @param gc_s The color to use for the south half of the compass
 * @param p The center of the compass
 * @param r The radius of the compass (around the center point @p p)
 * @param dir The direction the compass points to (0 being up, value is in degrees counter-clockwise)
 */
static void draw_compass(struct graphics *gr, struct graphics_gc *gc_n, struct graphics_gc *gc_s, struct point *p,
                         int r,
                         int dir) {
    struct point ph[3];
    int wh[3] = { 1, 1, 1 };	/* Width of each line of the polygon to draw */
    int l = r * 0.25;

    ph[0].x = -l;
    ph[0].y = 0;
    ph[1].x = 0;
    ph[1].y = -r;
    ph[2].x = l;
    ph[2].y = 0;
    transform_rotate(p, dir, ph, 3); /* Rotate to the correct direction */
    graphics_draw_polygon_clipped(gr, gc_n, ph, 3);	/* Draw north half */

    ph[0].x = -l;
    ph[0].y = 0;
    ph[1].x = 0;
    ph[1].y = r;
    ph[2].x = l;
    ph[2].y = 0;
    transform_rotate(p, dir, ph, 3); /* Rotate to the correct direction */
    graphics_draw_polyline_clipped(gr, gc_s, ph, 3, wh, 0);	/* Draw south half */
}

/**
 * @brief Draw an arrow of length @p r, centered at point @p p, with color @p gc, pointing to direction @p dir
 *
 * @param gr The graphics instance on which to draw
 * @param gc The color to draw the arrow
 * @param p The center of the compass
 * @param r The radius of the compass (around the center point @p p)
 * @param dir The direction the arrow points to (0 being up, value is in degrees counter-clockwise)
 */
static void draw_handle(struct graphics *gr, struct graphics_gc *gc, struct point *p, int r,
                        int dir) {
    struct point ph[6];
    int l = r * 0.4;
    int s = l * 0.4;

    ph[0].x = 0; /* Compute details for the body of the arrow */
    ph[0].y = r - l;
    ph[1].x = 0;
    ph[1].y = -r;
    transform_rotate(p, dir, ph, 2); /* Rotate to the correct direction */
    graphics_draw_lines(gr, gc, ph, 2); /* Draw the body */

    ph[0].x = -l; /* Compute details for the head of the arrow */
    ph[0].y = -r + l;
    ph[1].x = 0;
    ph[1].y = -r;
    ph[2].x = l;
    ph[2].y = -r + l;
    transform_rotate(p, dir, ph, 3); /* Rotate to the correct direction */
    graphics_draw_lines(gr, gc, ph, 3); /* Draw the head */

    ph[0].x = -s; /* Compute details for the tail of the arrow */
    ph[0].y = r - l + s;
    ph[1].x = 0;
    ph[1].y = r - l;
    ph[2].x = s;
    ph[2].y = r - l + s;
    ph[3]=ph[0]; /* Save these 3 points for future re-use */
    ph[4]=ph[1];
    ph[5]=ph[2];
    transform_rotate(p, dir, ph, 3); /* Rotate to the correct direction */
    graphics_draw_lines(gr, gc, ph, 3); /* Draw the tail */
    ph[0]=ph[3];	/* Restore saved points */
    ph[1]=ph[4];
    ph[2]=ph[5];
    transform_move(0, s, ph, 3);
    ph[3]=ph[0]; /* Save these 3 points for future re-use */
    ph[4]=ph[1];
    ph[5]=ph[2];
    transform_rotate(p, dir, ph, 3); /* Rotate to the correct direction */
    graphics_draw_lines(gr, gc, ph, 3); /* Draw the tail */
    ph[0]=ph[3];	/* Restore saved points */
    ph[1]=ph[4];
    ph[2]=ph[5];
    transform_move(0, s, ph, 3);
    transform_rotate(p, dir, ph, 3); /* Rotate to the correct direction */
    graphics_draw_lines(gr, gc, ph, 3); /* Draw the tail */
}

/**
 * * Format distance, choosing the unit (m or km) and precision depending on distance
 * *
 * * @param distance distance in meters
 * * @param sep separator character to be inserted between distance value and unit
 * * @returns a pointer to a string containing the formatted distance
 * */
static char *format_distance(double distance, char *sep, int imperial) {
    if (imperial) {
        distance *= FEET_PER_METER;
        if(distance <= 500) {
            return g_strdup_printf("%.0f%sft", round(distance / 10) * 10, sep);
        } else {
            return g_strdup_printf("%.1f%smi", distance / FEET_PER_MILE, sep);
        }
    } else {
        if (distance >= 10000)
            return g_strdup_printf("%.0f%skm", distance / 1000, sep);
        else if (distance >= 1000)
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
}

/**
 * * Format time (duration)
 * *
 * * @param tm pointer to a tm structure specifying the time
 * * @param days days
 * * @returns a pointer to a string containing the formatted time
 * */
static char *format_time(struct tm *tm, int days) {
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
static char *format_speed(double speed, char *sep, char *format, int imperial) {
    char *unit="km/h";
    if (imperial) {
        speed = speed*1000*FEET_PER_METER/FEET_PER_MILE;
        unit="mph";
    }
    if (!format || !strcmp(format,"named"))
        return g_strdup_printf((speed < 10) ? "%.1f%s%s":"%.0f%s%s", speed, sep, unit);
    else if (!strcmp(format,"value") || !strcmp(format,"unit")) {
        if (!strcmp(format,"value"))
            return g_strdup_printf((speed < 10) ? "%.1f":"%.0f", speed);
        else
            return g_strdup(unit);
    }
    return g_strdup("");
}

static char *format_float_0(double num) {
    return g_strdup_printf("%.0f", num);
}

int set_std_osd_attr(struct osd_priv *priv, struct attr*the_attr) {
    struct osd_priv_common *opc=(struct osd_priv_common *)priv;
    if(opc && the_attr && ATTR_IS_INT(the_attr->type)) {
        int attr_set=0;
        if(attr_w == the_attr->type) {
            opc->osd_item.rel_w = the_attr->u.num;
            attr_set=1;
        } else if(attr_h == the_attr->type) {
            opc->osd_item.rel_h = the_attr->u.num;
            attr_set=1;
        } else if(attr_x == the_attr->type) {
            opc->osd_item.rel_x = the_attr->u.num;
            attr_set=1;
        } else if(attr_y == the_attr->type) {
            opc->osd_item.rel_y = the_attr->u.num;
            attr_set=1;
        } else if(attr_font_size == the_attr->type) {
            opc->osd_item.font_size = the_attr->u.num;
            attr_set=1;
        }
        if(attr_set && opc->osd_item.gr) {
            osd_std_calculate_sizes(&opc->osd_item, navit_get_width(opc->osd_item.navit), navit_get_height(opc->osd_item.navit));
            osd_std_resize(&opc->osd_item);
            return 1;
        }
    }
    if(opc->spec_set_attr_func) {
        opc->spec_set_attr_func(opc, the_attr);
    }
    return 0;
}


struct route_guard {
    int coord_num;
    struct coord *coords;
    double min_dist;	//distance treshold, exceeding this distance will trigger announcement
    double max_dist;	//out of range distance, farther than this distance no warning will be given
    char*item_name;
    char*map_name;
    int warned;
    double last_time;
    int update_period;
    struct color active_color;
    struct graphics_gc *red;
    int width;
};

static void osd_route_guard_draw(struct osd_priv_common *opc, struct navit *nav, struct vehicle *v) {
    int i=0;
    struct vehicle* curr_vehicle = v;
    struct attr position_attr, vehicle_attr, imperial_attr;
    struct coord curr_coord;
    struct route_guard *this = (struct route_guard *)opc->data;
    double curr_time;
    struct timeval tv;
    struct point p;
    struct point bbox[4];
    char* dist_str;
    struct graphics_gc *curr_color;
    int imperial=0;
    double min_dist;

    //do not execute for each gps update
    gettimeofday(&tv,NULL);
    curr_time = (double)(tv.tv_usec)/1000000.0+tv.tv_sec;
    if ( this->last_time+this->update_period > curr_time) {
        return;
    }
    this->last_time = curr_time;
    if(nav) {
        navit_get_attr(nav, attr_vehicle, &vehicle_attr, NULL);
        if (vehicle_attr.u.vehicle) {
            curr_vehicle = vehicle_attr.u.vehicle;
        }
        if (navit_get_attr(nav, attr_imperial, &imperial_attr, NULL)) {
            imperial=imperial_attr.u.num;
        }
    }

    if(0==curr_vehicle)
        return;

    if(!vehicle_get_attr(curr_vehicle, attr_position_coord_geo,&position_attr, NULL)) {
        return;
    }
    transform_from_geo(projection_mg, position_attr.u.coord_geo, &curr_coord);

    min_dist = 1000000;
    //calculate min dist
    if(this->coord_num > 1) {
        double scale = transform_scale(curr_coord.y);
        for( i=1 ; i<this->coord_num ; ++i ) {
            struct coord proj_coord;
            double curr_dist;
            transform_distance_line_sq(&this->coords[i-1], &this->coords[i], &curr_coord, &proj_coord);
            curr_dist = transform_distance(projection_mg, &proj_coord, &curr_coord);
            curr_dist /= scale;
            if (curr_dist<min_dist) {
                min_dist = curr_dist;
            }
        }
        if ( this->warned == 0 && this->min_dist < min_dist && min_dist < this->max_dist) {
            navit_say(nav, _("Return to route!"));
            this->warned = 1;
        } else if( min_dist < this->min_dist ) {
            this->warned = 0;
        }
    }
    osd_fill_with_bgcolor(&opc->osd_item);

    dist_str = format_distance(min_dist, "", imperial);

    graphics_get_text_bbox(opc->osd_item.gr, opc->osd_item.font, dist_str, 0x10000, 0, bbox, 0);
    p.x=(opc->osd_item.w-bbox[2].x)/2;
    p.y = opc->osd_item.h-opc->osd_item.h/10;

    curr_color = (this->min_dist < min_dist && min_dist < this->max_dist) ? this->red : opc->osd_item.graphic_fg;
    graphics_draw_text(opc->osd_item.gr, curr_color, NULL, opc->osd_item.font, dist_str, &p, 0x10000, 0);

    g_free(dist_str);

    graphics_draw_mode(opc->osd_item.gr, draw_mode_end);
}

static void osd_route_guard_init(struct osd_priv_common *opc, struct navit *nav) {
    struct color red_color= {0xffff,0x0000,0x0000,0xffff};
    struct route_guard *this = (struct route_guard *)opc->data;
    osd_set_std_graphic(nav, &opc->osd_item, (struct osd_priv *)opc);
    //load coord data
    if (this->map_name && this->item_name) {
        struct mapset* ms;
        struct map_rect *mr;
        struct mapset_handle *msh;
        struct map *map = NULL;
        struct item *item = NULL;
        if(!(ms=navit_get_mapset(nav))) {
            return;
        }
        msh=mapset_open(ms);
        while ((map=mapset_next(msh, 1))) {
            struct attr attr;
            if(map_get_attr(map, attr_name, &attr, NULL)) {
                if( ! strcmp(this->map_name, attr.u.str) ) {
                    mr=map_rect_new(map, NULL);
                    if (mr) {
                        while ((item=map_rect_get_item(mr))) {
                            struct attr item_attr;
                            if(item_attr_get(item, attr_name, &item_attr)) {
                                if (!strcmp(item_attr.u.str,this->item_name)) {
                                    //item found, get coords
                                    struct coord c;
                                    this->coord_num=0;
                                    while (item_coord_get(item,&c,1)) {
                                        this->coords = g_renew(struct coord,this->coords,this->coord_num+1);
                                        this->coords[this->coord_num] = c;
                                        ++this->coord_num;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    continue;
                }
            } else {
                continue;
            }
        }
        mapset_close(msh);
    }

    this->red = graphics_gc_new(opc->osd_item.gr);
    graphics_gc_set_foreground(this->red, &red_color);
    graphics_gc_set_linewidth(this->red, this->width);

    opc->osd_item.graphic_fg = graphics_gc_new(opc->osd_item.gr);
    graphics_gc_set_foreground(opc->osd_item.graphic_fg, &opc->osd_item.text_color);
    graphics_gc_set_linewidth(opc->osd_item.graphic_fg, this->width);

    //setup draw callback
    navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_route_guard_draw), attr_position_coord_geo, opc));
}

static void osd_route_guard_destroy(struct osd_priv_common *opc) {
    struct route_guard *this = (struct route_guard *)opc->data;
    g_free(this->coords);
}

static struct osd_priv *osd_route_guard_new(struct navit *nav, struct osd_methods *meth,
        struct attr **attrs) {
    struct route_guard *this = g_new0(struct route_guard, 1);
    struct osd_priv_common *opc = g_new0(struct osd_priv_common,1);
    struct attr *attr;

    opc->data = (void*)this;
    opc->osd_item.rel_x = 120;
    opc->osd_item.rel_y = 20;
    opc->osd_item.rel_w = 60;
    opc->osd_item.rel_h = 80;
    opc->osd_item.navit = nav;
    opc->osd_item.font_size = 200;
    opc->osd_item.meth.draw = osd_draw_cast(osd_route_guard_draw);
    meth->set_attr = set_std_osd_attr;

    osd_set_std_attr(attrs, &opc->osd_item, ITEM_HAS_TEXT);

    attr = attr_search(attrs, NULL, attr_min_dist);
    if (attr) {
        this->min_dist = attr->u.num;
    } else
        this->min_dist = 30;	//default tolerance is 30m

    attr = attr_search(attrs, NULL, attr_max_dist);
    if (attr) {
        this->max_dist = attr->u.num;
    } else
        this->max_dist = 500;	//default

    attr = attr_search(attrs, NULL, attr_item_name);
    if (attr) {
        this->item_name = g_strdup(attr->u.str);
    } else
        this->item_name = NULL;

    attr = attr_search(attrs, NULL, attr_map_name);
    if (attr) {
        this->map_name = g_strdup(attr->u.str);
    } else
        this->map_name = NULL;

    attr = attr_search(attrs, NULL, attr_update_period);
    this->update_period=attr ? attr->u.num : 10;

    attr = attr_search(attrs, NULL, attr_width);
    this->width=attr ? attr->u.num : 2;

    navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_route_guard_init), attr_graphics_ready, opc));
    navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_route_guard_destroy), attr_destroy, opc));

    return (struct osd_priv *) opc;
}


static int odometers_saved = 0;
static GList* odometer_list = NULL;

static struct command_table commands[] = {
    {"odometer_reset",command_cast(osd_cmd_odometer_reset)},
};

struct odometer {
    int width;
    struct graphics_gc *orange;
    struct callback *click_cb;
    char *text;                 //text of label attribute for this osd
    char *name;                 //unique name of the odometer (needed for handling multiple odometers persistently)
    struct color idle_color;    //text color when counter is idle
    int align;
    int bDisableReset;
    int bAutoStart;
    int bActive;                //counting or not
    int autosave_period;        //autosave period in seconds
    double sum_dist;            //sum of distance ofprevious intervals in meters
    double sum_time;               //sum of time of previous intervals in seconds (needed for avg spd calculation)
    double time_all;
    double last_click_time;     //time of last click (for double click handling)
    double last_start_time;     //time of last start of counting
    double last_update_time;     //time of last  position update
    struct coord last_coord;
    double last_speed;
    double max_speed;
    double acceleration;
};

static void osd_cmd_odometer_reset(struct navit *this, char *function, struct attr **in, struct attr ***out,
                                   int *valid) {
    if (in && in[0] && ATTR_IS_STRING(in[0]->type) && in[0]->u.str) {
        GList* list = odometer_list;
        while(list) {
            if(!strcmp(((struct odometer*)((struct osd_priv_common *)(list->data))->data)->name,in[0]->u.str)) {
                osd_odometer_reset(list->data,3);
                osd_odometer_draw(list->data,this,NULL);
            }
            list = g_list_next(list);
        }
    }
}

static char* str_replace(char*output, char*input, char*pattern, char*replacement) {
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
static char *osd_odometer_to_string(struct odometer *this_) {
    return g_strdup_printf("odometer %s %lf %lf %d %lf\n",this_->name,this_->sum_dist,this_->time_all,this_->bActive,
                           this_->max_speed);
}

/*
 * load current odometer state from string
 */
static void osd_odometer_from_string(struct odometer *this_, char*str) {
    char*  tok;
    char*  name_str;
    char*  sum_dist_str;
    char*  sum_time_str;
    char*  active_str;
    char*  max_spd_str;
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
        g_free(name_str);
        return;
    }
    sum_time_str = g_strdup(strtok(NULL, " "));
    if(!sum_time_str) {
        g_free(name_str);
        g_free(sum_dist_str);
        return;
    }
    active_str = g_strdup(strtok(NULL, " "));
    if(!active_str) {
        g_free(name_str);
        g_free(sum_dist_str);
        g_free(sum_time_str);
        return;
    }
    max_spd_str = g_strdup(strtok(NULL, " "));
    if(!max_spd_str) {
        g_free(name_str);
        g_free(sum_dist_str);
        g_free(sum_time_str);
        g_free(active_str);
        return;
    }

    this_->name = name_str;
    this_->sum_dist = atof(sum_dist_str);
    this_->sum_time = atof(sum_time_str);
    this_->bActive = atoi(active_str);
    this_->max_speed = atof(max_spd_str);
    this_->last_coord.x = -1;
    g_free(active_str);
    g_free(sum_dist_str);
    g_free(sum_time_str);
    g_free(max_spd_str);
}

static void draw_multiline_osd_text(char *buffer,struct osd_item *osd_item, struct graphics_gc *curr_color) {
    gchar**bufvec = g_strsplit(buffer,"\n",0);
    struct point p, bbox[4];
    //count strings
    int strnum = 0;
    gchar**pch = bufvec;
    while(*pch) {
        ++strnum;
        ++pch;
    }

    if(0<strnum) {
        int dh = osd_item->h / strnum;

        pch = bufvec;
        p.y = 0;
        while (*pch) {
            graphics_get_text_bbox(osd_item->gr, osd_item->font, *pch, 0x10000, 0, bbox, 0);
            p.x=(osd_item->w-bbox[2].x)/2;
            p.y += dh;
            graphics_draw_text(osd_item->gr, curr_color, NULL, osd_item->font, *pch, &p, 0x10000, 0);
            ++pch;
        }
    }
    g_free(bufvec);
}

/**
 * * draw osd text based on the given alignment setting on a osd_item
 * *
 * * @param buffer pointer to a string containing the text to be displayed
 * * @param align alignment setting (to be taken form the osd attribute)
 * * @param osd_item the osd item to work on
 * * @param curr_color the color in which the osd text should be visible (defaults to osd_items foreground color)
 * * @returns nothing
 * */
static void draw_aligned_osd_text(char *buffer, int align, struct osd_item *osd_item, struct graphics_gc *curr_color) {

    int height=osd_item->font_size*13/256;
    int yspacing=height/2;
    int xspacing=height/4;
    char *next, *last;
    struct point p, p2[4];
    int lines;
    int do_draw = osd_item->do_draw;

    osd_fill_with_bgcolor(osd_item);
    lines=0;
    next=buffer;
    last=buffer;
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

    dbg(lvl_debug,"align=%d", align);
    switch (align & 51) {
    case 1:
        p.y=0;
        break;
    case 2:
        p.y=(osd_item->h-lines*(height+yspacing)-yspacing);
        break;
    case 16: // Grow from top to bottom
        p.y = 0;
        if (lines != 0) {
            osd_item->h = (lines-1) * (height+yspacing) + height;
        } else {
            osd_item->h = 0;
        }

        if (do_draw) {
            osd_std_resize(osd_item);
        }
    default:
        p.y=(osd_item->h-lines*(height+yspacing)-yspacing)/2;
    }

    while (buffer) {
        next=strstr(buffer, "\\n");
        if (next) {
            *next='\0';
            next+=2;
        }
        graphics_get_text_bbox(osd_item->gr,
                               osd_item->font,
                               buffer, 0x10000,
                               0x0, p2, 0);
        switch (align & 12) {
        case 4:
            p.x=xspacing;
            break;
        case 8:
            p.x=osd_item->w-(p2[2].x-p2[0].x)-xspacing;
            break;
        default:
            p.x = ((p2[0].x - p2[2].x) / 2) + (osd_item->w / 2);
        }
        p.y += height+yspacing;
        graphics_draw_text(osd_item->gr,
                           curr_color?curr_color:osd_item->graphic_fg_text,
                           NULL, osd_item->font,
                           buffer, &p, 0x10000,
                           0);
        buffer=next;

        graphics_draw_mode(osd_item->gr, draw_mode_end);

    }

}


static void osd_odometer_draw(struct osd_priv_common *opc, struct navit *nav, struct vehicle *v) {
    struct odometer *this = (struct odometer *)opc->data;

    struct coord curr_coord;
    struct graphics_gc *curr_color;

    char *dist_buffer=0;
    char *spd_buffer=0;
    char *max_spd_buffer=0;
    char *time_buffer = 0;
    char *acc_buffer = 0;
    struct attr position_attr,vehicle_attr,imperial_attr,speed_attr;
    enum projection pro;
    struct vehicle* curr_vehicle = v;
    double spd = 0;
    double curr_spd = 0;

    int remainder;
    int days;
    int hours;
    int mins;
    int secs;
    int imperial=0;

    char buffer [256+1]="";
    char buffer2[256+1]="";

    if(nav) {
        if (navit_get_attr(nav, attr_vehicle, &vehicle_attr, NULL))
            curr_vehicle=vehicle_attr.u.vehicle;
        if (navit_get_attr(nav, attr_imperial, &imperial_attr, NULL))
            imperial=imperial_attr.u.num;
    }

    if(0==curr_vehicle)
        return;

    osd_fill_with_bgcolor(&opc->osd_item);
    if(this->bActive) {
        if(!vehicle_get_attr(curr_vehicle, attr_position_coord_geo,&position_attr, NULL)) {
            return;
        }
        pro = projection_mg;//position_attr.u.pcoord->pro;
        transform_from_geo(pro, position_attr.u.coord_geo, &curr_coord);

        if (this->last_coord.x != -1 ) {
            const double cStepDistLimit = 10000;
            struct timeval tv;
            double curr_time;
            double dt;
            double dCurrDist = 0;

            gettimeofday(&tv,NULL);
            curr_time = (double)(tv.tv_usec)/1000000.0+tv.tv_sec;
            //we have valid previous position
            dt = curr_time-this->last_update_time;
            dCurrDist = transform_distance(pro, &curr_coord, &this->last_coord);
            if(dCurrDist<=cStepDistLimit) {
                this->sum_dist += dCurrDist;
            }
            this->time_all = curr_time-this->last_click_time+this->sum_time;
            spd = 3.6*(double)this->sum_dist/(double)this->time_all;
            if(dt != 0) {
                if (curr_coord.x!=this->last_coord.x || curr_coord.y!=this->last_coord.y) {
                    if(vehicle_get_attr(curr_vehicle, attr_position_speed,&speed_attr, NULL)) {
                        double dv;
                        curr_spd = *speed_attr.u.numd;
                        dv = (curr_spd-this->last_speed)/3.6;	//speed difference in m/sec
                        this->acceleration = dv/dt;
                        this->last_speed = curr_spd;
                        this->last_update_time = curr_time;
                    }
                }
            }
            this->max_speed = (curr_spd<=this->max_speed) ? this->max_speed : curr_spd;
        }
        this->last_coord = curr_coord;
    }

    dist_buffer = format_distance(this->sum_dist,"",imperial);
    spd_buffer = format_speed(spd,"","value",imperial);
    max_spd_buffer = format_speed(this->max_speed,"","value",imperial);
    acc_buffer = g_strdup_printf("%.3f m/s2",this->acceleration);
    remainder = (int)this->time_all;
    days  = remainder  / (24*60*60);
    remainder = remainder  % (24*60*60);
    hours = remainder  / (60*60);
    remainder = remainder  % (60*60);
    mins  = remainder  / (60);
    remainder = remainder  % (60);
    secs  = remainder;
    if(0<days) {
        time_buffer = g_strdup_printf("%02dd %02d:%02d:%02d",days,hours,mins,secs);
    } else {
        time_buffer = g_strdup_printf("%02d:%02d:%02d",hours,mins,secs);
    }

    buffer [0] = 0;
    buffer2[0] = 0;
    if(this->text) {
        str_replace(buffer,this->text,"${avg_spd}",spd_buffer);
        str_replace(buffer2,buffer,"${distance}",dist_buffer);
        str_replace(buffer,buffer2,"${time}",time_buffer);
        str_replace(buffer2,buffer,"${acceleration}",acc_buffer);
        str_replace(buffer,buffer2,"${max_spd}",max_spd_buffer);
    }
    g_free(time_buffer);

    curr_color = this->bActive?opc->osd_item.graphic_fg:this->orange;

    draw_aligned_osd_text(buffer, this->align, &opc->osd_item, curr_color);
    g_free(dist_buffer);
    g_free(spd_buffer);
    g_free(max_spd_buffer);
    g_free(acc_buffer);
    graphics_draw_mode(opc->osd_item.gr, draw_mode_end);
}


static void osd_odometer_reset(struct osd_priv_common *opc, int flags) {
    struct odometer *this = (struct odometer *)opc->data;

    if(!this->bDisableReset || (flags & 1)) {
        if (!(flags & 2))
            this->bActive         = 0;
        this->sum_dist        = 0;
        this->sum_time        = 0;
        this->max_speed       = 0;
        this->last_start_time = 0;
        this->last_coord.x    = -1;
        this->last_coord.y    = -1;
    }
}

static void osd_odometer_click(struct osd_priv_common *opc, struct navit *nav, int pressed, int button,
                               struct point *p) {
    struct odometer *this = (struct odometer *)opc->data;

    struct point bp = opc->osd_item.p;
    struct timeval tv;
    double curr_time;
    const double double_click_timewin = .5;
    osd_wrap_point(&bp, nav);
    if ((p->x < bp.x || p->y < bp.y || p->x > bp.x + opc->osd_item.w || p->y > bp.y + opc->osd_item.h
            || !opc->osd_item.configured ) && !opc->osd_item.pressed)
        return;
    if (button != 1)
        return;
    if (navit_ignore_button(nav))
        return;
    if (!!pressed == !!opc->osd_item.pressed)
        return;

    gettimeofday(&tv,NULL);
    curr_time = (double)(tv.tv_usec)/1000000.0+tv.tv_sec;

    if (pressed) { //single click handling
        if(this->bActive) { //being stopped
            this->last_coord.x = -1;
            this->last_coord.y = -1;
            this->sum_time += curr_time-this->last_click_time;
        }

        this->bActive ^= 1;  //toggle active flag

        if (curr_time-double_click_timewin <= this->last_click_time) { //double click handling
            osd_odometer_reset(opc,0);
        }

        this->last_click_time = curr_time;

        osd_odometer_draw(opc, nav,NULL);
    }
}


static int osd_odometer_save(struct navit* nav) {
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
        if(((struct odometer*)((struct osd_priv_common *)(list->data))->data)->name) {
            char*odo_str = osd_odometer_to_string((struct odometer*)((struct osd_priv_common *)(list->data))->data);
            fprintf(f,"%s",odo_str);
            g_free(odo_str);
        }
        list = g_list_next(list);
    }
    fclose(f);
    return TRUE;
}


static void osd_odometer_init(struct osd_priv_common *opc, struct navit *nav) {
    struct odometer *this = (struct odometer *)opc->data;

    osd_set_std_graphic(nav, &opc->osd_item, (struct osd_priv *)opc);

    this->orange = graphics_gc_new(opc->osd_item.gr);
    graphics_gc_set_foreground(this->orange, &this->idle_color);
    graphics_gc_set_linewidth(this->orange, this->width);

    opc->osd_item.graphic_fg = graphics_gc_new(opc->osd_item.gr);
    graphics_gc_set_foreground(opc->osd_item.graphic_fg, &opc->osd_item.text_color);
    graphics_gc_set_linewidth(opc->osd_item.graphic_fg, this->width);

    graphics_gc_set_linewidth(opc->osd_item.graphic_fg, this->width);

    navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_odometer_draw), attr_position_coord_geo, opc));

    navit_add_callback(nav, this->click_cb = callback_new_attr_1(callback_cast (osd_odometer_click), attr_button, opc));

    if(this->autosave_period>0) {
        event_add_timeout(this->autosave_period*1000, 1, callback_new_1(callback_cast(osd_odometer_save), NULL));
    }

    if(this->bAutoStart) {
        this->bActive = 1;
    }
    osd_odometer_draw(opc, nav, NULL);
}

static void osd_odometer_destroy(struct navit* nav) {
    if(!odometers_saved) {
        odometers_saved = 1;
        osd_odometer_save(NULL);
    }
}

static struct osd_priv *osd_odometer_new(struct navit *nav, struct osd_methods *meth,
        struct attr **attrs) {
    FILE* f;
    char* fn;

    struct odometer *this = g_new0(struct odometer, 1);
    struct osd_priv_common *opc = g_new0(struct osd_priv_common,1);
    struct attr *attr;
    struct color orange_color= {0xffff,0xa5a5,0x0000,0xffff};

    opc->data = (void*)this;
    opc->osd_item.rel_x = 120;
    opc->osd_item.rel_y = 20;
    opc->osd_item.rel_w = 60;
    opc->osd_item.rel_h = 80;
    opc->osd_item.navit = nav;
    opc->osd_item.font_size = 200;
    opc->osd_item.meth.draw = osd_draw_cast(osd_odometer_draw);
    meth->set_attr = set_std_osd_attr;

    this->bActive         = 0; //do not count on init
    this->sum_dist        = 0;
    this->max_speed       = 0;
    this->last_click_time = time(0);
    this->last_coord.x    = -1;
    this->last_coord.y    = -1;

    attr = attr_search(attrs, NULL, attr_label);
    //FIXME find some way to free text!!!!
    if (attr) {
        this->text = g_strdup(attr->u.str);
    } else
        this->text = NULL;

    attr = attr_search(attrs, NULL, attr_name);
    //FIXME find some way to free text!!!!
    if (attr) {
        this->name = g_strdup(attr->u.str);
    } else
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

    attr = attr_search(attrs, NULL, attr_align);
    if (attr)
        this->align=attr->u.num;

    osd_set_std_attr(attrs, &opc->osd_item, ITEM_HAS_TEXT);
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
            if(fgets(str,128,f)) {
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

    if(b_commandtable_added == 0) {
        navit_command_add_table(nav, commands, sizeof(commands)/sizeof(struct command_table));
        b_commandtable_added = 1;
    }
    navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_odometer_init), attr_graphics_ready, opc));
    navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_odometer_destroy), attr_destroy, nav));
    odometer_list = g_list_append(odometer_list, opc);

    return (struct osd_priv *) opc;
}


struct cmd_interface {
    int width;
    struct graphics_gc *orange;
    int update_period;   //in sec
    char* text;
    struct graphics_image *img;
    char*img_filename;
    char* command;
    int bReserved;
};

static void osd_cmd_interface_draw(struct osd_priv_common *opc, struct navit *nav,
                                   struct vehicle *v) {
    struct cmd_interface *this = (struct cmd_interface *)opc->data;

    struct point p;
    struct point bbox[4];
    struct graphics_gc *curr_color;
    struct attr navit;
    p.x = 0;
    p.y = 0;
    navit.type=attr_navit;
    navit.u.navit = opc->osd_item.navit;

    if(0==this->bReserved) {
        this->bReserved = 1;
        command_evaluate(&navit, this->command);
        this->bReserved = 0;
    }

    osd_fill_with_bgcolor(&opc->osd_item);

    //display image
    if(this->img) {
        graphics_draw_image(opc->osd_item.gr, opc->osd_item.graphic_bg, &p, this->img);
    }

    //display text
    graphics_get_text_bbox(opc->osd_item.gr, opc->osd_item.font, this->text, 0x10000, 0, bbox, 0);
    p.x=(opc->osd_item.w-bbox[2].x)/2;
    p.y = opc->osd_item.h-opc->osd_item.h/10;
    curr_color = opc->osd_item.graphic_fg;
    if(this->text)
        draw_multiline_osd_text(this->text,&opc->osd_item, curr_color);
    graphics_draw_mode(opc->osd_item.gr, draw_mode_end);
}



static void osd_cmd_interface_init(struct osd_priv_common *opc, struct navit *nav) {
    struct cmd_interface *this = (struct cmd_interface *)opc->data;

    osd_set_std_graphic(nav, &opc->osd_item, (struct osd_priv *)opc);

    opc->osd_item.graphic_fg = graphics_gc_new(opc->osd_item.gr);
    graphics_gc_set_foreground(opc->osd_item.graphic_fg, &opc->osd_item.text_color);
    graphics_gc_set_linewidth(opc->osd_item.graphic_fg, this->width);


    graphics_gc_set_linewidth(opc->osd_item.graphic_fg, this->width);

    if(this->update_period>0) {
        event_add_timeout(this->update_period*1000, 1, callback_new_1(callback_cast(osd_cmd_interface_draw), opc));
    }

    navit_add_callback(nav, callback_new_attr_1(callback_cast (osd_std_click), attr_button, &opc->osd_item));

    this->text = g_strdup("");
}

static int osd_cmd_interface_set_attr(struct osd_priv_common *opc, struct attr* attr) {
    struct cmd_interface *this_ = (struct cmd_interface *)opc->data;

    if(NULL==attr || NULL==this_) {
        return 0;
    }

    if(attr->type == attr_status_text) {
        if(this_->text) {
            g_free(this_->text);
        }
        if(attr->u.str) {
            this_->text = g_strdup(attr->u.str);
        }
        return 1;
    }
    if(attr->type == attr_src) {
        if(attr->u.str) {
            if((!this_->img_filename) || strcmp(this_->img_filename, graphics_icon_path(attr->u.str))) {
                //destroy old img, create new  image
                if(this_->img) {
                    graphics_image_free(opc->osd_item.gr, this_->img);
                }
                if(this_->img_filename) {
                    g_free(this_->img_filename);
                }
                this_->img_filename = graphics_icon_path(attr->u.str);
                this_->img = graphics_image_new(opc->osd_item.gr, this_->img_filename);
            }
        }
        return 1;
    }
    return 0;
}


static struct osd_priv *osd_cmd_interface_new(struct navit *nav, struct osd_methods *meth,
        struct attr **attrs) {
    struct cmd_interface *this = g_new0(struct cmd_interface, 1);
    struct osd_priv_common *opc = g_new0(struct osd_priv_common,1);
    struct attr *attr;

    opc->data = (void*)this;
    opc->osd_item.rel_x = 120;
    opc->osd_item.rel_y = 20;
    opc->osd_item.rel_w = 60;
    opc->osd_item.rel_h = 80;
    opc->osd_item.navit = nav;
    opc->osd_item.font_size = 200;
    opc->osd_item.meth.draw = osd_draw_cast(osd_cmd_interface_draw);

    opc->spec_set_attr_func = osd_cmd_interface_set_attr;
    meth->set_attr = set_std_osd_attr;

    osd_set_std_attr(attrs, &opc->osd_item, ITEM_HAS_TEXT);

    attr = attr_search(attrs, NULL, attr_width);
    this->width=attr ? attr->u.num : 2;

    attr = attr_search(attrs, NULL, attr_update_period);
    this->update_period=attr ? attr->u.num : 5; //default update period is 5 seconds

    attr = attr_search(attrs, NULL, attr_command);
    this->command = attr ? g_strdup(attr->u.str) : g_strdup("");

    if(b_commandtable_added == 0) {
        navit_command_add_table(nav, commands, sizeof(commands)/sizeof(struct command_table));
        b_commandtable_added = 1;
    }
    navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_cmd_interface_init), attr_graphics_ready, opc));
    return (struct osd_priv *) opc;
}




struct stopwatch {
    int width;
    struct graphics_gc *orange;
    struct callback *click_cb;
    struct color idle_color;    //text color when counter is idle

    int bDisableReset;
    int bActive;                //counting or not
    time_t current_base_time;   //base time of currently measured time interval
    time_t sum_time;            //sum of previous time intervals (except current intervals)
    time_t last_click_time;     //time of last click (for double click handling)
};

static void osd_stopwatch_draw(struct osd_priv_common *opc, struct navit *nav,
                               struct vehicle *v) {
    struct stopwatch *this = (struct stopwatch *)opc->data;

    struct graphics_gc *curr_color;
    char buffer[32]="00:00:00";
    struct point p;
    struct point bbox[4];
    time_t total_sec,total_min,total_hours,total_days;
    total_sec = this->sum_time;

    osd_fill_with_bgcolor(&opc->osd_item);

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

    graphics_get_text_bbox(opc->osd_item.gr, opc->osd_item.font, buffer, 0x10000, 0, bbox, 0);
    p.x=(opc->osd_item.w-bbox[2].x)/2;
    p.y = opc->osd_item.h-opc->osd_item.h/10;

    curr_color = this->bActive?opc->osd_item.graphic_fg:this->orange;
    graphics_draw_text(opc->osd_item.gr, curr_color, NULL, opc->osd_item.font, buffer, &p, 0x10000, 0);
    graphics_draw_mode(opc->osd_item.gr, draw_mode_end);
}


static void osd_stopwatch_click(struct osd_priv_common *opc, struct navit *nav, int pressed, int button,
                                struct point *p) {
    struct stopwatch *this = (struct stopwatch *)opc->data;

    struct point bp = opc->osd_item.p;
    osd_wrap_point(&bp, nav);
    if ((p->x < bp.x || p->y < bp.y || p->x > bp.x + opc->osd_item.w || p->y > bp.y + opc->osd_item.h
            || !opc->osd_item.configured ) && !opc->osd_item.pressed)
        return;
    if (button != 1)
        return;
    if (navit_ignore_button(nav))
        return;
    if (!!pressed == !!opc->osd_item.pressed)
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

    osd_stopwatch_draw(opc, nav,NULL);
}


static void osd_stopwatch_init(struct osd_priv_common *opc, struct navit *nav) {
    struct stopwatch *this = (struct stopwatch *)opc->data;

    osd_set_std_graphic(nav, &opc->osd_item, (struct osd_priv *)opc);

    this->orange = graphics_gc_new(opc->osd_item.gr);
    graphics_gc_set_foreground(this->orange, &this->idle_color);
    graphics_gc_set_linewidth(this->orange, this->width);

    opc->osd_item.graphic_fg = graphics_gc_new(opc->osd_item.gr);
    graphics_gc_set_foreground(opc->osd_item.graphic_fg, &opc->osd_item.text_color);
    graphics_gc_set_linewidth(opc->osd_item.graphic_fg, this->width);


    graphics_gc_set_linewidth(opc->osd_item.graphic_fg, this->width);

    event_add_timeout(500, 1, callback_new_1(callback_cast(osd_stopwatch_draw), opc));

    navit_add_callback(nav, this->click_cb = callback_new_attr_1(callback_cast (osd_stopwatch_click), attr_button, opc));

    osd_stopwatch_draw(opc, nav, NULL);
}

static struct osd_priv *osd_stopwatch_new(struct navit *nav, struct osd_methods *meth,
        struct attr **attrs) {
    struct stopwatch *this = g_new0(struct stopwatch, 1);
    struct osd_priv_common *opc = g_new0(struct osd_priv_common,1);
    struct attr *attr;
    struct color orange_color= {0xffff,0xa5a5,0x0000,0xffff};

    opc->data = (void*)this;
    opc->osd_item.rel_x = 120;
    opc->osd_item.rel_y = 20;
    opc->osd_item.rel_w = 60;
    opc->osd_item.rel_h = 80;
    opc->osd_item.navit = nav;
    opc->osd_item.font_size = 200;
    opc->osd_item.meth.draw = osd_draw_cast(osd_stopwatch_draw);
    meth->set_attr = set_std_osd_attr;

    this->bActive = 0; //do not count on init
    this->current_base_time = 0;
    this->sum_time = 0;
    this->last_click_time = 0;

    osd_set_std_attr(attrs, &opc->osd_item, ITEM_HAS_TEXT);
    attr = attr_search(attrs, NULL, attr_width);
    this->width=attr ? attr->u.num : 2;
    attr = attr_search(attrs, NULL, attr_idle_color);
    this->idle_color=attr ? *attr->u.color : orange_color; // text idle_color defaults to orange
    attr = attr_search(attrs, NULL, attr_disable_reset);
    if (attr)
        this->bDisableReset = attr->u.num;
    else
        this->bDisableReset = 0;

    navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_stopwatch_init), attr_graphics_ready, opc));
    return (struct osd_priv *) opc;
}

/**
 * @brief Draw the compass on the OSD (includes north and destination direction)
 *
 * @param opc A contextual private data pointer (see struct osd_priv_common)
 * @param nav The global navit object
 * @param v The current vehicle
 */
static void osd_compass_draw(struct osd_priv_common *opc, struct navit *nav,
                             struct vehicle *v) {
    struct compass *this = (struct compass *)opc->data;

    struct point p,bbox[4];
    struct attr attr_dir, destination_attr, position_attr, imperial_attr;
    double dir, vdir = 0;
    char *buffer;
    struct coord c1, c2;
    enum projection pro;
    int imperial=0;

    if (navit_get_attr(nav, attr_imperial, &imperial_attr, NULL))
        imperial=imperial_attr.u.num;

    osd_fill_with_bgcolor(&opc->osd_item);
    p.x = opc->osd_item.w/2;
    p.y = opc->osd_item.w/2;
    graphics_draw_circle(opc->osd_item.gr,
                         opc->osd_item.graphic_fg, &p, opc->osd_item.w*5/6);
    if (v) {
        if (vehicle_get_attr(v, attr_position_direction, &attr_dir, NULL)) {
            vdir = *attr_dir.u.numd;
            draw_compass(opc->osd_item.gr, this->north_gc, opc->osd_item.graphic_fg, &p, opc->osd_item.w/3,
                         -vdir); /* Draw a compass */
        }

        if (navit_get_attr(nav, attr_destination, &destination_attr, NULL)
                && vehicle_get_attr(v, attr_position_coord_geo,&position_attr, NULL)) {
            pro = destination_attr.u.pcoord->pro;
            transform_from_geo(pro, position_attr.u.coord_geo, &c1);
            c2.x = destination_attr.u.pcoord->x;
            c2.y = destination_attr.u.pcoord->y;
            dir = atan2(c2.x - c1.x, c2.y - c1.y) * 180.0 / M_PI;
            dir -= vdir;
            draw_handle(opc->osd_item.gr, this->destination_dir_gc, &p, opc->osd_item.w/3,
                        dir); /* Draw the green arrow pointing to the destination */
            buffer=format_distance(transform_distance(pro, &c1, &c2),"",imperial);
            graphics_get_text_bbox(opc->osd_item.gr, opc->osd_item.font, buffer, 0x10000, 0, bbox, 0);
            p.x=(opc->osd_item.w-bbox[2].x)/2;
            p.y = opc->osd_item.h-opc->osd_item.h/10;
            graphics_draw_text(opc->osd_item.gr, this->destination_dir_gc, NULL, opc->osd_item.font, buffer, &p, 0x10000, 0);
            g_free(buffer);
        }
    }
    graphics_draw_mode(opc->osd_item.gr, draw_mode_end);
}



static void osd_compass_init(struct osd_priv_common *opc, struct navit *nav) {
    struct compass *this = (struct compass *)opc->data;

    osd_set_std_graphic(nav, &opc->osd_item, (struct osd_priv *)opc);

    this->destination_dir_gc = graphics_gc_new(opc->osd_item.gr);
    graphics_gc_set_foreground(this->destination_dir_gc, &this->destination_dir_color);
    graphics_gc_set_linewidth(this->destination_dir_gc, this->width);

    this->north_gc = graphics_gc_new(opc->osd_item.gr);
    graphics_gc_set_foreground(this->north_gc, &this->north_color);
    graphics_gc_set_linewidth(this->north_gc, this->width);

    opc->osd_item.graphic_fg = graphics_gc_new(opc->osd_item.gr);
    graphics_gc_set_foreground(opc->osd_item.graphic_fg, &opc->osd_item.text_color);
    graphics_gc_set_linewidth(opc->osd_item.graphic_fg, this->width);

    navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_compass_draw), attr_position_coord_geo, opc));
    if (opc->osd_item.command)
        navit_add_callback(nav, this->click_cb = callback_new_attr_1(callback_cast (osd_std_click), attr_button,
                           &opc->osd_item));

    osd_compass_draw(opc, nav, NULL);
}

static struct osd_priv *osd_compass_new(struct navit *nav, struct osd_methods *meth,
                                        struct attr **attrs) {
    struct compass *this = g_new0(struct compass, 1);
    struct osd_priv_common *opc = g_new0(struct osd_priv_common,1);
    struct attr *attr;
    struct color green_color= {0x0400,0xffff,0x1000,0xffff};
    struct color red_color= {0xffff,0x0400,0x0400,0xffff};

    opc->data = (void*)this;
    opc->osd_item.rel_x = 20;
    opc->osd_item.rel_y = 20;
    opc->osd_item.rel_w = 60;
    opc->osd_item.rel_h = 80;
    opc->osd_item.navit = nav;
    opc->osd_item.font_size = 200;
    opc->osd_item.meth.draw = osd_draw_cast(osd_compass_draw);
    meth->set_attr = set_std_osd_attr;
    osd_set_std_attr(attrs, &opc->osd_item, ITEM_HAS_TEXT);
    attr = attr_search(attrs, NULL, attr_width);
    this->width=attr ? attr->u.num : 2;
    attr = attr_search(attrs, NULL, attr_destination_dir_color);
    this->destination_dir_color=attr ? *attr->u.color :
                                green_color; /* Pick destination color from configuration, default to green if unspecified */
    attr = attr_search(attrs, NULL, attr_north_color);
    this->north_color=attr ? *attr->u.color :
                      red_color; /* Pick north handle color from configuration, default to red if unspecified */

    navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_compass_init), attr_graphics_ready, opc));
    return (struct osd_priv *) opc;
}

struct osd_button {
    int use_overlay;
    /* FIXME: do we need navit_init_cb? It is set in two places but never read.
     * osd_button_new sets it to osd_button_init (init callback), and
     * osd_button_init sets it to osd_std_click (click callback). */
    struct callback *draw_cb,*navit_init_cb;
    struct graphics_image *img;
    char *src_dir,*src;
};


/**
 * @brief Adjusts width and height of an OSD item to fit the image it displays.
 *
 * A width or height of 0%, stored in relative attributes as {@code ATTR_REL_RELSHIFT}, is used as a flag
 * indicating that the respective dimension is unset, i.e. determined by the dimensions of its image.
 *
 * If this is the case for height and/or width, the respective dimension will be updated to fit the image.
 *
 * Note that this method is used by several OSD items, notably {@code osd_image}, {@code osd_button} and
 * {@code osd_android_menu}.
 *
 * @param opc The OSD item
 * @param img The image displayed by the item
 */
static void osd_button_adjust_sizes(struct osd_priv_common *opc, struct graphics_image *img) {
    if(opc->osd_item.rel_w==ATTR_REL_RELSHIFT)
        opc->osd_item.w=img->width;
    if(opc->osd_item.rel_h==ATTR_REL_RELSHIFT)
        opc->osd_item.h=img->height;
}

static void osd_button_draw(struct osd_priv_common *opc, struct navit *nav) {
    struct osd_button *this = (struct osd_button *)opc->data;

    // FIXME: Do we need this check?
    if(navit_get_blocked(nav)&1)
        return;

    struct point p;

    if (this->use_overlay) {
        struct graphics_image *img;
        img=graphics_image_new_scaled(opc->osd_item.gr, this->src, opc->osd_item.w, opc->osd_item.h);
        osd_button_adjust_sizes(opc, img);
        p.x=(opc->osd_item.w-img->width)/2;
        p.y=(opc->osd_item.h-img->height)/2;
        osd_fill_with_bgcolor(&opc->osd_item);
        graphics_draw_image(opc->osd_item.gr, opc->osd_item.graphic_bg, &p, img);
        graphics_image_free(opc->osd_item.gr, img);
    } else {
        struct graphics *gra;
        gra = navit_get_graphics(nav);
        this->img = graphics_image_new_scaled(gra, this->src, opc->osd_item.w, opc->osd_item.h);

        if (!this->img) {
            dbg(lvl_warning, "failed to load '%s'", this->src);
            return;
        }

        osd_std_calculate_sizes(&opc->osd_item, navit_get_width(nav), navit_get_height(nav));
        osd_button_adjust_sizes(opc, this->img);

        p = opc->osd_item.p;
        p.x+=(opc->osd_item.w-this->img->width)/2;
        p.y+=(opc->osd_item.h-this->img->height)/2;

        if (!opc->osd_item.configured)
            return;

        graphics_draw_image(opc->osd_item.gr, opc->osd_item.graphic_bg, &p, this->img);
    }
}

static void osd_button_init(struct osd_priv_common *opc, struct navit *nav) {
    struct osd_button *this = (struct osd_button *)opc->data;

    struct graphics *gra = navit_get_graphics(nav);

    /* translate properties to real size */
    osd_std_calculate_sizes(&opc->osd_item, navit_get_width(nav), navit_get_height(nav));
    /* most graphics plugins cannot accept w=0 or h=0. They require special w=-1 or h=-1 for "no size"*/
    if((opc->osd_item.w <= 0) || (opc->osd_item.h <=0)) {
        opc->osd_item.w = -1;
        opc->osd_item.h = -1;
    }
    dbg(lvl_debug, "enter");
    dbg(lvl_debug, "Get: %s, %d, %d, %d, %d", this->src, opc->osd_item.rel_w, opc->osd_item.rel_h, opc->osd_item.w,
        opc->osd_item.h);
    this->img = graphics_image_new_scaled(gra, this->src, opc->osd_item.w, opc->osd_item.h);
    if (!this->img) {
        dbg(lvl_warning, "failed to load '%s'", this->src);
        return;
    } else {
        dbg(lvl_debug,"Got %s: %d, %d", this->src, this->img->width, this->img->height);
    }
    osd_button_adjust_sizes(opc, this->img);
    if (this->use_overlay) {
        struct graphics_image *img;
        struct point p;
        osd_set_std_graphic(nav, &opc->osd_item, (struct osd_priv *)opc);
        img=graphics_image_new_scaled(opc->osd_item.gr, this->src, opc->osd_item.w, opc->osd_item.h);
        p.x=(opc->osd_item.w-this->img->width)/2;
        p.y=(opc->osd_item.h-this->img->height)/2;
        osd_fill_with_bgcolor(&opc->osd_item);
        graphics_draw_image(opc->osd_item.gr, opc->osd_item.graphic_bg, &p, img);
        graphics_draw_mode(opc->osd_item.gr, draw_mode_end);
        graphics_image_free(opc->osd_item.gr, img);
    } else {
        osd_set_std_config(nav, &opc->osd_item);
        osd_set_keypress(nav, &opc->osd_item);
        opc->osd_item.gr=gra;
        opc->osd_item.graphic_bg=graphics_gc_new(opc->osd_item.gr);
        graphics_add_callback(gra, this->draw_cb=callback_new_attr_2(callback_cast(osd_button_draw), attr_postdraw, opc, nav));
    }
    navit_add_callback(nav, this->navit_init_cb = callback_new_attr_1(callback_cast (osd_std_click), attr_button,
                       &opc->osd_item));
    osd_button_draw(opc,nav);
}

static char *osd_button_icon_path(struct osd_button *this_, char *src) {
    if (!this_->src_dir)
        return graphics_icon_path(src);
    return g_strdup_printf("%s%s%s", this_->src_dir, G_DIR_SEPARATOR_S, src);
}

int osd_button_set_attr(struct osd_priv_common *opc, struct attr* attr) {
    struct osd_button *this_ = (struct osd_button *)opc->data;

    if(NULL==attr || NULL==this_) {
        return 0;
    }
    if(attr->type == attr_src) {
        struct navit *nav;
        struct graphics *gra;
        if(this_->src) {
            g_free(this_->src);
        }
        if(attr->u.str) {
            this_->src = osd_button_icon_path(this_, attr->u.str);
        }
        nav = opc->osd_item.navit;
        gra = navit_get_graphics(nav);
        this_->img = graphics_image_new_scaled(gra, this_->src, opc->osd_item.w, opc->osd_item.h);
        if (!this_->img) {
            dbg(lvl_warning, "failed to load '%s'", this_->src);
            return 0;
        }

        if(navit_get_blocked(nav)&1)
            return 1;

        osd_button_draw(opc,nav);
        navit_draw(opc->osd_item.navit);
        return 1;
    }
    return 0;
}



static struct osd_priv *osd_button_new(struct navit *nav, struct osd_methods *meth,
                                       struct attr **attrs) {
    struct osd_button *this = g_new0(struct osd_button, 1);
    struct osd_priv_common *opc = g_new0(struct osd_priv_common,1);
    struct attr *attr;

    opc->data = (void*)this;
    opc->osd_item.navit = nav;
    opc->osd_item.meth.draw = osd_draw_cast(osd_button_draw);
    /*Value of 0% is stored in relative attributes as ATTR_REL_RELSHIFT, we use this value as "width/height unset" flag */
    opc->osd_item.rel_w = ATTR_REL_RELSHIFT;
    opc->osd_item.rel_h = ATTR_REL_RELSHIFT;

    meth->set_attr = set_std_osd_attr;
    opc->spec_set_attr_func = osd_button_set_attr;

    attr=attr_search(attrs, NULL, attr_use_overlay);
    if (attr)
        this->use_overlay=attr->u.num;

    osd_set_std_attr(attrs, &opc->osd_item, this->use_overlay ? TRANSPARENT_BG:(TRANSPARENT_BG|DISABLE_OVERLAY));

    if (!opc->osd_item.command) {
        dbg(lvl_error, "no command");
        goto error;
    }
    attr = attr_search(attrs, NULL, attr_src_dir);
    if (attr)
        this->src_dir=graphics_icon_path(attr->u.str);
    else
        this->src_dir=NULL;
    attr = attr_search(attrs, NULL, attr_src);
    if (!attr) {
        dbg(lvl_error, "no src");
        goto error;
    }

    this->src = osd_button_icon_path(this, attr->u.str);

    navit_add_callback(nav, this->navit_init_cb = callback_new_attr_1(callback_cast (osd_button_init), attr_graphics_ready,
                       opc));

    if(b_commandtable_added == 0) {
        navit_command_add_table(nav, commands, sizeof(commands)/sizeof(struct command_table));
        b_commandtable_added = 1;
    }

    return (struct osd_priv *) opc;
error:
    g_free(this);
    g_free(opc);
    return NULL;
}

static void osd_image_init(struct osd_priv_common *opc, struct navit *nav) {
    struct osd_button *this = (struct osd_button *)opc->data;

    struct graphics *gra = navit_get_graphics(nav);
    dbg(lvl_debug, "enter");
    this->img = graphics_image_new(gra, this->src);
    if (!this->img) {
        dbg(lvl_warning, "failed to load '%s'", this->src);
        return;
    }
    osd_button_adjust_sizes(opc, this->img);
    if (this->use_overlay) {
        struct graphics_image *img;
        struct point p;
        osd_set_std_graphic(nav, &opc->osd_item, (struct osd_priv *)opc);
        img=graphics_image_new(opc->osd_item.gr, this->src);
        p.x=(opc->osd_item.w-this->img->width)/2;
        p.y=(opc->osd_item.h-this->img->height)/2;
        osd_fill_with_bgcolor(&opc->osd_item);
        graphics_draw_image(opc->osd_item.gr, opc->osd_item.graphic_bg, &p, img);
        graphics_draw_mode(opc->osd_item.gr, draw_mode_end);
        graphics_image_free(opc->osd_item.gr, img);
    } else {
        osd_set_std_config(nav, &opc->osd_item);
        opc->osd_item.gr=gra;
        opc->osd_item.graphic_bg=graphics_gc_new(opc->osd_item.gr);
        graphics_add_callback(gra, this->draw_cb=callback_new_attr_2(callback_cast(osd_button_draw), attr_postdraw, opc, nav));
    }
    osd_button_draw(opc,nav);
}

static struct osd_priv *osd_image_new(struct navit *nav, struct osd_methods *meth,
                                      struct attr **attrs) {
    struct osd_button *this = g_new0(struct osd_button, 1);
    struct osd_priv_common *opc = g_new0(struct osd_priv_common,1);
    struct attr *attr;

    opc->data = (void*)this;
    opc->osd_item.navit = nav;
    opc->osd_item.meth.draw = osd_draw_cast(osd_button_draw);
    /*Value of 0% is stored in relative attributes as ATTR_REL_RELSHIFT, we use this value as "width/height unset" flag */
    opc->osd_item.rel_w = ATTR_REL_RELSHIFT;
    opc->osd_item.rel_h = ATTR_REL_RELSHIFT;
    meth->set_attr = set_std_osd_attr;
    opc->spec_set_attr_func = osd_button_set_attr;

    attr=attr_search(attrs, NULL, attr_use_overlay);
    if (attr)
        this->use_overlay=attr->u.num;


    osd_set_std_attr(attrs, &opc->osd_item, this->use_overlay ? TRANSPARENT_BG:(TRANSPARENT_BG|DISABLE_OVERLAY));

    attr = attr_search(attrs, NULL, attr_src);
    if (!attr) {
        dbg(lvl_error, "no src");
        goto error;
    }

    this->src = graphics_icon_path(attr->u.str);

    navit_add_callback(nav, this->navit_init_cb = callback_new_attr_1(callback_cast (osd_image_init), attr_graphics_ready,
                       opc));

    return (struct osd_priv *) opc;
error:
    g_free(opc);
    g_free(this);
    return NULL;
}


/**
 * Internal data for {@code navigation_status} OSD.
 */
struct navigation_status {
    char *icon_src;		/**< Source for icon, with a placeholder */
    int icon_h;
    int icon_w;
    int last_status;	/**< Last status displayed.
						     Apart from the usual values of {@code nav_status}, -2 is used to
						     indicate we have not yet received a status. */
};


/**
 * @brief Draws a `navigation_status` OSD.
 *
 * This method performs the actual operation of selecting and drawing the image. It can be called
 * directly as a callback method for the `navigation.nav_status` attribute, or indirectly through the
 * draw method.
 *
 * @param opc The OSD to draw
 * @param status The status of the navigation engine (the value of the {@code nav_status} attribute)
 */
static void osd_navigation_status_draw_do(struct osd_priv_common *opc, int status) {
    struct navigation_status *this = (struct navigation_status *)opc->data;
    struct point p;
    int do_draw = opc->osd_item.do_draw;
    struct graphics_image *gr_image;
    char *image;

    /* When we're routing, the status will flip from 4 (routing) to 3 (recalculating) and back on
     * every position update. This hack prevents unnecessary (and even undesirable) updates.
     */
    int status2 = (status == 3) ? 4 : status;


    if ((status2 != this->last_status) && (status2 != status_invalid)) {
        this->last_status = status2;
        do_draw = 1;
    }

    if (do_draw) {
        osd_fill_with_bgcolor(&opc->osd_item);
        image = g_strdup_printf(this->icon_src, nav_status_to_text(status2));
        dbg(lvl_debug, "image=%s", image);
        gr_image =
            graphics_image_new_scaled(opc->osd_item.gr,
                                      image, this->icon_w,
                                      this->icon_h);
        if (!gr_image) {
            dbg(lvl_error,"failed to load %s in %dx%d",image,this->icon_w,this->icon_h);
            g_free(image);
            image = graphics_icon_path("unknown.png");
            gr_image =
                graphics_image_new_scaled(opc->
                                          osd_item.gr,
                                          image,
                                          this->icon_w,
                                          this->
                                          icon_h);
        }
        dbg(lvl_debug, "gr_image=%p", gr_image);
        if (gr_image) {
            p.x =
                (opc->osd_item.w -
                 gr_image->width) / 2;
            p.y =
                (opc->osd_item.h -
                 gr_image->height) / 2;
            graphics_draw_image(opc->osd_item.gr,
                                opc->osd_item.
                                graphic_fg, &p,
                                gr_image);
            graphics_image_free(opc->osd_item.gr,
                                gr_image);
        }
        g_free(image);
        graphics_draw_mode(opc->osd_item.gr, draw_mode_end);
    }
}


/**
 * @brief Draws a `navigation_status` OSD.
 *
 * This is the draw method for the OSD. It exposes the standard signature for the `draw` method and acts
 * as a wrapper around `osd_navigation_status_draw_do()`.
 *
 * @param osd The OSD to draw.
 * @param navit The navit instance
 * @param v The vehicle (not used but part of the prototype)
 */
static void osd_navigation_status_draw(struct osd_priv *osd, struct navit *navit, struct vehicle *v) {
    struct navigation *nav = NULL;
    struct attr attr;

    if (navit)
        nav = navit_get_navigation(navit);
    if (nav) {
        if (navigation_get_attr(nav, attr_nav_status, &attr, NULL))
            osd_navigation_status_draw_do((struct osd_priv_common *) osd, attr.u.num);
    }
}


/**
 * @brief Initializes a new {@code navigation_status} OSD.
 *
 * This function is registered as a callback function in {@link osd_navigation_status_new(struct navit *, struct osd_methods *, struct attr **)}.
 * It is called after graphics initialization has finished and can be used for any initialization
 * tasks which rely on a functional graphics system.
 *
 * @param opc The OSD to initialize
 * @param navit The navit instance
 */
static void osd_navigation_status_init(struct osd_priv_common *opc, struct navit *navit) {
    struct navigation *nav = NULL;
    struct attr attr;

    dbg(lvl_debug, "enter, opc=%p", opc);
    osd_set_std_graphic(navit, &opc->osd_item, (struct osd_priv *)opc);
    if (navit)
        nav = navit_get_navigation(navit);
    if (nav) {
        navigation_register_callback(nav, attr_nav_status, callback_new_attr_1(callback_cast(osd_navigation_status_draw_do),
                                     attr_nav_status, opc));
        if (navigation_get_attr(nav, attr_nav_status, &attr, NULL))
            osd_navigation_status_draw_do(opc, attr.u.num);
    } else
        dbg(lvl_error, "navigation instance is NULL, OSD will never update");
    //navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_std_click), attr_button, &opc->osd_item)); // FIXME do we need this?
}


/**
 * @brief Creates a new {@code navigation_status} OSD.
 *
 * This initializes the data structures and registers {@link osd_navigation_status_init(struct osd_priv_common *, struct navit *)}
 * as a callback.
 *
 * Note that this function runs before the graphics system has been initialized. Therefore, code
 * that requires a functional graphics system must be placed in
 * {@link osd_navigation_status_init(struct osd_priv_common *, struct navit *)}.
 *
 * @param nav The navit instance
 * @param meth The methods for the new OSD
 * @param attrs The attributes for the new OSD
 */
static struct osd_priv *osd_navigation_status_new(struct navit *nav, struct osd_methods *meth, struct attr **attrs) {
    struct navigation_status *this = g_new0(struct navigation_status, 1);
    struct osd_priv_common *opc = g_new0(struct osd_priv_common,1);
    struct attr *attr;

    opc->data = (void*)this;
    opc->osd_item.rel_x = 20;
    opc->osd_item.rel_y = -80;
    opc->osd_item.rel_w = 70;
    opc->osd_item.navit = nav;
    opc->osd_item.rel_h = 70;
    opc->osd_item.font_size = 200;  // FIXME may not be needed
    opc->osd_item.meth.draw = osd_draw_cast(osd_navigation_status_draw);
    meth->set_attr = set_std_osd_attr;
    osd_set_std_attr(attrs, &opc->osd_item, 0);

    this->icon_w = -1;
    this->icon_h = -1;
    this->last_status = status_invalid;

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

    navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_navigation_status_init), attr_graphics_ready, opc));
    return (struct osd_priv *) opc;
}


struct nav_next_turn {
    char *test_text;
    char *icon_src;
    int icon_h, icon_w, active;
    char *last_name;
    int level;
};

static void osd_nav_next_turn_draw(struct osd_priv_common *opc, struct navit *navit,
                                   struct vehicle *v) {
    struct nav_next_turn *this = (struct nav_next_turn *)opc->data;

    struct point p;
    int do_draw = opc->osd_item.do_draw;
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
        dbg(lvl_debug, "name=%s", name);
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
        osd_fill_with_bgcolor(&opc->osd_item);
        if (this->active) {
            image = g_strdup_printf(this->icon_src, name);
            dbg(lvl_debug, "image=%s", image);
            gr_image =
                graphics_image_new_scaled(opc->osd_item.gr,
                                          image, this->icon_w,
                                          this->icon_h);
            if (!gr_image) {
                dbg(lvl_error,"failed to load %s in %dx%d",image,this->icon_w,this->icon_h);
                g_free(image);
                image = graphics_icon_path("unknown.png");
                gr_image =
                    graphics_image_new_scaled(opc->
                                              osd_item.gr,
                                              image,
                                              this->icon_w,
                                              this->
                                              icon_h);
            }
            dbg(lvl_debug, "gr_image=%p", gr_image);
            if (gr_image) {
                p.x =
                    (opc->osd_item.w -
                     gr_image->width) / 2;
                p.y =
                    (opc->osd_item.h -
                     gr_image->height) / 2;
                graphics_draw_image(opc->osd_item.gr,
                                    opc->osd_item.
                                    graphic_fg, &p,
                                    gr_image);
                graphics_image_free(opc->osd_item.gr,
                                    gr_image);
            }
            g_free(image);
        }
        graphics_draw_mode(opc->osd_item.gr, draw_mode_end);
    }
}

static void osd_nav_next_turn_init(struct osd_priv_common *opc, struct navit *nav) {
    osd_set_std_graphic(nav, &opc->osd_item, (struct osd_priv *)opc);
    navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_nav_next_turn_draw), attr_position_coord_geo, opc));
    navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_std_click), attr_button, &opc->osd_item));
    osd_nav_next_turn_draw(opc, nav, NULL);
}

static struct osd_priv *osd_nav_next_turn_new(struct navit *nav, struct osd_methods *meth,
        struct attr **attrs) {
    struct nav_next_turn *this = g_new0(struct nav_next_turn, 1);
    struct osd_priv_common *opc = g_new0(struct osd_priv_common,1);
    struct attr *attr;

    opc->data = (void*)this;
    opc->osd_item.rel_x = 20;
    opc->osd_item.rel_y = -80;
    opc->osd_item.rel_w = 70;
    opc->osd_item.navit = nav;
    opc->osd_item.rel_h = 70;
    opc->osd_item.font_size = 200;
    opc->osd_item.meth.draw = osd_draw_cast(osd_nav_next_turn_draw);
    meth->set_attr = set_std_osd_attr;

    osd_set_std_attr(attrs, &opc->osd_item, 0);

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

    navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_nav_next_turn_init), attr_graphics_ready, opc));
    return (struct osd_priv *) opc;
}

struct nav_toggle_announcer {
    int w,h;
    /* FIXME this is actually the click callback, which is set once but never read. Do we need this? */
    struct callback *navit_init_cb;
    char *icon_src;
    int icon_h, icon_w, active, last_state;
};

static void osd_nav_toggle_announcer_draw(struct osd_priv_common *opc, struct navit *navit, struct vehicle *v) {
    struct nav_toggle_announcer *this = (struct nav_toggle_announcer *)opc->data;

    struct point p;
    int do_draw = opc->osd_item.do_draw;
    struct graphics_image *gr_image;
    char *path;
    char *gui_sound_off = "gui_sound_off";
    char *gui_sound_on = "gui_sound";
    struct attr attr, speechattr;

    if (!navit_get_attr(navit, attr_speech, &speechattr, NULL)) {
        dbg(lvl_error, "No speech plugin available, toggle_announcer disabled.");
        return;
    }
    if (!speech_get_attr(speechattr.u.speech, attr_active, &attr, NULL))
        attr.u.num = 1;
    this->active = attr.u.num;

    if(this->active != this->last_state) {
        this->last_state = this->active;
        do_draw = 1;
    }

    if (do_draw) {
        graphics_draw_mode(opc->osd_item.gr, draw_mode_begin);
        p.x = 0;
        p.y = 0;
        graphics_draw_rectangle(opc->osd_item.gr, opc->osd_item.graphic_bg, &p, opc->osd_item.w, opc->osd_item.h);

        if (this->active)
            path = g_strdup_printf(this->icon_src, gui_sound_on);
        else
            path = g_strdup_printf(this->icon_src, gui_sound_off);

        gr_image = graphics_image_new_scaled(opc->osd_item.gr, path, this->icon_w, this->icon_h);
        if (!gr_image) {
            g_free(path);
            path = graphics_icon_path("unknown.png");
            gr_image = graphics_image_new_scaled(opc->osd_item.gr, path, this->icon_w, this->icon_h);
        }

        dbg(lvl_debug, "gr_image=%p", gr_image);

        if (gr_image) {
            p.x = (opc->osd_item.w - gr_image->width) / 2;
            p.y = (opc->osd_item.h - gr_image->height) / 2;
            graphics_draw_image(opc->osd_item.gr, opc->osd_item.graphic_fg, &p, gr_image);
            graphics_image_free(opc->osd_item.gr, gr_image);
        }

        g_free(path);
        graphics_draw_mode(opc->osd_item.gr, draw_mode_end);
    }
}

static void osd_nav_toggle_announcer_init(struct osd_priv_common *opc, struct navit *nav) {
    struct nav_toggle_announcer *this = (struct nav_toggle_announcer *)opc->data;

    osd_set_std_graphic(nav, &opc->osd_item, (struct osd_priv *)opc);
    navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_nav_toggle_announcer_draw), attr_speech, opc));
    navit_add_callback(nav, this->navit_init_cb = callback_new_attr_1(callback_cast(osd_std_click), attr_button,
                       &opc->osd_item));
    osd_nav_toggle_announcer_draw(opc, nav, NULL);
}

static struct osd_priv *osd_nav_toggle_announcer_new(struct navit *nav, struct osd_methods *meth, struct attr **attrs) {
    struct nav_toggle_announcer *this = g_new0(struct nav_toggle_announcer, 1);
    struct osd_priv_common *opc = g_new0(struct osd_priv_common,1);
    struct attr *attr;
    char *command = "announcer_toggle()";

    opc->data = (void*)this;
    opc->osd_item.rel_w = 48;
    opc->osd_item.rel_h = 48;
    opc->osd_item.rel_x = -64;
    opc->osd_item.rel_y = 76;
    opc->osd_item.navit = nav;
    opc->osd_item.meth.draw = osd_draw_cast(osd_nav_toggle_announcer_draw);
    meth->set_attr = set_std_osd_attr;

    osd_set_std_attr(attrs, &opc->osd_item, 0);

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

    opc->osd_item.command = g_strdup(command);

    navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_nav_toggle_announcer_init), attr_graphics_ready, opc));
    return (struct osd_priv *) opc;
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
    int width;
    int flags;
    struct graphics_gc *orange;
    struct graphics_gc *red;
    struct color idle_color;

    int announce_on;
    enum osd_speed_warner_eAnnounceState announce_state;
    char *text;                 //text of label attribute for this osd
};

static double angle_diff(int firstAngle,int secondAngle) {
    double difference = secondAngle - firstAngle;
    while (difference < -180) difference += 360;
    while (difference > 180) difference -= 360;
    return difference;
}

static void osd_speed_cam_draw(struct osd_priv_common *opc, struct navit *navit, struct vehicle *v) {
    struct osd_speed_cam *this_ = (struct osd_speed_cam *)opc->data;

    struct attr position_attr,vehicle_attr,imperial_attr;
    struct point bbox[4];
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
    int imperial=0;

    if (navit_get_attr(navit, attr_imperial, &imperial_attr, NULL))
        imperial=imperial_attr.u.num;


    if(navit) {
        navit_get_attr(navit, attr_vehicle, &vehicle_attr, NULL);
    } else {
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
        struct attr attr;
        if(map_get_attr(map, attr_type, &attr, NULL)) {
            if( strcmp("csv", attr.u.str) && strcmp("binfile", attr.u.str)) {
                continue;
            }
        } else {
            continue;
        }
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

    if(bFound && (idx==-1 || this_->flags & (1<<(idx-1))) ) {
        dCurrDist = transform_distance(projection_mg, &curr_coord, &cam_coord);
        ret_attr = vehicle_get_attr(curr_vehicle,attr_position_speed,&speed_attr, NULL);
        if(0==ret_attr) {
            graphics_overlay_disable(opc->osd_item.gr,1);
            return;
        }
        if (opc->osd_item.configured) {
            graphics_overlay_disable(opc->osd_item.gr,0);
        }
        speed = *speed_attr.u.numd;
        if(dCurrDist <= speed*750.0/130.0) {  //at speed 130 distance limit is 750m
            if(this_->announce_state==eNoWarn && this_->announce_on) {
                this_->announce_state=eWarningTold; //warning told
                navit_say(navit, _("Look out! Camera!"));
            }
        } else {
            this_->announce_state=eNoWarn;
        }

        if(this_->text) {
            char buffer [256]="";
            char buffer2[256]="";
            char dir_str[16];
            char spd_str[16];
            buffer [0] = 0;
            buffer2[0] = 0;

            osd_fill_with_bgcolor(&opc->osd_item);

            str_replace(buffer,this_->text,"${distance}",format_distance(dCurrDist,"",imperial));
            str_replace(buffer2,buffer,"${camera_type}",(0<=idx && idx<=CAM_TRAFFIPAX)?camera_t_strs[idx]:"");
            str_replace(buffer,buffer2,"${camera_dir}",(0<=dir_idx && dir_idx<=CAMDIR_TWO)?camdir_t_strs[dir_idx]:"");
            sprintf(dir_str,"%d",dir);
            sprintf(spd_str,"%d",spd);
            str_replace(buffer2,buffer,"${direction}",dir_str);
            str_replace(buffer,buffer2,"${speed_limit}",spd_str);

            graphics_get_text_bbox(opc->osd_item.gr, opc->osd_item.font, buffer, 0x10000, 0, bbox, 0);
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
            } else if(dCurrDist <= speed*750.0/130.0) {
                curr_color = this_->red;
            }
            draw_multiline_osd_text(buffer,&opc->osd_item, curr_color);
            graphics_draw_mode(opc->osd_item.gr, draw_mode_end);
        }
    } else {
        graphics_overlay_disable(opc->osd_item.gr,1);
    }
}

static void osd_speed_cam_init(struct osd_priv_common *opc, struct navit *nav) {
    struct osd_speed_cam *this = (struct osd_speed_cam *)opc->data;

    struct color red_color= {0xffff,0x0000,0x0000,0xffff};
    osd_set_std_graphic(nav, &opc->osd_item, (struct osd_priv *)opc);

    this->red = graphics_gc_new(opc->osd_item.gr);
    graphics_gc_set_foreground(this->red, &red_color);
    graphics_gc_set_linewidth(this->red, this->width);

    this->orange = graphics_gc_new(opc->osd_item.gr);
    graphics_gc_set_foreground(this->orange, &this->idle_color);
    graphics_gc_set_linewidth(this->orange, this->width);

    opc->osd_item.graphic_fg = graphics_gc_new(opc->osd_item.gr);
    graphics_gc_set_foreground(opc->osd_item.graphic_fg, &opc->osd_item.text_color);
    graphics_gc_set_linewidth(opc->osd_item.graphic_fg, this->width);


    graphics_gc_set_linewidth(opc->osd_item.graphic_fg, this->width);

    navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_speed_cam_draw), attr_position_coord_geo, opc));

}

static struct osd_priv *osd_speed_cam_new(struct navit *nav, struct osd_methods *meth, struct attr **attrs) {

    struct color default_color= {0xffff,0xa5a5,0x0000,0xffff};

    struct osd_speed_cam *this = g_new0(struct osd_speed_cam, 1);
    struct osd_priv_common *opc = g_new0(struct osd_priv_common,1);
    struct attr *attr;

    opc->data = (void*)this;
    opc->osd_item.p.x = 120;
    opc->osd_item.p.y = 20;
    opc->osd_item.w = 60;
    opc->osd_item.h = 80;
    opc->osd_item.navit = nav;
    opc->osd_item.font_size = 200;
    opc->osd_item.meth.draw = osd_draw_cast(osd_speed_cam_draw);
    meth->set_attr = set_std_osd_attr;

    osd_set_std_attr(attrs, &opc->osd_item, ITEM_HAS_TEXT);
    attr = attr_search(attrs, NULL, attr_width);
    this->width=attr ? attr->u.num : 2;
    attr = attr_search(attrs, NULL, attr_idle_color);
    this->idle_color=attr ? *attr->u.color : default_color; // text idle_color defaults to orange

    attr = attr_search(attrs, NULL, attr_label);
    if (attr) {
        this->text = g_strdup(attr->u.str);
    } else
        this->text = NULL;

    attr = attr_search(attrs, NULL, attr_announce_on);
    if (attr) {
        this->announce_on = attr->u.num;
    } else {
        this->announce_on = 1;    //announce by default
    }

    attr = attr_search(attrs, NULL, attr_flags);
    if (attr) {
        this->flags = attr->u.num;
    } else {
        this->flags = -1;    //every cam type is on by default
    }

    navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_speed_cam_init), attr_graphics_ready, opc));
    return (struct osd_priv *) opc;
}

struct osd_speed_warner {
    struct graphics_gc *red;
    struct graphics_gc *green;
    struct graphics_gc *grey;
    struct graphics_gc *black;
    int width;
    int active;
    int d;
    double speed_exceed_limit_offset;
    double speed_exceed_limit_percent;
    int announce_on;
    enum osd_speed_warner_eAnnounceState announce_state;
    int bTextOnly;
    struct graphics_image *img_active,*img_passive,*img_off;
    char* label_str;
    int timeout;
    int wait_before_warn;
    struct callback *click_cb;
};

static void osd_speed_warner_draw(struct osd_priv_common *opc, struct navit *navit, struct vehicle *v) {
    struct osd_speed_warner *this = (struct osd_speed_warner *)opc->data;

    struct point p,bbox[4];
    char text[16]="";

    struct tracking *tracking = NULL;
    struct graphics_gc *osd_color=this->grey;
    struct graphics_image *img = this->img_off;


    osd_fill_with_bgcolor(&opc->osd_item);
    p.x=opc->osd_item.w/2-this->d/4;
    p.y=opc->osd_item.h/2-this->d/4;
    p.x=opc->osd_item.w/2;
    p.y=opc->osd_item.h/2;

    if (navit) {
        tracking = navit_get_tracking(navit);
    }
    if (tracking  && this->active ) {

        struct attr maxspeed_attr,speed_attr,imperial_attr;
        int *flags;
        double routespeed = -1;
        double tracking_speed = -1;
        int osm_data = 0;
        struct item *item;
        int imperial=0;

        item=tracking_get_current_item(tracking);

        if(navit) {
            if (navit_get_attr(navit, attr_imperial, &imperial_attr, NULL))
                imperial=imperial_attr.u.num;
        }

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
            char*routespeed_str = format_speed(routespeed,"","value",imperial);
            g_snprintf(text,16,"%s%s",osm_data ? "" : "~",routespeed_str);
            g_free(routespeed_str);
            if( this->speed_exceed_limit_offset+routespeed<tracking_speed &&
                    (100.0+this->speed_exceed_limit_percent)/100.0*routespeed<tracking_speed ) {
                if(this->announce_state==eNoWarn && this->announce_on) {
                    if(this->wait_before_warn>0) {
                        this->wait_before_warn--;
                    } else {
                        this->announce_state=eWarningTold; //warning told
                        navit_say(navit,_("Please decrease your speed"));
                    }
                }
            } else {
                /* reset speed warning */
                this->wait_before_warn = this->timeout;
            }
            if( tracking_speed <= routespeed ) {
                this->announce_state=eNoWarn; //no warning
                osd_color = this->green;
                img = this->img_passive;
            } else {
                osd_color = this->red;
                img = this->img_active;
            }
        } else {
            osd_color = this-> grey;
            img = this->img_off;
            this->announce_state = eNoWarn;
        }
    } else {
        //when tracking is not available display grey
        osd_color = this->grey;
        img = this->img_off;
        this->announce_state = eNoWarn;
    }
    if(this->img_active && this->img_passive && this->img_off) {
        struct point p;
        p.x=(opc->osd_item.w-img->width)/2;
        p.y=(opc->osd_item.h-img->height)/2;
        graphics_draw_image(opc->osd_item.gr, opc->osd_item.graphic_bg, &p, img);
    } else if(0==this->bTextOnly) {
        graphics_draw_circle(opc->osd_item.gr, osd_color, &p, this->d-this->width*2 );
    }
    graphics_get_text_bbox(opc->osd_item.gr, opc->osd_item.font, text, 0x10000, 0, bbox, 0);
    p.x=(opc->osd_item.w-bbox[2].x)/2;
    p.y=(opc->osd_item.h+bbox[2].y)/2-bbox[2].y;
    graphics_draw_text(opc->osd_item.gr, osd_color, NULL, opc->osd_item.font, text, &p, 0x10000, 0);
    graphics_draw_mode(opc->osd_item.gr, draw_mode_end);
}

static void osd_speed_warner_click(struct osd_priv_common *opc, struct navit *nav, int pressed, int button,
                                   struct point *p) {
    struct osd_speed_warner *this = (struct osd_speed_warner *)opc->data;

    struct point bp = opc->osd_item.p;
    osd_wrap_point(&bp, nav);
    if ((p->x < bp.x || p->y < bp.y || p->x > bp.x + opc->osd_item.w || p->y > bp.y + opc->osd_item.h
            || !opc->osd_item.configured ) && !opc->osd_item.pressed)
        return;
    if (button != 1)
        return;
    if (navit_ignore_button(nav))
        return;
    if (!!pressed == !!opc->osd_item.pressed)
        return;

    this->active = !this->active;
    osd_speed_warner_draw(opc, nav, NULL);
}


static void osd_speed_warner_init(struct osd_priv_common *opc, struct navit *nav) {
    struct osd_speed_warner *this = (struct osd_speed_warner *)opc->data;

    struct color red_color= {0xffff,0,0,0xffff};
    struct color green_color= {0,0xffff,0,0xffff};
    struct color grey_color= {0x8888,0x8888,0x8888,0x8888};
    struct color black_color= {0x1111,0x1111,0x1111,0x9999};

    osd_set_std_graphic(nav, &opc->osd_item, (struct osd_priv *)opc);
    navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_speed_warner_draw), attr_position_coord_geo, opc));
    navit_add_callback(nav, this->click_cb = callback_new_attr_1(callback_cast (osd_speed_warner_click), attr_button, opc));

    this->d=opc->osd_item.w;
    if (opc->osd_item.h < this->d)
        this->d=opc->osd_item.h;
    this->width=this->d/10;
    this->wait_before_warn = this->timeout;
    if(this->label_str && !strncmp("images:",this->label_str,7)) {
        char *tok1=NULL, *tok2=NULL, *tok3=NULL;
        strtok(this->label_str,":");
        tok1 = strtok(NULL,":");
        if(tok1) {
            tok2 = strtok(NULL,":");
        }
        if(tok1 && tok2) {
            tok3 = strtok(NULL,":");
        }
        if(tok1 && tok2 && tok3) {
            tok1 = graphics_icon_path(tok1);
            tok2 = graphics_icon_path(tok2);
            tok3 = graphics_icon_path(tok3);
            this->img_active  = graphics_image_new(opc->osd_item.gr, tok1);
            this->img_passive = graphics_image_new(opc->osd_item.gr, tok2);
            this->img_off     = graphics_image_new(opc->osd_item.gr, tok3);
            g_free(tok1);
            g_free(tok2);
            g_free(tok3);
        }
    }

    g_free(this->label_str);
    this->label_str = NULL;

    graphics_gc_set_linewidth(opc->osd_item.graphic_fg, this->d/2-2 /*-this->width*/ );

    this->red=graphics_gc_new(opc->osd_item.gr);
    graphics_gc_set_foreground(this->red, &red_color);
    graphics_gc_set_linewidth(this->red, this->width);

    this->green=graphics_gc_new(opc->osd_item.gr);
    graphics_gc_set_foreground(this->green, &green_color);
    graphics_gc_set_linewidth(this->green, this->width-2);

    this->grey=graphics_gc_new(opc->osd_item.gr);
    graphics_gc_set_foreground(this->grey, &grey_color);
    graphics_gc_set_linewidth(this->grey, this->width);

    this->black=graphics_gc_new(opc->osd_item.gr);
    graphics_gc_set_foreground(this->black, &black_color);
    graphics_gc_set_linewidth(this->black, this->width);

    osd_speed_warner_draw(opc, nav, NULL);
}

static struct osd_priv *osd_speed_warner_new(struct navit *nav, struct osd_methods *meth, struct attr **attrs) {
    struct osd_speed_warner *this=g_new0(struct osd_speed_warner, 1);
    struct osd_priv_common *opc = g_new0(struct osd_priv_common,1);
    struct attr *attr;

    opc->data = (void*)this;
    opc->osd_item.rel_x=-80;
    opc->osd_item.rel_y=20;
    opc->osd_item.rel_w=60;
    opc->osd_item.rel_h=60;
    opc->osd_item.navit = nav;
    this->active=-1;
    opc->osd_item.meth.draw = osd_draw_cast(osd_speed_warner_draw);
    meth->set_attr = set_std_osd_attr;

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
        this->label_str = g_strdup(attr->u.str);
        if (!strcmp("text_only",attr->u.str)) {
            this->bTextOnly = 1;
        }
    }
    attr = attr_search(attrs, NULL, attr_timeout);
    if (attr)
        this->timeout = attr->u.num;
    else
        this->timeout = 10;    // 10s timeout by default

    attr = attr_search(attrs, NULL, attr_announce_on);
    if (attr)
        this->announce_on = attr->u.num;
    else
        this->announce_on = 1;    //announce by default

    osd_set_std_attr(attrs, &opc->osd_item, ITEM_HAS_TEXT);
    navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_speed_warner_init), attr_graphics_ready, opc));
    return (struct osd_priv *) opc;
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
    int active;
    char *text;
    int align;
    char *last;
    struct osd_text_item *items;
};


/**
 * @brief Formats a text attribute
 *
 * Returns the formatted current value of an attribute as a string
 *
 * @param attr The attribute to be formatted
 * @param format A string specifying how to format the attribute. Allowed format strings depend on the attribute; this member can be NULL.
 * @param imperial True to convert values to imperial, false to return metric values
 * @returns The formatted value
 */
static char *osd_text_format_attr(struct attr *attr, char *format, int imperial) {
    struct tm tm, text_tm, text_tm0;
    time_t textt;
    int days=0;
    char buffer[1024];

    switch (attr->type) {
    case attr_position_speed:
        return format_speed(*attr->u.numd,"",format,imperial);
    case attr_position_height:
    case attr_position_direction:
        return format_float_0(*attr->u.numd);
    case attr_position_magnetic_direction:
        return g_strdup_printf("%ld",attr->u.num);
    case attr_position_coord_geo:
        if ((!format) || (!strcmp(format,"pos_degminsec"))) {
            coord_format(attr->u.coord_geo->lat,attr->u.coord_geo->lng,DEGREES_MINUTES_SECONDS,buffer,sizeof(buffer));
            return g_strdup(buffer);
        } else if (!strcmp(format,"pos_degmin")) {
            coord_format(attr->u.coord_geo->lat,attr->u.coord_geo->lng,DEGREES_MINUTES,buffer,sizeof(buffer));
            return g_strdup(buffer);
        } else if (!strcmp(format,"pos_deg")) {
            coord_format(attr->u.coord_geo->lat,attr->u.coord_geo->lng,DEGREES_DECIMAL,buffer,sizeof(buffer));
            return g_strdup(buffer);
        } else if (!strcmp(format,"lat_degminsec")) {
            coord_format(attr->u.coord_geo->lat,360,DEGREES_MINUTES_SECONDS,buffer,sizeof(buffer));
            return g_strdup(buffer);
        } else if (!strcmp(format,"lat_degmin")) {
            coord_format(attr->u.coord_geo->lat,360,DEGREES_MINUTES,buffer,sizeof(buffer));
            return g_strdup(buffer);
        } else if (!strcmp(format,"lat_deg")) {
            coord_format(attr->u.coord_geo->lat,360,DEGREES_DECIMAL,buffer,sizeof(buffer));
            return g_strdup(buffer);
        } else if (!strcmp(format,"lng_degminsec")) {
            coord_format(360,attr->u.coord_geo->lng,DEGREES_MINUTES_SECONDS,buffer,sizeof(buffer));
            return g_strdup(buffer);
        } else if (!strcmp(format,"lng_degmin")) {
            coord_format(360,attr->u.coord_geo->lng,DEGREES_MINUTES,buffer,sizeof(buffer));
            return g_strdup(buffer);
        } else if (!strcmp(format,"lng_deg")) {
            coord_format(360,attr->u.coord_geo->lng,DEGREES_DECIMAL,buffer,sizeof(buffer));
            return g_strdup(buffer);
        } else {
            // fall back to pos_degminsec
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
            return format_distance(attr->u.num,"",imperial);
        if (!strcmp(format,"value") || !strcmp(format,"unit")) {
            char *ret,*tmp=format_distance(attr->u.num," ",imperial);
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
        if ((!format) || (!strcmp(format,"iso8601"))) {
            break;
        } else {
            if (strstr(format, "local;") == format) {
                textt = iso8601_to_secs(attr->u.str);
                memcpy ((void *) &tm, (void *) localtime(&textt), sizeof(tm));
                strftime(buffer, sizeof(buffer), (char *)(format + 6), &tm);
            } else if ((sscanf(format, "%*c%2d:%2d;", &(text_tm.tm_hour), &(text_tm.tm_min)) == 2) && (strchr("+-", format[0]))) {
                if (strchr("-", format[0])) {
                    textt = iso8601_to_secs(attr->u.str) - text_tm.tm_hour * 3600 - text_tm.tm_min * 60;
                } else {
                    textt = iso8601_to_secs(attr->u.str) + text_tm.tm_hour * 3600 + text_tm.tm_min * 60;
                }
                memcpy ((void *) &tm, (void *) gmtime(&textt), sizeof(tm));
                strftime(buffer, sizeof(buffer), &format[strcspn(format, ";") + 1], &tm);
            } else {
                sscanf(attr->u.str, "%4d-%2d-%2dT%2d:%2d:%2d", &(tm.tm_year), &(tm.tm_mon), &(tm.tm_mday), &(tm.tm_hour), &(tm.tm_min),
                       &(tm.tm_sec));
                // the tm structure definition is kinda weird and needs some extra voodoo
                tm.tm_year-=1900;
                tm.tm_mon--;
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
 * @brief Parses a string of the form key.subkey or key[index].subkey into its components, where subkey
 * can itself have its own index and further subkeys
 *
 * @param in String to parse (the part before subkey will be modified by the function); upon returning
 * this pointer will point to a string containing key
 * @param index Pointer to an address that will receive a pointer to a string containing index or NULL
 * if key does not have an index
 * @returns If the function succeeds, a pointer to a string containing subkey, i.e. everything following
 * the first period, or a pointer to an empty string if there is nothing left to parse. If the function
 * fails (index with missing closed bracket or passing a null pointer as index argument when an index
 * was encountered), the return value is NULL
 */
static char *osd_text_split(char *in, char **index) {
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

static void osd_text_draw(struct osd_priv_common *opc, struct navit *navit, struct vehicle *v) {
    struct osd_text *this = (struct osd_text *)opc->data;
    struct point p, p2[4];
    char *str,*last,*next,*value,*absbegin;
    int do_draw = opc->osd_item.do_draw;
    struct attr attr, vehicle_attr, maxspeed_attr, imperial_attr;
    struct navigation *nav = NULL;
    struct tracking *tracking = NULL;
    struct map *nav_map = NULL;
    struct map_rect *nav_mr = NULL;
    struct item *item;
    struct osd_text_item *oti;
    int offset,lines;
    int height=opc->osd_item.font_size*13/256;
    int yspacing=height/2;
    int xspacing=height/4;
    int imperial=0;

    if (navit_get_attr(navit, attr_imperial, &imperial_attr, NULL))
        imperial=imperial_attr.u.num;

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
                dbg(lvl_debug,"name %s", item_to_name(item->type));
                dbg(lvl_debug,"type %s", attr_to_name(oti->attr_typ));
                if (item_attr_get(item, oti->attr_typ, &attr))
                    value=osd_text_format_attr(&attr, oti->format, imperial);
            }
            if (nav_mr)
                map_rect_destroy(nav_mr);
        } else if (oti->section == attr_vehicle) {
            if (navit && !vehicle_attr.u.vehicle) {
                navit_get_attr(navit, attr_vehicle, &vehicle_attr, NULL);
            }
            if (vehicle_attr.u.vehicle) {
                if (vehicle_get_attr(vehicle_attr.u.vehicle, oti->attr_typ, &attr, NULL)) {
                    value=osd_text_format_attr(&attr, oti->format, imperial);
                }
            }
        } else if (oti->section == attr_tracking) {
            if (navit) {
                tracking = navit_get_tracking(navit);
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

                    value = format_speed(routespeed,"", oti->format, imperial);
                } else if (item) {
                    if (tracking_get_attr(tracking, oti->attr_typ, &attr, NULL))
                        value=osd_text_format_attr(&attr, oti->format, imperial);
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

    if ( !this->last || !str || strcmp(this->last, str) ) {
        do_draw=1;
        if (this->last)
            g_free(this->last);
        this->last = g_strdup(str);
    }

    absbegin=str;

    if (do_draw) {
        osd_fill_with_bgcolor(&opc->osd_item);
        if (str) {
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

            dbg(lvl_debug,"this->align=%d", this->align);
            switch (this->align & 51) {
            case 1:
                p.y=0;
                break;
            case 2:
                p.y=(opc->osd_item.h-lines*(height+yspacing)-yspacing);
                break;
            case 16: // Grow from top to bottom
                p.y = 0;
                if (lines != 0) {
                    opc->osd_item.h = (lines-1) * (height+yspacing) + height;
                } else {
                    opc->osd_item.h = 0;
                }

                if (do_draw) {
                    osd_std_resize(&opc->osd_item);
                }
            default:
                p.y=(opc->osd_item.h-lines*(height+yspacing)-yspacing)/2;
            }

            while (str) {
                next=strstr(str, "\\n");
                if (next) {
                    *next='\0';
                    next+=2;
                }
                graphics_get_text_bbox(opc->osd_item.gr,
                                       opc->osd_item.font,
                                       str, 0x10000,
                                       0x0, p2, 0);
                switch (this->align & 12) {
                case 4:
                    p.x=xspacing;
                    break;
                case 8:
                    p.x=opc->osd_item.w-(p2[2].x-p2[0].x)-xspacing;
                    break;
                default:
                    p.x = ((p2[0].x - p2[2].x) / 2) + (opc->osd_item.w / 2);
                }
                p.y += height+yspacing;
                graphics_draw_text(opc->osd_item.gr,
                                   opc->osd_item.graphic_fg_text,
                                   NULL, opc->osd_item.font,
                                   str, &p, 0x10000,
                                   0);
                str=next;
            }
        }
        graphics_draw_mode(opc->osd_item.gr, draw_mode_end);
    }
    g_free(absbegin);

}

/**
 * @brief Creates a new osd_text_item and inserts it into a linked list
 *
 * @param parent The preceding {@code osd_text_item} in the list. If NULL, the new item becomes the root
 * element of a new list
 * @returns The new {@code osd_text_item}
 */
static struct osd_text_item *oti_new(struct osd_text_item * parent) {
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
 * @brief Prepares a text type OSD element
 *
 * This function parses the label string (as specified in the XML file) for a text type OSD element
 * into attributes and static text.
 *
 * @param opc The {@code struct osd_priv_common} for the OSD element. {@code opc->data->items} will
 * receive a pointer to a list of {@code osd_text_item} structures.
 * @param nav The navit structure
 */
static void osd_text_prepare(struct osd_priv_common *opc, struct navit *nav) {
    struct osd_text *this = (struct osd_text *)opc->data;

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
            navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_text_draw), attr_position_coord_geo, opc));
            break;
        }

        str=(end);
    }

    if(*str!='\0') {
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

static void osd_text_init(struct osd_priv_common *opc, struct navit *nav) {
    osd_set_std_graphic(nav, &opc->osd_item, (struct osd_priv *)opc);
    navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_std_click), attr_button, &opc->osd_item));
    osd_text_prepare(opc,nav);
    osd_text_draw(opc, nav, NULL);

}

static int osd_text_set_attr(struct osd_priv_common *opc, struct attr* attr) {
    struct osd_text *this_ = (struct osd_text *)opc->data;

    if(NULL==attr || NULL==this_) {
        return 0;
    }
    if(attr->type == attr_label) {
        struct navit *nav=opc->osd_item.navit;

        if(this_->text)
            g_free(this_->text);

        if(attr->u.str)
            this_->text = g_strdup(attr->u.str);
        else
            this_->text=g_strdup("");

        osd_text_prepare(opc,nav);

        if(navit_get_blocked(nav)&1)
            return 1;

        osd_text_draw(opc,nav,NULL);
        navit_draw(opc->osd_item.navit);
        return 1;
    }
    return 0;
}


static struct osd_priv *osd_text_new(struct navit *nav, struct osd_methods *meth,
                                     struct attr **attrs) {
    struct osd_text *this = g_new0(struct osd_text, 1);
    struct osd_priv_common *opc = g_new0(struct osd_priv_common,1);
    struct attr *attr;

    opc->data = (void*)this;
    opc->osd_item.rel_x = -80;
    opc->osd_item.rel_y = 20;
    opc->osd_item.rel_w = 60;
    opc->osd_item.rel_h = 20;
    opc->osd_item.navit = nav;
    opc->osd_item.font_size = 200;
    opc->osd_item.meth.draw = osd_draw_cast(osd_text_draw);
    meth->set_attr = set_std_osd_attr;
    opc->spec_set_attr_func = osd_text_set_attr;
    osd_set_std_attr(attrs, &opc->osd_item, ITEM_HAS_TEXT);

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

    navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_text_init), attr_graphics_ready, opc));
    return (struct osd_priv *) opc;
}

struct gps_status {
    char *icon_src;
    int icon_h, icon_w, active;
    int strength;
};

static void osd_gps_status_draw(struct osd_priv_common *opc, struct navit *navit,
                                struct vehicle *v) {
    struct gps_status *this = (struct gps_status *)opc->data;

    struct point p;
    int do_draw = opc->osd_item.do_draw;
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
                    dbg(lvl_debug,"num=%ld", attr.u.num);
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
        osd_fill_with_bgcolor(&opc->osd_item);
        if (this->active) {
            image = g_strdup_printf(this->icon_src, strength);
            gr_image = graphics_image_new_scaled(opc->osd_item.gr, image, this->icon_w, this->icon_h);
            if (gr_image) {
                p.x = (opc->osd_item.w - gr_image->width) / 2;
                p.y = (opc->osd_item.h - gr_image->height) / 2;
                graphics_draw_image(opc->osd_item.gr, opc->osd_item.  graphic_fg, &p, gr_image);
                graphics_image_free(opc->osd_item.gr, gr_image);
            }
            g_free(image);
        }
        graphics_draw_mode(opc->osd_item.gr, draw_mode_end);
    }
}

static void osd_gps_status_init(struct osd_priv_common *opc, struct navit *nav) {
    osd_set_std_graphic(nav, &opc->osd_item, (struct osd_priv *)opc);
    navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_gps_status_draw), attr_position_coord_geo, opc));
    navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_gps_status_draw), attr_position_fix_type, opc));
    navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_gps_status_draw), attr_position_sats_used, opc));
    navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_gps_status_draw), attr_position_hdop, opc));
    osd_gps_status_draw(opc, nav, NULL);
}

static struct osd_priv *osd_gps_status_new(struct navit *nav, struct osd_methods *meth,
        struct attr **attrs) {
    struct gps_status *this = g_new0(struct gps_status, 1);
    struct osd_priv_common *opc = g_new0(struct osd_priv_common,1);
    struct attr *attr;

    opc->data = (void*)this;
    opc->osd_item.rel_x = 20;
    opc->osd_item.rel_y = -80;
    opc->osd_item.rel_w = 60;
    opc->osd_item.navit = nav;
    opc->osd_item.rel_h = 40;
    opc->osd_item.font_size = 200;
    opc->osd_item.meth.draw = osd_draw_cast(osd_gps_status_draw);
    meth->set_attr = set_std_osd_attr;

    osd_set_std_attr(attrs, &opc->osd_item, 0);

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

    navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_gps_status_init), attr_graphics_ready, opc));
    return (struct osd_priv *) opc;
}


struct volume {
    char *icon_src;
    int icon_h, icon_w, active;
    int strength;
    struct callback *click_cb;
};

static void osd_volume_draw(struct osd_priv_common *opc, struct navit *navit) {
    struct volume *this = (struct volume *)opc->data;

    struct point p;
    struct graphics_image *gr_image;
    char *image;

    osd_fill_with_bgcolor(&opc->osd_item);
    if (this->active) {
        image = g_strdup_printf(this->icon_src, this->strength);
        gr_image = graphics_image_new_scaled(opc->osd_item.gr, image, this->icon_w, this->icon_h);
        if (gr_image) {
            p.x = (opc->osd_item.w - gr_image->width) / 2;
            p.y = (opc->osd_item.h - gr_image->height) / 2;
            graphics_draw_image(opc->osd_item.gr, opc->osd_item.  graphic_fg, &p, gr_image);
            graphics_image_free(opc->osd_item.gr, gr_image);
        }
        g_free(image);
    }
    graphics_draw_mode(opc->osd_item.gr, draw_mode_end);
}

static void osd_volume_click(struct osd_priv_common *opc, struct navit *nav, int pressed, int button, struct point *p) {
    struct volume *this = (struct volume *)opc->data;

    struct point bp = opc->osd_item.p;
    if ((p->x < bp.x || p->y < bp.y || p->x > bp.x + opc->osd_item.w || p->y > bp.y + opc->osd_item.h)
            && !opc->osd_item.pressed)
        return;
    navit_ignore_button(nav);
    if (pressed) {
        if (p->y - bp.y < opc->osd_item.h/2)
            this->strength++;
        else
            this->strength--;
        if (this->strength < 0)
            this->strength=0;
        if (this->strength > 5)
            this->strength=5;
        osd_volume_draw(opc, nav);
    }
}
static void osd_volume_init(struct osd_priv_common *opc, struct navit *nav) {
    struct volume *this = (struct volume *)opc->data;

    osd_set_std_graphic(nav, &opc->osd_item, (struct osd_priv *)opc);
    navit_add_callback(nav, this->click_cb = callback_new_attr_1(callback_cast (osd_volume_click), attr_button, opc));
    osd_volume_draw(opc, nav);
}

static struct osd_priv *osd_volume_new(struct navit *nav, struct osd_methods *meth,
                                       struct attr **attrs) {
    struct volume *this = g_new0(struct volume, 1);
    struct osd_priv_common *opc = g_new0(struct osd_priv_common,1);
    struct attr *attr;

    opc->data = (void*)this;
    opc->osd_item.rel_x = 20;
    opc->osd_item.rel_y = -80;
    opc->osd_item.rel_w = 60;
    opc->osd_item.rel_h = 40;
    opc->osd_item.navit = nav;
    opc->osd_item.font_size = 200;
    opc->osd_item.meth.draw = osd_draw_cast(osd_volume_draw);
    meth->set_attr = set_std_osd_attr;

    osd_set_std_attr(attrs, &opc->osd_item, 0);

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

    navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_volume_init), attr_graphics_ready, opc));
    return (struct osd_priv *) opc;
}

struct osd_scale {
    int use_overlay;
    struct callback *draw_cb,*navit_init_cb;
    struct graphics_gc *black;
};

static int round_to_nice_value(double value) {
    double nearest_power_of10,mantissa;
    nearest_power_of10=pow(10,floor(log10(value)));
    mantissa=value/nearest_power_of10;
    if (mantissa >= 5)
        mantissa=5;
    else if (mantissa >= 2)
        mantissa=2;
    else
        mantissa=1;
    return mantissa*nearest_power_of10;
}

static void osd_scale_draw(struct osd_priv_common *opc, struct navit *nav) {
    struct osd_scale *this = (struct osd_scale *)opc->data;

    struct point item_pos,scale_line_start,scale_line_end;
    struct point p[10],bbox[4];
    struct attr transformation,imperial_attr;
    int scale_x_offset,scale_length_on_map,scale_length_pixels;
    double distance_on_map;
    char *text;
    int width_reduced;
    int imperial=0;

    osd_std_calculate_sizes(&opc->osd_item, navit_get_width(nav), navit_get_height(nav));

    width_reduced=opc->osd_item.w*9/10;

    if (navit_get_attr(nav, attr_imperial, &imperial_attr, NULL))
        imperial=imperial_attr.u.num;

    if (!navit_get_attr(nav, attr_transformation, &transformation, NULL))
        return;

    graphics_draw_mode(opc->osd_item.gr, draw_mode_begin);
    item_pos.x=0;
    item_pos.y=0;
    graphics_draw_rectangle(opc->osd_item.gr, opc->osd_item.graphic_bg, &item_pos, opc->osd_item.w, opc->osd_item.h);

    scale_line_start=item_pos;
    scale_line_start.y+=opc->osd_item.h/2;
    scale_line_start.x+=(opc->osd_item.w-width_reduced)/2;
    scale_line_end=scale_line_start;
    scale_line_end.x+=width_reduced;

    distance_on_map=transform_pixels_to_map_distance(transformation.u.transformation, width_reduced);
    scale_length_on_map=round_to_nice_value(distance_on_map);
    scale_length_pixels=scale_length_on_map/distance_on_map*width_reduced;
    scale_x_offset=(opc->osd_item.w-scale_length_pixels) / 2;

    p[0]=scale_line_start;
    p[1]=scale_line_end;
    p[0].x+=scale_x_offset;
    p[1].x-=scale_x_offset;
    p[2]=p[0];
    p[3]=p[0];
    p[2].y-=opc->osd_item.h/10;
    p[3].y+=opc->osd_item.h/10;
    p[4]=p[1];
    p[5]=p[1];
    p[4].y-=opc->osd_item.h/10;
    p[5].y+=opc->osd_item.h/10;
    p[6]=p[2];
    p[6].x-=2;
    p[6].y-=2;
    p[7]=p[0];
    p[7].y-=2;
    p[8]=p[4];
    p[8].x-=2;
    p[8].y-=2;
    graphics_draw_rectangle(opc->osd_item.gr, opc->osd_item.graphic_fg, p+6, 4,opc->osd_item.h/5+4);
    graphics_draw_rectangle(opc->osd_item.gr, opc->osd_item.graphic_fg, p+7, p[1].x-p[0].x, 4);
    graphics_draw_rectangle(opc->osd_item.gr, opc->osd_item.graphic_fg, p+8, 4,opc->osd_item.h/5+4);
    graphics_draw_lines(opc->osd_item.gr, opc->osd_item.graphic_fg_text, p, 2);
    graphics_draw_lines(opc->osd_item.gr, opc->osd_item.graphic_fg_text, p+2, 2);
    graphics_draw_lines(opc->osd_item.gr, opc->osd_item.graphic_fg_text, p+4, 2);
    text=format_distance(scale_length_on_map, "", imperial);
    graphics_get_text_bbox(opc->osd_item.gr, opc->osd_item.font, text, 0x10000, 0, bbox, 0);
    p[0].x=(opc->osd_item.w-bbox[2].x)/2+item_pos.x;
    p[0].y=item_pos.y+opc->osd_item.h-opc->osd_item.h/10;
    graphics_draw_text(opc->osd_item.gr, opc->osd_item.graphic_fg_text, opc->osd_item.graphic_fg, opc->osd_item.font, text,
                       &p[0], 0x10000, 0);
    g_free(text);
    if (this->use_overlay)
        graphics_draw_mode(opc->osd_item.gr, draw_mode_end);
}

static void osd_scale_init(struct osd_priv_common *opc, struct navit *nav) {
    struct osd_scale *this = (struct osd_scale *)opc->data;

    struct graphics *gra = navit_get_graphics(nav);

    struct color transparent = {0,0,0,0};

    opc->osd_item.color_fg.r = 0xffff-opc->osd_item.text_color.r;
    opc->osd_item.color_fg.g = 0xffff-opc->osd_item.text_color.g;
    opc->osd_item.color_fg.b = 0xffff-opc->osd_item.text_color.b;
    opc->osd_item.color_fg.a = 0xffff-opc->osd_item.text_color.a;


    if(COLOR_IS_SAME(opc->osd_item.color_fg, transparent)) {
        opc->osd_item.color_fg.r = 0x1111;
        opc->osd_item.color_fg.g = 0x1111;
        opc->osd_item.color_fg.b = 0x1111;
        opc->osd_item.color_fg.a = 0x1111;
    }

    osd_set_std_graphic(nav, &opc->osd_item, (struct osd_priv *)opc);

    graphics_add_callback(gra, this->draw_cb=callback_new_attr_2(callback_cast(osd_scale_draw), attr_postdraw, opc, nav));
    if (navit_get_ready(nav) == 3)
        osd_scale_draw(opc, nav);
}

static struct osd_priv *osd_scale_new(struct navit *nav, struct osd_methods *meth,
                                      struct attr **attrs) {
    struct osd_scale *this = g_new0(struct osd_scale, 1);
    struct osd_priv_common *opc = g_new0(struct osd_priv_common,1);
    struct attr *attr;

    opc->data = (void*)this;
    opc->osd_item.font_size = 200;
    opc->osd_item.navit = nav;
    opc->osd_item.meth.draw = osd_draw_cast(osd_scale_draw);
    meth->set_attr = set_std_osd_attr;

    osd_set_std_attr(attrs, &opc->osd_item, TRANSPARENT_BG | ITEM_HAS_TEXT);

    navit_add_callback(nav, this->navit_init_cb = callback_new_attr_1(callback_cast (osd_scale_init), attr_graphics_ready,
                       opc));

    return (struct osd_priv *) opc;
}

struct auxmap {
    struct displaylist *displaylist;
    struct transformation *ntrans;
    struct transformation *trans;
    struct layout *layout;
    struct callback *postdraw_cb;
    struct graphics_gc *red;
    struct navit *nav;
};

static void osd_auxmap_draw(struct osd_priv_common *opc) {
    struct auxmap *this = (struct auxmap *)opc->data;

    int d=10;
    struct point p;
    struct attr mapset;

    if (!opc->osd_item.configured)
        return;
    if (!navit_get_attr(this->nav, attr_mapset, &mapset, NULL) || !mapset.u.mapset)
        return;
    p.x=opc->osd_item.w/2;
    p.y=opc->osd_item.h/2;

    if (opc->osd_item.rel_h || opc->osd_item.rel_w) {
        struct map_selection sel;
        memset(&sel, 0, sizeof(sel));
        sel.u.p_rect.rl.x=opc->osd_item.w;
        sel.u.p_rect.rl.y=opc->osd_item.h;
        dbg(lvl_debug,"osd_auxmap_draw: sel.u.p_rect.rl=(%d, %d)", opc->osd_item.w, opc->osd_item.h);
        transform_set_screen_selection(this->trans, &sel);
        graphics_set_rect(opc->osd_item.gr, &sel.u.p_rect);
    }

    transform_set_center(this->trans, transform_get_center(this->ntrans));
    transform_set_scale(this->trans, 64);
    transform_set_yaw(this->trans, transform_get_yaw(this->ntrans));
    transform_setup_source_rect(this->trans);
    transform_set_projection(this->trans, transform_get_projection(this->ntrans));
#if 0
    graphics_displaylist_draw(opc->osd_item.gr, this->displaylist, this->trans, this->layout, 4);
#endif
    graphics_draw(opc->osd_item.gr, this->displaylist, mapset.u.mapset, this->trans, this->layout, 0, NULL, 1);
    graphics_draw_circle(opc->osd_item.gr, this->red, &p, d);
    graphics_draw_mode(opc->osd_item.gr, draw_mode_end);

}

static void osd_auxmap_init(struct osd_priv_common *opc, struct navit *nav) {
    struct auxmap *this = (struct auxmap *)opc->data;

    struct graphics *gra;
    struct attr attr;
    struct map_selection sel;
    struct pcoord center= { projection_mg, 0, 0};
    struct color red= {0xffff,0x0,0x0,0xffff};

    this->nav=nav;
    if (! navit_get_attr(nav, attr_graphics, &attr, NULL))
        return;
    gra=attr.u.graphics;
    graphics_add_callback(gra, callback_new_attr_1(callback_cast(osd_auxmap_draw), attr_postdraw, opc));
    if (! navit_get_attr(nav, attr_transformation, &attr, NULL))
        return;
    this->ntrans=attr.u.transformation;
    if (! navit_get_attr(nav, attr_displaylist, &attr, NULL))
        return;
    this->displaylist=attr.u.displaylist;
    if (! navit_get_attr(nav, attr_layout, &attr, NULL))
        return;
    this->layout=attr.u.layout;
    osd_set_std_graphic(nav, &opc->osd_item, NULL);
    graphics_init(opc->osd_item.gr);
    this->red=graphics_gc_new(gra);
    graphics_gc_set_foreground(this->red,&red);
    graphics_gc_set_linewidth(this->red,3);
    memset(&sel, 0, sizeof(sel));
    sel.u.p_rect.rl.x=opc->osd_item.w;
    sel.u.p_rect.rl.y=opc->osd_item.h;
    this->trans=transform_new(&center, 16, 0);
    transform_set_screen_selection(this->trans, &sel);
    graphics_set_rect(opc->osd_item.gr, &sel.u.p_rect);
#if 0
    osd_auxmap_draw(opc, nav);
#endif
}

static struct osd_priv *osd_auxmap_new(struct navit *nav, struct osd_methods *meth, struct attr **attrs) {
    struct auxmap *this = g_new0(struct auxmap, 1);
    struct osd_priv_common *opc = g_new0(struct osd_priv_common,1);
    opc->data = (void*)this;

    opc->osd_item.rel_x = 20;
    opc->osd_item.rel_y = -80;
    opc->osd_item.rel_w = 60;
    opc->osd_item.rel_h = 40;
    opc->osd_item.font_size = 200;
    meth->set_attr = set_std_osd_attr;

    osd_set_std_attr(attrs, &opc->osd_item, 0);

    navit_add_callback(nav, callback_new_attr_1(callback_cast(osd_auxmap_init), attr_navit, opc));
    return (struct osd_priv *) opc;
}


void plugin_init(void) {
    plugin_register_category_osd("compass", osd_compass_new);
    plugin_register_category_osd("navigation_next_turn", osd_nav_next_turn_new);
    plugin_register_category_osd("button", osd_button_new);
    plugin_register_category_osd("toggle_announcer", osd_nav_toggle_announcer_new);
    plugin_register_category_osd("speed_warner", osd_speed_warner_new);
    plugin_register_category_osd("speed_cam", osd_speed_cam_new);
    plugin_register_category_osd("text", osd_text_new);
    plugin_register_category_osd("gps_status", osd_gps_status_new);
    plugin_register_category_osd("volume", osd_volume_new);
    plugin_register_category_osd("scale", osd_scale_new);
    plugin_register_category_osd("image", osd_image_new);
    plugin_register_category_osd("stopwatch", osd_stopwatch_new);
    plugin_register_category_osd("odometer", osd_odometer_new);
    plugin_register_category_osd("auxmap", osd_auxmap_new);
    plugin_register_category_osd("cmd_interface", osd_cmd_interface_new);
    plugin_register_category_osd("route_guard", osd_route_guard_new);
    plugin_register_category_osd("navigation_status", osd_navigation_status_new);
}
