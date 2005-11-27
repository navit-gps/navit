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


struct street_name_info {
	int len;
	int tag;
	int dist;
	int country;
	struct coord *c;
	int first;
	int last;
	int segment_count;
	struct street_segment *segments;
	int aux_len;
	unsigned char *aux_data;
	int tmp_len;
	unsigned char *tmp_data;
};

struct street_name_number_info {
	int len;
	int tag;
	struct coord *c;
	int first;
	int last;
	struct street_name_segment *segment;
};

void street_name_get_by_id(struct street_name *name, struct map_data *mdat, unsigned long id);
void street_name_get(struct street_name *name, unsigned char **p);
int street_name_search(struct map_data *mdat, int country, int town_assoc, const char *name, int partial, int (*func)(struct street_name *name, void *data), void *data);
int street_name_get_info(struct street_name_info *inf, struct street_name *name);
int street_name_get_number_info(struct street_name_number_info *num, struct street_name_info *inf);

