
/**
 * POI search/filtering parameters.
 *
 */

struct poi_param {

		/**
 		 * =1 if selnb is defined, 0 otherwize.
		 */
		unsigned char sel;

		/**
 		 * Index to struct selector selectors[], shows what type of POIs is defined.
		 */
		unsigned char selnb;
		/**
 		 * Page number to display.
		 */
		unsigned char pagenb;
		/**
 		 * Radius (number of 10-kilometer intervals) to search for POIs.
		 */
		unsigned char dist;
		/**
 		 * Should filter phrase be compared to postal address of the POI.
 		 * =0 - name filter, =1 - address filter, =2 - address filter, including postal code
		 */
		unsigned char AddressFilterType;
		/**
 		 * Filter string, casefold()ed and divided into substrings at the spaces, which are replaced by ASCII 0*.
		 */
		char *filterstr;
		/**
 		 * list of pointers to individual substrings of filterstr.
		 */
		GList *filter;
		/**
		 * Number of POIs in this list
		 */
		int count;
};

/* prototypes */
struct coord;
struct gui_priv;
struct item;
struct poi_param;
struct widget;
void gui_internal_poi_param_free(void *p);
void gui_internal_poi_param_set_filter(struct poi_param *param, char *text);
struct widget *gui_internal_cmd_pois_item(struct gui_priv *this, struct coord *center, struct item *item, struct coord *c, struct route *route, int dist, char *name);
char *gui_internal_compose_item_address_string(struct item *item, int prependPostal);
void gui_internal_cmd_pois_filter(struct gui_priv *this, struct widget *wm, void *data);
void gui_internal_cmd_pois(struct gui_priv *this, struct widget *wm, void *data);
/* end of prototypes */
