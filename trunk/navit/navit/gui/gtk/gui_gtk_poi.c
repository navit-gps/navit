/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2013 Navit Team
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

#include <stdlib.h>
#include <gtk/gtk.h>
#include "gui_gtk_poi.h"
#include "popup.h"
#include "debug.h"
#include "navit_nls.h"
#include "coord.h"
#include "point.h"
#include "callback.h"
#include "graphics.h"
#include "navit.h"
#include "item.h"
#include "map.h"
#include "mapset.h"
#include "transform.h"
#include "attr.h"
#include "vehicle.h"
#include "util.h"

static struct gtk_poi_search{
	GtkWidget *entry_distance;
	GtkWidget *label_distance;
	GtkWidget *treeview_cat;
	GtkWidget *treeview_poi;
	GtkWidget *button_visit, *button_destination, *button_map;
	GtkListStore *store_poi;
	GtkListStore *store_cat;
	GtkTreeModel *store_poi_sorted;
	GtkTreeModel *store_cat_sorted;
	char *selected_cat;
	struct navit *nav;
} gtk_poi_search;

static GdkPixbuf *
geticon(const char *name){
	GdkPixbuf *icon=NULL;
	GError *error=NULL;
	icon=gdk_pixbuf_new_from_file(graphics_icon_path(name),&error);
	if (error) {
		dbg(0, "failed to load icon '%s': %s\n", name, error->message);
	}
	return icon;
}

/** Build the category list model with icons. */
static GtkTreeModel *
category_list_model(struct gtk_poi_search *search)
{
	GtkTreeIter iter;
	gtk_list_store_append(search->store_cat, &iter);
	gtk_list_store_set(search->store_cat, &iter, 0,geticon("pharmacy.png"), 1, _("Pharmacy"), 2, "poi_pharmacy", -1);
	gtk_list_store_append(search->store_cat, &iter);
	gtk_list_store_set(search->store_cat, &iter, 0, geticon("restaurant.png"), 1, _("Restaurant"), 2, "poi_restaurant", -1);
	gtk_list_store_append(search->store_cat, &iter);
	gtk_list_store_set(search->store_cat, &iter,0, geticon("restaurant.png"), 1, _("Restaurant. Fast food"), 2, "poi_fastfood", -1);
	gtk_list_store_append(search->store_cat, &iter);
	gtk_list_store_set(search->store_cat, &iter,0, geticon("hotel.png"), 1, _("Hotel"), 2, "poi_hotel", -1);
	gtk_list_store_append(search->store_cat, &iter);
	gtk_list_store_set(search->store_cat, &iter,0, geticon("parking.png"), 1, _("Car parking"), 2, "poi_car_parking", -1);
	gtk_list_store_append(search->store_cat, &iter);
	gtk_list_store_set(search->store_cat, &iter,0, geticon("fuel.png"), 1, _("Fuel station"), 2, "poi_fuel", -1);
	gtk_list_store_append(search->store_cat, &iter);
	gtk_list_store_set(search->store_cat, &iter,0, geticon("bank.png"), 1, _("Bank"), 2, "poi_bank", -1);
	gtk_list_store_append(search->store_cat, &iter);
	gtk_list_store_set(search->store_cat, &iter,0, geticon("hospital.png"), 1, _("Hospital"), 2, "poi_hospital", -1);
	gtk_list_store_append(search->store_cat, &iter);
	gtk_list_store_set(search->store_cat, &iter,0, geticon("cinema.png"), 1, _("Cinema"), 2, "poi_cinema", -1);
	gtk_list_store_append(search->store_cat, &iter);
	gtk_list_store_set(search->store_cat, &iter,0, geticon("rail_station.png"), 1, _("Train station"), 2, "poi_rail_station", -1);
	gtk_list_store_append(search->store_cat, &iter);
	gtk_list_store_set(search->store_cat, &iter,0, geticon("school.png"), 1, _("School"), 2, "poi_school", -1);
	gtk_list_store_append(search->store_cat, &iter);
	gtk_list_store_set(search->store_cat, &iter,0, geticon("police.png"), 1, _("Police"), 2, "poi_police", -1);
	gtk_list_store_append(search->store_cat, &iter);
	gtk_list_store_set(search->store_cat, &iter,0, geticon("justice.png"), 1, _("Justice"), 2, "poi_justice", -1);
	gtk_list_store_append(search->store_cat, &iter);
	gtk_list_store_set(search->store_cat, &iter,0, geticon("taxi.png"), 1, _("Taxi"), 2, "poi_taxi", -1);
	gtk_list_store_append(search->store_cat, &iter);
	gtk_list_store_set(search->store_cat, &iter,0, geticon("shopping.png"), 1, _("Shopping"), 2, "poi_shopping", -1);
	return GTK_TREE_MODEL (search->store_cat_sorted);
}


/** Construct model of POIs from map information. */
static GtkTreeModel *
model_poi (struct gtk_poi_search *search)
{
	GtkTreeIter iter;
	struct map_selection *sel,*selm;
	struct coord coord_item,center;
	struct pcoord pc;
	struct mapset_handle *h;
	int search_distance_meters,idist;
	struct map *m;
	struct map_rect *mr;
	struct item *item;
	struct point cursor_position;
	struct attr attr, vehicle_attr;
	struct vehicle *v=NULL;
	enum item_type selected;

	search_distance_meters=1000*atoi((char *) gtk_entry_get_text(GTK_ENTRY(search->entry_distance)));

	if(navit_get_attr(search->nav,attr_vehicle, &vehicle_attr,NULL) && (v=vehicle_attr.u.vehicle)) {
		vehicle_get_attr(v,attr_position_coord_geo, &attr, NULL);
	}
	if (!v || (attr.u.coord_geo->lng==0.0f && attr.u.coord_geo->lat==0.0f)){
		cursor_position.x=navit_get_width(search->nav)/2;
		cursor_position.y=navit_get_height(search->nav)/2;
		gtk_label_set_text(GTK_LABEL(search->label_distance),_("Distance from screen center (km)"));
	}else{
		cursor_position.x=navit_get_width(search->nav)/2;
		cursor_position.y=navit_get_height(search->nav)*4/5;
		gtk_label_set_text(GTK_LABEL(search->label_distance),_("Distance from vehicle cursor (km)"));
	}

	transform_reverse(navit_get_trans(search->nav), &cursor_position, &center);
	pc.pro = transform_get_projection(navit_get_trans(search->nav));
	pc.x = center.x;
	pc.y = center.y;

	//Search in the map, for pois
	sel=map_selection_rect_new(&pc ,search_distance_meters*transform_scale(abs(center.y)+search_distance_meters*1.5),18);
	gtk_list_store_clear(search->store_poi);

	h=mapset_open(navit_get_mapset(search->nav));

	selected=item_from_name(search->selected_cat);
	while ((m=mapset_next(h, 1))) {
		selm=map_selection_dup_pro(sel, projection_mg, map_projection(m));
		mr=map_rect_new(m, selm);
		if (mr) {
			while ((item=map_rect_get_item(mr))) {
				struct attr label_attr;
				item_attr_get(item,attr_label,&label_attr);
				item_coord_get(item,&coord_item,1);
				idist=transform_distance(1,&center,&coord_item);
				if (item->type==selected && idist<=search_distance_meters){
					char direction[5];
					gtk_list_store_append(search->store_poi, &iter);
					get_compass_direction(direction,transform_get_angle_delta(&center,&coord_item,0),1);
					gtk_list_store_set(search->store_poi, &iter, 0,direction, 1,idist,
						2,g_strdup(label_attr.u.str), 3,coord_item.x, 4,coord_item.y ,-1);
				}
			}
			map_rect_destroy(mr);
		}
		map_selection_destroy(selm);
	}
	map_selection_destroy(sel);
	mapset_close(h);

	return GTK_TREE_MODEL (search->store_poi_sorted);
}

/** Enable button if there is a selected row. */
static void
treeview_poi_changed(GtkWidget *widget, struct gtk_poi_search *search)
{
	GtkTreePath *path;
	GtkTreeViewColumn *focus_column;
	GtkTreeIter iter;

	gtk_tree_view_get_cursor(GTK_TREE_VIEW(search->treeview_cat), &path, &focus_column);
	if(!path) return;
	if(!gtk_tree_model_get_iter(GTK_TREE_MODEL(search->store_cat_sorted), &iter, path)) return;

	gtk_widget_set_sensitive(search->button_visit,TRUE);
	gtk_widget_set_sensitive(search->button_map,TRUE);
	gtk_widget_set_sensitive(search->button_destination,TRUE);
}

/** Reload the POI list and disable buttons. */
static void
treeview_poi_reload(GtkWidget *widget, struct gtk_poi_search *search)
{
	GtkTreePath *path;
	GtkTreeViewColumn *focus_column;
	GtkTreeIter iter;

	gtk_widget_set_sensitive(search->button_visit,FALSE);
	gtk_widget_set_sensitive(search->button_map,FALSE);
	gtk_widget_set_sensitive(search->button_destination,FALSE);

	gtk_tree_view_get_cursor(GTK_TREE_VIEW(search->treeview_cat), &path, &focus_column);
	if(!path) return;
	if(!gtk_tree_model_get_iter(GTK_TREE_MODEL(search->store_cat_sorted), &iter, path)) return;
	gtk_tree_model_get(GTK_TREE_MODEL(search->store_cat_sorted), &iter, 2, &search->selected_cat, -1);
	gtk_tree_view_set_model(GTK_TREE_VIEW (search->treeview_poi), model_poi(search));
}

/** Set the selected POI as destination. */
static void
button_destination_clicked(GtkWidget *widget, struct gtk_poi_search *search)
{
	GtkTreePath *path;
	GtkTreeViewColumn *focus_column;
	GtkTreeIter iter;
	long int lat, lon;
	char *label;
	char *category;
	char buffer[2000];

	//Get category
	gtk_tree_view_get_cursor(GTK_TREE_VIEW(search->treeview_cat), &path, &focus_column);
	if(!path) return;
	if(!gtk_tree_model_get_iter(GTK_TREE_MODEL(search->store_cat_sorted), &iter, path)) return;
	gtk_tree_model_get(GTK_TREE_MODEL(search->store_cat_sorted), &iter, 1, &category, -1);

	//Get label, lat, lon
	gtk_tree_view_get_cursor(GTK_TREE_VIEW(search->treeview_poi), &path, &focus_column);
	if(!path) return;
	if(!gtk_tree_model_get_iter(GTK_TREE_MODEL(search->store_poi_sorted), &iter, path)) return;
	gtk_tree_model_get(GTK_TREE_MODEL(search->store_poi_sorted), &iter, 2, &label, -1);
	gtk_tree_model_get(GTK_TREE_MODEL(search->store_poi_sorted), &iter, 3, &lat, -1);
	gtk_tree_model_get(GTK_TREE_MODEL(search->store_poi_sorted), &iter, 4, &lon, -1);
	sprintf(buffer, _("POI %s. %s"), category, label);

	struct pcoord dest;
	dest.x=lat;
	dest.y=lon;
	dest.pro=1;
	navit_set_destination(search->nav, &dest, buffer, 1);
	dbg(1,_("Set destination to %ld, %ld \n"),lat,lon);
}

/* Show the POI's position in the map. */
static void
button_map_clicked(GtkWidget *widget, struct gtk_poi_search *search)
{
	GtkTreePath *path;
	GtkTreeViewColumn *focus_column;
	GtkTreeIter iter;
	long int lat,lon;

	gtk_tree_view_get_cursor(GTK_TREE_VIEW(search->treeview_poi), &path, &focus_column);
	if(!path) return;
	if(!gtk_tree_model_get_iter(GTK_TREE_MODEL(search->store_poi_sorted), &iter, path)) return;
	gtk_tree_model_get(GTK_TREE_MODEL(search->store_poi_sorted), &iter, 3, &lat, -1);
	gtk_tree_model_get(GTK_TREE_MODEL(search->store_poi_sorted), &iter, 4, &lon, -1);

	struct pcoord dest;
	dest.x=lat;
	dest.y=lon;
	dest.pro=1;
	navit_set_center(search->nav, &dest,1);
	dbg(1,_("Set map to %ld, %ld \n"),lat,lon);
}

/** Set POI as the first "visit before". */
static void
button_visit_clicked(GtkWidget *widget, struct gtk_poi_search *search)
{
	GtkTreePath *path;
	GtkTreeViewColumn *focus_column;
	GtkTreeIter iter;
	long int lat,lon;

	gtk_tree_view_get_cursor(GTK_TREE_VIEW(search->treeview_poi), &path, &focus_column);
	if(!path) return;
	if(!gtk_tree_model_get_iter(GTK_TREE_MODEL(search->store_poi_sorted), &iter, path)) return;
	gtk_tree_model_get(GTK_TREE_MODEL(search->store_poi_sorted), &iter, 3, &lat, -1);
	gtk_tree_model_get(GTK_TREE_MODEL(search->store_poi_sorted), &iter, 4, &lon, -1);
	dbg(1,_("Set next visit to %ld, %ld \n"),lat,lon);

	struct pcoord dest;
	dest.x=lat;
	dest.y=lon;
	dest.pro=1;
	popup_set_visitbefore(search->nav,&dest,0);
}

/** Create UI and connect objects to functions. */
void gtk_gui_poi(struct navit *nav)
{
	GtkWidget *window2,*vbox, *keyboard, *table;
	GtkWidget *label_category, *label_poi;
	GtkWidget *listbox_cat, *listbox_poi;
	GtkCellRenderer *renderer;

	struct gtk_poi_search *search=&gtk_poi_search;
	search->nav=nav;

	window2 = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window2),_("POI search"));
	gtk_window_set_wmclass (GTK_WINDOW (window2), "navit", "Navit");
	gtk_window_set_default_size (GTK_WINDOW (window2),700,550);
	vbox = gtk_vbox_new(FALSE, 0);
	table = gtk_table_new(4, 4, FALSE);

	label_category = gtk_label_new(_("Select a category"));
	search->label_distance = gtk_label_new(_("Select a distance to look for (km)"));
	label_poi=gtk_label_new(_("Select a POI"));

	search->entry_distance=gtk_entry_new_with_max_length(2);
	gtk_entry_set_text(GTK_ENTRY(search->entry_distance),"10");

	search->treeview_cat=gtk_tree_view_new();
	listbox_cat = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (listbox_cat), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(listbox_cat),search->treeview_cat);
	search->store_cat = gtk_list_store_new (3, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING);
	renderer=gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (search->treeview_cat),-1, _(" "), renderer, "pixbuf", 0, NULL);
	renderer=gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (search->treeview_cat),-1, _("Category"), renderer, "text", 1, NULL);
	search->store_cat_sorted=gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(search->store_cat));
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(search->store_cat_sorted),1,GTK_SORT_ASCENDING);
	gtk_tree_view_set_model (GTK_TREE_VIEW (search->treeview_cat), category_list_model(search));

	search->treeview_poi=gtk_tree_view_new();
	listbox_poi = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (listbox_poi), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(listbox_poi),search->treeview_poi);
	search->store_poi = gtk_list_store_new (5, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_LONG, G_TYPE_LONG);
	renderer=gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (search->treeview_poi),-1, _("Direction"), renderer, "text",0,NULL);
	renderer=gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (search->treeview_poi),-1, _("Distance(m)"), renderer, "text", 1, NULL);
	renderer=gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (search->treeview_poi),-1, _("Name"), renderer, "text", 2, NULL);
	search->store_poi_sorted=gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(search->store_poi));
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(search->store_poi_sorted),1,GTK_SORT_ASCENDING);

	search->button_visit = gtk_button_new_with_label(_("Visit Before"));
	search->button_destination = gtk_button_new_with_label(_("Destination"));
	search->button_map = gtk_button_new_with_label(_("Map"));
	gtk_widget_set_sensitive(search->button_visit,FALSE);
	gtk_widget_set_sensitive(search->button_map,FALSE);
	gtk_widget_set_sensitive(search->button_destination,FALSE);

	gtk_table_attach(GTK_TABLE(table), search->label_distance,      0, 1, 0, 1,  0, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), search->entry_distance,     1, 2, 0, 1,  0, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), label_category,     0, 1, 2, 3,  0, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), listbox_cat,        0, 1, 3, 4,  GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
	gtk_table_attach(GTK_TABLE(table), label_poi,          1, 4, 2, 3,  0, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), listbox_poi,        1, 4, 3, 4,  GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
	gtk_table_attach(GTK_TABLE(table), search->button_map,         0, 1, 4, 5,  GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), search->button_visit,       1, 2, 4, 5,  GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), search->button_destination, 2, 3, 4, 5,  GTK_FILL, GTK_FILL, 0, 0);
	gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);

	g_signal_connect(G_OBJECT(search->entry_distance), "changed", G_CALLBACK(treeview_poi_reload), search);
	g_signal_connect(G_OBJECT(search->button_visit), "clicked", G_CALLBACK(button_visit_clicked), search);
	g_signal_connect(G_OBJECT(search->button_map), "clicked", G_CALLBACK(button_map_clicked), search);
	g_signal_connect(G_OBJECT(search->button_destination), "clicked", G_CALLBACK(button_destination_clicked), search);
	g_signal_connect(G_OBJECT(search->treeview_cat), "cursor_changed", G_CALLBACK(treeview_poi_reload), search);
	g_signal_connect(G_OBJECT(search->treeview_poi), "cursor_changed", G_CALLBACK(treeview_poi_changed), search);

	keyboard=gtk_socket_new();
	gtk_box_pack_end(GTK_BOX(vbox), keyboard, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window2), vbox);
	gtk_widget_show_all(window2);
}

