#include <string.h>
#include <gtk/gtk.h>
#include "graphics.h"
#include "gui_gtk.h"
#include "container.h"
#include "menu.h"
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


/* Create a list of entries which are passed to the Action constructor. 
 * This is a huge convenience over building Actions by hand. */
static GtkActionEntry entries[] = 
{
    { "DisplayMenuAction", NULL, "Display" },
    { "RouteMenuAction", NULL, "Route" },
    { "ZoomOutAction", GTK_STOCK_ZOOM_OUT, "ZoomOut", NULL, NULL, G_CALLBACK(zoom_out_action) },
    { "ZoomInAction", GTK_STOCK_ZOOM_IN, "ZoomIn", NULL, NULL, G_CALLBACK(zoom_in_action) },
    { "RefreshAction", GTK_STOCK_REFRESH, "Refresh", NULL, NULL, G_CALLBACK(refresh_action) },
    { "CursorAction", "cursor_icon","Cursor", NULL, NULL, G_CALLBACK(cursor_action) },
    { "OrientationAction", "orientation_icon", "Orientation", NULL, NULL, G_CALLBACK(orient_north_action) },
    { "DestinationAction", "flag_icon", "Destination", NULL, NULL, G_CALLBACK(destination_action) },
    { "QuitAction", GTK_STOCK_QUIT, "_Quit", "<control>Q",NULL, G_CALLBACK (quit_action) }
};

static guint n_entries = G_N_ELEMENTS (entries);

static GtkToggleActionEntry toggleentries[] = 
{
    { "CursorAction", NULL,"Cursor", NULL, NULL, G_CALLBACK(cursor_action),TRUE },
    { "OrientationAction", NULL, "Orientation", NULL, NULL, G_CALLBACK(orient_north_action),TRUE }
};

static guint n_toggleentries = G_N_ELEMENTS (toggleentries);

/* XPM */
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


/* XPM */
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


/* XPM */
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
		gtk_icon_factory_add (icon_factory, stock_icons[i].stockid, icon_set);
		gtk_icon_set_unref (icon_set);
#if 0
		icon_set = gtk_icon_set_new();
		icon_source = gtk_icon_source_new();
		gtk_icon_source_set_pixbuf(icon_source,pixbuf);
		gtk_icon_set_add_source(icon_set, icon_source);
		gtk_icon_source_set_state(icon_source,GTK_STATE_ACTIVE);
		gtk_icon_set_add_source(icon_set, icon_source);
		gtk_icon_source_free(icon_source);
		gtk_icon_factory_add(icon_factory, stock_icons[i].stockid, icon_set);
		gtk_icon_set_unref(icon_set);
#endif
	}

	gtk_icon_factory_add_default(icon_factory); 

	g_object_unref(icon_factory);
}

/* Implement a handler for GtkUIManager's "add_widget" signal. The UI manager
 * will emit this signal whenever it needs you to place a new widget it has. */
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
			<menu name=\"RouteMenu\" action=\"RouteMenuAction\">\
				<menuitem name=\"Refresh\" action=\"RefreshAction\" />\
				<menuitem name=\"Destination\" action=\"DestinationAction\" />\
				<menuitem name=\"Quit\" action=\"QuitAction\" />\
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
    GtkActionGroup      *action_group;          /* Packing group for our Actions */
    GtkUIManager        *menu_manager;          /* The magic widget! */
    GError              *error;                 /* For reporting exceptions or errors */

    struct action *this=g_new0(struct action, 1);

    this->gui=g_new0(struct action_gui, 1);
    this->gui->co=co;

    register_my_stock_icons();

    action_group = gtk_action_group_new ("BaseActions");
    menu_manager = gtk_ui_manager_new ();

    /* Pack up our objects:
     * vbox -> window
     * actions -> action_group
     * action_group -> menu_manager */
    gtk_action_group_add_actions (action_group, entries, n_entries, this);
    gtk_action_group_add_toggle_actions (action_group, toggleentries, n_toggleentries, this);
    gtk_ui_manager_insert_action_group (menu_manager, action_group, 0);

    /* Read in the UI from our XML file */
    error = NULL;
    gtk_ui_manager_add_ui_from_string (menu_manager, layout, strlen(layout), &error);

    if (error)
    {
	g_message ("building menus failed: %s", error->message);
	g_error_free (error);
    }

    /* This signal is necessary in order to place widgets from the UI manager
     * into the vbox */
    g_signal_connect ( menu_manager, "add_widget", G_CALLBACK (action_add_widget), *vbox);
}

