/*
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2011 Navit Team
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
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <zlib.h>
#include "maptool.h"
#include "debug.h"
#include "linguistics.h"
#include "file.h"
#include "fileformat.pb-c.h"
#include "osmformat.pb-c.h"


static double latlon_scale=10000000.0;

/* Maximum length of a BlobHeader, see
 * http://wiki.openstreetmap.org/wiki/PBF_Format */
#define MAX_HEADER_LENGTH 1024 * 64
/* Maximum length of a Blob */
#define MAX_BLOB_LENGTH 1024 * 1024 * 32

#define SANITY_CHECK_LENGTH(length, max_length) \
	if (length > max_length){                                          \
		fprintf(stderr,"Not a valid protobuf file. "               \
			"Invalid block size in input: %d, max is %d. \n",  \
				length, max_length);                       \
		return NULL;                                                   \
	}

static OSMPBF__BlobHeader *read_header(FILE *f) {
    unsigned char *buffer,lenb[4];
    int len;

    if (fread(lenb, 4, 1, f) != 1)
        return NULL;
    len=(lenb[0] << 24) | (lenb[1] << 16) | (lenb[2] << 8) | lenb[3];
    SANITY_CHECK_LENGTH(len, MAX_HEADER_LENGTH)
    buffer=alloca(len);
    if (fread(buffer, len, 1, f) != 1)
        return NULL;
    return osmpbf__blob_header__unpack(NULL, len, buffer);
}


static OSMPBF__Blob *read_blob(OSMPBF__BlobHeader *header, FILE *f, unsigned char *buffer) {
    int len=header->datasize;
    SANITY_CHECK_LENGTH(len, MAX_BLOB_LENGTH)
    if (fread(buffer, len, 1, f) != 1)
        return NULL;
    return osmpbf__blob__unpack(NULL, len, buffer);
}

static unsigned char *uncompress_blob(OSMPBF__Blob *blob) {
    unsigned char *ret=g_malloc(blob->raw_size);
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
        g_free(ret);
        return NULL;
    }
    zerr = inflate(&strm, Z_NO_FLUSH);
    if (zerr != Z_STREAM_END) {
        g_free(ret);
        return NULL;
    }
    inflateEnd(&strm);
    return ret;
}

static int get_string(char *buffer, int buffer_size, OSMPBF__PrimitiveBlock *primitive_block, int id, int escape) {
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


static void process_osmheader(OSMPBF__Blob *blob, unsigned char *data) {
    OSMPBF__HeaderBlock *header_block;
    header_block=osmpbf__header_block__unpack(NULL, blob->raw_size, data);
    osmpbf__header_block__free_unpacked(header_block, NULL);
}

#if 0

static void process_user(OSMPBF__PrimitiveBlock *primitive_block, int user_sid, int uid, int swap) {
    char userbuff[1024];
    get_string(userbuff, sizeof(userbuff), primitive_block, user_sid, 1);
    if (userbuff[0] && uid != -1) {
        if (swap)
            printf(" uid=\"%d\" user=\"%s\"",uid,userbuff);
        else
            printf(" user=\"%s\" uid=\"%d\"",userbuff,uid);
    }
}

static void process_timestamp(long long timestamp) {
    time_t ts;
    struct tm *tm;
    char tsbuff[1024];
    ts=timestamp;
    tm=gmtime(&ts);
    strftime(tsbuff, sizeof(tsbuff), "%Y-%m-%dT%H:%M:%SZ", tm);
    printf(" timestamp=\"%s\"",tsbuff);
}

#endif

static void process_tag(OSMPBF__PrimitiveBlock *primitive_block, int key, int val) {
    char keybuff[1024];
    char valbuff[1024];
    get_string(keybuff, sizeof(keybuff), primitive_block, key, 0);
    get_string(valbuff, sizeof(valbuff), primitive_block, val, 0);
    osm_add_tag(keybuff, valbuff);
}


static void process_dense(OSMPBF__PrimitiveBlock *primitive_block, OSMPBF__DenseNodes *dense, struct maptool_osm *osm) {
    int i,j=0,has_tags;
    long long id=0,lat=0,lon=0,changeset=0,timestamp=0;
    int user_sid=0,uid=0;

    if (!dense)
        return;

    for (i = 0 ; i < dense->n_id ; i++) {
        id+=dense->id[i];
        lat+=dense->lat[i];
        lon+=dense->lon[i];
        changeset+=dense->denseinfo->changeset[i];
        user_sid+=dense->denseinfo->user_sid[i];
        uid+=dense->denseinfo->uid[i];
        timestamp+=dense->denseinfo->timestamp[i];
        has_tags=dense->keys_vals && dense->keys_vals[j];
        osm_add_node(id, lat/latlon_scale,lon/latlon_scale);
        if (has_tags) {
            while (dense->keys_vals[j]) {
                process_tag(primitive_block, dense->keys_vals[j], dense->keys_vals[j+1]);
                j+=2;
            }
        }
        osm_end_node(osm);
        j++;
    }
}

#if 0
static void process_info(OSMPBF__PrimitiveBlock *primitive_block, OSMPBF__Info *info) {
    printf(" version=\"%d\" changeset=\"%Ld\"",info->version,info->changeset);
    process_user(primitive_block, info->user_sid, info->uid, 1);
    process_timestamp(info->timestamp);
}
#endif

static void process_way(OSMPBF__PrimitiveBlock *primitive_block, OSMPBF__Way *way, struct maptool_osm *osm) {
    int i;
    long long ref=0;

    osm_add_way(way->id);
    for (i = 0 ; i < way->n_refs ; i++) {
        ref+=way->refs[i];
        osm_add_nd(ref);
    }
    for (i = 0 ; i < way->n_keys ; i++)
        process_tag(primitive_block, way->keys[i], way->vals[i]);
    osm_end_way(osm);
}

static void process_relation(OSMPBF__PrimitiveBlock *primitive_block, OSMPBF__Relation *relation,
                             struct maptool_osm *osm) {
    int i;
    long long ref=0;
    char rolebuff[1024];

    osm_add_relation(relation->id);
    for (i = 0 ; i < relation->n_roles_sid ; i++) {
        ref+=relation->memids[i];
        get_string(rolebuff, sizeof(rolebuff), primitive_block, relation->roles_sid[i], 1);
        osm_add_member(relation->types[i]+1,ref,rolebuff);
    }
    for (i = 0 ; i < relation->n_keys ; i++)
        process_tag(primitive_block, relation->keys[i], relation->vals[i]);
    osm_end_relation(osm);
}

static void process_osmdata(OSMPBF__Blob *blob, unsigned char *data, struct maptool_osm *osm) {
    int i,j;
    OSMPBF__PrimitiveBlock *primitive_block;
    primitive_block=osmpbf__primitive_block__unpack(NULL, blob->raw_size, data);
    for (i = 0 ; i < primitive_block->n_primitivegroup ; i++) {
        OSMPBF__PrimitiveGroup *primitive_group=primitive_block->primitivegroup[i];
        process_dense(primitive_block, primitive_group->dense, osm);
        for (j = 0 ; j < primitive_group->n_ways ; j++)
            process_way(primitive_block, primitive_group->ways[j], osm);
        for (j = 0 ; j < primitive_group->n_relations ; j++)
            process_relation(primitive_block, primitive_group->relations[j], osm);
    }
    osmpbf__primitive_block__free_unpacked(primitive_block, NULL);
}


int map_collect_data_osm_protobuf(FILE *in, struct maptool_osm *osm) {
    OSMPBF__BlobHeader *header;
    OSMPBF__Blob *blob;
    unsigned char *data;
    unsigned char *buffer=g_malloc(MAX_BLOB_LENGTH);

    while ((header=read_header(in))) {
        blob=read_blob(header, in, buffer);
        data=uncompress_blob(blob);
        if (!g_strcmp0(header->type,"OSMHeader")) {
            process_osmheader(blob, data);
        } else if (!g_strcmp0(header->type,"OSMData")) {
            process_osmdata(blob, data, osm);
        } else {
            printf("skipping fileblock of unknown type '%s'\n", header->type);
            g_free(buffer);
            return 0;
        }
        g_free(data);
        osmpbf__blob__free_unpacked(blob, NULL);
        osmpbf__blob_header__free_unpacked(header, NULL);
    }
    g_free(buffer);
    return 1;
}
