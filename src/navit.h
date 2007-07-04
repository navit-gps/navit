#ifdef __cplusplus
extern "C" {
#endif
extern struct gui *main_loop_gui;
/* prototypes */
enum projection;
struct color;
struct coord;
struct displaylist;
struct gui;
struct layout;
struct mapset;
struct navigation;
struct navit;
struct point;
struct route;
struct transformation;
struct vehicle;
void navit_add_mapset(struct navit *this_, struct mapset *ms);
struct mapset *navit_get_mapset(struct navit *this_);
void navit_add_layout(struct navit *this_, struct layout *lay);
void navit_draw(struct navit *this_);
void navit_draw_displaylist(struct navit *this_);
void navit_zoom_in(struct navit *this_, int factor);
void navit_zoom_out(struct navit *this_, int factor);
struct navit *navit_new(const char *ui, const char *graphics, struct coord *center, enum projection pro, int zoom);
void navit_set_destination(struct navit *this_, struct coord *c, char *description);
void navit_init(struct navit *this_);
void navit_set_center(struct navit *this_, struct coord *center);
void navit_set_center_screen(struct navit *this_, struct point *p);
void navit_toggle_cursor(struct navit *this_);
void navit_set_position(struct navit *this_, struct coord *c);
void navit_vehicle_add(struct navit *this_, struct vehicle *v, struct color *c, int update, int follow);
struct gui *navit_get_gui(struct navit *this_);
struct transformation *navit_get_trans(struct navit *this_);
struct route *navit_get_route(struct navit *this_);
struct navigation *navit_get_navigation(struct navit *this_);
struct displaylist *navit_get_displaylist(struct navit *this_);
void navit_destroy(struct navit *this_);
/* end of prototypes */
#ifdef __cplusplus
}
#endif
