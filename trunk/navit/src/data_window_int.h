
struct data_window {
	GtkWidget *window;
	GtkWidget *scrolled_window;	
	GtkWidget *clist;
	void(*callback)(struct data_window *, char **cols);
};
