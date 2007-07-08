#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "debug.h"
#include "plugin.h"
#include "map.h"
#include "maptype.h"
#include "item.h"
#include "attr.h"
#include "coord.h"
#include "transform.h"
#include "projection.h"

#include "textfile.h"

static int map_id;

static int
contains_coord(char *line)
{
	return g_ascii_isdigit(line[0]);
}

static int debug=0;

static int
get_tag(char *line, char *name, int *pos, char *ret, char *name_ret)
{
	int len=0,quoted;
	char *p,*e,*n;

	if (debug)
		printf("get_tag %s from %s\n", name, line); 
	if (name)
		len=strlen(name);
	if (pos) 
		p=line+*pos;
	else
		p=line;
	for(;;) {
		while (*p == ' ') {
			p++;
		}
		if (! *p)
			return 0;
		n=p;
		e=index(p,'=');
		if (! e)
			return 0;
		p=e+1;
		quoted=0;
		while (*p) {
			if (*p == ' ' && !quoted)
				break;
			if (*p == '"')
				quoted=1-quoted;
			p++;
		}
		if (name == NULL || (e-n == len && !strncmp(n, name, len))) {
			if (name_ret) {
				len=e-n;
				strncpy(name_ret, n, len);
				name_ret[len]='\0';
			}
			e++;
			len=p-e;
			if (e[0] == '"') {
				e++;
				len-=2;
			}
			strncpy(ret, e, len);
			ret[len]='\0';
			if (pos)
				*pos=p-line;
			return 1;
		}
	}	
	return 0;
}

static void
get_line(struct map_rect_priv *mr)
{
	if(mr->f) {
		mr->pos=ftell(mr->f);
		fgets(mr->line, SIZE, mr->f);
		if (strlen(mr->line) >= SIZE-1) 
			printf("line too long\n");
	}
}

static void
map_destroy_textfile(struct map_priv *m)
{
	if (debug)
		printf("map_destroy_textfile\n");
	g_free(m);
}

static void
textfile_coord_rewind(void *priv_data)
{
}

static void
parse_line(struct map_rect_priv *mr, int attr)
{
	int pos=0;
	sscanf(mr->line,"%lf %c %lf %c %n",&mr->lat,&mr->lat_c,&mr->lng,&mr->lng_c,&pos);
	if (pos < strlen(mr->line) && attr) {
		strcpy(mr->attrs, mr->line+pos);
	}
}

static int
textfile_coord_get(void *priv_data, struct coord *c, int count)
{
	double lat,lng;
	struct coord_geo cg;
	struct map_rect_priv *mr=priv_data;
	int ret=0;
	dbg(1,"textfile_coord_get %d\n",count);
	while (count--) {
		if (contains_coord(mr->line) && mr->f && !feof(mr->f) && (!mr->item.id_hi || !mr->eoc)) {
			parse_line(mr, mr->item.id_hi);
			lat=mr->lat;
			lng=mr->lng;
			dbg(1,"lat=%f lng=%f\n", lat, lng);
			cg.lat=floor(lat/100);
			lat-=cg.lat*100;
			cg.lat+=lat/60;

			cg.lng=floor(lng/100);
			lng-=cg.lng*100;
			cg.lng+=lng/60;

			transform_from_geo(projection_mg, &cg, c);
			dbg(1,"c=0x%x,0x%x\n", c->x, c->y);
			c++;
			ret++;		
			get_line(mr);
			if (mr->item.id_hi)
				mr->eoc=1;
		} else {
			break;
		}
	}
	return ret;
}

static void
textfile_attr_rewind(void *priv_data)
{
}

static void
textfile_encode_attr(char *attr_val, enum attr_type attr_type, struct attr *attr)
{
	if (attr_type >= attr_type_int_begin && attr_type <= attr_type_int_end) 
		attr->u.num=atoi(attr_val);
	else
		attr->u.str=attr_val;
}

static int
textfile_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr)
{	
	struct map_rect_priv *mr=priv_data;
	char *str=NULL;
	if (debug)
		printf("textfile_attr_get mr=%p attrs='%s' ", mr, mr->attrs);
	if (attr_type != mr->attr_last) {
		if (debug)
			printf("reset attr_pos\n");
		mr->attr_pos=0;
		mr->attr_last=attr_type;
	}
	if (attr_type == attr_any) {
		if (debug)
			printf("attr_any");
		if (get_tag(mr->attrs,NULL,&mr->attr_pos,mr->attr, mr->attr_name)) {
			attr_type=attr_from_name(mr->attr_name);
			if (debug)
				printf("found attr '%s' 0x%x\n", mr->attr_name, attr_type);
			attr->type=attr_type;
			textfile_encode_attr(mr->attr, attr_type, attr);
			return 1;
		}
	} else {
		str=attr_to_name(attr_type);
		if (debug)
			printf("attr='%s' ",str);
		if (get_tag(mr->attrs,str,&mr->attr_pos,mr->attr, NULL)) {
			textfile_encode_attr(mr->attr, attr_type, attr);
			if (debug)
				printf("found\n");
			return 1;
		}
	}
	if (debug)
		printf("not found\n");
	return 0;
}

static struct item_methods methods_textfile = {
        textfile_coord_rewind,
        textfile_coord_get,
        textfile_attr_rewind,
        textfile_attr_get,
};

static struct map_rect_priv *
map_rect_new_textfile(struct map_priv *map, struct map_selection *sel)
{
	struct map_rect_priv *mr;

	if (debug)
		printf("map_rect_new_textfile\n");
	mr=g_new0(struct map_rect_priv, 1);
	mr->m=map;
	mr->sel=sel;
	mr->item.id_hi=0;
	mr->item.id_lo=0;
	mr->item.meth=&methods_textfile;
	mr->item.priv_data=mr;
	mr->f=fopen(map->filename, "r");
	if(!mr->f) {
		printf("map_rect_new_textfile unable to open textfile %s\n",map->filename);
	}
	get_line(mr);
	return mr;
}


static void
map_rect_destroy_textfile(struct map_rect_priv *mr)
{
	if (mr->f) {
		fclose(mr->f);
	}
        g_free(mr);
}

static struct item *
map_rect_get_item_textfile(struct map_rect_priv *mr)
{
	char *p,type[SIZE];
	if (debug)
		printf("map_rect_get_item_textfile id_hi=%d line=%s", mr->item.id_hi, mr->line);
	if (!mr->f) {
		return NULL;
	}
	for(;;) {
		if (feof(mr->f)) {
			if (debug)
				printf("map_rect_get_item_textfile: eof\n");
			if (mr->item.id_hi) {
				return NULL;
			}
			mr->item.id_hi++;
			fseek(mr->f, 0, SEEK_SET);
			get_line(mr);
		}
		if (mr->item.id_hi) {
			if (!contains_coord(mr->line)) {
				get_line(mr);
				continue;
			}
			if ((p=index(mr->line,'\n'))) 
				*p='\0';
			if (debug)
				printf("map_rect_get_item_textfile: point found\n");
			mr->attrs[0]='\0';
			parse_line(mr, 1);
			mr->eoc=0;
			mr->item.id_lo=mr->pos;
		} else {
			if (contains_coord(mr->line)) {
				get_line(mr);
				continue;
			}
			if ((p=index(mr->line,'\n'))) 
				*p='\0';
			dbg(1,"map_rect_get_item_textfile: line found\n");
			if (! mr->line[0]) {
				get_line(mr);
				continue;
			}
			mr->item.id_lo=mr->pos;
			strcpy(mr->attrs, mr->line);
			get_line(mr);
			if (debug)
				printf("mr=%p attrs=%s\n", mr, mr->attrs);
		}
		if (debug)
			printf("get_attrs %s\n", mr->attrs);
		if (get_tag(mr->attrs,"type",NULL,type,NULL)) {
			if (debug)
				printf("type='%s'\n", type);
			mr->item.type=item_from_name(type);
			if (mr->item.type == type_none) 
				printf("Warning: type '%s' unknown\n", type);
		} else {
			get_line(mr);
			continue;
		}
		mr->attr_last=attr_none;
		if (debug)
			printf("return attr='%s'\n", mr->attrs);
		return &mr->item;
	}
}

static struct item *
map_rect_get_item_byid_textfile(struct map_rect_priv *mr, int id_hi, int id_lo)
{
	fseek(mr->f, id_lo, SEEK_SET);
	get_line(mr);
	mr->item.id_hi=id_hi;
	return map_rect_get_item_textfile(mr);
}

static struct map_methods map_methods_textfile = {
	map_destroy_textfile,
	map_rect_new_textfile,
	map_rect_destroy_textfile,
	map_rect_get_item_textfile,
	map_rect_get_item_byid_textfile,
};

static struct map_priv *
map_new_textfile(struct map_methods *meth, char *filename, char **charset, enum projection *pro)
{
	struct map_priv *m;
	if (debug)
		printf("map_new_textfile %s\n",filename);	
	*meth=map_methods_textfile;
	*charset="iso8859-1";
	*pro=projection_mg;

	m=g_new(struct map_priv, 1);
	m->id=++map_id;
	m->filename=g_strdup(filename);
	return m;
}

void
plugin_init(void)
{
	if (debug)
		printf("textfile: plugin_init\n");
	plugin_register_map_type("textfile", map_new_textfile);
}

