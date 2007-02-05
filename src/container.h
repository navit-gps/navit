enum display_index {
        display_sea=0,
        display_wood,
        display_other,
        display_other1,
        display_other2,
        display_other3,
        display_water,
        display_rail,
        display_street,
        display_street1,
        display_street2,
        display_street3,
        display_street_no_pass,
        display_street_route,
        display_street_route_static,
        display_town,
        display_town1,
        display_town2,
        display_town3,
        display_town4,
        display_town5,
        display_town6,
        display_town7,
        display_town8,
        display_town9,
        display_town10,
        display_town11,
        display_town12,
        display_town13,
        display_town14,
        display_town15,
        display_bti,
        display_poi,
        display_end
};

enum data_window_type {
	data_window_type_block=0,
	data_window_type_town,
	data_window_type_poly,
	data_window_type_street,
	data_window_type_point,
	data_window_type_end
};

struct map_flags {
	int orient_north;
	int track;
};

struct container {
	struct window *win;
	struct transformation *trans;
	struct graphics *gra;
	struct compass *compass;
	struct osd *osd;
	struct display_list *disp[display_end];
	struct map_data *map_data;
	struct menu *menu;
	struct toolbar *toolbar;
	struct statusbar *statusbar;
	struct route *route;
	struct cursor *cursor;
	struct speech *speech;
	struct vehicle *vehicle;
	struct track *track;
        struct data_window *data_window[data_window_type_end];
	struct map_flags *flags;
	struct _GtkMap *map;
};
