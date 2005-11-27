enum search_param {
	search_country,
	search_postal,
	search_town,
	search_district,
	search_street,
	search_number
};

struct search_destination {
	char *country_name;
	char *country_car;
	char *country_iso2;	
	char *country_iso3;
	char *town_postal;
	char *town_name;
	char *district;
	char *street_name;
	char *street_number;
	struct country *country;
	struct town *town;
	struct street_name *street;
	struct coord *c;
};

struct search;
struct map_data;

void search_update(struct search *search, enum search_param what, char *val);
struct search *search_new(struct map_data *mdat, char *country, char *postal, char *town, char *district, char *street, char *number, int (*func)(struct search_destination *dest, void *user_data), void *user_data);
