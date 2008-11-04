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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#if !defined(GDK_Book) || !defined(GDK_Calendar)
#include <X11/XF86keysym.h>
#endif
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
#include "callback.h"
#include "layout.h"
#include "vehicle.h"
#include "map.h"
#include "coord.h"
#include "navit_nls.h"

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

static gboolean
keypress(GtkWidget *widget, GdkEventKey *event, struct gui_priv *this)
{
	int w,h;
	#ifdef USE_HILDON
	GtkToggleAction *action;
	gboolean *fullscreen;
	#endif /*HILDON*/
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
	gtk_window_set_wmclass (GTK_WINDOW (gui->dialog_win), "navit", "Navit");
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
	NULL,
	gui_gtk_popup_new,
	gui_gtk_set_graphics,
	NULL,
	gui_gtk_datawindow_new,
	gui_gtk_add_bookmark,
};

static gboolean
gui_gtk_delete(GtkWidget *widget, GdkEvent *event, struct navit *nav)
{
	/* FIXME remove attr_navit callback */
	navit_destroy(nav);

	return TRUE;
}

static void
gui_gtk_toggle_init(struct gui_priv *this)
{
	struct attr attr;
	GtkToggleAction *toggle_action;

	if (navit_get_attr(this->nav, attr_cursor, &attr, NULL)) {
		toggle_action = GTK_TOGGLE_ACTION(gtk_action_group_get_action(this->base_group, "CursorAction"));
		gtk_toggle_action_set_active(toggle_action, attr.u.num);
	} else {
		dbg(0, "Unable to locate CursorAction\n");
	}
	if (navit_get_attr(this->nav, attr_orientation, &attr, NULL)) {
		toggle_action = GTK_TOGGLE_ACTION(gtk_action_group_get_action(this->base_group, "OrientationAction"));
		gtk_toggle_action_set_active(toggle_action, attr.u.num);
	} else {
		dbg(0, "Unable to locate OrientationAction\n");
	}
	if (navit_get_attr(this->nav, attr_tracking, &attr, NULL)) {
		toggle_action = GTK_TOGGLE_ACTION(gtk_action_group_get_action(this->base_group, "TrackingAction"));
		gtk_toggle_action_set_active(toggle_action, attr.u.num);
	} else {
		dbg(0, "Unable to locate TrackingAction\n");
	}
}

struct action_cb_data {
	struct gui_priv *gui;
	struct attr attr;
};

static void
gui_gtk_action_activate(GtkAction *action, struct action_cb_data *data)
{
	if(data->attr.type == attr_destination) {
		char * label;
		g_object_get(G_OBJECT(action), "label", &label,NULL);
		navit_set_destination(data->gui->nav, data->attr.u.pcoord, label);
		g_free(label);
	}
}

struct gui_menu_info {
	guint merge_id;
	GtkAction *action;
};

static void
gui_gtk_del_menu(struct gui_priv *this, struct gui_menu_info *meninfo)
{
	gtk_action_group_remove_action(this->dyn_group, meninfo->action);
	gtk_ui_manager_remove_ui(this->ui_manager, meninfo->merge_id);
}

static struct gui_menu_info
gui_gtk_add_menu(struct gui_priv *this, char *name, char *label, char *path, int submenu, struct action_cb_data *data)
{
	struct gui_menu_info meninfo;
	GtkAction *action;
	guint merge_id;

	action=gtk_action_new(name, label, NULL, NULL);
	meninfo.action = action;
	if (data)
		g_signal_connect(action, "activate", G_CALLBACK(gui_gtk_action_activate), data);
	gtk_action_group_add_action(this->dyn_group, action);
	merge_id =gtk_ui_manager_new_merge_id(this->ui_manager);
	meninfo.merge_id = merge_id;
	gtk_ui_manager_add_ui(this->ui_manager, merge_id, path, name, name, submenu ? GTK_UI_MANAGER_MENU : GTK_UI_MANAGER_MENUITEM, FALSE);

	return meninfo;
}

static void
gui_gtk_action_toggled(GtkToggleAction *action, struct action_cb_data *data)
{
	struct attr active;
	active.type=attr_active;
	active.u.num=gtk_toggle_action_get_active(action);
	map_set_attr(data->attr.u.map, &active);
	navit_draw(data->gui->nav);
}

static void
gui_gtk_add_toggle_menu(struct gui_priv *this, char *name, char *label, char *path, struct action_cb_data *data, gboolean active)
{
	GtkToggleAction *toggle_action;
	guint merge_id;

	toggle_action=gtk_toggle_action_new(name, label, NULL, NULL);
	gtk_toggle_action_set_active(toggle_action, active);
	g_signal_connect(GTK_ACTION(toggle_action), "toggled", G_CALLBACK(gui_gtk_action_toggled), data);
	gtk_action_group_add_action(this->dyn_group, GTK_ACTION(toggle_action));
	merge_id=gtk_ui_manager_new_merge_id(this->ui_manager);
	gtk_ui_manager_add_ui(this->ui_manager, merge_id, path, name, name, GTK_UI_MANAGER_MENUITEM, FALSE);
}

static void
gui_gtk_action_changed(GtkRadioAction *action, GtkRadioAction *current, struct action_cb_data *data)
{
	if (action == current) {
		navit_set_attr(data->gui->nav, &data->attr);
	}
}

static void
gui_gtk_add_radio_menu(struct gui_priv *this, char *name, char *label, char *path, struct action_cb_data *data, GSList **g)
{
	GtkRadioAction *radio_action;
	guint merge_id;

	radio_action=gtk_radio_action_new(name, label, NULL, NULL, 0);
	gtk_radio_action_set_group(radio_action, *g);
	*g=gtk_radio_action_get_group(radio_action);
	g_signal_connect(GTK_ACTION(radio_action), "changed", G_CALLBACK(gui_gtk_action_changed), data);
	gtk_action_group_add_action(this->dyn_group, GTK_ACTION(radio_action));
	merge_id=gtk_ui_manager_new_merge_id(this->ui_manager);
	gtk_ui_manager_add_ui(this->ui_manager, merge_id, path, name, name, GTK_UI_MANAGER_MENUITEM, FALSE);
}

static void
gui_gtk_layouts_init(struct gui_priv *this)
{
	struct attr_iter *iter;
	struct attr attr;
	struct action_cb_data *data;
	int count=0;
	char *name;

	iter=navit_attr_iter_new();
	while(navit_get_attr(this->nav, attr_layout, &attr, iter)) {
		name=g_strdup_printf("Layout %d", count++);
		data=g_new(struct action_cb_data, 1);
		data->gui=this;
		data->attr.type=attr_layout;
		data->attr.u.layout=attr.u.layout;
		gui_gtk_add_radio_menu(this, name, attr.u.layout->name, "/ui/MenuBar/Map/Layout/LayoutMenuAdditions", data, &this->layout_group);
		g_free(name);
	}
	navit_attr_iter_destroy(iter);
}

static void
gui_gtk_projections_init(struct gui_priv *this)
{
	struct action_cb_data *data;

	data=g_new(struct action_cb_data, 1);
	data->gui=this;
	data->attr.type=attr_projection;
	data->attr.u.projection=projection_mg;
	gui_gtk_add_radio_menu(this, "Projection mg", "Map & Guide", "/ui/MenuBar/Map/Projection/ProjectionMenuAdditions", data, &this->projection_group);

	data=g_new(struct action_cb_data, 1);
	data->gui=this;
	data->attr.type=attr_projection;
	data->attr.u.projection=projection_garmin;
	gui_gtk_add_radio_menu(this, "Projection garmin", "Garmin", "/ui/MenuBar/Map/Projection/ProjectionMenuAdditions", data, &this->projection_group);
}

static void
gui_gtk_vehicles_init(struct gui_priv *this)
{
	struct attr_iter *iter;
	struct attr attr,vattr;
	struct action_cb_data *data;
	int count=0;
	char *name;

	iter=navit_attr_iter_new();
	while(navit_get_attr(this->nav, attr_vehicle, &attr, iter)) {
		vehicle_get_attr(attr.u.vehicle, attr_name, &vattr, NULL);
		name=g_strdup_printf("Vehicle %d", count++);
		data=g_new(struct action_cb_data, 1);
		data->gui=this;
		data->attr.type=attr_vehicle;
		data->attr.u.vehicle=attr.u.vehicle;
		gui_gtk_add_radio_menu(this, name, vattr.u.str, "/ui/MenuBar/Map/Vehicle/VehicleMenuAdditions", data, &this->vehicle_group);
		g_free(name);
	}
	navit_attr_iter_destroy(iter);
}

static void
gui_gtk_maps_init(struct gui_priv *this)
{
	struct attr_iter *iter;
	struct attr attr,active,type,data;
	struct action_cb_data *cb_data;
	int count=0;
	char *name, *label;

	iter=navit_attr_iter_new();
	while(navit_get_attr(this->nav, attr_map, &attr, iter)) {
		name=g_strdup_printf("Map %d", count++);
		if (! map_get_attr(attr.u.map, attr_type, &type, NULL))
			type.u.str="";
		if (! map_get_attr(attr.u.map, attr_data, &data, NULL))
			data.u.str="";
		label=g_strdup_printf("%s:%s", type.u.str, data.u.str);
		cb_data=g_new(struct action_cb_data, 1);
		cb_data->gui=this;
		cb_data->attr.type=attr_map;
		cb_data->attr.u.map=attr.u.map;
		if (! map_get_attr(attr.u.map, attr_active, &active, NULL))
			active.u.num=1;
		gui_gtk_add_toggle_menu(this, name, label, "/ui/MenuBar/Map/MapMenuAdditions", cb_data, active.u.num);
		g_free(name);
		g_free(label);
	}
	navit_attr_iter_destroy(iter);

}

static void
gui_gtk_destinations_update(struct gui_priv *this)
{
	GList *curr;
	struct attr attr;
	struct action_cb_data *data;
	struct map_rect *mr=NULL;
	struct item *item;
	struct gui_menu_info *meninfo;
	struct coord c;
	int count=0;
	char *name, *label;

	curr = g_list_first(this->dest_menuitems);

	while (curr) {
		gui_gtk_del_menu(this, (struct gui_menu_info *)curr->data);
		g_free((struct gui_menu_info *)curr->data);
		curr = g_list_next(curr);
	};

	g_list_free(this->dest_menuitems);
	this->dest_menuitems = NULL;

	if(navit_get_attr(this->nav, attr_former_destination_map, &attr, NULL) && attr.u.map && (mr=map_rect_new(attr.u.map, NULL))) {
		while ((item=map_rect_get_item(mr))) {
			if (item->type != type_former_destination) continue;
			name=g_strdup_printf("Destination %d", count++);
			item_attr_get(item, attr_label, &attr);
			label=attr.u.str;
			item_coord_get(item, &c, 1);
			data=g_new(struct action_cb_data, 1);
			data->gui=this;
			data->attr.type=attr_destination;
			data->attr.u.pcoord=g_new(struct pcoord, 1);
			data->attr.u.pcoord->pro=projection_mg;
			data->attr.u.pcoord->x=c.x;
			data->attr.u.pcoord->y=c.y;
			
			meninfo = g_new(struct gui_menu_info, 1);
			*meninfo = gui_gtk_add_menu(this, name, label, "/ui/MenuBar/Route/FormerDestinations/FormerDestinationMenuAdditions",0,data); 
			this->dest_menuitems = g_list_prepend(this->dest_menuitems, meninfo);
			g_free(name);
		}
		map_rect_destroy(mr);
	}
}

static void
gui_gtk_destinations_init(struct gui_priv *this)
{
	navit_add_callback(this->nav, callback_new_attr_1(callback_cast(gui_gtk_destinations_update), attr_destination, this));
	gui_gtk_destinations_update(this);
}

static void
gui_gtk_bookmarks_update(struct gui_priv *this)
{
	GList *curr;
	struct attr attr;
	struct action_cb_data *data;
	struct map_rect *mr=NULL;
	struct gui_menu_info *meninfo;
	struct item *item;
	struct coord c;
	int count=0;
	char *parent, *name, *label, *label_full, *menu_label, *tmp_parent, *s;
	GHashTable *hash;

	curr = g_list_first(this->bookmarks_menuitems);

	while (curr) {
		gui_gtk_del_menu(this, (struct gui_menu_info *)curr->data);
		g_free((struct gui_menu_info *)curr->data);
		curr = g_list_next(curr);
	};

	g_list_free(this->bookmarks_menuitems);
	this->bookmarks_menuitems = NULL;

	if(navit_get_attr(this->nav, attr_bookmark_map, &attr, NULL) && attr.u.map && (mr=map_rect_new(attr.u.map, NULL))) {
		hash=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
		while ((item=map_rect_get_item(mr))) {
			if (item->type != type_bookmark) continue;
			item_attr_get(item, attr_label, &attr);
			label_full=attr.u.str;
			item_coord_get(item, &c, 1);
			menu_label=g_malloc(strlen(label_full)+1);
			label=label_full;
			parent=g_strdup("/ui/MenuBar/Route/Bookmarks/BookmarkMenuAdditions");
			while ((s=strchr(label, '/'))) {
				strcpy(menu_label, label_full);
				menu_label[s-label_full]='\0';
				if ((tmp_parent=g_hash_table_lookup(hash, menu_label))) {
					tmp_parent=g_strdup(tmp_parent);
				} else {
					name=g_strdup_printf("Bookmark %d", count++);
					meninfo = g_new(struct gui_menu_info, 1);
					*meninfo = gui_gtk_add_menu(this, name, menu_label+(label-label_full),parent,1,NULL);
					this->bookmarks_menuitems = g_list_prepend(this->bookmarks_menuitems, meninfo);
					tmp_parent=g_strdup_printf("%s/%s", parent, name);
					g_hash_table_insert(hash, g_strdup(menu_label), g_strdup(tmp_parent));
					g_free(name);
				}
				g_free(parent);
				parent=tmp_parent;
				label=s+1;
			}
			g_free(menu_label);
			data=g_new(struct action_cb_data, 1);
			data->gui=this;
			data->attr.type=attr_destination;
			data->attr.u.pcoord=g_new(struct pcoord, 1);
			data->attr.u.pcoord->pro=projection_mg;
			data->attr.u.pcoord->x=c.x;
			data->attr.u.pcoord->y=c.y;
			name=g_strdup_printf("Bookmark %d", count++);
			meninfo = g_new(struct gui_menu_info, 1);
			*meninfo = gui_gtk_add_menu(this, name, label, parent,0,data);
			this->bookmarks_menuitems = g_list_prepend(this->bookmarks_menuitems, meninfo);
			g_free(name);
			g_free(parent);
		}
		g_hash_table_destroy(hash);
	}
}

static void
gui_gtk_bookmarks_init(struct gui_priv *this)
{
	navit_add_callback(this->nav, callback_new_attr_1(callback_cast(gui_gtk_bookmarks_update), attr_bookmark_map, this));
	gui_gtk_bookmarks_update(this);
}

static void
gui_gtk_init(struct gui_priv *this, struct navit *nav)
{


	gui_gtk_toggle_init(this);
	gui_gtk_layouts_init(this);
	gui_gtk_projections_init(this);
	gui_gtk_vehicles_init(this);
	gui_gtk_maps_init(this);
	gui_gtk_destinations_init(this);
	gui_gtk_bookmarks_init(this);
}

static struct gui_priv *
gui_gtk_new(struct navit *nav, struct gui_methods *meth, struct attr **attrs) 
{
	struct gui_priv *this;
	int w=792, h=547;
	char *cp = getenv("NAVIT_XID");
	unsigned xid = 0;
	struct attr *attr;
	GtkWidget *widget;

	if (cp) {
		xid = strtol(cp, NULL, 0);
	}

	this=g_new0(struct gui_priv, 1);
	this->nav=nav;

	attr = attr_search(attrs, NULL, attr_menubar);
	if (attr) {
		this->menubar_enable=attr->u.num;
	} else {
		this->menubar_enable=1;
	}
	attr=attr_search(attrs, NULL, attr_toolbar);
	if (attr) {
		this->toolbar_enable=attr->u.num;
	} else {
		this->toolbar_enable=1;
	}
	attr=attr_search(attrs, NULL, attr_statusbar);
	if (attr) {
		this->statusbar_enable=attr->u.num;
	} else {
		this->statusbar_enable=1;
	}

	*meth=gui_gtk_methods;

	if (!xid) 
		this->win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	else
		this->win = gtk_plug_new(xid);

	g_signal_connect(G_OBJECT(this->win), "delete-event", G_CALLBACK(gui_gtk_delete), nav);
	this->vbox = gtk_vbox_new(FALSE, 0);
	gtk_window_set_default_size(GTK_WINDOW(this->win), w, h);
	gtk_window_set_title(GTK_WINDOW(this->win), "Navit");
	gtk_window_set_wmclass (GTK_WINDOW (this->win), "navit", "Navit");
	gtk_widget_realize(this->win);
	gui_gtk_ui_init(this);
	if (this->menubar_enable) {
		widget=gtk_ui_manager_get_widget(this->ui_manager, "/ui/MenuBar");
		GTK_WIDGET_UNSET_FLAGS (widget, GTK_CAN_FOCUS);
		gtk_box_pack_start (GTK_BOX(this->vbox), widget, FALSE, FALSE, 0);
		gtk_widget_show (widget);
		this->menubar=widget;
	}
	if (this->toolbar_enable) {
		widget=gtk_ui_manager_get_widget(this->ui_manager, "/ui/ToolBar");
		GTK_WIDGET_UNSET_FLAGS (widget, GTK_CAN_FOCUS);
		gtk_box_pack_start (GTK_BOX(this->vbox), widget, FALSE, FALSE, 0);
		gtk_widget_show (widget);
	}
	if (this->statusbar_enable) {
		this->statusbar=gui_gtk_statusbar_new(this);
	}
	gtk_container_add(GTK_CONTAINER(this->win), this->vbox);
	gtk_widget_show_all(this->win);


	navit_add_callback(nav, callback_new_attr_1(callback_cast(gui_gtk_init), attr_navit, this));
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
