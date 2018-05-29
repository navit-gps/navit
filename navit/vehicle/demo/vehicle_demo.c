/*
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

#include <glib.h>
#include <string.h>
#include <math.h>
#include "config.h"
#include "debug.h"
#include "coord.h"
#include "item.h"
#include "navit.h"
#include "map.h"
#include "route.h"
#include "callback.h"
#include "transform.h"
#include "plugin.h"
#include "vehicle.h"
#include "event.h"
#include "util.h"

/**
 * @defgroup vehicle-demo Vehicle Demo
 * @ingroup vehicle-plugins
 * @brief The Vehicle for a demo. It followes the route automatically
 *
 */

struct vehicle_priv {
    int interval;
    int position_set;
    struct callback_list *cbl;
    struct navit *navit;
    struct route *route;
    struct coord_geo geo;
    struct coord last;
    double config_speed;
    double speed;
    double direction;
    struct callback *timer_callback;
    struct event_timeout *timer;
    char *timep;
    char *nmea;
    enum attr_position_valid valid;  /**< Whether the vehicle has valid position data **/

};

static void vehicle_demo_destroy(struct vehicle_priv *priv) {
    if (priv->timer)
        event_remove_timeout(priv->timer);
    callback_destroy(priv->timer_callback);
    g_free(priv->timep);
    g_free(priv);
}

static void nmea_chksum(char *nmea) {
    int i;
    if (nmea && strlen(nmea) > 3) {
        unsigned char csum=0;
        for (i = 1 ; i < strlen(nmea)-4 ; i++)
            csum^=(unsigned char)(nmea[i]);
        sprintf(nmea+strlen(nmea)-3,"%02X\n",csum);
    }
}

static int vehicle_demo_position_attr_get(struct vehicle_priv *priv,
        enum attr_type type, struct attr *attr) {
    char ns='N',ew='E',*timep,*rmc,*gga;
    int hr,min,sec,year,mon,day;
    double lat,lng;
    switch (type) {
    case attr_position_speed:
        attr->u.numd = &priv->speed;
        break;
    case attr_position_direction:
        attr->u.numd = &priv->direction;
        break;
    case attr_position_coord_geo:
        attr->u.coord_geo = &priv->geo;
        break;
    case attr_position_time_iso8601:
        g_free(priv->timep);
        priv->timep=current_to_iso8601();
        attr->u.str=priv->timep;
        break;
    case attr_position_fix_type:
        attr->u.num = 2;
        break;
    case attr_position_sats_used:
        attr->u.num = 9;
        break;
    case attr_position_nmea:
        lat=priv->geo.lat;
        if (lat < 0) {
            lat=-lat;
            ns='S';
        }
        lng=priv->geo.lng;
        if (lng < 0) {
            lng=-lng;
            ew='W';
        }
        timep=current_to_iso8601();
        sscanf(timep,"%d-%d-%dT%d:%d:%d",&year,&mon,&day,&hr,&min,&sec);
        g_free(timep);
        gga=g_strdup_printf("$GPGGA,%02d%02d%02d,%02.0f%07.4f,%c,%03.0f%07.4f,%c,1,08,2.5,0,M,,,,0000*  \n",hr,min,sec,
                            floor(lat),(lat-floor(lat))*60.0,ns,floor(lng),(lng-floor(lng))*60,ew);
        nmea_chksum(gga);
        rmc=g_strdup_printf("$GPRMC,%02d%02d%02d,A,%02.0f%07.4f,%c,%03.0f%07.4f,%c,%3.1f,%3.1f,%02d%02d%02d,,*  \n",hr,min,sec,
                            floor(lat),(lat-floor(lat))*60.0,ns,floor(lng),(lng-floor(lng))*60,ew,priv->speed/1.852,(double)priv->direction,day,mon,
                            year%100);
        nmea_chksum(rmc);
        g_free(priv->nmea);
        priv->nmea=g_strdup_printf("%s%s",gga,rmc);
        g_free(gga);
        g_free(rmc);
        attr->u.str=priv->nmea;
        break;
    case attr_position_valid:
        attr->u.num=priv->valid;
        break;
    default:
        return 0;
    }
    attr->type = type;
    return 1;
}

static int vehicle_demo_set_attr_do(struct vehicle_priv *priv, struct attr *attr) {
    switch(attr->type) {
    case attr_navit:
        priv->navit = attr->u.navit;
        break;
    case attr_route:
        priv->route = attr->u.route;
        break;
    case attr_speed:
        priv->config_speed=attr->u.num;
        break;
    case attr_interval:
        priv->interval=attr->u.num;
        if (priv->timer)
            event_remove_timeout(priv->timer);
        priv->timer=event_add_timeout(priv->interval, 1, priv->timer_callback);
        break;
    case attr_position_coord_geo:
        priv->geo=*(attr->u.coord_geo);
        if (priv->valid != attr_position_valid_valid) {
            priv->valid = attr_position_valid_valid;
            callback_list_call_attr_0(priv->cbl, attr_position_valid);
        }
        priv->position_set=1;
        dbg(lvl_debug,"position_set %f %f", priv->geo.lat, priv->geo.lng);
        break;
    case attr_profilename:
    case attr_source:
    case attr_name:
    case attr_follow:
    case attr_active:
        // Ignore; used by Navit's infrastructure, but not relevant for this vehicle.
        break;
    default:
        dbg(lvl_error,"unsupported attribute %s",attr_to_name(attr->type));
        return 0;
    }
    return 1;
}

static int vehicle_demo_set_attr(struct vehicle_priv *priv, struct attr *attr) {
    return vehicle_demo_set_attr_do(priv, attr);
}

struct vehicle_methods vehicle_demo_methods = {
    vehicle_demo_destroy,
    vehicle_demo_position_attr_get,
    vehicle_demo_set_attr,
};

static void vehicle_demo_timer(struct vehicle_priv *priv) {
    struct coord c, c2, pos, ci;
    int slen, len, dx, dy;
    struct route *route=NULL;
    struct map *route_map=NULL;
    struct map_rect *mr=NULL;
    struct item *item=NULL;

    len = (priv->config_speed * priv->interval / 1000)/ 3.6;
    dbg(lvl_debug, "###### Entering simulation loop");
    if (!priv->config_speed)
        return;
    if (priv->route)
        route=priv->route;
    else if (priv->navit)
        route=navit_get_route(priv->navit);
    if (route)
        route_map=route_get_map(route);
    if (route_map)
        mr=map_rect_new(route_map, NULL);
    if (mr)
        item=map_rect_get_item(mr);
    if (item && item->type == type_route_start)
        item=map_rect_get_item(mr);
    while(item && item->type!=type_street_route)
        item=map_rect_get_item(mr);
    if (item && item_coord_get(item, &pos, 1)) {
        priv->position_set=0;
        dbg(lvl_debug, "current pos=0x%x,0x%x", pos.x, pos.y);
        dbg(lvl_debug, "last pos=0x%x,0x%x", priv->last.x, priv->last.y);
        if (priv->last.x == pos.x && priv->last.y == pos.y) {
            dbg(lvl_warning, "endless loop");
        }
        priv->last = pos;
        while (item && priv->config_speed) {
            if (!item_coord_get(item, &c, 1)) {
                item=map_rect_get_item(mr);
                continue;
            }
            dbg(lvl_debug, "next pos=0x%x,0x%x", c.x, c.y);
            slen = transform_distance(projection_mg, &pos, &c);
            dbg(lvl_debug, "len=%d slen=%d", len, slen);
            if (slen < len) {
                len -= slen;
                pos = c;
            } else {
                if (item_coord_get(item, &c2, 1) || map_rect_get_item(mr)) {
                    dx = c.x - pos.x;
                    dy = c.y - pos.y;
                    ci.x = pos.x + dx * len / slen;
                    ci.y = pos.y + dy * len / slen;
                    priv->direction =
                        transform_get_angle_delta(&pos, &c, 0);
                    priv->speed=priv->config_speed;
                } else {
                    ci.x = pos.x;
                    ci.y = pos.y;
                    priv->speed=0;
                    dbg(lvl_debug,"destination reached");
                }
                dbg(lvl_debug, "ci=0x%x,0x%x", ci.x, ci.y);
                transform_to_geo(projection_mg, &ci,
                                 &priv->geo);
                if (priv->valid != attr_position_valid_valid) {
                    priv->valid = attr_position_valid_valid;
                    callback_list_call_attr_0(priv->cbl, attr_position_valid);
                }
                callback_list_call_attr_0(priv->cbl, attr_position_coord_geo);
                break;
            }
        }
    } else {
        if (priv->position_set)
            callback_list_call_attr_0(priv->cbl, attr_position_coord_geo);
    }
    if (mr)
        map_rect_destroy(mr);
}



static struct vehicle_priv *vehicle_demo_new(struct vehicle_methods
        *meth, struct callback_list
        *cbl, struct attr **attrs) {
    struct vehicle_priv *ret;

    dbg(lvl_debug, "enter");
    ret = g_new0(struct vehicle_priv, 1);
    ret->cbl = cbl;
    ret->interval=1000;
    ret->config_speed=40;
    ret->timer_callback=callback_new_1(callback_cast(vehicle_demo_timer), ret);
    ret->valid = attr_position_valid_invalid;
    *meth = vehicle_demo_methods;
    while (attrs && *attrs)
        vehicle_demo_set_attr_do(ret, *attrs++);
    if (!ret->timer)
        ret->timer=event_add_timeout(ret->interval, 1, ret->timer_callback);
    return ret;
}

void plugin_init(void) {
    dbg(lvl_debug, "enter");
    plugin_register_category_vehicle("demo", vehicle_demo_new);
}


/** @} */
