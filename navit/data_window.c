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

