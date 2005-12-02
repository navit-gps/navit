enum destination_type {
	destination_type_town=4,
	destination_type_poly=6,
	destination_type_street=8,
	destination_type_house=12,
	destination_type_map_point=16,
	destination_type_bookmark=128,
};

int destination_address(struct container *co);
int destination_set(struct container *co, enum destination_type type, char *text, struct coord *c);
