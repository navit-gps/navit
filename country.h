struct country {
	int id;
	char *car;
	char *iso2;
	char *iso3;
	char *name;
};
struct country * country_get_by_id(int id);
int country_search_by_name(const char *name, int partial, int (*func)(struct country *cou, void *data), void *data);
int country_search_by_car(const char *name, int partial, int (*func)(struct country *cou, void *data), void *data);
int country_search_by_iso2(const char *name, int partial, int (*func)(struct country *cou, void *data), void *data);
int country_search_by_iso3(const char *name, int partial, int (*func)(struct country *cou, void *data), void *data);
