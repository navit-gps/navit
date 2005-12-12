
struct statusbar *gui_gtk_statusbar_new(GtkWidget **widget);
struct toolbar *gui_gtk_toolbar_new(struct container *co, GtkWidget **widget);
struct menu *gui_gtk_menu_new(struct container *co, GtkWidget **widget);
struct container * gui_gtk_window(int x, int y, int scale);
