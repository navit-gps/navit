#include <glib.h>
#include "data_window.h"

void
datawindow_mode(struct datawindow *win, int start)
{
	win->meth.mode(win->priv, start);
}

void
datawindow_add(struct datawindow *win, struct param_list *param, int count)
{
	win->meth.add(win->priv, param, count);
}

void
datawindow_destroy(struct datawindow *win)
{
	win->meth.destroy(win->priv);
	g_free(win);
}

