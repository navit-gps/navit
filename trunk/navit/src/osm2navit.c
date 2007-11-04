#include <glib.h>
#include <malloc.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <zlib.h>
#include "item.h"
#include "zipfile.h"

GHashTable *dedupe_ways_hash;


static char *attrmap={
	"amenity\n"
	"building\n"
	"highway	cycleway	street_nopass\n"
	"highway	footway		street_nopass\n"
	"highway	steps		street_nopass\n"
	"highway	cyclepath	street_nopass\n"
	"highway	track		street_nopass\n"
	"highway	service		street_nopass\n"
	"highway	pedestrian	street_nopass\n"
	"highway	residential	street_1_city\n"
	"highway	unclassified	street_1_city\n"
	"highway	tertiary	street_2_city\n"
	"highway	secondary	street_3_city\n"
	"highway	primary		street_4_city\n"
	"highway	trunk		street_4_city\n"
	"highway	trunk_link	ramp\n"
	"highway	motorway	highway_city\n"
	"highway	motorway_link	ramp\n"
	"landuse	allotments	wood\n"
	"landuse	cemetery	cemetery_poly\n"
	"landuse	forest		wood\n"
	"leisure	park		park_poly\n"
	"natural	wood		wood\n"
	"natural	water		water_poly\n"
	"place		suburb		town_poly\n"
	"railway	rail		rail\n"
	"railway	subway		rail\n"
	"railway	tram		rail\n"
	"waterway	canal		water_line\n"
	"waterway	river		water_line\n"
	"waterway	weir		water_line\n"
	"waterway	stream		water_line\n"
	"waterway	drain		water_line\n"
};

GHashTable *key_hash;

static void
build_attrmap_line(char *line)
{
	char *k=NULL,*v=NULL,*i=NULL,*p;
	gpointer *data;
	GHashTable *value_hash;
	k=line;
	p=index(k,'\t');
	if (p) {
		while (*p == '\t')
			*p++='\0';
		v=p;
		p=index(v,'\t');
	}
	if (p) {
		while (*p == '\t')
			*p++='\0';
		i=p;
	}
	if (! i)
		i="street_unkn";
	if (! key_hash)
		key_hash=g_hash_table_new(g_str_hash, g_str_equal);
	value_hash=g_hash_table_lookup(key_hash, k);
	if (! value_hash) {
		value_hash=g_hash_table_new(g_str_hash, g_str_equal);
		g_hash_table_insert(key_hash, g_strdup(k), value_hash);
	}
	if (v) {
		data=(gpointer)item_from_name(i);
		g_hash_table_insert(value_hash, g_strdup(v), data);
	}
#if 0
	fprintf(stderr,"'%s' '%s' '%s'\n", k, v, i);
#endif
}

static void
build_attrmap(char *map)
{
	char *p;
	while (map) {
		p=index(map,'\n');
		if (p)
			*p++='\0';
		if (strlen(map))
			build_attrmap_line(map);
		map=p;
	}
}

static int processed_nodes, processed_ways, processed_relations, processed_tiles;
static int in_way, in_node, in_relation;

static void
sig_alrm(int sig)
{
	signal(SIGALRM, sig_alrm);
	alarm(30);
	fprintf(stderr,"PROGRESS: Processed %d nodes %d ways %d relations %d tiles\n", processed_nodes, processed_ways, processed_relations, processed_tiles);
}

struct item_bin {
	int len;
	enum item_type type;
	int clen;
} item;

struct attr_bin {
	int len;
	enum attr_type type;
};

struct attr_bin label_attr = {
	0, attr_label
};
char label_attr_buffer[1024];

struct attr_bin debug_attr = {
	0, attr_debug
};
char debug_attr_buffer[1024];

struct attr_bin limit_attr = {
	0, attr_limit
};
int limit_attr_value;

static void
pad_text_attr(struct attr_bin *a, char *buffer)
{
	int l;
	l=strlen(buffer)+1;
	while (l % 4) 
		buffer[l++]='\0';
	a->len=l/4+1;
}

static int
xml_get_attribute(char *xml, char *attribute, char *buffer, int buffer_size)
{
	int len=strlen(attribute);
	char *pos,*i,s,attr[len+2];
	strcpy(attr, attribute);
	strcpy(attr+len, "=");
	pos=strstr(xml, attr);
	if (! pos)
		return 0;
	pos+=len+1;
	s=*pos++;
	if (! s)
		return 0;
	i=index(pos, s);
	if (! i)
		return 0;
	if (i - pos > buffer_size)
		return 0;
	strncpy(buffer, pos, i-pos);
	buffer[i-pos]='\0';
	return 1;
}


static void
add_tag(char *k, char *v)
{
	GHashTable *value_hash;
	if (! strcmp(k,"name")) {
		strcpy(label_attr_buffer, v);
		pad_text_attr(&label_attr, label_attr_buffer);
		return;
	}
	if (! strcmp(k,"oneway")) {
		if (! strcmp(v,"true") || !strcmp(v,"yes")) {
			limit_attr_value=1;
			limit_attr.len=2;
		}
		if (! strcmp(v,"-1")) {
			limit_attr_value=2;
			limit_attr.len=2;
		}
			
	}
	value_hash=g_hash_table_lookup(key_hash, k);
	if (! value_hash)
		return;
	item.type=(enum item_type) g_hash_table_lookup(value_hash, v);
	if (! item.type) {
		item.type=type_street_unkn;
		g_hash_table_insert(value_hash, v, (gpointer)item.type);
	}
}

static int
parse_tag(char *p)
{
	char k_buffer[1024];
	char v_buffer[1024];
	if (!xml_get_attribute(p, "k", k_buffer, 1024))
		return 0;
	if (!xml_get_attribute(p, "v", v_buffer, 1024))
		return 0;
	add_tag(k_buffer, v_buffer);
	return 1;
}


struct buffer {
	int malloced_step;
	int malloced;
	unsigned char *base;
	int size;
};

struct tile_head {
	int size;
	int total_size;
	char *name;
	struct tile_head *next;
	char data[0];
};


struct coord {
	int x;
	int y;
} coord_buffer[65536];

struct rect {
	struct coord l,h;
};

int coord_count;

struct node_item {
	int id;
	char ref_node;
	char ref_way;
	char ref_ref;
	char dummy;
	struct coord c;
};

struct buffer node_buffer = {
	64*1024*1024,
};

struct buffer zipdir_buffer = {
	1024*1024,
};

static void
extend_buffer(struct buffer *b)
{
	b->malloced+=b->malloced_step;
	b->base=realloc(b->base, b->malloced);
	
}

int node_id_last;
GHashTable *node_hash;

static void
node_buffer_to_hash(void)
{
	int i,count=node_buffer.size/sizeof(struct node_item);
	struct node_item *ni=(struct node_item *)node_buffer.base;
	for (i = 0 ; i < count ; i++) 
		g_hash_table_insert(node_hash, (gpointer)(ni[i].id), (gpointer)i);
}

static void
add_node(int id, double lat, double lon)
{
	struct node_item *ni;
	if (node_buffer.size + sizeof(struct node_item) > node_buffer.malloced) 
		extend_buffer(&node_buffer);
	ni=(struct node_item *)(node_buffer.base+node_buffer.size);
	ni->id=id;
	ni->ref_node=0;
	ni->ref_way=0;
	ni->ref_ref=0;
	ni->dummy=0;
	ni->c.x=lon*6371000.0*M_PI/180;
	ni->c.y=log(tan(M_PI_4+lat*M_PI/360))*6371000.0;
	node_buffer.size+=sizeof(struct node_item);
	if (! node_hash) {
		if (ni->id > node_id_last) {
			node_id_last=ni->id;
		} else {
			fprintf(stderr,"INFO: Nodes out of sequence (new %d vs old %d), adding hash\n", ni->id, node_id_last);
			node_hash=g_hash_table_new(NULL, NULL);
			node_buffer_to_hash();
		}
	} else
		if (!g_hash_table_lookup(node_hash, (gpointer)(ni->id))) 
			g_hash_table_insert(node_hash, (gpointer)(ni->id), (gpointer)(ni-(struct node_item *)node_buffer.base));
		else
			node_buffer.size-=sizeof(struct node_item);

}

static int
parse_node(char *p)
{
	char id_buffer[1024];
	char lat_buffer[1024];
	char lon_buffer[1024];
	if (!xml_get_attribute(p, "id", id_buffer, 1024))
		return 0;
	if (!xml_get_attribute(p, "lat", lat_buffer, 1024))
		return 0;
	if (!xml_get_attribute(p, "lon", lon_buffer, 1024))
		return 0;
	add_node(atoi(id_buffer), atof(lat_buffer), atof(lon_buffer));
	return 1;
}


static struct node_item *
node_item_get(int id)
{
	struct node_item *ni=(struct node_item *)(node_buffer.base);
	int count=node_buffer.size/sizeof(struct node_item);
	int interval=count/4;
	int p=count/2;
	if (node_hash) {
		int i;
		i=(int)(g_hash_table_lookup(node_hash, (gpointer)id));
		return ni+i;
	}
	while (ni[p].id != id) {
#if 0
		fprintf(stderr,"p=%d count=%d interval=%d id=%d ni[p].id=%d\n", p, count, interval, id, ni[p].id);
#endif
		if (ni[p].id < id) {
			p+=interval;
			if (interval == 1) {
				if (p >= count)
					return NULL;
				if (ni[p].id > id)
					return NULL;
			} else {
				if (p >= count)
					p=count-1;
			}
		} else {
			p-=interval;
			if (interval == 1) {
				if (p < 0)
					return NULL;
				if (ni[p].id < id)
					return NULL;
			} else {
				if (p < 0)
					p=0;
			}
		}
		if (interval > 1)
			interval/=2;
	}

	return &ni[p];
}

static void
node_ref_way(int id)
{
	struct node_item *ni=node_item_get(id);
	if (! ni) {
		fprintf(stderr,"WARNING: node id %d not found\n", id);
		return;
	}
	ni->ref_way++;
}

int wayid;

static void
add_way(int id)
{
	wayid=id;
	coord_count=0;
	item.type=type_street_unkn;
	label_attr.len=0;
	debug_attr.len=0;
	limit_attr.len=0;
	sprintf(debug_attr_buffer,"way_id=%d", wayid);
}

static int
parse_way(char *p)
{
	char id_buffer[1024];
	if (!xml_get_attribute(p, "id", id_buffer, 1024))
		return 0;
	add_way(atoi(id_buffer));
	return 1;
}

static void
write_attr(FILE *out, struct attr_bin *attr, void *buffer)
{
	if (attr->len) {
		fwrite(attr, sizeof(*attr), 1, out);
		fwrite(buffer, (attr->len-1)*4, 1, out);
	}
}

static void
end_way(FILE *out)
{
	int alen=0;

	if (dedupe_ways_hash) {
		if (g_hash_table_lookup(dedupe_ways_hash, (gpointer)wayid))
			return;
		g_hash_table_insert(dedupe_ways_hash, (gpointer)wayid, (gpointer)1);
	}
	pad_text_attr(&debug_attr, debug_attr_buffer);
	if (label_attr.len)
		alen+=label_attr.len+1;	
	if (debug_attr.len)
		alen+=debug_attr.len+1;	
	if (limit_attr.len)
		alen+=limit_attr.len+1;	
	item.clen=coord_count*2;
	item.len=item.clen+2+alen;
	fwrite(&item, sizeof(item), 1, out);
	fwrite(coord_buffer, coord_count*sizeof(struct coord), 1, out);
	write_attr(out, &label_attr, label_attr_buffer);
	write_attr(out, &debug_attr, debug_attr_buffer);
	write_attr(out, &limit_attr, &limit_attr_value);
}

static void
add_nd(char *p, int ref)
{
	int len;
	struct node_item *ni;
	ni=node_item_get(ref);
	if (ni) {
#if 0
		coord_buffer[coord_count++]=ni->c;
#else
		coord_buffer[coord_count].y=0;
		coord_buffer[coord_count++].x=ref;
#endif
		ni->ref_way++;
	} else {
		len=strlen(p);
		if (len > 0 && p[len-1]=='\n')
			p[len-1]='\0';	
		fprintf(stderr,"WARNING: way %d: node %d not found (%s)\n",wayid,ref,p);
	}
	if (coord_count > 65536) {
		fprintf(stderr,"ERROR: Overflow\n");
		exit(1);
	}
}

static int
parse_nd(char *p)
{
	char ref_buffer[1024];
	if (!xml_get_attribute(p, "ref", ref_buffer, 1024))
		return 0;
	add_nd(p, atoi(ref_buffer));
	return 1;
}


static void
save_buffer(char *filename, struct buffer *b)
{	
	FILE *f;
	f=fopen(filename,"w+");
	fwrite(b->base, b->size, 1, f);
	fclose(f);
}

static void
load_buffer(char *filename, struct buffer *b)
{
	FILE *f;
	if (b->base) 
		free(b->base);
	b->malloced=0;
	f=fopen(filename,"r");
	fseek(f, 0, SEEK_END);
	b->size=b->malloced=ftell(f);
	fprintf(stderr,"reading %d bytes from %s\n", b->size, filename);
	fseek(f, 0, SEEK_SET);
	b->base=malloc(b->size);
	fread(b->base, b->size, 1, f);
	fclose(f);
}

static int
phase1(FILE *in, FILE *out)
{
	int size=4096;
	char buffer[size];
	char *p;
	sig_alrm(0);
	while (fgets(buffer, size, in)) {
		p=index(buffer,'<');
		if (! p) {
			fprintf(stderr,"WARNING: wrong line %s\n", buffer);
			continue;
		}
		if (!strncmp(p, "<?xml ",6)) {
		} else if (!strncmp(p, "<osm ",5)) {
		} else if (!strncmp(p, "<bound ",7)) {
		} else if (!strncmp(p, "<node ",6)) {
			if (!parse_node(p)) 
				fprintf(stderr,"WARNING: failed to parse %s\n", buffer);
			in_node=1;
			processed_nodes++;
		} else if (!strncmp(p, "<tag ",5)) {
			if (!parse_tag(p)) 
				fprintf(stderr,"WARNING: failed to parse %s\n", buffer);
		} else if (!strncmp(p, "<way ",5)) {
			in_way=1;
			if (!parse_way(p)) 
				fprintf(stderr,"WARNING: failed to parse %s\n", buffer);
			processed_ways++;
		} else if (!strncmp(p, "<nd ",4)) {
			if (!parse_nd(p)) 
				fprintf(stderr,"WARNING: failed to parse %s\n", buffer);
		} else if (!strncmp(p, "<relation ",10)) {
			in_relation=1;
			processed_relations++;
		} else if (!strncmp(p, "<member ",8)) {
		} else if (!strncmp(p, "</node>",7)) {
			in_node=0;
		} else if (!strncmp(p, "</way>",6)) {
			in_way=0;
			end_way(out);
		} else if (!strncmp(p, "</relation>",11)) {
			in_relation=0;
		} else if (!strncmp(p, "</osm>",6)) {
		} else {
			fprintf(stderr,"WARNING: unknown tag in %s\n", buffer);
		}
	}
	sig_alrm(0);
	alarm(0);
	return 1;
}

static char buffer[65536];

int bytes_read=0;

static struct item_bin *
read_item(FILE *in)
{
	struct item_bin *ib=(struct item_bin *) buffer;	
	int r,s;
	r=fread(ib, sizeof(*ib), 1, in);
	if (r != 1)
		return NULL;
	bytes_read+=r;
	s=(ib->len+1)*4-sizeof(*ib);
	r=fread(ib+1, s, 1, in);
	if (r != 1)
		return NULL;
	bytes_read+=r;
	return ib;
}

static void
bbox(struct coord *c, int count, struct rect *r)
{
	if (! count)
		return;
	r->l=*c;
	r->h=*c;	
	c++;
	while (--count) {
		if (c->x < r->l.x)
			r->l.x=c->x;
		if (c->y < r->l.y)
			r->l.y=c->y;
		if (c->x > r->h.x)
			r->h.x=c->x;
		if (c->y > r->h.y)
			r->h.y=c->y;
	}
}

static int
contains_bbox(int xl, int yl, int xh, int yh, struct rect *r)
{
        if (r->h.x < xl || r->h.x > xh) {
                return 0;
        }
        if (r->l.x > xh || r->l.x < xl) {
                return 0;
        }
        if (r->h.y < yl || r->h.y > yh) {
                return 0;
        }
        if (r->l.y > yh || r->l.y < yl) {
                return 0;
        }
        return 1;

}
struct rect world_bbox = {
	{ -20000000, -20000000},
	{  20000000,  20000000},
};

static void
tile(struct rect *r, char *ret)
{
	int x0,x1,x2,x3,x4;
	int y0,y1,y2,y3,y4;
	int i;
	x0=world_bbox.l.x;
	y0=world_bbox.l.y;
	x4=world_bbox.h.x;
	y4=world_bbox.h.y;
	for (i = 0 ; i < 14 ; i++) {
		x2=(x0+x4)/2;
                y2=(y0+y4)/2;
                x1=(x0+x2)/2;
                y1=(y0+y2)/2;
                x3=(x2+x4)/2;
                y3=(y2+y4)/2;
     		if (     contains_bbox(x0,y0,x2,y2,r)) {
			strcat(ret,"d");
                        x4=x2;
                        y4=y2;
                } else if (contains_bbox(x2,y0,x4,y2,r)) {
			strcat(ret,"c");
                        x0=x2;
                        y4=y2;
                } else if (contains_bbox(x0,y2,x2,y4,r)) {
			strcat(ret,"b");
                        x4=x2;
                        y0=y2;
                } else if (contains_bbox(x2,y2,x4,y4,r)) {
			strcat(ret,"a");
                        x0=x2;
                        y0=y2;
		} else 
			return;
	}
}

static void
tile_bbox(char *tile, struct rect *r)
{
	*r=world_bbox;
	struct coord c;
	while (*tile) {
		c.x=(r->l.x+r->h.x)/2;
		c.y=(r->l.y+r->h.y)/2;
		switch (*tile) {
		case 'a':
			r->l.x=c.x;
			r->l.y=c.y;
			break;
		case 'b':
			r->h.x=c.x;
			r->l.y=c.y;
			break;
		case 'c':
			r->l.x=c.x;
			r->h.y=c.y;
			break;
		case 'd':
			r->h.x=c.x;
			r->h.y=c.y;
			break;
		}
		tile++;
	}
}

GHashTable *tile_hash;

static void
tile_data_append(char *tile, void *data, int len)
{
	struct tile_head *th;
	th=g_hash_table_lookup(tile_hash, tile);
	if (! th) {
		th=malloc(sizeof(*th)+len);
		th->size=0;
		th->total_size=0;
		th->name=g_strdup(tile);
		th->next=NULL;
		processed_tiles++;
#if 0
		fprintf(stderr,"new %s\n", tile);
#endif
	} else {
		th=realloc(th, sizeof(*th)+len+th->size);
	}
	memcpy(th->data+th->size, data, len);
	th->size+=len;
	th->total_size+=len;
	g_hash_table_insert(tile_hash, th->name, th);
}

static int
tile_data_size(char *tile)
{
	struct tile_head *th;
	th=g_hash_table_lookup(tile_hash, tile);
	if (! th)
		return 0;
	return th->total_size;
}

static void
merge_tiles(char *base, char *sub)
{
	struct tile_head *thb, *ths;
	thb=g_hash_table_lookup(tile_hash, base);
	ths=g_hash_table_lookup(tile_hash, sub);
	if (! ths)
		return;
#if 0
	fprintf(stderr,"merging %s with %s\n", base, sub);
#endif
	if (! thb) {
		thb=ths;
		thb->name=g_strdup(base);
		g_hash_table_insert(tile_hash, thb->name, thb);
	} else {
#if 0
		thb=realloc(thb, sizeof(*thb)+ths->size+thb->size);
		memcpy(thb->data+thb->size, ths->data, ths->size);
		thb->size+=ths->size;
		thb->total_size+=ths->total_size;
		g_hash_table_insert(tile_hash, thb->name, thb);
#if 0
		tiles_list=g_list_remove(tiles_list,ths->name);
#endif
		free(ths);
#endif
		thb->total_size+=ths->total_size;
		while (thb->next) 
			thb=thb->next;
		thb->next=ths;
	}
	g_hash_table_remove(tile_hash, sub);
}

static void
get_tiles_list_func(char *key, struct tile_head *th, GList **list)
{
	*list=g_list_prepend(*list, key);
}

static GList *
get_tiles_list(void)
{
	GList *ret=NULL;
	g_hash_table_foreach(tile_hash, (GHFunc)get_tiles_list_func, &ret);
	return ret;
}

static void
write_tile(char *key, struct tile_head *th, gpointer dummy)
{
	FILE *f;
	char buffer[1024];
	fprintf(stderr,"DEBUG: Writing %s\n", key);
	strcpy(buffer,"tiles/");
	strcat(buffer,key);
#if 0
	strcat(buffer,".bin");
#endif
	f=fopen(buffer, "w+");
	while (th) {
		fwrite(th->data, th->size, 1, f);
		th=th->next;
	}
	fclose(f);
}

static void
write_item_part(FILE *out, struct item_bin *orig, int first, int last)
{
	struct item_bin new;
	struct coord *c=(struct coord *)(orig+1);
	char *attr=(char *)(c+orig->clen/2);
	int attr_len=orig->len-orig->clen-2;
	processed_ways++;
	new.type=orig->type;
	new.clen=(last-first+1)*2;
	new.len=new.clen+attr_len+2;
#if 0
	fprintf(stderr,"first %d last %d type 0x%x len %d clen %d attr_len %d\n", first, last, new.type, new.len, new.clen, attr_len);
#endif
	fwrite(&new, sizeof(new), 1, out);
	fwrite(c+first, new.clen*4, 1, out);
	fwrite(attr, attr_len*4, 1, out);
}

static int
phase2(FILE *in, FILE *out)
{
	struct coord *c;
	int i,ccount,last,ndref;
	struct item_bin *ib;
	struct node_item *ni;

	processed_nodes=processed_ways=processed_relations=processed_tiles=0;
	sig_alrm(0);
	while ((ib=read_item(in))) {
#if 0
		fprintf(stderr,"type 0x%x len %d clen %d\n", ib->type, ib->len, ib->clen);
#endif
		ccount=ib->clen/2;
		c=(struct coord *)(ib+1);
		last=0;
		for (i = 0 ; i < ccount ; i++) {
			ndref=c[i].x;
			ni=node_item_get(ndref);
			c[i]=ni->c;
			if (ni->ref_way > 1 && i != 0 && i != ccount-1 && ib->type >= type_street_nopass && ib->type <= type_ferry) {
				write_item_part(out, ib, last, i);
				last=i;
			}
		}
		write_item_part(out, ib, last, ccount-1);
	}
	sig_alrm(0);
	alarm(0);
	return 0;
}

static int
phase3(FILE *in)
{
	struct item_bin *ib;
	struct tile_head *th;
	struct rect r;
	char buffer[1024];
	char basetile[1024];
	char subtile[1024];
	GList *tiles_list_sorted,*last;
	int i,i_min,len,size[5],size_all,size_min,work_done;
	

	processed_nodes=processed_ways=processed_relations=processed_tiles=0;
	sig_alrm(0);
	tile_hash=g_hash_table_new(g_str_hash, g_str_equal);
	while ((ib=read_item(in))) {
		processed_ways++;
		bbox((struct coord *)(ib+1), ib->clen/2, &r);
		buffer[0]='\0';
		tile(&r, buffer);
#if 0
		fprintf(stderr,"%s\n", buffer);
#endif
		tile_data_append(buffer, ib, ib->len*4+4);
	}
	fprintf(stderr,"read %d bytes\n", bytes_read);
	do {
		tiles_list_sorted=get_tiles_list();
		fprintf(stderr,"PROGRESS: sorting %d tiles\n", g_list_length(tiles_list_sorted));
		tiles_list_sorted=g_list_sort(tiles_list_sorted, (GCompareFunc)strcmp);
		fprintf(stderr,"PROGRESS: sorting %d tiles done\n", g_list_length(tiles_list_sorted));
		last=g_list_last(tiles_list_sorted);
		size_all=0;
		while (last) {
			th=g_hash_table_lookup(tile_hash, last->data);
			size_all+=th->total_size;
			last=g_list_previous(last);
		}
		fprintf(stderr,"DEBUG: size=%d\n", size_all);
		last=g_list_last(tiles_list_sorted);
		work_done=0;
		while (last) {
			processed_tiles++;
			len=strlen(last->data);
			if (len >= 1) {
				strcpy(basetile,last->data);
				basetile[len-1]='\0';
				strcpy(subtile,last->data);
				for (i = 0 ; i < 4 ; i++) {
					subtile[len-1]='a'+i;
					size[i]=tile_data_size(subtile);
				}
				size[4]=tile_data_size(basetile);
				size_all=size[0]+size[1]+size[2]+size[3]+size[4];
				if (size_all < 65536 && size_all > 0 && size_all != size[4]) {
					for (i = 0 ; i < 4 ; i++) {
						subtile[len-1]='a'+i;
						merge_tiles(basetile, subtile);
					}
					work_done++;
				} else {
					for (;;) {
						size_min=size_all;
						i_min=-1;
						for (i = 0 ; i < 4 ; i++) {
							if (size[i] && size[i] < size_min) {
								size_min=size[i];
								i_min=i;
							}
						}
						if (i_min == -1)
							break;
						if (size[4]+size_min >= 65536)
							break;
						subtile[len-1]='a'+i_min;
						merge_tiles(basetile, subtile);
						size[4]+=size[i_min];
						size[i_min]=0;
					}
				}
			}
			last=g_list_previous(last);
		}
		g_list_free(tiles_list_sorted);
		fprintf(stderr,"PROGRESS: merged %d tiles\n", work_done);
	} while (work_done);
	sig_alrm(0);
	alarm(0);
#if 0	
	g_hash_table_foreach(tile_hash, write_tile, NULL);
#endif
	return 0;
		
}

int
dir_entries;

static void
add_zipdirentry(char *name, struct zip_cd *cd)
{
	int cd_size=sizeof(struct zip_cd)+strlen(name);
	struct zip_cd *cdn;
	if (zipdir_buffer.size + cd_size > zipdir_buffer.malloced) 
		extend_buffer(&zipdir_buffer);
	cdn=(struct zip_cd *)(zipdir_buffer.base+zipdir_buffer.size);
	*cdn=*cd;
	strcpy((char *)(cdn+1), name);
	zipdir_buffer.size += cd_size;
	dir_entries++;
}

int zipoffset;

static void
write_zipmember(FILE *out, char *name, int filelen, struct tile_head *th)
{
	struct tile_head *thc;
	struct zip_lfh lfh = {
		0x04034b50,
		0x0a,
		0x0,
		0x0,
		0xbe2a,
		0x5d37,
		0x0,
		0x0,
		0x0,
		filelen,
		0x0,
	};
	struct zip_cd cd = {
		0x02014b50,
		0x17,
		0x00,
		0x0a,
		0x00,
		0x0000,
		0x0000,
		0xbe2a,
		0x5d37,
		0x0,
		0x0,
		0x0,
		filelen,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x0,
		zipoffset,
	};
	char filename[filelen+1];
	int size=0,crc,len;

	crc=crc32(0, NULL, 0);
	thc=th;
	while (thc) {
		size+=thc->size;
		crc=crc32(crc, (unsigned char *)(thc->data), thc->size);
		thc=thc->next;
	}
	if (size != th->total_size) {
		fprintf(stderr,"ERROR: size error %d vs %d\n", size, th->total_size);
	}
	lfh.zipcrc=crc;
	lfh.zipsize=size;
	lfh.zipuncmp=size;
	cd.zipccrc=crc;
	cd.zipcsiz=size;
	cd.zipcunc=size;
	strcpy(filename, name);
	len=strlen(filename);
	while (len < filelen) {
		filename[len++]='_';
	}
	filename[filelen]='\0';
	add_zipdirentry(filename, &cd);
	fwrite(&lfh, sizeof(lfh), 1, out);
	fwrite(filename, filelen, 1, out);
	thc=th;
	while (thc) {
		fwrite(thc->data, thc->size, 1, out);
		thc=thc->next;
	}
	zipoffset+=sizeof(lfh)+filelen+size;
	
}

struct index_item {
	struct item_bin item;
	struct rect r;
	struct attr_bin attr_order_limit;
	short min;
	short max;
	struct attr_bin attr_zipfile_ref;
	int zipfile_ref;
};

static void
index_submap_add(char *tile)
{
	struct index_item ii;
	int len=strlen(tile);
	char index_tile[len+1];
	strcpy(index_tile, tile);
	if (len > 6)
		len=6;
	else
		len=0;
	index_tile[len]=0;
	tile_bbox(tile, &ii.r);

	ii.item.len=sizeof(ii)/4-1;
	ii.item.type=type_submap;
	ii.item.clen=4;

	ii.attr_order_limit.len=2;
	ii.attr_order_limit.type=attr_order_limit;
	ii.min=0;
	ii.max=0;

	ii.attr_zipfile_ref.len=2;
	ii.attr_zipfile_ref.type=attr_zipfile_ref;
	ii.zipfile_ref=dir_entries;
	tile_data_append(index_tile, &ii, sizeof(ii));
#if 0
	unsigned int *c=(unsigned int *)&ii;
	int i;
	for (i = 0 ; i < sizeof(ii)/4 ; i++) {
		fprintf(stderr,"%08x ", c[i]);
	}
	fprintf(stderr,"\n");	
#endif
}

static int
phase4(FILE *out)
{
	struct zip_eoc eoc = {
		0x06054b50,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x0,
		0x0,
		0x0,
	};

	struct tile_head *th;
	GList *tiles_list,*next;
	int size=0,len,maxlen=0;

	processed_nodes=processed_ways=processed_relations=processed_tiles=0;
	sig_alrm(0);
	tiles_list=get_tiles_list();
	next=g_list_first(tiles_list);
	while (next) {
		if (strlen(next->data) > maxlen) 
			maxlen=strlen(next->data);
		next=g_list_next(next);
	}
	len=maxlen;
	while (len > 0) {
		fprintf(stderr,"PROGRESS: collecting tiles with len=%d\n", len);
		next=g_list_first(tiles_list);
		while (next) {
			if (strlen(next->data) == len) {
				index_submap_add(next->data);
				th=g_hash_table_lookup(tile_hash, next->data);
				write_zipmember(out, next->data, maxlen, th);
				processed_tiles++;
				size+=th->total_size;
			}
			next=g_list_next(next);
		}
		len--;
	}
	th=g_hash_table_lookup(tile_hash, "");
	if (th) {
		write_zipmember(out, "index", 5, th);
		processed_tiles++;
		size+=th->total_size;
		fprintf(stderr, "DEBUG: wrote %d bytes (zip files header)\n", (sizeof(struct zip_lfh)+maxlen)*dir_entries);
		fprintf(stderr, "DEBUG: wrote %d bytes (zip files)\n", size);
		fwrite(zipdir_buffer.base, zipdir_buffer.size, 1, out);
		fprintf(stderr, "DEBUG: wrote %d bytes (zip directory)\n", zipdir_buffer.size);
		eoc.zipenum=dir_entries;
		eoc.zipecenn=dir_entries;
		eoc.zipecsz=zipdir_buffer.size;
		eoc.zipeofst=zipoffset;
		fprintf(stderr, "DEBUG: wrote %d bytes (zip end of directory)\n", sizeof(eoc));
		fwrite(&eoc, sizeof(eoc), 1, out);
		fprintf(stderr, "DEBUG: overall size %d bytes\n", size+zipdir_buffer.size+sizeof(eoc));
	}
	sig_alrm(0);
	alarm(0);
	return 0;	
}

int main(int argc, char **argv)
{
	FILE *tmp1,*tmp2;
	char *map=g_strdup(attrmap);
	int c;

	while (1) {
#if 0
		int this_option_optind = optind ? optind : 1;
#endif
		int option_index = 0;
		static struct option long_options[] = {
			{"dedupe-ways", 0, 0, 'w'},
			{0, 0, 0, 0}
		};
		c = getopt_long (argc, argv, "w", long_options, &option_index);
		if (c == -1)
			break;
		switch (c) {
		case 'w':
			dedupe_ways_hash=g_hash_table_new(NULL, NULL);
			break;
		case '?':
			exit(1);
			break;
		default:
			printf("c=%d\n", c);
		}	

	}
	build_attrmap(map);


#if 1
	tmp1=fopen("tmpfile1","w+");
	fprintf(stderr,"PROGRESS: Phase 1: collecting data\n");
	phase1(stdin,tmp1);
	fclose(tmp1);
#if 0
	save_buffer("coords",&node_buffer);
#endif
#else
	load_buffer("coords",&node_buffer);
#endif
	tmp1=fopen("tmpfile1","r");
	tmp2=fopen("tmpfile2","w+");
	fprintf(stderr,"PROGRESS: Phase 2: finding intersections\n");
	phase2(tmp1,tmp2);
	fclose(tmp2);
	fclose(tmp1);
	free(node_buffer.base);
	node_buffer.base=NULL;
	node_buffer.malloced=0;
	node_buffer.size=0;
	fprintf(stderr,"PROGRESS: Phase 3: generating tiles\n");
	tmp2=fopen("tmpfile2","r");
	phase3(tmp2);
	fclose(tmp2);
	fprintf(stderr,"PROGRESS: Phase 4: assembling map\n");
	phase4(stdout);
	return 0;
}
