/**
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
#include "generated-code/fileformat.pb-c.h"
#include "generated-code/osmformat.pb-c.h"

static double latlon_scale=10000000.0;

struct db_config {
    int node_ids_per_file;
    int node_ids_per_blob;
    int node_ids_per_group;
    int way_ids_per_file;
    int way_ids_per_blob;
    int way_ids_per_group;
    int relation_ids_per_file;
    int relation_ids_per_blob;
    int relation_ids_per_group;
} db_config = {
    200000, 30000, 500,
    40000,  1000,  30,
    10000,   500,  20,
};

struct osm_protobufdb_context {
    int current_file, current_block, active_block;
    int in_node, in_way, in_relation;
    OSMPBF__Node n;
    OSMPBF__Way w;
    OSMPBF__Relation r;
    OSMPBF__Info i;
    FILE *f;
    OSMPBF__PrimitiveBlock *pb;
    OSMPBF__PrimitiveGroup *pg;
    GHashTable *string_hash;
    OSMPBF__DenseInfo *di;
    OSMPBF__DenseNodes *dn;
    OSMPBF__StringTable *st;
} context;

static int osm_protobufdb_write_blob(OSMPBF__Blob *blob, FILE *out) {
    unsigned char lenb[4];
    int len,blen;
    unsigned char *buffer;
    OSMPBF__BlobHeader header=OSMPBF__BLOB_HEADER__INIT;



    blen=osmpbf__blob__get_packed_size(blob);
    header.type="OSMData";
    header.datasize=blen;
    len=osmpbf__blob_header__get_packed_size(&header);
    buffer=alloca(len);
    lenb[0]=len>>24;
    lenb[1]=len>>16;
    lenb[2]=len>>8;
    lenb[3]=len;
    osmpbf__blob_header__pack(&header, buffer);
    if (fwrite(lenb, sizeof(lenb), 1, out) != 1)
        return 0;
    if (fwrite(buffer, len, 1, out) != 1)
        return 0;
    buffer=alloca(blen);
    osmpbf__blob__pack(blob, buffer);
    if (fwrite(buffer, blen, 1, out) != 1)
        return 0;
    return 1;
}

#if 0
void dump_block(OSMPBF__PrimitiveBlock *pb) {
    int i,j;
    printf("%d groups\n",pb->n_primitivegroup);
    for (i = 0 ; i < pb->n_primitivegroup ; i++) {
        printf("%d relations\n",pb->primitivegroup[i]->n_relations);
        for (j = 0 ; j < pb->primitivegroup[i]->n_relations ; j++) {
            printf("Info %d\n",pb->primitivegroup[i]->relations[j]->info->version);
        }
    }
}
#endif

static int osm_protobufdb_finish_block(struct osm_protobufdb_context *ctx) {
    OSMPBF__Blob *blob,empty_blob=OSMPBF__BLOB__INIT;
    int len;
    if (!ctx->pb)
        return 0;
    len=osmpbf__primitive_block__get_packed_size(ctx->pb);

    while  (ctx->current_block < ctx->active_block) {
        osm_protobufdb_write_blob(&empty_blob, ctx->f);
        ctx->current_block++;
    }
    blob=g_malloc(sizeof(*blob));
    *blob=empty_blob;
    blob->has_raw=1;
    blob->has_raw_size=1;
    blob->raw.data=g_malloc(len);
    osmpbf__primitive_block__pack(ctx->pb, blob->raw.data);
    blob->raw.len=len;
    blob->raw_size=len;
    osm_protobufdb_write_blob(blob, ctx->f);
    osmpbf__blob__free_unpacked(blob, &protobuf_c_system_allocator);
    osmpbf__primitive_block__free_unpacked(ctx->pb, &protobuf_c_system_allocator);
    ctx->pb=NULL;
    ctx->current_block++;
    return 1;
}

static int osm_protobufdb_start_block(struct osm_protobufdb_context *ctx, int blocknum) {
    OSMPBF__PrimitiveBlock pb=OSMPBF__PRIMITIVE_BLOCK__INIT;
    OSMPBF__StringTable st=OSMPBF__STRING_TABLE__INIT;
    if (ctx->active_block == blocknum)
        return 0;
    osm_protobufdb_finish_block(ctx);
    ctx->active_block=blocknum;
    ctx->pb=g_malloc(sizeof(*ctx->pb));
    *ctx->pb=pb;
    ctx->pb->stringtable=g_malloc(sizeof(*ctx->pb->stringtable));
    *ctx->pb->stringtable=st;
    ctx->st=ctx->pb->stringtable;
    return 1;
}

static int osm_protobufdb_start_group(struct osm_protobufdb_context *ctx, int groupnum) {
    OSMPBF__PrimitiveGroup pg=OSMPBF__PRIMITIVE_GROUP__INIT;
    if (ctx->pb->n_primitivegroup <= groupnum) {
        ctx->pb->primitivegroup=g_realloc(ctx->pb->primitivegroup, (groupnum+1)*sizeof(ctx->pb->primitivegroup[0]));
        while (ctx->pb->n_primitivegroup <= groupnum) {
            ctx->pb->primitivegroup[ctx->pb->n_primitivegroup]=g_malloc(sizeof(*context.pg));
            *ctx->pb->primitivegroup[ctx->pb->n_primitivegroup++]=pg;
        }
        g_hash_table_destroy(ctx->string_hash);
        ctx->string_hash=g_hash_table_new(g_str_hash, g_str_equal);
    }
    ctx->pg=ctx->pb->primitivegroup[groupnum];
    if (!ctx->pg) {
        ctx->pg=g_malloc(sizeof(*context.pg));
        *ctx->pg=pg;
        ctx->pb->primitivegroup[groupnum]=ctx->pg;
    }
    return 1;
}

#if 0
static int osm_protobufdb_start_densenode(struct osm_protobufdb_context *ctx) {
    OSMPBF__DenseInfo di=OSMPBF__DENSE_INFO__INIT;
    OSMPBF__DenseNodes dn=OSMPBF__DENSE_NODES__INIT;

    if (!ctx->pg->dense) {
        ctx->dn=g_malloc(sizeof(*context.dn));
        *ctx->dn=dn;
        ctx->pg->dense=ctx->dn;
    } else
        ctx->dn=ctx->pg->dense;
    if (!ctx->dn->denseinfo) {
        ctx->di=g_malloc(sizeof(*context.di));
        *ctx->di=di;
        ctx->dn->denseinfo=ctx->di;
    } else
        ctx->di=ctx->dn->denseinfo;
    return 1;
}

static void osm_protobufdb_write_primitive_group(OSMPBF__PrimitiveGroup *pg, OSMPBF__PrimitiveBlock *pb) {
    pb->primitivegroup=g_realloc(pb->primitivegroup,(pb->n_primitivegroup+1)*sizeof(OSMPBF__PrimitiveGroup *));
    pb->primitivegroup[pb->n_primitivegroup++]=pg;
}
#endif


#define insert(struct, member, pos) {\
	int n=struct->n_##member; \
	int s=sizeof(struct->member[0]); \
	struct->member=g_realloc(struct->member, (n+1)*s); \
	memmove(&struct->member[n+1], &struct->member[n], (pos-n)*s); \
	memset(&struct->member[n], 0, s); \
	struct->n_##member++;\
}

#if 0
static int osm_protobufdb_insert_densenode(long long id, OSMPBF__Node *offset, OSMPBF__Info *offseti,
        OSMPBF__DenseNodes *dn) {
    int i,l,p;
    memset(offset, 0, sizeof(*offset));
    offseti->timestamp=0;
    offseti->changeset=0;
    offseti->user_sid=0;
    offseti->uid=0;
    l=dn->n_id;
    for (i = 0 ; i < l ; i++) {
        offset->id+=dn->id[i];
        offset->lat+=dn->lat[i];
        offset->lon+=dn->lon[i];
        offseti->timestamp+=dn->denseinfo->timestamp[i];
        offseti->changeset+=dn->denseinfo->changeset[i];
        offseti->user_sid+=dn->denseinfo->user_sid[i];
        offseti->uid+=dn->denseinfo->uid[i];
    }
    p=l;
    insert(dn, id, p);
    insert(dn, lat, p);
    insert(dn, lon, p);
    insert(dn->denseinfo, version, p);
    insert(dn->denseinfo, timestamp, p);
    insert(dn->denseinfo, changeset, p);
    insert(dn->denseinfo, user_sid, p);
    insert(dn->denseinfo, uid, p);
    return p;
}

static void osm_protobufdb_modify_densenode(OSMPBF__Node *node, OSMPBF__Info *info, OSMPBF__Node *offset,
        OSMPBF__Info *offseti,
        int pos, OSMPBF__DenseNodes *dn) {
    int i;
    if (pos+1 < dn->n_id) {
        dn->id[pos+1]+=dn->id[pos]-node->id;
        dn->lat[pos+1]+=dn->lat[pos]-node->lat;
        dn->lon[pos+1]+=dn->lon[pos]-node->lon;
        dn->denseinfo->timestamp[pos+1]+=dn->denseinfo->timestamp[pos]-info->timestamp;
        dn->denseinfo->changeset[pos+1]+=dn->denseinfo->changeset[pos]-info->changeset;
        dn->denseinfo->user_sid[pos+1]+=dn->denseinfo->user_sid[pos]-info->user_sid;
        dn->denseinfo->uid[pos+1]+=dn->denseinfo->uid[pos]-info->uid;
    }
    dn->id[pos]=node->id-offset->id;
    dn->lat[pos]=node->lat-offset->lat;
    dn->lon[pos]=node->lon-offset->lon;
    dn->keys_vals=g_realloc(dn->keys_vals, (dn->n_keys_vals+node->n_keys+node->n_vals+1)*sizeof(dn->keys_vals[0]));
    for (i = 0 ; i < node->n_keys ; i++) {
        dn->keys_vals[dn->n_keys_vals++]=node->keys[i];
        dn->keys_vals[dn->n_keys_vals++]=node->vals[i];
    }
    dn->keys_vals[dn->n_keys_vals++]=0;
    dn->denseinfo->version[pos]=info->version;
    dn->denseinfo->timestamp[pos]=info->timestamp-offseti->timestamp;
    dn->denseinfo->changeset[pos]=info->changeset-offseti->changeset;
    dn->denseinfo->user_sid[pos]=info->user_sid-offseti->user_sid;
    dn->denseinfo->uid[pos]=info->uid-offseti->uid;
}
#endif

static int osm_protobufdb_insert_node(long long id, OSMPBF__PrimitiveGroup *pg) {
    int l,p;
    OSMPBF__Node node=OSMPBF__NODE__INIT;
    l=pg->n_nodes;
    p=l;
    insert(pg, nodes, p);
    pg->nodes[p]=g_malloc(sizeof(*pg->nodes[0]));
    *pg->nodes[p]=node;
    return p;
}

static void osm_protobufdb_modify_node(OSMPBF__Node *node, OSMPBF__Info *info, int pos, OSMPBF__PrimitiveGroup *pg) {
    OSMPBF__Node *n=pg->nodes[pos];
    OSMPBF__Info *old_info;

    g_free(n->keys);
    g_free(n->vals);
    old_info=n->info;
    *n=*node;
    if (!info) {
        if (old_info)
            osmpbf__info__free_unpacked(old_info, &protobuf_c_system_allocator);
        n->info=NULL;
    } else {
        if (old_info)
            n->info=old_info;
        else
            n->info=g_malloc(sizeof(*info));
        *n->info=*info;
    }

}

static int osm_protobufdb_insert_way(long long id, OSMPBF__PrimitiveGroup *pg) {
    int l,p;
    OSMPBF__Way way=OSMPBF__WAY__INIT;
    l=pg->n_ways;
    p=l;
    insert(pg, ways, p);
    pg->ways[p]=g_malloc(sizeof(*pg->ways[0]));
    *pg->ways[p]=way;
    return p;
}

static void osm_protobufdb_modify_way(OSMPBF__Way *way, OSMPBF__Info *info, int pos, OSMPBF__PrimitiveGroup *pg) {
    OSMPBF__Way *w=pg->ways[pos];
    OSMPBF__Info *old_info;
    int i;
    long long ref=0;

    g_free(w->keys);
    g_free(w->vals);
    g_free(w->refs);
    old_info=w->info;
    *w=*way;
    for (i = 0 ; i < w->n_refs ; i++) {
        w->refs[i]-=ref;
        ref+=w->refs[i];
    }
    if (!info) {
        if (old_info)
            osmpbf__info__free_unpacked(old_info, &protobuf_c_system_allocator);
        w->info=NULL;
    } else {
        if (old_info)
            w->info=old_info;
        else
            w->info=g_malloc(sizeof(*info));
        *w->info=*info;
    }

}

static int osm_protobufdb_insert_relation(long long id, OSMPBF__PrimitiveGroup *pg) {
    int l,p;
    OSMPBF__Relation relation=OSMPBF__RELATION__INIT;
    l=pg->n_relations;
    p=l;
    insert(pg, relations, p);
    pg->relations[p]=g_malloc(sizeof(*pg->relations[0]));
    *pg->relations[p]=relation;
    return p;
}

static void osm_protobufdb_modify_relation(OSMPBF__Relation *relation, OSMPBF__Info *info, int pos,
        OSMPBF__PrimitiveGroup *pg) {
    OSMPBF__Relation *r=pg->relations[pos];
    OSMPBF__Info *old_info;
    int i;
    long long ref=0;

    g_free(r->keys);
    g_free(r->vals);
    g_free(r->roles_sid);
    g_free(r->memids);
    g_free(r->types);
    old_info=r->info;
    *r=*relation;
    for (i = 0 ; i < r->n_memids ; i++) {
        r->memids[i]-=ref;
        ref+=r->memids[i];
    }
    if (!info) {
        if (old_info)
            osmpbf__info__free_unpacked(old_info, &protobuf_c_system_allocator);
        r->info=NULL;
    } else {
        if (old_info)
            r->info=old_info;
        else
            r->info=g_malloc(sizeof(*info));
        *r->info=*info;
    }

}

static int osm_protobufdb_string(struct osm_protobufdb_context *ctx, char *str) {
    char *strd;
    OSMPBF__StringTable *st=ctx->st;

    gpointer value;
    assert(ctx->string_hash != NULL);
    if (g_hash_table_lookup_extended(ctx->string_hash, str, NULL, &value)) {
        return (long)value;
    }
    if (!st->n_s) {
        st->n_s++;
    }
    strd=g_strdup(str);
    st->s=g_realloc(st->s, sizeof(st->s[0])*(st->n_s+1));
    if (st->n_s == 1) {
        st->s[0].data=NULL;
        st->s[0].len=0;
    }
    st->s[st->n_s].data=(unsigned char *)strd;
    st->s[st->n_s].len=strlen(strd);
    g_hash_table_insert(ctx->string_hash, strd, (gpointer)st->n_s);
    return st->n_s++;
}


static int osm_protobufdb_finish_file(struct osm_protobufdb_context *ctx) {
    osm_protobufdb_finish_block(ctx);
    if (ctx->f) {
        fclose(ctx->f);
        ctx->f=NULL;
    }
    ctx->current_file=-1;
    return 1;
}


static int osm_protobufdb_start_file(struct osm_protobufdb_context *ctx, int type, int num) {
    char name[1024];
    if (ctx->current_file == num)
        return 0;
    osm_protobufdb_finish_file(ctx);
    sprintf(name,"tst/%d-%08d",type,num);
    ctx->f=fopen(name,"w");
    ctx->current_file=num;
    ctx->current_block=0;
    ctx->active_block=-1;
    return 1;
}

static void test(void) {
    context.current_file=-1;
}

static void finish(void) {
    osm_protobufdb_finish_file(&context);
}

static long long osm_protobufdb_timestamp(char *str) {
    struct tm tm;
    int res=sscanf(str,"%d-%d-%dT%d:%d:%dZ",&tm.tm_year,&tm.tm_mon,&tm.tm_mday,&tm.tm_hour,&tm.tm_min,&tm.tm_sec);
    if (res != 6)
        return 0;
    tm.tm_year-=1900;
    tm.tm_mon-=1;
#if defined(HAVE_API_WIN32_BASE) || defined(ANDROID)
    return 0;
#else
    return timegm(&tm);
#endif
}

static void osm_protobufdb_parse_info(struct osm_protobufdb_context *ctx, char *str) {
    char version[1024];
    char changeset[1024];
    char user[1024];
    char uid[1024];
    char timestamp[1024];

    OSMPBF__Info *i=&ctx->i, ii=OSMPBF__INFO__INIT;
    *i=ii;
    if (osm_xml_get_attribute(str, "version", version, sizeof(version))) {
        i->version=atoi(version);
        i->has_version=1;
    }
    if (osm_xml_get_attribute(str, "changeset", changeset, sizeof(changeset))) {
        i->changeset=atoll(changeset);
        i->has_changeset=1;
    }
    if (osm_xml_get_attribute(str, "user", user, sizeof(user))) {
        osm_xml_decode_entities(user);
        i->user_sid=osm_protobufdb_string(ctx, user);
        i->has_user_sid=1;
    }
    if (osm_xml_get_attribute(str, "uid", uid, sizeof(uid))) {
        i->uid=atoi(uid);
        i->has_uid=1;
    }
    if (osm_xml_get_attribute(str, "timestamp", timestamp, sizeof(timestamp))) {
        i->timestamp=osm_protobufdb_timestamp(timestamp);
        i->has_timestamp=1;
    }
}

static int osm_protobufdb_parse_node(struct osm_protobufdb_context *ctx, char *str) {
    char id[1024];
    char lat[1024];
    char lon[1024];
    OSMPBF__Node *n=&ctx->n, ni=OSMPBF__NODE__INIT;

    *n=ni;
    if (!osm_xml_get_attribute(str, "id", id, sizeof(id)))
        return 0;
    if (!osm_xml_get_attribute(str, "lat", lat, sizeof(lat)))
        return 0;
    if (!osm_xml_get_attribute(str, "lon", lon, sizeof(lon)))
        return 0;
    n->id=atoll(id);
    n->lat=atof(lat)*latlon_scale+0.5;
    n->lon=atof(lon)*latlon_scale+0.5;
    int file=n->id/db_config.node_ids_per_file;
    int fileo=n->id%db_config.node_ids_per_file;
    int blob=fileo/db_config.node_ids_per_blob;
    int blobo=fileo%db_config.node_ids_per_blob;
    int group=blobo/db_config.node_ids_per_group;

    osm_protobufdb_start_file(ctx, 1, file);
    osm_protobufdb_start_block(ctx, blob);
    osm_protobufdb_start_group(ctx, group);
    osm_protobufdb_parse_info(ctx, str);
    ctx->in_node=1;
    return 1;
}

static int osm_protobufdb_end_node(struct osm_protobufdb_context *ctx) {
    int p;
    p=osm_protobufdb_insert_node(ctx->n.id, ctx->pg);
    osm_protobufdb_modify_node(&ctx->n, &ctx->i, p, ctx->pg);
    ctx->in_node=0;
    return 1;
}


static int osm_protobufdb_parse_way(struct osm_protobufdb_context *ctx, char *str) {
    char id[1024];
    OSMPBF__Way *w=&ctx->w, wi=OSMPBF__WAY__INIT;

    *w=wi;
    if (!osm_xml_get_attribute(str, "id", id, sizeof(id)))
        return 0;
    w->id=atoll(id);
    int file=w->id/db_config.way_ids_per_file;
    int fileo=w->id%db_config.way_ids_per_file;
    int blob=fileo/db_config.way_ids_per_blob;
    int blobo=fileo%db_config.way_ids_per_blob;
    int group=blobo/db_config.way_ids_per_group;

    osm_protobufdb_start_file(ctx, 2, file);
    osm_protobufdb_start_block(ctx, blob);
    osm_protobufdb_start_group(ctx, group);
    osm_protobufdb_parse_info(ctx, str);
    ctx->in_way=1;
    return 1;
}

static int osm_protobufdb_end_way(struct osm_protobufdb_context *ctx) {
    int p;
    p=osm_protobufdb_insert_way(ctx->w.id, ctx->pg);
    osm_protobufdb_modify_way(&ctx->w, &ctx->i, p, ctx->pg);
    ctx->in_way=0;
    return 1;
}

static int osm_protobufdb_parse_relation(struct osm_protobufdb_context *ctx, char *str) {
    char id[1024];
    OSMPBF__Relation *r=&ctx->r, ri=OSMPBF__RELATION__INIT;

    *r=ri;
    if (!osm_xml_get_attribute(str, "id", id, sizeof(id)))
        return 0;
    r->id=atoll(id);
    int file=r->id/db_config.relation_ids_per_file;
    int fileo=r->id%db_config.relation_ids_per_file;
    int blob=fileo/db_config.relation_ids_per_blob;
    int blobo=fileo%db_config.relation_ids_per_blob;
    int group=blobo/db_config.relation_ids_per_group;

    osm_protobufdb_start_file(ctx, 3, file);
    osm_protobufdb_start_block(ctx, blob);
    osm_protobufdb_start_group(ctx, group);
    osm_protobufdb_parse_info(ctx, str);
    ctx->in_relation=1;
    return 1;
}

static int osm_protobufdb_end_relation(struct osm_protobufdb_context *ctx) {
    int p;
    p=osm_protobufdb_insert_relation(ctx->r.id, ctx->pg);
    osm_protobufdb_modify_relation(&ctx->r, &ctx->i, p, ctx->pg);
    ctx->in_node=0;
    return 1;
}

static int osm_protobufdb_parse_tag(struct osm_protobufdb_context *ctx, char *str) {
    OSMPBF__Node *n=&ctx->n;
    OSMPBF__Way *w=&ctx->w;
    OSMPBF__Relation *r=&ctx->r;
    char k_buffer[BUFFER_SIZE];
    char v_buffer[BUFFER_SIZE];
    if (!osm_xml_get_attribute(str, "k", k_buffer, BUFFER_SIZE))
        return 0;
    if (!osm_xml_get_attribute(str, "v", v_buffer, BUFFER_SIZE))
        return 0;
    osm_xml_decode_entities(v_buffer);
    if (ctx->in_node) {
        n->keys=g_realloc(n->keys, (n->n_keys+1)*sizeof(n->keys[0]));
        n->vals=g_realloc(n->vals, (n->n_vals+1)*sizeof(n->vals[0]));
        n->keys[n->n_keys++]=osm_protobufdb_string(ctx, k_buffer);
        n->vals[n->n_vals++]=osm_protobufdb_string(ctx, v_buffer);
    }
    if (ctx->in_way) {
        w->keys=g_realloc(w->keys, (w->n_keys+1)*sizeof(w->keys[0]));
        w->vals=g_realloc(w->vals, (w->n_vals+1)*sizeof(w->vals[0]));
        w->keys[w->n_keys++]=osm_protobufdb_string(ctx, k_buffer);
        w->vals[w->n_vals++]=osm_protobufdb_string(ctx, v_buffer);
    }
    if (ctx->in_relation) {
        r->keys=g_realloc(r->keys, (r->n_keys+1)*sizeof(r->keys[0]));
        r->vals=g_realloc(r->vals, (r->n_vals+1)*sizeof(r->vals[0]));
        r->keys[r->n_keys++]=osm_protobufdb_string(ctx, k_buffer);
        r->vals[r->n_vals++]=osm_protobufdb_string(ctx, v_buffer);
    }
    return 1;
}

static int osm_protobufdb_parse_nd(struct osm_protobufdb_context *ctx, char *str) {
    OSMPBF__Way *w=&ctx->w;
    char ref_buffer[BUFFER_SIZE];
    if (!osm_xml_get_attribute(str, "ref", ref_buffer, BUFFER_SIZE))
        return 0;
    if (ctx->in_way) {
        w->refs=g_realloc(w->refs, (w->n_refs+1)*sizeof(w->refs[0]));
        w->refs[w->n_refs++]=atoll(ref_buffer);
    }
    return 1;
}

static int osm_protobufdb_parse_member(struct osm_protobufdb_context *ctx, char *str) {
    OSMPBF__Relation *r=&ctx->r;
    char type_buffer[BUFFER_SIZE];
    char ref_buffer[BUFFER_SIZE];
    char role_buffer[BUFFER_SIZE];
    int type=0;
    if (!osm_xml_get_attribute(str, "type", type_buffer, BUFFER_SIZE))
        return 0;
    if (!osm_xml_get_attribute(str, "ref", ref_buffer, BUFFER_SIZE))
        return 0;
    if (!osm_xml_get_attribute(str, "role", role_buffer, BUFFER_SIZE))
        return 0;
    if (!strcmp(type_buffer,"node"))
        type=0;
    else if (!strcmp(type_buffer,"way"))
        type=1;
    else if (!strcmp(type_buffer,"relation"))
        type=2;
    if (ctx->in_relation) {
        r->roles_sid=g_realloc(r->roles_sid, (r->n_roles_sid+1)*sizeof(r->roles_sid[0]));
        r->roles_sid[r->n_roles_sid++]=osm_protobufdb_string(ctx, role_buffer);
        r->memids=g_realloc(r->memids, (r->n_memids+1)*sizeof(r->memids[0]));
        r->memids[r->n_memids++]=atoll(ref_buffer);
        r->types=g_realloc(r->types, (r->n_types+1)*sizeof(r->types[0]));
        r->types[r->n_types++]=type;
    }
    return 1;
}



int osm_protobufdb_load(FILE *in, char *dir) {
    int size=BUFFER_SIZE;
    char buffer[size];
    char *p;
    sig_alrm(0);
    test();
    while (fgets(buffer, size, in)) {
        int closed=strstr(buffer,"/>")?1:0;
        p=strchr(buffer,'<');
        if (! p) {
            fprintf(stderr,"WARNING: wrong line %s\n", buffer);
            continue;
        }
        if (!strncmp(p, "<?xml ",6)) {
        } else if (!strncmp(p, "<osm ",5)) {
        } else if (!strncmp(p, "<bound ",7)) {
        } else if (!strncmp(p, "<node ",6)) {
            if (!osm_protobufdb_parse_node(&context, p))
                fprintf(stderr,"WARNING: failed to parse %s\n", buffer);
            if (closed)
                osm_protobufdb_end_node(&context);
            processed_nodes++;
        } else if (!strncmp(p, "<tag ",5)) {
            if (!osm_protobufdb_parse_tag(&context, p))
                fprintf(stderr,"WARNING: failed to parse %s\n", buffer);
        } else if (!strncmp(p, "<way ",5)) {
            if (!osm_protobufdb_parse_way(&context, p))
                fprintf(stderr,"WARNING: failed to parse %s\n", buffer);
            if (closed)
                osm_protobufdb_end_way(&context);
            processed_ways++;
        } else if (!strncmp(p, "<nd ",4)) {
            if (!osm_protobufdb_parse_nd(&context, p))
                fprintf(stderr,"WARNING: failed to parse %s\n", buffer);
        } else if (!strncmp(p, "<relation ",10)) {
            if (!osm_protobufdb_parse_relation(&context, p))
                fprintf(stderr,"WARNING: failed to parse %s\n", buffer);
            processed_relations++;
        } else if (!strncmp(p, "<member ",8)) {
            if (!osm_protobufdb_parse_member(&context, p))
                fprintf(stderr,"WARNING: failed to parse %s\n", buffer);
        } else if (!strncmp(p, "</node>",7)) {
            osm_protobufdb_end_node(&context);
        } else if (!strncmp(p, "</way>",6)) {
            osm_protobufdb_end_way(&context);
        } else if (!strncmp(p, "</relation>",11)) {
            osm_protobufdb_end_relation(&context);
        } else if (!strncmp(p, "</osm>",6)) {
        } else {
            fprintf(stderr,"WARNING: unknown tag in %s\n", buffer);
        }
    }
    finish();
    return 1;
}
