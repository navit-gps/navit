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


#include <string.h>
#include "maptool.h"
#include "item.h"

int tile(struct rect *r, char *suffix, char *ret, int max, int overlap, struct rect *tr) {
    int x0,x2,x4;
    int y0,y2,y4;
    int xo,yo;
    int i;
    struct rect rr=*r;

    x0=world_bbox.l.x;
    y0=world_bbox.l.y;
    x4=world_bbox.h.x;
    y4=world_bbox.h.y;

    if(rr.l.x<x0)
        rr.l.x=x0;
    if(rr.h.x<x0)
        rr.h.x=x0;
    if(rr.l.y<y0)
        rr.l.y=y0;
    if(rr.h.y<y0)
        rr.h.y=y0;
    if(rr.l.x>x4)
        rr.l.x=x4;
    if(rr.h.x>x4)
        rr.h.x=x4;
    if(rr.l.y>y4)
        rr.l.y=y4;
    if(rr.h.y>y4)
        rr.h.y=y4;

    for (i = 0 ; i < max ; i++) {
        x2=(x0+x4)/2;
        y2=(y0+y4)/2;
        xo=(x4-x0)*overlap/100;
        yo=(y4-y0)*overlap/100;
        if (     contains_bbox(x0,y0,x2+xo,y2+yo,&rr)) {
            strcat(ret,"d");
            x4=x2+xo;
            y4=y2+yo;
        } else if (contains_bbox(x2-xo,y0,x4,y2+yo,&rr)) {
            strcat(ret,"c");
            x0=x2-xo;
            y4=y2+yo;
        } else if (contains_bbox(x0,y2-yo,x2+xo,y4,&rr)) {
            strcat(ret,"b");
            x4=x2+xo;
            y0=y2-yo;
        } else if (contains_bbox(x2-xo,y2-yo,x4,y4,&rr)) {
            strcat(ret,"a");
            x0=x2-xo;
            y0=y2-yo;
        } else
            break;
    }
    if (tr) {
        tr->l.x=x0;
        tr->l.y=y0;
        tr->h.x=x4;
        tr->h.y=y4;
    }
    if (suffix)
        strcat(ret,suffix);
    return i;
}

void tile_bbox(char *tile, struct rect *r, int overlap) {
    struct coord c;
    int xo,yo;
    *r=world_bbox;
    while (*tile) {
        c.x=(r->l.x+r->h.x)/2;
        c.y=(r->l.y+r->h.y)/2;
        xo=(r->h.x-r->l.x)*overlap/100;
        yo=(r->h.y-r->l.y)*overlap/100;
        switch (*tile) {
        case 'a':
            r->l.x=c.x-xo;
            r->l.y=c.y-yo;
            break;
        case 'b':
            r->h.x=c.x+xo;
            r->l.y=c.y-yo;
            break;
        case 'c':
            r->l.x=c.x-xo;
            r->h.y=c.y+yo;
            break;
        case 'd':
            r->h.x=c.x+xo;
            r->h.y=c.y+yo;
            break;
        }
        tile++;
    }
}

