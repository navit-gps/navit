#ifndef NAVIT_DATA_WINDOW_H
#define NAVIT_DATA_WINDOW_H

struct datawindow;
struct param_list;
struct datawindow_priv;

struct datawindow_methods {
	void (*destroy)(struct datawindow_priv *win);
	void (*add)(struct datawindow_priv *win, struct param_list *param, int count);
	void (*mode)(struct datawindow_priv *win, int start);
};

struct datawindow {
	struct datawindow_priv *priv;
	struct datawindow_methods meth;
};


void datawindow_destroy(struct datawindow *win);
void datawindow_add(struct datawindow *win, struct param_list *param, int count);
void datawindow_mode(struct datawindow *win, int start);

#endif

