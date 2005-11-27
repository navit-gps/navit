#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "file.h"
#include "block.h"
#include "map_data.h"
#include "log.h"

static struct map_data *load_map(char *dirname)
{
	int i,len=strlen(dirname);
	char *filename[file_end];
	char file[len+16];
	struct map_data *data=g_new0(struct map_data,1);

	memset(filename, 0, sizeof(*filename));

	filename[file_border_ply]="border.ply";
	filename[file_bridge_ply]="bridge.ply";
	filename[file_height_ply]="height.ply";
	filename[file_other_ply]="other.ply";
	filename[file_rail_ply]="rail.ply";
	filename[file_sea_ply]="sea.ply";
	filename[file_street_bti]="street.bti";
	filename[file_street_str]="street.str";
	filename[file_strname_stn]="strname.stn";
	filename[file_town_twn]="town.twn";
	filename[file_tunnel_ply]="tunnel.ply";
	filename[file_water_ply]="water.ply";
	filename[file_woodland_ply]="woodland.ply";

	strcpy(file, dirname);
	file[len]='/';
	for (i = 0 ; i < file_end ; i++) {
		if (filename[i]) {
			strcpy(file+len+1, filename[i]);
			data->file[i]=file_create_caseinsensitive(file);
			if (! data->file[i]) {
				g_warning("Failed to load %s", file);
			}
		}
	}
	return data;
}

struct map_data *load_maps(char *map)
{
	char *name;
	void *hnd;
	struct map_data *last,*ret;
	int i;

	if (! map)
		map=getenv("MAP_DATA");
	if (! map)
		map="/opt/reiseplaner/travel/DE.map";
	
	ret=load_map(map);
	last=ret;
	hnd=file_opendir(map);
	if (hnd) {	
		while ((name=file_readdir(hnd))) {
			if (!strcasecmp(name+strlen(name)-4,".smp")) {
				char next_name[strlen(map)+strlen(name)+2];
				strcpy(next_name, map);
				strcat(next_name, "/");
				strcat(next_name, name);
				last->next=load_map(next_name);
				last=last->next;
			}
		}
		file_closedir(hnd);	
	} else {
		g_warning("Unable to open Map Directory '%s'\n", map);
	}
	log_apply(ret, file_end);

#if 0
	last=ret;
	while (last) {
		for (i = 0 ; i < file_end ; i++) {
			if (last->file[i]) {
				file_set_readonly(last->file[i]);
			}
		}
		last=last->next;
	}
	*((int *)0)=0;
#endif

	return ret;
}


void
map_data_foreach(struct map_data *mdata, int file, struct transformation *t, int limit, 
		  void(*func)(struct block_info *, unsigned char *, unsigned char *, void *), void *data)
{
	struct block_info blk_inf;

	memset(&blk_inf, 0, sizeof(blk_inf));

	while (mdata) {
		if (mdata->file[file]) {
			blk_inf.mdata=mdata;
			blk_inf.file=mdata->file[file];
			block_foreach_visible(&blk_inf, t, limit, data, func);
		}
		mdata=mdata->next;
	}
}
