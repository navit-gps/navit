struct data_window;
struct param_list;
struct window;

struct data_window *data_window(char *name, struct window *parent, void(*callback)(struct data_window *, char **cols));
void data_window_add(struct data_window *win, struct param_list *param, int count);
void data_window_begin(struct data_window *win);
void data_window_end(struct data_window *win);
