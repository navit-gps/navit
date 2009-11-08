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


#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <glib.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <getopt.h>
#include <unistd.h>
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
#ifdef HAVE_POSTGRESQL
#include <libpq-fe.h>
#endif
#include "maptool.h"

struct rect world_bbox = {
	{ -20000000, -20000000},
	{  20000000,  20000000},
};

void
bbox_extend(struct coord *c, struct rect *r)
{
	if (c->x < r->l.x)
		r->l.x=c->x;
	if (c->y < r->l.y)
		r->l.y=c->y;
	if (c->x > r->h.x)
		r->h.x=c->x;
	if (c->y > r->h.y)
		r->h.y=c->y;
}

void
bbox(struct coord *c, int count, struct rect *r)
{
	if (! count)
		return;
	r->l=*c;
	r->h=*c;
	while (--count) {
		c++;
		bbox_extend(c, r);
	}
}

int
contains_bbox(int xl, int yl, int xh, int yh, struct rect *r)
{
	if (r->h.x < xl || r->h.x > xh) {
		return 0;
	}
	if (r->l.x > xh || r->l.x < xl) {
		return 0;
	}
	if (r->h.y < yl || r->h.y > yh) {
		return 0;
	}
	if (r->l.y > yh || r->l.y < yl) {
		return 0;
	}
	return 1;
}

void
phase1_map(struct map *map, FILE *out_ways, FILE *out_nodes)
{
	struct map_rect *mr=map_rect_new(map, NULL);
	struct item *item;
	int count,max=16384;
	struct coord ca[max];
	struct attr attr;

	while ((item = map_rect_get_item(mr))) {
		count=item_coord_get(item, ca, item->type < type_line ? 1: max);
		item_bin_init(item_bin, item->type);
		item_bin_add_coord(item_bin, ca, count);
		while (item_attr_get(item, attr_any, &attr)) {
			item_bin_add_attr(item_bin, &attr);
		}
		if (item->type >= type_line) 
			item_bin_write(item_bin, out_ways);
		else
			item_bin_write(item_bin, out_nodes);
	}
	map_rect_destroy(mr);
}

static void
phase34_process_file(struct tile_info *info, FILE *in, FILE *reference)
{
	struct item_bin *ib;
	int max;

	while ((ib=read_item(in))) {
		if (ib->type < 0x80000000)
			processed_nodes++;
		else
			processed_ways++;
		max=14;
		switch (ib->type) {
		case type_town_label_1e7:
		case type_town_label_5e6:
		case type_town_label_2e6:
		case type_town_label_1e6:
		case type_town_label_5e5:
		case type_district_label_1e7:
		case type_district_label_5e6:
		case type_district_label_2e6:
		case type_district_label_1e6:
		case type_district_label_5e5:
			max=6;
			break;
		case type_town_label_2e5:
		case type_town_label_1e5:
		case type_district_label_2e5:
		case type_district_label_1e5:
		case type_street_n_lanes:
		case type_highway_city:
		case type_highway_land:
		case type_ramp:
			max=8;
			break;
		case type_town_label_5e4:
		case type_town_label_2e4:
		case type_town_label_1e4:
		case type_district_label_5e4:
		case type_district_label_2e4:
		case type_district_label_1e4:
			max=9;
			break;
		case type_town_label_5e3:
		case type_town_label_2e3:
		case type_town_label_1e3:
		case type_district_label_5e3:
		case type_district_label_2e3:
		case type_district_label_1e3:
		case type_street_3_city:
		case type_street_4_city:
		case type_street_3_land:
		case type_street_4_land:
			max=12;
			break;
		default:
			break;
		}
		tile_write_item_minmax(info, ib, reference, 0, max);
	}
}

static int
phase34(struct tile_info *info, struct zip_info *zip_info, FILE **in, FILE **reference, int in_count)
{
	int i;

	processed_nodes=processed_nodes_out=processed_ways=processed_relations=processed_tiles=0;
	bytes_read=0;
	sig_alrm(0);
	if (! info->write)
		tile_hash=g_hash_table_new(g_str_hash, g_str_equal);
	for (i = 0 ; i < in_count ; i++) {
		if (in[i])
			phase34_process_file(info, in[i], reference ? reference[i]:NULL);
	}
	if (! info->write)
		merge_tiles(info);
	sig_alrm(0);
	sig_alrm_end();
	write_tilesdir(info, zip_info, info->tilesdir_out);

	return 0;

}


void
dump(FILE *in)
{
	struct item_bin *ib;
	while ((ib=read_item(in))) {
		dump_itembin(ib);
	}
}

int
phase4(FILE **in, int in_count, char *suffix, FILE *tilesdir_out, struct zip_info *zip_info)
{
	struct tile_info info;
	info.write=0;
	info.maxlen=0;
	info.suffix=suffix;
	info.tiles_list=NULL;
	info.tilesdir_out=tilesdir_out;
	return phase34(&info, zip_info, in, NULL, in_count);
}

static int
process_slice(FILE **in, FILE **reference, int in_count, long long size, char *suffix, struct zip_info *zip_info)
{
	struct tile_head *th;
	char *slice_data,*zip_data;
	int zipfiles=0;
	struct tile_info info;
	int i;

	slice_data=malloc(size);
	assert(slice_data != NULL);
	zip_data=slice_data;
	th=tile_head_root;
	while (th) {
		if (th->process) {
			th->zip_data=zip_data;
			zip_data+=th->total_size;
		}
		th=th->next;
	}
	for (i = 0 ; i < in_count ; i++) {
		if (in[i])
			fseek(in[i], 0, SEEK_SET);
		if (reference && reference[i]) {
			fseek(reference[i], 0, SEEK_SET);
		}
	}
	info.write=1;
	info.maxlen=zip_info->maxnamelen;
	info.suffix=suffix;
	info.tiles_list=NULL;
	info.tilesdir_out=NULL;
	phase34(&info, zip_info, in, reference, in_count);

	th=tile_head_root;
	while (th) {
		if (th->process) {
			if (th->name[0]) {
				if (th->total_size != th->total_size_used) {
					fprintf(stderr,"Size error '%s': %d vs %d\n", th->name, th->total_size, th->total_size_used);
					exit(1);
				}
				write_zipmember(zip_info, th->name, zip_info->maxnamelen, th->zip_data, th->total_size);
				zipfiles++;
			} else 
				fwrite(th->zip_data, th->total_size, 1, zip_info->index);
		}
		th=th->next;
	}
	free(slice_data);

	return zipfiles;
}

int
phase5(FILE **in, FILE **references, int in_count, char *suffix, struct zip_info *zip_info)
{
	long long size;
	int slices;
	int zipnum,written_tiles;
	struct tile_head *th,*th2;
	create_tile_hash();

	th=tile_head_root;
	size=0;
	slices=0;
	fprintf(stderr, "Maximum slice size %Ld\n", slice_size);
	while (th) {
		if (size + th->total_size > slice_size) {
			fprintf(stderr,"Slice %d is of size %Ld\n", slices, size);
			size=0;
			slices++;
		}
		size+=th->total_size;
		th=th->next;
	}
	if (size)
		fprintf(stderr,"Slice %d is of size %Ld\n", slices, size);
	th=tile_head_root;
	size=0;
	slices=0;
	while (th) {
		th2=tile_head_root;
		while (th2) {
			th2->process=0;
			th2=th2->next;
		}
		size=0;
		while (th && size+th->total_size < slice_size) {
			size+=th->total_size;
			th->process=1;
			th=th->next;
		}
		/* process_slice() modifies zip_info, but need to retain old info */
		zipnum=zip_info->zipnum;
		written_tiles=process_slice(in, references, in_count, size, suffix, zip_info);
		zip_info->zipnum=zipnum+written_tiles;
		slices++;
	}
	return 0;
}

void
process_binfile(FILE *in, FILE *out)
{
	struct item_bin *ib;
	while ((ib=read_item(in))) {
		fwrite(ib, (ib->len+1)*4, 1, out);
	}
}
