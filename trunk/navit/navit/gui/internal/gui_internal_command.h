/* prototypes */
struct attr;
struct gui_priv;
struct pcoord;
char *gui_internal_coordinates(struct pcoord *pc, char sep);
void gui_internal_cmd2_quit(struct gui_priv *this, char *function, struct attr **in, struct attr ***out, int *valid);
void gui_internal_command_init(struct gui_priv *this, struct attr **attrs);
/* end of prototypes */
