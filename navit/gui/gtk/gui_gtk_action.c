/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include <string.h>
#include <gtk/gtk.h>
#include "navit.h"
#include "gui_gtk.h"
#ifdef __APPLE__
#include "navit/menu.h"
#else
#include "menu.h"
#endif
#include "coord.h"
#include "item.h"
#include "attr.h"
#include "callback.h"
#include "debug.h"
#include "destination.h"
#include "navit_nls.h"
#include "gui_gtk_poi.h"

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

static void zoom_in_action(GtkWidget *w, struct gui_priv *gui, void *dummy) {
    navit_zoom_in(gui->nav, 2, NULL);
}

static void zoom_out_action(GtkWidget *w, struct gui_priv *gui, void *dummy) {
    navit_zoom_out(gui->nav, 2, NULL);
}

static void refresh_action(GtkWidget *w, struct gui_priv *gui, void *dummy) {
    navit_draw(gui->nav);
}

// Forward declarations, these should not be visible outside the GUI, so
// they are not in the header files, but here
void gui_gtk_datawindow_set_button(struct datawindow_priv *this_, GtkWidget *btn);
void gui_gtk_datawindow_destroy(struct datawindow_priv *win);

static void roadbook_action(GtkWidget *w, struct gui_priv *gui, void *dummy) {

    if (! gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(w))) {
        gui_gtk_datawindow_destroy(gui->datawindow);
    } else {
        navit_window_roadbook_new(gui->nav);
        if (gui->datawindow) {
            gui_gtk_datawindow_set_button(gui->datawindow, w);
        }
    }
}

static void autozoom_action(GtkWidget *w, struct gui_priv *gui, void *dummy) {
    struct attr autozoom_attr;

    autozoom_attr.type = attr_autozoom_active;
    if (! gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(w))) {
        autozoom_attr.u.num = 0;
    } else {
        autozoom_attr.u.num = 1;
    }

    navit_set_attr(gui->nav, &autozoom_attr);
}

static void cursor_action(GtkWidget *w, struct gui_priv *gui, void *dummy) {
    struct attr attr;

    attr.type=attr_cursor;
    attr.u.num=gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(w));
    if(!navit_set_attr(gui->nav, &attr)) {
        dbg(lvl_error, "Failed to set attr_cursor");
    }
}

static void tracking_action(GtkWidget *w, struct gui_priv *gui, void *dummy) {
    struct attr attr;

    attr.type=attr_tracking;
    attr.u.num=gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(w));
    if(!navit_set_attr(gui->nav, &attr)) {
        dbg(lvl_error, "Failed to set attr_tracking");
    }
}

/** @brief Toggles the ability to follow the vehicle at the
 *  cursor. Suitable for use in the GTK menu as below.
 *
 *  @param GtkWidget is the generic storage type for widgets.
 *  @param gui The gui. I think, I'm new here.
 *  @param dummy Ignore the pointer behind the curtain.
 *  @return void
 */

static void follow_vehicle_action(GtkWidget *w, struct gui_priv *gui, void *dummy) {
    struct attr attr;

    attr.type=attr_follow_cursor;
    attr.u.num=gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(w));
    if(!navit_set_attr(gui->nav, &attr)) {
        dbg(lvl_error, "Failed to set attr_follow_gps");
    }
}

static void orient_north_action(GtkWidget *w, struct gui_priv *gui, void *dummy) {
    struct attr attr;

    attr.type=attr_orientation;
    attr.u.num=gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(w)) ? 0:-1;
    if(!navit_set_attr(gui->nav, &attr)) {
        dbg(lvl_error, "Failed to set attr_orientation");
    }
}

static void window_fullscreen_action(GtkWidget *w, struct gui_priv *gui, void *dummy) {
    if(gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(w)))
        gtk_window_fullscreen(GTK_WINDOW(gui->win));
    else
        gtk_window_unfullscreen(GTK_WINDOW(gui->win));
}

#include <stdlib.h>
#include "point.h"
#include "transform.h"

static void info_action(GtkWidget *w, struct gui_priv *gui, void *dummy) {
    char buffer[512];
    int mw,mh;
    struct coord lt, rb;
    struct point p;
    struct transformation *t;
    int retval;

    t=navit_get_trans(gui->nav);
    transform_get_size(t, &mw, &mh);
    p.x=0;
    p.y=0;
    transform_reverse(t, &p, &lt);
    p.x=mw;
    p.y=mh;
    transform_reverse(t, &p, &rb);

    sprintf(buffer,"./info.sh %d,%d 0x%x,0x%x 0x%x,0x%x", mw, mh, lt.x, lt.y, rb.x, rb.y);
    retval=system(buffer);
    dbg(lvl_debug,"calling %s returned %d", buffer, retval);

}


static void route_clear_action(GtkWidget *w, struct gui_priv *gui, void *dummy) {
    navit_set_destination(gui->nav, NULL, NULL, 0);
}

static void poi_search_action(GtkWidget *w, struct gui_priv *gui, void *dummy) {
    gtk_gui_poi(gui->nav);
}

static void destination_action(GtkWidget *w, struct gui_priv *gui, void *dummy) {
    destination_address(gui->nav);
}

static void quit_action (GtkWidget *w, struct gui_priv *gui, void *dummy) {
    navit_destroy(gui->nav);
    exit(0);
}

static GtkActionEntry entries[] = {
    /* TRANSLATORS: These texts are for menu items in GTK GUI. The _ indicates the mnemonic key (=underlined character) for this menu item. Please place the _ before a suitable character in the translation (or omit it if no mnemonic key is desired). */
    { "DisplayMenuAction", NULL, _n("_Display") },
    { "RouteMenuAction", NULL, _n("_Route") },
    { "FormerDestinationMenuAction", NULL, _n("_Former Destinations") },
    { "BookmarkMenuAction", NULL, _n("_Bookmarks") },
    { "MapMenuAction", NULL, _n("_Map") },
    { "LayoutMenuAction", NULL, _n("_Layout") },
    { "ProjectionMenuAction", NULL, _n("_Projection") },
    { "VehicleMenuAction", NULL, _n("_Vehicle") },
    { "ZoomOutAction", GTK_STOCK_ZOOM_OUT, _n("Zoom_Out"), "<control>minus", _n("Decrease zoom level"), G_CALLBACK(zoom_out_action) },
    { "ZoomInAction", GTK_STOCK_ZOOM_IN, _n("Zoom_In"), "<control>plus", _n("Increase zoom level"), G_CALLBACK(zoom_in_action) },
    { "RefreshAction", GTK_STOCK_REFRESH, _n("_Recalculate"), "<control>R", _n("Redraw map"), G_CALLBACK(refresh_action) },
#ifdef GTK_STOCK_INFO
    { "InfoAction", GTK_STOCK_INFO, _n("_Info"), NULL, NULL, G_CALLBACK(info_action) },
#else
    { "InfoAction", NULL, _n("_Info"), NULL, NULL, G_CALLBACK(info_action) },
#endif /*GTK_STOCK_INFO*/
    { "DestinationAction", "flag_icon", _n("Set _destination"), "<control>D", _n("Opens address search dialog"), G_CALLBACK(destination_action) },
    { "POIAction", "flag_icon", _n("_POI search"), "<control>P", _n("Opens POI search dialog"), G_CALLBACK(poi_search_action) },
    { "RouteClearAction", NULL, _n("_Stop Navigation"), "<control>S", NULL, G_CALLBACK(route_clear_action) },
    { "Test", NULL, _n("Test"), NULL, NULL, G_CALLBACK(destination_action) },
    { "QuitAction", GTK_STOCK_QUIT, _n("_Quit"), "<control>Q",_n("Quit the application"), G_CALLBACK (quit_action) }
};

static guint n_entries = G_N_ELEMENTS (entries);

static GtkToggleActionEntry toggleentries[] = {
    { "CursorAction", "cursor_icon",_n("Show position _cursor"), NULL, NULL, G_CALLBACK(cursor_action),TRUE },
    { "TrackingAction", NULL,_n("_Lock on Road"), NULL, NULL, G_CALLBACK(tracking_action),TRUE },
    { "FollowVehicleAction", NULL,_n("_Follow Vehicle"), NULL, NULL, G_CALLBACK(follow_vehicle_action),TRUE },
    { "OrientationAction", "orientation_icon", _n("_Keep orientation to the North"), NULL, _n("Switches map orientation to the north or the vehicle"), G_CALLBACK(orient_north_action),FALSE },
    { "RoadbookAction", GTK_STOCK_JUSTIFY_FILL, _n("_Roadbook"), "<control>B", _n("Show/hide route description"), G_CALLBACK(roadbook_action), FALSE },
    { "AutozoomAction", GTK_STOCK_ZOOM_FIT, _n("_Autozoom"), "<control>A", _n("Enable/disable automatic zoom level changing"), G_CALLBACK(autozoom_action), FALSE },
#ifdef GTK_STOCK_FULLSCREEN
    { "FullscreenAction",GTK_STOCK_FULLSCREEN, _n("_Fullscreen"), "<control>F", NULL, G_CALLBACK(window_fullscreen_action), FALSE }
#else
    { "FullscreenAction", NULL, _n("_Fullscreen"), "<control>F", NULL, G_CALLBACK(window_fullscreen_action), FALSE }
#endif /*GTK_STOCK_FULLSCREEN*/
};

static guint n_toggleentries = G_N_ELEMENTS (toggleentries);

static GtkActionEntry debug_entries[] = {
    { "DataMenuAction", NULL, _n("Data") },
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
    "                      "
};


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
    "                      "
};


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
    "+                     "
};



static struct {
    gchar *stockid;
    const char **icon_xpm;
} stock_icons[] = {
    {"cursor_icon", cursor_xpm },
    {"orientation_icon", north_xpm },
    {"flag_icon", flag_xpm }
};


static gint n_stock_icons = G_N_ELEMENTS (stock_icons);


static void register_my_stock_icons (void) {
    GtkIconFactory *icon_factory;
    GtkIconSet *icon_set;
    GdkPixbuf *pixbuf;
    gint i;

    icon_factory = gtk_icon_factory_new ();

    for (i = 0; i < n_stock_icons; i++) {
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
				<menuitem name=\"Follow Vehicle\" action=\"FollowVehicleAction\"/>\
				<menuitem name=\"Orientation\" action=\"OrientationAction\"/>\
				<menuitem name=\"Roadbook\" action=\"RoadbookAction\"/>\
				<menuitem name=\"Autozoom\" action=\"AutozoomAction\"/>\
				<menuitem name=\"Fullscreen\" action=\"FullscreenAction\"/>\
				<menuitem name=\"Quit\" action=\"QuitAction\" />\
				<placeholder name=\"RouteMenuAdditions\" />\
			</menu>\
			<menu name=\"Data\" action=\"DataMenuAction\">\
				<placeholder name=\"DataMenuAdditions\" />\
			</menu>\
			<menu name=\"Route\" action=\"RouteMenuAction\">\
				<menuitem name=\"Refresh\" action=\"RefreshAction\" />\
				<menuitem name=\"Destination\" action=\"DestinationAction\" />\
				<menuitem name=\"POI\" action=\"POIAction\" />\
				<menuitem name=\"Clear\" action=\"RouteClearAction\" />\
				<menu name=\"FormerDestinations\" action=\"FormerDestinationMenuAction\">\
					<placeholder name=\"FormerDestinationMenuAdditions\" />\
				</menu>\
				<menu name=\"Bookmarks\" action=\"BookmarkMenuAction\">\
					<placeholder name=\"BookmarkMenuAdditions\" />\
				</menu>\
				<placeholder name=\"RouteMenuAdditions\" />\
			</menu>\
			<menu name=\"Map\" action=\"MapMenuAction\">\
				<menu name=\"Layout\" action=\"LayoutMenuAction\">\
					<placeholder name=\"LayoutMenuAdditions\" />\
				</menu>\
				<menu name=\"Projection\" action=\"ProjectionMenuAction\">\
					<placeholder name=\"ProjectionMenuAdditions\" />\
				</menu>\
				<menu name=\"Vehicle\" action=\"VehicleMenuAction\">\
					<placeholder name=\"VehicleMenuAdditions\" />\
				</menu>\
				<placeholder name=\"MapMenuAdditions\" />\
			</menu>\
		</menubar>\
	 	<toolbar name=\"ToolBar\" action=\"BaseToolbar\" action=\"BaseToolbarAction\">\
			<placeholder name=\"ToolItems\">\
				<separator/>\
				<toolitem name=\"Zoom in\" action=\"ZoomInAction\"/>\
				<toolitem name=\"Zoom out\" action=\"ZoomOutAction\"/>\
				<toolitem name=\"Refresh\" action=\"RefreshAction\"/>\
				<!-- <toolitem name=\"Cursor\" action=\"CursorAction\"/> -->\
				<toolitem name=\"Orientation\" action=\"OrientationAction\"/>\
				<toolitem name=\"Destination\" action=\"DestinationAction\"/>\
				<toolitem name=\"POI\" action=\"POIAction\"/>\
				<!-- <toolitem name=\"Info\" action=\"InfoAction\"/> -->\
				<toolitem name=\"Roadbook\" action=\"RoadbookAction\"/>\
				<toolitem name=\"Autozoom\" action=\"AutozoomAction\"/>\
				<toolitem name=\"Quit\" action=\"QuitAction\"/>\
				<separator/>\
			</placeholder>\
		</toolbar>\
		<popup name=\"PopUp\">\
		</popup>\
	</ui>";


static void activate(void *dummy, struct menu_priv *menu) {
    if (menu->cb)
        callback_call_0(menu->cb);
}

static struct menu_methods menu_methods;

static struct menu_priv *add_menu(struct menu_priv *menu, struct menu_methods *meth, char *name, enum menu_type type,
                                  struct callback *cb) {
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
        ret->merge_id=gtk_ui_manager_new_merge_id(menu->gui->ui_manager);
        gtk_ui_manager_add_ui( menu->gui->ui_manager, ret->merge_id, menu->path, dynname, dynname,
                               type == menu_type_submenu ? GTK_UI_MANAGER_MENU : GTK_UI_MANAGER_MENUITEM, FALSE);
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

static void remove_menu(struct menu_priv *item, int recursive) {

    if (recursive) {
        struct menu_priv *next,*child=item->child;
        while (child) {
            next=child->sibling;
            remove_menu(child, recursive);
            child=next;
        }
    }
    if (item->action) {
        gtk_ui_manager_remove_ui(item->gui->ui_manager, item->merge_id);
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

static void set_toggle(struct menu_priv *menu, int active) {
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(menu->action), active);
}

static  int get_toggle(struct menu_priv *menu) {
    return gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(menu->action));
}

static struct menu_methods menu_methods = {
#if 1
    add_menu,
    set_toggle,
    get_toggle,
#else
    NULL,
    NULL,
    NULL
#endif
};


static void popup_deactivate(GtkWidget *widget, struct menu_priv *menu) {
    g_signal_handler_disconnect(widget, menu->handler_id);
    remove_menu(menu, 1);
}

static void popup_activate(struct menu_priv *menu) {
#ifdef _WIN32
    menu->widget=gtk_ui_manager_get_widget(menu->gui->ui_manager, menu->path );
#endif
    gtk_menu_popup(GTK_MENU(menu->widget), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time ());
    menu->handler_id=g_signal_connect(menu->widget, "selection-done", G_CALLBACK(popup_deactivate), menu);
}

void gui_gtk_ui_init(struct gui_priv *this) {
    GError *error = NULL;
    struct attr attr;
    GtkToggleAction *toggle_action;

    this->base_group = gtk_action_group_new ("BaseActions");
    this->debug_group = gtk_action_group_new ("DebugActions");
    this->dyn_group = gtk_action_group_new ("DynamicActions");
    register_my_stock_icons();
    this->ui_manager = gtk_ui_manager_new ();
    gtk_action_group_set_translation_domain(this->base_group,"navit");
    gtk_action_group_set_translation_domain(this->debug_group,"navit");
    gtk_action_group_set_translation_domain(this->dyn_group,"navit");
    gtk_action_group_add_actions (this->base_group, entries, n_entries, this);
    gtk_action_group_add_toggle_actions (this->base_group, toggleentries, n_toggleentries, this);
    gtk_ui_manager_insert_action_group (this->ui_manager, this->base_group, 0);
    gtk_action_group_add_actions (this->debug_group, debug_entries, n_debug_entries, this);
    gtk_ui_manager_insert_action_group (this->ui_manager, this->debug_group, 0);
    gtk_ui_manager_add_ui_from_string (this->ui_manager, layout, strlen(layout), &error);
    gtk_ui_manager_insert_action_group (this->ui_manager, this->dyn_group, 0);
    if (error) {
        g_message ("building menus failed: %s", error->message);
        g_error_free (error);
    }
    if (navit_get_attr(this->nav, attr_cursor, &attr, NULL)) {
        toggle_action = GTK_TOGGLE_ACTION(gtk_action_group_get_action(this->base_group, "CursorAction"));
        gtk_toggle_action_set_active(toggle_action, attr.u.num);
    }
    if (navit_get_attr(this->nav, attr_follow_cursor, &attr, NULL)) {
        toggle_action = GTK_TOGGLE_ACTION(gtk_action_group_get_action(this->base_group, "FollowVehicleAction"));
        gtk_toggle_action_set_active(toggle_action, attr.u.num);
    }
    if (navit_get_attr(this->nav, attr_orientation, &attr, NULL)) {
        toggle_action = GTK_TOGGLE_ACTION(gtk_action_group_get_action(this->base_group, "OrientationAction"));
        gtk_toggle_action_set_active(toggle_action, attr.u.num != -1);
    }
    if (navit_get_attr(this->nav, attr_tracking, &attr, NULL)) {
        toggle_action = GTK_TOGGLE_ACTION(gtk_action_group_get_action(this->base_group, "TrackingAction"));
        gtk_toggle_action_set_active(toggle_action, attr.u.num);
    }
    toggle_action = GTK_TOGGLE_ACTION(gtk_action_group_get_action(this->base_group, "RoadbookAction"));
    gtk_toggle_action_set_active(toggle_action, 0);

    if (navit_get_attr(this->nav, attr_autozoom_active, &attr, NULL)) {
        toggle_action = GTK_TOGGLE_ACTION(gtk_action_group_get_action(this->base_group, "AutozoomAction"));
        gtk_toggle_action_set_active(toggle_action, attr.u.num);
    }

}

static struct menu_priv *gui_gtk_ui_new (struct gui_priv *this, struct menu_methods *meth, char *path, int popup,
        GtkWidget **widget_ret) {
    struct menu_priv *ret;
    GtkWidget *widget;

    *meth=menu_methods;
    ret=g_new0(struct menu_priv, 1);
    ret->path=g_strdup(path);
    ret->gui=this;

    widget=gtk_ui_manager_get_widget(this->ui_manager, path);
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

#if 0
struct menu_priv *
gui_gtk_menubar_new(struct gui_priv *this, struct menu_methods *meth) {
    return gui_gtk_ui_new(this, meth, "/ui/MenuBar", 0, &this->menubar);
}
#endif

struct menu_priv *
gui_gtk_popup_new(struct gui_priv *this, struct menu_methods *meth) {
    return gui_gtk_ui_new(this, meth, "/ui/PopUp", 1, NULL);
}
