#ifndef NAVIT_GUI_H
#define NAVIT_GUI_H

struct navit;
struct gui_priv;
struct menu_methods;
struct datawindow_methods;
struct callback;
struct graphics;
struct coord;
struct pcoord;

struct gui_methods {
	struct menu_priv *(*menubar_new)(struct gui_priv *priv, struct menu_methods *meth);
	struct menu_priv *(*popup_new)(struct gui_priv *priv, struct menu_methods *meth);
	int (*set_graphics)(struct gui_priv *priv, struct graphics *gra);
	int (*run_main_loop)(struct gui_priv *priv);
	struct datawindow_priv *(*datawindow_new)(struct gui_priv *priv, char *name, struct callback *click, struct callback *close, struct datawindow_methods *meth);
	int (*add_bookmark)(struct gui_priv *priv, struct pcoord *c, char *description);
};


/* prototypes */
enum attr_type;
struct attr;
struct attr_iter;
struct callback;
struct datawindow;
struct graphics;
struct gui;
struct menu;
struct navit;
struct pcoord;
struct gui *gui_new(struct attr *parent, struct attr **attrs);
int gui_get_attr(struct gui *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter);
struct menu *gui_menubar_new(struct gui *gui);
struct menu *gui_popup_new(struct gui *gui);
struct datawindow *gui_datawindow_new(struct gui *gui, char *name, struct callback *click, struct callback *close);
int gui_add_bookmark(struct gui *gui, struct pcoord *c, char *description);
int gui_set_graphics(struct gui *this_, struct graphics *gra);
int gui_has_main_loop(struct gui *this_);
int gui_run_main_loop(struct gui *this_);
/* end of prototypes */

#endif

