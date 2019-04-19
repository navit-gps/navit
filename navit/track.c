/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2018 Navit Team
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

#include <glib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "item.h"
#include "attr.h"
#include "track.h"
#include "xmlconfig.h"
#include "debug.h"
#include "transform.h"
#include "coord.h"
#include "route.h"
#include "projection.h"
#include "map.h"
#include "mapset.h"
#include "plugin.h"
#include "vehicleprofile.h"
#include "vehicle.h"
#include "roadprofile.h"
#include "util.h"
#include "config.h"
#include "callback.h"

struct object_func tracking_func;

struct tracking_line {
    struct street_data *street;
    struct tracking_line *next;
    int angle[0];
};


/**
 * @brief Conatins a list of previous speeds
 *
 * This structure is used to hold a list of previously reported
 * speeds. This data is used by the CDF.
 */
struct cdf_speed {
    struct cdf_speed *next;
    int speed;
    time_t time;
};

/**
 * @brief Contains data for the CDF
 *
 * This structure holds all data needed by the
 * cumulative displacement filter.
 */
struct cdf_data {
    int extrapolating;
    int available;
    int first_pos;
    int poscount;
    int hist_size;
    struct cdf_speed *speed_hist;
    struct pcoord *pos_hist;
    int *dir_hist;
    double last_dist;
    struct pcoord last_out;
    int last_dir;
};

struct tracking {
    NAVIT_OBJECT
    struct callback_list *callback_list;
    struct mapset *ms;
    struct route *rt;
    struct map *map;
    struct vehicle *vehicle;
    struct vehicleprofile *vehicleprofile;
    struct coord last_updated;
    struct tracking_line *lines;
    struct tracking_line *curr_line;
    int pos;
    struct coord curr[2], curr_in, curr_out;
    int curr_angle;
    struct coord last[2], last_in, last_out;
    struct cdf_data cdf;
    struct attr *attr;
    int valid;
    int time;
    double direction, direction_matched;
    double speed;
    int coord_geo_valid;
    struct coord_geo coord_geo;
    enum projection pro;
    int street_direction;
    int no_gps;
    int tunnel;
    int angle_pref;
    int connected_pref;
    int nostop_pref;
    int offroad_limit_pref;
    int route_pref;
    int overspeed_pref;
    int overspeed_percent_pref;
    int tunnel_extrapolation;
};




static void tracking_init_cdf(struct cdf_data *cdf, int hist_size) {
    cdf->extrapolating = 0;
    cdf->available = 0;
    cdf->poscount = 0;
    cdf->last_dist = 0;
    cdf->hist_size = hist_size;

    cdf->pos_hist = g_new0(struct pcoord, hist_size);
    cdf->dir_hist = g_new0(int, hist_size);
}

// Variables for finetuning the CDF

// Minimum average speed
#define CDF_MINAVG 1.f
// Maximum average speed
#define CDF_MAXAVG 6.f // only ~ 20 km/h
// We need a low value here because otherwise we would extrapolate whenever we are not accelerating

// Mininum distance (square of it..), below which we ignore gps updates
#define CDF_MINDIST 49 // 7 meters, I guess this value has to be changed for pedestrians.

#if 0
static void tracking_process_cdf(struct cdf_data *cdf, struct pcoord *pin, struct pcoord *pout, int dirin, int *dirout,
                                 int cur_speed, time_t fixtime) {
    struct cdf_speed *speed,*sc,*sl;
    double speed_avg;
    int speed_num,i;

    if (cdf->hist_size == 0) {
        dbg(lvl_warning,"No CDF.");
        *pout = *pin;
        *dirout = dirin;
        return;
    }

    speed = g_new0(struct cdf_speed, 1);
    speed->speed = cur_speed;
    speed->time = fixtime;

    speed->next = cdf->speed_hist;
    cdf->speed_hist = speed;

    sc = speed;
    sl = NULL;
    speed_num = 0;
    speed_avg = 0;
    while (sc && ((fixtime - speed->time) < 4)) { // FIXME static maxtime
        speed_num++;
        speed_avg += sc->speed;
        sl = sc;
        sc = sc->next;
    }

    speed_avg /= (double)speed_num;

    if (sl) {
        sl->next = NULL;
    }

    while (sc) {
        sl = sc->next;
        g_free(sc);
        sc = sl;
    }

    if (speed_avg < CDF_MINAVG) {
        speed_avg = CDF_MINAVG;
    } else if (speed_avg > CDF_MAXAVG) {
        speed_avg = CDF_MAXAVG;
    }


    if (cur_speed >= speed_avg) {
        if (cdf->extrapolating) {
            cdf->poscount = 0;
            cdf->extrapolating = 0;
        }

        cdf->first_pos--;
        if (cdf->first_pos < 0) {
            cdf->first_pos = cdf->hist_size - 1;
        }

        if (cdf->poscount < cdf->hist_size) {
            cdf->poscount++;
        }

        cdf->pos_hist[cdf->first_pos] = *pin;
        cdf->dir_hist[cdf->first_pos] = dirin;

        *pout = *pin;
        *dirout = dirin;
    } else if (cdf->poscount > 0) {

        double mx,my; // Average position's x and y values
        double sx,sy; // Support vector
        double dx,dy; // Difference between average and current position
        double len;   // Length of support vector
        double dist;

        mx = my = 0;
        sx = sy = 0;

        for (i = 0; i < cdf->poscount; i++) {
            mx += (double)cdf->pos_hist[((cdf->first_pos + i) % cdf->hist_size)].x / cdf->poscount;
            my += (double)cdf->pos_hist[((cdf->first_pos + i) % cdf->hist_size)].y / cdf->poscount;


            if (i != 0) {
                sx += cdf->pos_hist[((cdf->first_pos + i) % cdf->hist_size)].x - cdf->pos_hist[((cdf->first_pos + i - 1) %
                        cdf->hist_size)].x;
                sy += cdf->pos_hist[((cdf->first_pos + i) % cdf->hist_size)].y - cdf->pos_hist[((cdf->first_pos + i - 1) %
                        cdf->hist_size)].y;
            }

        }

        if (cdf->poscount > 1) {
            // Normalize the support vector
            len = sqrt(sx * sx + sy * sy);
            sx /= len;
            sy /= len;

            // Calculate the new direction
            *dirout = (int)rint(atan(sx / sy) / M_PI * 180 + 180);
        } else {
            // If we only have one position, we can't use differences of positions, but we have to use the reported
            // direction of that position
            sx = sin((double)cdf->dir_hist[cdf->first_pos] / 180 * M_PI);
            sy = cos((double)cdf->dir_hist[cdf->first_pos] / 180 * M_PI);
            *dirout = cdf->dir_hist[cdf->first_pos];
        }


        dx = pin->x - mx;
        dy = pin->y - my;
        dist = dx * sx + dy * sy;

        if (cdf->extrapolating && (dist < cdf->last_dist)) {
            dist = cdf->last_dist;
        }

        cdf->last_dist = dist;
        cdf->extrapolating = 1;

        pout->x = (int)rint(mx + sx * dist);
        pout->y = (int)rint(my + sy * dist);
        pout->pro = pin->pro;

    } else {
        // We should extrapolate, but don't have an old position available
        *pout = *pin;
        *dirout = dirin;
    }

    if (cdf->available) {
        int dx,dy;

        dx = pout->x - cdf->last_out.x;
        dy = pout->y - cdf->last_out.y;

        if ((dx*dx + dy*dy) < CDF_MINDIST) {
            *pout = cdf->last_out;
            *dirout = cdf->last_dir;
        }
    }

    cdf->last_out = *pout;
    cdf->last_dir = *dirout;

    cdf->available = 1;
}
#endif

int tracking_get_angle(struct tracking *tr) {
    return tr->curr_angle;
}

struct coord *
tracking_get_pos(struct tracking *tr) {
    return &tr->curr_out;
}

int tracking_get_street_direction(struct tracking *tr) {
    return tr->street_direction;
}

int tracking_get_segment_pos(struct tracking *tr) {
    return tr->pos;
}

struct street_data *
tracking_get_street_data(struct tracking *tr) {
    if (tr->curr_line)
        return tr->curr_line->street;
    return NULL;
}

int tracking_get_attr(struct tracking *_this, enum attr_type type, struct attr *attr, struct attr_iter *attr_iter) {
    struct item *item;
    struct map_rect *mr;
    struct tracking_line *tl;

    int result=0;
    dbg(lvl_debug,"enter %s",attr_to_name(type));
    if (_this->attr) {
        attr_free(_this->attr);
        _this->attr=NULL;
    }
    attr->type=type;
    switch (type) {
    case attr_position_valid:
        attr->u.num=_this->valid;
        return 1;
    case attr_position_direction:
        attr->u.numd=&_this->direction;
        return 1;
    case attr_position_direction_matched:
        attr->u.numd=&_this->direction_matched;
        return 1;
    case attr_position_speed:
        attr->u.numd=&_this->speed;
        return 1;
    case attr_directed:
        attr->u.num=_this->street_direction;
        return 1;
    case attr_position_coord_geo:
        if (!_this->coord_geo_valid) {
            struct coord c;
            c.x=_this->curr_out.x;
            c.y=_this->curr_out.y;
            transform_to_geo(_this->pro, &c, &_this->coord_geo);
            _this->coord_geo_valid=1;
        }
        attr->u.coord_geo=&_this->coord_geo;
        return 1;
    case attr_current_item:
        if (! _this->curr_line || ! _this->curr_line->street)
            return 0;
        attr->u.item=&_this->curr_line->street->item;
        return 1;
    case attr_street_count:
        attr->u.num=0;
        tl=_this->lines;
        while (tl) {
            attr->u.num++;
            tl=tl->next;
        }
        return 1;
    default:
        if (! _this->curr_line || ! _this->curr_line->street)
            return 0;
        item=&_this->curr_line->street->item;
        mr=map_rect_new(item->map,NULL);
        item=map_rect_get_item_byid(mr, item->id_hi, item->id_lo);
        if (item_attr_get(item, type, attr)) {
            _this->attr=attr_dup(attr);
            *attr=*_this->attr;
            result=1;
        }
        map_rect_destroy(mr);
        return result;
    }
}

struct item *
tracking_get_current_item(struct tracking *_this) {
    if (! _this->curr_line || ! _this->curr_line->street)
        return NULL;
    return &_this->curr_line->street->item;
}

int *tracking_get_current_flags(struct tracking *_this) {
    if (! _this->curr_line || ! _this->curr_line->street)
        return NULL;
    return &_this->curr_line->street->flags;
}

static void tracking_get_angles(struct tracking_line *tl) {
    int i;
    struct street_data *sd=tl->street;
    for (i = 0 ; i < sd->count-1 ; i++)
        tl->angle[i]=transform_get_angle_delta(&sd->c[i], &sd->c[i+1], 0);
}

static int street_data_within_selection(struct street_data *sd, struct map_selection *sel) {
    struct coord_rect r;
    struct map_selection *curr;
    int i;

    if (!sel)
        return 1;
    r.lu=sd->c[0];
    r.rl=sd->c[0];
    for (i = 1 ; i < sd->count ; i++) {
        if (r.lu.x > sd->c[i].x)
            r.lu.x=sd->c[i].x;
        if (r.rl.x < sd->c[i].x)
            r.rl.x=sd->c[i].x;
        if (r.rl.y > sd->c[i].y)
            r.rl.y=sd->c[i].y;
        if (r.lu.y < sd->c[i].y)
            r.lu.y=sd->c[i].y;
    }
    curr=sel;
    while (curr) {
        struct coord_rect *sr=&curr->u.c_rect;
        if (r.lu.x <= sr->rl.x && r.rl.x >= sr->lu.x &&
                r.lu.y >= sr->rl.y && r.rl.y <= sr->lu.y)
            return 1;
        curr=curr->next;
    }
    return 0;
}


static void tracking_doupdate_lines(struct tracking *tr, struct coord *pc, enum projection pro) {
    int max_dist=1000;
    struct map_selection *sel;
    struct mapset_handle *h;
    struct map *m;
    struct map_rect *mr;
    struct item *item;
    struct street_data *street;
    struct tracking_line *tl;
    struct coord_geo g;
    struct coord cc;

    dbg(lvl_debug,"enter");
    h=mapset_open(tr->ms);
    while ((m=mapset_next(h,2))) {
        cc.x = pc->x;
        cc.y = pc->y;
        if (map_projection(m) != pro) {
            transform_to_geo(pro, &cc, &g);
            transform_from_geo(map_projection(m), &g, &cc);
        }
        sel = route_rect(18, &cc, &cc, 0, max_dist);
        mr=map_rect_new(m, sel);
        if (!mr)
            continue;
        while ((item=map_rect_get_item(mr))) {
            if (item_get_default_flags(item->type)) {
                street=street_get_data(item);
                if (street_data_within_selection(street, sel)) {
                    tl=g_malloc(sizeof(struct tracking_line)+(street->count-1)*sizeof(int));
                    tl->street=street;
                    tracking_get_angles(tl);
                    tl->next=tr->lines;
                    tr->lines=tl;
                } else
                    street_data_free(street);
            }
        }
        map_selection_destroy(sel);
        map_rect_destroy(mr);
    }
    mapset_close(h);
    dbg(lvl_debug, "exit");
}


void tracking_flush(struct tracking *tr) {
    struct tracking_line *tl=tr->lines,*next;
    dbg(lvl_debug,"enter(tr=%p)", tr);

    while (tl) {
        next=tl->next;
        street_data_free(tl->street);
        g_free(tl);
        tl=next;
    }
    tr->lines=NULL;
    tr->curr_line = NULL;
}

static int tracking_angle_diff(int a1, int a2, int full) {
    int ret=(a1-a2)%full;
    if (ret > full/2)
        ret-=full;
    if (ret < -full/2)
        ret+=full;
    return ret;
}

static int tracking_angle_abs_diff(int a1, int a2, int full) {
    int ret=tracking_angle_diff(a1, a2, full);
    if (ret < 0)
        ret=-ret;
    return ret;
}

static int tracking_angle_delta(struct tracking *tr, int vehicle_angle, int street_angle, int flags) {
    int full=180,ret=360,fwd=0,rev=0;
    struct vehicleprofile *profile=tr->vehicleprofile;

    if (profile) {
        fwd=((flags & profile->flags_forward_mask) == profile->flags);
        rev=((flags & profile->flags_reverse_mask) == profile->flags);
    }
    if (fwd || rev) {
        if (!fwd || !rev) {
            full=360;
            if (rev)
                street_angle=(street_angle+180)%360;
        }
        ret=tracking_angle_abs_diff(vehicle_angle, street_angle, full);
    }
    return ret*ret;
}

static int tracking_is_connected(struct tracking *tr, struct coord *c1, struct coord *c2) {
    if (c1[0].x == c2[0].x && c1[0].y == c2[0].y)
        return 0;
    if (c1[0].x == c2[1].x && c1[0].y == c2[1].y)
        return 0;
    if (c1[1].x == c2[0].x && c1[1].y == c2[0].y)
        return 0;
    if (c1[1].x == c2[1].x && c1[1].y == c2[1].y)
        return 0;
    return tr->connected_pref;
}

static int tracking_is_no_stop(struct tracking *tr, struct coord *c1, struct coord *c2) {
    if (c1->x == c2->x && c1->y == c2->y)
        return tr->nostop_pref;
    return 0;
}

static int tracking_is_on_route(struct tracking *tr, struct route *rt, struct item *item) {
#ifdef USE_ROUTING
    if (! rt)
        return 0;
    if (route_contains(rt, item))
        return 0;
    return tr->route_pref;
#else
    return 0;
#endif
}

static int tracking_value(struct tracking *tr, struct tracking_line *t, int offset, struct coord *lpnt, int min,
                          int flags) {
    int value=0;
    struct street_data *sd=t->street;
    dbg(lvl_info, "%d: (0x%x,0x%x)-(0x%x,0x%x)", offset, sd->c[offset].x, sd->c[offset].y, sd->c[offset+1].x,
        sd->c[offset+1].y);
    if (flags & 1) {
        struct coord c1, c2, cp;
        c1.x = sd->c[offset].x;
        c1.y = sd->c[offset].y;
        c2.x = sd->c[offset+1].x;
        c2.y = sd->c[offset+1].y;
        cp.x = tr->curr_in.x;
        cp.y = tr->curr_in.y;
        value+=transform_distance_line_sq(&c1, &c2, &cp, lpnt);
    }
    if (value >= min)
        return value;
    if (flags & 2)
        value += tracking_angle_delta(tr, tr->curr_angle, t->angle[offset], sd->flags)*tr->angle_pref>>4;
    if (value >= min)
        return value;
    if ((flags & 4)  && tr->connected_pref)
        value += tracking_is_connected(tr, tr->last, &sd->c[offset]);
    if ((flags & 8)  && tr->nostop_pref)
        value += tracking_is_no_stop(tr, lpnt, &tr->last_out);
    if (value >= min)
        return value;
    if ((flags & 16) && tr->route_pref)
        value += tracking_is_on_route(tr, tr->rt, &sd->item);
    if ((flags & 32) && tr->overspeed_percent_pref && tr->overspeed_pref ) {
        struct roadprofile *roadprofile=g_hash_table_lookup(tr->vehicleprofile->roadprofile_hash, (void *)t->street->item.type);
        if (roadprofile && tr->speed > roadprofile->speed * tr->overspeed_percent_pref/ 100)
            value += tr->overspeed_pref;
    }
    if ((flags & 64) && !!(sd->flags & AF_UNDERGROUND) != tr->no_gps)
        value+=200;
    return value;
}


/**
 * @brief Processes a position update.
 *
 * @param tr The {@code struct tracking} which will receive the position update
 * @param v The vehicle whose position has changed
 * @param vehicleprofile The vehicle profile to use
 * @param pro The projection to use for transformations
 */
void tracking_update(struct tracking *tr, struct vehicle *v, struct vehicleprofile *vehicleprofile,
                     enum projection pro) {
    struct tracking_line *t;
    int i,value,min,time;
    struct coord lpnt;
    struct coord cin;
    struct attr valid,speed_attr,direction_attr,coord_geo,lag,time_attr,static_speed,static_distance;
    double speed, direction;
    if (v)
        tr->vehicle=v;
    if (vehicleprofile)
        tr->vehicleprofile=vehicleprofile;

    if (! tr->vehicle)
        return;
    if (!vehicle_get_attr(tr->vehicle, attr_position_valid, &valid, NULL))
        valid.u.num=attr_position_valid_valid;
    if (valid.u.num == attr_position_valid_invalid) {
        tr->valid=valid.u.num;
        return;
    }
    if (!vehicle_get_attr(tr->vehicle, attr_position_speed, &speed_attr, NULL) ||
            !vehicle_get_attr(tr->vehicle, attr_position_direction, &direction_attr, NULL) ||
            !vehicle_get_attr(tr->vehicle, attr_position_coord_geo, &coord_geo, NULL)) {
        dbg(lvl_error,"failed to get position data %d %d %d",
            vehicle_get_attr(tr->vehicle, attr_position_speed, &speed_attr, NULL),
            vehicle_get_attr(tr->vehicle, attr_position_direction, &direction_attr, NULL),
            vehicle_get_attr(tr->vehicle, attr_position_coord_geo, &coord_geo, NULL));
        return;
    }
    if (tr->tunnel_extrapolation) {
        struct attr fix_type;
        if (!vehicle_get_attr(tr->vehicle, attr_position_fix_type, &fix_type, NULL))
            fix_type.u.num=2;
        if (fix_type.u.num) {
            tr->no_gps=0;
            tr->tunnel=0;
        } else
            tr->no_gps=1;
    }
    if (!vehicleprofile_get_attr(vehicleprofile,attr_static_speed,&static_speed,NULL)
            || !vehicleprofile_get_attr(vehicleprofile,attr_static_distance,&static_distance,NULL)) {
        static_speed.u.num=3;
        static_distance.u.num=10;
        dbg(lvl_debug,"Using defaults for static position detection");
    }
    dbg(lvl_info,"Static speed: %ld, static distance: %ld",static_speed.u.num, static_distance.u.num);
    speed=*speed_attr.u.numd;
    direction=*direction_attr.u.numd;
    tr->valid=attr_position_valid_valid;
    transform_from_geo(pro, coord_geo.u.coord_geo, &tr->curr_in);
    if ((speed < static_speed.u.num && transform_distance(pro, &tr->last_in, &tr->curr_in) < static_distance.u.num )) {
        dbg(lvl_debug,"static speed %f coord 0x%x,0x%x vs 0x%x,0x%x",speed,tr->last_in.x,tr->last_in.y, tr->curr_in.x,
            tr->curr_in.y);
        tr->valid=attr_position_valid_static;
        tr->speed=0;
        return;
    }
    if (tr->tunnel) {
        tr->curr_in=tr->curr_out;
        dbg(lvl_debug,"tunnel extrapolation speed %f dir %f",tr->speed,tr->direction);
        dbg(lvl_debug,"old 0x%x,0x%x",tr->curr_in.x, tr->curr_in.y);
        speed=tr->speed;
        direction=tr->curr_line->angle[tr->pos];
        transform_project(pro, &tr->curr_in, tr->speed*tr->tunnel_extrapolation/36, tr->direction, &tr->curr_in);
        dbg(lvl_debug,"new 0x%x,0x%x",tr->curr_in.x, tr->curr_in.y);
    } else if (vehicle_get_attr(tr->vehicle, attr_lag, &lag, NULL) && lag.u.num > 0) {
        double espeed;
        time=iso8601_to_secs(time_attr.u.str);
        int edirection;
        if (time-tr->time == 1) {
            dbg(lvl_debug,"extrapolating speed from %f and %f (%f)",tr->speed, speed, speed-tr->speed);
            espeed=speed+(speed-tr->speed)*lag.u.num/10;
            dbg(lvl_debug,"extrapolating angle from %f and %f (%d)",tr->direction, direction, tracking_angle_diff(direction,
                    tr->direction,360));
            edirection=direction+tracking_angle_diff(direction,tr->direction,360)*lag.u.num/10;
        } else {
            dbg(lvl_debug,"no speed and direction extrapolation");
            espeed=speed;
            edirection=direction;
        }
        dbg(lvl_debug,"lag %ld speed %f direction %d",lag.u.num,espeed,edirection);
        dbg(lvl_debug,"old 0x%x,0x%x",tr->curr_in.x, tr->curr_in.y);
        transform_project(pro, &tr->curr_in, espeed*lag.u.num/36, edirection, &tr->curr_in);
        tr->time=time;
        dbg(lvl_debug,"new 0x%x,0x%x",tr->curr_in.x, tr->curr_in.y);
    }
    tr->pro=pro;
    tr->curr_angle=tr->direction=direction;
    tr->speed=speed;
    tr->last_in=tr->curr_in;
    tr->last_out=tr->curr_out;
    tr->last[0]=tr->curr[0];
    tr->last[1]=tr->curr[1];
    if (!tr->lines || transform_distance(pro, &tr->last_updated, &tr->curr_in) > 500) {
        dbg(lvl_debug, "update");
        tracking_flush(tr);
        tracking_doupdate_lines(tr, &tr->curr_in, pro);
        tr->last_updated=tr->curr_in;
        dbg(lvl_debug,"update end");
    }

    tr->street_direction=0;
    t=tr->lines;
    tr->curr_line=NULL;
    min=INT_MAX/2;
    while (t) {
        struct street_data *sd=t->street;
        for (i = 0; i < sd->count-1 ; i++) {
            value=tracking_value(tr,t,i,&lpnt,min,-1);
            if (value < min) {
                struct coord lpnt_tmp;
                int angle_delta=tracking_angle_abs_diff(tr->curr_angle, t->angle[i], 360);
                tr->curr_line=t;
                tr->pos=i;
                tr->curr[0]=sd->c[i];
                tr->curr[1]=sd->c[i+1];
                tr->direction_matched=t->angle[i];
                dbg(lvl_debug,"lpnt.x=0x%x,lpnt.y=0x%x pos=%d %d+%d+%d+%d=%d", lpnt.x, lpnt.y, i,
                    transform_distance_line_sq(&sd->c[i], &sd->c[i+1], &cin, &lpnt_tmp),
                    tracking_angle_delta(tr, tr->curr_angle, t->angle[i], 0)*tr->angle_pref,
                    tracking_is_connected(tr, tr->last, &sd->c[i]) ? tr->connected_pref : 0,
                    lpnt.x == tr->last_out.x && lpnt.y == tr->last_out.y ? tr->nostop_pref : 0,
                    value
                   );
                tr->curr_out.x=lpnt.x;
                tr->curr_out.y=lpnt.y;
                tr->coord_geo_valid=0;
                if (angle_delta < 70)
                    tr->street_direction=1;
                else if (angle_delta > 110)
                    tr->street_direction=-1;
                else
                    tr->street_direction=0;
                min=value;
            }
        }
        t=t->next;
    }
    dbg(lvl_debug,"tr->curr_line=%p min=%d", tr->curr_line, min);
    if (!tr->curr_line || min > tr->offroad_limit_pref) {
        tr->curr_out=tr->curr_in;
        tr->coord_geo_valid=0;
        tr->street_direction=0;
    }
    if (tr->curr_line && (tr->curr_line->street->flags & AF_UNDERGROUND)) {
        if (tr->no_gps)
            tr->tunnel=1;
    } else if (tr->tunnel) {
        tr->speed=0;
    }
    dbg(lvl_debug,"found 0x%x,0x%x", tr->curr_out.x, tr->curr_out.y);
    callback_list_call_attr_0(tr->callback_list, attr_position_coord_geo);
}

static int tracking_set_attr_do(struct tracking *tr, struct attr *attr, int initial) {
    switch (attr->type) {
    case attr_angle_pref:
        tr->angle_pref=attr->u.num;
        return 1;
    case attr_connected_pref:
        tr->connected_pref=attr->u.num;
        return 1;
    case attr_nostop_pref:
        tr->nostop_pref=attr->u.num;
        return 1;
    case attr_offroad_limit_pref:
        tr->offroad_limit_pref=attr->u.num;
        return 1;
    case attr_route_pref:
        tr->route_pref=attr->u.num;
        return 1;
    case attr_overspeed_pref:
        tr->overspeed_pref=attr->u.num;
        return 1;
    case attr_overspeed_percent_pref:
        tr->overspeed_percent_pref=attr->u.num;
        return 1;
    case attr_tunnel_extrapolation:
        tr->tunnel_extrapolation=attr->u.num;
        return 1;
    default:
        return 0;
    }
}

int tracking_set_attr(struct tracking *tr, struct attr *attr) {
    return tracking_set_attr_do(tr, attr, 0);
}

int tracking_add_attr(struct tracking *this_, struct attr *attr) {
    switch (attr->type) {
    case attr_callback:
        callback_list_add(this_->callback_list, attr->u.callback);
        return 1;
    default:
        return 0;
    }
}

int tracking_remove_attr(struct tracking *this_, struct attr *attr) {
    switch (attr->type) {
    case attr_callback:
        callback_list_remove(this_->callback_list, attr->u.callback);
        return 1;
    default:
        return 0;
    }
}

struct object_func tracking_func = {
    attr_trackingo,
    (object_func_new)tracking_new,
    (object_func_get_attr)tracking_get_attr,
    (object_func_iter_new)NULL,
    (object_func_iter_destroy)NULL,
    (object_func_set_attr)tracking_set_attr,
    (object_func_add_attr)tracking_add_attr,
    (object_func_remove_attr)tracking_remove_attr,
    (object_func_init)tracking_init,
    (object_func_destroy)tracking_destroy,
    (object_func_dup)NULL,
    (object_func_ref)navit_object_ref,
    (object_func_unref)navit_object_unref,
};


struct tracking *
tracking_new(struct attr *parent, struct attr **attrs) {
    struct tracking *this=g_new0(struct tracking, 1);
    struct attr hist_size;
    this->func=&tracking_func;
    navit_object_ref((struct navit_object *)this);
    this->angle_pref=10;
    this->connected_pref=10;
    this->nostop_pref=10;
    this->offroad_limit_pref=5000;
    this->route_pref=300;
    this->callback_list=callback_list_new();


    if (! attr_generic_get_attr(attrs, NULL, attr_cdf_histsize, &hist_size, NULL)) {
        hist_size.u.num = 0;
    }
    if (attrs) {
        for (; *attrs; attrs++)
            tracking_set_attr_do(this, *attrs, 1);
    }

    tracking_init_cdf(&this->cdf, hist_size.u.num);

    return this;
}

void tracking_set_mapset(struct tracking *this, struct mapset *ms) {
    this->ms=ms;
}

void tracking_set_route(struct tracking *this, struct route *rt) {
    this->rt=rt;
}

void tracking_destroy(struct tracking *tr) {
    if (tr->attr)
        attr_free(tr->attr);
    tracking_flush(tr);
    callback_list_destroy(tr->callback_list);
    g_free(tr);
}

struct map *
tracking_get_map(struct tracking *this_) {
    struct attr *attrs[5];
    struct attr type,navigation,data,description;
    type.type=attr_type;
    type.u.str="tracking";
    navigation.type=attr_trackingo;
    navigation.u.tracking=this_;
    data.type=attr_data;
    data.u.str="";
    description.type=attr_description;
    description.u.str="Tracking";

    attrs[0]=&type;
    attrs[1]=&navigation;
    attrs[2]=&data;
    attrs[3]=&description;
    attrs[4]=NULL;
    if (! this_->map)
        this_->map=map_new(NULL, attrs);
    return this_->map;
}


struct map_priv {
    struct tracking *tracking;
};

struct map_rect_priv {
    struct tracking *tracking;
    struct item item;
    struct tracking_line *curr,*next;
    int coord;
    enum attr_type attr_next;
    int ccount;
    int debug_idx;
    char *str;
};

static void tracking_map_item_coord_rewind(void *priv_data) {
    struct map_rect_priv *this=priv_data;
    this->ccount=0;
}

static int tracking_map_item_coord_get(void *priv_data, struct coord *c, int count) {
    struct map_rect_priv *this=priv_data;
    enum projection pro;
    int ret=0;
    dbg(lvl_debug,"enter");
    while (this->ccount < 2 && count > 0) {
        pro = map_projection(this->curr->street->item.map);
        if (projection_mg != pro) {
            transform_from_to(&this->curr->street->c[this->ccount+this->coord],
                              pro,
                              c,projection_mg);
        } else
            *c=this->curr->street->c[this->ccount+this->coord];
        dbg(lvl_debug,"coord %d 0x%x,0x%x",this->ccount,c->x,c->y);
        this->ccount++;
        ret++;
        c++;
        count--;
    }
    return ret;
}

static void tracking_map_item_attr_rewind(void *priv_data) {
    struct map_rect_priv *this_=priv_data;
    this_->debug_idx=0;
    this_->attr_next=attr_debug;
}

static int tracking_map_item_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr) {
    struct map_rect_priv *this_=priv_data;
    struct coord lpnt,*c;
    struct tracking *tr=this_->tracking;
    int value;
    attr->type=attr_type;

    if (this_->str) {
        g_free(this_->str);
        this_->str=NULL;
    }

    switch(attr_type) {
    case attr_debug:
        switch(this_->debug_idx) {
        case 0:
            this_->debug_idx++;
            this_->str=attr->u.str=g_strdup_printf("overall: %d (limit %d)",tracking_value(tr, this_->curr, this_->coord, &lpnt,
                                                   INT_MAX/2, -1), tr->offroad_limit_pref);
            return 1;
        case 1:
            this_->debug_idx++;
            c=&this_->curr->street->c[this_->coord];
            value=tracking_value(tr, this_->curr, this_->coord, &lpnt, INT_MAX/2, 1);
            this_->str=attr->u.str=g_strdup_printf("distance: (0x%x,0x%x) from (0x%x,0x%x)-(0x%x,0x%x) at (0x%x,0x%x) %d",
                                                   tr->curr_in.x, tr->curr_in.y,
                                                   c[0].x, c[0].y, c[1].x, c[1].y,
                                                   lpnt.x, lpnt.y, value);
            return 1;
        case 2:
            this_->debug_idx++;
            this_->str=attr->u.str=g_strdup_printf("angle: %d to %d (flags %d) %d",
                                                   tr->curr_angle, this_->curr->angle[this_->coord], this_->curr->street->flags & 3,
                                                   tracking_value(tr, this_->curr, this_->coord, &lpnt, INT_MAX/2, 2));
            return 1;
        case 3:
            this_->debug_idx++;
            this_->str=attr->u.str=g_strdup_printf("connected: %d", tracking_value(tr, this_->curr, this_->coord, &lpnt, INT_MAX/2,
                                                   4));
            return 1;
        case 4:
            this_->debug_idx++;
            this_->str=attr->u.str=g_strdup_printf("no_stop: %d", tracking_value(tr, this_->curr, this_->coord, &lpnt, INT_MAX/2,
                                                   8));
            return 1;
        case 5:
            this_->debug_idx++;
            this_->str=attr->u.str=g_strdup_printf("route: %d", tracking_value(tr, this_->curr, this_->coord, &lpnt, INT_MAX/2,
                                                   16));
            return 1;
        case 6:
            this_->debug_idx++;
            this_->str=attr->u.str=g_strdup_printf("overspeed: %d", tracking_value(tr, this_->curr, this_->coord, &lpnt, INT_MAX/2,
                                                   32));
            return 1;
        case 7:
            this_->debug_idx++;
            this_->str=attr->u.str=g_strdup_printf("tunnel: %d", tracking_value(tr, this_->curr, this_->coord, &lpnt, INT_MAX/2,
                                                   64));
            return 1;
        case 8:
            this_->debug_idx++;
            this_->str=attr->u.str=g_strdup_printf("line %p", this_->curr);
            return 1;
        default:
            this_->attr_next=attr_none;
            return 0;
        }
    case attr_any:
        while (this_->attr_next != attr_none) {
            if (tracking_map_item_attr_get(priv_data, this_->attr_next, attr))
                return 1;
        }
        return 0;
    default:
        attr->type=attr_none;
        return 0;
    }
}

static struct item_methods tracking_map_item_methods = {
    tracking_map_item_coord_rewind,
    tracking_map_item_coord_get,
    tracking_map_item_attr_rewind,
    tracking_map_item_attr_get,
};


static void tracking_map_destroy(struct map_priv *priv) {
    g_free(priv);
}

static void tracking_map_rect_init(struct map_rect_priv *priv) {
    priv->next=priv->tracking->lines;
    priv->curr=NULL;
    priv->coord=0;
    priv->item.id_lo=0;
    priv->item.id_hi=0;
}

static struct map_rect_priv *tracking_map_rect_new(struct map_priv *priv, struct map_selection *sel) {
    struct tracking *tracking=priv->tracking;
    struct map_rect_priv *ret=g_new0(struct map_rect_priv, 1);
    ret->tracking=tracking;
    tracking_map_rect_init(ret);
    ret->item.meth=&tracking_map_item_methods;
    ret->item.priv_data=ret;
    ret->item.type=type_tracking_100;
    return ret;
}

static void tracking_map_rect_destroy(struct map_rect_priv *priv) {
    g_free(priv);
}

static struct item *tracking_map_get_item(struct map_rect_priv *priv) {
    struct item *ret=&priv->item;
    int value;
    struct coord lpnt;

    if (!priv->next)
        return NULL;
    if (! priv->curr || priv->coord + 2 >= priv->curr->street->count) {
        priv->curr=priv->next;
        priv->next=priv->curr->next;
        priv->coord=0;
        priv->item.id_lo=0;
        priv->item.id_hi++;
    } else {
        priv->coord++;
        priv->item.id_lo++;
    }
    value=tracking_value(priv->tracking, priv->curr, priv->coord, &lpnt, INT_MAX/2, -1);
    if (value < 64)
        priv->item.type=type_tracking_100;
    else if (value < 128)
        priv->item.type=type_tracking_90;
    else if (value < 256)
        priv->item.type=type_tracking_80;
    else if (value < 512)
        priv->item.type=type_tracking_70;
    else if (value < 1024)
        priv->item.type=type_tracking_60;
    else if (value < 2048)
        priv->item.type=type_tracking_50;
    else if (value < 4096)
        priv->item.type=type_tracking_40;
    else if (value < 8192)
        priv->item.type=type_tracking_30;
    else if (value < 16384)
        priv->item.type=type_tracking_20;
    else if (value < 32768)
        priv->item.type=type_tracking_10;
    else
        priv->item.type=type_tracking_0;
    dbg(lvl_debug,"item %d %d points", priv->coord, priv->curr->street->count);
    tracking_map_item_coord_rewind(priv);
    tracking_map_item_attr_rewind(priv);
    return ret;
}

static struct item *tracking_map_get_item_byid(struct map_rect_priv *priv, int id_hi, int id_lo) {
    struct item *ret;
    tracking_map_rect_init(priv);
    while ((ret=tracking_map_get_item(priv))) {
        if (ret->id_hi == id_hi && ret->id_lo == id_lo)
            return ret;
    }
    return NULL;
}

static struct map_methods tracking_map_meth = {
    projection_mg,
    "utf-8",
    tracking_map_destroy,
    tracking_map_rect_new,
    tracking_map_rect_destroy,
    tracking_map_get_item,
    tracking_map_get_item_byid,
    NULL,
    NULL,
    NULL,
};

static struct map_priv *tracking_map_new(struct map_methods *meth, struct attr **attrs, struct callback_list *cbl) {
    struct map_priv *ret;
    struct attr *tracking_attr;

    tracking_attr=attr_search(attrs, NULL, attr_trackingo);
    if (! tracking_attr)
        return NULL;
    ret=g_new0(struct map_priv, 1);
    *meth=tracking_map_meth;
    ret->tracking=tracking_attr->u.tracking;

    return ret;
}


void tracking_init(void) {
    plugin_register_category_map("tracking", tracking_map_new);
}
