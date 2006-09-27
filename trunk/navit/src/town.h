#include "types.h"

/*! A town description */
struct town {
	u32 id; /*!< Identifier */
	struct coord *c; /*!< Coordinates */
	char *name; /*!< Name */
	char *district; /*!< District */
	char *postal_code1; /*!< Postal code */
	u8  order; /*!< Order (Importance) */
	u8  type; /*!< Type */
	u16 country; /*!< Country */
	u32 unknown2; /*!< Unknown */
	u8  size; /*!< Size of town */
	u32 street_assoc; /*!< Association to streets */
	u8  unknown3; /*!< Unknown */
	char *postal_code2; /*!< 2nd postal code */
	u32 unknown4; /*!< Unknown */
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
