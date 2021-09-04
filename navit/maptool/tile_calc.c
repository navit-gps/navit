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
#if 0
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

void tile_bbox(char *tile, int len, struct rect *r, int overlap) {
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

#else

static int overlaps_bbox(int xl, int yl, int xh, int yh, struct rect *r) {
    int delta =0;
    if(r->h.x < xl) {
        int tmp = xl - r->h.x;
        if(tmp > delta)
            delta = tmp;
    }
    if(r->h.x > xh) {
        int tmp = r->h.x - xh;
        if(tmp > delta)
            delta = tmp;
    }
    if(r->l.x > xh) {
        int tmp = r->l.x - xh;
        if(tmp > delta)
            delta = tmp;
    }
    if(r->l.x < xl) {
        int tmp = xl- r->l.x;
        if(tmp > delta)
            delta = tmp;
    }
    if(r->h.y < yl) {
        int tmp = yl - r->h.y;
        if(tmp > delta)
            delta = tmp;
    }
    if(r->h.y > yh) {
        int tmp = r->h.y - yh;
        if(tmp > delta)
            delta = tmp;
    }
    if(r->l.y > yh) {
        int tmp = r->l.y - yh;
        if(tmp > delta)
            delta = tmp;
    }
    if(r->l.y < yl) {
        int tmp = yl - r->l.y;
        if(tmp > delta)
            delta = tmp;
    }

    return delta;
}

void tile_bbox(char *tile, int len, struct rect *r, int overlap) {
    struct coord c;
    int xo,yo;
    *r=world_bbox;
    while ((*tile) && (len)) {
        //calculate next center point
        c.x=(r->l.x+r->h.x)/2;
        c.y=(r->l.y+r->h.y)/2;
        //calculate next overlap size
        xo=(r->h.x-r->l.x)*overlap/100;
        yo=(r->h.y-r->l.y)*overlap/100;
        //calculate subtile based on current tile character
        switch (*tile) {
        case 'a':
            r->l.x=c.x;
            r->l.y=c.y;
            break;
        case 'b':
            r->h.x=c.x;
            r->l.y=c.y;
            break;
        case 'c':
            r->l.x=c.x;
            r->h.y=c.y;
            break;
        case 'd':
            r->h.x=c.x;
            r->h.y=c.y;
            break;
        }
        //if deepest tile, add overlap.
        if((!(*(tile+1))) || (!(len -1))) {
            switch (*tile) {
            case 'a':
                r->l.x-=xo;
                r->l.y-=yo;
                r->h.x+=xo;
                r->h.y+=yo;
                break;
            case 'b':
                r->h.x+=xo;
                r->l.y-=yo;
                r->l.x-=xo;
                r->h.y+=yo;
                break;
            case 'c':
                r->l.x-=xo;
                r->h.y+=yo;
                r->h.x+=xo;
                r->l.y-=yo;
                break;
            case 'd':
                r->h.x+=xo;
                r->h.y+=yo;
                r->l.x-=xo;
                r->l.y-=yo;
                break;
            }
        }
        //next tile level
        tile++;
        len--;
    }
}

static void calculate_subtile(struct rect * tile, struct rect * a, struct rect *b, struct rect *c, struct rect * d,
                              int overlap) {
    struct coord cp; /* the center point */
    int xo,yo; /* the overlap */
    *a=*tile;
    *b=*tile;
    *c=*tile;
    *d=*tile;
    /* calculate the next center point */
    cp.x=(tile->l.x+tile->h.x)/2;
    cp.y=(tile->l.y+tile->h.y)/2;
    /* calculate next overlap size */
    xo=(tile->h.x-tile->l.x)*overlap/100;
    yo=(tile->h.y-tile->l.y)*overlap/100;
    /* calculate a, b, c, d without overlap */
    a->l.x=cp.x;
    a->l.y=cp.y;
    b->h.x=cp.x;
    b->l.y=cp.y;
    c->l.x=cp.x;
    c->h.y=cp.y;
    d->h.x=cp.x;
    d->h.y=cp.y;
    /* add overlaps */
    a->l.x-=xo;
    a->l.y-=yo;
    a->h.x+=xo;
    a->h.y+=yo;
    b->h.x+=xo;
    b->l.y-=yo;
    b->l.x-=xo;
    b->h.y+=yo;
    c->l.x-=xo;
    c->h.y+=yo;
    c->h.x+=xo;
    c->l.y-=yo;
    d->h.x+=xo;
    d->h.y+=yo;
    d->l.x-=xo;
    d->l.y-=yo;

}

/**
* @brief Calculate the tile address of a guiven map rectangle
*
* @param r - rectangle
* @param suffix - additional text to add to the tile address
* @param ret - buffer to write tile address into. Must be at least 15 + strlen(suffix) long
* @param max - smallest tile number to check rectangle against.
* @param overlap - allow the items within the tiles to overlap by %
* @param tr - returns the choosen tile bbox with overlaps.
*
* @returns number of tile depth
*/
int tile(struct rect *r, char *suffix, char *ret, int max, int overlap, struct rect *tr) {
    /* start at tile 0, the whole world */
    int depth = 0;
    struct rect tile = world_bbox;
    /* it will always match the world */
    int found=1;

    /* try for all allowed depths (up to max) */
    while ((found == 1) && (depth < max)) {
        struct rect a; /* the new a */
        struct rect b; /* the new b */
        struct rect c; /* the new c */
        struct rect d; /* the new d */
        calculate_subtile(&tile, &a, &b, &c, &d, 0);
        /* add overlaps */
        struct rect aov; /* the new a */
        struct rect bov; /* the new b */
        struct rect cov; /* the new c */
        struct rect dov; /* the new d */
        calculate_subtile(&tile, &aov, &bov, &cov, &dov, overlap);

        /* default to no match */
        found = 0;

        /* check if the bbox fits in one of the four sub tiles without overlap */
        /* try a */
        if (contains_bbox(a.l.x,a.l.y,a.h.x,a.h.y, r)) {
            found = 0;
            *ret='a';
            tile = a;
            if(tr != NULL)
                *tr=aov;
        }
        /* try b */
        else if (contains_bbox(b.l.x,b.l.y,b.h.x,b.h.y, r)) {
            found = 0;
            *ret='b';
            tile = b;
            if(tr != NULL)
                *tr=bov;
        }
        /* try c */
        else if (contains_bbox(c.l.x,c.l.y,c.h.x,c.h.y, r)) {
            found = 0;
            *ret='c';
            tile = c;
            if(tr != NULL)
                *tr=cov;
        }
        /* try d */
        else if (contains_bbox(d.l.x,d.l.y,d.h.x,d.h.y, r)) {
            found = 0;
            *ret='d';
            tile = d;
            if(tr != NULL)
                *tr=dov;
        }
        /* we have a match. One more time */
        if(found==1) {
            ret ++;
            depth++;
        } else {
            int overlap = 0;
            /* did not fit in any sub tile. So we're stuck at this level*/
            /* check if the bbox fits in one of the four sub tiles with overlap */
            /* try a */
            if (contains_bbox(aov.l.x,aov.l.y,aov.h.x,aov.h.y, r)) {
                overlap = overlaps_bbox(a.l.x, a.l.y, a.h.x, a.h.y, r);
                found = 1;
                *ret='a';
                tile = a;
                if(tr != NULL)
                    *tr=aov;
            }
            /* try b */
            if (contains_bbox(bov.l.x,bov.l.y,bov.h.x,bov.h.y, r)) {
                int tmp = overlaps_bbox(b.l.x, b.l.y, b.h.x, b.h.y, r);
                if((found != 1) || (tmp < overlap)) {
                    overlap = tmp;
                    found = 1;
                    *ret='b';
                    tile = b;
                    if(tr != NULL)
                        *tr=bov;
                }
            }
            /* try c */
            if (contains_bbox(cov.l.x,cov.l.y,cov.h.x,cov.h.y, r)) {
                int tmp = overlaps_bbox(c.l.x, c.l.y, c.h.x, c.h.y, r);
                if((found!=1) || (tmp < overlap)) {
                    overlap = tmp;
                    found = 1;
                    *ret='c';
                    tile = c;
                    if(tr != NULL)
                        *tr=cov;
                }
            }
            /* try d */
            if (contains_bbox(dov.l.x,dov.l.y,dov.h.x,dov.h.y, r)) {
                int tmp = overlaps_bbox(d.l.x, d.l.y, d.h.x, d.h.y, r);
                if((found!=1) || (tmp < overlap)) {
                    overlap = tmp;
                    found = 1;
                    *ret='d';
                    tile = d;
                    if(tr != NULL)
                        *tr=dov;
                }
            }
            if(found==1) {
                ret ++;
                depth++;
            }
        }
    }
    /* close the string in ret */
    *ret=0;
    /* add suffix */
    if (suffix)
        strcat(ret,suffix);
    /* return tile depth */
    return depth;
}

#endif
