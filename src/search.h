#ifdef __cplusplus
extern "C" {
#endif
struct search_list_country {
        struct item item;
        char *car;
        char *iso2;
        char *iso3;
        char *name;
};

struct search_list_town {
	struct item item;
        struct item itemt;
	struct coord *c;
	char *postal;
        char *name;
};

struct search_list_street {
	struct item item;
	struct coord *c;
        char *name;
};

struct search_list_result {
	struct coord *c;
	struct search_list_country *country;
	struct search_list_town *town;
	struct search_list_street *street;
};

/* prototypes */
struct attr;
struct mapset;
struct search_list;
struct search_list_result;
struct search_list *search_list_new(struct mapset *ms);
void search_list_search(struct search_list *this_, struct attr *search_attr, int partial);
struct search_list_result *search_list_get_result(struct search_list *this_);
void search_list_destroy(struct search_list *this_);
/* end of prototypes */
#ifdef __cplusplus
}
#endif
