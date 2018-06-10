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
#include "menu.h"
#include "debug.h"

struct menu *
menu_add(struct menu *menu, char *name, enum menu_type type, struct callback *cb) {
    struct menu *this;
    if (! menu || ! menu->meth.add)
        return NULL;
    this=g_new0(struct menu, 1);
    this->priv=(*menu->meth.add)(menu->priv, &this->meth, name, type, cb);
    if (! this->priv) {
        g_free(this);
        return NULL;
    }

    return this;
}

void menu_popup(struct menu *menu) {
    if (! menu || ! menu->meth.popup)
        return;
    (*menu->meth.popup)(menu->priv);

}
