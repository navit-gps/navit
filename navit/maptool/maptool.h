#include <glib.h>
#include "config.h"
#include "coord.h"
#include "item.h"
#include "attr.h"

#ifdef HAVE_API_WIN32_BASE
#define LONGLONG_FMT "%I64d"
#else
#define LONGLONG_FMT "%Ld"
#endif

#define BUFFER_SIZE 1280

#define debug_tile(x) 0
#define debug_itembin(x) 0

struct rect {
	struct coord l,h;
};

struct tile_data {
	char buffer[1024];
	int tile_depth;
	struct rect item_bbox;
	struct rect tile_bbox;
};

struct tile_parameter {
	int min;
	int max;
	int overlap;
};

struct tile_info {
	int write;
	int maxlen;
	char *suffix;
	GList **tiles_list;
	FILE *tilesdir_out;
};

extern struct tile_head {
	int num_subtiles;
	int total_size;
	char *name;
	char *zip_data;
	int total_size_used;
	int zipnum;
	int process;
	struct tile_head *next;
	// char subtiles[0];
} *tile_head_root;



struct item_bin {
	int len;
	enum item_type type;
	int clen;
};

struct attr_bin {
	int len;
	enum attr_type type;
};


struct item_bin_sink_func {
	int (*func)(struct item_bin_sink_func *func, struct item_bin *ib, struct tile_data *tile_data);
	void *priv_data[8];
};

struct item_bin_sink {
	void *priv_data[8];
	GList *sink_funcs;
};

struct zip_info {
	int zipnum;
	int dir_size;
	long long offset;
	int compression_level;
	int maxnamelen;
	int zip64;
	short date;
	short time;
	char *passwd;
	FILE *res;
	FILE *index;
	FILE *dir;
};

/* buffer.c */
struct buffer {
	int malloced_step;
	long long malloced;
	unsigned char *base;
	long long size;
};

void save_buffer(char *filename, struct buffer *b, long long offset);
void load_buffer(char *filename, struct buffer *b, long long offset, long long size);

/* ch.c */

void ch_generate_tiles(char *map_suffix, char *suffix, FILE *tilesdir_out, struct zip_info *zip_info);
void ch_assemble_map(char *map_suffix, char *suffix, struct zip_info *zip_info);

/* coastline.c */

void process_coastlines(FILE *in, FILE *out);

/* geom.c */

enum geom_poly_segment_type {
	geom_poly_segment_type_none,
	geom_poly_segment_type_way_inner,
	geom_poly_segment_type_way_outer,
	geom_poly_segment_type_way_left_side,
	geom_poly_segment_type_way_right_side,
	geom_poly_segment_type_way_unknown,

};

struct geom_poly_segment {
	enum geom_poly_segment_type type;
	struct coord *first,*last;
};

void geom_coord_copy(struct coord *from, struct coord *to, int count, int reverse);
void geom_coord_revert(struct coord *c, int count);
long long geom_poly_area(struct coord *c, int count);
GList *geom_poly_segments_insert(GList *list, struct geom_poly_segment *first, struct geom_poly_segment *second, struct geom_poly_segment *third);
void geom_poly_segment_destroy(struct geom_poly_segment *seg);
GList *geom_poly_segments_remove(GList *list, struct geom_poly_segment *seg);
int geom_poly_segment_compatible(struct geom_poly_segment *s1, struct geom_poly_segment *s2, int dir);
GList *geom_poly_segments_sort(GList *in, enum geom_poly_segment_type type);
struct geom_poly_segment *item_bin_to_poly_segment(struct item_bin *ib, int type);
void clip_line(struct item_bin *ib, struct rect *r, struct tile_parameter *param, struct item_bin_sink *out);
void clip_polygon(struct item_bin *ib, struct rect *r, struct tile_parameter *param, struct item_bin_sink *out);

/* itembin.c */

int item_bin_read(struct item_bin *ib, FILE *in);
void item_bin_set_type(struct item_bin *ib, enum item_type type);
void item_bin_init(struct item_bin *ib, enum item_type type);
void item_bin_add_coord(struct item_bin *ib, struct coord *c, int count);
void item_bin_bbox(struct item_bin *ib, struct rect *r);
void item_bin_copy_coord(struct item_bin *ib, struct item_bin *from, int dir);
void item_bin_add_coord_rect(struct item_bin *ib, struct rect *r);
int attr_bin_write_data(struct attr_bin *ab, enum attr_type type, void *data, int size);
int attr_bin_write_attr(struct attr_bin *ab, struct attr *attr);
void item_bin_add_attr_data(struct item_bin *ib, enum attr_type type, void *data, int size);
void item_bin_add_attr(struct item_bin *ib, struct attr *attr);
void item_bin_add_attr_int(struct item_bin *ib, enum attr_type type, int val);
void *item_bin_get_attr(struct item_bin *ib, enum attr_type type, void *last);
struct attr_bin * item_bin_get_attr_bin_last(struct item_bin *ib);
void item_bin_add_attr_longlong(struct item_bin *ib, enum attr_type type, long long val);
void item_bin_add_attr_string(struct item_bin *ib, enum attr_type type, char *str);
void item_bin_add_attr_range(struct item_bin *ib, enum attr_type type, short min, short max);
void item_bin_write(struct item_bin *ib, FILE *out);
void item_bin_write_range(struct item_bin *ib, FILE *out, int min, int max);
void item_bin_write_clipped(struct item_bin *ib, struct tile_parameter *param, struct item_bin_sink *out);
void item_bin_dump(struct item_bin *ib, FILE *out);
void dump_itembin(struct item_bin *ib);
void item_bin_set_type_by_population(struct item_bin *ib, int population);
void item_bin_write_match(struct item_bin *ib, enum attr_type type, enum attr_type match, FILE *out);
int item_bin_sort_file(char *in_file, char *out_file, struct rect *r, int *size);

/* itembin_buffer.c */
struct item_bin *read_item(FILE *in);
struct item_bin *read_item_range(FILE *in, int *min, int *max);
struct item_bin *init_item(enum item_type type);

/* maptool.c */

extern long long slice_size;
extern int attr_debug_level;
extern char *suffix;
extern int ignore_unkown;
extern GHashTable *dedupe_ways_hash;
extern int phase;
extern int slices;
extern struct buffer node_buffer;
extern int processed_nodes, processed_nodes_out, processed_ways, processed_relations, processed_tiles;
extern struct item_bin *item_bin;
extern int bytes_read;
extern int overlap;
void sig_alrm(int sig);
void sig_alrm_end(void);

/* misc.c */
extern struct rect world_bbox;


void bbox_extend(struct coord *c, struct rect *r);
void bbox(struct coord *c, int count, struct rect *r);
int contains_bbox(int xl, int yl, int xh, int yh, struct rect *r);
void phase1_map(GList *maps, FILE *out_ways, FILE *out_nodes);
void dump(FILE *in);
int phase4(FILE **in, int in_count, int with_range, char *suffix, FILE *tilesdir_out, struct zip_info *zip_info);
int phase5(FILE **in, FILE **references, int in_count, int with_range, char *suffix, struct zip_info *zip_info);
void process_binfile(FILE *in, FILE *out);
void add_aux_tiles(char *name, struct zip_info *info);
void cat(FILE *in, FILE *out);


/* osm.c */

long long item_bin_get_id(struct item_bin *ib);
void flush_nodes(int final);
void sort_countries(int keep_tmpfiles);
void process_turn_restrictions(FILE *in, FILE *coords, FILE *ways, FILE *ways_index, FILE *out);
int resolve_ways(FILE *in, FILE *out);
int map_collect_data_osm(FILE *in, FILE *out_ways, FILE *out_nodes, FILE *out_turn_restrictions);
int map_collect_data_osm_db(char *dbstr, FILE *out_ways, FILE *out_nodes);
int map_find_intersections(FILE *in, FILE *out, FILE *out_index, FILE *out_graph, FILE *out_coastline, int final);
void write_countrydir(struct zip_info *zip_info);
void remove_countryfiles(void);
void osm_init(void);

/* sourcesink.c */

struct item_bin_sink *item_bin_sink_new(void);
struct item_bin_sink_func *item_bin_sink_func_new(int (*func)(struct item_bin_sink_func *func, struct item_bin *ib, struct tile_data *tile_data));
void item_bin_sink_func_destroy(struct item_bin_sink_func *func);
void item_bin_sink_add_func(struct item_bin_sink *sink, struct item_bin_sink_func *func);
void item_bin_sink_destroy(struct item_bin_sink *sink);
int item_bin_write_to_sink(struct item_bin *ib, struct item_bin_sink *sink, struct tile_data *tile_data);
struct item_bin_sink *file_reader_new(FILE *in, int limit, int offset);
int file_reader_finish(struct item_bin_sink *sink);
int file_writer_process(struct item_bin_sink_func *func, struct item_bin *ib, struct tile_data *tile_data);
struct item_bin_sink_func *file_writer_new(FILE *out);
int file_writer_finish(struct item_bin_sink_func *file_writer);
int tile_collector_process(struct item_bin_sink_func *tile_collector, struct item_bin *ib, struct tile_data *tile_data);
struct item_bin_sink_func *tile_collector_new(struct item_bin_sink *out);

/* tempfile.c */

char *tempfile_name(char *suffix, char *name);
FILE *tempfile(char *suffix, char *name, int mode);
void tempfile_unlink(char *suffix, char *name);
void tempfile_rename(char *suffix, char *from, char *to);

/* tile.c */
extern GHashTable *tile_hash,*tile_hash2;

struct aux_tile {
	char *name;
	char *filename;
	int size;
};

extern GList *aux_tile_list;

int tile(struct rect *r, char *suffix, char *ret, int max, int overlap, struct rect *tr);
void tile_bbox(char *tile, struct rect *r, int overlap);
int tile_len(char *tile);
void tile_write_item_to_tile(struct tile_info *info, struct item_bin *ib, FILE *reference, char *name);
void tile_write_item_minmax(struct tile_info *info, struct item_bin *ib, FILE *reference, int min, int max);
int add_aux_tile(struct zip_info *zip_info, char *name, char *filename, int size);
int write_aux_tiles(struct zip_info *zip_info);
int create_tile_hash(void);
void write_tilesdir(struct tile_info *info, struct zip_info *zip_info, FILE *out);
void merge_tiles(struct tile_info *info);
void index_init(struct zip_info *info, int version);
void index_submap_add(struct tile_info *info, struct tile_head *th);

/* zip.c */

void write_zipmember(struct zip_info *zip_info, char *name, int filelen, char *data, int data_size);
void zip_write_index(struct zip_info *info);
int zip_write_directory(struct zip_info *info);
