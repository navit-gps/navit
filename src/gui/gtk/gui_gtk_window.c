#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#if !defined(GDK_Book) || !defined(GDK_Calendar)
#include <X11/XF86keysym.h>
#endif
#include <libintl.h>
#include <gtk/gtk.h>
#include "config.h"
#include "item.h"
#include "navit.h"
#include "debug.h"
#include "gui.h"
#include "coord.h"
#include "point.h"
#include "plugin.h"
#include "graphics.h"
#include "gui_gtk.h"
#include "transform.h"
#include "config.h"

#ifdef USE_HILDON
#include "hildon-widgets/hildon-defines.h"
#define KEY_ZOOM_IN HILDON_HARDKEY_INCREASE
#define KEY_ZOOM_OUT HILDON_HARDKEY_DECREASE
#define KEY_UP HILDON_HARDKEY_UP
#define KEY_DOWN HILDON_HARDKEY_DOWN
#define KEY_LEFT HILDON_HARDKEY_LEFT
#define KEY_RIGHT HILDON_HARDKEY_RIGHT
#else
#ifndef GDK_Book
#define GDK_Book XF86XK_Book
#endif
#ifndef GDK_Calendar
#define GDK_Calendar XF86XK_Calendar
#endif
#define KEY_ZOOM_IN GDK_Book
#define KEY_ZOOM_OUT GDK_Calendar 
#define KEY_UP GDK_Up
#define KEY_DOWN GDK_Down
#define KEY_LEFT GDK_Left
#define KEY_RIGHT GDK_Right
#endif

#define _(text) gettext(text)

static gboolean
keypress(GtkWidget *widget, GdkEventKey *event, struct gui_priv *this)
{
	int w,h;
	GtkToggleAction *action;
	gboolean *fullscreen;
	struct point p;
	if (event->type != GDK_KEY_PRESS)
		return FALSE;
	dbg(1,"keypress 0x%x\n", event->keyval);
        transform_get_size(navit_get_trans(this->nav), &w, &h);
	switch (event->keyval) {
	case GDK_KP_Enter:
		gtk_menu_shell_select_first(GTK_MENU_SHELL(this->menubar), TRUE);
		break;
	case KEY_UP:
		p.x=w/2;
		p.y=0;
		navit_set_center_screen(this->nav, &p);
		break;
	case KEY_DOWN:
		p.x=w/2;
		p.y=h;
		navit_set_center_screen(this->nav, &p);
		break;
	case KEY_LEFT:
		p.x=0;
		p.y=h/2;
		navit_set_center_screen(this->nav, &p);
		break;
	case KEY_RIGHT:
		p.x=w;
		p.y=h/2;
		navit_set_center_screen(this->nav, &p);
		break;
	case KEY_ZOOM_IN:
		navit_zoom_in(this->nav, 2, NULL);
		break;
	case KEY_ZOOM_OUT:
		navit_zoom_out(this->nav, 2, NULL);
		break;
	#ifdef USE_HILDON
	case HILDON_HARDKEY_FULLSCREEN:
		action = GTK_TOGGLE_ACTION (gtk_action_group_get_action (this->base_group, "FullscreenAction"));
			
		if ( gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action)))
		{
			fullscreen = 0;
		} else  {
			fullscreen = 1;
		}
		gtk_toggle_action_set_active (action, fullscreen);
		break;
	#endif /*HILDON*/
	default:
		return FALSE;
	}
	return TRUE;
}

static int
gui_gtk_set_graphics(struct gui_priv *this, struct graphics *gra)
{
	GtkWidget *graphics;

	graphics=graphics_get_data(gra, "gtk_widget");
	if (! graphics)
		return 1;
	GTK_WIDGET_SET_FLAGS (graphics, GTK_CAN_FOCUS);
	gtk_widget_set_sensitive(graphics, TRUE);
	g_signal_connect(G_OBJECT(graphics), "key-press-event", G_CALLBACK(keypress), this);
	gtk_box_pack_end(GTK_BOX(this->vbox), graphics, TRUE, TRUE, 0);
	gtk_widget_show_all(graphics);
	gtk_widget_grab_focus(graphics);

	return 0;
}

static void
gui_gtk_add_bookmark_do(struct gui_priv *gui)
{
	navit_add_bookmark(gui->nav, &gui->dialog_coord, gtk_entry_get_text(GTK_ENTRY(gui->dialog_entry)));
	gtk_widget_destroy(gui->dialog_win);
}

static int
gui_gtk_add_bookmark(struct gui_priv *gui, struct pcoord *c, char *description)
{
        GtkWidget *button_ok,*button_cancel,*label,*vbox,*hbox;

	gui->dialog_coord=*c;	
	gui->dialog_win=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	vbox=gtk_vbox_new(FALSE, 0);
	gtk_container_add (GTK_CONTAINER (gui->dialog_win), vbox);
	gtk_window_set_title(GTK_WINDOW(gui->dialog_win),_("Add Bookmark"));
	gtk_window_set_transient_for(GTK_WINDOW(gui->dialog_win), GTK_WINDOW(gui->win));
	gtk_window_set_modal(GTK_WINDOW(gui->dialog_win), TRUE);
	label=gtk_label_new(_("Name"));
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);
	gui->dialog_entry=gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(gui->dialog_entry), description);
	gtk_box_pack_start(GTK_BOX(vbox), gui->dialog_entry, TRUE, TRUE, 0);
	hbox=gtk_hbox_new(FALSE, 0);
	button_ok = gtk_button_new_from_stock (GTK_STOCK_OK);
	gtk_box_pack_start(GTK_BOX(hbox), button_ok, TRUE, TRUE, 10);
	button_cancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
	gtk_box_pack_start(GTK_BOX(hbox), button_cancel, TRUE, TRUE, 10);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 10);
	gtk_widget_show_all(gui->dialog_win);
	GTK_WIDGET_SET_FLAGS (button_ok, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(button_ok);
	g_signal_connect_swapped (G_OBJECT (button_cancel), "clicked", G_CALLBACK (gtk_widget_destroy), G_OBJECT (gui->dialog_win));
	g_signal_connect_swapped (G_OBJECT (gui->dialog_entry), "activate", G_CALLBACK (gui_gtk_add_bookmark_do), gui);

	g_signal_connect_swapped(G_OBJECT (button_ok), "clicked", G_CALLBACK (gui_gtk_add_bookmark_do), gui);

	return 1;
}

struct gui_methods gui_gtk_methods = {
	gui_gtk_menubar_new,
	gui_gtk_toolbar_new,
	gui_gtk_statusbar_new,
	gui_gtk_popup_new,
	gui_gtk_set_graphics,
	NULL,
	gui_gtk_datawindow_new,
	gui_gtk_add_bookmark,
};

static gboolean
gui_gtk_delete(GtkWidget *widget, GdkEvent *event, struct navit *nav)
{
	navit_destroy(nav);

	return TRUE;
}

static struct gui_priv *
gui_gtk_new(struct navit *nav, struct gui_methods *meth, struct attr **attrs) 
{
	struct gui_priv *this;
	int w=792, h=547;
	char *cp = getenv("NAVIT_XID");
	unsigned xid = 0;
	struct attr *menubar, *toolbar, *statusbar;

	if (cp) {
		xid = strtol(cp, NULL, 0);
	}

	menubar = attr_search(attrs, NULL, attr_menubar);
	if (menubar && menubar->u.num == 0) {
		gui_gtk_methods.menubar_new = NULL;
	}
	toolbar = attr_search(attrs, NULL, attr_toolbar);
	if (toolbar && toolbar->u.num == 0) {
		gui_gtk_methods.toolbar_new = NULL;
	}
	statusbar = attr_search(attrs, NULL, attr_statusbar);
	if (statusbar && statusbar->u.num == 0) {
		gui_gtk_methods.statusbar_new = NULL;
	}
	*meth=gui_gtk_methods;

	this=g_new0(struct gui_priv, 1);
	this->nav=nav;
	if (!xid) 
		this->win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	else
		this->win = gtk_plug_new(xid);

	g_signal_connect(G_OBJECT(this->win), "delete-event", G_CALLBACK(gui_gtk_delete), nav);
	this->vbox = gtk_vbox_new(FALSE, 0);
	gtk_window_set_default_size(GTK_WINDOW(this->win), w, h);
	gtk_window_set_title(GTK_WINDOW(this->win), "Navit");
	gtk_widget_realize(this->win);
	gtk_container_add(GTK_CONTAINER(this->win), this->vbox);
	gtk_widget_show_all(this->win);

	return this;
}

static int gtk_argc;
static char **gtk_argv={NULL};

void
plugin_init(void)
{
	gtk_init(&gtk_argc, &gtk_argv);
	gtk_set_locale();


	plugin_register_gui_type("gtk", gui_gtk_new);
}
