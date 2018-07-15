/**
 * @brief A structure to store configuration values.
 *
 * This structure stores configuration values for how gui elements in the internal GUI
 * should be drawn.
 */
struct gui_config_settings {

  /**
   * The base size (in fractions of a point) to use for text.
   */
  int font_size;
  /**
   * The size (in pixels) that xs style icons should be scaled to.
   * This icon size is typically used in various lists and should be set to value which allows a list row to be easily cliked or dragged.
   */
  int icon_xs;
  /**
   * The size (in pixels) that s style icons (small) should be scaled to, used for the menu top row icons
   */
  int icon_s;
  /**
   * The size (in pixels) that l style icons should be scaled to, used for icons defined in the menu html
   */
  int icon_l;
  /**
   * The default amount of spacing (in pixels) to place between GUI elements.
   */
  int spacing;

};

struct route_data {
  struct widget * route_table;
  int route_showing;
};

/**
 * Private data for the internal GUI.
 *
 * @author Martin Schaller (04/2008)
 */
struct gui_priv {
	struct navit *nav;
	struct attr self;
	struct window *win;
	struct graphics *gra;
	struct graphics_gc *background;
	struct graphics_gc *background2;
	struct graphics_gc *highlight_background;
	struct graphics_gc *foreground;
	struct graphics_gc *text_foreground;
	struct graphics_gc *text_background;
	struct color background_color, background2_color, text_foreground_color, text_background_color;
	int spacing;
	int font_size;
	char *font_name;
	int fullscreen;
	struct graphics_font *fonts[3];
	int icon_xs;						/**< The size (in pixels) that xs style icons should be scaled to.
										  *  This icon size can be too small to click it on some devices.
										  */
	int icon_s;							/**< The size (in pixels) that s style icons (small) should be scaled to */
	int icon_l;							/**< The size (in pixels) that l style icons should be scaled to */
	int pressed;
	struct widget *widgets;
	int widgets_count;
	int redraw;
	struct widget root;
	struct widget *highlighted,*editable;
	struct widget *highlighted_menu;
	struct pcoord clickp, vehiclep;
	struct attr *click_coord_geo, *position_coord_geo;
	struct search_list *sl;
	int ignore_button;
	int menu_on_map_click;
	char *on_map_click;
	int signal_on_map_click;
	char *country_iso2;
	int speech;
	int keyboard;						/**< Whether the internal GUI keyboard is enabled */
	int keyboard_required;				/**< Whether keyboard input is needed. This is only used by the
										  *  HTML menu, text entry dialogs do not use this member.
										  */
	struct gui_config_settings config;	/**< The setting information read from the configuration file.
										  *  values of -1 indicate no value was specified in the config file.
										  */
	struct event_idle *idle;
	struct callback *motion_cb,*button_cb,*resize_cb,*keypress_cb,*window_closed_cb,*idle_cb, *motion_timeout_callback;
	struct event_timeout *motion_timeout_event;
	struct point current;

	struct callback * vehicle_cb;
	struct route_data route_data;		/**< Stores information about the route. */

	struct gui_internal_data data;
	struct callback_list *cbl;
	int flags;
	int cols;
	struct attr osd_configuration;		/**< The OSD configuration, a set of flags controlling which OSD
	                                      *  items will be visible.
	                                      */
	int pitch;							/**< The pitch for the 3D map view. */
	int flags_town,flags_street,flags_house_number;
	int radius;
	int mouse_button_clicked_on_map;
/* html */
	char *html_text;
	int html_depth;
	struct widget *html_container;
	int html_skip;
	char *html_anchor;
	char *href;
	int html_anchor_found;
	struct form *form;
	struct html {
		int skip;
		enum html_tag {
			html_tag_none,
			html_tag_a,
			html_tag_h1,
			html_tag_html,
			html_tag_img,
			html_tag_script,
			html_tag_form,
			html_tag_input,
			html_tag_div,
		} tag;
		char *command;
		char *name;
		char *href;
		char *refresh_cond;
		char *class;
		int font_size;
		struct widget *w;
		struct widget *container;
	} html[10];

/* gestures */

	struct gesture_elem {
		long long msec;
		struct point p;
	} gesture_ring[GESTURE_RINGSIZE];
	int gesture_ring_last, gesture_ring_first;

	int hide_keys; //Flag to set the keyboard mode 1: hide impossible keys on search; 0: highlight them.
	int results_map_population;
};

struct menu_data {
	struct widget *search_list;
	struct widget *keyboard;
	struct widget *button_bar;
	struct widget *menu;
	int keyboard_mode;
	void (*redisplay)(struct gui_priv *priv, struct widget *widget, void *data);
	struct widget *redisplay_widget;
	char *href;
	struct attr refresh_callback_obj,refresh_callback;
};

struct heightline {
	struct heightline *next;
	int height;
	struct coord_rect bbox;
	int count;
	struct coord c[0];
};

struct diagram_point {
	struct diagram_point *next;
	struct coord c;
};
/* prototypes */
enum flags;
struct attr;
struct coord;
struct coord_geo;
struct graphics_image;
struct gui_priv;
struct heightline;
struct item;
struct map;
struct navit;
struct pcoord;
struct point;
struct vehicle;
struct widget;
struct graphics_image *image_new_xs(struct gui_priv *this, const char *name);
struct graphics_image *image_new_s(struct gui_priv *this, const char *name);
struct graphics_image *image_new_l(struct gui_priv *this, const char *name);
struct widget *gui_internal_button_navit_attr_new(struct gui_priv *this, const char *text, enum flags flags, struct attr *on, struct attr *off);
struct widget *gui_internal_button_map_attr_new(struct gui_priv *this, const char *text, enum flags flags, struct map *map, struct attr *on, struct attr *off, int deflt);
void gui_internal_say(struct gui_priv *this, struct widget *w, int questionmark);
void gui_internal_back(struct gui_priv *this, struct widget *w, void *data);
void gui_internal_cmd_return(struct gui_priv *this, struct widget *wm, void *data);
void gui_internal_cmd_main_menu(struct gui_priv *this, struct widget *wm, void *data);
struct widget *gui_internal_time_help(struct gui_priv *this);
void gui_internal_apply_config(struct gui_priv *this);
void gui_internal_select_waypoint(struct gui_priv *this, const char *title, const char *hint, struct widget *wm_, void (*cmd)(struct gui_priv *priv, struct widget *widget, void *data), void *data);
void gui_internal_call_linked_on_finish(struct gui_priv *this, struct widget *wm, void *data);
char *removecase(char *s);
void gui_internal_cmd_position_do(struct gui_priv *this, struct pcoord *pc_in, struct coord_geo *g_in, struct widget *wm, const char *name, int flags);
void gui_internal_cmd_position(struct gui_priv *this, struct widget *wm, void *data);
void gui_internal_cmd_bookmarks(struct gui_priv *this, struct widget *wm, void *data);
void gui_internal_keypress_do(struct gui_priv *this, char *key);
char *gui_internal_cmd_match_expand(char *pattern, struct attr **in);
int gui_internal_set(char *remove, char *add);
void gui_internal_cmd_map_download(struct gui_priv *this, struct widget *wm, void *data);
void gui_internal_menu_vehicle_settings(struct gui_priv *this, struct vehicle *v, char *name);
void gui_internal_cmd_vehicle_settings(struct gui_priv *this, struct widget *wm, void *data);
void gui_internal_evaluate(struct gui_priv *this, const char *command);
void gui_internal_enter(struct gui_priv *this, int ignore);
void gui_internal_leave(struct gui_priv *this);
void gui_internal_set_click_coord(struct gui_priv *this, struct point *p);
void gui_internal_enter_setup(struct gui_priv *this);
void gui_internal_cmd_menu(struct gui_priv *this, int ignore, char *href);
void gui_internal_cmd_log_clicked(struct gui_priv *this, struct widget *widget, void *data);
void gui_internal_check_exit(struct gui_priv *this);
void gui_internal_cmd_enter_coord_clicked(struct gui_priv *this, struct widget *widget, void *data);
int line_intersection(struct coord *a1, struct coord *a2, struct coord *b1, struct coord *b2, struct coord *res);
struct heightline *item_get_heightline(struct item *item);
void gui_internal_route_update(struct gui_priv *this, struct navit *navit, struct vehicle *v);
void gui_internal_route_screen_free(struct gui_priv *this_, struct widget *w);
void gui_internal_populate_route_table(struct gui_priv *this, struct navit *navit);
void plugin_init(void);
/* end of prototypes */
