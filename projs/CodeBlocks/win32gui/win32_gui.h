#ifndef WIN32_GUI_INCLUDED
#define WIN32_GUI_INCLUDED

#include "coord.h"
#include "point.h"
#include "graphics.h"

#define ID_CHILD_GFX 2000
#define ID_CHILD_1 2001
#define ID_CHILD_2 ID_CHILD_1 + 1
#define ID_CHILD_3 ID_CHILD_2 + 1
#define ID_CHILD_4 ID_CHILD_4 + 1
#define ID_DISPLAY_ZOOMIN 8000
#define ID_DISPLAY_ZOOMOUT 8001

#define ID_FILE_EXIT 9001
#define ID_STUFF_GO 9002

#define _(text) gettext(text)


struct statusbar_methods;
struct menu_methods;
struct datawindow_methods;
struct navit;
struct callback;

struct gui_priv {
	struct navit *nav;
	HANDLE	hwnd;
};


struct graphics_priv {
	struct point p;
	int width;
	int height;
	int library_init;
	int visible;
	HANDLE wnd_parent_handle;
	HANDLE wnd_handle;
	COLORREF bg_color;


	void (*resize_callback)(void *data, int w, int h);
	void *resize_callback_data;
	void (*motion_callback)(void *data, struct point *p);
	void *motion_callback_data;
	void (*button_callback)(void *data, int press, int button, struct point *p);
	void *button_callback_data;
	enum draw_mode_num mode;
};

struct menu_priv *gui_gtk_menubar_new(struct gui_priv *gui, struct menu_methods *meth);
struct menu_priv *gui_gtk_toolbar_new(struct gui_priv *gui, struct menu_methods *meth);
struct statusbar_priv *gui_gtk_statusbar_new(struct gui_priv *gui, struct statusbar_methods *meth);
struct menu_priv *gui_gtk_popup_new(struct gui_priv *gui, struct menu_methods *meth);
struct datawindow_priv *gui_gtk_datawindow_new(struct gui_priv *gui, char *name, struct callback *click, struct callback *close, struct datawindow_methods *meth);

struct graphics_priv* win32_graphics_new( struct graphics_methods *meth, struct attr **attrs);

#endif
