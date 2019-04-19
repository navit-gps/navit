/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2011 Navit Team
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
#include "maptool.h"
#include "debug.h"

struct coastline_tile {
    osmid wayid;
    int edges;
};

static int distance_from_ll(struct coord *c, struct rect *bbox) {
    int dist=0;
    if (c->x == bbox->l.x)
        return dist+c->y-bbox->l.y;
    dist+=bbox->h.y-bbox->l.y;
    if (c->y == bbox->h.y)
        return dist+c->x-bbox->l.x;
    dist+=bbox->h.x-bbox->l.x;
    if (c->x == bbox->h.x)
        return dist+bbox->h.y-c->y;
    dist+=bbox->h.y-bbox->l.y;
    if (c->y == bbox->l.y)
        return dist+bbox->h.x-c->x;
    return -1;
}

static struct geom_poly_segment *find_next(struct rect *bbox, GList *segments, struct coord *c, int exclude,
        struct coord *ci) {
    int min=INT_MAX,search=distance_from_ll(c, bbox)+(exclude?1:0);
    GList *curr;
    int i;
    struct geom_poly_segment *ret=NULL;

    for (i = 0 ; i < 2 ; i++) {
        curr=segments;
        dbg(lvl_debug,"search distance %d",search);
        while (curr) {
            struct geom_poly_segment *seg=curr->data;
            int dist=distance_from_ll(seg->first, bbox);
            dbg(lvl_debug,"0x%x 0x%x dist %d",seg->first->x,seg->first->y,dist);
            if (dist != -1 && seg->first != seg->last && dist < min && (dist >= search)) {
                min=dist;
                ci[0]=*seg->first;
                ci[1]=*seg->last;
                ret=seg;
            }
            curr=g_list_next(curr);
        }
        if (ret || !search)
            break;
        search=0;
    }
    return ret;
}

static void close_polygon(struct item_bin *ib, struct coord *from, struct coord *to, int dir, struct rect *bbox,
                          int *edges) {
    int i,e,dist,fromdist,todist;
    int full=(bbox->h.x-bbox->l.x+bbox->h.y-bbox->l.y)*2;
    int corners=0,first_corner=0;
    struct coord c;
    if (dir > 0) {
        fromdist=distance_from_ll(from, bbox);
        todist=distance_from_ll(to, bbox);
    } else {
        fromdist=distance_from_ll(to, bbox);
        todist=distance_from_ll(from, bbox);
    }
    if (fromdist > todist)
        todist+=full;
    for (i = 0 ; i < 8 ; i++) {
        if (dir > 0)
            e=i;
        else
            e=7-i;
        switch (e%4) {
        case 0:
            c=bbox->l;
            break;
        case 1:
            c.x=bbox->l.x;
            c.y=bbox->h.y;
            break;
        case 2:
            c=bbox->h;
            break;
        case 3:
            c.x=bbox->h.x;
            c.y=bbox->l.y;
            break;
        }
        dist=distance_from_ll(&c, bbox);
        if (e & 4)
            dist+=full;
        if (dist > fromdist && dist < todist) {
            item_bin_add_coord(ib, &c, 1);
        }
        if (dist >= fromdist && dist <= todist) {
            if (!corners)
                first_corner=e;
            corners++;
        }
    }
    while (corners >= 2) {
        *edges |= 1<<(first_corner%4);
        first_corner++;
        corners--;
    }
}

struct coastline_tile_data {
    struct item_bin_sink_func *sink;
    GHashTable *tile_edges;
    int level;
    GList *k,*v;
};

static GList *tile_data_to_segments(int *tile_data) {
    int *end=tile_data+tile_data[0];
    int *curr=tile_data+1;
    GList *segments=NULL;
    int count=0;

    while (curr < end) {
        struct item_bin *ib=(struct item_bin *)curr;
        segments=g_list_prepend(segments,item_bin_to_poly_segment(ib, geom_poly_segment_type_way_right_side));
        curr+=ib->len+1;
        count++;
    }
    return segments;
}

static void tile_collector_process_tile(char *tile, int *tile_data, struct coastline_tile_data *data) {
    int poly_start_valid,tile_start_valid,exclude,search=0;
    struct rect bbox;
    struct coord cn[2],end,poly_start,tile_start;
    struct geom_poly_segment *first;
    struct item_bin *ib=NULL;
    struct item_bin_sink *out=data->sink->priv_data[1];
    int edges=0,flags;
    GList *sorted_segments,*curr;
    struct item_bin *ibt=(struct item_bin *)(tile_data+1);
    struct coastline_tile *ct=g_new0(struct coastline_tile, 1);
    ct->wayid=item_bin_get_wayid(ibt);
    tile_bbox(tile, &bbox, 0);
    curr=tile_data_to_segments(tile_data);
    sorted_segments=geom_poly_segments_sort(curr, geom_poly_segment_type_way_right_side);
    g_list_foreach(curr,(GFunc)geom_poly_segment_destroy,NULL);
    g_list_free(curr);

    flags=0;
    curr=sorted_segments;
    while (curr) {
        struct geom_poly_segment *seg=curr->data;
        switch (seg->type) {
        case geom_poly_segment_type_way_inner:
            flags|=1;
            break;
        case geom_poly_segment_type_way_outer:
            flags|=2;
            break;
        default:
            flags|=4;
            break;
        }
        curr=g_list_next(curr);
    }
    if (flags == 1) {
        ct->edges=15;
        ib=init_item(type_poly_water_tiled);
        item_bin_bbox(ib, &bbox);
        item_bin_add_attr_longlong(ib, attr_osm_wayid, ct->wayid);
        item_bin_write_to_sink(ib, out, NULL);
        g_hash_table_insert(data->tile_edges, g_strdup(tile), ct);
        return;
    }
    end=bbox.l;
    tile_start_valid=0;
    poly_start_valid=0;
    exclude=0;
    poly_start.x=0;
    poly_start.y=0;
    tile_start.x=0;
    tile_start.y=0;
    for (;;) {
        search++;
        // item_bin_write_debug_point_to_sink(out, &end, "Search %d",search);
        dbg(lvl_debug,"searching next polygon from 0x%x 0x%x",end.x,end.y);
        first=find_next(&bbox, sorted_segments, &end, exclude, cn);
        exclude=1;
        if (!first)
            break;
        if (!tile_start_valid) {
            tile_start=cn[0];
            tile_start_valid=1;
        } else {
            if (cn[0].x == tile_start.x && cn[0].y == tile_start.y) {
                dbg(lvl_debug,"end of tile reached");
                break;
            }
        }
        if (first->type == geom_poly_segment_type_none) {
            end=cn[0];
            continue;
        }
        poly_start_valid=0;
        dbg(lvl_debug,"start of polygon 0x%x 0x%x",cn[0].x,cn[0].y);
        for (;;) {
            if (!poly_start_valid) {
                poly_start=cn[0];
                poly_start_valid=1;
                ib=init_item(type_poly_water_tiled);
            } else {
                close_polygon(ib, &end, &cn[0], 1, &bbox, &edges);
                if (cn[0].x == poly_start.x && cn[0].y == poly_start.y) {
                    dbg(lvl_debug,"poly end reached");
                    item_bin_add_attr_longlong(ib, attr_osm_wayid, ct->wayid);
                    item_bin_write_to_sink(ib, out, NULL);
                    end=cn[0];
                    break;
                }
            }
            if (first->type == geom_poly_segment_type_none)
                break;
            item_bin_add_coord(ib, first->first, first->last-first->first+1);
            first->type=geom_poly_segment_type_none;
            end=cn[1];
            if (distance_from_ll(&end, &bbox) == -1) {
                dbg(lvl_debug,"incomplete");
                break;
            }
            first=find_next(&bbox, sorted_segments, &end, 1, cn);
            dbg(lvl_debug,"next segment of polygon 0x%x 0x%x",cn[0].x,cn[0].y);
        }
        if (search > 55)
            break;
    }
    g_list_foreach(sorted_segments,(GFunc)geom_poly_segment_destroy,NULL);
    g_list_free(sorted_segments);

    ct->edges=edges;
    g_hash_table_insert(data->tile_edges, g_strdup(tile), ct);
}

static void ocean_tile(GHashTable *hash, char *tile, char c, osmid wayid, struct item_bin_sink *out) {
    int len=strlen(tile);
    char *tile2=g_alloca(sizeof(char)*(len+1));
    struct rect bbox;
    struct item_bin *ib;
    struct coastline_tile *ct;

    strcpy(tile2, tile);
    tile2[len-1]=c;
    //fprintf(stderr,"Testing %s\n",tile2);
    ct=g_hash_table_lookup(hash, tile2);
    if (ct)
        return;
    //fprintf(stderr,"%s ok\n",tile2);
    tile_bbox(tile2, &bbox, 0);
    ib=init_item(type_poly_water_tiled);
    item_bin_bbox(ib, &bbox);
    item_bin_add_attr_longlong(ib, attr_osm_wayid, wayid);
    item_bin_write_to_sink(ib, out, NULL);
    ct=g_new0(struct coastline_tile, 1);
    ct->edges=15;
    ct->wayid=wayid;
    g_hash_table_insert(hash, g_strdup(tile2), ct);
}

/* ba */
/* dc */

static void tile_collector_add_siblings(char *tile, struct coastline_tile *ct, struct coastline_tile_data *data) {
    int len=strlen(tile);
    char t=tile[len-1];
    struct item_bin_sink *out=data->sink->priv_data[1];
    int edges=ct->edges;
    int debug=0;
    if (debug)
        fprintf(stderr,"%s (%c) has %d edges active\n",tile,t,edges);
    if (t == 'a' && (edges & 1))
        ocean_tile(data->tile_edges, tile, 'b', ct->wayid, out);
    if (t == 'a' && (edges & 8))
        ocean_tile(data->tile_edges, tile, 'c', ct->wayid, out);
    if (t == 'b' && (edges & 4))
        ocean_tile(data->tile_edges, tile, 'a', ct->wayid, out);
    if (t == 'b' && (edges & 8))
        ocean_tile(data->tile_edges, tile, 'd', ct->wayid, out);
    if (t == 'c' && (edges & 1))
        ocean_tile(data->tile_edges, tile, 'd', ct->wayid, out);
    if (t == 'c' && (edges & 2))
        ocean_tile(data->tile_edges, tile, 'a', ct->wayid, out);
    if (t == 'd' && (edges & 4))
        ocean_tile(data->tile_edges, tile, 'c', ct->wayid, out);
    if (t == 'd' && (edges & 2))
        ocean_tile(data->tile_edges, tile, 'b', ct->wayid, out);
}

static int tile_sibling_edges(GHashTable *hash, char *tile, char c) {
    int len=strlen(tile);
    char *tile2=g_alloca(sizeof(char)*(len+1));
    struct coastline_tile *ct;
    strcpy(tile2, tile);
    tile2[len-1]=c;
    ct=g_hash_table_lookup(hash, tile2);
    if (ct)
        return ct->edges;
    return 15;
}

#if 0
static void ocean_tile2(struct rect *r, int dx, int dy, int wf, int hf, struct item_bin_sink *out) {
    struct item_bin *ib;
    int w=r->h.x-r->l.x;
    int h=r->h.y-r->l.y;
    char tile2[32];
    struct rect bbox;
    bbox.l.x=r->l.x+dx*w;
    bbox.l.y=r->l.y+dy*h;
    bbox.h.x=bbox.l.x+w*wf;
    bbox.h.y=bbox.l.y+h*hf;
    //fprintf(stderr,"0x%x,0x%x-0x%x,0x%x -> 0x%x,0x%x-0x%x,0x%x\n",r->l.x,r->l.y,r->h.x,r->h.y,bbox.l.x,bbox.l.y,bbox.h.x,bbox.h.y);
    ib=init_item(type_poly_water_tiled);
    item_bin_bbox(ib, &bbox);
    item_bin_write_to_sink(ib, out, NULL);
#if 0
    item_bin_init(ib, type_border_country);
    item_bin_bbox(ib, &bbox);
    item_bin_add_attr_string(ib, attr_debug, tile2);
    item_bin_write_to_sink(ib, out, NULL);
#endif
    tile(&bbox, NULL, tile2, 32, 0, NULL);
#if 0
    {
        struct coord co;
        co.x=(bbox.l.x+bbox.h.x)/2;
        co.y=(bbox.l.y+bbox.h.y)/2;
        item_bin_write_debug_point_to_sink(out, &co, "%s 15",tile2);
    }
#endif
}
#endif

static void tile_collector_add_siblings2(char *tile, struct coastline_tile *ct, struct coastline_tile_data *data) {
    int edges=ct->edges;
    int pedges=0;
    int debug=0;
    int len=strlen(tile);
    struct coastline_tile *cn, *co;
    char *tile2=g_alloca(sizeof(char)*(len+1));
    char t=tile[len-1];
    strcpy(tile2, tile);
    tile2[len-1]='\0';
    if (debug)
        fprintf(stderr,"len of %s %d vs %d\n",tile,len,data->level);


    if (debug)
        fprintf(stderr,"checking siblings of '%s' with %d edges active\n",tile,edges);
    if (t == 'b' && (edges & 1) && (tile_sibling_edges(data->tile_edges, tile, 'd') & 1))
        pedges|=1;
    if (t == 'd' && (edges & 2) && (tile_sibling_edges(data->tile_edges, tile, 'b') & 1))
        pedges|=1;
    if (t == 'a' && (edges & 2) && (tile_sibling_edges(data->tile_edges, tile, 'b') & 2))
        pedges|=2;
    if (t == 'b' && (edges & 2) && (tile_sibling_edges(data->tile_edges, tile, 'a') & 2))
        pedges|=2;
    if (t == 'a' && (edges & 4) && (tile_sibling_edges(data->tile_edges, tile, 'c') & 4))
        pedges|=4;
    if (t == 'c' && (edges & 4) && (tile_sibling_edges(data->tile_edges, tile, 'a') & 4))
        pedges|=4;
    if (t == 'd' && (edges & 8) && (tile_sibling_edges(data->tile_edges, tile, 'c') & 8))
        pedges|=8;
    if (t == 'c' && (edges & 8) && (tile_sibling_edges(data->tile_edges, tile, 'd') & 8))
        pedges|=8;
    co=g_hash_table_lookup(data->tile_edges, tile2);
    if (debug)
        fprintf(stderr,"result '%s' %d old %d\n",tile2,pedges,co?co->edges:0);
    cn=g_new0(struct coastline_tile, 1);
    cn->edges=pedges;
    if (co) {
        cn->edges|=co->edges;
        cn->wayid=co->wayid;
    } else
        cn->wayid=ct->wayid;
    g_hash_table_insert(data->tile_edges, g_strdup(tile2), cn);
}

static void foreach_tile_func(gpointer key, gpointer value, gpointer user_data) {
    struct coastline_tile_data *data=user_data;
    if (strlen((char *)key) == data->level) {
        data->k=g_list_prepend(data->k, key);
        data->v=g_list_prepend(data->v, value);
    }
}

static void foreach_tile(struct coastline_tile_data *data, void(*func)(char *, struct coastline_tile *,
                         struct coastline_tile_data *)) {
    GList *k,*v;
    data->k=NULL;
    data->v=NULL;

    g_hash_table_foreach(data->tile_edges, foreach_tile_func, data);
    k=data->k;
    v=data->v;
    while (k) {
        func(k->data,v->data,data);
        k=g_list_next(k);
        v=g_list_next(v);
    }
    g_list_free(data->k);
    g_list_free(data->v);
}

static int tile_collector_finish(struct item_bin_sink_func *tile_collector) {
    struct coastline_tile_data data;
    int i;
    GHashTable *hash;
    data.sink=tile_collector;
    data.tile_edges=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    hash=tile_collector->priv_data[0];
    fprintf(stderr,"tile_collector_finish\n");
#if 1
    g_hash_table_foreach(hash, (GHFunc) tile_collector_process_tile, &data);
#endif
    fprintf(stderr,"tile_collector_finish foreach done\n");
    g_hash_table_destroy(hash);
    fprintf(stderr,"tile_collector_finish destroy done\n");
    for (i = 14 ; i > 0 ; i--) {
        fprintf(stderr,"Level=%d\n",i);
        data.level=i;
        foreach_tile(&data, tile_collector_add_siblings);
        fprintf(stderr,"*");
        foreach_tile(&data, tile_collector_add_siblings);
        fprintf(stderr,"*");
        foreach_tile(&data, tile_collector_add_siblings);
        fprintf(stderr,"*");
        foreach_tile(&data, tile_collector_add_siblings);
        fprintf(stderr,"*");
        foreach_tile(&data, tile_collector_add_siblings2);
        fprintf(stderr,"*\n");
        foreach_tile(&data, tile_collector_add_siblings2);
        fprintf(stderr,"*\n");
        foreach_tile(&data, tile_collector_add_siblings2);
        fprintf(stderr,"*\n");
        foreach_tile(&data, tile_collector_add_siblings2);
        fprintf(stderr,"*\n");
    }
    item_bin_sink_func_destroy(tile_collector);
    fprintf(stderr,"tile_collector_finish done\n");
    return 0;
}


static int coastline_processor_process(struct item_bin_sink_func *func, struct item_bin *ib,
                                       struct tile_data *tile_data) {
    item_bin_write_clipped(ib, func->priv_data[0], func->priv_data[1]);
    return 0;
}

static struct item_bin_sink_func *coastline_processor_new(struct item_bin_sink *out) {
    struct item_bin_sink_func *coastline_processor=item_bin_sink_func_new(coastline_processor_process);
    struct item_bin_sink *tiles=item_bin_sink_new();
    struct item_bin_sink_func *tile_collector=tile_collector_new(out);
    struct tile_parameter *param=g_new0(struct tile_parameter, 1);

    param->min=14;
    param->max=14;
    param->overlap=0;
    param->attr_to_copy=attr_osm_wayid;

    item_bin_sink_add_func(tiles, tile_collector);
    coastline_processor->priv_data[0]=param;
    coastline_processor->priv_data[1]=tiles;
    coastline_processor->priv_data[2]=tile_collector;
    return coastline_processor;
}

static void coastline_processor_finish(struct item_bin_sink_func *coastline_processor) {
    struct tile_parameter *param=coastline_processor->priv_data[0];
    struct item_bin_sink *tiles=coastline_processor->priv_data[1];
    struct item_bin_sink_func *tile_collector=coastline_processor->priv_data[2];
    g_free(param);
    tile_collector_finish(tile_collector);
    item_bin_sink_destroy(tiles);
    item_bin_sink_func_destroy(coastline_processor);
}

void process_coastlines(FILE *in, FILE *out) {
    struct item_bin_sink *reader=file_reader_new(in,-1,0);
    struct item_bin_sink_func *file_writer=file_writer_new(out);
    struct item_bin_sink *result=item_bin_sink_new();
    struct item_bin_sink_func *coastline_processor=coastline_processor_new(result);
    item_bin_sink_add_func(reader, coastline_processor);
    item_bin_sink_add_func(result, file_writer);
    file_reader_finish(reader);
    coastline_processor_finish(coastline_processor);
    file_writer_finish(file_writer);
    item_bin_sink_destroy(result);
}
