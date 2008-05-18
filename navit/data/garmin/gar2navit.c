/*
	Copyright (C) 2007  Alexander Atanasov      <aatanasov@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2 of the License.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
	MA  02110-1301  USA
    
	Garmin and MapSource are registered trademarks or trademarks
	of Garmin Ltd. or one of its subsidiaries.

*/

/*
 Street's are routable by:
 ALL - by all
 W	pedestrian (1<<0)
 B	bycycle	   (1<<1)
 M	motorcycle (1<<2)
 C	car	   (1<<3)
 T	truck	   (1<<4)
 L	largetruck   (1<<5)
File format is:

POINT
0x0100 = town_label_1e5, Megapolis (10M +)
0x0200 = town_label_5e4, Megapolis (5-10M)
...
0x1e00-0x1e3f = district_label, District, Province, State Name
...
POLYLINE
0x00 = ALL, street_1_land, Road
0x01 = MCTL, highway_land, Major HWY thick
0x02 = MCTL, street_4_land, Principal HWY-thick
0x03 = MCTL, street_2_land, Principal HWY-medium
....
POLYGONE
0x01 = town_poly, City (>200k)
0x02 = town_poly, City (<200k)
0x03 = town_poly, Village

 */

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "item.h"
#include "attr.h"
#include "garmin.h"
#include "gar2navit.h"

static int add_def(struct gar2nav_conv *conv, int type, unsigned short minid,
		unsigned short maxid, unsigned int routable, char *ntype,
		char *descr)
{
	enum item_type it;
	struct gar2navit *g2n;
	dlog(11, "type=%d routable=%u min=%04X max=%04X ntype=%s descr=%s\n",
		type, routable, minid, maxid, ntype, descr);
	it = item_from_name(ntype);
	if (it==type_none) {
		dlog(1, "Please define: %s\n", ntype);
	} 
	g2n = calloc(1, sizeof(*g2n));
	if (!g2n)
		return -1;
	g2n->id = minid;
	g2n->maxid = maxid;
	g2n->ntype = it;
	g2n->descr = strdup(descr);
	g2n->routable = routable;
	if (type == 1) {
		g2n->next = conv->points;
		conv->points = g2n;
	} else if (type == 2) {
		g2n->next = conv->polylines;
		conv->polylines = g2n;
	} else if (type == 3) {
		g2n->next = conv->polygons;
		conv->polygons = g2n;
	}
	return 0;
}

static unsigned int get_rtmask(char *p)
{
	char *cp;
	unsigned int mask = 0;
	cp = p;
	while (*cp) {
		if (!strcasecmp(cp, "none"))
			return 0;
		if (!strcasecmp(cp, "all")) {
			mask = ~0;
			break;
		} if (*cp == 'W')
			mask |= RT_PEDESTRIAN;
		else if (*cp == 'B')
			mask |= RT_BYCYCLE;
		else if (*cp == 'M')
			mask |= RT_MOTORCYCLE;
		else if (*cp == 'C')
			mask |= RT_CAR;
		else if (*cp == 'T')
			mask |= RT_TRUCK;
		else if (*cp == 'L')
			mask |= RT_LONGTRUCK;
		cp++;
	}
	return mask;
}

static int load_types_file(char *file, struct gar2nav_conv *conv)
{
	char buf[4096];
	char descr[4096];
	char ntype[4096];
	char rtby[4096];
	FILE *fp;
	unsigned int minid, maxid, routable;
	int rc;
	int type = -1;

	fp = fopen(file, "r");
	if (!fp)
		return -1;
	while (fgets(buf, sizeof(buf), fp)) {
		if (*buf == '#' || *buf == '\n')
			continue;
		routable = 0;
		if (!strncasecmp(buf, "POINT", 5)) {
			type = 1;
			continue;
		} else if (!strncasecmp(buf, "POI", 3)) {
			type = 1;
			continue;
		} else if (!strncasecmp(buf, "POLYLINE", 8)) {
			type = 2;
			continue;
		} else if (!strncasecmp(buf, "POLYGONE", 8)) {
			type = 3;
			continue;
		}
		// assume only lines are routable
		if (type == 2) {
			rc = sscanf(buf, "0x%04X = %[^\t, ] , %[^\t, ], %[^\n]",
					&minid, rtby, ntype, descr);
				if (rc != 4) {
					dlog(1, "Invalid line rc=%d:[%s]\n",rc, buf);
					dlog(1, "minid=%04X ntype=[%s] des=[%s]\n",
						minid, ntype, descr);
					continue;
				}
				routable = get_rtmask(rtby);
		} else {
			rc = sscanf(buf, "0x%04X - 0x%04X = %[^\t , ] , %[^\n]",
				&minid, &maxid, ntype, descr);
			if (rc != 4) { 
				maxid = 0;
				rc = sscanf(buf, "0x%04X = %[^\t, ], %[^\n]",
					&minid, ntype, descr);
				if (rc != 3) {
					dlog(1, "Invalid line rc=%d:[%s]\n",rc, buf);
					dlog(1, "minid=%04X ntype=[%s] des=[%s]\n",
						minid, ntype, descr);
					continue;
				}
			}
		}
		add_def(conv, type, minid, maxid, routable, ntype, descr);
	}
	fclose(fp);
	return 1;
}

struct gar2nav_conv *g2n_conv_load(char *file)
{
	struct gar2nav_conv *c;
	int rc;

	c = calloc(1, sizeof(*c));
	if (!c)
		return c;
	rc = load_types_file(file, c);
	if (rc < 0) {
		dlog(1, "Failed to load: [%s]\n", file);
		free(c);
		return NULL;
	}
	return c;
}

enum item_type g2n_get_type(struct gar2nav_conv *c, int type, unsigned short id)
{
	struct gar2navit *def = NULL;
	if (type == G2N_POINT)
		def = c->points;
	else if (type == G2N_POLYLINE)
		def = c->polylines;
	else if (type == G2N_POLYGONE)
		def = c->polygons;
	else {
		dlog(1, "Unknown conversion type:%d\n", type);
		return type_none;
	}

	if (!def) {
		dlog(5, "No conversion data for %d\n", type);
		return type_none;
	}

	while (def) {
		if ((!def->maxid && def->id == id) || 
			(def->id <= id && id <= def->maxid))
			return def->ntype;
		def = def->next;
	}
	dlog(5, "Type[%d]:ID:[%04X] unknown\n", type, id);
	return type_none;
}

int g2n_get_routable(struct gar2nav_conv *c, int type, unsigned short id)
{
	struct gar2navit *def = NULL;
	if (type == G2N_POINT)
		def = c->points;
	else if (type == G2N_POLYLINE)
		def = c->polylines;
	else if (type == G2N_POLYGONE)
		def = c->polygons;
	else {
		dlog(1, "Unknown conversion type:%d\n", type);
		return type_none;
	}

	if (!def) {
		dlog(5, "No conversion data for %d\n", type);
		return type_none;
	}

	while (def) {
		if ((!def->maxid && def->id == id) || 
				(def->id <= id && id <= def->maxid))
			return def->routable;
		def = def->next;
	}
	dlog(5, "Type[%d]:ID:[%04X] unknown\n", type, id);
	return 0;
}

char *g2n_get_descr(struct gar2nav_conv *c, int type, unsigned short id)
{
	struct gar2navit *def = NULL;
	if (type == G2N_POINT)
		def = c->points;
	else if (type == G2N_POLYLINE)
		def = c->polylines;
	else if (type == G2N_POLYGONE)
		def = c->polygons;
	else {
		dlog(1, "Unknown conversion type:%d\n", type);
		return NULL;
	}
	while (def) {
		if ((!def->maxid && def->id == id) || 
				(def->id <= id && id <= def->maxid))
			return def->descr;
		def = def->next;
	}
	dlog(5, "Type[%d]:ID:[%04X] unknown\n", type, id);
	return NULL;
}

#if 0
int main(int argc, char **argv)
{
	load_types_file(argv[1], NULL);
	return 0;
}
#endif

#include "g2nbuiltin.h"
