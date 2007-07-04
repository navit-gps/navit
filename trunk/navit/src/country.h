/* prototypes */
struct attr;
struct country_search;
struct item;
struct country_search *country_search_new(struct attr *search, int partial);
struct item *country_search_get_item(struct country_search *this);
void country_search_destroy(struct country_search *this);
