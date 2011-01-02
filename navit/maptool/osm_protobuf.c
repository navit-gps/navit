#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <zlib.h>
#include "maptool.h"
#include "debug.h"
#include "linguistics.h"
#include "file.h"
#include "generated-code/fileformat.pb-c.h"
#include "generated-code/osmformat.pb-c.h"


unsigned char buffer[65536];
double latlon_scale=10000000.0;
double timestamp_scale=1000.0;

OSMPBF__BlobHeader *
read_header(FILE *f)
{
	unsigned char *buffer,lenb[4];
	int len;

	len=sizeof(lenb);
	if (fread(lenb, 4, 1, f) != 1)
		return NULL;
	len=(lenb[0] << 24) | (lenb[1] << 16) | (lenb[2] << 8) | lenb[3];
	buffer=alloca(len);
	if (fread(buffer, len, 1, f) != 1)
		return NULL;
	return osmpbf__blob_header__unpack(&protobuf_c_system_allocator, len, buffer);
}


OSMPBF__Blob *
read_blob(OSMPBF__BlobHeader *header, FILE *f)
{
	unsigned char *buffer;
	int len=header->datasize;

	buffer=alloca(len);
	if (fread(buffer, len, 1, f) != 1)
		return NULL;
	return osmpbf__blob__unpack(&protobuf_c_system_allocator, len, buffer);
}

unsigned char *
uncompress_blob(OSMPBF__Blob *blob)
{
	unsigned char *ret=malloc(blob->raw_size);
	int zerr;
	z_stream strm;

	if (!ret)
		return NULL;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in=blob->zlib_data.len;
	strm.next_in=blob->zlib_data.data;
	strm.avail_out=blob->raw_size;
	strm.next_out=ret;
	zerr = inflateInit(&strm);
	if (zerr != Z_OK) {
		free(ret);
		return NULL;
	}
	zerr = inflate(&strm, Z_NO_FLUSH);
	if (zerr != Z_STREAM_END) {
		free(ret);
		return NULL;
	}
	inflateEnd(&strm);
	return ret;
}

int
get_string(char *buffer, int buffer_size, OSMPBF__PrimitiveBlock *primitive_block, int id, int escape)
{
	int len=primitive_block->stringtable->s[id].len;
	char *data=(char *)primitive_block->stringtable->s[id].data;
	if (primitive_block->stringtable->s[id].len >= buffer_size) {
		buffer[0]='\0';
		return 0;
	}
	if (escape) {
		int i;
		char *p=buffer;
		for (i = 0 ; i < len ; i++) {
			switch(data[i]) {
			case '\t':
			case '\r':
			case '\n':
			case '>':
			case '<':
			case '\'':
			case '"':
			case '&':
				sprintf(p,"&#%d;",data[i]);
				p+=strlen(p);
				break;
			default:
				*p++=data[i];
			}
		}
		*p++='\0';
		return 1;
	} else {
		strncpy(buffer, data, len);
		buffer[len]='\0';
		return 1;
	}
}


void
process_osmheader(OSMPBF__Blob *blob, unsigned char *data)
{
	OSMPBF__HeaderBlock *header_block;
	header_block=osmpbf__header_block__unpack(&protobuf_c_system_allocator, blob->raw_size, data);
	osmpbf__header_block__free_unpacked(header_block, &protobuf_c_system_allocator);
}

void
process_user(OSMPBF__PrimitiveBlock *primitive_block, int user_sid, int uid, int swap)
{
	char userbuff[1024];
	get_string(userbuff, sizeof(userbuff), primitive_block, user_sid, 1);
	if (userbuff[0] && uid != -1) {
		if (swap)
			printf(" uid=\"%d\" user=\"%s\"",uid,userbuff);
		else
			printf(" user=\"%s\" uid=\"%d\"",userbuff,uid);
	}
}

void
process_timestamp(long long timestamp)
{
	time_t ts;
	struct tm *tm;
	char tsbuff[1024];
	ts=timestamp;
	tm=gmtime(&ts);
	strftime(tsbuff, sizeof(tsbuff), "%Y-%m-%dT%H:%M:%SZ", tm);
	printf(" timestamp=\"%s\"",tsbuff);
}

void
process_tag(OSMPBF__PrimitiveBlock *primitive_block, int key, int val)
{
	char keybuff[1024];
	char valbuff[1024];
#if 0
	get_string(keybuff, sizeof(keybuff), primitive_block, key, 1);
	get_string(valbuff, sizeof(valbuff), primitive_block, val, 1);
	printf("\t\t<tag k=\"%s\" v=\"%s\" />\n",keybuff,valbuff);
#else
	get_string(keybuff, sizeof(keybuff), primitive_block, key, 0);
	get_string(valbuff, sizeof(valbuff), primitive_block, val, 0);
	osm_add_tag(keybuff, valbuff);
#endif
}


void
process_dense(OSMPBF__PrimitiveBlock *primitive_block, OSMPBF__DenseNodes *dense, FILE *out_nodes)
{
	int i,j=0,has_tags;
	long long id=0,lat=0,lon=0,changeset=0,timestamp=0;
	int version,user_sid=0,uid=0;

	if (!dense)
		return;

	for (i = 0 ; i < dense->n_id ; i++) {
		id+=dense->id[i];
		lat+=dense->lat[i];
		lon+=dense->lon[i];
		version=dense->denseinfo->version[i];
		changeset+=dense->denseinfo->changeset[i];
		user_sid+=dense->denseinfo->user_sid[i];
		uid+=dense->denseinfo->uid[i];
		timestamp+=dense->denseinfo->timestamp[i];
		has_tags=dense->keys_vals && dense->keys_vals[j];
		osm_add_node(id, lat/latlon_scale,lon/latlon_scale);
#if 0
		printf("\t<node id=\"%Ld\" lat=\"%.7f\" lon=\"%.7f\" version=\"%d\" changeset=\"%Ld\"",id,lat/latlon_scale,lon/latlon_scale,version,changeset);
		process_user(primitive_block, user_sid, uid, 0);
		process_timestamp(timestamp);
#endif
		if (has_tags) {
#if 0
			printf(">\n");
#endif
			while (dense->keys_vals[j]) {
				process_tag(primitive_block, dense->keys_vals[j], dense->keys_vals[j+1]);
				j+=2;
			}
#if 0
			printf("\t</node>\n");
		} else
			printf("/>\n");
#else
		}
#endif
		osm_end_node(out_nodes);
		j++;
	}
}

void
process_info(OSMPBF__PrimitiveBlock *primitive_block, OSMPBF__Info *info)
{
	printf(" version=\"%d\" changeset=\"%Ld\"",info->version,info->changeset);
	process_user(primitive_block, info->user_sid, info->uid, 1);
	process_timestamp(info->timestamp);
}

void
process_way(OSMPBF__PrimitiveBlock *primitive_block, OSMPBF__Way *way, FILE *out_ways)
{
	int i;
	long long ref=0;

	osm_add_way(way->id);
#if 0
	printf("\t<way id=\"%Ld\"",way->id);
	process_info(primitive_block, way->info);
	printf(">\n");
#else
#endif
	for (i = 0 ; i < way->n_refs ; i++) {
		ref+=way->refs[i];
		osm_add_nd(ref);
#if 0
		printf("\t\t<nd ref=\"%Ld\"/>\n",ref);
#else
#endif
	}
	for (i = 0 ; i < way->n_keys ; i++) 
		process_tag(primitive_block, way->keys[i], way->vals[i]);
#if 0
	printf("\t</way>\n");
#endif
	osm_end_way(out_ways);
}

void
process_relation(OSMPBF__PrimitiveBlock *primitive_block, OSMPBF__Relation *relation, FILE *out_turn_restrictions, FILE *out_boundaries)
{
	int i;
	long long ref=0;
	char rolebuff[1024];

#if 0
	printf("\t<relation id=\"%Ld\"",relation->id);
	process_info(primitive_block, relation->info);
	printf(">\n");
#endif
	osm_add_relation(relation->id);
	for (i = 0 ; i < relation->n_roles_sid ; i++) {
#if 0
		printf("\t\t<member type=\"");
		switch (relation->types[i]) {
		case 0:
			printf("node");
			break;
		case 1:
			printf("way");
			break;
		case 2:
			printf("relation");
			break;
		default:
			printf("unknown");
			break;
		}
#endif
		ref+=relation->memids[i];
		get_string(rolebuff, sizeof(rolebuff), primitive_block, relation->roles_sid[i], 1);
#if 0
		printf("\" ref=\"%Ld\" role=\"%s\"/>\n",ref,rolebuff);
#else
		osm_add_member(relation->types[i]+1,ref,rolebuff);
#endif
	}
	for (i = 0 ; i < relation->n_keys ; i++) 
		process_tag(primitive_block, relation->keys[i], relation->vals[i]);
#if 0
	printf("\t</relation>\n");
#else
	osm_end_relation(out_turn_restrictions, out_boundaries);
#endif
}

void
process_osmdata(OSMPBF__Blob *blob, unsigned char *data, FILE *out_nodes, FILE *out_ways, FILE *out_turn_restrictions, FILE *out_boundaries)
{
	int i,j;
	OSMPBF__PrimitiveBlock *primitive_block;
	primitive_block=osmpbf__primitive_block__unpack(&protobuf_c_system_allocator, blob->raw_size, data);
	for (i = 0 ; i < primitive_block->n_primitivegroup ; i++) {
		OSMPBF__PrimitiveGroup *primitive_group=primitive_block->primitivegroup[i];
		process_dense(primitive_block, primitive_group->dense, out_nodes);
		for (j = 0 ; j < primitive_group->n_ways ; j++)
			process_way(primitive_block, primitive_group->ways[j], out_ways);
		for (j = 0 ; j < primitive_group->n_relations ; j++)
			process_relation(primitive_block, primitive_group->relations[j], out_turn_restrictions, out_boundaries);
#if 0
		printf("Group %p %d %d %d %d\n",primitive_group->dense,primitive_group->n_nodes,primitive_group->n_ways,primitive_group->n_relations,primitive_group->n_changesets);
#endif
	}
	osmpbf__primitive_block__free_unpacked(primitive_block, &protobuf_c_system_allocator);
}


int
map_collect_data_osm_protobuf(FILE *in, FILE *out_ways, FILE *out_nodes, FILE *out_turn_restrictions, FILE *out_boundaries)
{
	OSMPBF__BlobHeader *header;
	OSMPBF__Blob *blob;
	unsigned char *data;
#if 0
	printf("<?xml version='1.0' encoding='UTF-8'?>\n");
	printf("<osm version=\"0.6\" generator=\"pbf2osm\">\n");
#endif
	while ((header=read_header(in))) {
		blob=read_blob(header, in);
		data=uncompress_blob(blob);
		if (!strcmp(header->type,"OSMHeader")) {
			process_osmheader(blob, data);
		} else if (!strcmp(header->type,"OSMData")) {
			process_osmdata(blob, data, out_nodes, out_ways, out_turn_restrictions, out_boundaries);
		} else {
			printf("unknown\n");
			return 0;
		}
		free(data);
		osmpbf__blob__free_unpacked(blob, &protobuf_c_system_allocator);
		osmpbf__blob_header__free_unpacked(header, &protobuf_c_system_allocator);
	}
#if 0
	printf("</osm>\n");
#endif
	return 1;
}
