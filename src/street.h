#include "types.h"

struct container;
struct block_info;
struct segment;
struct transformation;
struct street_coord_handle;

struct street_header {
	unsigned char order;
	int count;
} __attribute__((packed));

struct street_type {
	unsigned char order;
	unsigned short country;
} __attribute__((packed));

struct street_str {
        s32 segid;
        u8  limit;           	/* 0x03,0x30=One Way,0x33=No Passing */
        u8  unknown2;
        u8  unknown3;
        u8  type;
        u32 nameid;
};

struct street_bti {
	u8  unknown1;
	s32 segid1;
	u32 country1;
	s32 segid2;
	u32 country2;
	u8  unknown5;
	struct coord c;
} __attribute__((packed));

struct street_route {
	struct street_route *next;
	s32 segid;
	int offset;
	struct coord c[2];
};

struct street_coord {
	int count;
	struct coord c[0];
};


void street_draw_block(struct block_info *blk_inf, unsigned char *start, unsigned char *end, void *data);
struct street_coord *street_coord_get(struct block_info *blk_inf, struct street_str *str);
struct street_coord_handle * street_coord_handle_new(struct block_info *blk_inf, struct street_str *str);
void street_coord_handle_rewind(struct street_coord_handle *h);
int street_coord_handle_get(struct street_coord_handle *h, struct coord *c, int count);
void street_coord_handle_destroy(struct street_coord_handle *handle);
void street_get_block(struct map_data *mdata, struct transformation *t, void (*callback)(void *data), void *data);
int street_get_by_id(struct map_data *mdat, int country, int id, struct block_info *res_blk_inf, struct street_str **res_str);
void street_bti_draw_block(struct block_info *blk_inf, unsigned char *start, unsigned char *end, void *data);
int street_get_param(struct segment *seg, struct param_list *param, int count, int verbose);
int street_bti_get_param(struct segment *seg, struct param_list *param, int count);
void street_route_draw(struct container *co);

