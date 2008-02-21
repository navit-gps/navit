#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <glib.h>
#include <assert.h>
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
#include "config.h"

#define BUFFER_SIZE 1280

/* #define GENERATE_INDEX */

#if 1
#define debug_tile(x) 0
#else
#define debug_tile(x) (!strcmp(x,"bcdbd") || !strcmp(x,"bcdbd") || !strcmp(x,"bcdbda") || !strcmp(x,"bcdbdb") || !strcmp(x,"bcdbdba") || !strcmp(x,"bcdbdbb") || !strcmp(x,"bcdbdbba") || !strcmp(x,"bcdbdbaa") || !strcmp(x,"bcdbdbacaa") || !strcmp(x,"bcdbdbacab") || !strcmp(x,"bcdbdbacaba") || !strcmp(x,"bcdbdbacabaa") || !strcmp(x,"bcdbdbacabab") || !strcmp(x,"bcdbdbacababb") || !strcmp(x,"bcdbdbacababba") || !strcmp(x,"bcdbdbacababbb") || !strcmp(x,"bcdbdbacababbd") || !strcmp(x,"bcdbdbacababaa") || !strcmp(x,"bcdbdbacababab") || !strcmp(x,"bcdbdbacababac") || !strcmp(x,"bcdbdbacababad") || !strcmp(x,"bcdbdbacabaaa") || !strcmp(x,"bcdbdbacabaaba") || !strcmp(x,"bcdbdbacabaabb") || !strcmp(x,"bcdbdbacabaabc") || !strcmp(x,"bcdbdbacabaabd") || !strcmp(x,"bcdbdbacabaaaa") || !strcmp(x,"bcdbdbacabaaab") || !strcmp(x,"bcdbdbacabaaac") || !strcmp(x,"bcdbdbacabaaad") || 0)
#endif


static GHashTable *dedupe_ways_hash;

static int attr_debug_level=1;
static int nodeid,wayid;
static int report,phase;
static int ignore_unkown = 0, coverage=0;

static char *attrmap={
	"n	historic\n"
	"n	type	junktion\n"
	"n	amenity	hospital	poi_hospital\n"
	"n	amenity	atm		poi_bank\n"
	"n	amenity	bank		poi_bank\n"
	"n	amenity	pub		poi_bar\n"
	"n	amenity	cafe		poi_cafe\n"
	"n	amenity	bus_station	poi_bus_station\n"
	"n	amenity	parking		poi_car_parking\n"
	"n	amenity	cinema		poi_cinema\n"
	"n	amenity	fire_station	poi_firebrigade\n"
	"n	amenity	fuel		poi_fuel\n"
	"n	amenity	courthouse	poi_justice\n"
	"n	amenity	library		poi_library\n"
	"n	amenity	pharmacy	poi_pharmacy\n"
	"n	amenity	place_of_worship	poi_church\n"
	"n	amenity	police		poi_police\n"
	"n	amenity	post_office	poi_post\n"
	"n	amenity	post_box	poi_post\n"
	"n	amenity	public_building	poi_public_office\n"
	"n	amenity	restaurant	poi_restaurant\n"
	"n	amenity	fast_food	poi_fastfood\n"
	"n	amenity	toilets		poi_restroom\n"
	"n	amenity	school		poi_school\n"
	"n	amenity	university	poi_school\n"
	"n	amenity	college		poi_school\n"
	"n	amenity	telephone	poi_telephone\n"
	"n	amenity	theatre		poi_theater\n"
	"n	highway	bus_stop	poi_bus_stop\n"
	"n	highway	motorway_junction	highway_exit\n"
	"n	highway	traffic_signals	traffic_signals\n"
	"n	leisure	slipway		poi_boat_ramp\n"
	"n	leisure	fishing		poi_fish\n"
	"n	sport	golf	poi_golf\n"
	"n	leisure	golf_course	poi_golf\n"
	"n	leisure	marina		poi_marine\n"
	"n	leisure	sports_centre	poi_sport\n"
	"n	leisure	stadium		poi_stadium\n"
	"n	shop	supermarket	poi_shopping\n"
	"n	shop	convenience	poi_shop_grocery\n"
	"n	tourism	attraction	poi_attraction\n"
	"n	tourism	camp_site	poi_camp_rv\n"
	"n	tourism	caravan_site	poi_camp_rv\n"
	"n	tourism	hotel		poi_hotel\n"
	"n	tourism	motel		poi_hotel\n"
	"n	tourism	guest_house	poi_hotel\n"
	"n	tourism	hostel		poi_hotel\n"
	"n	tourism	information	poi_information\n"
	"n	tourism	picnic_site	poi_picnic\n"
	"n	tourism	theme_park	poi_resort\n"
	"n	tourism	zoo		poi_zoo\n"
	"n	historic	museum	poi_museum_history\n"
	"n	amenity	grave_yard	poi_cemetery\n"
	"n	landuse	cemetery	poi_cemetery\n"
	"n	military	airfield	poi_military\n"
	"n	military	bunker		poi_military\n"
	"n	military	barracks	poi_military\n"
	"n	military	range		poi_military\n"
	"n	military	danger_area	poi_danger_area\n"
	"n	sport	swimming	poi_swimming\n"
	"n	sport	skiing		poi_skiing\n"
	"n	aeroway	aerodrome	poi_airport\n"
	"n	aeroway	airport		poi_airport\n"
	"n	aeroway	terminal	poi_airport\n"
	"n	aeroway	helipad		poi_heliport\n"
	"n	man_made	tower	poi_tower\n"
	"n	natural	bay		poi_bay\n"
	"n	place	suburb		district_label\n"
	"n	place	city		town_label_2e5\n"
	"n	place	town		town_label_2e4\n"
	"n	place	village		town_label_2e3\n"
	"n	place	hamlet		town_label_2e2\n"
	"w	amenity	place_of_worship	poly_building\n"
	"w	building	glasshouse	poly_building\n"
	"w	building	1	poly_building\n"
	"w	building\n"
	"w	aeroway	aerodrome	poly_airport\n"
	"w	aeroway	apron		poly_apron\n"
	"w	aeroway	runway		aeroway_runway\n"
	"w	aeroway	taxiway		aeroway_taxiway\n"
	"w	aeroway	terminal	poly_terminal\n"
	"w	amenity	parking		poly_car_parking\n"
	"w	highway	cycleway	street_nopass\n"
	"w	highway	footway		street_nopass\n"
	"w	highway	steps		street_nopass\n"
	"w	highway	cyclepath	street_nopass\n"
	"w	highway	track		street_nopass\n"
	"w	highway	service		street_service\n"
	"w	highway	pedestrian	street_pedestrian\n"
	"w	highway	residential	street_1_city\n"
	"w	highway	unclassified	street_1_city\n"
	"w	highway	minor		street_1_land\n"
	"w	highway	tertiary	street_2_city\n"
	"w	highway	secondary	street_3_city\n"
	"w	highway	unsurfaced	street_nopass\n"
	"w	highway	primary		street_4_city\n"
	"w	highway	primary_link	ramp\n"
	"w	highway	trunk		street_4_city\n"
	"w	highway	trunk_link	ramp\n"
	"w	highway	motorway	highway_city\n"
	"w	highway	motorway_link	ramp\n"
	"w	historic	town gate	poly_building\n"
	"w	landuse	allotments	poly_wood\n"
	"w	landuse	cemetery	poly_cemetery\n"
	"w	landuse	forest		poly_wood\n"
	"w	landuse	industrial	poly_industry\n"
	"w	landuse	residential	poly_town\n"
	"w	landuse	farm	poly_farm\n"
	"w	leisure	park		poly_park\n"
	"w	natural	wood		poly_wood\n"
	"w	natural	water		poly_water\n"
	"w	natural	coastline	water_line\n"
	"w	place		suburb		poly_town\n"
	"w	place		town		poly_town\n"
	"w	power	line	powerline\n"
	"w	railway	rail		rail\n"
	"w	railway	narrow_gauge	rail\n"
	"w	railway	station		poly_building\n"
	"w	railway	subway		rail\n"
	"w	railway	tram		rail\n"
	"w	leisure	golf_course	poly_golf_course\n"
	"w	waterway	canal		water_line\n"
	"w	waterway	river		water_line\n"
	"w	waterway	weir		water_line\n"
	"w	waterway	stream		water_line\n"
	"w	waterway	drain		water_line\n"
	"w	waterway	riverbank	poly_water\n"
	"w	boundary	administrative	border_country\n"
	"w	route		ferry		ferry\n"
};

#ifdef GENERATE_INDEX

struct country_table {
	int countryid;
	char *names;
	FILE *file;
} country_table[] = {
	{276,"Germany,Deutschland,Bundesrepublik Deutschland"},
};

static GHashTable *country_table_hash;
#endif

static GHashTable *way_key_hash, *node_key_hash;

static GHashTable *strings_hash = NULL;


static char* string_hash_lookup( const char* key )
{
	char* key_ptr = NULL;

	if ( strings_hash == NULL ) {
		strings_hash = g_hash_table_new(g_str_hash, g_str_equal);
	}

	if ( ( key_ptr = g_hash_table_lookup(strings_hash, key )) == NULL ) {
		key_ptr = g_strdup( key );
		g_hash_table_insert(strings_hash, key_ptr,  (gpointer)key_ptr );

	}
	return key_ptr;
}

static void
build_attrmap_line(char *line)
{
	char *t=NULL,*k=NULL,*v=NULL,*i=NULL,*p;
	gpointer *data;
	GHashTable *key_hash,*value_hash;
	t=line;
	p=strchr(t,'\t');
	if (p) {
		while (*p == '\t')
			*p++='\0';
		k=p;
		p=strchr(k,'\t');
	}
	if (p) {
		while (*p == '\t')
			*p++='\0';
		v=p;
		p=strchr(v,'\t');
	}
	if (p) {
		while (*p == '\t')
			*p++='\0';
		i=p;
	}
	if (t[0] == 'w') {
		if (! i)
			i="street_unkn";
		key_hash=way_key_hash;
	} else {
		if (! i)
			i="point_unkn";
		key_hash=node_key_hash;
	}
	value_hash=g_hash_table_lookup(key_hash, k);
	if (! value_hash) {
		value_hash=g_hash_table_new(g_str_hash, g_str_equal);
		g_hash_table_insert(key_hash, string_hash_lookup(k), value_hash);
	}
	if (v) {
		data=(gpointer)item_from_name(i);
		g_hash_table_insert(value_hash, string_hash_lookup(v), data);
	}
#if 0
	fprintf(stderr,"'%s' '%s' '%s'\n", k, v, i);
#endif
}

static void
build_attrmap(char *map)
{
	char *p;
	way_key_hash=g_hash_table_new(g_str_hash, g_str_equal);
	node_key_hash=g_hash_table_new(g_str_hash, g_str_equal);
	while (map) {
		p=strchr(map,'\n');
		if (p)
			*p++='\0';
		if (strlen(map))
			build_attrmap_line(map);
		map=p;
	}
}

#ifdef GENERATE_INDEX
static void
build_countrytable(void)
{
	int i;
	char *names,*str,*tok;
	country_table_hash=g_hash_table_new(g_str_hash, g_str_equal);
	for (i = 0 ; i < sizeof(country_table)/sizeof(struct country_table) ; i++) {
		names=g_strdup(country_table[i].names);
		str=names;
		while ((tok=strtok(str, ","))) {
			str=NULL;
			g_hash_table_insert(country_table_hash, tok,  (gpointer)&country_table[i]);
		}
	}
}
#endif



static int processed_nodes, processed_nodes_out, processed_ways, processed_relations, processed_tiles;
static int in_way, in_node, in_relation;

static void
sig_alrm(int sig)
{
#ifndef _WIN32
	signal(SIGALRM, sig_alrm);
#endif
	alarm(30);
	fprintf(stderr,"PROGRESS%d: Processed %d nodes (%d out) %d ways %d relations %d tiles\n", phase, processed_nodes, processed_nodes_out, processed_ways, processed_relations, processed_tiles);
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
char label_attr_buffer[BUFFER_SIZE];

struct attr_bin street_name_attr = {
	0, attr_street_name
};
char street_name_attr_buffer[BUFFER_SIZE];

struct attr_bin street_name_systematic_attr = {
	0, attr_street_name_systematic
};
char street_name_systematic_attr_buffer[BUFFER_SIZE];

struct attr_bin debug_attr = {
	0, attr_debug
};
char debug_attr_buffer[BUFFER_SIZE];

struct attr_bin flags_attr = {
	0, attr_flags
};
int flags_attr_value;

char is_in_buffer[BUFFER_SIZE];


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
	i=strchr(pos, s);
	if (! i)
		return 0;
	if (i - pos > buffer_size) {
		fprintf(stderr,"Buffer overflow %d vs %d\n", i-pos, buffer_size);
		return 0;
	}
	strncpy(buffer, pos, i-pos);
	buffer[i-pos]='\0';
	return 1;
}

static int node_is_tagged;

static void
add_tag(char *k, char *v)
{
	GHashTable *value_hash;
	enum item_type type;
	int level=2;
	if (! strcmp(k,"ele"))
		level=9;
	if (! strcmp(k,"time"))
		level=9;
	if (! strcmp(k,"created_by"))
		level=9;
	if (! strncmp(k,"tiger:",6) || !strcmp(k,"AND_nodes"))
		level=9;
	if (! strcmp(k,"converted_by") || ! strcmp(k,"source"))
		level=8;
	if (! strncmp(k,"osmarender:",11) || !strncmp(k,"svg:",4))
		level=8;
	if (! strcmp(k,"layer"))
		level=7;
	if (! strcasecmp(v,"true") || ! strcasecmp(v,"yes"))
		v="1";
	if (! strcmp(k,"oneway")) {
		if (!strcmp(v,"1")) {
			flags_attr_value=AF_ONEWAY;
			flags_attr.len=2;
		}
		if (! strcmp(v,"-1")) {
			flags_attr_value=AF_ONEWAYREV;
			flags_attr.len=2;
		}
		if (!in_way)
			level=6;
		else
			level=5;
	}
	if (! strcmp(k,"junction")) {
		if (! strcmp(v,"roundabout")) {
			flags_attr_value=AF_ONEWAY;
			flags_attr.len=2;
		}
	}
	if (! strcmp(k,"maxspeed")) {
		level=5;
	}
	if (! strcmp(k,"bicycle")) {
		level=5;
	}
	if (! strcmp(k,"foot")) {
		level=5;
	}
	if (! strcmp(k,"note"))
		level=5;
	if (! strcmp(k,"name")) {
		if (in_way) {
			strcpy(street_name_attr_buffer, v);
			pad_text_attr(&street_name_attr, street_name_attr_buffer);
		} else {
		strcpy(label_attr_buffer, v);
		pad_text_attr(&label_attr, label_attr_buffer);
		}
		level=5;
	}
	if (! strcmp(k,"ref")) {
		if (in_way) {
			strcpy(street_name_systematic_attr_buffer, v);
			pad_text_attr(&street_name_systematic_attr, street_name_systematic_attr_buffer);
		}
		level=5;
	}
	if (! strcmp(k,"is_in")) {
		strcpy(is_in_buffer, v);
		level=5;
	}
	if (! strcmp(k,"lanes")) {
		level=5;
	}
	if (attr_debug_level >= level) {
		int bytes_left = sizeof( debug_attr_buffer ) - strlen(debug_attr_buffer) - 1;
		if ( bytes_left > 0 )
		{
			snprintf(debug_attr_buffer+strlen(debug_attr_buffer), bytes_left,  " %s=%s", k, v);
			debug_attr_buffer[ sizeof( debug_attr_buffer ) -  1 ] = '\0';
			node_is_tagged=1;
		}
	}
	if (level < 6)
		node_is_tagged=1;
	if (level >= 5)
		return;
	if (in_way)
		value_hash=g_hash_table_lookup(way_key_hash, k);
	else
		value_hash=g_hash_table_lookup(node_key_hash, k);
	if (! value_hash) {
		if (report)
			fprintf(stderr,"Unknown %s %d key '%s' (value '%s')\n", in_way ? "way" : "node", in_way ? wayid:nodeid, k, v);
		return;
	}
	type=(enum item_type) g_hash_table_lookup(value_hash, v);
	if (!type) {
		if (report)
			fprintf(stderr,"Unknown %s %d value of '%s' '%s'\n", in_way ? "way" : "node", in_way ? wayid:nodeid, k, v);
		if ( ignore_unkown == 1 )
			return;
		type=in_way ? type_street_unkn : type_point_unkn;
	        g_hash_table_insert(value_hash, string_hash_lookup( v ), (gpointer)item.type);
	}
	if ( (type != type_street_unkn ) && ( type != type_point_unkn )  )
	{
		if (coverage && type >= type_street_nopass && type <= type_ramp)
			item.type=type_coverage;
		else
			item.type=type;
	}
	else
	{
        	if ( ignore_unkown == 1 )
		{
		    level = 10;
		}
	}
}

static int
parse_tag(char *p)
{
	char k_buffer[BUFFER_SIZE];
	char v_buffer[BUFFER_SIZE];
	if (!xml_get_attribute(p, "k", k_buffer, BUFFER_SIZE))
		return 0;
	if (!xml_get_attribute(p, "v", v_buffer, BUFFER_SIZE))
		return 0;
	add_tag(k_buffer, v_buffer);
	return 1;
}


struct buffer {
	int malloced_step;
	size_t malloced;
	unsigned char *base;
	size_t size;
};

static struct tile_head {
	int num_subtiles;
	int total_size;
	char *name;
	char *zip_data;
	int total_size_used;
	int zipnum;
	int process;
	struct tile_head *next;
	// char subtiles[0];
} *tile_head_root;


struct coord {
	int x;
	int y;
} coord_buffer[65536];

#define IS_REF(c) ((c).x >= (1 << 30))
#define REF(c) ((c).y)
#define SET_REF(c,ref) do { (c).x = 1 << 30; (c).y = ref ; } while(0)

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

static struct buffer node_buffer = {
	64*1024*1024,
};


static char** th_get_subtile( const struct tile_head* th, int idx )
{
	char* subtile_ptr = NULL;
	subtile_ptr = (char*)th + sizeof( struct tile_head ) + idx * sizeof( char *);
	return (char**)subtile_ptr;
}

static void
extend_buffer(struct buffer *b)
{
	b->malloced+=b->malloced_step;
	b->base=realloc(b->base, b->malloced);
	if (b->base == NULL) {
		fprintf(stderr,"realloc of %d bytes failed\n",b->malloced);
		exit(1);
	}

}

int nodeid_last;
GHashTable *node_hash;

static void
node_buffer_to_hash(void)
{
	int i,count=node_buffer.size/sizeof(struct node_item);
	struct node_item *ni=(struct node_item *)node_buffer.base;
	for (i = 0 ; i < count ; i++)
		g_hash_table_insert(node_hash, (gpointer)(ni[i].id), (gpointer)i);
}

static struct node_item *ni;

static void
add_node(int id, double lat, double lon)
{
	if (node_buffer.size + sizeof(struct node_item) > node_buffer.malloced)
		extend_buffer(&node_buffer);
	node_is_tagged=0;
	nodeid=id;
	item.type=type_point_unkn;
	label_attr.len=0;
	debug_attr.len=0;
	is_in_buffer[0]='\0';
	sprintf(debug_attr_buffer,"nodeid=%d", nodeid);
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
		if (ni->id > nodeid_last) {
			nodeid_last=ni->id;
		} else {
			fprintf(stderr,"INFO: Nodes out of sequence (new %d vs old %d), adding hash\n", ni->id, nodeid_last);
			node_hash=g_hash_table_new(NULL, NULL);
			node_buffer_to_hash();
		}
	} else
		if (!g_hash_table_lookup(node_hash, (gpointer)(ni->id)))
			g_hash_table_insert(node_hash, (gpointer)(ni->id), (gpointer)(ni-(struct node_item *)node_buffer.base));
		else {
			node_buffer.size-=sizeof(struct node_item);
			nodeid=0;
		}

}

static int
parse_node(char *p)
{
	char id_buffer[BUFFER_SIZE];
	char lat_buffer[BUFFER_SIZE];
	char lon_buffer[BUFFER_SIZE];
	if (!xml_get_attribute(p, "id", id_buffer, BUFFER_SIZE))
		return 0;
	if (!xml_get_attribute(p, "lat", lat_buffer, BUFFER_SIZE))
		return 0;
	if (!xml_get_attribute(p, "lon", lon_buffer, BUFFER_SIZE))
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


static void
add_way(int id)
{
	wayid=id;
	coord_count=0;
	item.type=type_street_unkn;
	street_name_attr.len=0;
	street_name_systematic_attr.len=0;
	debug_attr.len=0;
	flags_attr.len=0;
	sprintf(debug_attr_buffer,"wayid=%d", wayid);
}

static int
parse_way(char *p)
{
	char id_buffer[BUFFER_SIZE];
	if (!xml_get_attribute(p, "id", id_buffer, BUFFER_SIZE))
		return 0;
	add_way(atoi(id_buffer));
	return 1;
}

static int
parse_relation(char *p)
{
	debug_attr_buffer[0]='\0';
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

	if (! out)
		return;
	if (dedupe_ways_hash) {
		if (g_hash_table_lookup(dedupe_ways_hash, (gpointer)wayid))
			return;
		g_hash_table_insert(dedupe_ways_hash, (gpointer)wayid, (gpointer)1);
	}
	pad_text_attr(&debug_attr, debug_attr_buffer);
	if (street_name_attr.len)
		alen+=street_name_attr.len+1;
	if (street_name_systematic_attr.len)
		alen+=street_name_systematic_attr.len+1;
	if (debug_attr.len)
		alen+=debug_attr.len+1;
	if (flags_attr.len)
		alen+=flags_attr.len+1;
	item.clen=coord_count*2;
	item.len=item.clen+2+alen;
	fwrite(&item, sizeof(item), 1, out);
	fwrite(coord_buffer, coord_count*sizeof(struct coord), 1, out);
	write_attr(out, &street_name_attr, street_name_attr_buffer);
	write_attr(out, &street_name_systematic_attr, street_name_systematic_attr_buffer);
	write_attr(out, &debug_attr, debug_attr_buffer);
	write_attr(out, &flags_attr, &flags_attr_value);
}

static void
end_node(FILE *out)
{
	int alen=0;
	int conflict=0;
	struct country_table *result=NULL, *lookup;
	if (!out || ! node_is_tagged || ! nodeid)
		return;
	pad_text_attr(&debug_attr, debug_attr_buffer);
	if (label_attr.len)
		alen+=label_attr.len+1;
	if (debug_attr.len)
		alen+=debug_attr.len+1;
	item.clen=2;
	item.len=item.clen+2+alen;
	fwrite(&item, sizeof(item), 1, out);
	fwrite(&ni->c, 1*sizeof(struct coord), 1, out);
	write_attr(out, &label_attr, label_attr_buffer);
	write_attr(out, &debug_attr, debug_attr_buffer);
#ifdef GENERATE_INDEX
	if (item.type >= type_town_label && item.type <= type_town_label_1e7 && label_attr.len) {
		char *tok,*buf=is_in_buffer;
		while ((tok=strtok(buf, ","))) {
			while (*tok==' ')
				tok++;
			lookup=g_hash_table_lookup(country_table_hash,tok);
			if (lookup) {
				if (result && result->countryid != lookup->countryid) {
					fprintf(stderr,"conflict for %s %s country %d vs %d\n", label_attr_buffer, debug_attr_buffer, lookup->countryid, result->countryid);
					conflict=1;
				} else
					result=lookup;
			}
			buf=NULL;
		}
		if (result && !conflict) {
			if (!result->file) {
				char *name=g_strdup_printf("country_%d.bin", result->countryid);
				result->file=fopen(name,"w");
			}
			if (result->file) {
				item.clen=2;
				item.len=item.clen+2+label_attr.len+1;
				fwrite(&item, sizeof(item), 1, result->file);
				fwrite(&ni->c, 1*sizeof(struct coord), 1, result->file);
				write_attr(result->file, &label_attr, label_attr_buffer);
			}
			
		}
	}
#endif
	processed_nodes_out++;
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
		SET_REF(coord_buffer[coord_count], ref);
		coord_count++;
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
	char ref_buffer[BUFFER_SIZE];
	if (!xml_get_attribute(p, "ref", ref_buffer, BUFFER_SIZE))
		return 0;
	add_nd(p, atoi(ref_buffer));
	return 1;
}


static void
save_buffer(char *filename, struct buffer *b)
{
	FILE *f;
	f=fopen(filename,"wb+");
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
	f=fopen(filename,"rb");
	fseek(f, 0, SEEK_END);
	b->size=b->malloced=ftell(f);
	fprintf(stderr,"reading %d bytes from %s\n", b->size, filename);
	fseek(f, 0, SEEK_SET);
	b->base=malloc(b->size);
	assert(b->base != NULL);
	fread(b->base, b->size, 1, f);
	fclose(f);
}

static int
phase1(FILE *in, FILE *out_ways, FILE *out_nodes)
{
	int size=4096;
	char buffer[size];
	char *p;
	sig_alrm(0);
	while (fgets(buffer, size, in)) {
		p=strchr(buffer,'<');
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
			if (!parse_relation(p))
				fprintf(stderr,"WARNING: failed to parse %s\n", buffer);
			processed_relations++;
		} else if (!strncmp(p, "<member ",8)) {
		} else if (!strncmp(p, "</node>",7)) {
			in_node=0;
			end_node(out_nodes);
		} else if (!strncmp(p, "</way>",6)) {
			in_way=0;
			end_way(out_ways);
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

static char buffer[150000];

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
	assert((ib->len+1) < sizeof(buffer));
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
	while (--count) {
		c++;
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
tile(struct rect *r, char *ret, int max)
{
	int x0,x1,x2,x3,x4;
	int y0,y1,y2,y3,y4;
	int i;
	x0=world_bbox.l.x;
	y0=world_bbox.l.y;
	x4=world_bbox.h.x;
	y4=world_bbox.h.y;
	for (i = 0 ; i < max ; i++) {
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
GHashTable *tile_hash2;

static void
tile_extend(char *tile, struct item_bin *ib, GList **tiles_list)
{
	struct tile_head *th=NULL;
	if (debug_tile(tile))
		fprintf(stderr,"Tile:Writing %d bytes to '%s' (%p,%p)\n", (ib->len+1)*4, tile, g_hash_table_lookup(tile_hash, tile), tile_hash2 ? g_hash_table_lookup(tile_hash2, tile) : NULL);
	if (tile_hash2)
		th=g_hash_table_lookup(tile_hash2, tile);
	if (!th)
		th=g_hash_table_lookup(tile_hash, tile);
	if (! th) {
		th=malloc(sizeof(struct tile_head)+ sizeof( char* ) );
		assert(th != NULL);
		// strcpy(th->subtiles, tile);
		th->num_subtiles=1;
		th->total_size=0;
		th->total_size_used=0;
		th->zipnum=0;
		th->zip_data=NULL;
		th->name=string_hash_lookup(tile);
		*th_get_subtile( th, 0 ) = th->name;

		if (tile_hash2)
			g_hash_table_insert(tile_hash2, string_hash_lookup( th->name ), th);
		if (tiles_list)
			*tiles_list=g_list_append(*tiles_list, string_hash_lookup( th->name ) );
		processed_tiles++;
		if (debug_tile(tile))
			fprintf(stderr,"new '%s'\n", tile);
	}
	th->total_size+=ib->len*4+4;
	if (debug_tile(tile))
		fprintf(stderr,"New total size of %s(%p):%d\n", th->name, th, th->total_size);
	g_hash_table_insert(tile_hash, string_hash_lookup( th->name ), th);
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

static int
merge_tile(char *base, char *sub)
{
	struct tile_head *thb, *ths;
	thb=g_hash_table_lookup(tile_hash, base);
	ths=g_hash_table_lookup(tile_hash, sub);
	if (! ths)
		return 0;
	if (debug_tile(base) || debug_tile(sub))
		fprintf(stderr,"merging '%s'(%p) (%d) with '%s'(%p) (%d)\n", base, thb, thb ? thb->total_size : 0, sub, ths, ths->total_size);
	if (! thb) {
		thb=ths;
		g_hash_table_remove(tile_hash, sub);
		thb->name=string_hash_lookup(base);
		g_hash_table_insert(tile_hash, string_hash_lookup( thb->name ), thb);

	} else {
		thb=realloc(thb, sizeof(struct tile_head)+( ths->num_subtiles+thb->num_subtiles ) * sizeof( char*) );
		assert(thb != NULL);
		memcpy( th_get_subtile( thb, thb->num_subtiles ), th_get_subtile( ths, 0 ), ths->num_subtiles * sizeof( char*) );
		thb->num_subtiles+=ths->num_subtiles;
		thb->total_size+=ths->total_size;
		g_hash_table_insert(tile_hash, string_hash_lookup( thb->name ), thb);
		g_hash_table_remove(tile_hash, sub);
		g_free(ths);
	}
	return 1;
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

#if 0
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
	f=fopen(buffer, "wb+");
	while (th) {
		fwrite(th->data, th->size, 1, f);
		th=th->next;
	}
	fclose(f);
}
#endif

static void
write_item(char *tile, struct item_bin *ib)
{
	struct tile_head *th;
	int size;

	th=g_hash_table_lookup(tile_hash2, tile);
	if (! th)
		th=g_hash_table_lookup(tile_hash, tile);
	if (th) {
		if (th->process != 0 && th->process != 1) {
			fprintf(stderr,"error with tile '%s' of length %d\n", tile, strlen(tile));
			abort();
		}
		if (! th->process)
			return;
		if (debug_tile(tile))
			fprintf(stderr,"Data:Writing %d bytes to '%s' (%p,%p)\n", (ib->len+1)*4, tile, g_hash_table_lookup(tile_hash, tile), tile_hash2 ? g_hash_table_lookup(tile_hash2, tile) : NULL);
		size=(ib->len+1)*4;
		if (th->total_size_used+size > th->total_size) {
			fprintf(stderr,"Overflow in tile %s (used %d max %d item %d)\n", tile, th->total_size_used, th->total_size, size);
			exit(1);
			return;
		}
		memcpy(th->zip_data+th->total_size_used, ib, size);
		th->total_size_used+=size;
	} else {
		fprintf(stderr,"no tile hash found for %s\n", tile);
		exit(1);
	}
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

	processed_nodes=processed_nodes_out=processed_ways=processed_relations=processed_tiles=0;
	sig_alrm(0);
	while ((ib=read_item(in))) {
#if 0
		fprintf(stderr,"type 0x%x len %d clen %d\n", ib->type, ib->len, ib->clen);
#endif
		ccount=ib->clen/2;
		c=(struct coord *)(ib+1);
		last=0;
		for (i = 0 ; i < ccount ; i++) {
			if (IS_REF(c[i])) {
				ndref=REF(c[i]);
				ni=node_item_get(ndref);
#if 0
				fprintf(stderr,"ni=%p\n", ni);
#endif
				c[i]=ni->c;
				if (ni->ref_way > 1 && i != 0 && i != ccount-1 && ib->type >= type_street_nopass && ib->type <= type_ferry) {
					write_item_part(out, ib, last, i);
					last=i;
				}
			}
		}
		write_item_part(out, ib, last, ccount-1);
	}
	sig_alrm(0);
	alarm(0);
	return 0;
}

static void
phase34_process_file(int phase, FILE *in)
{
	struct item_bin *ib;
	struct rect r;
	char buffer[1024];
	int max;

	while ((ib=read_item(in))) {
		if (ib->type < 0x80000000)
			processed_nodes++;
		else
			processed_ways++;
		bbox((struct coord *)(ib+1), ib->clen/2, &r);
		buffer[0]='\0';
		max=14;
		if (ib->type == type_street_n_lanes || ib->type == type_highway_city || ib->type == type_highway_land || ib->type == type_ramp)
			max=8;
		if (ib->type == type_street_3_city || ib->type == type_street_4_city || ib->type == type_street_3_land || ib->type == type_street_4_land)
			max=12;

		tile(&r, buffer, max);
#if 0
		fprintf(stderr,"%s\n", buffer);
#endif
		if (phase == 3)
			tile_extend(buffer, ib, NULL);
		else
			write_item(buffer, ib);
	}
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
index_submap_add(int phase, struct tile_head *th, GList **tiles_list)
{
	struct index_item ii;
	int len=strlen(th->name);
	char index_tile[len+1];

	ii.min=(len > 4) ? len-4 : 0;
	ii.max=255;
	strcpy(index_tile, th->name);
	if (len > 6)
		len=6;
	else
		len=0;
	index_tile[len]=0;
	tile_bbox(th->name, &ii.r);

	ii.item.len=sizeof(ii)/4-1;
	ii.item.type=type_submap;
	ii.item.clen=4;

	ii.attr_order_limit.len=2;
	ii.attr_order_limit.type=attr_order_limit;

	ii.attr_zipfile_ref.len=2;
	ii.attr_zipfile_ref.type=attr_zipfile_ref;
	ii.zipfile_ref=th->zipnum;

	if (phase == 3)
		tile_extend(index_tile, (struct item_bin *)&ii, tiles_list);
	else
		write_item(index_tile, (struct item_bin *)&ii);
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
add_tile_hash(struct tile_head *th)
{
	int idx,len,maxnamelen=0;
	char **data;

#if 0
	g_hash_table_insert(tile_hash2, string_hash_lookup( th->name ), th);
#endif
	for( idx = 0; idx < th->num_subtiles; idx++ ) {

        data = th_get_subtile( th, idx );

		if (debug_tile(data) || debug_tile(th->name)) {
			fprintf(stderr,"Parent for '%s' is '%s'\n", *data, th->name);
		}

		g_hash_table_insert(tile_hash2, *data, th);

		len = strlen( *data );

		if (len > maxnamelen) {
			maxnamelen=len;
		}
	}
	return maxnamelen;
}


static int
create_tile_hash(void)
{
	struct tile_head *th;
	int len,maxnamelen=0;

	tile_hash2=g_hash_table_new(g_str_hash, g_str_equal);
	th=tile_head_root;
	while (th) {
		len=add_tile_hash(th);
		if (len > maxnamelen)
			maxnamelen=len;
		th=th->next;
	}
	return maxnamelen;
}

static void
create_tile_hash_list(GList *list)
{
	GList *next;
	struct tile_head *th;

	tile_hash2=g_hash_table_new(g_str_hash, g_str_equal);

	fprintf(stderr,"list=%p\n", list);
	next=g_list_first(list);
	while (next) {
		th=g_hash_table_lookup(tile_hash, next->data);
		if (!th) {
			fprintf(stderr,"No tile found for '%s'\n", (char *)(next->data));
		}
		add_tile_hash(th);
		next=g_list_next(next);
	}
}

static void
destroy_tile_hash(void)
{
	g_hash_table_destroy(tile_hash2);
	tile_hash2=NULL;
}


static void
write_tilesdir(int phase, int maxlen, FILE *out)
{
	int idx,len,zipnum=0;
	GList *tiles_list,*next;
	char **data;
	struct tile_head *th,**last=NULL;

	tiles_list=get_tiles_list();
	if (phase == 3)
		create_tile_hash_list(tiles_list);
	next=g_list_first(tiles_list);
	last=&tile_head_root;
	if (! maxlen) {
		while (next) {
			if (strlen(next->data) > maxlen)
				maxlen=strlen(next->data);
			next=g_list_next(next);
		}
	}
	len=maxlen;
	while (len >= 0) {
#if 0
		fprintf(stderr,"PROGRESS: collecting tiles with len=%d\n", len);
#endif
		next=g_list_first(tiles_list);
		while (next) {
			if (strlen(next->data) == len) {
				th=g_hash_table_lookup(tile_hash, next->data);
				if (phase == 3) {
					*last=th;
					last=&th->next;
					th->next=NULL;
					th->zipnum=zipnum++;
					fprintf(out,"%s:%d",(char *)next->data,th->total_size);

					for ( idx = 0; idx< th->num_subtiles; idx++ ){
                        data= th_get_subtile( th, idx );
						fprintf(out,":%s", *data);
					}

					fprintf(out,"\n");
				}
				if (th->name[0])
					index_submap_add(phase, th, &tiles_list);
				processed_tiles++;
			}
			next=g_list_next(next);
		}
		len--;
	}
}

static void
merge_tiles(void)
{
	struct tile_head *th;
	char basetile[1024];
	char subtile[1024];
	GList *tiles_list_sorted,*last;
	int i,i_min,len,size_all,size[5],size_min,work_done;
	long long zip_size;

	do {
		tiles_list_sorted=get_tiles_list();
		fprintf(stderr,"PROGRESS: sorting %d tiles\n", g_list_length(tiles_list_sorted));
		tiles_list_sorted=g_list_sort(tiles_list_sorted, (GCompareFunc)strcmp);
		fprintf(stderr,"PROGRESS: sorting %d tiles done\n", g_list_length(tiles_list_sorted));
		last=g_list_last(tiles_list_sorted);
		zip_size=0;
		while (last) {
			th=g_hash_table_lookup(tile_hash, last->data);
			zip_size+=th->total_size;
			last=g_list_previous(last);
		}
		fprintf(stderr,"DEBUG: size=%Ld\n", zip_size);
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
						work_done+=merge_tile(basetile, subtile);
					}
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
						work_done+=merge_tile(basetile, subtile);
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
}


static int
phase34(int phase, int maxnamelen, FILE *ways_in, FILE *nodes_in, FILE *tilesdir_out)
{

	processed_nodes=processed_nodes_out=processed_ways=processed_relations=processed_tiles=0;
	bytes_read=0;
	sig_alrm(0);
	if (phase == 3)
		tile_hash=g_hash_table_new(g_str_hash, g_str_equal);
	if (ways_in)
		phase34_process_file(phase, ways_in);
	if (nodes_in)
		phase34_process_file(phase, nodes_in);
	fprintf(stderr,"read %d bytes\n", bytes_read);
	if (phase == 3)
		merge_tiles();
	sig_alrm(0);
	alarm(0);
	write_tilesdir(phase, maxnamelen, tilesdir_out);

	return 0;

}

static int
phase3(FILE *ways_in, FILE *nodes_in, FILE *tilesdir_out)
{
	return phase34(3, 0, ways_in, nodes_in, tilesdir_out);
}

static long long zipoffset;
static int zipdir_size;

static int
compress2_int(Byte *dest, uLongf *destLen, const Bytef *source, uLong sourceLen, int level)
{
	z_stream stream;
	int err;

	stream.next_in = (Bytef*)source;
	stream.avail_in = (uInt)sourceLen;
	stream.next_out = dest;
	stream.avail_out = (uInt)*destLen;
	if ((uLong)stream.avail_out != *destLen) return Z_BUF_ERROR;

	stream.zalloc = (alloc_func)0;
	stream.zfree = (free_func)0;
	stream.opaque = (voidpf)0;

	err = deflateInit2(&stream, level, Z_DEFLATED, -15, 9, Z_DEFAULT_STRATEGY);
	if (err != Z_OK) return err;

	err = deflate(&stream, Z_FINISH);
	if (err != Z_STREAM_END) {
		deflateEnd(&stream);
		return err == Z_OK ? Z_BUF_ERROR : err;
	}
	*destLen = stream.total_out;

	err = deflateEnd(&stream);
	return err;
}

static void
write_zipmember(FILE *out, FILE *dir_out, char *name, int filelen, char *data, int data_size, int compression_level)
{
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
		0x0,
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
	int error,crc,len,comp_size=data_size;
	uLongf destlen=data_size+data_size/500+12;
	char compbuffer[destlen];

	crc=crc32(0, NULL, 0);
	crc=crc32(crc, (unsigned char *)data, data_size);
	if (compression_level) {
		error=compress2_int((Byte *)compbuffer, &destlen, (Bytef *)data, data_size, compression_level);
		if (error == Z_OK) {
			if (destlen < data_size) {
				data=compbuffer;
				comp_size=destlen;
			}
		} else {
			fprintf(stderr,"compress2 returned %d\n", error);
		}
	}
	lfh.zipcrc=crc;
	lfh.zipsize=comp_size;
	lfh.zipuncmp=data_size;
	lfh.zipmthd=compression_level ? 8:0;
	cd.zipccrc=crc;
	cd.zipcsiz=comp_size;
	cd.zipcunc=data_size;
	cd.zipcmthd=compression_level ? 8:0;
	strcpy(filename, name);
	len=strlen(filename);
	while (len < filelen) {
		filename[len++]='_';
	}
	filename[filelen]='\0';
	fwrite(&lfh, sizeof(lfh), 1, out);
	fwrite(filename, filelen, 1, out);
	fwrite(data, comp_size, 1, out);
	zipoffset+=sizeof(lfh)+filelen+comp_size;
	fwrite(&cd, sizeof(cd), 1, dir_out);
	fwrite(filename, filelen, 1, dir_out);
	zipdir_size+=sizeof(cd)+filelen;
}

static int
process_slice(FILE *ways_in, FILE *nodes_in, int size, int maxnamelen, FILE *out, FILE *dir_out, int compression_level)
{
	struct tile_head *th;
	char *slice_data,*zip_data;
	int zipfiles=0;

	slice_data=malloc(size);
	assert(slice_data != NULL);
	zip_data=slice_data;
	th=tile_head_root;
	while (th) {
		if (th->process) {
			th->zip_data=zip_data;
			zip_data+=th->total_size;
		}
		th=th->next;
	}
	if (ways_in)
		fseek(ways_in, 0, SEEK_SET);
	if (nodes_in)
		fseek(nodes_in, 0, SEEK_SET);
	phase34(4, maxnamelen, ways_in, nodes_in, NULL);

	th=tile_head_root;
	while (th) {
		if (th->process) {
			if (th->total_size != th->total_size_used) {
				fprintf(stderr,"Size error '%s': %d vs %d\n", th->name, th->total_size, th->total_size_used);
				exit(1);
			} else {
				if (strlen(th->name))
					write_zipmember(out, dir_out, th->name, maxnamelen, th->zip_data, th->total_size, compression_level);
				else
					write_zipmember(out, dir_out, "index", sizeof("index")-1, th->zip_data, th->total_size, compression_level);
				zipfiles++;
			}
		}
		th=th->next;
	}
	free(slice_data);

	return zipfiles;
}

static void
cat(FILE *in, FILE *out)
{
	size_t size;
	char buffer[4096];
	while ((size=fread(buffer, 1, 4096, in)))
		fwrite(buffer, 1, size, out);
}

static int
phase4(FILE *ways_in, FILE *nodes_in, FILE *out, FILE *dir_out, int compression_level)
{
	int slice_size=1024*1024*1024;
	int maxnamelen,size,slices;
	int zipfiles=0;
	struct tile_head *th,*th2;
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

	maxnamelen=create_tile_hash();

	th=tile_head_root;
	size=0;
	slices=0;
	fprintf(stderr, "Maximum slice size %d\n", slice_size);
	while (th) {
		if (size + th->total_size > slice_size) {
			fprintf(stderr,"Slice %d is of size %d\n", slices, size);
			size=0;
			slices++;
		}
		size+=th->total_size;
		th=th->next;
	}
	if (size)
		fprintf(stderr,"Slice %d is of size %d\n", slices, size);
	th=tile_head_root;
	size=0;
	slices=0;
	while (th) {
		th2=tile_head_root;
		while (th2) {
			th2->process=0;
			th2=th2->next;
		}
		size=0;
		while (th && size+th->total_size < slice_size) {
			size+=th->total_size;
			th->process=1;
			th=th->next;
		}
		zipfiles+=process_slice(ways_in, nodes_in, size, maxnamelen, out, dir_out, compression_level);
		slices++;
	}
	fseek(dir_out, 0, SEEK_SET);
	cat(dir_out, out);
	eoc.zipenum=zipfiles;
	eoc.zipecenn=zipfiles;
	eoc.zipecsz=zipdir_size;
	eoc.zipeofst=zipoffset;
	fwrite(&eoc, sizeof(eoc), 1, out);
	sig_alrm(0);
	alarm(0);
	return 0;
}

static void
usage(FILE *f)
{
	fprintf(f,"\n");
	fprintf(f,"osm2navit - parse osm textfile and converts to NavIt binfile format\n\n");
	fprintf(f,"Usage :\n");
	fprintf(f,"bzcat planet.osm.bz2 | osm2navit mymap.bin\n");
	fprintf(f,"Available switches:\n");
	fprintf(f,"-h (--help)              : this screen\n");
	fprintf(f,"-N (--nodes-only)        : process only nodes\n");
	fprintf(f,"-W (--ways-only)         : process only ways\n");
	fprintf(f,"-a (--attr-debug-level)  : control which data is included in the debug attribute\n");
	fprintf(f,"-c (--dump-coordinates)  : dump coordinates after phase 1\n");
	fprintf(f,"-e (--end)               : end at specified phase\n");
	fprintf(f,"-k (--keep-tmpfiles)     : do not delete tmp files after processing. useful to reuse them\n\n");
	fprintf(f,"-o (--coverage)          : map every street to item overage\n");
	fprintf(f,"-s (--start)             : start at specified phase\n");
	fprintf(f,"-i (--input-file)        : specify the input file name (OSM), overrules default stdin\n");
	fprintf(f,"-w (--dedupe-ways)       : ensure no duplicate ways or nodes. useful when using several input files\n");
	fprintf(f,"-z (--compression-level) : set the compression level\n");
	exit(1);
}

int main(int argc, char **argv)
{
	FILE *ways=NULL,*ways_split=NULL,*nodes=NULL,*tilesdir,*zipdir,*res;
	char *map=g_strdup(attrmap);
	int c,start=1,end=4,dump_coordinates=0;
	int keep_tmpfiles=0;
	int process_nodes=1, process_ways=1;
	int compression_level=9;
	char *result;
	FILE* input_file = stdin;


	while (1) {
#if 0
		int this_option_optind = optind ? optind : 1;
#endif
		int option_index = 0;
		static struct option long_options[] = {
			{"attr-debug-level", 1, 0, 'a'},
			{"compression-level", 1, 0, 'z'},
			{"coverage", 0, 0, 'o'},
			{"dedupe-ways", 0, 0, 'w'},
			{"end", 1, 0, 'e'},
			{"help", 0, 0, 'h'},
			{"keep-tmpfiles", 0, 0, 'k'},
			{"nodes-only", 0, 0, 'N'},
			{"start", 1, 0, 's'},
			{"input-file", 1, 0, 'i'},
			{"ignore-unknown", 0, 0, 'n'},
			{"ways-only", 0, 0, 'W'},
			{0, 0, 0, 0}
		};
		c = getopt_long (argc, argv, "Nni:Wa:ce:hks:w", long_options, &option_index);
		if (c == -1)
			break;
		switch (c) {
		case 'N':
			process_ways=0;
			break;
		case 'W':
			process_nodes=0;
			break;
		case 'a':
			attr_debug_level=atoi(optarg);
			break;
		case 'c':
			dump_coordinates=1;
			break;
		case 'e':
			end=atoi(optarg);
			break;
		case 'h':
			usage(stdout);
			break;
		case 'n':
			fprintf(stderr,"I will IGNORE unknown types\n");
			ignore_unkown=1;
			break;
		case 'k':
			fprintf(stderr,"I will KEEP tmp files\n");
			keep_tmpfiles=1;
			break;
		case 'o':
			coverage=1;
			break;
		case 's':
			start=atoi(optarg);
			break;
		case 'w':
			dedupe_ways_hash=g_hash_table_new(NULL, NULL);
			break;
		case 'i':
			input_file = fopen( optarg, "r" );
			if ( input_file ==  NULL )
			{
			    fprintf( stderr, "\nInput file (%s) not found\n", optarg );
			    exit( -1 );
			}
			break;
		case 'z':
			compression_level=atoi(optarg);
			break;
		case '?':
			usage(stderr);
			break;
		default:
			fprintf(stderr,"c=%d\n", c);
		}

	}
	if (optind != argc-1)
		usage(stderr);
	result=argv[optind];
	build_attrmap(map);
#ifdef GENERATE_INDEX
	build_countrytable();
#endif


	if (start == 1) {
		if (process_ways)
			ways=fopen("ways.tmp","wb+");
		if (process_nodes)
			nodes=fopen("nodes.tmp","wb+");
		phase=1;
		fprintf(stderr,"PROGRESS: Phase 1: collecting data\n");
		phase1(input_file,ways,nodes);
		if (ways)
			fclose(ways);
		if (nodes)
			fclose(nodes);
	}
	if (end == 1 || dump_coordinates)
		save_buffer("coords.tmp",&node_buffer);
	if (end == 1)
		exit(0);
	if (start == 2)
		load_buffer("coords.tmp",&node_buffer);
	if (start <= 2) {
		if (process_ways) {
			ways=fopen("ways.tmp","rb");
			ways_split=fopen("ways_split.tmp","wb+");
			phase=2;
			fprintf(stderr,"PROGRESS: Phase 2: finding intersections\n");
			phase2(ways,ways_split);
			fclose(ways_split);
			fclose(ways);
			if(!keep_tmpfiles)
				remove("ways.tmp");
		} else
			fprintf(stderr,"PROGRESS: Skipping Phase 2\n");
	}
	free(node_buffer.base);
	node_buffer.base=NULL;
	node_buffer.malloced=0;
	node_buffer.size=0;
	if (end == 2)
		exit(0);
	if (start <= 3) {
		phase=3;
		fprintf(stderr,"PROGRESS: Phase 3: generating tiles\n");
		if (process_ways)
			ways_split=fopen("ways_split.tmp","rb");
		if (process_nodes)
			nodes=fopen("nodes.tmp","rb");
		tilesdir=fopen("tilesdir.tmp","wb+");
		phase3(ways_split,nodes,tilesdir);
		fclose(tilesdir);
		if (nodes)
			fclose(nodes);
		if (ways_split)
			fclose(ways_split);
	}
	if (end == 3)
		exit(0);
	if (start <= 4) {
		phase=4;
		fprintf(stderr,"PROGRESS: Phase 4: assembling map\n");
		if (process_ways)
			ways_split=fopen("ways_split.tmp","rb");
		if (process_nodes)
			nodes=fopen("nodes.tmp","rb");
		res=fopen(result,"wb+");
		zipdir=fopen("zipdir.tmp","wb+");
		phase4(ways_split,nodes,res,zipdir,compression_level);
		fclose(zipdir);
		fclose(res);
		if (nodes)
			fclose(nodes);
		if (ways_split)
			fclose(ways_split);
		if(!keep_tmpfiles) {
			remove("nodes.tmp");
			remove("ways_split.tmp");
			remove("tilesdir.tmp");
			remove("zipdir.tmp");
		}
	}
	return 0;
}
