/*
    Copyright (C) 2007  Alexander Atanasov	<aatanasov@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA


*/

#include <sys/types.h>

#ifdef _WIN32
#define u_int8_t  unsigned char
#define int8_t  char
#define int16_t short
#define u_int16_t unsigned short
#define int32_t int
#define u_int32_t unsigned int
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum {
	L_LBL,
	L_NET,
	L_POI,
};

struct gcoord {
	int x;
	int y;
};

#define GS_COUNTRY	1
#define GS_REGION	2
#define GS_CITY		3
#define GS_ZIP		4
#define GS_ROAD		5
#define GS_INTERSECT	6
#define GS_HOUSENUMBER	7
#define GS_POI		8

#define GM_EXACT	0
#define GM_START	1
#define GM_ANY		2

struct gar_search {
	unsigned char type;
	unsigned char match;
	char *needle;
	struct gobject *fromobj;
};

#define GO_POINT	1
#define GO_POI 		2
#define GO_POLYLINE	3
#define GO_POLYGON	4
#define GO_ROAD		5

struct gar_subfile;
struct gar_objdraworder;

struct gmap {
	struct gar_objdraworder *draworder;
	int subfiles;
	int lastsub;
	struct gar_subfile **subs;
	int zoomlevels;
	int basebits;
	int minlevel;
	int maxlevel;
};

struct gobject {
	int type;
	void *gptr;
	void *priv_data;
	struct gobject *next;
};

struct gar_rect	{
	double lulat;
	double lulong;
	double rllat;
	double rllong;
};


/* Walk and parse all data */
#define OPM_PARSE	(1<<0)
/* Call a callback w/ every object */
#define OPM_DUMP	(1<<1)
/* Work as a map backend */
#define OPM_GPS		(1<<2)

typedef void (*dump_fn)(struct gobject *obj);

#define DBGM_LBLS	(1<<0)
#define DBGM_OBJSRC	(1<<1)


struct gar_config {
	int opm;
	int maxsubdivs;		/* Load max N subdivs for OPM_GPS */
	dump_fn cb;		/* Callback function for OPM_DUMP */
	unsigned debugmask;	/* Debuging aid */
	int debuglevel;		/* Debug level */
};

struct gimg;
struct gar;

typedef void (*log_fn)(char *file, int line, int level, char *fmt, ...)
	__attribute__ ((format(printf,4,5)));
/* Default init w/ config, keep for the latest navit release */
struct gar *gar_init(char *tbd, log_fn l);
struct gar *gar_init_cfg(char *tbd, log_fn l, struct gar_config *cfg);
void gar_free(struct gar *g);
int gar_img_load(struct gar *gar, char *file, int data);
struct gmap *gar_find_subfiles(struct gar *gar, void *select, int flags);
void gar_free_gmap(struct gmap *g);
int gar_get_zoomlevels(struct gar_subfile *sub);

#define GO_GET_SORTED	(1<<0)
#define GO_GET_ROUTABLE	(1<<1)
#define GO_GET_SEARCH	(1<<2)

struct gobject *gar_get_object(struct gar *gar, void *ptr);
int gar_get_objects(struct gmap *gm, int level, void *select,
			struct gobject **ret, int flags);
void gar_free_objects(struct gobject *g);
u_int8_t gar_obj_type(struct gobject *o);
int gar_get_object_position(struct gobject *o, struct gcoord *ret);
int gar_object_subtype(struct gobject *o);
int gar_get_object_dcoord(struct gmap *gm, struct gobject *o, int ndelta, struct gcoord *ret);
int gar_get_object_coord(struct gmap *gm, struct gobject *o, struct gcoord *ret);

int gar_is_object_dcoord_node(struct gmap *gm, struct gobject *o, int ndelta);

int gar_get_object_deltas(struct gobject *o);
/* Get lbl as strdup'ed string */
char *gar_get_object_lbl(struct gobject *o);
int gar_get_object_intlbl(struct gobject *o);
int gar_object_get_draw_order(struct gobject *o);
char *gar_object_debug_str(struct gobject *o);
/* Object index is (subdividx << 16) | (idx << 8) | otype */
int gar_object_index(struct gobject *o);
/* Object mapid is the id of the file containing the object */
int gar_object_mapid(struct gobject *o);
struct gobject *gar_get_object_by_id(struct gar *gar, unsigned int mapid,
					unsigned int objid);

int gar_fat_file2fd(struct gimg *g, char *name, int fd);
/* Get ptr to a dskimg file */
struct gimg *gar_get_dskimg(struct gar *gar, char *file);

#define F_ONEWAY	(1<<0)
#define F_SEGMENTED	(1<<1)

int gar_object_flags(struct gobject *o);


#define GARDEG(x) ((x) < 0x800000 ? (double)(x) * 360.0 / 16777216.0 : -(double)((x) - 0x100000) * 360.0 / 16777216.0)
#define GARRAD(x) ((x) < 0x800000 ? (double)(x) * TWOPI / 16777216.0 : -(double)((x) - 0x100000) * TWOPI / 16777216.0)
#define DEGGAR(x) ((x) * (1/(360.0/(1<<24))))
#define FEET2METER(x) ((x)/3.28084)


#ifdef __cplusplus
}
#endif
