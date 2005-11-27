#include <malloc.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include "param.h"
#include "data_window.h"
#include "data_window_int.h"

struct data_window *
data_window(char *name, struct window *parent, void(*callback)(struct data_window *, char **cols))
{
	struct data_window *win;

	win=malloc(sizeof(*win));
	win->window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(win->window), 320, 200);
	gtk_window_set_title(GTK_WINDOW(win->window), name);

	win->scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (win->scrolled_window),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_container_add(GTK_CONTAINER(win->window), win->scrolled_window);

	win->clist=NULL;
	win->callback=callback;
	if (parent) 
		gtk_window_set_transient_for(GTK_WINDOW((GtkWidget *)(win->window)), GTK_WINDOW(parent));
	gtk_widget_show_all(win->window);
	return win;
}

void
data_window_begin(struct data_window *win)
{
	if (win && win->clist) {
	       	gtk_clist_clear(GTK_CLIST(win->clist));
		gtk_clist_freeze(GTK_CLIST(win->clist));
	}
}

static void 
click_column(GtkCList *clist, int column)
{
	if (column != clist->sort_column) {
		gtk_clist_set_sort_type(clist, GTK_SORT_ASCENDING);
		gtk_clist_set_sort_column(clist, column);
	} else {
		if (clist->sort_type == GTK_SORT_ASCENDING)
			gtk_clist_set_sort_type(clist, GTK_SORT_DESCENDING);
		else
			gtk_clist_set_sort_type(clist, GTK_SORT_ASCENDING);
	}
	gtk_clist_sort(clist);
}

static void 
select_row(GtkCList *clist, int row, int column, GdkEventButton *event, struct data_window *win)
{
	int i;
	if (win->callback) {
		char *cols[20];
		for (i=0;i<20;i++) {
			gtk_clist_get_text(clist, row, i, &cols[i]);
		}	
		win->callback(win, cols);
	}
}

void
data_window_add(struct data_window *win, struct param_list *param, int count)
{
	int i;
	char *column[count+1];
	char *utf8;
	if (! win->clist) {
		for (i = 0 ; i < count ; i++) {
			column[i]=param[i].name;
		}
		win->clist=gtk_clist_new_with_titles(count, column);
        	gtk_clist_clear(GTK_CLIST(win->clist));
	        gtk_container_add(GTK_CONTAINER(win->scrolled_window), win->clist);  
		gtk_clist_freeze(GTK_CLIST(win->clist));
		gtk_signal_connect(GTK_OBJECT(win->clist), "click-column", GTK_SIGNAL_FUNC(click_column), NULL);
		gtk_signal_connect(GTK_OBJECT(win->clist), "select-row", GTK_SIGNAL_FUNC(select_row), win);
		gtk_widget_show_all(win->window);
	}
	for (i = 0 ; i < count ; i++) {
		utf8=g_locale_to_utf8(param[i].value,-1,NULL,NULL,NULL);
		column[i]=utf8;
	}
	column[i]=NULL;
	gtk_clist_append(GTK_CLIST(win->clist), column); 
	for (i = 0 ; i < count ; i++) {
		g_free(column[i]);
	}
}

void
data_window_end(struct data_window *win)
{
	if (win && win->clist) {
		gtk_clist_thaw(GTK_CLIST(win->clist));
		gtk_clist_columns_autosize (GTK_CLIST(win->clist));
	}
}
