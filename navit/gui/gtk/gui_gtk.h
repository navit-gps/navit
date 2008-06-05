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

#include "coord.h"

struct menu_methods;
struct datawindow_methods;
struct navit;
struct callback;
struct statusbar_priv;

struct gui_priv {
	struct navit *nav;
        GtkWidget *win;
	GtkWidget *dialog_win;
	GtkWidget *dialog_entry;
	struct pcoord dialog_coord;
        GtkWidget *vbox;
	GtkWidget *menubar;
	GtkActionGroup *base_group;
	GtkActionGroup *debug_group;
	GtkActionGroup *dyn_group;
	GtkUIManager *ui_manager;
	GSList *layout_group;
	GSList *projection_group;
	GSList *vehicle_group;
	GtkUIManager *menu_manager; // old
        struct statusbar_priv *statusbar;
	int menubar_enable;
	int toolbar_enable;
	int statusbar_enable;
	int dyn_counter;
};

void gui_gtk_ui_init(struct gui_priv *this);
struct menu_priv *gui_gtk_menubar_new(struct gui_priv *gui, struct menu_methods *meth);
struct statusbar_priv *gui_gtk_statusbar_new(struct gui_priv *gui);
struct menu_priv *gui_gtk_popup_new(struct gui_priv *gui, struct menu_methods *meth);
struct datawindow_priv *gui_gtk_datawindow_new(struct gui_priv *gui, char *name, struct callback *click, struct callback *close, struct datawindow_methods *meth);
