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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include "debug.h"
#include "callback.h"
#include "param.h"
#include "data_window.h"
#include "gui_gtk.h"

struct datawindow_priv {
	GtkWidget *window;
	GtkWidget *scrolled_window;
	GtkWidget *treeview;
	GtkListStore *liststore;
	GtkTreeModel *sortmodel;
	struct callback *click, *close;
};

static void
gui_gtk_datawindow_destroy(struct datawindow_priv *win)
{
	return;
}

static GValue value;
static void
select_row(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *column, struct datawindow_priv *win)
{
	char *cols[20];
	GtkTreeIter iter;
	GtkTreeModel *model;
	int i;

	dbg(0,"win=%p\n", win);

	model=gtk_tree_view_get_model(tree);
	gtk_tree_model_get_iter(model, &iter, path);

	for (i=0;i<gtk_tree_model_get_n_columns(model);i++) {
		gtk_tree_model_get_value(model, &iter, i, &value);
		cols[i]=g_strdup_value_contents(&value)+1;
		cols[i][strlen(cols[i])-1]='\0';
		g_value_unset(&value);
	}
	callback_call_1(win->click, cols);
}

static void
gui_gtk_datawindow_add(struct datawindow_priv *win, struct param_list *param, int count)
{
	int i;
	GtkCellRenderer *cell;
	GtkTreeIter iter;
	GType types[count];

	if (! win->treeview) {
		win->treeview=gtk_tree_view_new();
		gtk_tree_view_set_model (GTK_TREE_VIEW (win->treeview), NULL);
		gtk_container_add(GTK_CONTAINER(win->scrolled_window), win->treeview);
		gtk_widget_show_all(GTK_WIDGET(win->window));
		gtk_widget_grab_focus(GTK_WIDGET(win->treeview));

		/* add column names to treeview */
		for(i=0;i<count;i++) {
			if (param[i].name) {
				cell=gtk_cell_renderer_text_new();
				gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (win->treeview),-1,param[i].name,
					cell,"text",i, NULL);
			}
		}
#if 0
		g_signal_connect(G_OBJECT(win->treeview), "click-column", G_CALLBACK(click_column), NULL);
#endif
		g_signal_connect(G_OBJECT(win->treeview), "row-activated", G_CALLBACK(select_row), win);
	}

	/* find data storage and create a new one if none is there */
	if (gtk_tree_view_get_model(GTK_TREE_VIEW (win->treeview)) == NULL) {
		for(i=0;i<count;i++) {
			if (param[i].name && !strcmp(param[i].name, "Distance"))
				types[i]=G_TYPE_INT;
			else
				types[i]=G_TYPE_STRING;
		}
	    	win->liststore=gtk_list_store_newv(count,types);
		if (! strcmp(param[0].name, "Distance")) {
			win->sortmodel=gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(win->liststore));
			gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (win->sortmodel), 0, GTK_SORT_ASCENDING);
			gtk_tree_view_set_model (GTK_TREE_VIEW (win->treeview), GTK_TREE_MODEL(win->sortmodel));
		} else
			gtk_tree_view_set_model (GTK_TREE_VIEW (win->treeview), GTK_TREE_MODEL(win->liststore));
	}

	gtk_list_store_append(win->liststore,&iter);

	/* add data to data storage */
	for(i=0;i<count;i++) {
		if (param[i].name && !strcmp(param[i].name, "Distance")) {
			gtk_list_store_set(win->liststore,&iter,i,atoi(param[i].value),-1);
		} else {
			gtk_list_store_set(win->liststore,&iter,i,param[i].value,-1);
		}
	}
}

static void
gui_gtk_datawindow_mode(struct datawindow_priv *win, int start)
{
	if (start) {
		if (win && win->treeview) {
			gtk_tree_view_set_model (GTK_TREE_VIEW (win->treeview), NULL);
		}
	}
}

static gboolean
gui_gtk_datawindow_delete(GtkWidget *widget, GdkEvent *event, struct datawindow_priv *win)
{
	callback_call_0(win->close);

	return FALSE;
}

static gboolean
keypress(GtkWidget *widget, GdkEventKey *event, struct datawindow_priv *win)
{
	if (event->type != GDK_KEY_PRESS)
		return FALSE;
	if (event->keyval == GDK_Cancel) {
		gui_gtk_datawindow_delete(widget, event, win);
		gtk_widget_destroy(win->window);
	}
	return FALSE;
}


static struct datawindow_methods gui_gtk_datawindow_meth = {
	gui_gtk_datawindow_destroy,
	gui_gtk_datawindow_add,
	gui_gtk_datawindow_mode,
};

struct datawindow_priv *
gui_gtk_datawindow_new(struct gui_priv *gui, char *name, struct callback *click, struct callback *close, struct datawindow_methods *meth)
{
	struct datawindow_priv *win;

	*meth=gui_gtk_datawindow_meth;
	win=g_new0(struct datawindow_priv, 1);
	win->window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(win->window), 320, 200);
	gtk_window_set_title(GTK_WINDOW(win->window), name);
	gtk_window_set_wmclass (GTK_WINDOW (win->window), "navit", "Navit");

	win->scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (win->scrolled_window),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(win->window), win->scrolled_window);
	g_signal_connect(G_OBJECT(win->window), "key-press-event", G_CALLBACK(keypress), win);
	win->treeview=NULL;
	win->click=click;
	win->close=close;
	if (gui)
		gtk_window_set_transient_for(GTK_WINDOW((GtkWidget *)(win->window)), GTK_WINDOW(gui->win));
	g_signal_connect(G_OBJECT(win->window), "delete-event", G_CALLBACK(gui_gtk_datawindow_delete), win);
	gtk_widget_show_all(win->window);
	return win;
	return NULL;
}

