struct statusbar_methods;
struct menu_methods;
struct datawindow_methods;
struct navit;

struct gui_priv {
	struct navit *nav;
        GtkWidget *win;
        GtkWidget *vbox;
	GtkWidget *menubar;
	GtkActionGroup *base_group;
	GtkActionGroup *debug_group;
	GtkActionGroup *dyn_group;
	GtkUIManager *menu_manager;
        void *statusbar;
	int dyn_counter;
};

struct menu_priv *gui_gtk_menubar_new(struct gui_priv *gui, struct menu_methods *meth);
struct menu_priv *gui_gtk_toolbar_new(struct gui_priv *gui, struct menu_methods *meth);
struct statusbar_priv *gui_gtk_statusbar_new(struct gui_priv *gui, struct statusbar_methods *meth);
struct menu_priv *gui_gtk_popup_new(struct gui_priv *gui, struct menu_methods *meth);
struct datawindow_priv *gui_gtk_datawindow_new(struct gui_priv *gui, char *name, struct callback *click, struct callback *close, struct datawindow_methods *meth);
