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

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include "debug.h"
#include "destination.h"
#include "navit.h"
#include "item.h"
#include "coord.h"
#include "track.h"
#include "country.h"
#include "search.h"
#include "projection.h"

#define COL_COUNT 8

#define gettext_noop(String) String
#define _(STRING)    gettext(STRING)
#define _n(STRING)    gettext_noop(STRING)

static struct search_param {
	struct navit *nav;
	struct mapset *ms;
	struct search_list *sl;
	struct attr attr;
	int partial;
	GtkWidget *entry_country, *entry_postal, *entry_city, *entry_district;
	GtkWidget *entry_street, *entry_number;
	GtkWidget *listbox;
	GtkWidget *treeview;
	GtkListStore *liststore;
	GtkTreeModel *liststore2;
} search_param;

static void button_map(GtkWidget *widget, struct search_param *search)
{
	GtkTreePath *path;
	GtkTreeViewColumn *focus_column;
	struct pcoord *c=NULL;
	GtkTreeIter iter;

	gtk_tree_view_get_cursor(GTK_TREE_VIEW(search->treeview), &path, &focus_column);
	if(!path)
		return;
	if(!gtk_tree_model_get_iter(search->liststore2, &iter, path))
		return;
	gtk_tree_model_get (GTK_TREE_MODEL (search->liststore2), &iter, COL_COUNT, &c, -1);
	if (c) {
		navit_set_center(search->nav, c);
	}
}

static char *description(struct search_param *search, GtkTreeIter *iter)
{
	char *desc,*car,*postal,*town,*street;
	gtk_tree_model_get (GTK_TREE_MODEL (search->liststore2), iter, 0, &car, -1);
	gtk_tree_model_get (GTK_TREE_MODEL (search->liststore2), iter, 1, &postal, -1);
	gtk_tree_model_get (GTK_TREE_MODEL (search->liststore2), iter, 2, &town, -1);
	gtk_tree_model_get (GTK_TREE_MODEL (search->liststore2), iter, 4, &street, -1);
	if (search->attr.type == attr_town_name)
		desc=g_strdup_printf("%s-%s %s", car, postal, town);
	else
		desc=g_strdup_printf("%s-%s %s, %s", car, postal, town, street);
	return desc;
}

static void button_destination(GtkWidget *widget, struct search_param *search)
{
	struct pcoord *c=NULL;
	GtkTreeIter iter;
	char *desc;

	if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (search->liststore2), &iter))
		return;
	gtk_tree_model_get (GTK_TREE_MODEL (search->liststore2), &iter, COL_COUNT, &c, -1);
	if (c) {
		desc=description(search, &iter);
		navit_set_destination(search->nav, c, desc);
		g_free(desc);
	}
}

static void button_bookmark(GtkWidget *widget, struct search_param *search)
{
	struct pcoord *c=NULL;
	GtkTreeIter iter;
	char *desc;

	if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (search->liststore2), &iter))
		return;
	gtk_tree_model_get (GTK_TREE_MODEL (search->liststore2), &iter, COL_COUNT, &c, -1);
	if (c) {
		desc=description(search, &iter);
		navit_add_bookmark(search->nav, c, desc);
		g_free(desc);
	}
}


char **columns_text[] = {
	(char *[]){_n("Car"),_n("Iso2"),_n("Iso3"),_n("Country"),NULL},
	(char *[]){_n("Car"),_n("Postal"),_n("Town"),_n("District"),NULL},
	(char *[]){_n("Car"),_n("Postal"),_n("Town"),_n("District"),_n("Street"),NULL},
	(char *[]){_n("Car"),_n("Postal"),_n("Town"),_n("District"),_n("Street"),_n("Number"),NULL},
};

static void set_columns(struct search_param *param, int mode)
{
	GList *columns_list,*columns;
	char **column_text=columns_text[mode];
	int i=0;

	columns_list=gtk_tree_view_get_columns(GTK_TREE_VIEW(param->treeview));
	columns=columns_list;
	while (columns) {
		gtk_tree_view_remove_column(GTK_TREE_VIEW(param->treeview), columns->data);
		columns=g_list_next(columns);
	}
	g_list_free(columns_list);
	while (*column_text) {
		printf("column_text=%p\n", column_text);
		printf("*column_text=%s\n", *column_text);
		GtkCellRenderer *cell=gtk_cell_renderer_text_new();
		gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (param->treeview),-1, gettext(*column_text), cell, "text", i, NULL);
		i++;
		column_text++;
	}

}

static void row_activated(GtkWidget *widget, GtkTreePath *p1, GtkTreeViewColumn *c, struct search_param *search)
{
	GtkTreePath *path;
	GtkTreeViewColumn *focus_column;
	GtkTreeIter iter;
	GtkWidget *entry_widget;
	char *str;
	int column;

	dbg(0,"enter\n");
	gtk_tree_view_get_cursor(GTK_TREE_VIEW(search->treeview), &path, &focus_column);
	if(!path)
		return;
	if(!gtk_tree_model_get_iter(search->liststore2, &iter, path))
		return;
	switch(search->attr.type) {
	case attr_country_all:
		entry_widget=search->entry_country;
		column=3;
		break;
	case attr_town_name:
		entry_widget=search->entry_city;
		column=2;
		break;
	case attr_street_name:
		entry_widget=search->entry_street;
		column=4;
		break;
	default:
		dbg(0,"Unknown mode\n");
		return;
	}
	gtk_tree_model_get(search->liststore2, &iter, column, &str, -1);
	dbg(0,"str=%s\n", str);
	search->partial=0;
	gtk_entry_set_text(GTK_ENTRY(entry_widget), str);
}

static void tree_view_button_release(GtkWidget *widget, GdkEventButton *event, struct search_param *search)
{
	GtkTreePath *path;
	GtkTreeViewColumn *column;
	gtk_tree_view_get_cursor(GTK_TREE_VIEW(search->treeview), &path, &column);
	gtk_tree_view_row_activated(GTK_TREE_VIEW(search->treeview), path, column);
	
}
static void
next_focus(struct search_param *search, GtkWidget *widget)
{
	if (widget == search->entry_country)
		gtk_widget_grab_focus(search->entry_city);
	if (widget == search->entry_city)
		gtk_widget_grab_focus(search->entry_street);
	if (widget == search->entry_street)
		gtk_widget_grab_focus(search->entry_number);
		
}

static void changed(GtkWidget *widget, struct search_param *search)
{
	struct search_list_result *res;
	GtkTreeIter iter;

	search->attr.u.str=(char *)gtk_entry_get_text(GTK_ENTRY(widget));
	printf("changed %s partial %d\n", search->attr.u.str, search->partial);
	if (widget == search->entry_country) {
		gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (search->liststore2), 3, GTK_SORT_ASCENDING);
		dbg(0,"country\n");
		search->attr.type=attr_country_all;
		set_columns(search, 0);
	}
	if (widget == search->entry_postal) {
		gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (search->liststore2), 1, GTK_SORT_ASCENDING);
		dbg(0,"postal\n");
		search->attr.type=attr_town_postal;
		if (strlen(search->attr.u.str) < 2)
			return;
		set_columns(search, 1);
	}
	if (widget == search->entry_city) {
		gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (search->liststore2), 2, GTK_SORT_ASCENDING);
		dbg(0,"town\n");
		search->attr.type=attr_town_name;
		if (strlen(search->attr.u.str) < 3)
			return;
		set_columns(search, 1);
	}
	if (widget == search->entry_street) {
		dbg(0,"street\n");
		search->attr.type=attr_street_name;
		set_columns(search, 2);
	}


	search_list_search(search->sl, &search->attr, search->partial);
	gtk_list_store_clear(search->liststore);
	while((res=search_list_get_result(search->sl))) {
		gtk_list_store_append(search->liststore,&iter);
		gtk_list_store_set(search->liststore,&iter,COL_COUNT,res->c,-1);
		if (widget == search->entry_country) {
			if (res->country) {
				gtk_list_store_set(search->liststore,&iter,0,res->country->car,-1);
				gtk_list_store_set(search->liststore,&iter,1,res->country->iso3,-1);
				gtk_list_store_set(search->liststore,&iter,2,res->country->iso2,-1);
				gtk_list_store_set(search->liststore,&iter,3,res->country->name,-1);
			}
		} else {
			if (res->country)
				gtk_list_store_set(search->liststore,&iter,0,res->country->car,-1);
			else
				gtk_list_store_set(search->liststore,&iter,0,"",-1);
			if (res->town) {
				gtk_list_store_set(search->liststore,&iter,1,res->town->postal,-1);
				gtk_list_store_set(search->liststore,&iter,2,res->town->name,-1);
				gtk_list_store_set(search->liststore,&iter,3,res->town->district,-1);
			} else {
				gtk_list_store_set(search->liststore,&iter,1,"",-1);
				gtk_list_store_set(search->liststore,&iter,2,"",-1);
				gtk_list_store_set(search->liststore,&iter,3,"",-1);
			}
			if (res->street)
				gtk_list_store_set(search->liststore,&iter,4,res->street->name,-1);
			else
				gtk_list_store_set(search->liststore,&iter,4,"",-1);

		}
	}
	if (! search->partial)
		next_focus(search, widget);
	search->partial=1;
}

/* borrowed from gpe-login */


#define MAX_ARGS 8

static void
parse_xkbd_args (const char *cmd, char **argv)
{
	const char *p = cmd;
	char buf[strlen (cmd) + 1], *bufp = buf;
	int nargs = 0;
	int escape = 0, squote = 0, dquote = 0;

	while (*p)
	{
		if (escape)
		{
			*bufp++ = *p;
			 escape = 0;
		}
		else
		{
			switch (*p)
			{
			case '\\':
				escape = 1;
				break;
			case '"':
				if (squote)
					*bufp++ = *p;
				else
					dquote = !dquote;
				break;
			case '\'':
				if (dquote)
					*bufp++ = *p;
				else
					squote = !squote;
				break;
			case ' ':
				if (!squote && !dquote)
				{
					*bufp = 0;
					if (nargs < MAX_ARGS)
					argv[nargs++] = strdup (buf);
					bufp = buf;
					break;
				}
			default:
				*bufp++ = *p;
				break;
			}
		}
		p++;
	}

	if (bufp != buf)
	{
		*bufp = 0;
		if (nargs < MAX_ARGS)
			argv[nargs++] = strdup (buf);
	}
	argv[nargs] = NULL;
}

int kbd_pid;

static int
spawn_xkbd (char *xkbd_path, char *xkbd_str)
{
#ifdef _WIN32 // AF FIXME for WIN32
    #ifndef F_SETFD
        #define F_SETFD 2
    #endif
#else
	char *xkbd_args[MAX_ARGS + 1];
	int fd[2];
	char buf[256];
	char c;
	int a = 0;
	size_t n;

	pipe (fd);
	kbd_pid = fork ();
	if (kbd_pid == 0)
	{
		close (fd[0]);
		if (dup2 (fd[1], 1) < 0)
			perror ("dup2");
		close (fd[1]);
		if (fcntl (1, F_SETFD, 0))
			perror ("fcntl");
		xkbd_args[0] = (char *)xkbd_path;
		xkbd_args[1] = "-xid";
		if (xkbd_str)
			parse_xkbd_args (xkbd_str, xkbd_args + 2);
		else
			xkbd_args[2] = NULL;
		execvp (xkbd_path, xkbd_args);
		perror (xkbd_path);
		_exit (1);
	}
	close (fd[1]);
	do {
		n = read (fd[0], &c, 1);
		if (n)
		{
			buf[a++] = c;
		}
	} while (n && (c != 10) && (a < (sizeof (buf) - 1)));

	if (a)
	{
		buf[a] = 0;
		return atoi (buf);
	}
#endif
	return 0;
}

int destination_address(struct navit *nav)
{

	GtkWidget *window2, *keyboard, *vbox, *table;
	GtkWidget *label_country;
	GtkWidget *label_postal, *label_city, *label_district;
	GtkWidget *label_street, *label_number;
	GtkWidget *hseparator1,*hseparator2;
	GtkWidget *button1,*button2,*button3;
	int i;
	struct search_param *search=&search_param;
	struct attr search_attr, country_name, *country_attr;
	struct tracking *tracking;
	struct country_search *cs;
	struct item *item;


	search->nav=nav;
	search->ms=navit_get_mapset(nav);
	search->sl=search_list_new(search->ms);

	window2 = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window2),_("Enter Destination"));
	vbox = gtk_vbox_new(FALSE, 0);
	table = gtk_table_new(3, 8, FALSE);

	search->entry_country = gtk_entry_new();
	label_country = gtk_label_new(_("Country"));
	search->entry_postal = gtk_entry_new();
	gtk_widget_set_sensitive(GTK_WIDGET(search->entry_postal), FALSE);
	label_postal = gtk_label_new(_("Zip Code"));
	search->entry_city = gtk_entry_new();
	label_city = gtk_label_new(_("City"));
	search->entry_district = gtk_entry_new();
	gtk_widget_set_sensitive(GTK_WIDGET(search->entry_district), FALSE);
	label_district = gtk_label_new(_("District/Township"));
	hseparator1 = gtk_vseparator_new();
	search->entry_street = gtk_entry_new();
	label_street = gtk_label_new(_("Street"));
	search->entry_number = gtk_entry_new();
	label_number = gtk_label_new(_("Number"));
 	search->treeview=gtk_tree_view_new();
	search->listbox = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (search->listbox),
                        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_tree_view_set_model (GTK_TREE_VIEW (search->treeview), NULL);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(search->listbox),search->treeview);
	{
		GType types[COL_COUNT+1];
		for(i=0;i<COL_COUNT;i++)
			types[i]=G_TYPE_STRING;
		types[i]=G_TYPE_POINTER;
                search->liststore=gtk_list_store_newv(COL_COUNT+1,types);
                search->liststore2=gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(search->liststore));
		gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (search->liststore2), 3, GTK_SORT_ASCENDING);
                gtk_tree_view_set_model (GTK_TREE_VIEW (search->treeview), GTK_TREE_MODEL(search->liststore2));
	}




	hseparator2 = gtk_vseparator_new();
	button1 = gtk_button_new_with_label(_("Map"));
	button2 = gtk_button_new_with_label(_("Bookmark"));
	button3 = gtk_button_new_with_label(_("Destination"));

	gtk_table_attach(GTK_TABLE(table), label_country,  0, 1,  0, 1,  0, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), label_postal,   1, 2,  0, 1,  0, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), label_city,     2, 3,  0, 1,  0, GTK_FILL, 0, 0);

	gtk_table_attach(GTK_TABLE(table), search->entry_country,  0, 1,  1, 2,  0, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), search->entry_postal,   1, 2,  1, 2,  0, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), search->entry_city,     2, 3,  1, 2,  0, GTK_FILL, 0, 0);

	gtk_table_attach(GTK_TABLE(table), label_district, 0, 1,  2, 3,  0, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), label_street,   1, 2,  2, 3,  0, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), label_number,   2, 3,  2, 3,  0, GTK_FILL, 0, 0);

	gtk_table_attach(GTK_TABLE(table), search->entry_district, 0, 1,  3, 4,  0, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), search->entry_street,   1, 2,  3, 4,  0, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), search->entry_number,   2, 3,  3, 4,  0, GTK_FILL, 0, 0);

	gtk_table_attach(GTK_TABLE(table), search->listbox,        0, 3,  4, 5,  GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);

	gtk_table_attach(GTK_TABLE(table), button1, 0, 1, 5, 6, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), button2, 1, 2, 5, 6, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), button3, 2, 3, 5, 6, GTK_FILL, GTK_FILL, 0, 0);

	g_signal_connect(G_OBJECT(search->entry_country), "changed", G_CALLBACK(changed), search);
	g_signal_connect(G_OBJECT(search->entry_postal), "changed", G_CALLBACK(changed), search);
	g_signal_connect(G_OBJECT(search->entry_city), "changed", G_CALLBACK(changed), search);
	g_signal_connect(G_OBJECT(search->entry_district), "changed", G_CALLBACK(changed), search);
	g_signal_connect(G_OBJECT(search->entry_street), "changed", G_CALLBACK(changed), search);
	g_signal_connect(G_OBJECT(search->entry_number), "changed", G_CALLBACK(changed), search);
	g_signal_connect(G_OBJECT(button1), "clicked", G_CALLBACK(button_map), search);
	g_signal_connect(G_OBJECT(button2), "clicked", G_CALLBACK(button_bookmark), search);
	g_signal_connect(G_OBJECT(button3), "clicked", G_CALLBACK(button_destination), search);
	g_signal_connect(G_OBJECT(search->treeview), "button-release-event", G_CALLBACK(tree_view_button_release), search);
	g_signal_connect(G_OBJECT(search->treeview), "row_activated", G_CALLBACK(row_activated), search);

	gtk_widget_grab_focus(search->entry_city);

	gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);
	keyboard=gtk_socket_new();
	gtk_box_pack_end(GTK_BOX(vbox), keyboard, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window2), vbox);
#if 0
	g_signal_connect(G_OBJECT(listbox), "select-row", G_CALLBACK(select_row), NULL);
#endif
	gtk_widget_show_all(window2);

#ifndef _WIN32
	gtk_socket_steal(GTK_SOCKET(keyboard), spawn_xkbd("xkbd","-geometry 200x100"));
#endif

	country_attr=country_default();
	tracking=navit_get_tracking(nav);
	if (tracking && tracking_get_current_attr(tracking, attr_country_id, &search_attr))
		country_attr=&search_attr;
	if (country_attr) {
		cs=country_search_new(country_attr, 0);
		item=country_search_get_item(cs);
		if (item && item_attr_get(item, attr_country_name, &country_name))
			gtk_entry_set_text(GTK_ENTRY(search->entry_country), country_name.u.str);
		country_search_destroy(cs);
	} else {
		dbg(0,"warning: no default country found\n");
	}
	search->partial=1;
	return 0;
}
