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

#include <stdio.h>
#include <string.h>
#include "config.h"
#include "debug.h"
#include "plugin.h"
#include "maptype.h"
#include "mg.h"


static struct country_isonum {
	int country;
	int isonum;
	int postal_len;
	char *postal_prefix;
} country_isonums[]={
  {  1,203},
  {  2,703},
  {  7,674},
  { 11,233},
  { 12,268},
  { 13,428},
  { 14,440},
  { 15,498},
  { 16,643},
  { 17,804},
  { 18,112},
  { 20,818},
  { 30,300},
  { 31,528},
  { 32, 56},
  { 33,250},
  { 34,724},
  { 36,348},
  { 39,380},
  { 40,642},
  { 41,756},
  { 43, 40},
  { 44,826},
  { 45,208},
  { 46,752},
  { 47,578},
  { 48,616},
  { 49,276,5,"D@@"},
  { 50,292},
  { 51,620},
  { 52,442},
  { 53,372},
  { 54,352},
  { 55,  8},
  { 56,470},
  { 57,196},
  { 58,246},
  { 59,100},
  { 61,422},
  { 62, 20},
  { 63,760},
  { 66,682},
  { 71,434},
  { 72,376},
  { 73,275},
  { 75,438},
  { 76,504},
  { 77, 12},
  { 78,788},
  { 81,688},
  { 83,400},
  { 85,191},
  { 86,705},
  { 87, 70},
  { 89,807},
  { 90,792},
  { 93,492},
  { 94, 31},
  { 95, 51},
  { 98,234},
  { 99,732},
  {336,774},
};

struct map_priv * map_new_mg(struct map_methods *meth, struct attr **attrs);

static int map_id;

static char *file[]={
	[file_border_ply]="border.ply",
	[file_bridge_ply]="bridge.ply",
	[file_build_ply]="build.ply",
	[file_golf_ply]="golf.ply",
	[file_height_ply]="height.ply",
	[file_natpark_ply]="natpark.ply",
	[file_nature_ply]="nature.ply",
	[file_other_ply]="other.ply",
	[file_rail_ply]="rail.ply",
	[file_sea_ply]="sea.ply",
	[file_street_bti]="street.bti",
	[file_street_str]="street.str",
	[file_strname_stn]="strname.stn",
	[file_town_twn]="town.twn",
	[file_tunnel_ply]="tunnel.ply",
	[file_water_ply]="water.ply",
	[file_woodland_ply]="woodland.ply",
};

int mg_country_from_isonum(int isonum)
{
	int i;
	for (i = 0 ; i < sizeof(country_isonums)/sizeof(struct country_isonum) ; i++)
		if (country_isonums[i].isonum == isonum)
			return country_isonums[i].country;
	return 0;
}

int mg_country_to_isonum(int country)
{
	int i;
	for (i = 0 ; i < sizeof(country_isonums)/sizeof(struct country_isonum) ; i++)
		if (country_isonums[i].country == country)
			return country_isonums[i].isonum;
	return 0;
}

int mg_country_postal_len(int country)
{
	int i;
	for (i = 0 ; i < sizeof(country_isonums)/sizeof(struct country_isonum) ; i++)
		if (country_isonums[i].country == country)
			return country_isonums[i].postal_len;
	return 0;
}

static char *mg_country_postal_prefix(int isonum)
{
	int i;
	for (i = 0 ; i < sizeof(country_isonums)/sizeof(struct country_isonum) ; i++)
		if (country_isonums[i].isonum == isonum)
			return country_isonums[i].postal_prefix;
	return NULL;
}

static int
file_next(struct map_rect_priv *mr)
{
	int debug=0;
	enum layer_type layer;

	for (;;) {
		mr->current_file++;
		if (mr->current_file >= file_end)
			return 0;
		mr->file=mr->m->file[mr->current_file];
		if (! mr->file)
			continue;
		switch (mr->current_file) {
		case file_strname_stn:
			continue;
		case file_town_twn:
			layer=layer_town;
			break;
		case file_street_str:
			layer=layer_street;
			break;
		default:
			layer=layer_poly;
		}
		if (mr->cur_sel && !mr->cur_sel->order[layer])
			continue;
		if (debug)
			printf("current file: '%s'\n", file[mr->current_file]);
		mr->cur_sel=mr->xsel;
		if (block_init(mr))
			return 1;
	}
}

static void
map_destroy_mg(struct map_priv *m)
{
	int i;

	printf("mg_map_destroy\n");
	for (i = 0 ; i < file_end ; i++) {
		if (m->file[i])
			file_destroy(m->file[i]);
	}
}

extern int block_lin_count,block_idx_count,block_active_count,block_mem,block_active_mem;

static struct map_rect_priv *
map_rect_new_mg(struct map_priv *map, struct map_selection *sel)
{
	struct map_rect_priv *mr;
	int i;

	block_lin_count=0;
	block_idx_count=0;
	block_active_count=0;
	block_mem=0;
	block_active_mem=0;
	mr=g_new0(struct map_rect_priv, 1);
	mr->m=map;
	mr->xsel=sel;
	mr->current_file=-1;
	if (sel && sel->next)
		for (i=0 ; i < file_end ; i++) 
			mr->block_hash[i]=g_hash_table_new(g_int_hash,g_int_equal);
		
	file_next(mr);
	return mr;
}


static struct item *
map_rect_get_item_mg(struct map_rect_priv *mr)
{
	for (;;) {
		switch (mr->current_file) {
		case file_town_twn:
			if (town_get(mr, &mr->town, &mr->item))
				return &mr->item;
			break;
		case file_border_ply:
		case file_bridge_ply:
		case file_build_ply: 
		case file_golf_ply: 
		/* case file_height_ply: */
		case file_natpark_ply: 
		case file_nature_ply: 
		case file_other_ply:
		case file_rail_ply:
		case file_sea_ply:
		/* case file_tunnel_ply: */
		case file_water_ply:
		case file_woodland_ply:
			if (poly_get(mr, &mr->poly, &mr->item))
				return &mr->item;
			break;
		case file_street_str:
			if (street_get(mr, &mr->street, &mr->item))
				return &mr->item;
			break;
		case file_end:
			return NULL;
		default:
			break;
		}
		if (block_next(mr))
			continue;
		if (mr->cur_sel->next) {
			mr->cur_sel=mr->cur_sel->next;
			if (block_init(mr))
				continue;
		}
		if (file_next(mr))
			continue;
		dbg(1,"lin_count %d idx_count %d active_count %d %d kB (%d kB)\n", block_lin_count, block_idx_count, block_active_count, (block_mem+block_active_mem)/1024, block_active_mem/1024);
		return NULL;
	}
}

static struct item *
map_rect_get_item_byid_mg(struct map_rect_priv *mr, int id_hi, int id_lo)
{
	mr->current_file = id_hi >> 16;
	switch (mr->current_file) {
	case file_town_twn:
		if (town_get_byid(mr, &mr->town, id_hi, id_lo, &mr->item))
			return &mr->item;
		break;
	case file_street_str:
		if (street_get_byid(mr, &mr->street, id_hi, id_lo, &mr->item))
			return &mr->item;
		break;
	default:	
		if (poly_get_byid(mr, &mr->poly, id_hi, id_lo, &mr->item))
			return &mr->item;
		break;
	}
	return NULL;
}


static void
map_rect_destroy_mg(struct map_rect_priv *mr)
{
	int i;
	for (i=0 ; i < file_end ; i++) 
		if (mr->block_hash[i])
			g_hash_table_destroy(mr->block_hash[i]);	
	g_free(mr);
}

static char *
map_search_mg_convert_special(char *str)
{
	char *ret,*c=g_malloc(strlen(str)*2+1);

	ret=c;
	for (;;) {
		switch ((unsigned char)(*str)) {
		case 0xc4:
			*c++='A';
			break;
		case 0xd6:
			*c++='O';
			break;
		case 0xdc:
			*c++='U';
			break;
		case 0xdf:
			*c++='s';
			*c++='s';
			break;
		case 0xe4:
			*c++='a';
			break;
		case 0xf6:
			*c++='o';
			break;
		case 0xfc:
			*c++='u';
			break;
		default:
			dbg(1,"0x%x\n", *str);
			*c++=*str;
			break;
		}
		if (! *str)
			return ret;
		str++;
	}
}


static struct map_search_priv *
map_search_new_mg(struct map_priv *map, struct item *item, struct attr *search, int partial)
{
	struct map_rect_priv *mr=g_new0(struct map_rect_priv, 1);
	char *prefix;
	dbg(1,"id_lo=0x%x\n", item->id_lo);
	dbg(1,"search=%s\n", search->u.str);
	mr->m=map;
	mr->search_type=search->type;

	/*
     * NOTE: If you implement search for other attributes than attr_town_postal, attr_town_name and attr_street_name,
     * please update this comment and the documentation for map_search_new() in map.c
     */

	switch (search->type) {
	case attr_town_postal:
		if (item->type != type_country_label)
			return NULL;
		prefix=mg_country_postal_prefix(item->id_lo);
		if (! prefix)
			return NULL;
		tree_search_init(map->dirname, "town.b1", &mr->ts, 0);
		mr->current_file=file_town_twn;
		mr->search_str=g_strdup_printf("%s%s",prefix,search->u.str);
		dbg(0,"search_str='%s'\n",mr->search_str);
		mr->search_country=mg_country_from_isonum(item->id_lo);
		break;
	case attr_town_name:
		if (item->type != type_country_label)
			return NULL;
		tree_search_init(map->dirname, "town.b2", &mr->ts, 0x1000);
		mr->current_file=file_town_twn;
		mr->search_str=map_search_mg_convert_special(search->u.str);
		mr->search_country=mg_country_from_isonum(item->id_lo);
		break;
	case attr_street_name:
		if (item->type != type_town_streets)
			return NULL;
		dbg(1,"street_assoc=0x%x\n", item->id_lo);
		tree_search_init(map->dirname, "strname.b1", &mr->ts, 0);
		mr->current_file=file_strname_stn;
		mr->search_str=g_strdup(search->u.str);
		break;
	default:
		dbg(0,"unknown search\n");
		g_free(mr);
		return NULL;
	}
	mr->search_item=*item;
	mr->search_partial=partial;
	mr->file=mr->m->file[mr->current_file];
	block_init(mr);
	return (struct map_search_priv *)mr;
}

static void
map_search_destroy_mg(struct map_search_priv *ms)
{
	struct map_rect_priv *mr=(struct map_rect_priv *)ms;

	dbg(1,"mr=%p\n", mr);
	if (! mr)
		return;
	g_free(mr->search_str);
	tree_search_free(&mr->ts);
	g_free(mr);
}

static struct item *
map_search_get_item_mg(struct map_search_priv *ms)
{
	struct map_rect_priv *mr=(struct map_rect_priv *)ms;

	if (! mr)
		return NULL;
	switch (mr->search_type) {
	case attr_town_postal:
	case attr_town_name:
		return town_search_get_item(mr);
	case attr_street_name:
		return street_search_get_item(mr);
	default:
		return NULL;
	}
}

static struct map_methods map_methods_mg = {
	projection_mg,
	"iso8859-1",
	map_destroy_mg,
	map_rect_new_mg,
	map_rect_destroy_mg,
	map_rect_get_item_mg,
	map_rect_get_item_byid_mg,
	map_search_new_mg,
	map_search_destroy_mg,
	map_search_get_item_mg,
};

struct map_priv *
map_new_mg(struct map_methods *meth, struct attr **attrs)
{
	struct map_priv *m;
	int i,maybe_missing;
	struct attr *data;
	char *filename;
	
	*meth=map_methods_mg;
	data=attr_search(attrs, NULL, attr_data);
	if (! data)
		return NULL;

	m=g_new(struct map_priv, 1);
	m->id=++map_id;
	m->dirname=g_strdup(data->u.str);
	for (i = 0 ; i < file_end ; i++) {
		if (file[i]) {
			filename=g_strdup_printf("%s/%s", data->u.str, file[i]);
			m->file[i]=file_create_caseinsensitive(filename);
			if (! m->file[i]) {
				maybe_missing=(i == file_border_ply || i == file_height_ply || i == file_sea_ply);
				if (! maybe_missing)
					dbg(0,"Failed to load %s", filename);
			} else
				file_mmap(m->file[i]);
			g_free(filename);
		}
	}

	return m;
}

void
plugin_init(void)
{
	plugin_register_map_type("mg", map_new_mg);
}
