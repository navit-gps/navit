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

#ifndef NAVIT_GUI_H
#define NAVIT_GUI_H

#ifdef __cplusplus
extern "C" {
#endif
struct navit;
struct gui_priv;
struct menu_methods;
struct datawindow_methods;
struct callback;
struct graphics;
struct coord;
struct pcoord;

struct gui_methods {
	struct menu_priv *(*menubar_new)(struct gui_priv *priv, struct menu_methods *meth);
	struct menu_priv *(*popup_new)(struct gui_priv *priv, struct menu_methods *meth);
	int (*set_graphics)(struct gui_priv *priv, struct graphics *gra);
	int (*run_main_loop)(struct gui_priv *priv);
	struct datawindow_priv *(*datawindow_new)(struct gui_priv *priv, const char *name, struct callback *click, struct callback *close, struct datawindow_methods *meth);
	int (*add_bookmark)(struct gui_priv *priv, struct pcoord *c, char *description);
	void (*disable_suspend)(struct gui_priv *priv);
	int (*get_attr)(struct gui_priv *priv, enum attr_type type, struct attr *attr);
	int (*add_attr)(struct gui_priv *priv, struct attr *attr);
	int (*set_attr)(struct gui_priv *priv, struct attr *attr);
};


/* prototypes */
enum attr_type;
struct attr;
struct attr_iter;
struct callback;
struct datawindow;
struct graphics;
struct gui;
struct menu;
struct pcoord;
struct gui *gui_new(struct attr *parent, struct attr **attrs);
int gui_get_attr(struct gui *this_, enum attr_type type, struct attr *attr, struct attr_iter *iter);
int gui_set_attr(struct gui *this_, struct attr *attr);
int gui_add_attr(struct gui *this_, struct attr *attr);
struct menu *gui_menubar_new(struct gui *gui);
struct menu *gui_popup_new(struct gui *gui);
struct datawindow *gui_datawindow_new(struct gui *gui, const char *name, struct callback *click, struct callback *close);
int gui_add_bookmark(struct gui *gui, struct pcoord *c, char *description);
int gui_set_graphics(struct gui *this_, struct graphics *gra);
void gui_disable_suspend(struct gui *this_);
int gui_has_main_loop(struct gui *this_);
int gui_run_main_loop(struct gui *this_);
/* end of prototypes */
#ifdef __cplusplus
}
#endif

#endif

