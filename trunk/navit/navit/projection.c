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
	{projection_utm, "utm"},
	{projection_gk, "gk"},
};


enum projection
projection_from_name(const char *name, struct coord *offset)
{
	int i;
	int zone;
	char ns;

	dbg(0,"name=%s\n",name);
	for (i=0 ; i < sizeof(projection_names)/sizeof(struct projection_name) ; i++) {
		if (! strcmp(projection_names[i].name, name))
			return projection_names[i].projection;
	}
	if (offset) {
		dbg(0,"%s %d\n",name,sscanf(name,"utm%d%c",&zone,&ns));
		if (sscanf(name,"utm%d%c",&zone,&ns) == 2 && zone > 0 && zone <= 60 && (ns == 'n' || ns == 's')) {
                	offset->x=zone*1000000;
			offset->y=(ns == 's' ? -10000000:0);
			return projection_utm;
		}
	}
	return projection_none;
}

char *
projection_to_name(enum projection proj, struct coord *offset)
{
	int i;

	for (i=0 ; i < sizeof(projection_names)/sizeof(struct projection_name) ; i++) {
		if (projection_names[i].projection == proj)
			return projection_names[i].name;
	}
	return NULL; 
}

