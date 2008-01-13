#include <string.h>
#include <gtk/gtk.h>
#include "navit.h"
#include "gui_gtk.h"
#include "menu.h"
#include "coord.h"
#include "item.h"
#include "attr.h"
#include "callback.h"
#include "debug.h"
#include "destination.h"

#define gettext_noop(String) String
#define _(STRING)    gettext(STRING)
#define _n(STRING)    gettext_noop(STRING)

struct menu_priv {
	char *path;
	GtkAction *action;
	struct gui_priv *gui;
	enum menu_type type;
	struct callback *cb;
	struct menu_priv *child;
	struct menu_priv *sibling;
	gulong handler_id;
	guint merge_id;
	GtkWidget *widget;
};

/* Create callbacks that implement our Actions */

static void
zoom_in_action(GtkWidget *w, struct gui_priv *gui, void *dummy)
{
	navit_zoom_in(gui->nav, 2, NULL);
}

static void
zoom_out_action(GtkWidget *w, struct gui_priv *gui, void *dummy)
{
	navit_zoom_out(gui->nav, 2, NULL);
}

static void
refresh_action(GtkWidget *w, struct gui_priv *gui, void *dummy)
{
	navit_draw(gui->nav);
}

static void
roadbook_action(GtkWidget *w, struct gui_priv *gui, void *dummy)
{
	navit_window_roadbook_new(gui->nav);
}

static void
cursor_action(GtkWidget *w, struct gui_priv *gui, void *dummy)
{
	struct attr attr;

	attr.type=attr_cursor;
	attr.u.num=gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(w));
	if(!navit_set_attr(gui->nav, &attr)) {
		dbg(0, "Failed to set attr_cursor\n");
	}
}

static void
tracking_action(GtkWidget *w, struct gui_priv *gui, void *dummy)
{
	struct attr attr;

	attr.type=attr_tracking;
	attr.u.num=gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(w));
	if(!navit_set_attr(gui->nav, &attr)) {
		dbg(0, "Failed to set attr_tracking\n");
	}
}

static void
orient_north_action(GtkWidget *w, struct gui_priv *gui, void *dummy)
{
	struct attr attr;

	attr.type=attr_orientation;
	attr.u.num=gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(w));
	if(!navit_set_attr(gui->nav, &attr)) {
		dbg(0, "Failed to set attr_orientation\n");
	}
}

static void
window_fullscreen_action(GtkWidget *w, struct gui_priv *gui, void *dummy)
{
	if(gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(w)))
		gtk_window_fullscreen(GTK_WINDOW(gui->win));
	else
		gtk_window_unfullscreen(GTK_WINDOW(gui->win));
}

#include <stdlib.h>
#include "point.h"
#include "transform.h"

static void
info_action(GtkWidget *w, struct gui_priv *gui, void *dummy)
{
	char buffer[512];
	int mw,mh;
	struct coord lt, rb;
	struct point p;
	struct transformation *t;

	t=navit_get_trans(gui->nav);
	transform_get_size(t, &mw, &mh);
	p.x=0;
	p.y=0;
	transform_reverse(t, &p, &lt);
	p.x=mw;
	p.y=mh;
	transform_reverse(t, &p, &rb);

	sprintf(buffer,"./info.sh %d,%d 0x%x,0x%x 0x%x,0x%x", mw, mh, lt.x, lt.y, rb.x, rb.y);
	system(buffer);

}


static void
route_clear_action(GtkWidget *w, struct gui_priv *gui, void *dummy)
{
	navit_set_destination(gui->nav, NULL, NULL);
}

static void
destination_action(GtkWidget *w, struct gui_priv *gui, void *dummy)
{
	destination_address(gui->nav);
}


static void
quit_action (GtkWidget *w, struct gui_priv *gui, void *dummy)
{
	navit_destroy(gui->nav);
}

static void
visible_blocks_action(GtkWidget *w, struct container *co)
{
#if 0
	co->data_window[data_window_type_block]=data_window("Visible Blocks",co->win,NULL);
	graphics_redraw(co);
#endif
}

static void
visible_towns_action(GtkWidget *w, struct container *co)
{
#if 0
	co->data_window[data_window_type_town]=data_window("Visible Towns",co->win,NULL);
	graphics_redraw(co);
#endif
}

static void
visible_polys_action(GtkWidget *w, struct container *co)
{
#if 0
	co->data_window[data_window_type_street]=data_window("Visible Polys",co->win,NULL);
	graphics_redraw(co);
#endif
}

static void
visible_streets_action(GtkWidget *w, struct container *co)
{
#if 0
	co->data_window[data_window_type_street]=data_window("Visible Streets",co->win,NULL);
	graphics_redraw(co);
#endif
}

static void
visible_points_action(GtkWidget *w, struct container *co)
{
#if 0
	co->data_window[data_window_type_point]=data_window("Visible Points",co->win,NULL);
	graphics_redraw(co);
#endif
}

static GtkActionEntry entries[] =
{
	{ "DisplayMenuAction", NULL, gettext_noop("Display") },
	{ "RouteMenuAction", NULL, _n("Route") },
	{ "Map", NULL, _n("Map") },
	{ "LayoutMenuAction", NULL, _n("Layout") },
	{ "ZoomOutAction", GTK_STOCK_ZOOM_OUT, _n("ZoomOut"), NULL, NULL, G_CALLBACK(zoom_out_action) },
	{ "ZoomInAction", GTK_STOCK_ZOOM_IN, _n("ZoomIn"), NULL, NULL, G_CALLBACK(zoom_in_action) },
	{ "RefreshAction", GTK_STOCK_REFRESH, _n("Refresh"), NULL, NULL, G_CALLBACK(refresh_action) },
	{ "RoadbookAction", GTK_STOCK_JUSTIFY_FILL, _n("Roadbook"), NULL, NULL, G_CALLBACK(roadbook_action) },
#ifdef GTK_STOCK_INFO
	{ "InfoAction", GTK_STOCK_INFO, _n("Info"), NULL, NULL, G_CALLBACK(info_action) },
#else
	{ "InfoAction", NULL, _n("Info"), NULL, NULL, G_CALLBACK(info_action) },
#endif /*GTK_STOCK_INFO*/
	{ "DestinationAction", "flag_icon", _n("Destination"), NULL, NULL, G_CALLBACK(destination_action) },
	{ "RouteClearAction", NULL, _n("Clear"), NULL, NULL, G_CALLBACK(route_clear_action) },
	{ "Test", NULL, _n("Test"), NULL, NULL, G_CALLBACK(destination_action) },
	{ "QuitAction", GTK_STOCK_QUIT, _n("_Quit"), "<control>Q",NULL, G_CALLBACK (quit_action) }
};

static guint n_entries = G_N_ELEMENTS (entries);

static GtkToggleActionEntry toggleentries[] =
{
	{ "CursorAction", "cursor_icon",_n("Cursor"), NULL, NULL, G_CALLBACK(cursor_action),TRUE },
	{ "TrackingAction", NULL ,_n("Tracking"), NULL, NULL, G_CALLBACK(tracking_action),TRUE },
	{ "OrientationAction", "orientation_icon", _n("Orientation"), NULL, NULL, G_CALLBACK(orient_north_action),FALSE },
#ifdef GTK_STOCK_FULLSCREEN
	{ "FullscreenAction",GTK_STOCK_FULLSCREEN, _n("Fullscreen"), NULL, NULL, G_CALLBACK(window_fullscreen_action), FALSE }
#else
	{ "FullscreenAction", NULL, _n("Fullscreen"), NULL, NULL, G_CALLBACK(window_fullscreen_action), FALSE }
#endif /*GTK_STOCK_FULLSCREEN*/
};

static guint n_toggleentries = G_N_ELEMENTS (toggleentries);

static GtkActionEntry debug_entries[] =
{
	{ "DataMenuAction", NULL, _n("Data") },
	{ "VisibleBlocksAction", NULL, _n("VisibleBlocks"), NULL, NULL, G_CALLBACK(visible_blocks_action) },
	{ "VisibleTownsAction", NULL, _n("VisibleTowns"), NULL, NULL, G_CALLBACK(visible_towns_action) },
	{ "VisiblePolysAction", NULL, _n("VisiblePolys"), NULL, NULL, G_CALLBACK(visible_polys_action) },
	{ "VisibleStreetsAction", NULL, _n("VisibleStreets"), NULL, NULL, G_CALLBACK(visible_streets_action) },
	{ "VisiblePointsAction", NULL, _n("VisiblePoints"), NULL, NULL, G_CALLBACK(visible_points_action) },
};

static guint n_debug_entries = G_N_ELEMENTS (debug_entries);


static const char * cursor_xpm[] = {
"22 22 2 1",
" 	c None",
".	c #0000FF",
"                      ",
"                      ",
"                      ",
"          ..          ",
"        ..  ..        ",
"      ..      ..      ",
"     .          .     ",
"     .          .     ",
"    .        ... .    ",
"    .     ... .  .    ",
"   .   ...   .    .   ",
"   . ..     .     .   ",
"    .      .     .    ",
"    .     .      .    ",
"     .   .      .     ",
"     .  .       .     ",
"      ..      ..      ",
"        ..  ..        ",
"          ..          ",
"                      ",
"                      ",
"                      "};


static const char * north_xpm[] = {
"22 22 2 1",
" 	c None",
".	c #000000",
"                      ",
"                      ",
"           .          ",
"          ...         ",
"         . . .        ",
"        .  .  .       ",
"           .          ",
"     ....  .  ....    ",
"     ....  .  ....    ",
"      .... .   ..     ",
"      .. ..    ..     ",
"      ..  ..   ..     ",
"      ..   ..  ..     ",
"      ..    .. ..     ",
"      ..   . ....     ",
"     ....  .  ....    ",
"     ....  .  ....    ",
"           .          ",
"           .          ",
"           .          ",
"                      ",
"                      "};


static const char * flag_xpm[] = {
"22 22 2 1",
" 	c None",
"+	c #000000",
"+++++++               ",
"+   +++++++++         ",
"+   +++   +++++++++   ",
"+   +++   +++   +++   ",
"++++      +++   +++   ",
"++++   +++      +++   ",
"++++   +++   +++  +   ",
"+   ++++++   +++  +   ",
"+   +++   ++++++  +   ",
"+   +++   +++   +++   ",
"++++      +++   +++   ",
"++++   +++      +++   ",
"++++++++++   +++  +   ",
"+      +++++++++  +   ",
"+            ++++++   ",
"+                     ",
"+                     ",
"+                     ",
"+                     ",
"+                     ",
"+                     ",
"+                     "};



static struct {
	gchar *stockid;
	const char **icon_xpm;
} stock_icons[] = {
	{"cursor_icon", cursor_xpm },
	{"orientation_icon", north_xpm },
	{"flag_icon", flag_xpm }
};


static gint n_stock_icons = G_N_ELEMENTS (stock_icons);


static void
register_my_stock_icons (void)
{
	GtkIconFactory *icon_factory;
	GtkIconSet *icon_set;
	GdkPixbuf *pixbuf;
	gint i;

	icon_factory = gtk_icon_factory_new ();

	for (i = 0; i < n_stock_icons; i++)
	{
		pixbuf = gdk_pixbuf_new_from_xpm_data(stock_icons[i].icon_xpm);
		icon_set = gtk_icon_set_new_from_pixbuf (pixbuf);
		g_object_unref(pixbuf);
		gtk_icon_factory_add (icon_factory, stock_icons[i].stockid, icon_set);
		gtk_icon_set_unref (icon_set);
	}

	gtk_icon_factory_add_default(icon_factory);

	g_object_unref(icon_factory);
}


static char layout[] =
	"<ui>\
		<menubar name=\"MenuBar\">\
			<menu name=\"Display\" action=\"DisplayMenuAction\">\
				<menuitem name=\"Zoom in\" action=\"ZoomInAction\" />\
				<menuitem name=\"Zoom out\" action=\"ZoomOutAction\" />\
				<menuitem name=\"Cursor\" action=\"CursorAction\"/>\
				<menuitem name=\"Tracking\" action=\"TrackingAction\"/>\
				<menuitem name=\"Orientation\" action=\"OrientationAction\"/>\
				<menuitem name=\"Roadbook\" action=\"RoadbookAction\"/>\
				<menuitem name=\"Fullscreen\" action=\"FullscreenAction\"/>\
				<menuitem name=\"Quit\" action=\"QuitAction\" />\
				<placeholder name=\"RouteMenuAdditions\" />\
			</menu>\
			<menu name=\"Data\" action=\"DataMenuAction\">\
				<menuitem name=\"Visible Blocks\" action=\"VisibleBlocksAction\" />\
				<menuitem name=\"Visible Towns\" action=\"VisibleTownsAction\" />\
				<menuitem name=\"Visible Polys\" action=\"VisiblePolysAction\" />\
				<menuitem name=\"Visible Streets\" action=\"VisibleStreetsAction\" />\
				<menuitem name=\"Visible Points\" action=\"VisiblePointsAction\" />\
				<menuitem name=\"Visible Routegraph\" action=\"VisibleRouteGraphAction\" />\
				<placeholder name=\"DataMenuAdditions\" />\
			</menu>\
			<menu name=\"Route\" action=\"RouteMenuAction\">\
				<menuitem name=\"Refresh\" action=\"RefreshAction\" />\
				<menuitem name=\"Destination\" action=\"DestinationAction\" />\
				<menuitem name=\"Clear\" action=\"RouteClearAction\" />\
				<placeholder name=\"RouteMenuAdditions\" />\
			</menu>\
		</menubar>\
	 	<toolbar name=\"ToolBar\" action=\"BaseToolbar\" action=\"BaseToolbarAction\">\
			<placeholder name=\"ToolItems\">\
				<separator/>\
				<toolitem name=\"Zoom in\" action=\"ZoomInAction\"/>\
				<toolitem name=\"Zoom out\" action=\"ZoomOutAction\"/>\
				<toolitem name=\"Refresh\" action=\"RefreshAction\"/>\
				<toolitem name=\"Cursor\" action=\"CursorAction\"/>\
				<toolitem name=\"Orientation\" action=\"OrientationAction\"/>\
				<toolitem name=\"Destination\" action=\"DestinationAction\"/>\
				<!-- <toolitem name=\"Info\" action=\"InfoAction\"/> -->\
				<toolitem name=\"Roadbook\" action=\"RoadbookAction\"/>\
				<toolitem name=\"Quit\" action=\"QuitAction\"/>\
				<separator/>\
			</placeholder>\
		</toolbar>\
		<popup name=\"PopUp\">\
		</popup>\
	</ui>";


static void
activate(void *dummy, struct menu_priv *menu)
{
	if (menu->cb)
		callback_call_0(menu->cb);
}

static struct menu_methods menu_methods;

static struct menu_priv *
add_menu(struct menu_priv *menu, struct menu_methods *meth, char *name, enum menu_type type, struct callback *cb)
{
	struct menu_priv *ret;
	char *dynname;

	ret=g_new0(struct menu_priv, 1);
	*meth=menu_methods;
	if (! strcmp(menu->path, "/ui/MenuBar") && !strcmp(name,"Route")) {
		dynname=g_strdup("Route");
	} else if (! strcmp(menu->path, "/ui/MenuBar") && !strcmp(name,"Data")) {
		dynname=g_strdup("Data");
	} else {
		dynname=g_strdup_printf("%d", menu->gui->dyn_counter++);
		if (type == menu_type_toggle)
			ret->action=GTK_ACTION(gtk_toggle_action_new(dynname, name, NULL, NULL));
		else
			ret->action=gtk_action_new(dynname, name, NULL, NULL);
		if (cb)
			ret->handler_id=g_signal_connect(ret->action, "activate", G_CALLBACK(activate), ret);
		gtk_action_group_add_action(menu->gui->dyn_group, ret->action);
		ret->merge_id=gtk_ui_manager_new_merge_id(menu->gui->menu_manager);
		gtk_ui_manager_add_ui( menu->gui->menu_manager, ret->merge_id, menu->path, dynname, dynname, type == menu_type_submenu ? GTK_UI_MANAGER_MENU : GTK_UI_MANAGER_MENUITEM, FALSE);
	}
	ret->gui=menu->gui;
	ret->path=g_strdup_printf("%s/%s", menu->path, dynname);
	ret->type=type;
	ret->cb=cb;
	ret->sibling=menu->child;
	menu->child=ret;
	g_free(dynname);
	return ret;

}

static void
remove_menu(struct menu_priv *item, int recursive)
{

	if (recursive) {
		struct menu_priv *next,*child=item->child;
		while (child) {
			next=child->sibling;
			remove_menu(child, recursive);
			child=next;
		}
	}
	if (item->action) {
		gtk_ui_manager_remove_ui(item->gui->menu_manager, item->merge_id);
		gtk_action_group_remove_action(item->gui->dyn_group, item->action);
#if 0
		if (item->callback)
			g_signal_handler_disconnect(item->action, item->handler_id);
#endif
		g_object_unref(item->action);
	}
	g_free(item->path);
	g_free(item);
}

static void
set_toggle(struct menu_priv *menu, int active)
{
	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(menu->action), active);
}

static  int
get_toggle(struct menu_priv *menu)
{
	return gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(menu->action));
}

static struct menu_methods menu_methods = {
	add_menu,
	set_toggle,
	get_toggle,
};


static void
popup_deactivate(GtkWidget *widget, struct menu_priv *menu)
{
	g_signal_handler_disconnect(widget, menu->handler_id);
	remove_menu(menu, 1);
}

static void
popup_activate(struct menu_priv *menu)
{
#ifdef _WIN32
	menu->widget=gtk_ui_manager_get_widget(menu->gui->menu_manager, menu->path );
#endif
	gtk_menu_popup(GTK_MENU(menu->widget), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time ());
	menu->handler_id=g_signal_connect(menu->widget, "selection-done", G_CALLBACK(popup_deactivate), menu);
}

static struct menu_priv *
gui_gtk_ui_new (struct gui_priv *this, struct menu_methods *meth, char *path, int popup, GtkWidget **widget_ret)
{
	struct menu_priv *ret;
	GError *error;
	GtkWidget *widget;
	struct attr attr;
	GtkToggleAction *toggle_action;

	*meth=menu_methods;
	ret=g_new0(struct menu_priv, 1);
	ret->path=g_strdup(path);
	ret->gui=this;
	if (! this->menu_manager) {
		this->base_group = gtk_action_group_new ("BaseActions");
		this->debug_group = gtk_action_group_new ("DebugActions");
		this->dyn_group = gtk_action_group_new ("DynamicActions");
		register_my_stock_icons();
		this->menu_manager = gtk_ui_manager_new ();
		gtk_action_group_set_translation_domain(this->base_group,"navit");
		gtk_action_group_set_translation_domain(this->debug_group,"navit");
		gtk_action_group_set_translation_domain(this->dyn_group,"navit");
		gtk_action_group_add_actions (this->base_group, entries, n_entries, this);
		gtk_action_group_add_toggle_actions (this->base_group, toggleentries, n_toggleentries, this);
		gtk_ui_manager_insert_action_group (this->menu_manager, this->base_group, 0);
		gtk_action_group_add_actions (this->debug_group, debug_entries, n_debug_entries, this);
		gtk_ui_manager_insert_action_group (this->menu_manager, this->debug_group, 0);
		gtk_ui_manager_add_ui_from_string (this->menu_manager, layout, strlen(layout), &error);
		gtk_ui_manager_insert_action_group (this->menu_manager, this->dyn_group, 0);
		error=NULL;
		if (error) {
			g_message ("building menus failed: %s", error->message);
			g_error_free (error);
		}
		if (navit_get_attr(this->nav, attr_cursor, &attr)) {
			toggle_action = GTK_TOGGLE_ACTION(gtk_action_group_get_action(this->base_group, "CursorAction"));
			gtk_toggle_action_set_active(toggle_action, attr.u.num);
		} else {
			dbg(0, "Unable to locate CursorAction\n");
		}
		if (navit_get_attr(this->nav, attr_orientation, &attr)) {
			toggle_action = GTK_TOGGLE_ACTION(gtk_action_group_get_action(this->base_group, "OrientationAction"));
			gtk_toggle_action_set_active(toggle_action, attr.u.num);
		} else {
			dbg(0, "Unable to locate OrientationAction\n");
		}
		if (navit_get_attr(this->nav, attr_tracking, &attr)) {
			toggle_action = GTK_TOGGLE_ACTION(gtk_action_group_get_action(this->base_group, "TrackingAction"));
			gtk_toggle_action_set_active(toggle_action, attr.u.num);
		} else {
			dbg(0, "Unable to locate TrackingAction\n");
		}
	}
	widget=gtk_ui_manager_get_widget(this->menu_manager, path);
	GTK_WIDGET_UNSET_FLAGS (widget, GTK_CAN_FOCUS);
	if (widget_ret)
		*widget_ret=widget;
	if (! popup) {
		gtk_box_pack_start (GTK_BOX(this->vbox), widget, FALSE, FALSE, 0);
		gtk_widget_show (widget);
	} else {
		ret->widget=widget;
		meth->popup=popup_activate;
	}
	return ret;
}

struct menu_priv *
gui_gtk_toolbar_new(struct gui_priv *this, struct menu_methods *meth)
{
	return gui_gtk_ui_new(this, meth, "/ui/ToolBar", 0, NULL);
}

struct menu_priv *
gui_gtk_menubar_new(struct gui_priv *this, struct menu_methods *meth)
{
	return gui_gtk_ui_new(this, meth, "/ui/MenuBar", 0, &this->menubar);
}

struct menu_priv *
gui_gtk_popup_new(struct gui_priv *this, struct menu_methods *meth)
{
	return gui_gtk_ui_new(this, meth, "/ui/PopUp", 1, NULL);
}
