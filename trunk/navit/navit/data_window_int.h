#ifndef NAVIT_DATA_WINDOW_INT_H
#define NAVIT_DATA_WINDOW_INT_H

struct data_window {
	GtkWidget *window;
	GtkWidget *scrolled_window;	
	GtkWidget *treeview;
	void(*callback)(struct data_window *, char **cols);
};

#endif

