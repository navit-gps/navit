struct navit;
struct gui_priv;
struct menu_methods;
struct statusbar_methods;
struct datawindow_methods;
struct callback;
struct graphics;
struct coord;

struct gui_methods {
	struct menu_priv *(*menubar_new)(struct gui_priv *priv, struct menu_methods *meth);
	struct menu_priv *(*toolbar_new)(struct gui_priv *priv, struct menu_methods *meth);
	struct statusbar_priv *(*statusbar_new)(struct gui_priv *priv, struct statusbar_methods *meth);
	struct menu_priv *(*popup_new)(struct gui_priv *priv, struct menu_methods *meth);
	int (*set_graphics)(struct gui_priv *priv, struct graphics *gra);
	int (*run_main_loop)(struct gui_priv *priv);
	struct datawindow_priv *(*datawindow_new)(struct gui_priv *priv, char *name, struct callback *click, struct callback *close, struct datawindow_methods *meth);
	int (*add_bookmark)(struct gui_priv *priv, struct coord *c, char *description);
};


struct gui {
	struct gui_methods meth;
	struct gui_priv *priv;		
};

/* prototypes */
struct callback;
struct coord;
struct datawindow;
struct graphics;
struct gui;
struct menu;
struct navit;
struct statusbar;
struct gui *gui_new(struct navit *nav, const char *type, int w, int h);
struct statusbar *gui_statusbar_new(struct gui *gui);
struct menu *gui_menubar_new(struct gui *gui);
struct menu *gui_toolbar_new(struct gui *gui);
struct menu *gui_popup_new(struct gui *gui);
struct datawindow *gui_datawindow_new(struct gui *gui, char *name, struct callback *click, struct callback *close);
int gui_add_bookmark(struct gui *gui, struct coord *c, char *description);
int gui_set_graphics(struct gui *this_, struct graphics *gra);
int gui_has_main_loop(struct gui *this_);
int gui_run_main_loop(struct gui *this_);
/* end of prototypes */
