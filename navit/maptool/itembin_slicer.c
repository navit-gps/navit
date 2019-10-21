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


#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <stdlib.h>
#include <glib.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <math.h>
#ifndef _MSC_VER
#include <getopt.h>
#include <unistd.h>
#endif
#include <fcntl.h>
#include <sys/stat.h>
#include <zlib.h>
#include "file.h"
#include "item.h"
#include "map.h"
#include "zipfile.h"
#include "main.h"
#include "config.h"
#include "linguistics.h"
#include "plugin.h"

#include "maptool.h"

/**
 * @brief advance tile number on same tile level
 * @param[inout] buffer current tile number, after call next tile number
 * @return void
 */
static void next_tile(char * buffer) {
    int done = 0;
    int len;
    len = strlen(buffer);

    while ((len > 0) && (!done)) {
        if(buffer[len-1] == 'd') {
            buffer[len-1] = 'a';
            len --;
        } else {
            buffer[len-1] ++;
            done = 1;
        }
    }
}

/**
 * @brief check if rectangles overlap
 *
 * Ths function calculates if two rectangles overlap
 * @param[in] b1 - one rectangle
 * @param[in] b2 - another rectangle
 * @return 1 if they overlap, 0 otherwise
 */
static int itembin_bbox_intersects (struct rect * b1, struct rect * b2) {
    // If one rectangle is on left side of other
    if (b1->l.x > b2->h.x || b2->l.x > b1->h.x)
        return 0;

    // If one rectangle is above other
    if (b1->h.y < b2->l.y || b2->h.y < b1->l.y)
        return 0;

    return 1;
}

/**
 * @brief calculate intersection point of line with one side of an rectangle
 *
 * WARNING; Check if the line actually crosses the border before using this
 * function
 *
 * @param[in] p1 - one point of the line
 * @param[in] p2 - another point of the line
 * @param[in] rect - rectangle to use as cuttinng data
 * @param[in] edge - edge of rectangle to use. 0 left, 1 right, 2 bottom, 3 top
 * @param[out] ret - resulting intersection point
 */
static void itembin_poly_intersection(struct coord *p1, struct coord *p2, struct rect *r, int edge, struct coord *ret) {
    double dy=p2->y-p1->y;
    double dx=p2->x-p1->x;
    switch(edge) {
    case 0:
        ret->x=r->l.x;
        ret->y=p1->y+(r->l.x-p1->x)*dy/dx;
        break;
    case 1:
        ret->x=r->h.x;
        ret->y=p1->y+(r->h.x-p1->x)*dy/dx;
        break;
    case 2:
        ret->y=r->l.y;
        ret->x=p1->x+(r->l.y-p1->y)*dx/dy;
        break;
    case 3:
        ret->y=r->h.y;
        ret->x=p1->x+(r->h.y-p1->y)*dx/dy;
        break;
    }
}

/**
 * @brief calculate intersection point of line with x axis
 *
 * @param[in] p1 - one point of the line
 * @param[in] p2 - another point of the line
 * @param[in] x - x coordinate to check against
 * @param[out y - returns y coordinate of intersection of line with x if intersecting
 * @returns - 0 if not intersecting, 1 if y could be calculated.
 */
static int itembin_line_intersect_x(struct coord *p1, struct coord *p2, int x, int *y) {
    struct rect r;
    struct coord ret;
    memset(&r,0,sizeof(r));
    r.l.x = x;
    if(((p1->x > x) && (p2->x < x)) || ((p2->x >x) && (p1->x < x))) {
        itembin_poly_intersection(p1, p2, &r, 0, &ret);
        *y = ret.y;
        return 1;
    }
    return 0;
}

/**
 * @brief calculate if a point is inside a polygon
 *
 * This function calculates if a point is inside a polygon. To do this, it casts a ray along the
 * points x axis through the polygon and counts the intersections. If the count is even the point
 * is outside, if odd inside
 * @param[in] pt - point to check
 * @param[in] poly - polygon to check
 * @param[in] ccount - number of coordinates in poly
 * @return 0 if outside 1 if inside
 */
static int itembin_poly_is_in(struct coord * pt, struct coord * poly, int ccount) {
    int intersect_count=0;
    /* p is first element in current buffer */
    struct coord *p=poly;
    /* s is lasst element in current buffer */
    struct coord *s=poly+ ccount -1;
    int i;

    /* using x coordinate to trace. So pt is on trace */
    for(i=0; i < ccount; i ++) {
        if (p->x < pt->x) {
            if(s->x > pt->x) {
                int y=0;
                /* we crossed the ray. calculate crossing point*/
                if(itembin_line_intersect_x(p, s, pt->x, &y))
                    if(y < pt->y)
                        intersect_count ++;
            }
        } else {
            if(s->x < pt->x) {
                int y=0;
                /* we crossed the ray. calculate crossing point*/
                if(itembin_line_intersect_x(p, s, pt->x, &y))
                    if(y < pt->y)
                        intersect_count ++;
            }
        }
        /* move one coordinate forward */
        s=p;
        p++;
    }
    if((intersect_count %2) ==0)
        return 0;
    else
        return 1;

}

/**
 * @brief data structure to eas up slicing
 */
struct slice_result {
    int count; /**< number of loops / parts in coord */
    int * ccount; /**< array of numbers of coordinates per part*/
    struct coord ** coord; /**< array of coordinate arrays*/
};

/**
 * @brief data structure to transfer data between this and the multipolygon
 *  code in osm.c
 */
struct itembin_loop_combiner_context {
    /** combiner input */
    int count;
    int * ccount;
    struct item_bin ** part;
    /** combiner result */
    int sequence_count;
    int *scount;
    int *direction;
    int **sequences;
};

/**
 * @brief data structure to make partd of an item easily avalable for the slicer code
 */
struct slicerpolygon {
    struct item_bin * ib; /**< reference to the original item */
    /* transfer through the layers */
    FILE* reference;
    char * buffer;
    struct tile_info *info;
    int number;
    /* decoded data */
    struct rect bbox; /**< bbox of the polygons outline */
    int count; /**< number of coordinates in outer polygon */
    struct coord * poly; /**< outer polygon */
    int hole_count; /**< number of hole polygons */
    int * ccount; /**< number of coordinates per hole polygon */
    struct coord ** holes; /**< the hole polygons */
    struct rect * holes_bbox; /**< bboxes of the hole polygons */
    int attr_len; /**< length of attrs in bytes including the holes as is */
    struct attr_bin * attrs; /**< first byte of the attrs */
    int f_attr_len; /**< number of bytes attrs without the holes */
    struct attr_bin * f_attrs; /**< first byte of attr imege with holes removed */
};

/**
 * @brief free all possibly allocated members of a itembin_loop_combiner_context
 *
 * @param c - context to clear
 */
static void clear_loop_combiner_context(struct itembin_loop_combiner_context * c) {
    int i;
    for(i=0; i < c->count; i ++) {
        if(c->part[i] != NULL)
            g_free(c->part[i]);
    }
    if(c->ccount != NULL)
        g_free(c->ccount);

    if(c->part != NULL)
        g_free(c->part);

    for(i=0; i < c->sequence_count; i ++) {
        if(c->sequences[i] != NULL)
            g_free(c->sequences[i]);
    }
    if(c->scount != NULL)
        g_free(c->scount);

    if(c->direction != NULL)
        g_free(c->direction);

    if(c->sequences != NULL)
        g_free(c->sequences);
}

/**
 * @brief free all possibly allocated members of a slice_result
 *
 * @param r - context to clear
 */
static void clear_slice_result (struct slice_result * r) {
    int i;
    for(i=0; i < r->count; i ++) {
        if(r->coord[i] != NULL)
            g_free(r->coord[i]);
    }
    if(r->ccount !=NULL)
        g_free(r->ccount);
    if(r->coord !=NULL)
        g_free(r->coord);

    memset(r, 0, sizeof(*r));
}

/**
 * @brief add a part to a slice result structure
 *
 * This function adds a given line part to a slice result. It allocates the
 * required space for pointers, but keeps the memory of the parts coordinate array.
 * the coordinate should not be freed outside
 *
 * @param[in] r - slice result structure
 * @param[in] part - new part coordinate array to add
 * @param[in] ccount - number of coordinates in new part array.
 */
static void itembin_slice_add_part(struct slice_result *r, struct coord * part, int ccount) {
    r->ccount = g_realloc(r->ccount, sizeof(int) * (r->count +1));
    r->ccount[r->count] = ccount;
    r->coord = g_realloc(r->coord, sizeof(struct coord *) * (r->count +1));
    r->coord[r->count] = part;
    r->count ++;
}

#if 0
static void dump_coords(struct coord *coords, int ccount, FILE* file) {
    int i;
    for (i=0; i < ccount; i ++) {
        fprintf(file, "(%d,%d)",coords[i].x, coords[i].y);
    }
    fprintf(file, "\n");
}
#endif

/**
 * @brief combine the parts of a slice result to closed loops
 *
 * This function uses the osm.c multipolygon code to combine the given parts
 * of a slice resutl structure to closed loops,
 *
 * @param[inout] r - slice result to reassign to loops.
 */
static void itembin_loop_combiner (struct slice_result *r) {
    struct slice_result out;
    struct itembin_loop_combiner_context c;
    memset(&out, 0, sizeof(out));
    /* prepare context for reusing multipolygon processing parts */
    memset(&c, 0, sizeof(c));
    int i;
    c.count = r->count;
    c.ccount = g_malloc(r->count * sizeof(int));
    c.part = g_malloc(r->count * sizeof(struct item_bin *));
    for(i=0; i < c.count; i ++) {
        c.ccount[i] = r->ccount[i];
        c.part[i] = (struct item_bin *) g_malloc(r->ccount[i] * sizeof(struct coord) + sizeof(struct item_bin));
        c.part[i]->type = 0;
        c.part[i]->clen = r->ccount[i] * 2;
        memcpy(c.part[i] +1, r->coord[i], r->ccount[i] * sizeof(struct coord));
        c.part[i]->len = (sizeof(struct item_bin) /4) -1 + c.part[i]->clen;
    }
    /* call multipolygon processing part */
    c.sequence_count = process_multipolygons_find_loops(0, c.count, c.part, &(c.scount), &(c.sequences), &(c.direction));
    /* assemble new slice_result */
    for(i = 0; i < c.sequence_count; i ++) {
        int ccount = 0;
        struct coord * loop = NULL;
        /* count total coordinates for new loop */
        ccount = process_multipolygons_loop_count(c.part, c.scount[i], c.sequences[i]);
        /* alloc memory for new loop */
        loop = (struct coord *) g_malloc0(ccount * sizeof(struct coord));
        /* copy in coordinates */
        ccount = process_multipolygons_loop_dump(c.part, c.scount[i], c.sequences[i], c.direction, loop);
        /* attach to result */
        itembin_slice_add_part(&out, loop, ccount);
        /* do not free loop. */
    }

    //fprintf(stderr, "Combined to %d loops %d parts\n", c.sequence_count, out.count);
    clear_loop_combiner_context(&c);
    /* copy back the result */
    clear_slice_result(r);
    *r = out;

}

/**
 * @brief sort functions for qsort to sort coordinates along x or y axis
 * @param[in] a - coordinate a
 * @param[in] b - coordinate b
 * @param[in] axis - 1 if x axis, 0 if y axis
 * @return <1 a<b 0 a==b >1 a>b
 */
static inline int itembin_sort_coordinates_xy(const void * a, const void * b, int axis) {
    struct coord * c1 = (struct coord *) a;
    struct coord * c2 = (struct coord *) b;
    if(axis) {
        return (c1->x - c2->x);
    } else {
        return (c1->y - c2->y);
    }
}

/**
 * @brief sort functions for qsort to sort coordinates along x axis
 * @param[in] a - coordinate a
 * @param[in] b - coordinate b
 * @return <1 a<b 0 a==b >1 a>b
 */
static int itembin_sort_coordinates_x(const void * a, const void * b) {
    return itembin_sort_coordinates_xy(a,b,1);
}

/**
 * @brief sort functions for qsort to sort coordinates along y axis
 * @param[in] a - coordinate a
 * @param[in] b - coordinate b
 * @return <1 a<b 0 a==b >1 a>b
 */
static int itembin_sort_coordinates_y(const void * a, const void * b) {
    return itembin_sort_coordinates_xy(a,b,0);
}

/**
 * @brief srt coordinates along cut edge
 *
 * @param[inout] coord - array of points on one axis. Sorted on return.
 * @param[in] ccpunt - number of coords in coord
 * @param[in] edge - 0 or 1, sort along y axis, 2 or 3 sort along x axis
 */
static void itembin_sort_coordinates(struct coord * coord, int ccount, int edge) {
    if((edge==0) || (edge==1))
        qsort(coord, ccount,sizeof(struct coord),itembin_sort_coordinates_y);
    else
        qsort(coord, ccount,sizeof(struct coord),itembin_sort_coordinates_x);
}

static void itembin_slice_part_direction(struct coord *p_in, int p_in_ccount, struct slice_result * p_out, int edge,
        struct rect * box, struct coord ** cutpoints, int* cutpoint_ccount) {
    int i;

    /* get some space */
    int part_ccount =0;
    struct coord * part = g_malloc((p_in_ccount + 4) * sizeof(struct coord));
    *cutpoint_ccount =0;
    /* hold the cut points */
    *cutpoints=g_malloc((p_in_ccount * 8 + 4) * sizeof(struct coord));

    /* p is first element in current buffer */
    struct coord *p=p_in;
    /* s is lasst element in current buffer */
    struct coord *s=p_in+ p_in_ccount -1;

    /* iterate all point in this loop, remember the cut points, remember the parts */
    for(i=0; i < p_in_ccount; i ++) {
        if (geom_is_inside(p, box, edge)) {
            if (! geom_is_inside(s, box, edge)) {
                struct coord pi;
                /* current segment crosses border from outside to inside. Add crossing point with border first */
                itembin_poly_intersection(s,p,box,edge,&pi);
                part[part_ccount++]=pi;
                /* remember the cutpoint */
                (*cutpoints)[(*cutpoint_ccount)++]=pi;
            }
            /* add point if inside */
            part[part_ccount++]=*p;
        } else {
            if (geom_is_inside(s, box, edge)) {
                struct coord pi;
                /*current segment crosses border from inside to outside. Add crossing point with border */
                itembin_poly_intersection(p,s,box,edge,&pi);
                part[part_ccount++]=pi;
                /* remember the cutpoint */
                (*cutpoints)[(*cutpoint_ccount)++]=pi;
                /*this part is complete. add it to result */
                itembin_slice_add_part(p_out, part, part_ccount);
                /* do NOT free this part as it has been added */
                /* new buffer, new part */
                part = g_malloc((p_in_ccount +4) * sizeof(struct coord));
                part_ccount = 0;
            }
            /* skip point if outside */
        }
        /* move one coordinate forward */
        s=p;
        p++;
    }
    if(part_ccount > 0) {
        /* some points left. We started not at the border */
        if (geom_is_inside(p_in, box, edge))
            part[part_ccount++]=*(p_in);
        itembin_slice_add_part(p_out, part, part_ccount);
        /* do NOT free this part as it has been added */
    } else {
        /* we allocated a new buffer for the next part, but there were no points left. */
        g_free(part);
    }
}

static void itembin_slice_direction(struct slice_result * p_in_outer, struct slice_result *p_in_inner,
                                    struct slice_result * p_out_outer, struct slice_result * p_out_inner, int edge, struct rect * box) {
    int outer;
    int inner;
    int outer_cutpoint_ccount=0;
    struct coord * outer_cutpoints = NULL;
    //fprintf(stderr,"going to slice %d loops %d holes\n", p_in_outer->count, p_in_inner->count);
    for(outer=0; outer < p_in_outer->count; outer ++) {
        int i;
        /* get some space */
        int cutpoint_ccount =0;
        /* hold the cut points */
        struct coord * cutpoints=NULL;;

        /* cut this loop into pieces */
        itembin_slice_part_direction(p_in_outer->coord[outer], p_in_outer->ccount[outer], p_out_outer, edge,
                                     box, &cutpoints, &cutpoint_ccount);
        if(cutpoint_ccount > 0) {
            outer_cutpoints=g_realloc(outer_cutpoints, (outer_cutpoint_ccount + cutpoint_ccount) * sizeof(struct coord));
            for(i=0; i < cutpoint_ccount; i ++) {
                outer_cutpoints[outer_cutpoint_ccount + i] = cutpoints[i];
            }
            outer_cutpoint_ccount += cutpoint_ccount;
        }
        /* clear the cutpoints if we got any */
        if(cutpoints != NULL)
            g_free(cutpoints);

    }
    for(inner=0; inner < p_in_inner->count; inner ++) {
        int i;
        /* get some space */
        int cutpoint_ccount =0;
        /* hold the cut points */
        struct coord * cutpoints =NULL;
        struct slice_result temp;
        memset(&temp, 0, sizeof(temp));

        /* cut this loop into pieces */
        itembin_slice_part_direction(p_in_inner->coord[inner], p_in_inner->ccount[inner], &temp, edge,
                                     box, &cutpoints, &cutpoint_ccount);

        /* if we have cutlines, we need to add the remains of this loop to outer loop */
        if(cutpoint_ccount > 0) {
            int a;
            outer_cutpoints=g_realloc(outer_cutpoints, (outer_cutpoint_ccount + cutpoint_ccount) * sizeof(struct coord));
            for(i=0; i < cutpoint_ccount; i ++) {
                outer_cutpoints[outer_cutpoint_ccount + i] = cutpoints[i];
            }
            outer_cutpoint_ccount += cutpoint_ccount;
            /* move the parts to the outer loop */
            for(a = 0; a < temp.count; a ++) {
                itembin_slice_add_part(p_out_outer, temp.coord[a], temp.ccount[a]);
                /* unlink from tem not to get freed later by clear_slice_result */
                temp.coord[a] = NULL;
            }
        } else {
            int a;
            /* move the parts to the inner loop */
            for(a = 0; a < temp.count; a ++) {
                itembin_slice_add_part(p_out_inner, temp.coord[a], temp.ccount[a]);
                /* unlink from tem not to get freed later by clear_slice_result */
                temp.coord[a] = NULL;
            }
        }
        /* clear temp */
        clear_slice_result(&temp);
        /* clear the cutpoints if we got any */
        if(cutpoints != NULL)
            g_free(cutpoints);
    }

    /* calculate and add the outer cut lines (there are no inner) */
    if(outer_cutpoint_ccount >0) {
        int i;
        struct coord * part;
        /* sort the cut points */
        //dump_coords(outer_cutpoints, outer_cutpoint_ccount, stderr);
        itembin_sort_coordinates(outer_cutpoints, outer_cutpoint_ccount, edge);
        //dump_coords(outer_cutpoints, outer_cutpoint_ccount, stderr);
        /* add the cut lines */
        for (i = 0; i < outer_cutpoint_ccount; i+=2) {
            part = g_malloc(sizeof(struct coord) * 4);
            part[0] = outer_cutpoints[i];
            part[1] = outer_cutpoints[i];
            part[2] = outer_cutpoints[i+1];
            part[3] = outer_cutpoints[i+1];
            itembin_slice_add_part(p_out_outer, part, 4);
            /* do NOT free this part as it has been added */
        }
    }

    if(outer_cutpoints != NULL)
        g_free(outer_cutpoints);
    /* reassemble the parts to loops */
    //fprintf(stderr, "reassemble %d outer parts\n", p_out_outer->count);
    itembin_loop_combiner (p_out_outer);
    //fprintf(stderr, "reassemble %d inner parts\n", p_out_inner->count);
    itembin_loop_combiner (p_out_inner);
    //fprintf(stderr, "slicing resulted in %d parts outer, %d parts inner\n", p_out_outer->count, p_out_inner->count);
}


static void itembin_write_slice_result (struct slicerpolygon * sp, struct slice_result * outer,
                                        struct slice_result * inner) {
    int i;
    int hole_size = 0;
    /* calculate maximum space for loops */
    for(i=0; i < inner->count; i ++) {
        /* add space for the coordnates */
        hole_size += inner->ccount[i] * sizeof(struct coord);
        /* add space for attr overhead */
        hole_size += sizeof(int) * 8;
    }
    for(i=0; i < outer->count; i ++) {
        int h;
        struct item_bin * out;
        char  buffer[50];
        snprintf(buffer, 50, "slice %d", sp->number);
        out = g_malloc(sizeof(struct item_bin) + (outer->ccount[i] * sizeof(struct coord)) + sp->f_attr_len + 50 + hole_size);
        out->type = sp->ib->type;
        out->clen = outer->ccount[i] * 2;
        memcpy(out +1, outer->coord[i], outer->ccount[i] * sizeof(struct coord));
        memcpy((struct coord *)(out +1) + outer->ccount[i], sp->f_attrs, sp->f_attr_len);
        out->len = ((sizeof(struct item_bin) + sp->f_attr_len) / 4) -1 + out->clen;
        item_bin_add_attr_string(out, attr_debug, buffer);
        /* add holes */
        for(h=0; h < inner->count; h ++) {
            if(itembin_poly_is_in(inner->coord[h], outer->coord[i], outer->ccount[i]))
                item_bin_add_hole(out, inner->coord[h], inner->ccount[h]);
        }
        tile_write_item_to_tile(sp->info, out, sp->reference, sp->buffer);
        g_free(out);
    }
}

static void itembin_slice(struct slicerpolygon * sp, struct rect *box) {
    int i;
    struct slice_result inner;
    struct slice_result outer;
    struct slice_result out_inner;
    struct slice_result out_outer;
    struct slice_result * p_in_inner = &inner;
    struct slice_result * p_out_inner = &out_inner;
    struct slice_result * p_in_outer = &outer;
    struct slice_result * p_out_outer = &out_outer;
    memset(&outer, 0, sizeof(outer));
    memset(&inner, 0, sizeof(inner));
    memset(&out_outer, 0, sizeof(out_outer));
    memset(&out_inner, 0, sizeof(out_inner));

    /*prepare in for first round */
    /* outer */
    outer.count=1;
    outer.ccount=g_malloc(sizeof(int) * outer.count);
    outer.coord=g_malloc(sizeof(struct coord*) * outer.count);
    outer.coord[0]=g_malloc(sizeof(struct coord) * sp->count);
    memcpy(outer.coord[0], sp->poly, sizeof(struct coord) * sp->count);
    outer.ccount[0] = sp->count;
    /* holes or inner if matching box*/
    inner.count=0;
    inner.ccount=g_malloc(sizeof(int) * sp->hole_count);
    inner.coord=g_malloc(sizeof(struct coord*) * sp->hole_count);
    for(i=0; i < sp->hole_count; i ++) {
        if(itembin_bbox_intersects (&sp->holes_bbox[i], box)) {
            //fprintf(stderr, "Got intersecting hole %d %d coords\n",i,sp->ccount[i]);
            inner.coord[inner.count]=g_malloc(sizeof(struct coord) * sp->ccount[i]);
            memcpy(inner.coord[inner.count], sp->holes[i], sizeof(struct coord) * sp->ccount[i]);
            inner.ccount[inner.count] = sp->ccount[i];
            inner.count ++;
        }
    }
    //fprintf(stderr,"%d holes intersecting this tile\n", inner.count);

    /* cut for all 4 directions */
    for(i =0; i < 4; i ++) {
        itembin_slice_direction(p_in_outer,p_in_inner, p_out_outer, p_out_inner, i, box);
        /* clear old input */
        clear_slice_result(p_in_outer);
        clear_slice_result(p_in_inner);
        /* switch buffer */
        if(p_in_outer == &outer) {
            p_out_outer = &outer;
            p_in_outer = &out_outer;
            p_out_inner = &inner;
            p_in_inner = &out_inner;
        } else {
            p_out_outer = &out_outer;
            p_in_outer = &outer;
            p_out_inner = &out_inner;
            p_in_inner = &inner;
        }
    }

    /* write the result to tiles */
    itembin_write_slice_result (sp, &outer, &inner);

    /* clean up */
    clear_slice_result(&inner);
    clear_slice_result(&outer);

}


/**
 * @brief copy item_bin attrs filtering out one kind
 *
 * @param[in] in - attrs to filter
 * @param[in] size - attrs total size in bytes
 * @param[out] out - space for filtered attr. At least size long.
 * @param[in] remove - type of item to remove from attr
 */
static int itembin_filter_attr(struct attr_bin * in, int size, struct attr_bin * out, enum attr_type remove) {
    int nout=0;
    char * in_pos = (char *) in;
    char * out_pos = (char *) out;
    while(in_pos < ((char*)in) + size ) {
        struct attr_bin * reader=(struct attr_bin *) in_pos;
        if(reader-> type != remove) {
            /* write it */
            struct attr_bin * writer = (struct attr_bin *) out_pos;
            memcpy(writer, reader, (reader->len+1) *4);
            /* calculate new writer position */
            nout +=(reader->len+1) *4;
            out_pos +=(reader->len+1) *4;
        }
        /* calculate new posotion */
        in_pos +=(reader->len+1) *4;
    }
    return nout;
}

/**
 * @brief free structure filled by itembin_disassemble
 * @param sp structure to free
 */
static void itembin_slicerpolygon_free(struct slicerpolygon * sp) {
    if(sp->ccount != NULL)
        g_free(sp->ccount);
    if(sp->holes != NULL)
        g_free(sp->holes);
    if(sp->holes_bbox != NULL)
        g_free(sp->holes_bbox);
    if(sp->f_attrs != NULL)
        g_free(sp->f_attrs);
}

/**
 * @brief parse an item_bin and make it's contents accessable.
 *
 * This method parses the contents of a item_bin and makes it accessable via direct
 * pointers. Additionally it filters out holes if any from attr. This allows easy
 * reassembly of the sliced parts to item_bins.
 * @param[in] ib item to disassemble
 * @param{out] out structure containing the parsing result.
 */
static void itembin_disassemble (struct item_bin *ib, struct slicerpolygon * out) {
    struct attr_bin *ab=NULL;
    /* basic setup*/
    out->ib = ib;
    out->count = ib->clen / 2;
    out->poly = (struct coord *)(ib +1);
    bbox(out->poly, out->count, &out->bbox);
    /* init hole storage */
    out->hole_count=0;
    out->ccount = NULL;
    out->holes = NULL;
    out->holes_bbox = NULL;
    /* scan for holes and extract pointers to coordinates */
    while ((ab=item_bin_get_attr_bin(ib, attr_poly_hole, ab))!=NULL) {
        /* got one hole */
        /* realloc ccount and holes and holes_bbox*/
        out->ccount = g_realloc(out->ccount, sizeof(int) * (out->hole_count +1));
        out->holes = g_realloc(out->holes, sizeof(struct coord *) * (out->hole_count +1));
        out->holes_bbox = g_realloc(out->holes_bbox, sizeof(struct rect) * (out->hole_count +1));
        /* get ccount */
        out->ccount[out->hole_count] = *(int*)(ab + 1);
        /* get hole coords */
        out->holes[out->hole_count] = (struct coord *)((int *)(ab + 1) + 1);
        /* get hole bbox */
        bbox(out->holes[out->hole_count], out->ccount[out->hole_count], &(out->holes_bbox[out->hole_count]));
        /* count hole */
        out->hole_count ++;
        /* next attr */
        ab ++;
    }
    /* get attr pointer and size */
    out->attrs = (struct attr_bin *) (out->poly + out->count);
    out->attr_len = (ib->len - ib->clen +1) * 4 - sizeof(struct item_bin);

    /* get memory for filtered attrs */
    out->f_attrs=g_malloc(out->attr_len);
    /* filter_attrs */
    out->f_attr_len=itembin_filter_attr(out->attrs, out->attr_len, out->f_attrs, attr_poly_hole);

    //fprintf(stderr,"Got %d coords and %d holes. %d bytes attrs, %d without holes\n", out->count, out->hole_count,
    //        out->attr_len, out->f_attr_len);
}

void itembin_nicer_slicer(struct tile_info *info, struct item_bin *ib, FILE *reference, char * buffer, int min) {
    char tilecode[22];
    char tileend[22];
    struct slicerpolygon sp;
    long long * id;
    int is_relation=0;

    /* for now only slice polygons and things > min. */
    if (ib->type < type_area || min <= tile_len(buffer)) {
        tile_write_item_to_tile(info, ib, reference, buffer);
        return;
    }

    id = item_bin_get_attr(ib, attr_osm_wayid, NULL);
    if(id!= NULL) {
        is_relation = 0;
    } else {
        id = item_bin_get_attr(ib, attr_osm_relationid, NULL);
        is_relation =1;
    }

    /* seems we have a polygon. Calculate tile range from*/
    memset(tilecode, 0, sizeof(tilecode));
    memset(tilecode,'a', min);
    memcpy(tilecode, buffer, tile_len(buffer));
    /* to */
    memset(tileend, 0, sizeof(tileend));
    memset(tileend,'d', min);
    memcpy(tileend, buffer, tile_len(buffer));
    next_tile(tileend);

    if(id != NULL) {
        if(is_relation)
            osm_info("relation",*id,0,"slice down %d steps from %s to (%s - %s)\n",min - tile_len(buffer), buffer, tilecode,
                     tileend);
        else
            osm_info("way",*id,0,"slice down %d steps from %s to (%s - %s)\n",min - tile_len(buffer), buffer, tilecode, tileend);

    }
    itembin_disassemble (ib, &sp);
    sp.reference = reference;
    sp.info = info;
    sp.buffer = buffer;
    sp.number=0;

    /* for all tiles in range. Allow this to overflow if tile_len(buffer) == 0*/
    do {
        struct rect bbox;
        /* get tile rectangle. One might like slicing without overlap, but since the
         * overlapping tiles do not cover the same area per tile code than the
         * overlapping ones we cannot. This will look ugly. But there is no chance.*/
        tile_bbox(tilecode, &bbox, overlap);

        /* only process tiles which do intersect wit ib's bbox */
        if(!itembin_bbox_intersects (&sp.bbox, &bbox)) {
            next_tile(tilecode);
            sp.number ++;
            continue;
        }
        fprintf(stderr, "slice %d intersection with %s\n", sp.number, tilecode);
        sp.buffer=tilecode;
        itembin_slice(&sp, &bbox);

        /* next tile */
        next_tile(tilecode);
        sp.number ++;
    } while (strcmp(tilecode,tileend) != 0);
    itembin_slicerpolygon_free(&sp);
}

