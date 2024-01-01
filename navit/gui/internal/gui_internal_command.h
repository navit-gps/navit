/* prototypes */
struct attr;
struct gui_priv;
struct pcoord;
int gui_internal_cmd2_quit(struct gui_priv *this, char *function, struct attr **in, struct attr ***out);
void gui_internal_command_init(struct gui_priv *this, struct attr **attrs);
/* end of prototypes */
