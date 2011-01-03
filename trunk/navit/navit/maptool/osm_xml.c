#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include "maptool.h"

static int
osm_xml_get_attribute(char *xml, char *attribute, char *buffer, int buffer_size)
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
		fprintf(stderr,"Buffer overflow %ld vs %d\n", (long)(i-pos), buffer_size);
		return 0;
	}
	strncpy(buffer, pos, i-pos);
	buffer[i-pos]='\0';
	return 1;
}

static struct entity {
	char *entity;
	char c;
} entities[]= {
	{"&quot;",'"'},
	{"&apos;",'\''},
	{"&amp;",'&'},
	{"&lt;",'<'},
	{"&gt;",'>'},
	{"&#34;",'"'},
	{"&#39;",'\''},
	{"&#38;",'&'},
	{"&#60;",'<'},
	{"&#62;",'>'},
};

static void
osm_xml_decode_entities(char *buffer)
{
	char *pos=buffer;
	int i,len,found;

	while ((pos=strchr(pos, '&'))) {
		found=0;
		for (i = 0 ; i < sizeof(entities)/sizeof(struct entity); i++) {
			len=strlen(entities[i].entity);
			if (!strncmp(pos, entities[i].entity, len)) {
				*pos=entities[i].c;
			 	memmove(pos+1, pos+len, strlen(pos+len)+1);
				found=1;
				break;
			}
		}
		pos++;
	}
}

static int
parse_tag(char *p)
{
	char k_buffer[BUFFER_SIZE];
	char v_buffer[BUFFER_SIZE];
	if (!osm_xml_get_attribute(p, "k", k_buffer, BUFFER_SIZE))
		return 0;
	if (!osm_xml_get_attribute(p, "v", v_buffer, BUFFER_SIZE))
		return 0;
	osm_xml_decode_entities(v_buffer);
	osm_add_tag(k_buffer, v_buffer);
	return 1;
}


static int
parse_node(char *p)
{
	char id_buffer[BUFFER_SIZE];
	char lat_buffer[BUFFER_SIZE];
	char lon_buffer[BUFFER_SIZE];
	if (!osm_xml_get_attribute(p, "id", id_buffer, BUFFER_SIZE))
		return 0;
	if (!osm_xml_get_attribute(p, "lat", lat_buffer, BUFFER_SIZE))
		return 0;
	if (!osm_xml_get_attribute(p, "lon", lon_buffer, BUFFER_SIZE))
		return 0;
	osm_add_node(atoll(id_buffer), atof(lat_buffer), atof(lon_buffer));
	return 1;
}


static int
parse_way(char *p)
{
	char id_buffer[BUFFER_SIZE];
	if (!osm_xml_get_attribute(p, "id", id_buffer, BUFFER_SIZE))
		return 0;
	osm_add_way(atoll(id_buffer));
	return 1;
}

static int
parse_relation(char *p)
{
	char id_buffer[BUFFER_SIZE];
	if (!osm_xml_get_attribute(p, "id", id_buffer, BUFFER_SIZE))
		return 0;
	osm_add_relation(atoll(id_buffer));
	return 1;
}

static int
parse_member(char *p)
{
	char type_buffer[BUFFER_SIZE];
	char ref_buffer[BUFFER_SIZE];
	char role_buffer[BUFFER_SIZE];
	int type;
	if (!osm_xml_get_attribute(p, "type", type_buffer, BUFFER_SIZE))
		return 0;
	if (!osm_xml_get_attribute(p, "ref", ref_buffer, BUFFER_SIZE))
		return 0;
	if (!osm_xml_get_attribute(p, "role", role_buffer, BUFFER_SIZE))
		return 0;
	if (!strcmp(type_buffer,"node")) 
		type=1;
	else if (!strcmp(type_buffer,"way")) 
		type=2;
	else if (!strcmp(type_buffer,"relation")) 
		type=3;
	else {
		fprintf(stderr,"Unknown type %s\n",type_buffer);
		type=0;
	}
	osm_add_member(type, atoll(ref_buffer), role_buffer);
	
	return 1;
}

static int
parse_nd(char *p)
{
	char ref_buffer[BUFFER_SIZE];
	if (!osm_xml_get_attribute(p, "ref", ref_buffer, BUFFER_SIZE))
		return 0;
	osm_add_nd(atoll(ref_buffer));
	return 1;
}

int
map_collect_data_osm(FILE *in, FILE *out_ways, FILE *out_nodes, FILE *out_turn_restrictions, FILE *out_boundaries)
{
	int size=BUFFER_SIZE;
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
			processed_nodes++;
		} else if (!strncmp(p, "<tag ",5)) {
			if (!parse_tag(p))
				fprintf(stderr,"WARNING: failed to parse %s\n", buffer);
		} else if (!strncmp(p, "<way ",5)) {
			if (!parse_way(p))
				fprintf(stderr,"WARNING: failed to parse %s\n", buffer);
			processed_ways++;
		} else if (!strncmp(p, "<nd ",4)) {
			if (!parse_nd(p))
				fprintf(stderr,"WARNING: failed to parse %s\n", buffer);
		} else if (!strncmp(p, "<relation ",10)) {
			if (!parse_relation(p))
				fprintf(stderr,"WARNING: failed to parse %s\n", buffer);
			processed_relations++;
		} else if (!strncmp(p, "<member ",8)) {
			if (!parse_member(p))
				fprintf(stderr,"WARNING: failed to parse %s\n", buffer);
		} else if (!strncmp(p, "</node>",7)) {
			osm_end_node(out_nodes);
		} else if (!strncmp(p, "</way>",6)) {
			osm_end_way(out_ways);
		} else if (!strncmp(p, "</relation>",11)) {
			osm_end_relation(out_turn_restrictions, out_boundaries);
		} else if (!strncmp(p, "</osm>",6)) {
		} else {
			fprintf(stderr,"WARNING: unknown tag in %s\n", buffer);
		}
	}
	sig_alrm(0);
	sig_alrm_end();
	return 1;
}
