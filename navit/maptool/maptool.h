/*
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
#include "config.h"
#include <glib.h>
#include "coord.h"
#include "item.h"
#include "attr.h"
#include "geom.h"
#include "types.h"

#define sq(x) ((double)(x)*(x))

#define BUFFER_SIZE 1280

#define debug_tile(x) 0
#define debug_itembin(x) 0

#define RELATION_MEMBER_PRINT_FORMAT "%d:"LONGLONG_FMT":%s"
#define RELATION_MEMBER_PARSE_FORMAT "%d:"LONGLONG_FMT":%n"

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
    enum attr_type attr_to_copy;
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


/**
 * A map item (street, POI, border etc.) as it is stored in a Navit binfile.
 * Note that this struct only has fields for the header of the item. The
 * actual data (coordinates and attributes) is stored in memory after
 * this struct as two arrays of type struct coord and struct attr_bin
 * respectively.
 * See also http://wiki.navit-project.org/index.php/Navit%27s_binary_map_driver .
 * @see struct coord
 * @see struct attr_bin
 */
struct item_bin {
    /** Length of this item (not including this length field) in 32-bit ints. */
    int len;
    /** Item type. */
    enum item_type type;
    /** Length of the following coordinate array in 32-bit ints. */
    int clen;
};

/**
 * An attribute for an item_bin as it is stored in a Navit binfile.
 * Note that this struct only has fields for the header of the attribute.
 * The attribute value is stored in memory after this struct. The type of the value
 * (string, number, ...) depends on the attribute type.
 * @see struct item_bin
 */
struct attr_bin {
    /** Length of this attribute (not including this length field) in 32-bit ints. */
    int len;
    /** Attribute type. */
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
#define NODE_ID_BITS 56
struct node_item {
    struct coord c;
unsigned long long int nd_id:
    NODE_ID_BITS;
    char ref_way;
};

struct zip_info;

struct country_table;

/**
 * Data type for the ID of an OSM element (node/way/relation).
 * Must be at least 64 bit wide because IDs will soon exceed 32 bit.
 */
typedef unsigned long long int osmid;
#define OSMID_FMT ULONGLONG_FMT

/** Files needed for processing a relation. */
struct files_relation_processing {
    FILE *ways_in;
    FILE *ways_out;
    FILE *nodes_in;
    FILE *nodes_out;
    FILE *nodes2_in;
    FILE *nodes2_out;
};

/* boundaries.c */

struct boundary {
    struct item_bin *ib;
    struct country_table *country;
    char *iso2;
    GList *segments,*sorted_segments;
    GList *children;
    struct rect r;
    osmid admin_centre;
};

char *osm_tag_value(struct item_bin *ib, char *key);

osmid boundary_relid(struct boundary *b);

GList *process_boundaries(FILE *boundaries, FILE *ways);

GList *boundary_find_matches(GList *bl, struct coord *c);

void free_boundaries(GList *l);

/* buffer.c */

/** A buffer that can be grown as needed. */
struct buffer {
    /** Number of bytes to extend the buffer by when it must grow. */
    int malloced_step;
    /** Current allocated size (bytes). */
    long long malloced;
    /** Base address of this buffer. */
    unsigned char *base;
    /** Size of currently used part of the buffer. */
    long long size;
};

void save_buffer(char *filename, struct buffer *b, long long offset);
int load_buffer(char *filename, struct buffer *b, long long offset, long long size);
long long sizeof_buffer(char *filename);

/* ch.c */

void ch_generate_tiles(char *map_suffix, char *suffix, FILE *tilesdir_out, struct zip_info *zip_info);
void ch_assemble_map(char *map_suffix, char *suffix, struct zip_info *zip_info);

/* coastline.c */

void process_coastlines(FILE *in, FILE *out);

/* itembin.c */

int item_bin_read(struct item_bin *ib, FILE *in);
void item_bin_set_type(struct item_bin *ib, enum item_type type);
void item_bin_init(struct item_bin *ib, enum item_type type);
void item_bin_add_coord(struct item_bin *ib, struct coord *c, int count);
void item_bin_add_coord_reverse(struct item_bin *ib, struct coord *c, int count);
void item_bin_bbox(struct item_bin *ib, struct rect *r);
void item_bin_copy_coord(struct item_bin *ib, struct item_bin *from, int dir);
void item_bin_copy_attr(struct item_bin *ib, struct item_bin *from, enum attr_type attr);
void item_bin_add_coord_rect(struct item_bin *ib, struct rect *r);
int attr_bin_write_data(struct attr_bin *ab, enum attr_type type, void *data, int size);
int attr_bin_write_attr(struct attr_bin *ab, struct attr *attr);
void item_bin_add_attr_data(struct item_bin *ib, enum attr_type type, void *data, int size);
void item_bin_add_attr(struct item_bin *ib, struct attr *attr);
void item_bin_add_attr_int(struct item_bin *ib, enum attr_type type, int val);
void *item_bin_get_attr(struct item_bin *ib, enum attr_type type, void *last);
struct attr_bin * item_bin_get_attr_bin(struct item_bin *ib, enum attr_type type, void *last);
struct attr_bin * item_bin_get_attr_bin_last(struct item_bin *ib);
void item_bin_add_attr_longlong(struct item_bin *ib, enum attr_type type, long long val);
void item_bin_add_attr_string(struct item_bin *ib, enum attr_type type, char *str);
void item_bin_add_hole(struct item_bin * ib, struct coord * coord, int ccount);
void item_bin_add_attr_range(struct item_bin *ib, enum attr_type type, short min, short max);
void item_bin_remove_attr(struct item_bin *ib, void *ptr);
void item_bin_write(struct item_bin *ib, FILE *out);
struct item_bin *item_bin_dup(struct item_bin *ib);
void item_bin_write_clipped(struct item_bin *ib, struct tile_parameter *param, struct item_bin_sink *out);
void item_bin_dump(struct item_bin *ib, FILE *out);
void dump_itembin(struct item_bin *ib);
void item_bin_set_type_by_population(struct item_bin *ib, int population);
void item_bin_write_match(struct item_bin *ib, enum attr_type type, enum attr_type match, int maxdepth, FILE *out);
int item_bin_sort_file(char *in_file, char *out_file, struct rect *r, int *size);
void clip_line(struct item_bin *ib, struct rect *r, struct tile_parameter *param, struct item_bin_sink *out);
void clip_polygon(struct item_bin *ib, struct rect *r, struct tile_parameter *param, struct item_bin_sink *out);
struct geom_poly_segment *item_bin_to_poly_segment(struct item_bin *ib, int type);

/* itembin_buffer.c */
struct node_item *read_node_item(FILE *in);
struct item_bin *read_item(FILE *in);
struct item_bin *read_item_range(FILE *in, int *min, int *max);
struct item_bin *init_item(enum item_type type);
extern struct item_bin *tmp_item_bin;

/* itembin_slicer.c */
void itembin_nicer_slicer(struct tile_info *info, struct item_bin *ib, FILE *reference, char * buffer, int min);


/* maptool.c */

extern long long slice_size;
extern int thread_count;
extern int attr_debug_level;
extern char *suffix;
extern int ignore_unknown;
extern GHashTable *dedupe_ways_hash;
extern int slices;
extern struct buffer node_buffer;
extern int processed_nodes, processed_nodes_out, processed_ways, processed_relations, processed_tiles;
extern int bytes_read;
extern int overlap;
extern int unknown_country;
extern int experimental;
void sig_alrm(int sig);
void sig_alrm_end(void);

/* misc.c */
extern struct rect world_bbox;


void bbox_extend(struct coord *c, struct rect *r);
void bbox(struct coord *c, int count, struct rect *r);
int contains_bbox(int xl, int yl, int xh, int yh, struct rect *r);
int bbox_contains_coord(struct rect *r, struct coord *c);
int bbox_contains_bbox(struct rect *out, struct rect *in);
long long bbox_area(struct rect const *r);
void phase1_map(GList *maps, FILE *out_ways, FILE *out_nodes);
void dump(FILE *in);
int phase4(FILE **in, int in_count, int with_range, char *suffix, FILE *tilesdir_out, struct zip_info *zip_info);
int phase5(FILE **in, FILE **references, int in_count, int with_range, char *suffix, struct zip_info *zip_info);
void process_binfile(FILE *in, FILE *out);
void add_aux_tiles(char *name, struct zip_info *info);
void cat(FILE *in, FILE *out);
int item_order_by_type(enum item_type type);


/* osm.c */
struct maptool_osm {
    FILE *boundaries;
    FILE *multipolygons;
    FILE *turn_restrictions;
    FILE *associated_streets;
    FILE *house_number_interpolations;
    FILE *nodes;
    FILE *ways;
    FILE *line2poi;
    FILE *poly2poi;
    FILE *towns;
};

/** Type of a relation member. */
enum relation_member_type {
    UNUSED,
    rel_member_node,
    rel_member_way,
    rel_member_relation,
};

void osm_warning(char *type, osmid id, int cont, char *fmt, ...);
void osm_info(char *type, osmid id, int cont, char *fmt, ...);
void osm_add_tag(char *k, char *v);
void osm_add_node(osmid id, double lat, double lon);
void osm_add_way(osmid id);
void osm_add_relation(osmid id);
void osm_end_relation(struct maptool_osm *osm);
void osm_add_member(enum relation_member_type type, osmid ref, char *role);
void osm_end_way(struct maptool_osm *osm);
void osm_end_node(struct maptool_osm *osm);
void osm_add_nd(osmid ref);
osmid item_bin_get_id(struct item_bin *ib);
void flush_nodes(int final);
void sort_countries(int keep_tmpfiles);
void process_associated_streets(FILE *in, struct files_relation_processing *files_relproc);
void process_house_number_interpolations(FILE *in, struct files_relation_processing *files_relproc);
void process_multipolygons(FILE *in, FILE *coords, FILE *ways, FILE *ways_index, FILE *out);
void process_turn_restrictions(FILE *in, FILE *coords, FILE *ways, FILE *ways_index, FILE *out);
void process_turn_restrictions_old(FILE *in, FILE *coords, FILE *ways, FILE *ways_index, FILE *out);
void clear_node_item_buffer(void);
void ref_ways(FILE *in);
void resolve_ways(FILE *in, FILE *out);
unsigned long long item_bin_get_nodeid(struct item_bin *ib);
unsigned long long item_bin_get_wayid(struct item_bin *ib);
unsigned long long item_bin_get_relationid(struct item_bin *ib);
void process_way2poi(FILE *in, FILE *out, int type);
int map_resolve_coords_and_split_at_intersections(FILE *in, FILE *out, FILE *out_index, FILE *out_graph,
        FILE *out_coastline, int final);
void write_countrydir(struct zip_info *zip_info, int max_index_size);
void osm_process_towns(FILE *in, FILE *boundaries, FILE *ways, char *suffix);
void load_countries(void);
void remove_countryfiles(void);
struct country_table * country_from_iso2(char *iso);
void osm_init(FILE*);

/* osm_o5m.c */
int map_collect_data_osm_o5m(FILE *in, struct maptool_osm *osm);

/* osm_psql.c */
int map_collect_data_osm_db(char *dbstr, struct maptool_osm *osm);

/* osm_protobuf.c */
int map_collect_data_osm_protobuf(FILE *in, struct maptool_osm *osm);
int osm_protobufdb_load(FILE *in, char *dir);

/* osm_relations.c */
struct relations * relations_new(void);
struct relations_func *relations_func_new(void (*func)(void *func_priv, void *relation_priv, struct item_bin *member,
        void *member_priv), void *func_priv);
void relations_add_relation_member_entry(struct relations *rel, struct relations_func *func, void *relation_priv,
        void *member_priv, enum relation_member_type type, osmid id);
void relations_add_relation_default_entry(struct relations *rel, struct relations_func *func);
void relations_process(struct relations *rel, FILE *nodes, FILE *ways);
void relations_process_multi(struct relations **rel, int count, FILE *nodes, FILE *ways);
void relations_destroy(struct relations *rel);


/* osm_xml.c */
int osm_xml_get_attribute(char *xml, char *attribute, char *buffer, int buffer_size);
void osm_xml_decode_entities(char *buffer);
int map_collect_data_osm(FILE *in, struct maptool_osm *osm);


/* sourcesink.c */

struct item_bin_sink *item_bin_sink_new(void);
struct item_bin_sink_func *item_bin_sink_func_new(int (*func)(struct item_bin_sink_func *func, struct item_bin *ib,
        struct tile_data *tile_data));
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
void load_tilesdir(FILE *in);
void tile_write_item_to_tile(struct tile_info *info, struct item_bin *ib, FILE *reference, char *name);
void tile_write_item_minmax(struct tile_info *info, struct item_bin *ib, FILE *reference, int min, int max);
int add_aux_tile(struct zip_info *zip_info, char *name, char *filename, int size);
int write_aux_tiles(struct zip_info *zip_info);
int create_tile_hash(void);
void write_tilesdir(struct tile_info *info, struct zip_info *zip_info, FILE *out);
void merge_tiles(struct tile_info *info);
extern struct attr map_information_attrs[32];
void index_init(struct zip_info *info, int version);
void index_submap_add(struct tile_info *info, struct tile_head *th);

/* zip.c */
void write_zipmember(struct zip_info *zip_info, char *name, int filelen, char *data, int data_size);
int zip_write_index(struct zip_info *info);
int zip_write_directory(struct zip_info *info);
struct zip_info *zip_new(void);
void zip_set_zip64(struct zip_info *info, int on);
void zip_set_compression_level(struct zip_info *info, int level);
void zip_set_maxnamelen(struct zip_info *info, int max);
int zip_get_maxnamelen(struct zip_info *info);
int zip_add_member(struct zip_info *info);
int zip_set_timestamp(struct zip_info *info, char *timestamp);
int zip_open(struct zip_info *info, char *out, char *dir, char *index);
FILE *zip_get_index(struct zip_info *info);
int zip_get_zipnum(struct zip_info *info);
void zip_set_zipnum(struct zip_info *info, int num);
void zip_close(struct zip_info *info);
void zip_destroy(struct zip_info *info);

/* osm.c */
int process_multipolygons_find_loops(osmid relid, int in_count, struct item_bin ** parts, int **scount,
                                     int *** sequences,
                                     int **direction);
int process_multipolygons_loop_dump(struct item_bin** bin, int scount, int*sequence, int*direction,
                                    struct coord *  buffer);
int process_multipolygons_loop_count(struct item_bin** bin, int scount, int*sequence);

/* Break compilation on 32 bit architectures, as we're going to cast osmid's to gpointer to use them as keys to GHashTable's */
struct maptool_force_64 {
    char s[sizeof(gpointer)<sizeof(osmid)?-1:1];
};
