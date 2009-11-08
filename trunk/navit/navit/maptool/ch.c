#include <math.h>
#include <stdlib.h>
#include "maptool.h"
#include "coord.h"
#include "file.h"
#include "debug.h"

struct ch_edge {
	int flags;
	int weight;
	struct item_id target,middle;
};

struct node {
	int first_edge;
	int dummy;
} *nodes;
int node_count;

struct edge {
	unsigned target:26;
	unsigned scedge1:6;
	unsigned weight:28;
	unsigned type:2;
	unsigned flags:2;
	unsigned int edge_count;
	unsigned scedge2:6;
	unsigned scmiddle:26;
} *edges;

int edge_count;

struct newnode {
	int newnode;
} *newnodes;

int newnode_count;

GHashTable *newnode_hash;

struct edge_hash_item {
	int first,last;
};


GHashTable *edge_hash;

struct file *sgr,*ddsg_node_index;

struct coord *node_index;

GHashTable *sgr_nodes_hash;

static int ch_levels=14;

static int
road_speed(enum item_type type)
{
	switch (type) {
	case type_street_0:
	case type_street_1_city:
	case type_living_street:
	case type_street_service:
	case type_track_gravelled:
	case type_track_unpaved:
		return 10;
	case type_street_2_city:
	case type_track_paved:
		return 30;
	case type_street_3_city:
		return 40;
	case type_street_4_city:
		return 50;
	case type_highway_city:
		return 80;
	case type_street_1_land:
		return 60;
	case type_street_2_land:
		return 65;
	case type_street_3_land:
		return 70;
	case type_street_4_land:
		return 80;
	case type_street_n_lanes:
		return 120;
	case type_highway_land:
		return 120;
	case type_ramp:
		return 40;
	case type_roundabout:
		return 10;
	case type_ferry:
		return 40;
	default:
		return 0;
	}
}

static GHashTable *
coord_hash_new(void)
{
        return g_hash_table_new_full(coord_hash, coord_equal, g_free, NULL);
}

#define sq(x) ((double)(x)*(x))

static void
add_node_to_hash(FILE *idx, GHashTable *hash, struct coord *c, int *nodes)
{
	if (! g_hash_table_lookup(hash, c)) {
		struct coord *ct=g_new(struct coord, 1);
		*ct=*c;
		fwrite(c, sizeof(*c), 1, idx);
		(*nodes)++;
		g_hash_table_insert(hash, ct, (void *)(*nodes));
	}

}

static guint
edge_hash_hash(gconstpointer key)
{
        const struct edge_hash_item *itm=key;
        return itm->first*2654435761UL+itm->last;
}

static gboolean
edge_hash_equal(gconstpointer a, gconstpointer b)
{
        const struct edge_hash_item *itm_a=a;
        const struct edge_hash_item *itm_b=b;
	return (itm_a->first == itm_b->first && itm_a->last == itm_b->last);
}



static void
ch_generate_ddsg(FILE *in, FILE *ref, FILE *idx, FILE *ddsg)
{
	GHashTable *hash=coord_hash_new();
	struct item_bin *ib;
	int nodes=0,edges=0;

	while ((ib=read_item(in))) {
		int ccount=ib->clen/2;
                struct coord *c=(struct coord *)(ib+1);
		if (road_speed(ib->type)) {
			add_node_to_hash(idx, hash, &c[0], &nodes);
			add_node_to_hash(idx, hash, &c[ccount-1], &nodes);
			edges++;
		}
	}
	edge_hash=g_hash_table_new_full(edge_hash_hash, edge_hash_equal, g_free, g_free);
	fseek(in, 0, SEEK_SET);
	fprintf(ddsg,"d\n");
	fprintf(ddsg,"%d %d\n", nodes, edges);
	while ((ib=read_item(in))) {
		int i,ccount=ib->clen/2;
                struct coord *c=(struct coord *)(ib+1);
		int n1,n2,speed=road_speed(ib->type);
		struct item_id road_id;
		double l;
		fread(&road_id, sizeof(road_id), 1, ref);
		if (speed) {
			struct edge_hash_item *hi=g_new(struct edge_hash_item, 1);
			struct item_id *id=g_new(struct item_id, 1);
			*id=road_id;
			dbg_assert((n1=GPOINTER_TO_INT(g_hash_table_lookup(hash, &c[0]))) != 0);
			dbg_assert((n2=GPOINTER_TO_INT(g_hash_table_lookup(hash, &c[ccount-1]))) != 0);
			l=0;
			for (i = 0 ; i < ccount-1 ; i++) {
				l+=sqrt(sq(c[i+1].x-c[i].x)+sq(c[i+1].y-c[i].y));
			}
			fprintf(ddsg,"%d %d %d 0\n", n1-1, n2-1, (int)(l*36/speed));
			hi->first=n1-1;
			hi->last=n2-1;
			g_hash_table_insert(edge_hash, hi, id);
		}
	}
	g_hash_table_destroy(hash);
}

static void
ch_generate_sgr(char *suffix)
{
	char command[1024];
	sprintf(command,"./contraction-hierarchies-20080621/main -s -p -f ddsg_%s.tmp -o hcn_%s.tmp -l hcn_log_%s.tmp -x 190 -y 1 -e 600 -p 1000 -k 1,3.3,2,10,3,10,5",suffix,suffix,suffix);
	printf("%s\n",command);
	system(command);
	sprintf(command,"./contraction-hierarchies-20080621/main -c -f ddsg_%s.tmp -h hcn_%s.tmp -k 1,3.3,2,10,3,10,5 -C ch_%s.tmp -O 1 -z sgr_%s.tmp",suffix,suffix,suffix,suffix);
	printf("%s\n",command);
	system(command);
}

static void
ch_process_node(FILE *out, int node, int resolve)
{
	int first_edge_id=nodes[node].first_edge;
	int last_edge_id=nodes[node+1].first_edge;
	int edge_id;
	struct ch_edge ch_edge;
	memset(&ch_edge, 0, sizeof(ch_edge));
	struct edge_hash_item fwd,rev;
	item_bin_init(item_bin, type_ch_node);
	int oldnode=GPOINTER_TO_INT(g_hash_table_lookup(newnode_hash, GINT_TO_POINTER(node)));
#if 0
	dbg(0,"0x%x,0x%x\n",node_index[oldnode].x,node_index[oldnode].y);
#endif
	item_bin_add_coord(item_bin, &node_index[oldnode], 1);
	fwd.first=oldnode;
	rev.last=oldnode;
	for (edge_id = first_edge_id ; edge_id < last_edge_id ; edge_id++) {
		if (resolve)  {
			struct edge *edge=&edges[edge_id];
			int oldnode=GPOINTER_TO_INT(g_hash_table_lookup(newnode_hash, GINT_TO_POINTER((int)edge->target)));
			struct item_id *id;
			ch_edge.weight=edge->weight;
			fwd.last=oldnode;
			rev.first=oldnode;
			ch_edge.flags=edge->flags & 3;
			if (edge->scmiddle == 67108863) {
				id=g_hash_table_lookup(edge_hash, &fwd);
				if (!id) {
					ch_edge.flags|=8;
					id=g_hash_table_lookup(edge_hash, &rev);
				}
				if (id == NULL) {
					fprintf(stderr,"Shortcut %d Weight %d\n",edge->scmiddle,edge->weight);
					fprintf(stderr,"Neither %d-%d nor %d-%d exists\n",fwd.first,fwd.last,rev.first,rev.last);
					exit(1);
				} else {
					ch_edge.middle=*id;
#if 0
					dbg(0,"middle street id for is "ITEM_ID_FMT"\n",ITEM_ID_ARGS(*id));
#endif
				}
			} else {
				ch_edge.flags|=4;
				id=g_hash_table_lookup(sgr_nodes_hash, GINT_TO_POINTER((int)edge->scmiddle));
				dbg_assert(id != NULL);
				ch_edge.middle=*id;
#if 0
				dbg(0,"middle node id for is "ITEM_ID_FMT"\n",ITEM_ID_ARGS(*id));
#endif
			}
			id=g_hash_table_lookup(sgr_nodes_hash, GINT_TO_POINTER((int)edge->target));
			if (id == NULL) {
				fprintf(stderr,"Failed to look up target %d\n",edge->target);
			}
#if 0
			dbg(0,"id for %d is "ITEM_ID_FMT"\n",edge->target,ITEM_ID_ARGS(*id));
#endif
			ch_edge.target=*id;
		}
		item_bin_add_attr_data(item_bin,attr_ch_edge,&ch_edge,sizeof(ch_edge));
	}
	item_bin_write(item_bin, out);
}

static void
ch_process_nodes(FILE *out, int pos, int count, int resolve)
{
	int i;
	printf("count %d sum=%d newnode_count=%d\n",count,pos,newnode_count);
	for (i = 0 ; i < count ; i++) 
		ch_process_node(out, pos+i, resolve);
}


static void
ch_process(FILE **files, int depth, int resolve)
{
	int count=newnode_count;
	int pos=0;

	while (depth > 0 && pos < newnode_count) {
		count=(count+1)/2;
		ch_process_nodes(files[depth], pos, count, resolve);
		pos+=count;
		depth--;
	}
	ch_process_nodes(files[depth], pos, newnode_count-pos, resolve);
}

static void
ch_setup(char *suffix)
{
	int i;
	if (!sgr) {
		int *data,size,offset=0;
		char *filename=tempfile_name(suffix,"sgr");
		printf("filename=%s\n",filename);
		sgr=file_create(filename,0);
		g_free(filename);
		dbg_assert(sgr != NULL);
		file_mmap(sgr);

		size=sizeof(int);
		data=(int *)file_data_read(sgr, offset, size);
		node_count=*data;
		offset+=size;

		size=node_count*sizeof(struct node);
		nodes=(struct node *)file_data_read(sgr, offset, size);
		offset+=size;

		size=sizeof(int);
		data=(int *)file_data_read(sgr, offset, size);
		edge_count=*data;
		offset+=size;

		size=edge_count*sizeof(struct edge);
		edges=(struct edge *)file_data_read(sgr, offset, size);
		offset+=size;

		size=sizeof(int);
		data=(int *)file_data_read(sgr, offset, size);
		newnode_count=*data;
		offset+=size;

		size=edge_count*sizeof(struct newnode);
		newnodes=(struct newnode *)file_data_read(sgr, offset, size);
		offset+=size;

		newnode_hash=g_hash_table_new(NULL, NULL);

		for (i = 0 ; i < newnode_count ; i++) {
			g_hash_table_insert(newnode_hash, GINT_TO_POINTER(newnodes[i].newnode), GINT_TO_POINTER(i));
        	}
	}
	if (!ddsg_node_index) {
		char *filename=tempfile_name(suffix,"ddsg_coords");
		ddsg_node_index=file_create(filename,0);
		g_free(filename);
		dbg_assert(ddsg_node_index != NULL);
		file_mmap(ddsg_node_index);
		node_index=(struct coord *)file_data_read(ddsg_node_index, 0, file_size(ddsg_node_index));
	}
}

static void
ch_create_tempfiles(char *suffix, FILE **files, int count, int mode)
{	
	char name[256];
	int i;

	for (i = 0 ; i <= count ; i++) {
		sprintf(name,"graph_%d",i);
		files[i]=tempfile(suffix, name, mode);
	}
}

static void
ch_close_tempfiles(FILE **files, int count)
{
	int i;

	for (i = 0 ; i <= count ; i++) {
		fclose(files[i]);
	}
}

static void
ch_remove_tempfiles(char *suffix, int count)
{
	char name[256];
	int i;

	for (i = 0 ; i <= count ; i++) {
		sprintf(name,"graph_%d",i);
		tempfile_unlink(suffix, name);
	}
}

static void
ch_copy_to_tiles(char *suffix, int count, struct tile_info *info, FILE *ref)
{
	char name[256];
	int i;
	FILE *f;
	struct item_bin *item_bin;

	for (i = count ; i >= 0 ; i--) {
		sprintf(name,"graph_%d",i);
		f=tempfile(suffix, name, 0);
		while ((item_bin = read_item(f))) {
			tile_write_item_minmax(info, item_bin, ref, i, i);
		}
		fclose(f);
	}
}

void
ch_generate_tiles(char *map_suffix, char *suffix, FILE *tilesdir_out, struct zip_info *zip_info)
{
	struct tile_info info;
	FILE *in,*ref,*ddsg_coords,*ddsg;
        info.write=0;
        info.maxlen=0;
        info.suffix=suffix;
        info.tiles_list=NULL;
        info.tilesdir_out=tilesdir_out;
	FILE *graphfiles[ch_levels+1];

	ch_create_tempfiles(suffix, graphfiles, ch_levels, 1);
	in=tempfile(map_suffix,"ways_split",0);
	ref=tempfile(map_suffix,"ways_split_ref",0);
	ddsg_coords=tempfile(suffix,"ddsg_coords",1);
	ddsg=tempfile(suffix,"ddsg",1);
	ch_generate_ddsg(in, ref, ddsg_coords, ddsg);
	fclose(in);
	fclose(ref);
	fclose(ddsg_coords);
	fclose(ddsg);
	ch_generate_sgr(suffix);
	ch_setup(suffix);
	ch_process(graphfiles, ch_levels, 0);
	ch_close_tempfiles(graphfiles, ch_levels);

	tile_hash=g_hash_table_new(g_str_hash, g_str_equal);
	ch_copy_to_tiles(suffix, ch_levels, &info, NULL);
	merge_tiles(&info);

	write_tilesdir(&info, zip_info, tilesdir_out);
}

void
ch_assemble_map(char *map_suffix, char *suffix, struct zip_info *zip_info)
{
	struct tile_info info;
	struct tile_head *th;
	FILE *graphfiles[ch_levels+1];

        info.write=1;
        info.maxlen=zip_info->maxnamelen;
        info.suffix=suffix;
        info.tiles_list=NULL;
        info.tilesdir_out=NULL;
	FILE *ref=tempfile(suffix,"sgr_ref",1);
	struct item_id id;
	int nodeid=0;
	
	create_tile_hash();
	th=tile_head_root;
        while (th) {
		th->zip_data=malloc(th->total_size);
		th->process=1;
                th=th->next;
        }

	ch_setup(suffix);
	ch_copy_to_tiles(suffix, ch_levels, &info, ref);
	fclose(ref);
	ref=tempfile(suffix,"sgr_ref",0);
	sgr_nodes_hash=g_hash_table_new_full(NULL, NULL, NULL, g_free);
	while (fread(&id, sizeof(id), 1, ref)) {
		struct item_id *id2=g_new(struct item_id, 1);
		*id2=id;
#if 0
		dbg(0,"%d is "ITEM_ID_FMT"\n",nodeid,ITEM_ID_ARGS(*id2));
#endif
		g_hash_table_insert(sgr_nodes_hash, GINT_TO_POINTER(nodeid), id2);
		nodeid++;
	}
	th=tile_head_root;
        while (th) {
		th->zip_data=malloc(th->total_size);
		th->total_size_used=0;
                th=th->next;
        }
	ch_create_tempfiles(suffix, graphfiles, ch_levels, 1);
	ch_process(graphfiles, ch_levels, 1);
	ch_close_tempfiles(graphfiles, ch_levels);

	g_hash_table_destroy(newnode_hash);
	g_hash_table_destroy(edge_hash);
	g_hash_table_destroy(sgr_nodes_hash);

	ch_copy_to_tiles(suffix, ch_levels, &info, NULL);
	write_tilesdir(&info, zip_info, NULL);

	th=tile_head_root;
        while (th) {
		if (th->name[0]) {
			if (th->total_size != th->total_size_used) {
				fprintf(stderr,"Size error '%s': %d vs %d\n", th->name, th->total_size, th->total_size_used);
				exit(1);
			}
			write_zipmember(zip_info, th->name, zip_info->maxnamelen, th->zip_data, th->total_size);
		} else {
			fwrite(th->zip_data, th->total_size, 1, zip_info->index);
		}
                th=th->next;
        }
}
