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

#include <string.h>
#include <glib.h>
#include "coord.h"
#include "debug.h"
#include "projection.h"

struct projection_name {
        enum projection projection;
        char *name;
};


struct projection_name projection_names[]={
	{projection_none, ""},
	{projection_mg, "mg"},
	{projection_garmin, "garmin"},
};


enum projection
projection_from_name(const char *name)
{
	int i;

	for (i=0 ; i < sizeof(projection_names)/sizeof(struct projection_name) ; i++) {
		if (! strcmp(projection_names[i].name, name))
			return projection_names[i].projection;
	}
	return projection_none;
}

char *
projection_to_name(enum projection proj)
{
	int i;

	for (i=0 ; i < sizeof(projection_names)/sizeof(struct projection_name) ; i++) {
		if (projection_names[i].projection == proj)
			return projection_names[i].name;
	}
	return NULL; 
}

