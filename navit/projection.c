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

#include <string.h>
#include <glib.h>
#include "coord.h"
#include "debug.h"
#include "projection.h"

struct projection_name {
    enum projection projection;
    char *name;
};


struct projection_name projection_names[]= {
    {projection_none, ""},
    {projection_mg, "mg"},
    {projection_garmin, "garmin"},
    {projection_utm, "utm"},
};

static int utmref_letter(char l) {
    if (l < 'a' || l == 'i' || l == 'o')
        return -1;
    if (l < 'i')
        return l-'a';
    if (l < 'o')
        return l-'a'-1;
    if (l <= 'z')
        return l-'a'-2;
    return -1;
}

/**
 * Look up a projection by name.
 *
 * @param name name of projection to look up (values from projection_names, and UTM projections)
 * @utm_offset Only for UTM projections: Used to return the offset for the UTM projection
 * @returns projection, or projection_none if no projection could be determined
 */
enum projection projection_from_name(const char *name, struct coord *utm_offset) {
    int i;
    int zone,baserow;
    char ns,zone_field,square_x,square_y;

    for (i=0 ; i < sizeof(projection_names)/sizeof(struct projection_name) ; i++) {
        if (! strcmp(projection_names[i].name, name))
            return projection_names[i].projection;
    }
    if (utm_offset) {
        if (sscanf(name,"utm%d%c",&zone,&ns) == 2 && zone > 0 && zone <= 60 && (ns == 'n' || ns == 's')) {
            utm_offset->x=zone*1000000;
            utm_offset->y=(ns == 's' ? -10000000:0);
            return projection_utm;
        }
        if (sscanf(name,"utmref%d%c%c%c",&zone,&zone_field,&square_x,&square_y)) {
            i=utmref_letter(zone_field);
            if (i < 2 || i > 21) {
                dbg(lvl_error,"invalid zone field '%c' in '%s'",zone_field,name);
                return projection_none;
            }
            i-=12;
            dbg(lvl_debug,"zone_field %d",i);
            baserow=i*887.6/100;
            utm_offset->x=zone*1000000;
            i=utmref_letter(square_x);
            utm_offset->x+=((i%8)+1)*100000;
            i=utmref_letter(square_y);
            dbg(lvl_debug,"baserow %d",baserow);
            if (!(zone % 2))
                i-=5;
            dbg(lvl_debug,"i=%d",i);
            i=(i-baserow+100)%20+baserow;
            utm_offset->y=i*100000;
            return projection_utm;
        }
    }
    return projection_none;
}

char *projection_to_name(enum projection proj) {
    int i;

    for (i=0 ; i < sizeof(projection_names)/sizeof(struct projection_name) ; i++) {
        if (projection_names[i].projection == proj)
            return projection_names[i].name;
    }
    return NULL;
}

