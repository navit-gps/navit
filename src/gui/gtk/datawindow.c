#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
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

static void
gui_gtk_datawindow_add(struct datawindow_priv *win, struct param_list *param, int count)
{
	int i;
	GtkCellRenderer *cell;
	GtkTreeIter iter;
	GType types[count];
	gchar *utf8;

	if (! win->treeview) {
		win->treeview=gtk_tree_view_new();
		gtk_tree_view_set_model (GTK_TREE_VIEW (win->treeview), NULL);
		gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(win->scrolled_window),win->treeview);
		gtk_widget_show_all(GTK_WIDGET(win->window));
		/* add column names to treeview */
		for(i=0;i<count;i++) {
			cell=gtk_cell_renderer_text_new();
			gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (win->treeview),-1,param[i].name,
					cell,"text",i, NULL);
		}
#if 0
		g_signal_connect(G_OBJECT(win->treeview), "click-column", G_CALLBACK(click_column), NULL);
		g_signal_connect(G_OBJECT(win->treeview), "row-activated", G_CALLBACK(select_row), win);
#endif
	}

	/* find data storage and create a new one if none is there */
	if (gtk_tree_view_get_model(GTK_TREE_VIEW (win->treeview)) == NULL) {
		for(i=0;i<count;i++) {
			if (! strcmp(param[i].name, "Distance")) 
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
		if (! strcmp(param[i].name, "Distance")) {
			gtk_list_store_set(win->liststore,&iter,i,atoi(param[i].value),-1);
		} else {
			utf8=g_locale_to_utf8(param[i].value,-1,NULL,NULL,NULL);
			gtk_list_store_set(win->liststore,&iter,i,utf8,-1);
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

static void
gui_gtk_datawindow_delete(GtkWidget *widget, GdkEvent *event, struct datawindow_priv *win)
{
	callback_call_0(win->close);
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

	win->scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (win->scrolled_window),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_container_add(GTK_CONTAINER(win->window), win->scrolled_window);

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

