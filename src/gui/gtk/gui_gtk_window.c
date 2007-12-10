#include <stdio.h>
#include <gdk/gdkkeysyms.h>
#if !defined(GDK_Book) || !defined(GDK_Calendar)
#include <X11/XF86keysym.h>
#endif
#include <libintl.h>
#include <gtk/gtk.h>
#include "navit.h"
#include "debug.h"
#include "gui.h"
#include "coord.h"
#include "point.h"
#include "plugin.h"
#include "graphics.h"
#include "gui_gtk.h"
#include "transform.h"

#ifndef GDK_Book
#define GDK_Book XF86XK_Book
#endif

#ifndef GDK_Calendar
#define GDK_Calendar XF86XK_Calendar
#endif

#define _(text) gettext(text)

static gboolean
keypress(GtkWidget *widget, GdkEventKey *event, struct gui_priv *this)
{
	int w,h;
	struct point p;
	if (event->type != GDK_KEY_PRESS)
		return FALSE;
	dbg(1,"keypress 0x%x\n", event->keyval);
        transform_get_size(navit_get_trans(this->nav), &w, &h);
	switch (event->keyval) {
	case GDK_KP_Enter:
		gtk_menu_shell_select_first(GTK_MENU_SHELL(this->menubar), TRUE);
		break;
	case GDK_Up:
		p.x=w/2;
		p.y=0;
		navit_set_center_screen(this->nav, &p);
		break;
	case GDK_Down:
		p.x=w/2;
		p.y=h;
		navit_set_center_screen(this->nav, &p);
		break;
	case GDK_Left:
		p.x=0;
		p.y=h/2;
		navit_set_center_screen(this->nav, &p);
		break;
	case GDK_Right:
		p.x=w;
		p.y=h/2;
		navit_set_center_screen(this->nav, &p);
		break;
	case GDK_Book:
		navit_zoom_in(this->nav, 2, NULL);
		break;
	case GDK_Calendar:
		navit_zoom_out(this->nav, 2, NULL);
		break;
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

	*meth=gui_gtk_methods;
	
	this=g_new0(struct gui_priv, 1);
	this->nav=nav;
	this->win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
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
	gdk_rgb_init();
	gtk_set_locale();


	plugin_register_gui_type("gtk", gui_gtk_new);
}
