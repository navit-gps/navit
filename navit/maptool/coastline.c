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

static int distance_from_ll(struct coord *c, struct rect *bbox)
{
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

static struct geom_poly_segment *
find_next(struct rect *bbox, GList *segments, struct coord *c, int exclude, struct coord *ci)
{
	int min=INT_MAX,search=distance_from_ll(c, bbox)+(exclude?1:0);
	GList *curr;
	int i;
	struct geom_poly_segment *ret=NULL;
	int dbgl=1;

	for (i = 0 ; i < 2 ; i++) {
		curr=segments;
		dbg(dbgl,"search distance %d\n",search);
		while (curr) {
			struct geom_poly_segment *seg=curr->data;
			int dist=distance_from_ll(seg->first, bbox);
			dbg(dbgl,"0x%x 0x%x dist %d\n",seg->first->x,seg->first->y,dist);
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

static void
close_polygon(struct item_bin *ib, struct coord *from, struct coord *to, int dir, struct rect *bbox, int *edges)
{
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
#if 0
	fprintf(stderr,"close_polygon fromdist %d todist %d full %d dir %d\n", fromdist, todist, full, dir);
#endif
	if (fromdist > todist)
		todist+=full;
#if 0
	fprintf(stderr,"close_polygon corrected fromdist %d todist %d full %d dir %d\n", fromdist, todist, full, dir);
#endif
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
#if 0
		fprintf(stderr,"dist %d %d\n",e,dist);
#endif
		if (dist > fromdist && dist < todist) {
			item_bin_add_coord(ib, &c, 1);
#if 0
			fprintf(stderr,"add\n");
#endif
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
};

static GList *
tile_data_to_segments(int *tile_data)
{
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
#if 0
	fprintf(stderr,"%d segments\n",count);
#endif
	return segments;
}

static void
tile_collector_process_tile(char *tile, int *tile_data, struct coastline_tile_data *data)
{
	int poly_start_valid,tile_start_valid,exclude,search=0;
	struct rect bbox;
	struct coord cn[2],end,poly_start,tile_start;
	struct geom_poly_segment *first;
	struct item_bin *ib=NULL;
	struct item_bin_sink *out=data->sink->priv_data[1];
	int dbgl=1;
	int edges=0,flags;
	GList *sorted_segments,*curr;
#if 0
	if (strncmp(tile,"bcdbdcabddddba",7))
		return;
#endif
#if 0
	if (strncmp(tile,"bcdbdcaaaaddba",14))
		return;
#endif
#if 0
	fprintf(stderr,"tile %s of size %d\n", tile, *tile_data);
#endif
	tile_bbox(tile, &bbox, 0);
	sorted_segments=geom_poly_segments_sort(tile_data_to_segments(tile_data), geom_poly_segment_type_way_right_side);
#if 0
{
	GList *sort_segments=sorted_segments;
	int count=0;
	while (sort_segments) {
		struct geom_poly_segment *seg=sort_segments->data;
		struct item_bin *ib=(struct item_bin *)buffer;
		char *text=g_strdup_printf("segment %d type %d %p %s area "LONGLONG_FMT,count++,seg->type,sort_segments,coord_is_equal(*seg->first, *seg->last) ? "closed":"open",geom_poly_area(seg->first,seg->last-seg->first+1));
		item_bin_init(ib, type_rg_segment);
		item_bin_add_coord(ib, seg->first, seg->last-seg->first+1);
		item_bin_add_attr_string(ib, attr_debug, text);
		// fprintf(stderr,"%s\n",text);
		g_free(text);
		// item_bin_dump(ib, stderr);
		item_bin_write_to_sink(ib, out, NULL);
		sort_segments=g_list_next(sort_segments);
	}
}
#endif
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
		ib=init_item(type_poly_water_tiled);
		item_bin_bbox(ib, &bbox);
		item_bin_write_to_sink(ib, out, NULL);
		g_hash_table_insert(data->tile_edges, g_strdup(tile), (void *)15);
		return;
	}
#if 1
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
		dbg(dbgl,"searching next polygon from 0x%x 0x%x\n",end.x,end.y);
		first=find_next(&bbox, sorted_segments, &end, exclude, cn);
		exclude=1;
		if (!first)
			break;
		if (!tile_start_valid) {
			tile_start=cn[0];
			tile_start_valid=1;
		} else {
			if (cn[0].x == tile_start.x && cn[0].y == tile_start.y) {
				dbg(dbgl,"end of tile reached\n");
				break;
			}
		}
		if (first->type == geom_poly_segment_type_none) {
			end=cn[0];
			continue;
		}
		poly_start_valid=0;
		dbg(dbgl,"start of polygon 0x%x 0x%x\n",cn[0].x,cn[0].y);
		for (;;) {
			if (!poly_start_valid) {
				poly_start=cn[0];
				poly_start_valid=1;
				ib=init_item(type_poly_water_tiled);
			} else {
				close_polygon(ib, &end, &cn[0], 1, &bbox, &edges);
				if (cn[0].x == poly_start.x && cn[0].y == poly_start.y) {
					dbg(dbgl,"poly end reached\n");
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
				dbg(dbgl,"incomplete\n");
				break;
			}
			first=find_next(&bbox, sorted_segments, &end, 1, cn);
			dbg(dbgl,"next segment of polygon 0x%x 0x%x\n",cn[0].x,cn[0].y);
		}
		if (search > 55)
			break;
	}
#endif

#if 0
	{
		int *end=tile_data+tile_data[0];
		int *curr=tile_data+1;
		while (curr < end) {
			struct item_bin *ib=(struct item_bin *)curr;
			// item_bin_dump(ib);
			ib->type=type_rg_segment;
			item_bin_write_to_sink(ib, out, NULL);
			curr+=ib->len+1;
#if 0
			{
				struct coord *c[2];
				int i;
				char *s;
				c[0]=(struct coord *)(ib+1);
				c[1]=c[0]+ib->clen/2-1;
				for (i = 0 ; i < 2 ; i++) {
					s=coord_to_str(c[i]);
					item_bin_write_debug_point_to_sink(out, c[i], "%s",s);
					g_free(s);
				}
				
			}
#endif
		}
	}
#endif
	g_hash_table_insert(data->tile_edges, g_strdup(tile), (void *)edges);
#if 0
	item_bin_init(ib, type_border_country);
	item_bin_bbox(ib, &bbox);
	item_bin_add_attr_string(ib, attr_debug, tile);
	item_bin_write_to_sink(ib, out, NULL);
#endif
#if 0
	c.x=(bbox.l.x+bbox.h.x)/2;
	c.y=(bbox.l.y+bbox.h.y)/2;
	item_bin_write_debug_point_to_sink(out, &c, "%s %d",tile,edges);
#endif
}

static void
ocean_tile(GHashTable *hash, char *tile, char c, struct item_bin_sink *out)
{
	int len=strlen(tile);
	char *tile2=g_alloca(sizeof(char)*(len+1));
	struct rect bbox;
	struct item_bin *ib;
	struct coord co;

	strcpy(tile2, tile);
	tile2[len-1]=c;
	//fprintf(stderr,"Testing %s\n",tile2);
	if (g_hash_table_lookup_extended(hash, tile2, NULL, NULL))
		return;
	//fprintf(stderr,"%s ok\n",tile2);
	tile_bbox(tile2, &bbox, 0);
	ib=init_item(type_poly_water_tiled);
	item_bin_bbox(ib, &bbox);
	item_bin_write_to_sink(ib, out, NULL);
	g_hash_table_insert(hash, g_strdup(tile2), (void *)15);
#if 0
	item_bin_init(ib, type_border_country);
	item_bin_bbox(ib, &bbox);
	item_bin_add_attr_string(ib, attr_debug, tile2);
	item_bin_write_to_sink(ib, out, NULL);
#endif
	co.x=(bbox.l.x+bbox.h.x)/2;
	co.y=(bbox.l.y+bbox.h.y)/2;
	//item_bin_write_debug_point_to_sink(out, &co, "%s 15",tile2);
	
}

/* ba */
/* dc */

static void
tile_collector_add_siblings(char *tile, void *edgesp, struct coastline_tile_data *data)
{
	int len=strlen(tile);
	char t=tile[len-1];
	struct item_bin_sink *out=data->sink->priv_data[1];
	int edges=(int)edgesp;
	int debug=0;

	if (len != data->level)
		return;
#if 0
	if (!strncmp(tile,"bcacccaadbdcd",10))
		debug=1;
#endif
	if (debug)
		fprintf(stderr,"%s (%c) has %d edges active\n",tile,t,edges);
	if (t == 'a' && (edges & 1)) 
		ocean_tile(data->tile_edges, tile, 'b', out);
	if (t == 'a' && (edges & 8)) 
		ocean_tile(data->tile_edges, tile, 'c', out);
	if (t == 'b' && (edges & 4)) 
		ocean_tile(data->tile_edges, tile, 'a', out);
	if (t == 'b' && (edges & 8)) 
		ocean_tile(data->tile_edges, tile, 'd', out);
	if (t == 'c' && (edges & 1)) 
		ocean_tile(data->tile_edges, tile, 'd', out);
	if (t == 'c' && (edges & 2)) 
		ocean_tile(data->tile_edges, tile, 'a', out);
	if (t == 'd' && (edges & 4)) 
		ocean_tile(data->tile_edges, tile, 'c', out);
	if (t == 'd' && (edges & 2)) 
		ocean_tile(data->tile_edges, tile, 'b', out);
}

static int
tile_sibling_edges(GHashTable *hash, char *tile, char c)
{
	int len=strlen(tile);
	int ret;
	char *tile2=g_alloca(sizeof(char)*(len+1));
	void *data;
	strcpy(tile2, tile);
	tile2[len-1]=c;
	if (!g_hash_table_lookup_extended(hash, tile2, NULL, &data))
		ret=15;
	else
		ret=(int)data;
	//fprintf(stderr,"checking '%s' with %d edges active\n",tile2,ret);

	return ret;
}

static void
ocean_tile2(struct rect *r, int dx, int dy, int wf, int hf, struct item_bin_sink *out)
{
	struct item_bin *ib;
	int w=r->h.x-r->l.x;
	int h=r->h.y-r->l.y;
	char tile2[32];
	struct rect bbox;
	struct coord co;
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
	co.x=(bbox.l.x+bbox.h.x)/2;
	co.y=(bbox.l.y+bbox.h.y)/2;
	//item_bin_write_debug_point_to_sink(out, &co, "%s 15",tile2);
}

static void
tile_collector_add_siblings2(char *tile, void *edgesp, struct coastline_tile_data *data)
{
	int edges=(int)edgesp;
	int pedges=0;
	int debug=0;
	int len=strlen(tile);
	char *tile2=g_alloca(sizeof(char)*(len+1));
	char t=tile[len-1];
	strcpy(tile2, tile);
	tile2[len-1]='\0';
#if 0
	if (!strncmp(tile,"bcacccaadbdcd",10))
		debug=1;
#endif
	if (debug) 
		fprintf(stderr,"len of %s %d vs %d\n",tile,len,data->level);
	if (len != data->level)
		return;


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
	if (debug)
		fprintf(stderr,"result '%s' %d old %d\n",tile2,pedges,(int)g_hash_table_lookup(data->tile_edges, tile2));
	g_hash_table_insert(data->tile_edges, g_strdup(tile2), (void *)((int)g_hash_table_lookup(data->tile_edges, tile2)|pedges));
}

static int
tile_collector_finish(struct item_bin_sink_func *tile_collector)
{
	struct coastline_tile_data data;
	int i;
	GHashTable *hash;
	data.sink=tile_collector;
	data.tile_edges=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	hash=tile_collector->priv_data[0];
	fprintf(stderr,"tile_collector_finish\n");
	g_hash_table_foreach(hash, (GHFunc) tile_collector_process_tile, &data);
	fprintf(stderr,"tile_collector_finish foreach done\n");
	g_hash_table_destroy(hash);
	fprintf(stderr,"tile_collector_finish destroy done\n");
	for (i = 14 ; i > 0 ; i--) {
		fprintf(stderr,"Level=%d\n",i);
		data.level=i;
		g_hash_table_foreach(data.tile_edges, (GHFunc) tile_collector_add_siblings, &data);
		fprintf(stderr,"*");
		g_hash_table_foreach(data.tile_edges, (GHFunc) tile_collector_add_siblings, &data);
		fprintf(stderr,"*");
		g_hash_table_foreach(data.tile_edges, (GHFunc) tile_collector_add_siblings, &data);
		fprintf(stderr,"*");
		g_hash_table_foreach(data.tile_edges, (GHFunc) tile_collector_add_siblings, &data);
		fprintf(stderr,"*");
		g_hash_table_foreach(data.tile_edges, (GHFunc) tile_collector_add_siblings2, &data);
		fprintf(stderr,"*\n");
		g_hash_table_foreach(data.tile_edges, (GHFunc) tile_collector_add_siblings2, &data);
		fprintf(stderr,"*\n");
		g_hash_table_foreach(data.tile_edges, (GHFunc) tile_collector_add_siblings2, &data);
		fprintf(stderr,"*\n");
		g_hash_table_foreach(data.tile_edges, (GHFunc) tile_collector_add_siblings2, &data);
		fprintf(stderr,"*\n");
	}
#if 0
	data.level=13;
	g_hash_table_foreach(data.tile_edges, tile_collector_add_siblings, &data);
	g_hash_table_foreach(data.tile_edges, tile_collector_add_siblings, &data);
	g_hash_table_foreach(data.tile_edges, tile_collector_add_siblings2, &data);
	data.level=12;
	g_hash_table_foreach(data.tile_edges, tile_collector_add_siblings, &data);
	g_hash_table_foreach(data.tile_edges, tile_collector_add_siblings, &data);
#endif
	item_bin_sink_func_destroy(tile_collector);
	fprintf(stderr,"tile_collector_finish done\n");
	return 0;
}


static int
coastline_processor_process(struct item_bin_sink_func *func, struct item_bin *ib, struct tile_data *tile_data)
{
#if 0
	int i;
	struct coord *c=(struct coord *)(ib+1);
	for (i = 0 ; i < 19 ; i++) {
		c[i]=c[i+420];
	}
	ib->clen=(i-1)*2;
#endif
	item_bin_write_clipped(ib, func->priv_data[0], func->priv_data[1]);
	return 0;
}

static struct item_bin_sink_func *
coastline_processor_new(struct item_bin_sink *out)
{
	struct item_bin_sink_func *coastline_processor=item_bin_sink_func_new(coastline_processor_process);
	struct item_bin_sink *tiles=item_bin_sink_new();
	struct item_bin_sink_func *tile_collector=tile_collector_new(out);
	struct tile_parameter *param=g_new0(struct tile_parameter, 1);

	fprintf(stderr,"new:out=%p\n",out);
	param->min=14;
	param->max=14;
	param->overlap=0;

	item_bin_sink_add_func(tiles, tile_collector);
	coastline_processor->priv_data[0]=param;
	coastline_processor->priv_data[1]=tiles;
	coastline_processor->priv_data[2]=tile_collector;
	return coastline_processor;
}

static void
coastline_processor_finish(struct item_bin_sink_func *coastline_processor)
{
	struct tile_parameter *param=coastline_processor->priv_data[0];
	struct item_bin_sink *tiles=coastline_processor->priv_data[1];
	struct item_bin_sink_func *tile_collector=coastline_processor->priv_data[2];
	g_free(param);
	tile_collector_finish(tile_collector);
	item_bin_sink_destroy(tiles);
	item_bin_sink_func_destroy(coastline_processor);
}

void
process_coastlines(FILE *in, FILE *out)
{
	struct item_bin_sink *reader=file_reader_new(in,1000000,0);
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
