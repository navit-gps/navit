#include <string.h>
#include <gtk/gtk.h>
#include "graphics.h"
#include "gui_gtk.h"
#include "container.h"
#include "menu.h"
#include "data_window.h"
#include "coord.h"
#include "destination.h"

struct action_gui {
	struct container *co;
};

#include "action.h"


/* Create callbacks that implement our Actions */

static void
zoom_in_action(GtkWidget *w, struct action *ac)
{
	unsigned long scale;
	graphics_get_view(ac->gui->co, NULL, NULL, &scale);
	scale/=2;
	if (scale < 1)
		scale=1;
	graphics_set_view(ac->gui->co, NULL, NULL, &scale);
}

static void
zoom_out_action(GtkWidget *w, struct action *ac)
{
	unsigned long scale;
	graphics_get_view(ac->gui->co, NULL, NULL, &scale);
	scale*=2;
	graphics_set_view(ac->gui->co, NULL, NULL, &scale);
}

static void
refresh_action(GtkWidget *w, struct action *ac)
{
	menu_route_update(ac->gui->co);
}

static void
cursor_action(GtkWidget *w, struct action *ac)
{
	if (ac->gui->co->flags->track) {
		ac->gui->co->flags->track=0;
	} else {
		ac->gui->co->flags->track=1;
	}
}

static void
orient_north_action(GtkWidget *w, struct action *ac)
{
	if (ac->gui->co->flags->orient_north) {
		ac->gui->co->flags->orient_north=0;
	} else {
		ac->gui->co->flags->orient_north=1;
	}
}

static void
destination_action(GtkWidget *w, struct action *ac)
{
	destination_address(ac->gui->co);
}


static void     
quit_action (GtkWidget *w, struct action *ac)
{
    gtk_main_quit();
}

static void
visible_blocks_action(GtkWidget *w, struct container *co)
{
	co->data_window[data_window_type_block]=data_window("Visible Blocks",co->win,NULL);
	graphics_redraw(co);
}

static void
visible_towns_action(GtkWidget *w, struct container *co)
{
	co->data_window[data_window_type_town]=data_window("Visible Towns",co->win,NULL);
	graphics_redraw(co);
}

static void
visible_polys_action(GtkWidget *w, struct container *co)
{
	co->data_window[data_window_type_street]=data_window("Visible Polys",co->win,NULL);
	graphics_redraw(co);
}

static void
visible_streets_action(GtkWidget *w, struct container *co)
{
	co->data_window[data_window_type_street]=data_window("Visible Streets",co->win,NULL);
	graphics_redraw(co);
}

static void
visible_points_action(GtkWidget *w, struct container *co)
{
	co->data_window[data_window_type_point]=data_window("Visible Points",co->win,NULL);
	graphics_redraw(co);
}


static GtkActionEntry entries[] = 
{
    { "DisplayMenuAction", NULL, "Display" },
    { "RouteMenuAction", NULL, "Route" },
    { "ZoomOutAction", GTK_STOCK_ZOOM_OUT, "ZoomOut", NULL, NULL, G_CALLBACK(zoom_out_action) },
    { "ZoomInAction", GTK_STOCK_ZOOM_IN, "ZoomIn", NULL, NULL, G_CALLBACK(zoom_in_action) },
    { "RefreshAction", GTK_STOCK_REFRESH, "Refresh", NULL, NULL, G_CALLBACK(refresh_action) },
    { "DestinationAction", "flag_icon", "Destination", NULL, NULL, G_CALLBACK(destination_action) },
    { "QuitAction", GTK_STOCK_QUIT, "_Quit", "<control>Q",NULL, G_CALLBACK (quit_action) }
};

static guint n_entries = G_N_ELEMENTS (entries);

static GtkToggleActionEntry toggleentries[] = 
{
    { "CursorAction", "cursor_icon","Cursor", NULL, NULL, G_CALLBACK(cursor_action),TRUE },
    { "OrientationAction", "orientation_icon", "Orientation", NULL, NULL, G_CALLBACK(orient_north_action),TRUE }
};

static guint n_toggleentries = G_N_ELEMENTS (toggleentries);

static GtkActionEntry debug_entries[] = 
{
    { "DataMenuAction", NULL, "Data" },
    { "VisibleBlocksAction", NULL, "VisibleBlocks", NULL, NULL, G_CALLBACK(visible_blocks_action) },
    { "VisibleTownsAction", NULL, "VisibleTowns", NULL, NULL, G_CALLBACK(visible_towns_action) },
    { "VisiblePolysAction", NULL, "VisiblePolys", NULL, NULL, G_CALLBACK(visible_polys_action) },
    { "VisibleStreetsAction", NULL, "VisibleStreets", NULL, NULL, G_CALLBACK(visible_streets_action) },
    { "VisiblePointsAction", NULL, "VisiblePoints", NULL, NULL, G_CALLBACK(visible_points_action) }
};

static guint n_debug_entries = G_N_ELEMENTS (debug_entries);


static gchar * cursor_xpm[] = {
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


static gchar * north_xpm[] = {
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


static gchar * flag_xpm[] = {
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
	gchar **icon_xpm;
} stock_icons[] = {
	{"cursor_icon", &cursor_xpm },
	{"orientation_icon", &north_xpm },
	{"flag_icon", &flag_xpm }
};

static gint n_stock_icons = G_N_ELEMENTS (stock_icons);


static void
register_my_stock_icons (void)
{
	GtkIconFactory *icon_factory;
	GtkIconSet *icon_set; 
	GtkIconSource *icon_source;
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

static void
action_add_widget (GtkUIManager *ui, GtkWidget *widget, GtkContainer *container)
{
    gtk_box_pack_start (GTK_BOX (container), widget, FALSE, FALSE, 0);
    gtk_widget_show (widget);
}

static char layout[] =
	"<ui>\
		<menubar>\
			<menu name=\"DisplayMenu\" action=\"DisplayMenuAction\">\
				<menuitem name=\"Zoom in\" action=\"ZoomInAction\" />\
				<menuitem name=\"Zoom out\" action=\"ZoomOutAction\" />\
				<menuitem name=\"Cursor\" action=\"CursorAction\"/>\
				<menuitem name=\"Orientation\" action=\"OrientationAction\"/>\
				<menuitem name=\"Quit\" action=\"QuitAction\" />\
				<placeholder name=\"RouteMenuAdditions\" />\
			</menu>\
			<menu name=\"DataMenu\" action=\"DataMenuAction\">\
				<menuitem name=\"Visible Blocks\" action=\"VisibleBlocksAction\" />\
				<menuitem name=\"Visible Towns\" action=\"VisibleTownsAction\" />\
				<menuitem name=\"Visible Polys\" action=\"VisiblePolysAction\" />\
				<menuitem name=\"Visible Streets\" action=\"VisibleStreetsAction\" />\
				<menuitem name=\"Visible Points\" action=\"VisiblePointsAction\" />\
				<placeholder name=\"DataMenuAdditions\" />\
			</menu>\
			<menu name=\"RouteMenu\" action=\"RouteMenuAction\">\
				<menuitem name=\"Refresh\" action=\"RefreshAction\" />\
				<menuitem name=\"Destination\" action=\"DestinationAction\" />\
				<placeholder name=\"RouteMenuAdditions\" />\
			</menu>\
		</menubar>\
	 	<toolbar action=\"BaseToolbar\" action=\"BaseToolbarAction\">\
			<placeholder name=\"ToolItems\">\
				<separator/>\
				<toolitem name=\"Zoom in\" action=\"ZoomInAction\"/>\
				<toolitem name=\"Zoom out\" action=\"ZoomOutAction\"/>\
				<toolitem name=\"Refresh\" action=\"RefreshAction\"/>\
				<toolitem name=\"Cursor\" action=\"CursorAction\"/>\
				<toolitem name=\"Orientation\" action=\"OrientationAction\"/>\
				<toolitem name=\"Destination\" action=\"DestinationAction\"/>\
				<toolitem name=\"Quit\" action=\"QuitAction\"/>\
				<separator/>\
			</placeholder>\
		</toolbar>\
	</ui>";
			

void
gui_gtk_actions_new(struct container *co, GtkWidget **vbox)
{
    GtkActionGroup      *base_group,*debug_group;
    GtkUIManager        *menu_manager;
    GError              *error;

    struct action *this=g_new0(struct action, 1);

    this->gui=g_new0(struct action_gui, 1);
    this->gui->co=co;

    register_my_stock_icons();

    base_group = gtk_action_group_new ("BaseActions");
    debug_group = gtk_action_group_new ("DebugActions");
    menu_manager = gtk_ui_manager_new ();

    /* Pack up our objects:
     * vbox -> window
     * actions -> action_group
     * action_group -> menu_manager */
    gtk_action_group_add_actions (base_group, entries, n_entries, this);
    gtk_action_group_add_toggle_actions (base_group, toggleentries, n_toggleentries, this);
    gtk_ui_manager_insert_action_group (menu_manager, base_group, 0);

    gtk_action_group_add_actions (debug_group, debug_entries, n_debug_entries, co);
    gtk_ui_manager_insert_action_group (menu_manager, debug_group, 0);
    
    error = NULL;
    gtk_ui_manager_add_ui_from_string (menu_manager, layout, strlen(layout), &error);

    if (error)
    {
	g_message ("building menus failed: %s", error->message);
	g_error_free (error);
    }

    g_signal_connect ( menu_manager, "add_widget", G_CALLBACK (action_add_widget), *vbox);
}

