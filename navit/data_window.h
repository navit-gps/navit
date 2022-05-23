/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

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

