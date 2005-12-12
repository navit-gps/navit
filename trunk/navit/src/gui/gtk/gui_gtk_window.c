#include <stdio.h>
#include <gtk/gtk.h>
#include "coord.h"
#include "transform.h"
#include "container.h"
#include "gui_gtk.h"


extern void container_init_gra(struct container *co);

static struct container *
container_new(GtkWidget **widget)
{
	struct container *co=g_new0(struct container, 1);
	extern struct map_data *map_data_default;
	struct transformation *t=g_new0(struct transformation, 1);
	struct map_flags *flags=g_new0(struct map_flags, 1);
	struct graphics *gra;

	co->map_data=map_data_default;	
#if 1
	extern struct graphics *graphics_gtk_drawing_area_new(struct container *co, GtkWidget **widget);
	gra=graphics_gtk_drawing_area_new(co, widget);
#else
	extern struct graphics *graphics_gtk_gl_area_new(struct container *co, GtkWidget **widget);
	gra=graphics_gtk_gl_area_new(co, widget);
#endif
	co->gra=gra;
	co->trans=t;	
	co->flags=flags;

	return co;
}

struct container *
gui_gtk_window(int x, int y, int scale)
{
	GtkWidget *window,*map_widget;
	GtkWidget *vbox;
	GtkWidget *statusbar,*toolbar,*menu;
	struct container *co;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(window), 792, 547);
	gtk_window_set_title(GTK_WINDOW(window), "Map");
	gtk_widget_realize(window);
	vbox = gtk_vbox_new(FALSE, 0);
	co=container_new(&map_widget);
	
	transform_setup(co->trans, x, y, scale, 0);

	co->win=(struct window *) window;
	co->statusbar=gui_gtk_statusbar_new(&statusbar);
	co->toolbar=gui_gtk_toolbar_new(co, &toolbar);
	co->menu=gui_gtk_menu_new(co, &menu);
	gtk_box_pack_start(GTK_BOX(vbox), menu, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(vbox), statusbar, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(vbox), map_widget, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox);
	
	gtk_widget_show_all(window);
	container_init_gra(co);
	return co;
}


