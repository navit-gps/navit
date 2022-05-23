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

#define GROUP_RGN1	0
#define GROUP_RGN2	1

struct gar2navit {
	unsigned short id;
	unsigned short maxid;
	enum item_type ntype;
	unsigned group;
	char *descr;
	struct gar2navit *next;
};

#define G2N_POINT		1
#define G2N_POLYLINE		2
#define G2N_POLYGONE		3
#define G2N_KIND_MASK		0xF0000000
#define G2N_KIND_SHIFT		28
#define G2N_RGN1		(GROUP_RGN1<<29)
#define G2N_RGN2		(GROUP_RGN2<<29)

struct gar2nav_conv {
	struct gar2navit *points;
	struct gar2navit *polylines;
	struct gar2navit *polygons;
};

struct gar2nav_conv *g2n_conv_load(char *file);
enum item_type g2n_get_type(struct gar2nav_conv *c, unsigned int type, unsigned short id);
char *g2n_get_descr(struct gar2nav_conv *c, int type, unsigned short id);
struct gar2nav_conv *g2n_default_conv(void);
