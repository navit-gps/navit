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

#include <stdlib.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "debug.h"
#include "item.h"
#include "coord.h"
#include "transform.h"
#include "projection.h"
/**
 * @defgroup coord Coordinate handling functions
 * @{
 */

/**
 * Get a coordinate
 *
 * @param p Pointer to the coordinate
 * @returns the coordinate
 */

struct coord *
coord_get(unsigned char **p) {
    struct coord *ret=(struct coord *)(*p);
    *p += sizeof(*ret);
    return ret;
}

struct coord *
coord_new(int x, int y) {
    struct coord *c=g_new(struct coord, 1);

    c->x=x;
    c->y=y;

    return c;
}

struct coord *
coord_new_from_attrs(struct attr *parent, struct attr **attrs) {
    struct attr *x,*y;
    x=attr_search(attrs, NULL, attr_x);
    y=attr_search(attrs, NULL, attr_y);
    if (!x || !y)
        return NULL;
    return coord_new(x->u.num, y->u.num);
}


void coord_destroy(struct coord *c) {
    g_free(c);
}

struct coord_rect *
coord_rect_new(struct coord *lu, struct coord *rl) {
    struct coord_rect *r=g_new(struct coord_rect, 1);

    dbg_assert(lu->x <= rl->x);
    dbg_assert(lu->y >= rl->y);

    r->lu=*lu;
    r->rl=*rl;

    return r;

}

void coord_rect_destroy(struct coord_rect *r) {
    g_free(r);
}

int coord_rect_overlap(struct coord_rect *r1, struct coord_rect *r2) {
    dbg_assert(r1->lu.x <= r1->rl.x);
    dbg_assert(r1->lu.y >= r1->rl.y);
    dbg_assert(r2->lu.x <= r2->rl.x);
    dbg_assert(r2->lu.y >= r2->rl.y);
    dbg(lvl_debug,"0x%x,0x%x - 0x%x,0x%x vs 0x%x,0x%x - 0x%x,0x%x", r1->lu.x, r1->lu.y, r1->rl.x, r1->rl.y, r2->lu.x,
        r2->lu.y, r2->rl.x, r2->rl.y);
    if (r1->lu.x > r2->rl.x)
        return 0;
    if (r1->rl.x < r2->lu.x)
        return 0;
    if (r1->lu.y < r2->rl.y)
        return 0;
    if (r1->rl.y > r2->lu.y)
        return 0;
    return 1;
}

int coord_rect_contains(struct coord_rect *r, struct coord *c) {
    dbg_assert(r->lu.x <= r->rl.x);
    dbg_assert(r->lu.y >= r->rl.y);
    if (c->x < r->lu.x)
        return 0;
    if (c->x > r->rl.x)
        return 0;
    if (c->y < r->rl.y)
        return 0;
    if (c->y > r->lu.y)
        return 0;
    return 1;
}

void coord_rect_extend(struct coord_rect *r, struct coord *c) {
    if (c->x < r->lu.x)
        r->lu.x=c->x;
    if (c->x > r->rl.x)
        r->rl.x=c->x;
    if (c->y < r->rl.y)
        r->rl.y=c->y;
    if (c->y > r->lu.y)
        r->lu.y=c->y;
}

/**
 * Parses \c char \a *coord_input and writes back the coordinates to \c coord \a *result, using \c projection \a output_projection.
 * \a *coord_input may specify its projection at the beginning.
 * The format for \a *coord_input can be:
 * 	\li [Proj:][-]0xXX.... [-]0xXX... - Mercator coordinates, hex integers (XX), Proj can be "mg" or "garmin", defaults to mg
 * 	\li [Proj:][D][D]Dmm.mm.. N/S [D][D]DMM.mm... E/W - lat/long (WGS 84), integer degrees (DD) and minutes as decimal fraction (MM), Proj must be "geo" or absent
 * 	\li [Proj:][-][D]D.d[d]... [-][D][D]D.d[d] - long/lat (WGS 84, note order!), degrees as decimal fraction, Proj does not matter
 * 	\li utm[zoneinfo]:[-][D]D.d[d]... [-][D][D]D.d[d] - UTM coordinates, as decimal fraction, with optional zone information (?)
 * Note that the spaces are relevant for parsing.
 *
 * @param *coord_input String to be parsed
 * @param output_projection Desired projection of the result
 * @param *result For returning result
 * @returns The lenght of the parsed string
 */

int coord_parse(const char *coord_input, enum projection output_projection, struct coord *result) {
    char *proj=NULL,*s,*co;
    const char *str=coord_input;
    int args,ret = 0;
    struct coord_geo g;
    struct coord c,offset;
    enum projection str_pro=projection_none;

    dbg(lvl_debug,"enter('%s',%d,%p)", coord_input, output_projection, result);
    s=strchr(str,' ');
    co=strchr(str,':');
    if (co && co < s) {
        proj=malloc(co-str+1);
        strncpy(proj, str, co-str);
        proj[co-str]='\0';
        dbg(lvl_debug,"projection=%s", proj);
        str=co+1;
        s=strchr(str,' ');
        if (!strcmp(proj, "geo"))
            str_pro = projection_none;
        else {
            str_pro = projection_from_name(proj,&offset);
            if (str_pro == projection_none) {
                dbg(lvl_error, "Unknown projection: %s", proj);
                goto out;
            }
        }
    }
    if (! s) {
        ret=0;
        goto out;
    }
    while (*s == ' ') {
        s++;
    }
    if (!strncmp(s, "0x", 2) || !strncmp(s, "-0x", 3)) {
        args=sscanf(str, "%i %i%n",&c.x, &c.y, &ret);
        if (args < 2)
            goto out;
        dbg(lvl_debug,"str='%s' x=0x%x y=0x%x c=%d", str, c.x, c.y, ret);
        dbg(lvl_debug,"rest='%s'", str+ret);

        if (str_pro == projection_none)
            str_pro=projection_mg;
        if (str_pro != output_projection) {
            transform_to_geo(str_pro, &c, &g);
            transform_from_geo(output_projection, &g, &c);
        }
        *result=c;
    } else if (*s == 'N' || *s == 'n' || *s == 'S' || *s == 's') {
        double lng, lat;
        char ns, ew;
        dbg(lvl_debug,"str='%s'", str);
        args=sscanf(str, "%lf %c %lf %c%n", &lat, &ns, &lng, &ew, &ret);
        dbg(lvl_debug,"args=%d", args);
        dbg(lvl_debug,"lat=%f %c lon=%f %c", lat, ns, lng, ew);
        if (args < 4)
            goto out;
        dbg(lvl_debug,"projection=%d str_pro=%d projection_none=%d", output_projection, str_pro, projection_none);
        if (str_pro == projection_none) {
            g.lat=floor(lat/100);
            lat-=g.lat*100;
            g.lat+=lat/60;
            g.lng=floor(lng/100);
            lng-=g.lng*100;
            g.lng+=lng/60;
            if (ns == 's' || ns == 'S')
                g.lat=-g.lat;
            if (ew == 'w' || ew == 'W')
                g.lng=-g.lng;
            dbg(lvl_debug,"transform_from_geo(%f,%f)",g.lat,g.lng);
            transform_from_geo(output_projection, &g, result);
            dbg(lvl_debug,"result 0x%x,0x%x", result->x,result->y);
        }
        dbg(lvl_debug,"str='%s' x=%f ns=%c y=%f ew=%c c=%d", str, lng, ns, lat, ew, ret);
        dbg(lvl_debug,"rest='%s'", str+ret);
    } else if (str_pro == projection_utm) {
        double x,y;
        args=sscanf(str, "%lf %lf%n", &x, &y, &ret);
        if (args < 2)
            goto out;
        c.x=x+offset.x;
        c.y=y+offset.y;
        if (str_pro != output_projection) {
            transform_to_geo(str_pro, &c, &g);
            transform_from_geo(output_projection, &g, &c);
        }
        *result=c;
    } else {
        double lng, lat;
        args=sscanf(str, "%lf %lf%n", &lng, &lat, &ret);
        if (args < 2)
            goto out;
        dbg(lvl_debug,"str='%s' x=%f y=%f  c=%d", str, lng, lat, ret);
        dbg(lvl_debug,"rest='%s'", str+ret);
        g.lng=lng;
        g.lat=lat;
        transform_from_geo(output_projection, &g, result);
    }
    ret+=str-coord_input;
    dbg(lvl_info, "ret=%d delta=%d ret_str='%s'", ret, GPOINTER_TO_INT(str-coord_input), coord_input+ret);
out:
    free(proj);
    return ret;
}

/**
 * A wrapper for coord_parse that also returns the projection.
 * For parameters see coord_parse.
 */

int pcoord_parse(const char *c_str, enum projection pro, struct pcoord *pc_ret) {
    struct coord c;
    int ret;
    ret = coord_parse(c_str, pro, &c);
    pc_ret->x = c.x;
    pc_ret->y = c.y;
    pc_ret->pro = pro;
    return ret;
}

void coord_print(enum projection pro, struct coord *c, FILE *out) {
    unsigned int x;
    unsigned int y;
    char *sign_x = "";
    char *sign_y = "";

    if ( c->x < 0 ) {
        x = -c->x;
        sign_x = "-";
    } else {
        x = c->x;
    }
    if ( c->y < 0 ) {
        y = -c->y;
        sign_y = "-";
    } else {
        y = c->y;
    }
    fprintf( out, "%s: %s0x%x %s0x%x\n",
             projection_to_name( pro ),
             sign_x, x,
             sign_y, y );
    return;
}

/**
 * @brief Converts a lat/lon into a text formatted text string.
 * @param lat The latitude (if lat is 360 or greater, the latitude will be omitted)
 * @param lng The longitude (if lng is 360 or greater, the longitude will be omitted)
 * @param fmt The format to use.
 *    @li DEGREES_DECIMAL=>Degrees with decimal places, i.e. 20.5000°N 110.5000°E
 *    @li DEGREES_MINUTES=>Degrees and minutes, i.e. 20°30.00'N 110°30.00'E
 *    @li DEGREES_MINUTES_SECONDS=>Degrees, minutes and seconds, i.e. 20°30'30.00"N 110°30'30"E
 *
 *
 * @param buffer  A buffer large enough to hold the output + a terminating NULL (up to 31 bytes)
 * @param size The size of the buffer
 *
 */
void coord_format(float lat,float lng, enum coord_format fmt, char * buffer, int size) {

    char lat_c='N';
    char lng_c='E';
    float lat_deg,lat_min,lat_sec;
    float lng_deg,lng_min,lng_sec;
    int size_used=0;

    if (lng < 0) {
        lng=-lng;
        lng_c='W';
    }
    if (lat < 0) {
        lat=-lat;
        lat_c='S';
    }
    lat_deg=lat;
    lat_min=(lat-floor(lat_deg))*60;
    lat_sec=fmod(lat*3600,60);
    lng_deg=lng;
    lng_min=(lng-floor(lng_deg))*60;
    lng_sec=fmod(lng*3600,60);
    switch(fmt) {

    case DEGREES_DECIMAL:
        if (lat<360)
            size_used+=g_snprintf(buffer+size_used,size-size_used,"%02.6f°%c",lat,lat_c);
        if ((lat<360)&&(lng<360))
            size_used+=g_snprintf(buffer+size_used,size-size_used," ");
        if (lng<360)
            size_used+=g_snprintf(buffer+size_used,size-size_used,"%03.7f°%c",lng,lng_c);
        break;
    case DEGREES_MINUTES:
        if (lat<360)
            size_used+=g_snprintf(buffer+size_used,size-size_used,"%02.0f°%07.4f' %c",floor(lat_deg),lat_min,lat_c);
        if ((lat<360)&&(lng<360))
            size_used+=g_snprintf(buffer+size_used,size-size_used," ");
        if (lng<360)
            size_used+=g_snprintf(buffer+size_used,size-size_used,"%03.0f°%07.4f' %c",floor(lng_deg),lng_min,lng_c);
        break;
    case DEGREES_MINUTES_SECONDS:
        if (lat<360)
            size_used+=g_snprintf(buffer+size_used,size-size_used,"%02.0f°%02.0f'%05.2f\" %c",floor(lat_deg),floor(lat_min),
                                  lat_sec,lat_c);
        if ((lat<360)&&(lng<360))
            size_used+=g_snprintf(buffer+size_used,size-size_used," ");
        if (lng<360)
            size_used+=g_snprintf(buffer+size_used,size-size_used,"%03.0f°%02.0f'%05.2f\" %c",floor(lng_deg),floor(lng_min),
                                  lng_sec,lng_c);
        break;


    }

}

unsigned int coord_hash(const void *key) {
    const struct coord *c=key;
    return c->x^c->y;
}

int coord_equal(const void *a, const void *b) {
    const struct coord *c_a=a;
    const struct coord *c_b=b;
    if (c_a->x == c_b->x && c_a->y == c_b->y)
        return TRUE;
    return FALSE;
}
/** @} */
