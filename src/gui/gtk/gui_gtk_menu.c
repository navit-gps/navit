#define GTK_ENABLE_BROKEN
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <gtk/gtk.h>
#include "coord.h"
#include "graphics.h"
#include "data_window.h"
#include "route.h"
#include "cursor.h"
#include "command.h"
#include "transform.h"
#include "block.h"
#include "street.h"
#include "statusbar.h"
#include "destination.h"
#include "main.h"
#include "container.h"
#include "gui_gtk.h"

struct menu_gui {
        struct container *co;
};

#include "menu.h"

extern struct coord current_pos;

extern struct data_window *navigation_window;

struct destination {
	struct container *co;
	char *text;
	struct coord pos;
};

static void
menu_window_clone(struct container *co)
{
#if 0
	void window(int x, int y, int scale);
	window(co->trans->center.x, co->trans->center.y, co->trans->scale);
#endif
}

static gboolean
menu_window_command_key_press(GtkWidget *widget, GdkEventKey *event,
	GtkWidget *win)
{
	GtkWidget *text;
	const char *t;
	struct container *co;

	if (! strcmp(event->string,"\r")) {
		text=gtk_object_get_data(GTK_OBJECT(win), "Input");
		co=gtk_object_get_data(GTK_OBJECT(win), "Container");
		t=gtk_entry_get_text(GTK_ENTRY(text));
		if (!strncmp(t,"goto ",5)) {
			command_goto(co, t+5);
		}
	}
	return TRUE;
}


static void
menu_window_command(struct container *co)
{
	GtkWidget *win,*entry,*text,*box;
	win=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(win), 320, 200);
	gtk_window_set_title(GTK_WINDOW(win), "Command");
	entry=gtk_entry_new();
	text=gtk_text_new(NULL, NULL);	
	box=gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), entry, 1, 1, 0);
	gtk_box_pack_start(GTK_BOX(box), text, 1, 1, 0);
	gtk_container_add(GTK_CONTAINER(win), box);	
	gtk_object_set_data(GTK_OBJECT(win), "Container", co);
	gtk_object_set_data(GTK_OBJECT(win), "Input", entry);
	gtk_object_set_data(GTK_OBJECT(win), "Output", text);
	gtk_signal_connect(GTK_OBJECT(win), "key-press-event", GTK_SIGNAL_FUNC(menu_window_command_key_press), win);
	gtk_widget_show_all(win);
}

static void
menu_window_visible_blocks(struct container *co)
{
	co->data_window[data_window_type_block]=data_window("Visible Blocks",co->win,NULL);
	graphics_redraw(co);
}

static void
menu_window_visible_towns(struct container *co)
{
	co->data_window[data_window_type_town]=data_window("Visible Towns",co->win,NULL);
	graphics_redraw(co);
}

static void
menu_window_visible_polys(struct container *co)
{
	co->data_window[data_window_type_street]=data_window("Visible Polys",co->win,NULL);
	graphics_redraw(co);
}

static void
menu_window_visible_streets(struct container *co)
{
	co->data_window[data_window_type_street]=data_window("Visible Streets",co->win,NULL);
	graphics_redraw(co);
}

static void
menu_window_visible_points(struct container *co)
{
	co->data_window[data_window_type_point]=data_window("Visible Points",co->win,NULL);
	graphics_redraw(co);
}

static void
menu_map_compare(struct container *co)
{
	char cmd[256];
	int x_min, x_max, y_min, y_max;

	x_min=co->trans->center.x-co->trans->scale/16*co->trans->width*4/10;
        x_max=co->trans->center.x+co->trans->scale/16*co->trans->width*4/10;
        y_min=co->trans->center.y-co->trans->scale/16*co->trans->height*4/10;
        y_max=co->trans->center.y+co->trans->scale/16*co->trans->height*4/10;      	
	sprintf(cmd, "./get_map %d %d %d %d %d %d ; xv map.xpm &", co->trans->width, co->trans->height, x_min, y_max, x_max, y_min);
	system(cmd);
}

static void
menu_map_distances(struct container *co)
{
	route_display_points(co->route, co);
}

static void
menu_destination_selected(GtkMenuItem *item, struct destination *dest)
{
	struct container *co =dest->co;

	destination_set(co, destination_type_bookmark, dest->text, &dest->pos);
}

static void
menu_item(struct menu *me, GtkWidget *menu, char *name, void (*func)(struct container *co))
{
	GtkWidget *item;
	item=gtk_menu_item_new_with_label(name);
	gtk_menu_append (GTK_MENU(menu), item);
	gtk_signal_connect_object(GTK_OBJECT(item), "activate",
		GTK_SIGNAL_FUNC (func), me);
}

static int
menu_clock_update(void *data)
{
	char buffer[16];
        time_t now=time(NULL);
	GtkWidget *widget=GTK_WIDGET(data);
        struct tm *now_tm=localtime(&now);

	sprintf(buffer,"%02d:%02d", now_tm->tm_hour, now_tm->tm_min);
	gtk_label_set_text(GTK_LABEL(widget), buffer);
	gtk_timeout_add((60-now_tm->tm_sec)*1000,menu_clock_update,widget);
	return FALSE;
}

struct menu *
gui_gtk_menu_new(struct container *co, GtkWidget **widget)
{
	struct menu *this=g_new0(struct menu, 1);

	this->gui=g_new0(struct menu_gui,1);
        this->gui->co=co;


	GtkWidget *menu,*item,*menu2,*item2,*menu3,*clock;

	menu=gtk_menu_bar_new();
	item=gtk_menu_item_new_with_label("Goto");
	gtk_menu_bar_append(GTK_MENU_BAR(menu), item);
	{
		menu2=gtk_menu_new();
	}
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu2);

	item=gtk_menu_item_new_with_label("Window");
	gtk_menu_bar_append(GTK_MENU_BAR(menu), item);
	{
		menu2=gtk_menu_new();

		item2=gtk_menu_item_new_with_label("Clone");
		gtk_menu_append (GTK_MENU(menu2), item2); 
		gtk_signal_connect_object(GTK_OBJECT(item2), "activate",
			GTK_SIGNAL_FUNC (menu_window_clone), this);

		item2=gtk_menu_item_new_with_label("Command");
		gtk_menu_append (GTK_MENU(menu2), item2); 
		gtk_signal_connect_object(GTK_OBJECT(item2), "activate",
			GTK_SIGNAL_FUNC (menu_window_command), this);

		item2=gtk_menu_item_new_with_label("Visible Blocks");
		gtk_menu_append (GTK_MENU(menu2), item2);
		gtk_signal_connect_object(GTK_OBJECT(item2), "activate",
			GTK_SIGNAL_FUNC (menu_window_visible_blocks), co);

		item2=gtk_menu_item_new_with_label("Visible Towns");
		gtk_menu_append (GTK_MENU(menu2), item2);
		gtk_signal_connect_object(GTK_OBJECT(item2), "activate",
			GTK_SIGNAL_FUNC (menu_window_visible_towns), co);

		item2=gtk_menu_item_new_with_label("Visible Polys");
		gtk_menu_append (GTK_MENU(menu2), item2);
		gtk_signal_connect_object(GTK_OBJECT(item2), "activate",
			GTK_SIGNAL_FUNC (menu_window_visible_polys), co);


		item2=gtk_menu_item_new_with_label("Visible Streets");
		gtk_menu_append (GTK_MENU(menu2), item2);
		gtk_signal_connect_object(GTK_OBJECT(item2), "activate",
			GTK_SIGNAL_FUNC (menu_window_visible_streets), co);

		menu_item(this, menu2, "Visible Points", menu_window_visible_points);

		item2=gtk_menu_item_new_with_label("Exit");
		gtk_menu_append (GTK_MENU(menu2), item2); 
		gtk_signal_connect_object(GTK_OBJECT(item2), "activate",
			GTK_SIGNAL_FUNC (exit), this);
	}
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu2);

	item=gtk_menu_item_new_with_label("Map");
	gtk_menu_bar_append(GTK_MENU_BAR(menu), item);
	{
		menu2=gtk_menu_new();

		item2=gtk_menu_item_new_with_label("Compare");
		gtk_menu_append (GTK_MENU(menu2), item2); 
		gtk_signal_connect_object(GTK_OBJECT(item2), "activate",
			GTK_SIGNAL_FUNC (menu_map_compare), this);

		item2=gtk_menu_item_new_with_label("Distances");
		gtk_menu_append (GTK_MENU(menu2), item2); 
		gtk_signal_connect_object(GTK_OBJECT(item2), "activate",
			GTK_SIGNAL_FUNC (menu_map_distances), this);
	}
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu2);

	item=gtk_menu_item_new_with_label("Route");
	gtk_menu_bar_append(GTK_MENU_BAR(menu), item);
	{
		menu2=gtk_menu_new();

		item2=gtk_menu_item_new_with_label("Start");
		gtk_menu_append (GTK_MENU(menu2), item2); 
		gtk_signal_connect_object(GTK_OBJECT(item2), "activate",
			GTK_SIGNAL_FUNC (route_start), co);

		item2=gtk_menu_item_new_with_label("Trace");
		gtk_menu_append (GTK_MENU(menu2), item2); 
		gtk_signal_connect_object(GTK_OBJECT(item2), "activate",
			GTK_SIGNAL_FUNC (route_trace), co);

		item2=gtk_menu_item_new_with_label("Update");
		gtk_menu_append (GTK_MENU(menu2), item2); 
		gtk_signal_connect_object(GTK_OBJECT(item2), "activate",
			GTK_SIGNAL_FUNC (menu_route_update), this);
	}
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu2);

	item=gtk_menu_item_new_with_label("Destinations");
	gtk_menu_bar_append(GTK_MENU_BAR(menu), item);
	menu2=gtk_menu_new();

	item2=gtk_menu_item_new_with_label("Last Destinations");
	gtk_menu_append (GTK_MENU(menu2), item2); 
	menu3=gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item2), menu3);

	item2=gtk_menu_item_new_with_label("Address");
	gtk_menu_append (GTK_MENU(menu2), item2); 

	{
		FILE *file;
		char buffer[8192];
		double lat,lng,lat_deg,lng_deg;
		char lat_c,lng_c;
		struct destination *dest;
		int pos,len;
		char *utf8,*text,*tok,*label;
		GList *list;
	
		file=fopen("locations.txt","r");
		while (file && fgets(buffer,8192,file)) {
			dest=malloc(sizeof(*dest));
			dest->co=co;
			len=strlen(buffer)-1;
			while (len >= 0 && buffer[len] == '\n') {
				buffer[len]='\0';
			}
			sscanf(buffer,"%lf %c %lf %c %n",&lat, &lat_c, &lng, &lng_c, &pos);
		
			lat_deg=floor(lat/100);
			lat-=lat_deg*100;
			lat_deg+=lat/60;     	

			lng_deg=floor(lng/100);
			lng-=lng_deg*100;
			lng_deg+=lng/60;          

			transform_mercator(&lng_deg, &lat_deg, &dest->pos);
			
			text=buffer+pos;
			dest->text=strdup(text);
			item2=NULL;
			menu3=menu2;
			while ((tok=strtok(text,"/"))) {
				list=NULL;
				if (item2) {
					menu3=gtk_menu_new();
					gtk_menu_item_set_submenu(GTK_MENU_ITEM(item2), menu3);
				}
				list=gtk_container_get_children(GTK_CONTAINER(menu3));
				while (list) {
					item2=GTK_WIDGET(list->data);
					gtk_label_get(GTK_LABEL(gtk_bin_get_child(GTK_BIN(item2))),&label);
					if (!strcmp(label, tok)) {
						menu3=gtk_menu_item_get_submenu(GTK_MENU_ITEM(item2));
						break;
					}
					list=list->next;
				}
				item2=NULL;
				if (! list) {
					utf8=g_locale_to_utf8(tok,-1,NULL,NULL,NULL);
					item2=gtk_menu_item_new_with_label(utf8);
					gtk_menu_append (GTK_MENU(menu3), item2); 
					g_free(utf8);
				}
				text=NULL;
			}
			gtk_signal_connect(GTK_OBJECT(item2), "activate",
				GTK_SIGNAL_FUNC (menu_destination_selected), dest);
		}
	}
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu2);

	item=gtk_menu_item_new();
	clock=gtk_label_new(NULL);
	gtk_menu_item_right_justify(GTK_MENU_ITEM(item));
	gtk_container_add(GTK_CONTAINER(item), clock);
	gtk_menu_bar_append(GTK_MENU_BAR(menu), item);
	menu_clock_update(clock);
	
	*widget=menu;
	return this;
}
