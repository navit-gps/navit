#include <stdio.h>
#include <string.h>
#include "debug.h"
#include "plugin.h"
#include "maptype.h"
#include "projection.h"
#include "mg.h"


struct map_priv * map_new_mg(struct map_methods *meth, char *dirname, char **charset, enum projection *pro);

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
		/* case file_bridge_ply: */
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

static struct map_search_priv *
map_search_new_mg(struct map_priv *map, struct item *item, struct attr *search, int partial)
{
	struct map_rect_priv *mr=g_new0(struct map_rect_priv, 1);
	dbg(1,"id_lo=0x%x\n", item->id_lo);
	dbg(1,"search=%s\n", search->u.str);
	mr->m=map;
	mr->search_type=search->type;
	switch (search->type) {
	case attr_town_name:
		if (item->type != type_country_label)
			return NULL;
		tree_search_init(map->dirname, "town.b2", &mr->ts, 0x1000);
		break;
	case attr_street_name:
		if (item->type != type_town_streets)
			return NULL;
		dbg(1,"street_assoc=0x%x\n", item->id_lo);
		tree_search_init(map->dirname, "strname.b1", &mr->ts, 0);
		break;
	default:
		dbg(0,"unknown search\n");
		g_free(mr);
		return NULL;
	}
	mr->search_item=*item;
	mr->search_country=item->id_lo;
	mr->search_str=search->u.str;
	mr->search_partial=partial;
	mr->current_file=file_town_twn-1;
	file_next(mr);
	return (struct map_search_priv *)mr;
}

static void
map_search_destroy_mg(struct map_search_priv *ms)
{
	struct map_rect_priv *mr=(struct map_rect_priv *)ms;

	if (! mr)
		return;
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
	case attr_town_name:
		return town_search_get_item(mr);
	case attr_street_name:
		return street_search_get_item(mr);
	default:
		return NULL;
	}
}

static struct map_methods map_methods_mg = {
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
map_new_mg(struct map_methods *meth, char *dirname, char **charset, enum projection *pro)
{
	struct map_priv *m;
	int i,maybe_missing,len=strlen(dirname);
	char filename[len+16];
	
	*meth=map_methods_mg;
	*charset="iso8859-1";
	*pro=projection_mg;

	m=g_new(struct map_priv, 1);
	m->id=++map_id;
	m->dirname=g_strdup(dirname);
	strcpy(filename, dirname);
	filename[len]='/';
	for (i = 0 ; i < file_end ; i++) {
		if (file[i]) {
			strcpy(filename+len+1, file[i]);
			m->file[i]=file_create_caseinsensitive(filename);
			if (! m->file[i]) {
				maybe_missing=(i == file_border_ply || i == file_height_ply || i == file_sea_ply);
				if (! maybe_missing)
					g_warning("Failed to load %s", filename);
			}
		}
	}

	return m;
}

void
plugin_init(void)
{
	plugin_register_map_type("mg", map_new_mg);
}
