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

#ifndef NAVIT_MENU_H
#define NAVIT_MENU_H

enum menu_type {
	menu_type_submenu,
	menu_type_menu,
	menu_type_toggle,
};

struct container;
struct menu;
struct callback;

struct menu_methods {
	struct menu_priv *(*add)(struct menu_priv *menu, struct menu_methods *meth, char *name, enum menu_type type, struct callback *cb);
	void (*set_toggle)(struct menu_priv *menu, int active);
	int (*get_toggle)(struct menu_priv *menu);
	void (*popup)(struct menu_priv *menu);
};

struct menu {
	struct menu_priv *priv;
	struct menu_methods meth;
};

/* prototypes */
struct menu *menu_add(struct menu *menu, char *name, enum menu_type type, struct callback *cb);
void menu_popup(struct menu *menu);
/* end of prototypes */
#endif
