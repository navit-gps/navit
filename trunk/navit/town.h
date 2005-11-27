struct town {
	unsigned long id;
	struct coord *c;
	char *name;
	char *district;
	char *postal_code1;
	unsigned char order;
	unsigned char type;
	unsigned short country;
	unsigned long unknown2;
	unsigned char size;
	unsigned long street_assoc;
	unsigned char unknown3;
	char *postal_code2;
	unsigned long unknown4;
};

struct block_info;
struct segment;
struct container;
struct param_list;
struct map_data;

void town_draw_block(struct block_info *blk_inf, unsigned char *start, unsigned char *end, void *data);
int town_get_param(struct segment *seg, struct param_list *param, int count);
int town_search_by_name(struct map_data *mdat, int country, const char *name, int partial, int (*func)(struct town *t, void *data), void *data);
void town_get_by_id(struct town *town, struct map_data *mdat, int country, int id);
