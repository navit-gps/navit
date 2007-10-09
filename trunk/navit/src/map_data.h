#ifndef NAVIT_MAP_DATA_H
#define NAVIT_MAP_DATA_H

enum file_index {
	file_border_ply=0,
	file_bridge_ply,
	file_height_ply,
	file_other_ply,
	file_rail_ply,
	file_sea_ply,
	file_street_bti,
	file_street_str,
	file_strname_stn,
	file_town_twn,
	file_tunnel_ply,
	file_water_ply,
	file_woodland_ply,
	file_end
};

struct map_data {
	struct file *file[file_end];
	struct map_data *next;
};

struct map_data *load_maps(char *map);

struct transformation;
struct block_info;

void map_data_foreach(struct map_data *mdata, int file, struct transformation *t, int limit,
     void(*func)(struct block_info *, unsigned char *, unsigned char *, void *), void *data);

#endif

