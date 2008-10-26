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
#include "debug.h"
#include "plugin.h"
#include "item.h"
#include "osd.h"


struct osd {
	struct osd_methods meth;
	struct osd_priv *priv;
};

struct osd *
osd_new(struct attr *parent, struct attr **attrs)
{
        struct osd *o;
        struct osd_priv *(*new)(struct navit *nav, struct osd_methods *meth, struct attr **attrs);
	struct attr *type=attr_search(attrs, NULL, attr_type);

	if (! type)
		return NULL;
        new=plugin_get_osd_type(type->u.str);
        if (! new)
                return NULL;
        o=g_new0(struct osd, 1);
        o->priv=new(parent->u.navit, &o->meth, attrs);
        return o;
}

