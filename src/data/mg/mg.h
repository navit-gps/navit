#include <glib.h>
#include "attr.h"
#include "coord.h"
#include "data.h"
#include "item.h"
#include "map.h"
#include "file.h"

struct block_data {
	struct file *file;
};

struct block {
	int blocks;
	int size;
	int next;
	struct coord_rect r;
	int count;
};

struct item_priv {
	int cidx;
	int aidx;
	unsigned char *cstart,*cp,*cend;
	unsigned char *astart,*ap,*aend;
	enum attr_type attr_last;
	enum attr_type attr_next;
	struct item item;
};

struct town_priv {
	unsigned int id; /*!< Identifier */
	struct coord c; /*!< Coordinates */
	char *name; /*!< Name */
	char *district; /*!< District */
	char *postal_code1; /*!< Postal code */
	unsigned char order; /*!< Order (Importance) */
	unsigned char type; /*!< Type */
	unsigned short country; /*!< Country */
	unsigned int unknown2; /*!< Unknown */
	unsigned char size; /*!< Size of town */
	unsigned int street_assoc; /*!< Association to streets */
	unsigned char unknown3; /*!< Unknown */
	char *postal_code2; /*!< 2nd postal code */
	unsigned int unknown4; /*!< Unknown */

	int cidx;
	int aidx;
	enum attr_type attr_next;
	char debug[256];
	struct item town_attr_item;
};

struct poly_priv {
	int poly_num;
	unsigned char *poly_next;
	int subpoly_num;
	int subpoly_num_all;
	unsigned char *subpoly_next;
	unsigned char *subpoly_start;
	unsigned char *p;
	struct coord c[2];
	char *name;
	unsigned char order;
	unsigned char type;
	unsigned int polys;
	unsigned int *count;
	unsigned int count_sum;

	int aidx;
	enum attr_type attr_next;
};

struct street_header {
        unsigned char order;
        int count;
} __attribute__((packed));

struct street_type {
        unsigned char order;
        unsigned short country;
} __attribute__((packed));

struct street_header_type {
	struct street_header *header;
	int type_count;
	struct street_type *type;
};

struct street_str {
        int segid;
        unsigned char limit;            /* 0x03,0x30=One Way,0x33=No Passing */
        unsigned char unknown2;
        unsigned char unknown3;
        unsigned char type;
        unsigned int nameid;
};

struct street_name_segment {
	int segid;
	int country;
};

struct street_name {
	int len;
	int country;
	int townassoc;
	char *name1;
	char *name2;
	int segment_count;
	struct street_name_segment *segments;
	int aux_len;
	unsigned char *aux_data;
	int tmp_len;
	unsigned char *tmp_data;
};

struct street_name_numbers {
	int len;
	int tag;
	int dist;
	int country;
	struct coord *c;
	int first;
	int last;
	int segment_count;
	struct street_name_segment *segments;
	int aux_len;
	unsigned char *aux_data;
	int tmp_len;
	unsigned char *tmp_data;
};

struct street_name_number {
        int len;
        int tag;
        struct coord *c;
        int first;
        int last;
        struct street_name_segment *segment;
};



struct street_priv {
	struct file *name_file;
	struct street_header *header;
	int type_count;
	struct street_type *type;
	struct street_str *str;
	struct street_str *str_start;
	unsigned char *coord_begin;
	unsigned char *p;
	unsigned char *p_rewind;
	unsigned char *end;
	unsigned char *next;
	int status;
	int status_rewind;
	struct coord *ref;
	int bytes;
	struct street_name name;
	enum attr_type attr_next;
	char debug[256];
};

enum file_index {
        file_border_ply=0,
        file_bridge_ply,
	file_build_ply,
	file_golf_ply,
        file_height_ply,
	file_natpark_ply,
	file_nature_ply,
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

struct map_priv {
	int id;
	struct file *file[file_end];
	char *dirname;
};

#define BT_STACK_SIZE 32

struct block_bt_priv {
	struct block *b;
	struct coord_rect r, r_curr;
	int next;
	int block_count;
	struct coord_rect stack[BT_STACK_SIZE];
	int stackp;
	int order;
	unsigned char *p;
	unsigned char *end;
};

struct block_priv {
	int block_num;
	struct coord_rect b_rect;
	unsigned char *block_start;
	struct block *b;
	unsigned char *p;
	unsigned char *end;
	unsigned char *p_start;
	int binarytree;
	struct block_bt_priv bt;
};

struct block_offset {
	unsigned short offset;
	unsigned short block;
};


struct tree_search_node {
	struct tree_hdr *hdr;
	unsigned char *p;
	unsigned char *last;
	unsigned char *end;
	int low;
	int high;
	int last_low;
	int last_high;
	};

struct tree_search {
	struct file *f;
	int last_node;
	int curr_node;
	struct tree_search_node nodes[5];
};


struct map_rect_priv {
	struct map_selection *xsel;
	struct map_selection *cur_sel;

	struct map_priv *m;
	enum file_index current_file;
	struct file *file;
	struct block_priv b;
	struct item item;
	struct town_priv town;
	struct poly_priv poly;
	struct street_priv street;
	struct tree_search ts;
	int search_country;
	struct item search_item;
	char *search_str;
	int search_partial;
	int search_linear;
	unsigned char *search_p;
	int search_blk_count;
	enum attr_type search_type;
	struct block_offset *search_blk_off;
	int search_block;
	GHashTable *block_hash[file_end];
	struct item_priv item3;
};

int block_init(struct map_rect_priv *mr);
int block_next(struct map_rect_priv *mr);
int block_get_byindex(struct file *file, int idx, struct block_priv *blk);
int block_next_lin(struct map_rect_priv *mr);

int tree_search_hv(char *dirname, char *filename, unsigned int search1, unsigned int search2, int *result);
int town_get(struct map_rect_priv *mr, struct town_priv *poly, struct item *item);
int town_get_byid(struct map_rect_priv *mr, struct town_priv *twn, int id_hi, int id_lo, struct item *item);
struct item * town_search_get_item(struct map_rect_priv *mr);
int poly_get(struct map_rect_priv *mr, struct poly_priv *poly, struct item *item);
int poly_get_byid(struct map_rect_priv *mr, struct poly_priv *poly, int id_hi, int id_lo, struct item *item);
int street_get(struct map_rect_priv *mr, struct street_priv *street, struct item *item);
int street_get_byid(struct map_rect_priv *mr, struct street_priv *street, int id_hi, int id_lo, struct item *item);
struct item * street_search_get_item(struct map_rect_priv *mr);
void tree_search_init(char *dirname, char *filename, struct tree_search *ts, int offset);
void tree_search_free(struct tree_search *ts);
int tree_search_next(struct tree_search *ts, unsigned char **p, int dir);
int tree_search_next_lin(struct tree_search *ts, unsigned char **p);
