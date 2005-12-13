/*! A town description */
struct town {
	unsigned long id; /*!< Identifier */
	struct coord *c; /*!< Coordinates */
	char *name; /*!< Name */
	char *district; /*!< District */
	char *postal_code1; /*!< Postal code */
	unsigned char order; /*!< Order (Importance) */
	unsigned char type; /*!< Type */
	unsigned short country; /*!< Country */
	unsigned long unknown2; /*!< Unknown */
	unsigned char size; /*!< Size of town */
	unsigned long street_assoc; /*!< Association to streets */
	unsigned char unknown3; /*!< Unknown */
	char *postal_code2; /*!< 2nd postal code */
	unsigned long unknown4; /*!< Unknown */
};

struct block_info;
struct segment;
struct container;
struct param_list;
struct map_data;

void town_draw_block(struct block_info *blk_inf, unsigned char *start, unsigned char *end, void *data);
int town_get_param(struct segment *seg, struct param_list *param, int count);
int town_search_by_postal_code(struct map_data *mdat, int country, const char *name, int partial, int (*func)(struct town *, void *data), void *data);
int town_search_by_name(struct map_data *mdat, int country, const char *name, int partial, int (*func)(struct town *, void *data), void *data);
int town_search_by_district(struct map_data *mdat, int country, const char *name, int partial, int (*func)(struct town *, void *data), void *data);
int town_search_by_name_phon(struct map_data *mdat, int country, const char *name, int partial, int (*func)(struct town *, void *data), void *data);
int town_search_by_district_phon(struct map_data *mdat, int country, const char *name, int partial, int (*func)(struct town *, void *data), void *data);
void town_get_by_id(struct town *town, struct map_data *mdat, int country, int id);
