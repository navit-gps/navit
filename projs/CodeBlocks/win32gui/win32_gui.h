#ifndef WIN32_GUI_INCLUDED
#define WIN32_GUI_INCLUDED

#include "coord.h"

struct statusbar_methods;
struct menu_methods;
struct datawindow_methods;
struct navit;
struct callback;

struct gui_priv {
	struct navit *nav;
	HANDLE	hwnd;
};

struct menu_priv *gui_gtk_menubar_new(struct gui_priv *gui, struct menu_methods *meth);
struct menu_priv *gui_gtk_toolbar_new(struct gui_priv *gui, struct menu_methods *meth);
struct statusbar_priv *gui_gtk_statusbar_new(struct gui_priv *gui, struct statusbar_methods *meth);
struct menu_priv *gui_gtk_popup_new(struct gui_priv *gui, struct menu_methods *meth);
struct datawindow_priv *gui_gtk_datawindow_new(struct gui_priv *gui, char *name, struct callback *click, struct callback *close, struct datawindow_methods *meth);

#endif
