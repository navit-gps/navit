#ifndef WIN32_GUI_INCLUDED
#define WIN32_GUI_INCLUDED

#include "resources/resource.h"
#include "coord.h"
#include "point.h"
#include "graphics.h"
#include "event.h"

#define ID_CHILD_GFX 100
#define ID_CHILD_TOOLBAR (ID_CHILD_GFX + 1)
#define ID_CHILD_1 (ID_CHILD_TOOLBAR + 1)
#define ID_CHILD_2 (ID_CHILD_1 + 1)
#define ID_CHILD_3 (ID_CHILD_2 + 1)
#define ID_CHILD_4 (ID_CHILD_4 + 1)

#define ID_DISPLAY_ZOOMIN 		200
#define ID_DISPLAY_ZOOMOUT		201
#define ID_DISPLAY_REFRESH		202
#define ID_DISPLAY_CURSOR		203
#define ID_DISPLAY_ORIENT		204

#define ID_FILE_EXIT 		9001
#define ID_STUFF_GO 		9002

#define _(text) gettext(text)

#define POPUP_MENU_OFFSET  4000

struct statusbar_methods;
struct menu_methods;
struct datawindow_methods;
struct navit;
struct callback;


struct menu_priv {
	HWND wnd_handle;
	HMENU hMenu;
	struct callback* cb;
};

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
	struct callback_list *cbl;
	enum draw_mode_num mode;
};

struct menu_priv *gui_gtk_menubar_new(struct gui_priv *gui, struct menu_methods *meth);
struct menu_priv *gui_gtk_toolbar_new(struct gui_priv *gui, struct menu_methods *meth);
struct statusbar_priv *gui_gtk_statusbar_new(struct gui_priv *gui, struct statusbar_methods *meth);
struct menu_priv *gui_gtk_popup_new(struct gui_priv *gui, struct menu_methods *meth);
struct datawindow_priv *gui_gtk_datawindow_new(struct gui_priv *gui, char *name, struct callback *click, struct callback *close, struct datawindow_methods *meth);

struct graphics_priv* win32_graphics_new( struct navit *nav, struct graphics_methods *meth, struct attr **attrs, struct callback_list *cbl);

#endif
