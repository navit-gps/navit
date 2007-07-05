#include <stdio.h>
#include <gdk/gdkkeysyms.h>
#if !defined(GDK_Book) || !defined(GDK_Calendar)
#include <X11/XF86keysym.h>
#endif
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
		navit_zoom_in(this->nav, 2);
		break;
	case GDK_Calendar:
		navit_zoom_out(this->nav, 2);
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

struct gui_methods gui_gtk_methods = {
	gui_gtk_menubar_new,
	gui_gtk_toolbar_new,
	gui_gtk_statusbar_new,
	gui_gtk_popup_new,
	gui_gtk_set_graphics,
};

static struct gui_priv *
gui_gtk_new(struct navit *nav, struct gui_methods *meth, int w, int h) 
{
	struct gui_priv *this;

	*meth=gui_gtk_methods;

	this=g_new0(struct gui_priv, 1);
	this->nav=nav;
	this->win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	this->vbox = gtk_vbox_new(FALSE, 0);
	gtk_window_set_default_size(GTK_WINDOW(this->win), w, h);
	gtk_window_set_title(GTK_WINDOW(this->win), "Navit");
	gtk_widget_realize(this->win);
	gtk_container_add(GTK_CONTAINER(this->win), this->vbox);
	gtk_widget_show_all(this->win);

	return this;
}

static int gtk_argc;
static char *gtk_argv[]={NULL};

void
plugin_init(void)
{
	gtk_init(&gtk_argc, &gtk_argv);
	gdk_rgb_init();
	gtk_set_locale();


	plugin_register_gui_type("gtk", gui_gtk_new);
}
