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
#include <ctype.h>
#include "util.h"

void
strtoupper(char *dest, const char *src)
{
	while (*src)
		*dest++=toupper(*src++);
	*dest='\0';
}

void
strtolower(char *dest, const char *src)
{
	while (*src)
		*dest++=tolower(*src++);
	*dest='\0';
}


static void
hash_callback(gpointer key, gpointer value, gpointer user_data)
{
	GList **l=user_data;
	*l=g_list_prepend(*l, value);
}

GList *
g_hash_to_list(GHashTable *h)
{
	GList *ret=NULL;
	g_hash_table_foreach(h, hash_callback, &ret);

	return ret;
}
