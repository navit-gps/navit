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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *
 * Boston, MA  02110-1301, USA.
 */

#include <stdlib.h>
#include <glib.h>
#include <string.h>
#include <math.h>
#include "debug.h"
#include "projection.h"
#include "item.h"
#include "xmlconfig.h"
#include "map.h"
#include "mapset.h"
#include "coord.h"
#include "transform.h"
#include "search.h"
#include "country.h"
#include "linguistics.h"
#include "geom.h"
#include "util.h"
#include "search_houseno_interpol.h"

struct hn_interpol_attr {
    enum attr_type house_number_interpol_attr;
    int interpol_increment;
    enum include_end_nodes include_end_nodes;
};

#define house_number_interpol_attr_END -1
/**
 * Attributes that indicate a house number interpolation,
 * along with interpolation information.
 */
struct hn_interpol_attr house_number_interpol_attrs[] = {
    { attr_house_number_left,       1, end_nodes_yes },
    { attr_house_number_left_odd,   2, end_nodes_yes },
    { attr_house_number_left_even,  2, end_nodes_yes },
    { attr_house_number_right,      1, end_nodes_yes },
    { attr_house_number_right_odd,  2, end_nodes_yes },
    { attr_house_number_right_even, 2, end_nodes_yes },
    { attr_house_number_interpolation_no_ends_incrmt_1, 1, end_nodes_no },
    { attr_house_number_interpolation_no_ends_incrmt_2, 2, end_nodes_no },
    { house_number_interpol_attr_END, -1, -1 },
};

void house_number_interpolation_clear_current(struct house_number_interpolation *inter) {
    g_free(inter->first);
    g_free(inter->last);
    g_free(inter->curr);
    inter->first=inter->last=inter->curr=NULL;
    inter->increment=inter->include_end_nodes=-1;
}

void house_number_interpolation_clear_all(struct house_number_interpolation *inter) {
    inter->curr_interpol_attr_idx=0;
    house_number_interpolation_clear_current(inter);
}

static char *search_next_house_number_curr_interpol_with_ends(struct house_number_interpolation *inter) {
    dbg(lvl_debug,"interpolate %s-%s %s",inter->first,inter->last,inter->curr);
    if (!inter->first || !inter->last)
        return NULL;
    if (!inter->curr)
        inter->curr=g_strdup(inter->first);
    else {
        if (strcmp(inter->curr, inter->last)) {
            int next=atoi(inter->curr)+(inter->increment);
            g_free(inter->curr);
            if (next == atoi(inter->last))
                inter->curr=g_strdup(inter->last);
            else
                inter->curr=g_strdup_printf("%d",next);
        } else {
            g_free(inter->curr);
            inter->curr=NULL;
        }
    }
    dbg(lvl_debug,"interpolate result %s",inter->curr);
    return inter->curr;
}

static int house_number_is_end_number(char* house_number, struct house_number_interpolation *inter) {
    return ( (!strcmp(house_number, inter->first))
             || (!strcmp(house_number, inter->last)) );
}

static char *search_next_house_number_curr_interpol(struct house_number_interpolation *inter) {
    char* hn=NULL;
    switch (inter->include_end_nodes) {
    case end_nodes_yes:
        hn=search_next_house_number_curr_interpol_with_ends(inter);
        break;
    case end_nodes_no:
        do {
            hn=search_next_house_number_curr_interpol_with_ends(inter);
        } while (hn!=NULL && house_number_is_end_number(hn, inter));
        break;
    }
    return hn;
}

static void search_house_number_interpolation_split(char *str, struct house_number_interpolation *inter) {
    char *pos=strchr(str,'-');
    char *first,*last;
    int len;
    if (!pos) {
        inter->first=g_strdup(str);
        inter->last=g_strdup(str);
        inter->rev=0;
        return;
    }
    len=pos-str;
    first=g_malloc(len+1);
    strncpy(first, str, len);
    first[len]='\0';
    last=g_strdup(pos+1);
    dbg(lvl_debug,"%s = %s - %s",str, first, last);
    if (atoi(first) > atoi(last)) {
        inter->first=last;
        inter->last=first;
        inter->rev=1;
    } else {
        inter->first=first;
        inter->last=last;
        inter->rev=0;
    }
}

struct pcoord *
search_house_number_coordinate(struct item *item, struct house_number_interpolation *inter) {
    struct pcoord *ret=g_new(struct pcoord, 1);
    ret->pro = map_projection(item->map);
    dbg(lvl_debug,"%s",item_to_name(item->type));
    if (!inter) {
        struct coord c;
        if (item_coord_get(item, &c, 1)) {
            ret->x=c.x;
            ret->y=c.y;
        } else {
            g_free(ret);
            ret=NULL;
        }
    } else {
        int count,max=1024;
        int hn_pos,hn_length;
        int inter_increment=inter->increment;
        struct coord *c=g_alloca(sizeof(struct coord)*max);
        item_coord_rewind(item);
        count=item_coord_get(item, c, max);
        hn_length=atoi(inter->last)-atoi(inter->first);
        if (inter->rev)
            hn_pos=atoi(inter->last)-atoi(inter->curr);
        else
            hn_pos=atoi(inter->curr)-atoi(inter->first);
        if (count) {
            int i,distance_sum=0,hn_distance;
            int *distances=g_alloca(sizeof(int)*(count-1));
            dbg(lvl_debug,"count=%d hn_length=%d hn_pos=%d (%s of %s-%s)",count,hn_length,hn_pos,inter->curr,inter->first,
                inter->last);
            if (!hn_length) {
                hn_length=2;
                hn_pos=1;
            }
            if (count == max)
                dbg(lvl_error,"coordinate overflow");
            for (i = 0 ; i < count-1 ; i++) {
                distances[i]=navit_sqrt(transform_distance_sq(&c[i],&c[i+1]));
                distance_sum+=distances[i];
                dbg(lvl_debug,"distance[%d]=%d",i,distances[i]);
            }
            dbg(lvl_debug,"sum=%d",distance_sum);
#if 0
            hn_distance=distance_sum*hn_pos/hn_length;
#else
            hn_distance=(distance_sum*hn_pos+distance_sum*inter_increment/2)/(hn_length+inter_increment);
#endif
            dbg(lvl_debug,"hn_distance=%d",hn_distance);
            i=0;
            while (i < count-1 && hn_distance > distances[i])
                hn_distance-=distances[i++];
            dbg(lvl_debug,"remaining distance=%d from %d",hn_distance,distances[i]);
            ret->x=(c[i+1].x-c[i].x)*hn_distance/distances[i]+c[i].x;
            ret->y=(c[i+1].y-c[i].y)*hn_distance/distances[i]+c[i].y;
        }
    }
    return ret;
}

static int search_match(char *str, char *search, int partial) {
    if (!partial)
        return (!g_ascii_strcasecmp(str, search));
    else
        return (!g_ascii_strncasecmp(str, search, strlen(search)));
}

char *search_next_interpolated_house_number(struct item *item, struct house_number_interpolation *inter,
        char *inter_match,
        int inter_partial) {
    while (1) {
        char *hn;
        struct attr attr;
        struct hn_interpol_attr curr_interpol_attr;
        while((hn=search_next_house_number_curr_interpol(inter))) {
            if (search_match(hn, inter_match, inter_partial)) {
                return map_convert_string(item->map, hn);
            }
        }

        house_number_interpolation_clear_current(inter);
        curr_interpol_attr=house_number_interpol_attrs[inter->curr_interpol_attr_idx];
        if (curr_interpol_attr.house_number_interpol_attr==house_number_interpol_attr_END) {
            return NULL;
        }
        if (item_attr_get(item, curr_interpol_attr.house_number_interpol_attr, &attr)) {
            search_house_number_interpolation_split(attr.u.str, inter);
            inter->increment=curr_interpol_attr.interpol_increment;
            inter->include_end_nodes=curr_interpol_attr.include_end_nodes;
        }
        inter->curr_interpol_attr_idx++;
    }
}

