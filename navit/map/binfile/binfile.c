/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
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

#include <stdlib.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "config.h"
#include "debug.h"
#include "plugin.h"
#include "projection.h"
#include "item.h"
#include "map.h"
#include "maptype.h"
#include "attr.h"
#include "coord.h"
#include "transform.h"
#include "file.h"
#include "zipfile.h"
#include "linguistics.h"
#include "endianess.h"
#include "callback.h"
#include "types.h"
#include "geom.h"

static int map_id;


/**
 * @brief A map tile, a rectangular region of the world.
 *
 * Represents a "map tile", a rectangular region of the world. The
 * binfile format divides the world into tiles of different sizes for
 * easy handling.
 * A binfile is a ZIP archive; each member file (with the exception of
 * index files) represents one tile. The data from a tile file is read
 * into memory and used directly as the tile data of this struct.
 * <p>
 * See the Navit wiki for details on the binfile format:
 *
 * http://wiki.navit-project.org/index.php/Navit%27s_binary_map_driver
 * <p>
 * Note that this tile struct also maintains pointers to several current positions
 * inside the tile. These are not part of the actual tile data, but are
 * used for working with the data.
 */
struct tile {
    int *start;             //!< Memory address of the buffer containing the tile data (the actual map data).
    int *end;               //!< First memory address not belonging to the tile data.
    /**< Thus tile->end - tile->start represents the size of the tile data
     * in multiples of 4 Bytes.
     */
    int *pos;               //!< Pointer to current position (start of current item) inside the tile data.
    int *pos_coord_start;   //!< Pointer to the first element inside the current item that is a coordinate.
    /**< That is the first position after the header of an
     * item. The header holds 3 entries each 32bit wide integers:
     * header[0] holds the size of the whole item (excluding this size field)
     * header[1] holds the type of the item
     * header[2] holds the size of the coordinates in the tile
     */
    int *pos_coord;         //!< Current position in the coordinates region of the current item.
    int *pos_attr_start;    //!< Pointer to the first attr data structure of the current item.
    int *pos_attr;          //!< Current position in the attr region of the current item.
    int *pos_next;          //!< Pointer to the next item (the item which follows the "current item" as indicated by *pos).
    struct file *fi;        //!< The file from which this tile was loaded.
    int zipfile_num;
    int mode;
};


struct map_download {
    int state;
    struct map_priv *m;
    struct map_rect_priv *mr;
    struct file *http,*file;
    int zipfile,toffset,tlength,progress,read,dl_size;
    long long offset,start_offset,cd1offset,size;
    struct zip64_eoc *zip64_eoc;
    struct zip64_eocl *zip64_eocl;
    struct zip_eoc *zip_eoc;
    struct zip_cd *cd_copy,*cd;
};

/**
 * @brief Represents the map from a single binfile.
 *
 */
struct map_priv {
    int id;
    char *filename;              //!< Filename of the binfile.
    char *cachedir;
    struct file *fi,*http;
    struct file **fis;
    struct zip_cd *index_cd;
    int index_offset;
    int cde_size;
    struct zip_eoc *eoc;
    struct zip64_eoc *eoc64;
    int zip_members;
    unsigned char *search_data;
    int search_offset;
    int search_size;
    int version;
    int check_version;
    int map_version;
    GHashTable *changes;
    char *map_release;
    int flags;
    char *url;
    int update_available;
    char *progress;
    struct callback_list *cbl;
    struct map_download *download;
    int redirect;
    long download_enabled;
    int last_searched_town_id_hi;
    int last_searched_town_id_lo;
};

struct map_rect_priv {
    int *start;
    int *end;
    enum attr_type attr_last;
    int label;
    int *label_attr[5];
    struct map_selection *sel;
    struct map_priv *m;
    struct item item;
    int tile_depth;
    struct tile tiles[8];
    struct tile *t;
    int country_id;
    char *url;
    struct attr attrs[8];
    int status;
    struct map_search_priv *msp;
#ifdef DEBUG_SIZE
    int size;
#endif
};

/**
 * @brief Represents a search on a map.
 * This struct represents a search on a map; it is created
 * when starting a search, and is used for retrieving results.
 */
struct map_search_priv {
    struct map_priv *map; /**< Map to search in. */
    struct map_rect_priv *mr; /**< Map rectangle to search inside. */
    struct map_rect_priv *mr_item;
    struct item *item;
    struct attr search; /**< Attribute specifying what to search for. */
    struct map_selection ms;
    GList *boundaries;
    int partial; /**< Find partial matches? */
    int mode;
    struct coord_rect rect_new;
    char *parent_name;
    GHashTable *search_results;
};


static void push_tile(struct map_rect_priv *mr, struct tile *t, int offset, int length);
static void setup_pos(struct map_rect_priv *mr);
static void map_binfile_close(struct map_priv *m);
static int map_binfile_open(struct map_priv *m);
static void map_binfile_destroy(struct map_priv *m);

static void lfh_to_cpu(struct zip_lfh *lfh) {
    dbg_assert(lfh != NULL);
    if (lfh->ziplocsig != zip_lfh_sig) {
        lfh->ziplocsig = le32_to_cpu(lfh->ziplocsig);
        lfh->zipver    = le16_to_cpu(lfh->zipver);
        lfh->zipgenfld = le16_to_cpu(lfh->zipgenfld);
        lfh->zipmthd   = le16_to_cpu(lfh->zipmthd);
        lfh->ziptime   = le16_to_cpu(lfh->ziptime);
        lfh->zipdate   = le16_to_cpu(lfh->zipdate);
        lfh->zipcrc    = le32_to_cpu(lfh->zipcrc);
        lfh->zipsize   = le32_to_cpu(lfh->zipsize);
        lfh->zipuncmp  = le32_to_cpu(lfh->zipuncmp);
        lfh->zipfnln   = le16_to_cpu(lfh->zipfnln);
        lfh->zipxtraln = le16_to_cpu(lfh->zipxtraln);
    }
}

static void cd_to_cpu(struct zip_cd *zcd) {
    dbg_assert(zcd != NULL);
    if (zcd->zipcensig != zip_cd_sig) {
        zcd->zipcensig = le32_to_cpu(zcd->zipcensig);
        zcd->zipccrc   = le32_to_cpu(zcd->zipccrc);
        zcd->zipcsiz   = le32_to_cpu(zcd->zipcsiz);
        zcd->zipcunc   = le32_to_cpu(zcd->zipcunc);
        zcd->zipcfnl   = le16_to_cpu(zcd->zipcfnl);
        zcd->zipcxtl   = le16_to_cpu(zcd->zipcxtl);
        zcd->zipccml   = le16_to_cpu(zcd->zipccml);
        zcd->zipdsk    = le16_to_cpu(zcd->zipdsk);
        zcd->zipint    = le16_to_cpu(zcd->zipint);
        zcd->zipext    = le32_to_cpu(zcd->zipext);
        zcd->zipofst   = le32_to_cpu(zcd->zipofst);
    }
}

static void eoc_to_cpu(struct zip_eoc *eoc) {
    dbg_assert(eoc != NULL);
    if (eoc->zipesig != zip_eoc_sig) {
        eoc->zipesig   = le32_to_cpu(eoc->zipesig);
        eoc->zipedsk   = le16_to_cpu(eoc->zipedsk);
        eoc->zipecen   = le16_to_cpu(eoc->zipecen);
        eoc->zipenum   = le16_to_cpu(eoc->zipenum);
        eoc->zipecenn  = le16_to_cpu(eoc->zipecenn);
        eoc->zipecsz   = le32_to_cpu(eoc->zipecsz);
        eoc->zipeofst  = le32_to_cpu(eoc->zipeofst);
        eoc->zipecoml  = le16_to_cpu(eoc->zipecoml);
    }
}

static void binfile_check_version(struct map_priv *m);

static struct zip_eoc *binfile_read_eoc(struct file *fi) {
    struct zip_eoc *eoc;
    eoc=(struct zip_eoc *)file_data_read(fi,fi->size-sizeof(struct zip_eoc), sizeof(struct zip_eoc));
    if (eoc) {
        eoc_to_cpu(eoc);
        dbg(lvl_debug,"sig 0x%x", eoc->zipesig);
        if (eoc->zipesig != zip_eoc_sig) {
            dbg(lvl_error,"map file %s: eoc signature check failed: 0x%x vs 0x%x", fi->name, eoc->zipesig,zip_eoc_sig);
            file_data_free(fi,(unsigned char *)eoc);
            eoc=NULL;
        }
    }
    return eoc;
}

static struct zip64_eoc *binfile_read_eoc64(struct file *fi) {
    struct zip64_eocl *eocl;
    struct zip64_eoc *eoc;
    eocl=(struct zip64_eocl *)file_data_read(fi,fi->size-sizeof(struct zip_eoc)-sizeof(struct zip64_eocl),
            sizeof(struct zip64_eocl));
    if (!eocl)
        return NULL;
    dbg(lvl_debug,"sig 0x%x", eocl->zip64lsig);
    if (eocl->zip64lsig != zip64_eocl_sig) {
        file_data_free(fi,(unsigned char *)eocl);
        dbg(lvl_warning,"map file %s: eocl wrong", fi->name);
        return NULL;
    }
    eoc=(struct zip64_eoc *)file_data_read(fi,eocl->zip64lofst, sizeof(struct zip64_eoc));
    if (eoc) {
        if (eoc->zip64esig != zip64_eoc_sig) {
            file_data_free(fi,(unsigned char *)eoc);
            dbg(lvl_warning,"map file %s: eoc wrong", fi->name);
            eoc=NULL;
        }
        dbg(lvl_debug,"eoc64 ok 0x"LONGLONG_HEX_FMT " 0x"LONGLONG_HEX_FMT "",eoc->zip64eofst,eoc->zip64ecsz);
    }
    file_data_free(fi,(unsigned char *)eocl);
    return eoc;
}

static int binfile_cd_extra(struct zip_cd *cd) {
    return cd->zipcfnl+cd->zipcxtl;
}

static struct zip_cd *binfile_read_cd(struct map_priv *m, int offset, int len) {
    struct zip_cd *cd;
    long long cdoffset=m->eoc64?m->eoc64->zip64eofst:m->eoc->zipeofst;
    if (len == -1) {
        cd=(struct zip_cd *)file_data_read(m->fi,cdoffset+offset, sizeof(*cd));
        cd_to_cpu(cd);
        len=binfile_cd_extra(cd);
        file_data_free(m->fi,(unsigned char *)cd);
    }
    cd=(struct zip_cd *)file_data_read(m->fi,cdoffset+offset, sizeof(*cd)+len);
    if (cd) {
        dbg(lvl_debug,"cd at "LONGLONG_FMT" %zu bytes",cdoffset+offset, sizeof(*cd)+len);
        cd_to_cpu(cd);
        dbg(lvl_debug,"sig 0x%x", cd->zipcensig);
        if (cd->zipcensig != zip_cd_sig) {
            file_data_free(m->fi,(unsigned char *)cd);
            cd=NULL;
        }
    }
    return cd;
}

/**
 * @brief Get the ZIP64 extra field data corresponding to a zip central
 * directory header.
 *
 * @param cd pointer to zip central directory structure
 * @return pointer to ZIP64 extra field, or NULL if not available
 */
static struct zip_cd_ext *binfile_cd_ext(struct zip_cd *cd) {
    struct zip_cd_ext *ext;
    if (cd->zipofst != zip_size_64bit_placeholder)
        return NULL;
    if (cd->zipcxtl != sizeof(*ext))
        return NULL;
    ext=(struct zip_cd_ext *)((unsigned char *)cd+sizeof(*cd)+cd->zipcfnl);
    if (ext->tag != zip_extra_header_id_zip64 || ext->size != 8)
        return NULL;
    return ext;
}

/**
 * @param cd pointer to zip central directory structure
 * @return Offset of local file header in zip file.
 * Will use ZIP64 data if present.
 */
static long long binfile_cd_offset(struct zip_cd *cd) {
    struct zip_cd_ext *ext=binfile_cd_ext(cd);
    if (ext)
        return ext->zipofst;
    else
        return cd->zipofst;
}

static struct zip_lfh *binfile_read_lfh(struct file *fi, long long offset) {
    struct zip_lfh *lfh;

    lfh=(struct zip_lfh *)(file_data_read(fi,offset,sizeof(struct zip_lfh)));
    if (lfh) {
        lfh_to_cpu(lfh);
        if (lfh->ziplocsig != zip_lfh_sig) {
            file_data_free(fi,(unsigned char *)lfh);
            lfh=NULL;
        }
    }
    return lfh;
}

static unsigned char *binfile_read_content(struct map_priv *m, struct file *fi, long long offset, struct zip_lfh *lfh) {
    unsigned char *ret=NULL;

    offset+=sizeof(struct zip_lfh)+lfh->zipfnln;
    switch (lfh->zipmthd) {
    case 0:
        offset+=lfh->zipxtraln;
        ret=file_data_read(fi,offset, lfh->zipuncmp);
        break;
    case 8:
        offset+=lfh->zipxtraln;
        ret=file_data_read_compressed(fi,offset, lfh->zipsize, lfh->zipuncmp);
        break;
    default:
        dbg(lvl_error,"map file %s: unknown compression method %d", fi->name, lfh->zipmthd);
    }
    return ret;
}

static int binfile_search_cd(struct map_priv *m, int offset, char *name, int partial, int skip) {
    int size=4096;
    int end=m->eoc64?m->eoc64->zip64ecsz:m->eoc->zipecsz;
    int len=strlen(name);
    long long cdoffset=m->eoc64?m->eoc64->zip64eofst:m->eoc->zipeofst;
    struct zip_cd *cd;
#if 0
    dbg(lvl_debug,"end=%d",end);
#endif
    while (offset < end) {
        cd=(struct zip_cd *)(m->search_data+offset-m->search_offset);
        if (! m->search_data ||
                m->search_offset > offset ||
                offset-m->search_offset+sizeof(*cd) > m->search_size ||
                offset-m->search_offset+sizeof(*cd)+cd->zipcfnl+cd->zipcxtl > m->search_size
           ) {
#if 0
            dbg(lvl_debug,"reload %p %d %d", m->search_data, m->search_offset, offset);
#endif
            if (m->search_data)
                file_data_free(m->fi,m->search_data);
            m->search_offset=offset;
            m->search_size=end-offset;
            if (m->search_size > size)
                m->search_size=size;
            m->search_data=file_data_read(m->fi,cdoffset+m->search_offset,m->search_size);
            cd=(struct zip_cd *)m->search_data;
        }
#if 0
        dbg(lvl_debug,"offset=%d search_offset=%d search_size=%d search_data=%p cd=%p", offset, m->search_offset,
            m->search_size, m->search_data, cd);
        dbg(lvl_debug,"offset=%d fn='%s'",offset,cd->zipcfn);
#endif
        if (!skip &&
                (partial || cd->zipcfnl == len) &&
                !strncmp(cd->zipcfn, name, len))
            return offset;
        skip=0;
        offset+=sizeof(*cd)+cd->zipcfnl+cd->zipcxtl+cd->zipccml;
        ;
    }
    return -1;
}

static void map_destroy_binfile(struct map_priv *m) {
    dbg(lvl_debug,"map_destroy_binfile");
    if (m->fi)
        map_binfile_close(m);
    map_binfile_destroy(m);
}

static void binfile_coord_rewind(void *priv_data) {
    struct map_rect_priv *mr=priv_data;
    struct tile *t=mr->t;
    t->pos_coord=t->pos_coord_start;
}

static inline int binfile_coord_left(void *priv_data) {
    struct map_rect_priv *mr=priv_data;
    struct tile *t=mr->t;
    return (t->pos_attr_start-t->pos_coord)/2;
}

static int binfile_coord_get(void *priv_data, struct coord *c, int count) {
    struct map_rect_priv *mr=priv_data;
    struct tile *t=mr->t;
    int max,ret=0;
    max=binfile_coord_left(priv_data);
    if (count > max)
        count=max;
#if __BYTE_ORDER == __LITTLE_ENDIAN
    memcpy(c, t->pos_coord, count*sizeof(struct coord));
#else
    {
        int i=0,end=count*sizeof(struct coord)/sizeof(int);
        int *src=(int *)t->pos_coord;
        int *dst=(int *)c;
        while (i++ < end) {
            *dst++=le32_to_cpu(*src);
            src++;
        }
    }
#endif
    t->pos_coord+=count*2;
    ret=count;
    return ret;
}

/**
 * @brief Get nuber of coords left for current item
 *
 * @param prv_data
 */
static int binfile_coords_left(void *priv_data) {
    return binfile_coord_left(priv_data);
}

/**
 * @brief
 * @param
 * @return
 */
static void binfile_attr_rewind(void *priv_data) {
    struct map_rect_priv *mr=priv_data;
    struct tile *t=mr->t;
    t->pos_attr=t->pos_attr_start;
    mr->label=0;
    memset(mr->label_attr, 0, sizeof(mr->label_attr));

}

static char *binfile_extract(struct map_priv *m, char *dir, char *filename, int partial) {
    char *full,*fulld,*sep;
    unsigned char *start;
    int len,offset=m->index_offset;
    struct zip_cd *cd;
    struct zip_lfh *lfh;
    FILE *f;

    for (;;) {
        offset=binfile_search_cd(m, offset, filename, partial, 1);
        if (offset == -1)
            break;
        cd=binfile_read_cd(m, offset, -1);
        len=strlen(dir)+1+cd->zipcfnl+1;
        full=g_malloc(len);
        strcpy(full,dir);
        strcpy(full+strlen(full),"/");
        strncpy(full+strlen(full),cd->zipcfn,cd->zipcfnl);
        full[len-1]='\0';
        fulld=g_strdup(full);
        sep=strrchr(fulld, '/');
        if (sep) {
            *sep='\0';
            file_mkdir(fulld, 1);
        }
        if (full[len-2] != '/') {
            lfh=binfile_read_lfh(m->fi, binfile_cd_offset(cd));
            start=binfile_read_content(m, m->fi, binfile_cd_offset(cd), lfh);
            dbg(lvl_debug,"fopen '%s'", full);
            f=fopen(full,"w");
            fwrite(start, lfh->zipuncmp, 1, f);
            fclose(f);
            file_data_free(m->fi, start);
            file_data_free(m->fi, (unsigned char *)lfh);
        }
        file_data_free(m->fi, (unsigned char *)cd);
        g_free(fulld);
        g_free(full);
        if (! partial)
            break;
    }

    return g_strdup_printf("%s/%s",dir,filename);
}

static int binfile_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr) {
    struct map_rect_priv *mr=priv_data;
    struct tile *t=mr->t;
    enum attr_type type;
    int i,size;

    if (attr_type != mr->attr_last) {
        t->pos_attr=t->pos_attr_start;
        mr->attr_last=attr_type;
    }
    while (t->pos_attr < t->pos_next) {
        size=le32_to_cpu(*(t->pos_attr++));
        type=le32_to_cpu(t->pos_attr[0]);
        if (type == attr_label)
            mr->label=1;
        if (type == attr_house_number)
            mr->label_attr[0]=t->pos_attr;
        if (type == attr_street_name)
            mr->label_attr[1]=t->pos_attr;
        if (type == attr_street_name_systematic)
            mr->label_attr[2]=t->pos_attr;
        if (type == attr_district_name && mr->item.type < type_line)
            mr->label_attr[3]=t->pos_attr;
        if (type == attr_town_name && mr->item.type < type_line)
            mr->label_attr[4]=t->pos_attr;
        if (type == attr_type || attr_type == attr_any) {
            if (attr_type == attr_any) {
                dbg(lvl_debug,"pos %p attr %s size %d", t->pos_attr-1, attr_to_name(type), size);
            }
            attr->type=type;
            if (ATTR_IS_GROUP(type)) {
                int i=0;
                int *subpos=t->pos_attr+1;
                int size_rem=size-1;
                i=0;
                while (size_rem > 0 && i < 7) {
                    int subsize=le32_to_cpu(*subpos++);
                    int subtype=le32_to_cpu(subpos[0]);
                    mr->attrs[i].type=subtype;
                    attr_data_set_le(&mr->attrs[i], subpos+1);
                    subpos+=subsize;
                    size_rem-=subsize+1;
                    i++;
                }
                mr->attrs[i].type=type_none;
                mr->attrs[i].u.data=NULL;
                attr->u.attrs=mr->attrs;
            } else {
                attr_data_set_le(attr, t->pos_attr+1);
                if (type == attr_url_local) {
                    g_free(mr->url);
                    mr->url=binfile_extract(mr->m, mr->m->cachedir, attr->u.str, 1);
                    attr->u.str=mr->url;
                }
                if (type == attr_flags && mr->m->map_version < 1)
                    attr->u.num |= AF_CAR;
            }
            t->pos_attr+=size;
            return 1;
        } else {
            t->pos_attr+=size;
        }
    }
    if (!mr->label && (attr_type == attr_any || attr_type == attr_label)) {
        for (i = 0 ; i < sizeof(mr->label_attr)/sizeof(int *) ; i++) {
            if (mr->label_attr[i]) {
                mr->label=1;
                attr->type=attr_label;
                attr_data_set_le(attr,mr->label_attr[i]+1);
                return 1;
            }
        }
    }
    return 0;
}

struct binfile_hash_entry {
    struct item_id id;
    int flags;
    int data[0];
};

static guint binfile_hash_entry_hash(gconstpointer key) {
    const struct binfile_hash_entry *entry=key;
    return (entry->id.id_hi ^ entry->id.id_lo);
}

static gboolean binfile_hash_entry_equal(gconstpointer a, gconstpointer b) {
    const struct binfile_hash_entry *entry1=a,*entry2=b;
    return (entry1->id.id_hi==entry2->id.id_hi && entry1->id.id_lo == entry2->id.id_lo);
}

static int *binfile_item_dup(struct map_priv *m, struct item *item, struct tile *t, int extend) {
    int size=le32_to_cpu(t->pos[0]);
    struct binfile_hash_entry *entry=g_malloc(sizeof(struct binfile_hash_entry)+(size+1+extend)*sizeof(int));
    void *ret=entry->data;
    entry->id.id_hi=item->id_hi;
    entry->id.id_lo=item->id_lo;
    entry->flags=1;
    dbg(lvl_debug,"id 0x%x,0x%x",entry->id.id_hi,entry->id.id_lo);

    memcpy(ret, t->pos, (size+1)*sizeof(int));
    if (!m->changes)
        m->changes=g_hash_table_new_full(binfile_hash_entry_hash, binfile_hash_entry_equal, g_free, NULL);
    g_hash_table_replace(m->changes, entry, entry);
    dbg(lvl_debug,"ret %p",ret);
    return ret;
}

static int binfile_coord_set(void *priv_data, struct coord *c, int count, enum change_mode mode) {
    struct map_rect_priv *mr=priv_data;
    struct tile *t=mr->t,*tn,new;
    int i,delta,move_len;
    int write_offset,move_offset,aoffset,coffset,clen;
    int *data;

    {
        int *i=t->pos,j=0;
        dbg(lvl_debug,"Before: pos_coord=%td",t->pos_coord-i);
        while (i < t->pos_next)
            dbg(lvl_debug,"%d:0x%x",j++,*i++);

    }
    aoffset=t->pos_attr-t->pos_attr_start;
    coffset=t->pos_coord-t->pos_coord_start-2;
    clen=t->pos_attr_start-t->pos_coord+2;
    dbg(lvl_debug,"coffset=%d clen=%d",coffset,clen);
    switch (mode) {
    case change_mode_delete:
        if (count*2 > clen)
            count=clen/2;
        delta=-count*2;
        move_offset=coffset+count*2;
        move_len=t->pos_next-t->pos_coord_start-move_offset;
        write_offset=0;
        break;
    case change_mode_modify:
        write_offset=coffset;
        if (count*2 > clen) {
            delta=count*2-clen;
            move_offset=t->pos_attr_start-t->pos_coord_start;
            move_len=t->pos_next-t->pos_coord_start-move_offset;
        } else {
            move_len=0;
            move_offset=0;
            delta=0;
        }
        break;
    case change_mode_prepend:
        delta=count*2;
        move_offset=coffset-2;
        move_len=t->pos_next-t->pos_coord_start-move_offset;
        write_offset=coffset-2;
        break;
    case change_mode_append:
        delta=count*2;
        move_offset=coffset;
        move_len=t->pos_next-t->pos_coord_start-move_offset;
        write_offset=coffset;
        break;
    default:
        return 0;
    }
    dbg(lvl_debug,"delta %d",delta);
    data=binfile_item_dup(mr->m, &mr->item, t, delta > 0 ? delta:0);
    data[0]=cpu_to_le32(le32_to_cpu(data[0])+delta);
    data[2]=cpu_to_le32(le32_to_cpu(data[2])+delta);
    new.pos=new.start=data;
    new.zipfile_num=t->zipfile_num;
    new.mode=2;
    push_tile(mr, &new, 0, 0);
    setup_pos(mr);
    tn=mr->t;
    tn->pos_coord=tn->pos_coord_start+coffset;
    tn->pos_attr=tn->pos_attr_start+aoffset;
    dbg(lvl_debug,"moving %d ints from offset %td to %td",move_len,tn->pos_coord_start+move_offset-data,
        tn->pos_coord_start+move_offset+delta-data);
    memmove(tn->pos_coord_start+move_offset+delta, tn->pos_coord_start+move_offset, move_len*4);
    {
        int *i=tn->pos,j=0;
        dbg(lvl_debug,"After move: pos_coord=%td",tn->pos_coord-i);
        while (i < tn->pos_next)
            dbg(lvl_debug,"%d:0x%x",j++,*i++);
    }
    if (mode != change_mode_append)
        tn->pos_coord+=move_offset;
    if (mode != change_mode_delete) {
        dbg(lvl_debug,"writing %d ints at offset %td",count*2,write_offset+tn->pos_coord_start-data);
        for (i = 0 ; i < count ; i++) {
            tn->pos_coord_start[write_offset++]=c[i].x;
            tn->pos_coord_start[write_offset++]=c[i].y;
        }

    }
    {
        int *i=tn->pos,j=0;
        dbg(lvl_debug,"After: pos_coord=%td",tn->pos_coord-i);
        while (i < tn->pos_next)
            dbg(lvl_debug,"%d:0x%x",j++,*i++);
    }
    return 1;
}

static int binfile_attr_set(void *priv_data, struct attr *attr, enum change_mode mode) {
    struct map_rect_priv *mr=priv_data;
    struct tile *t=mr->t,*tn,new;
    int offset,delta,move_len;
    int write_offset,move_offset,naoffset,coffset,oattr_len;
    int nattr_size,nattr_len,pad;
    int *data;

    {
        int *i=t->pos,j=0;
        dbg(lvl_debug,"Before: pos_attr=%td",t->pos_attr-i);
        while (i < t->pos_next)
            dbg(lvl_debug,"%d:0x%x",j++,*i++);

    }

    write_offset=0;
    naoffset=t->pos_attr-t->pos_attr_start;
    coffset=t->pos_coord-t->pos_coord_start;
    offset=0;
    oattr_len=0;
    if (!naoffset) {
        if (mode == change_mode_delete || mode == change_mode_modify) {
            dbg(lvl_error,"no attribute selected");
            return 0;
        }
        if (mode == change_mode_append)
            naoffset=t->pos_next-t->pos_attr_start;
    }
    while (offset < naoffset) {
        oattr_len=le32_to_cpu(t->pos_attr_start[offset])+1;
        dbg(lvl_debug,"len %d",oattr_len);
        write_offset=offset;
        offset+=oattr_len;
    }
    move_len=t->pos_next-t->pos_attr_start-offset;
    move_offset=offset;
    switch (mode) {
    case change_mode_delete:
        nattr_size=0;
        nattr_len=0;
        pad=0;
        break;
    case change_mode_modify:
    case change_mode_prepend:
    case change_mode_append:
        nattr_size=attr_data_size(attr);
        pad=(4-(nattr_size%4))%4;
        nattr_len=(nattr_size+pad)/4+2;
        if (mode == change_mode_prepend) {
            move_offset=write_offset;
            move_len+=oattr_len;
        }
        if (mode == change_mode_append) {
            write_offset=move_offset;
        }
        break;
    default:
        return 0;
    }
    if (mode == change_mode_delete || mode == change_mode_modify)
        delta=nattr_len-oattr_len;
    else
        delta=nattr_len;
    dbg(lvl_debug,"delta %d oattr_len %d nattr_len %d",delta,oattr_len, nattr_len);
    data=binfile_item_dup(mr->m, &mr->item, t, delta > 0 ? delta:0);
    data[0]=cpu_to_le32(le32_to_cpu(data[0])+delta);
    new.pos=new.start=data;
    new.zipfile_num=t->zipfile_num;
    new.mode=2;
    push_tile(mr, &new, 0, 0);
    setup_pos(mr);
    tn=mr->t;
    tn->pos_coord=tn->pos_coord_start+coffset;
    tn->pos_attr=tn->pos_attr_start+offset;
    dbg(lvl_debug,"attr start %td offset %d",tn->pos_attr_start-data,offset);
    dbg(lvl_debug,"moving %d ints from offset %td to %td",move_len,tn->pos_attr_start+move_offset-data,
        tn->pos_attr_start+move_offset+delta-data);
    memmove(tn->pos_attr_start+move_offset+delta, tn->pos_attr_start+move_offset, move_len*4);
    if (mode != change_mode_append)
        tn->pos_attr+=delta;
    {
        int *i=tn->pos,j=0;
        dbg(lvl_debug,"After move: pos_attr=%td",tn->pos_attr-i);
        while (i < tn->pos_next)
            dbg(lvl_debug,"%d:0x%x",j++,*i++);
    }
    if (nattr_len) {
        int *nattr=tn->pos_attr_start+write_offset;
        dbg(lvl_debug,"writing %d ints at %td",nattr_len,nattr-data);
        nattr[0]=cpu_to_le32(nattr_len-1);
        nattr[1]=cpu_to_le32(attr->type);
        memcpy(nattr+2, attr_data_get(attr), nattr_size);
        memset((unsigned char *)(nattr+2)+nattr_size, 0, pad);
    }
    {
        int *i=tn->pos,j=0;
        dbg(lvl_debug,"After: pos_attr=%td",tn->pos_attr-i);
        while (i < tn->pos_next)
            dbg(lvl_debug,"After: pos_attr=%td",tn->pos_attr-i);
        while (i < tn->pos_next)
            dbg(lvl_debug,"%d:0x%x",j++,*i++);
    }
    return 1;
}

static struct item_methods methods_binfile = {
    binfile_coord_rewind,
    binfile_coord_get,
    binfile_attr_rewind,
    binfile_attr_get,
    NULL,
    binfile_attr_set,
    binfile_coord_set,
    NULL,
    binfile_coords_left,
};

static void push_tile(struct map_rect_priv *mr, struct tile *t, int offset, int length) {
    dbg_assert(mr->tile_depth < 8);
    mr->t=&mr->tiles[mr->tile_depth++];
    *(mr->t)=*t;
    mr->t->pos=mr->t->pos_next=mr->t->start+offset;
    if (length == -1)
        length=le32_to_cpu(mr->t->pos[0])+1;
    if (length > 0)
        mr->t->end=mr->t->pos+length;
}

static int pop_tile(struct map_rect_priv *mr) {
    if (mr->tile_depth <= 1)
        return 0;
    if (mr->t->mode < 2)
        file_data_free(mr->m->fi, (unsigned char *)(mr->t->start));
#ifdef DEBUG_SIZE
#if DEBUG_SIZE > 0
    dbg(lvl_debug,"leave %d",mr->t->zipfile_num);
#endif
#endif
    mr->t=&mr->tiles[--mr->tile_depth-1];
    return 1;
}


static int zipfile_to_tile(struct map_priv *m, struct zip_cd *cd, struct tile *t) {
    char buffer[1024];
    struct zip_lfh *lfh;
    char *zipfn;
    struct file *fi;
    dbg(lvl_debug,"enter %p %p %p", m, cd, t);
    dbg(lvl_debug,"cd->zipofst=0x"LONGLONG_HEX_FMT "", binfile_cd_offset(cd));
    t->start=NULL;
    t->mode=1;
    if (m->fis)
        fi=m->fis[cd->zipdsk];
    else
        fi=m->fi;
    lfh=binfile_read_lfh(fi, binfile_cd_offset(cd));
    zipfn=(char *)(file_data_read(fi,binfile_cd_offset(cd)+sizeof(struct zip_lfh), lfh->zipfnln));
    strncpy(buffer, zipfn, lfh->zipfnln);
    buffer[lfh->zipfnln]='\0';
    t->start=(int *)binfile_read_content(m, fi, binfile_cd_offset(cd), lfh);
    t->end=t->start+lfh->zipuncmp/4;
    t->fi=fi;
    file_data_free(fi, (unsigned char *)zipfn);
    file_data_free(fi, (unsigned char *)lfh);
    return t->start != NULL;
}


static int map_binfile_handle_redirect(struct map_priv *m) {
    char *location=file_http_header(m->http, "location");
    if (!location) {
        m->redirect=0;
        return 0;
    }
    if (m->redirect)
        return 0;
    m->redirect=1;
    dbg(lvl_debug,"redirected from %s to %s",m->url,location);
    g_free(m->url);
    m->url=g_strdup(location);
    file_destroy(m->http);
    m->http=NULL;

    return 1;
}

static int map_binfile_http_request(struct map_priv *m, struct attr **attrs) {
    if (!m->http) {
        m->http=file_create(NULL, attrs);
    } else {
        file_request(m->http, attrs);
    }
    return 1;
}


static long long map_binfile_download_size(struct map_priv *m) {
    struct attr url= {attr_url};
    struct attr http_method= {attr_http_method};
    struct attr persistent= {attr_persistent};
    struct attr *attrs[4];
    int size_ret;
    long long ret;
    void *data;

    do {
        attrs[0]=&url;
        url.u.str=m->url;
        attrs[1]=&http_method;
        http_method.u.str="HEAD";
        persistent.u.num=1;
        attrs[2]=&persistent;
        attrs[3]=NULL;

        map_binfile_http_request(m, attrs);
        data=file_data_read_special(m->http, 0, &size_ret);
        g_free(data);
        if (size_ret < 0)
            return 0;
    } while (map_binfile_handle_redirect(m));

    ret=file_size(m->http);
    dbg(lvl_debug,"file size "LONGLONG_FMT"",ret);
    return ret;
}


static int map_binfile_http_close(struct map_priv *m) {
    if (m->http) {
        file_destroy(m->http);
        m->http=NULL;
    }
    return 1;
}


static struct file *map_binfile_http_range(struct map_priv *m, long long offset, int size) {
    struct attr *attrs[4];
    struct attr url= {attr_url};
    struct attr http_header= {attr_http_header};
    struct attr persistent= {attr_persistent};

    persistent.u.num=1;
    attrs[0]=&url;
    attrs[1]=&http_header;
    attrs[2]=&persistent;
    attrs[3]=NULL;

    url.u.str=m->url;
    http_header.u.str=g_strdup_printf("Range: bytes="LONGLONG_FMT"-"LONGLONG_FMT,offset, offset+size-1);
    map_binfile_http_request(m, attrs);
    g_free(http_header.u.str);
    return m->http;
}

static unsigned char *map_binfile_download_range(struct map_priv *m, long long offset, int size) {
    unsigned char *ret;
    int size_ret;
    struct file *http=map_binfile_http_range(m, offset, size);

    ret=file_data_read_special(http, size, &size_ret);
    if (size_ret != size) {
        dbg(lvl_debug,"size %d vs %d",size,size_ret);
        g_free(ret);
        return NULL;
    }
    return ret;
}

static struct zip_cd *download_cd(struct map_download *download) {
    struct map_priv *m=download->m;
    struct zip64_eoc *zip64_eoc=(struct zip64_eoc *)file_data_read(m->fi, 0, sizeof(*zip64_eoc));
    struct zip_cd *cd=(struct zip_cd *)map_binfile_download_range(m, zip64_eoc->zip64eofst+download->zipfile*m->cde_size,
                      m->cde_size);
    file_data_free(m->fi, (unsigned char *)zip64_eoc);
    dbg(lvl_debug,"needed cd, result %p",cd);
    return cd;
}

static int download_request(struct map_download *download) {
    struct attr url= {attr_url};
    struct attr http_header= {attr_http_header};
    struct attr persistent= {attr_persistent};
    struct attr *attrs[4];

    if(!download->m->download_enabled) {
        dbg(lvl_error,"Tried downloading while it's not allowed");
        return 0;
    }
    attrs[0]=&url;
    persistent.u.num=1;
    attrs[1]=&persistent;
    attrs[2]=NULL;
    if (strchr(download->m->url,'?')) {
        url.u.str=g_strdup_printf("%smemberid=%d",download->m->url,download->zipfile);
        download->dl_size=-1;
    } else {
        long long offset=binfile_cd_offset(download->cd_copy);
        int size=download->cd_copy->zipcsiz+sizeof(struct zip_lfh)+download->cd_copy->zipcfnl;
        url.u.str=g_strdup(download->m->url);
        http_header.u.str=g_strdup_printf("Range: bytes="LONGLONG_FMT"-"LONGLONG_FMT,offset,offset+size-1);
        attrs[2]=&http_header;
        attrs[3]=NULL;
        download->dl_size=size;
    }
    dbg(lvl_debug,"encountered missing tile %d %s(%s), Downloading %d bytes at "LONGLONG_FMT"",download->zipfile, url.u.str,
        (char *)(download->cd_copy+1), download->dl_size, download->offset);
    map_binfile_http_request(download->m, attrs);
    g_free(url.u.str);
    download->http=download->m->http;
    return 1;
}


static int download_start(struct map_download *download) {
    long long offset;
    struct zip_eoc *eoc;

    if (!download->cd->zipcensig) {
        download->cd_copy=download_cd(download);
    } else {
        download->cd_copy=g_malloc(download->m->cde_size);
        memcpy(download->cd_copy, download->cd, download->m->cde_size);
    }
    file_data_remove(download->file, (unsigned char *)download->cd);
    download->cd=NULL;
    offset=file_size(download->file);
    offset-=sizeof(struct zip_eoc);
    eoc=(struct zip_eoc *)file_data_read(download->file, offset, sizeof(struct zip_eoc));
    download->zip_eoc=g_malloc(sizeof(struct zip_eoc));
    memcpy(download->zip_eoc, eoc, sizeof(struct zip_eoc));
    file_data_remove(download->file, (unsigned char *)eoc);
    download->start_offset=download->offset=offset;
    return download_request(download);
}

static int download_download(struct map_download *download) {
    int size=64*1024,size_ret;
    unsigned char *data;
    if (download->dl_size != -1 && size > download->dl_size)
        size=download->dl_size;
    if (!size)
        return 1;
    data=file_data_read_special(download->http, size, &size_ret);
    if (!download->read && download->m->http && map_binfile_handle_redirect(download->m)) {
        g_free(data);
        download_request(download);
        return 0;
    }

    dbg(lvl_debug,"got %d bytes writing at offset "LONGLONG_FMT"",size_ret,download->offset);
    if (size_ret <= 0) {
        g_free(data);
        return 1;
    }
    file_data_write(download->file, download->offset, size_ret, data);
    download->offset+=size_ret;
    download->read+=size_ret;
    download->dl_size-=size_ret;
    if (download->dl_size != -1)
        download->progress=download->read*100/(download->read+download->dl_size);
    return 0;
}

static int download_finish(struct map_download *download) {
    struct zip_lfh *lfh;
    char *lfh_filename;
    struct zip_cd_ext *ext;
    long long lfh_offset;
    file_data_write(download->file, download->offset, sizeof(struct zip_eoc), (void *)download->zip_eoc);
    lfh=(struct zip_lfh *)(file_data_read(download->file,download->start_offset, sizeof(struct zip_lfh)));
    ext=binfile_cd_ext(download->cd_copy);
    if (ext)
        ext->zipofst=download->start_offset;
    else
        download->cd_copy->zipofst=download->start_offset;
    download->cd_copy->zipcsiz=lfh->zipsize;
    download->cd_copy->zipcunc=lfh->zipuncmp;
    download->cd_copy->zipccrc=lfh->zipcrc;
    lfh_offset = binfile_cd_offset(download->cd_copy)+sizeof(struct zip_lfh);
    lfh_filename=(char *)file_data_read(download->file,lfh_offset,lfh->zipfnln);
    memcpy(download->cd_copy+1,lfh_filename,lfh->zipfnln);
    file_data_remove(download->file,(void *)lfh_filename);
    file_data_remove(download->file,(void *)lfh);
    file_data_write(download->file, download->m->eoc->zipeofst + download->zipfile*download->m->cde_size,
                    binfile_cd_extra(download->cd_copy)+sizeof(struct zip_cd), (void *)download->cd_copy);
    file_data_flush(download->file, download->m->eoc->zipeofst + download->zipfile*download->m->cde_size,
                    sizeof(struct zip_cd));

    g_free(download->cd_copy);
    download->cd=(struct zip_cd *)(file_data_read(download->file,
                                   download->m->eoc->zipeofst + download->zipfile*download->m->cde_size, download->m->cde_size));
    cd_to_cpu(download->cd);
    dbg(lvl_debug,"Offset %d",download->cd->zipofst);
    return 1;
}

static int download_planet_size(struct map_download *download) {
    download->size=map_binfile_download_size(download->m);
    dbg(lvl_debug,"Planet size "LONGLONG_FMT"",download->size);
    if (!download->size)
        return 0;
    return 1;
}

static int download_eoc(struct map_download *download) {
    download->zip64_eoc=(struct zip64_eoc *)map_binfile_download_range(download->m, download->size-98, 98);
    if (!download->zip64_eoc)
        return 0;
    download->zip64_eocl=(struct zip64_eocl *)(download->zip64_eoc+1);
    download->zip_eoc=(struct zip_eoc *)(download->zip64_eocl+1);
    if (download->zip64_eoc->zip64esig != zip64_eoc_sig || download->zip64_eocl->zip64lsig != zip64_eocl_sig
            || download->zip_eoc->zipesig != zip_eoc_sig) {
        dbg(lvl_error,"wrong signature on zip64_eoc downloaded from "LONGLONG_FMT"",download->size-98);
        g_free(download->zip64_eoc);
        return 0;
    }
    return 1;
}

static int download_directory_start(struct map_download *download) {
    download->http=map_binfile_http_range(download->m, download->zip64_eoc->zip64eofst, download->zip64_eoc->zip64ecsz);
    if (!download->http)
        return 0;
    return 1;
}

static int download_directory_do(struct map_download *download) {
    int count;

    for (count = 0 ; count < 100 ; count++) {
        int cd_xlen, size_ret;
        unsigned char *cd_data;
        struct zip_cd *cd;
        cd=(struct zip_cd *)file_data_read_special(download->http, sizeof(*cd), &size_ret);
        cd->zipcunc=0;
        dbg(lvl_debug,"size_ret=%d",size_ret);
        if (!size_ret)
            return 0;
        if (size_ret != sizeof(*cd) || cd->zipcensig != zip_cd_sig) {
            dbg(lvl_error,"error1 size=%d vs %zu",size_ret, sizeof(*cd));
            return 0;
        }
        file_data_write(download->file, download->offset, sizeof(*cd), (unsigned char *)cd);
        download->offset+=sizeof(*cd);
        cd_xlen=cd->zipcfnl+cd->zipcxtl;
        cd_data=file_data_read_special(download->http, cd_xlen, &size_ret);
        if (size_ret != cd_xlen) {
            dbg(lvl_error,"error2 size=%d vs %d",size_ret,cd_xlen);
            return 0;
        }
        file_data_write(download->file, download->offset, cd_xlen, cd_data);
        download->offset+=cd_xlen;
        g_free(cd);
        g_free(cd_data);
    }
    return 1;
}

static int download_directory_finish(struct map_download *download) {
    download->http=NULL;
    return 1;
}

static int download_initial_finish(struct map_download *download) {
    download->zip64_eoc->zip64eofst=download->cd1offset;
    download->zip64_eocl->zip64lofst=download->offset;
    download->zip_eoc->zipeofst=download->cd1offset;
#if 0
    file_data_write(download->file, download->offset, sizeof(*download->zip64_eoc), (unsigned char *)download->zip64_eoc);
    download->offset+=sizeof(*download->zip64_eoc);
    file_data_write(download->file, download->offset, sizeof(*download->zip64_eocl), (unsigned char *)download->zip64_eocl);
    download->offset+=sizeof(*download->zip64_eocl);
#endif
    file_data_write(download->file, download->offset, sizeof(*download->zip_eoc), (unsigned char *)download->zip_eoc);
    download->offset+=sizeof(*download->zip_eoc);
    g_free(download->zip64_eoc);
    download->zip64_eoc=NULL;
    return 1;
}

static void push_zipfile_tile_do(struct map_rect_priv *mr, struct zip_cd *cd, int zipfile, int offset, int length)

{
    struct tile t;
    struct map_priv *m=mr->m;
    struct file *f=m->fi;

    dbg(lvl_debug,"enter %p %d", mr, zipfile);
#ifdef DEBUG_SIZE
#if DEBUG_SIZE > 0
    {
        char filename[cd->zipcfnl+1];
        memcpy(filename, cd+1, cd->zipcfnl);
        filename[cd->zipcfnl]='\0';
        dbg(lvl_debug,"enter %d (%s) %d",zipfile, filename, cd->zipcunc);
    }
#endif
    mr->size+=cd->zipcunc;
#endif
    t.zipfile_num=zipfile;
    if (zipfile_to_tile(m, cd, &t))
        push_tile(mr, &t, offset, length);
    file_data_free(f, (unsigned char *)cd);
}


static struct zip_cd *download(struct map_priv *m, struct map_rect_priv *mr, struct zip_cd *cd, int zipfile, int offset,
                               int length,
                               int async) {
    struct map_download *download;

    if(!m->download_enabled)
        return NULL;

    if (async == 2) {
        download=m->download;
    } else {
        download=g_new0(struct map_download, 1);
        if (mr) {
            download->m=m;
            download->mr=mr;
            download->file=m->fi;
            download->cd=cd;
            download->zipfile=zipfile;
            download->toffset=offset;
            download->tlength=length;
            download->state=1;
        } else {
            struct attr readwrite= {attr_readwrite,{(void *)1}};
            struct attr create= {attr_create,{(void *)1}};
            struct attr *attrs[3];
            attrs[0]=&readwrite;
            attrs[1]=&create;
            attrs[2]=NULL;
            download->file=file_create(m->filename,attrs);
            download->m=m;
            download->state=4;
        }
    }
    if (async == 1) {
        m->download=download;
        g_free(m->progress);
        if (download->mr)
            m->progress=g_strdup_printf("Download Tile %d 0%%",download->zipfile);
        else
            m->progress=g_strdup_printf("Download Map Information 0%%");
        callback_list_call_attr_0(m->cbl, attr_progress);
        return NULL;
    }
    for (;;) {
        dbg(lvl_debug,"state=%d",download->state);
        switch (download->state) {
        case 0:
            dbg(lvl_error,"error");
            break;
        case 1:
            if (download_start(download))
                download->state=2;
            else
                download->state=0;
            break;
        case 2:
            if (download_download(download))
                download->state=3;
            else {
                g_free(m->progress);
                m->progress=g_strdup_printf("Download Tile %d %d%%",download->zipfile,download->progress);
                callback_list_call_attr_0(m->cbl, attr_progress);
            }
            break;
        case 3:
            if (download_finish(download)) {
                struct zip_cd *ret;
                g_free(m->progress);
                m->progress=g_strdup_printf("Download Tile %d 100%%",download->zipfile);
                callback_list_call_attr_0(m->cbl, attr_progress);
                if (async) {
                    push_zipfile_tile_do(download->mr, download->cd, download->zipfile, download->toffset, download->tlength);
                    ret=NULL;
                } else
                    ret=download->cd;
                g_free(m->progress);
                m->progress=NULL;
                g_free(download);
                if (async)
                    m->download=NULL;
                return ret;
            } else
                download->state=0;
            break;
        case 4:
            if (download_planet_size(download))
                download->state=5;
            else
                download->state=0;
            break;
        case 5:
            g_free(m->progress);
            m->progress=g_strdup_printf("Download Map Information 50%%");
            callback_list_call_attr_0(m->cbl, attr_progress);
            if (download_eoc(download))
                download->state=6;
            else {
                dbg(lvl_error,"download of eoc failed");
                download->state=0;
            }
            break;
        case 6:
            g_free(m->progress);
            m->progress=g_strdup_printf("Download Map Information 100%%");
            callback_list_call_attr_0(m->cbl, attr_progress);
            if (download_directory_start(download))
                download->state=7;
            else
                download->state=0;
            break;
        case 7:
            g_free(m->progress);
            m->progress=g_strdup_printf("Download Map Directory %d%%",(int)(download->offset*100/download->zip64_eoc->zip64ecsz));
            callback_list_call_attr_0(m->cbl, attr_progress);
            if (!download_directory_do(download))
                download->state=8;
            break;
        case 8:
            if (download_directory_finish(download))
                download->state=9;
            else
                download->state=0;
            break;
        case 9:
            download_initial_finish(download);
            m->fi=download->file;
            g_free(m->progress);
            m->progress=NULL;
            g_free(download);
            if (async)
                m->download=NULL;
            map_binfile_open(m);
            break;
        }
        if (async)
            return NULL;
    }
}

static int push_zipfile_tile(struct map_rect_priv *mr, int zipfile, int offset, int length, int async) {
    struct map_priv *m=mr->m;
    struct file *f=m->fi;
    long long cdoffset=m->eoc64?m->eoc64->zip64eofst:m->eoc->zipeofst;
    struct zip_cd *cd=(struct zip_cd *)(file_data_read(f, cdoffset + zipfile*m->cde_size, m->cde_size));
    dbg(lvl_debug,"read from "LONGLONG_FMT" %d bytes",cdoffset + zipfile*m->cde_size, m->cde_size);
    cd_to_cpu(cd);
    if (!cd->zipcunc && m->url) {
        cd=download(m, mr, cd, zipfile, offset, length, async);
        if (!cd)
            return 1;
    }
    push_zipfile_tile_do(mr, cd, zipfile, offset, length);
    return 0;
}

static struct map_rect_priv *map_rect_new_binfile_int(struct map_priv *map, struct map_selection *sel) {
    struct map_rect_priv *mr;

    binfile_check_version(map);
    dbg(lvl_debug,"map_rect_new_binfile");
    if (!map->fi && !map->url)
        return NULL;
    map_binfile_http_close(map);
    mr=g_new0(struct map_rect_priv, 1);
    mr->m=map;
    mr->sel=sel;
    mr->item.id_hi=0;
    mr->item.id_lo=0;
    mr->item.meth=&methods_binfile;
    mr->item.priv_data=mr;
    return mr;
}

static void tile_bbox(char *tile, int len, struct coord_rect *r) {
    struct coord c;
    int overlap=1;
    int xo,yo;
    struct coord_rect world_bbox = {
        { WORLD_BOUNDINGBOX_MIN_X, WORLD_BOUNDINGBOX_MAX_Y}, /* left upper corner */
        { WORLD_BOUNDINGBOX_MAX_X, WORLD_BOUNDINGBOX_MIN_Y}, /* right lower corner */
    };
    *r=world_bbox;
    while (len) {
        c.x=(r->lu.x+r->rl.x)/2;
        c.y=(r->lu.y+r->rl.y)/2;
        xo=(r->rl.x-r->lu.x)*overlap/100;
        yo=(r->lu.y-r->rl.y)*overlap/100;
        switch (*tile) {
        case 'a':
            r->lu.x=c.x-xo;
            r->rl.y=c.y-yo;
            break;
        case 'b':
            r->rl.x=c.x+xo;
            r->rl.y=c.y-yo;
            break;
        case 'c':
            r->lu.x=c.x-xo;
            r->lu.y=c.y+yo;
            break;
        case 'd':
            r->rl.x=c.x+xo;
            r->lu.y=c.y+yo;
            break;
        default:
            return;
        }
        tile++;
        len--;
    }
}

static int map_download_selection_check(struct zip_cd *cd, struct map_selection *sel) {
    struct coord_rect cd_rect;
    if (cd->zipcunc)
        return 0;
    tile_bbox((char *)(cd+1), cd->zipcfnl, &cd_rect);
    while (sel) {
        if (coord_rect_overlap(&cd_rect, &sel->u.c_rect))
            return 1;
        sel=sel->next;
    }
    return 0;
}

static void map_download_selection(struct map_priv *m, struct map_rect_priv *mr, struct map_selection *sel) {
    int i;
    struct zip_cd *cd;
    for (i = 0 ; i < m->zip_members ; i++) {
        cd=binfile_read_cd(m, m->cde_size*i, -1);
        if (map_download_selection_check(cd, sel))
            download(m, mr, cd, i, 0, 0, 0);
        file_data_free(m->fi, (unsigned char *)cd);
    }
}

static struct map_rect_priv *map_rect_new_binfile(struct map_priv *map, struct map_selection *sel) {
    struct map_rect_priv *mr=map_rect_new_binfile_int(map, sel);
    struct tile t;
    dbg(lvl_debug,"zip_members=%d", map->zip_members);
    if (map->url && map->fi && sel && sel->order == 255) {
        map_download_selection(map, mr, sel);
    }
    if (map->eoc)
        mr->status=1;
    else {
        unsigned char *d;
        if (map->fi) {
            d=file_data_read(map->fi, 0, map->fi->size);
            t.start=(int *)d;
            t.end=(int *)(d+map->fi->size);
            t.fi=map->fi;
            t.zipfile_num=0;
            t.mode=0;
            push_tile(mr, &t, 0, 0);
        } else if (map->url && !map->download) {
            download(map, NULL, NULL, 0, 0, 0, 1);
            mr->status=1;
        }
    }
    return mr;
}

static void write_changes_do(gpointer key, gpointer value, gpointer user_data) {
    struct binfile_hash_entry *entry=key;
    FILE *out=user_data;
    if (entry->flags) {
        entry->flags=0;
        fwrite(entry, sizeof(*entry)+(le32_to_cpu(entry->data[0])+1)*4, 1, out);
        dbg(lvl_debug,"yes");
    }
}

static void write_changes(struct map_priv *m) {
    FILE *changes;
    char *changes_file;
    if (!m->changes)
        return;
    changes_file=g_strdup_printf("%s.log",m->filename);
    changes=fopen(changes_file,"ab");
    g_hash_table_foreach(m->changes, write_changes_do, changes);
    fclose(changes);
    g_free(changes_file);
}

static void load_changes(struct map_priv *m) {
    FILE *changes;
    char *changes_file;
    struct binfile_hash_entry entry,*e;
    int size;
    changes_file=g_strdup_printf("%s.log",m->filename);
    changes=fopen(changes_file,"rb");
    if (! changes) {
        g_free(changes_file);
        return;
    }
    m->changes=g_hash_table_new_full(binfile_hash_entry_hash, binfile_hash_entry_equal, g_free, NULL);
    while (fread(&entry, sizeof(entry), 1, changes) == 1) {
        if (fread(&size, sizeof(size), 1, changes) != 1)
            break;
        e=g_malloc(sizeof(struct binfile_hash_entry)+(le32_to_cpu(size)+1)*4);
        *e=entry;
        e->data[0]=size;
        if (fread(e->data+1, le32_to_cpu(size)*4, 1, changes) != 1)
            break;
        g_hash_table_replace(m->changes, e, e);
    }
    fclose(changes);
    g_free(changes_file);
}


static void map_rect_destroy_binfile(struct map_rect_priv *mr) {
    write_changes(mr->m);
    while (pop_tile(mr));
#ifdef DEBUG_SIZE
    dbg(lvl_debug,"size=%d kb",mr->size/1024);
#endif
    if (mr->tiles[0].fi && mr->tiles[0].start)
        file_data_free(mr->tiles[0].fi, (unsigned char *)(mr->tiles[0].start));
    g_free(mr->url);
    map_binfile_http_close(mr->m);
    g_free(mr);
}

static void setup_pos(struct map_rect_priv *mr) {
    int size,coord_size;
    struct tile *t=mr->t;
    size=le32_to_cpu(t->pos[0]);
    if (size > 1024*1024 || size < 0) {
        dbg(lvl_debug,"size=0x%x", size);
#if 0
        fprintf(stderr,"offset=%d\n", (unsigned char *)(mr->pos)-mr->m->f->begin);
#endif
        dbg(lvl_debug,"size error");
    }
    t->pos_next=t->pos+size+1;
    mr->item.type=le32_to_cpu(t->pos[1]);
    coord_size=le32_to_cpu(t->pos[2]);
    t->pos_coord_start=t->pos+3;
    t->pos_attr_start=t->pos_coord_start+coord_size;
}

static int selection_contains(struct map_selection *sel, struct coord_rect *r, struct range *mima) {
    int order;
    if (! sel)
        return 1;
    while (sel) {
        if (coord_rect_overlap(r, &sel->u.c_rect)) {
            order=sel->order;
            dbg(lvl_debug,"min %d max %d order %d", mima->min, mima->max, order);
            if (!mima->min && !mima->max)
                return 1;
            if (order >= mima->min && order <= mima->max)
                return 1;
        }
        sel=sel->next;
    }
    return 0;
}

static void map_parse_country_binfile(struct map_rect_priv *mr) {
    struct attr at;

    if (!binfile_attr_get(mr->item.priv_data, attr_country_id, &at))
        return;

    if( at.u.num != mr->country_id)
        return;

    if (!binfile_attr_get(mr->item.priv_data, attr_zipfile_ref, &at))
        return;

    if(mr->msp) {
        struct attr *search=&mr->msp->search;
        if(search->type==attr_town_name || search->type==attr_district_name || search->type==attr_town_or_district_name) {
            struct attr af, al;
            if(binfile_attr_get(mr->item.priv_data, attr_first_key, &af)) {
                if(linguistics_compare(af.u.str,search->u.str,linguistics_cmp_partial)>0) {
                    dbg(lvl_debug,"Skipping index item with first_key='%s'", af.u.str);
                    return;
                }
            }
            if(binfile_attr_get(mr->item.priv_data, attr_last_key, &al)) {
                if(linguistics_compare(al.u.str,search->u.str,linguistics_cmp_partial)<0) {
                    dbg(lvl_debug,"Skipping index item with first_key='%s', last_key='%s'", af.u.str, al.u.str);
                    return;
                }
            }
        }
    }

    push_zipfile_tile(mr, at.u.num, 0, 0, 0);
}

static int map_parse_submap(struct map_rect_priv *mr, int async) {
    struct coord_rect r;
    struct coord c[2];
    struct attr at;
    struct range mima;
    if (binfile_coord_get(mr->item.priv_data, c, 2) != 2)
        return 0;
    r.lu.x=c[0].x;
    r.lu.y=c[1].y;
    r.rl.x=c[1].x;
    r.rl.y=c[0].y;
    if (!binfile_attr_get(mr->item.priv_data, attr_order, &at))
        return 0;
#if __BYTE_ORDER == __BIG_ENDIAN
    mima.min=le16_to_cpu(at.u.range.max);
    mima.max=le16_to_cpu(at.u.range.min);
#else
    mima=at.u.range;
#endif
    if (!mr->m->eoc || !selection_contains(mr->sel, &r, &mima))
        return 0;
    if (!binfile_attr_get(mr->item.priv_data, attr_zipfile_ref, &at))
        return 0;
    dbg(lvl_debug,"pushing zipfile %ld from %d", at.u.num, mr->t->zipfile_num);
    return push_zipfile_tile(mr, at.u.num, 0, 0, async);
}

static int push_modified_item(struct map_rect_priv *mr) {
    struct item_id id;
    struct binfile_hash_entry *entry;
    id.id_hi=mr->item.id_hi;
    id.id_lo=mr->item.id_lo;
    entry=g_hash_table_lookup(mr->m->changes, &id);
    if (entry) {
        struct tile tn;
        tn.pos_next=tn.pos=tn.start=entry->data;
        tn.zipfile_num=mr->item.id_hi;
        tn.mode=2;
        tn.end=tn.start+le32_to_cpu(entry->data[0])+1;
        push_tile(mr, &tn, 0, 0);
        return 1;
    }
    return 0;
}

static struct item *map_rect_get_item_binfile(struct map_rect_priv *mr) {
    struct tile *t;
    struct map_priv *m=mr->m;
    if (m->download) {
        download(m, NULL, NULL, 0, 0, 0, 2);
        return &busy_item;
    }
    if (mr->status == 1) {
        mr->status=0;
        if (push_zipfile_tile(mr, m->zip_members-1, 0, 0, 1))
            return &busy_item;
    }
    for (;;) {
        t=mr->t;
        if (! t)
            return NULL;
        t->pos=t->pos_next;
        if (t->pos >= t->end) {
            if (pop_tile(mr))
                continue;
            return NULL;
        }
        setup_pos(mr);
        binfile_coord_rewind(mr);
        binfile_attr_rewind(mr);
        if ((mr->item.type == type_submap) && (!mr->country_id)) {
            if (map_parse_submap(mr, 1))
                return &busy_item;
            continue;
        }
        if (t->mode != 2) {
            mr->item.id_hi=t->zipfile_num;
            mr->item.id_lo=t->pos-t->start;
            if (mr->m->changes && push_modified_item(mr))
                continue;
        }
        if (mr->country_id) {
            if (mr->item.type == type_countryindex) {
                map_parse_country_binfile(mr);
            }
            if (item_is_town(mr->item)) {
                return &mr->item;
            } else {
                continue;
            }
        }
        return &mr->item;
    }
}

static struct item *map_rect_get_item_byid_binfile(struct map_rect_priv *mr, int id_hi, int id_lo) {
    struct tile *t;
    if (mr->m->eoc) {
        while (pop_tile(mr));
        push_zipfile_tile(mr, id_hi, 0, 0, 0);
    }
    t=mr->t;
    t->pos=t->start+id_lo;
    mr->item.id_hi=id_hi;
    mr->item.id_lo=id_lo;
    if (mr->m->changes)
        push_modified_item(mr);
    setup_pos(mr);
    binfile_coord_rewind(mr);
    binfile_attr_rewind(mr);
    return &mr->item;
}

static int binmap_search_by_index(struct map_priv *map, struct item *item, struct map_rect_priv **ret) {
    struct attr zipfile_ref;
    int *data;

    if (!item) {
        *ret=NULL;
        return 0;
    }
    if (item_attr_get(item, attr_item_id, &zipfile_ref)) {
        data=zipfile_ref.u.data;
        *ret=map_rect_new_binfile_int(map, NULL);
        push_zipfile_tile(*ret, le32_to_cpu(data[0]), le32_to_cpu(data[1]), -1, 0);
        return 3;
    }
    if (item_attr_get(item, attr_zipfile_ref, &zipfile_ref)) {
        *ret=map_rect_new_binfile_int(map, NULL);
        push_zipfile_tile(*ret, zipfile_ref.u.num, 0, 0, 0);
        return 1;
    }
    if (item_attr_get(item, attr_zipfile_ref_block, &zipfile_ref)) {
        data=zipfile_ref.u.data;
        *ret=map_rect_new_binfile_int(map, NULL);
        push_zipfile_tile(*ret, le32_to_cpu(data[0]), le32_to_cpu(data[1]), le32_to_cpu(data[2]), 0);
        return 2;
    }
    *ret=NULL;
    return 0;
}

static struct map_rect_priv *binmap_search_street_by_place(struct map_priv *map, struct item *town, struct coord *c,
        struct map_selection *sel,
        GList **boundaries) {
    struct attr town_name, poly_town_name;
    struct map_rect_priv *map_rec2;
    struct item *place;
    int found=0;

    if (!item_attr_get(town, attr_label, &town_name))
        return NULL;
    sel->range = item_range_all;
    sel->order = 18;
    sel->next = NULL;
    sel->u.c_rect.lu=*c;
    sel->u.c_rect.rl=*c;
    map_rec2=map_rect_new_binfile(map, sel);
    while ((place=map_rect_get_item_binfile(map_rec2))) {
        if (item_is_poly_place(*place) &&
                item_attr_get(place, attr_label, &poly_town_name) &&
                !strcmp(poly_town_name.u.str,town_name.u.str)) {
            struct coord *c;
            int i,count;
            struct geom_poly_segment *bnd;
            count=binfile_coord_left(map_rec2);
            c=g_new(struct coord,count);
            found=1;
            item_coord_get(place, c, count);
            for (i = 0 ; i < count ; i++)
                coord_rect_extend(&sel->u.c_rect, &c[i]);
            bnd=g_new(struct geom_poly_segment,1);
            bnd->first=c;
            bnd->last=c+count-1;
            bnd->type=geom_poly_segment_type_way_outer;
            *boundaries=g_list_prepend(*boundaries,bnd);
        }
    }
    map_rect_destroy_binfile(map_rec2);
    if (found)
        return map_rect_new_binfile(map, sel);
    return NULL;
}

static int binmap_get_estimated_town_size(struct item *town) {
    int size = 10000;
    switch (town->type) {
    case type_town_label_1e5:
    case type_town_label_5e4:
    case type_town_label_2e4:
    case type_district_label_1e5:
    case type_district_label_5e4:
    case type_district_label_2e4:
        size = 5000;
        break;
    case type_town_label_1e4:
    case type_town_label_5e3:
    case type_town_label_2e3:
    case type_district_label_1e4:
    case type_district_label_5e3:
    case type_district_label_2e3:
        size = 2500;
        break;
    case type_town_label_1e3:
    case type_town_label_5e2:
    case type_town_label_2e2:
    case type_town_label_1e2:
    case type_town_label_5e1:
    case type_town_label_2e1:
    case type_town_label_1e1:
    case type_town_label_5e0:
    case type_town_label_2e0:
    case type_town_label_1e0:
    case type_town_label_0e0:
    case type_district_label_1e3:
    case type_district_label_5e2:
    case type_district_label_2e2:
    case type_district_label_1e2:
    case type_district_label_5e1:
    case type_district_label_2e1:
    case type_district_label_1e1:
    case type_district_label_5e0:
    case type_district_label_2e0:
    case type_district_label_1e0:
    case type_district_label_0e0:
        size = 1000;
        break;
    default:
        break;
    }
    return size;
}

static struct map_rect_priv *binmap_search_street_by_estimate(struct map_priv *map, struct item *town, struct coord *c,
        struct map_selection *sel) {
    int size = binmap_get_estimated_town_size(town);

    sel->u.c_rect.lu.x = c->x-size;
    sel->u.c_rect.lu.y = c->y+size;
    sel->u.c_rect.rl.x = c->x+size;
    sel->u.c_rect.rl.y = c->y-size;
    return map_rect_new_binfile(map, sel);
}

static struct map_rect_priv *binmap_search_housenumber_by_estimate(struct map_priv *map, struct coord *c,
        struct map_selection *sel) {
    int size = 400;
    sel->u.c_rect.lu.x = c->x-size;
    sel->u.c_rect.lu.y = c->y+size;
    sel->u.c_rect.rl.x = c->x+size;
    sel->u.c_rect.rl.y = c->y-size;

    sel->range = item_range_all;
    sel->order = 18;

    return map_rect_new_binfile(map, sel);
}


static int binmap_get_estimated_boundaries (struct item *town, GList **boundaries) {
    int size = binmap_get_estimated_town_size(town);
    struct coord tc;

    if (item_coord_get(town, &tc, 1)) {
        struct geom_poly_segment *bnd;
        struct coord *c;
        c=g_new(struct coord,5);
        bnd=g_new(struct geom_poly_segment,1);
        c[0].x = tc.x + size;
        c[0].y = tc.y - size;
        c[1].x = tc.x - size;
        c[1].y = tc.y - size;
        c[2].x = tc.x - size;
        c[2].y = tc.y + size;
        c[3].x = tc.x + size;
        c[3].y = tc.y + size;
        c[4].x = c[0].x;
        c[4].y = c[0].y;
        bnd->first=&c[0];
        bnd->last=&c[4];
        bnd->type=geom_poly_segment_type_way_outer;
        *boundaries=g_list_prepend(*boundaries,bnd);
        return 1;
    }
    return 0;
}

static struct map_search_priv *binmap_search_new(struct map_priv *map, struct item *item, struct attr *search,
        int partial) {
    struct map_rect_priv *map_rec;
    struct map_search_priv *msp=g_new0(struct map_search_priv, 1);
    struct item *town;
    int idx;

    msp->search = *search;
    msp->partial = partial;
    if(ATTR_IS_STRING(msp->search.type))
        msp->search.u.str=linguistics_casefold(search->u.str);

    /*
     * NOTE: If you implement search for other attributes than attr_town_name and attr_street_name,
     * please update this comment and the documentation for map_search_new() in map.c
     */
    switch (search->type) {
    case attr_country_name:
        break;
    case attr_town_name:
    case attr_town_or_district_name:
    case attr_town_postal:
        map_rec = map_rect_new_binfile(map, NULL);
        if (!map_rec)
            break;
        map_rec->country_id = item->id_lo;
        map_rec->msp = msp;
        msp->mr = map_rec;
        return msp;
        break;
    case attr_street_name:
        if (! item->map)
            break;
        if (!map_priv_is(item->map, map))
            break;
        map_rec = map_rect_new_binfile(map, NULL);
        town = map_rect_get_item_byid_binfile(map_rec, item->id_hi, item->id_lo);
        if (town) {
            struct coord c;

            if (binmap_search_by_index(map, town, &msp->mr))
                msp->mode = 1;
            else {
                map->last_searched_town_id_hi = town->id_hi;
                map->last_searched_town_id_lo = town->id_lo;
                if (item_coord_get(town, &c, 1)) {
                    if ((msp->mr=binmap_search_street_by_place(map, town, &c, &msp->ms, &msp->boundaries)))
                        msp->mode = 2;
                    else {
                        msp->mr=binmap_search_street_by_estimate(map, town, &c, &msp->ms);
                        msp->mode = 3;
                    }
                }
            }
            map_rect_destroy_binfile(map_rec);
            if (!msp->mr)
                break;
            return msp;
        }
        map_rect_destroy_binfile(map_rec);
        break;
    case attr_house_number:
        dbg(lvl_debug,"case house_number");
        if (! item->map)
            break;
        if (!map_priv_is(item->map, map))
            break;
        msp->map=map;
        msp->mr_item = map_rect_new_binfile(map, NULL);
        msp->item = map_rect_get_item_byid_binfile(msp->mr_item, item->id_hi, item->id_lo);
        idx=binmap_search_by_index(map, msp->item, &msp->mr);
        if (idx)
            msp->mode = 1;
        else {
            struct coord c;
            if (item_coord_get(msp->item, &c, 1)) {
                struct attr attr;
                map_rec = map_rect_new_binfile(map, NULL);
                town = map_rect_get_item_byid_binfile(map_rec, map->last_searched_town_id_hi, map->last_searched_town_id_lo);
                if (town)
                    msp->mr = binmap_search_street_by_place(map, town, &c, &msp->ms, &msp->boundaries);
                if (msp->boundaries)
                    dbg(lvl_debug, "using map town boundaries");
                if (!msp->boundaries && town) {
                    binmap_get_estimated_boundaries(town, &msp->boundaries);
                    if (msp->boundaries)
                        dbg(lvl_debug, "using estimated boundaries");
                }
                map_rect_destroy_binfile(map_rec);
                /* start searching in area around the street segment even if town boundaries are available */
                msp->mr=binmap_search_housenumber_by_estimate(map, &c, &msp->ms);
                msp->mode = 2;
                msp->rect_new=msp->ms.u.c_rect;
                if(item_attr_get(msp->item, attr_street_name, &attr))
                    msp->parent_name=g_strdup(attr.u.str);
                dbg(lvl_debug,"pn=%s",msp->parent_name);
            }
        }
        if (idx != 3) {
            map_rect_destroy_binfile(msp->mr_item);
            msp->mr_item=NULL;
        }
        if (!msp->mr) {
            break;
        }
        return msp;
    default:
        break;
    }
    if(ATTR_IS_STRING(msp->search.type))
        g_free(msp->search.u.str);
    g_free(msp);
    return NULL;
}


struct duplicate {
    struct coord c;
    char str[0];
};

static guint duplicate_hash(gconstpointer key) {
    const struct duplicate *d=key;
    return d->c.x^d->c.y^g_str_hash(d->str);
}

static gboolean duplicate_equal(gconstpointer a, gconstpointer b) {
    const struct duplicate *da=a;
    const struct duplicate *db=b;
    return (da->c.x == db->c.x && da->c.y == db->c.y && g_str_equal(da->str,db->str));
}

/**
 * @brief Test an item if it's duplicate. If it's not a duplicate, return new struct duplicate to be duplicate_insert()'ed.
 * @param msp pointer to private map search data
 * @param item item to check
 * @param attr_type
 * @param attr_type2, optional, allows to check for duplicate combinations of 2 attributes
 * returns - pointer to new struct duplicate, if this item is not already exist in hash
 *         - NULL if this item already exists in duplicate hash or doesnt have an attr_type attr;
 */
static struct duplicate* duplicate_test(struct map_search_priv *msp, struct item *item, enum attr_type attr_type,
                                        enum attr_type attr_type2) {
    struct attr attr;
    struct attr attr2;
    int len;
    char *buffer;
    struct duplicate *d;

    if (!msp->search_results)
        msp->search_results = g_hash_table_new_full(duplicate_hash, duplicate_equal, g_free, NULL);
    binfile_attr_rewind(item->priv_data);
    if (!item_attr_get(item, attr_type, &attr))
        return NULL;
    len=sizeof(struct  coord)+strlen(attr.u.str)+1;
    binfile_attr_rewind(item->priv_data);
    if (attr_type2 && item_attr_get(item, attr_type2, &attr2) && attr2.u.str) {
        len = len + strlen(attr2.u.str);
    }
    buffer=g_alloca(sizeof(char)*len);
    d=(struct duplicate *)buffer;
    if (!item_coord_get(item, &d->c, 1)) {
        d->c.x=0;
        d->c.y=0;
    }
    strcpy(d->str, attr.u.str);
    if(attr_type2 && attr2.u.str) {
        strcat(d->str,attr2.u.str);
    }
    binfile_coord_rewind(item->priv_data);
    binfile_attr_rewind(item->priv_data);
    if (!g_hash_table_lookup(msp->search_results, d)) {
        struct duplicate *dr=g_malloc(len);
        memcpy(dr, d, len);
        return dr;
    }
    return NULL;
}

/**
 * @brief Insert struct duplicate item into the duplicate hash.
 * @param msp pointer to private map search data
 * @param duplicate Duplicate info to insert
 */
static void duplicate_insert(struct map_search_priv *msp, struct duplicate *d) {
    g_hash_table_insert(msp->search_results, d, GINT_TO_POINTER(1));
}

/**
 * @brief Check if item is duplicate and update duplicate hash if needed.
 * @param msp pointer to private map search data
 * @param item item to check
 * @param attr_type
 * @param attr_type2, optional, allows to check for duplicate combinations of 2 attributes
 * @returns 0 if item is not a duplicate
 * 	    1 if item is duplicate or doesn't have required attr_type attribute
 */
static int duplicate(struct map_search_priv *msp, struct item *item, enum attr_type attr_type,
                     enum attr_type attr_type2) {
    struct duplicate *d=duplicate_test(msp, item, attr_type, attr_type2);
    if(!d)
        return 1;
    duplicate_insert(msp,d);
    return 0;
}

static int item_inside_poly_list(struct item *it, GList *l) {

    while(l) {
        struct geom_poly_segment *p=l->data;
        int count=p->last-p->first+1;
        struct coord c;
        int ccount;
        item_coord_rewind(it);
        ccount=binfile_coord_left(it->priv_data);
        if(ccount==1)
            item_coord_get(it,&c,1);
        else if(ccount==2) {
            struct coord c2;
            item_coord_get(it,&c,1);
            item_coord_get(it,&c2,1);
            c.x=(c.x+c2.x)/2;
            c.y=(c.y+c2.y)/2;
        } else {
            if(ccount>3)
                ccount/=2;
            else
                ccount=2;
            while(--ccount>0)
                item_coord_get(it,&c,1);
        }
        if(geom_poly_point_inside(p->first,count,&c))
            return 1;
        l=g_list_next(l);
    }
    return 0;
}

static struct item *binmap_search_get_item(struct map_search_priv *map_search) {
    struct item* it;
    struct attr at;
    enum linguistics_cmp_mode mode=(map_search->partial?linguistics_cmp_partial:0);

    for (;;) {
        while ((it  = map_rect_get_item_binfile(map_search->mr))) {
            int has_house_number=0;
            switch (map_search->search.type) {
            case attr_town_postal:
            case attr_town_name:
            case attr_district_name:
            case attr_town_or_district_name:
                if (map_search->mr->tile_depth > 1 && item_is_town(*it) && map_search->search.type == attr_town_postal) {
                    if (binfile_attr_get(it->priv_data, attr_town_postal, &at)) {
                        if (!linguistics_compare(at.u.str, map_search->search.u.str, mode)) {
                            /* check for duplicate combination of town_name and town_postal */
                            if (!duplicate(map_search, it, attr_town_name, attr_town_postal)) {
                                return it;
                            } else {
                                break;
                            }
                        }
                    }
                }
                if (map_search->mr->tile_depth > 1 && item_is_town(*it) && map_search->search.type != attr_district_name) {
                    if (binfile_attr_get(it->priv_data, attr_town_name_match, &at)
                            || binfile_attr_get(it->priv_data, attr_town_name, &at)) {
                        if (!linguistics_compare(at.u.str, map_search->search.u.str, mode) && !duplicate(map_search, it, attr_town_name,0))
                            return it;
                    }
                }
                if (map_search->mr->tile_depth > 1 && item_is_district(*it) && map_search->search.type != attr_town_name) {
                    if (binfile_attr_get(it->priv_data, attr_district_name_match, &at)
                            || binfile_attr_get(it->priv_data, attr_district_name, &at)) {
                        if (!linguistics_compare(at.u.str, map_search->search.u.str, mode) && !duplicate(map_search, it, attr_town_name,0))
                            return it;
                    }
                }
                break;
            case attr_street_name:
                if (map_search->mode == 1) {
                    if (binfile_attr_get(it->priv_data, attr_street_name_match, &at)
                            || binfile_attr_get(it->priv_data, attr_street_name, &at)) {
                        if (!linguistics_compare(at.u.str, map_search->search.u.str, mode) && !duplicate(map_search, it, attr_street_name,0)) {
                            return it;
                        }
                    }
                    continue;
                }
                if (item_is_street(*it)) {
                    struct attr at;
                    if (!map_selection_contains_item_rect(map_search->mr->sel, it))
                        break;

                    if(binfile_attr_get(it->priv_data, attr_label, &at)) {
                        struct coord c[128];
                        struct duplicate *d;

                        /* Extracting all coords here makes duplicate_new() not consider them (we don't want all
                         * street segments to be reported as separate streets). */
                        while(item_coord_get(it,c,128)>0);
                        d=duplicate_test(map_search, it, attr_label,0);
                        if(!d)
                            break;

                        if(linguistics_compare(at.u.str, map_search->search.u.str, mode|linguistics_cmp_expand|linguistics_cmp_words)) {
                            /* Remember this non-matching street name in duplicate hash to skip name
                             * comparison for its following segments */
                            duplicate_insert(map_search, d);
                            break;
                        }

                        if(map_search->boundaries && !item_inside_poly_list(it,map_search->boundaries)) {
                            /* Other segments may fit the town poly. Do not update hash for now. */
                            g_free(d);
                            break;
                        }

                        duplicate_insert(map_search, d);
                        item_coord_rewind(it);
                        return it;
                    }
                }
                break;
            case attr_house_number:
                has_house_number=binfile_attr_get(it->priv_data, attr_house_number, &at);
                if ((has_house_number
                        || it->type == type_house_number_interpolation_even || it->type == type_house_number_interpolation_odd
                        || it->type == type_house_number_interpolation_all
                        || (map_search->mode == 1 && item_is_street(*it))|| it->type == type_house_number)
                        && !(map_search->boundaries && !item_inside_poly_list(it,map_search->boundaries))) {
                    if (has_house_number) {
                        struct attr at2;
                        if ((binfile_attr_get(it->priv_data, attr_street_name, &at2) || map_search->mode!=2)
                                && !linguistics_compare(at.u.str, map_search->search.u.str, mode)
                                && !strcmp(at2.u.str, map_search->parent_name)) {
                            if (!duplicate(map_search, it, attr_house_number,0)) {
                                binfile_attr_rewind(it->priv_data);
                                return it;
                            }
                        }
                    } else {
                        struct attr at2;
                        if ((binfile_attr_get(it->priv_data, attr_street_name, &at2) || map_search->mode!=2)
                                && !strcmp(at2.u.str, map_search->parent_name)) {
                            if (!duplicate(map_search, it, attr_house_number_interpolation_no_ends_incrmt_2,0)) {
                                binfile_attr_rewind(it->priv_data);
                                return it;
                            } else if (!duplicate(map_search, it, attr_house_number_interpolation_no_ends_incrmt_1,0)) {
                                binfile_attr_rewind(it->priv_data);
                                return it;
                            }
                        } else {
                            if (!( it->type == type_house_number_interpolation_even || it->type == type_house_number_interpolation_odd
                                    || it->type == type_house_number_interpolation_all))
                                return it;
                        }

                    }
                } else if( item_is_street(*it) && map_search->mode==2 && map_search->parent_name
                           && binfile_attr_get(it->priv_data, attr_street_name, &at) && !strcmp(at.u.str, map_search->parent_name) ) {
                    /* If matching street segment found, prepare to expand house number search region +100m around each way point */
                    if (!(map_search->boundaries && !item_inside_poly_list(it,map_search->boundaries))) {
                        struct coord c;
                        while(item_coord_get(it,&c,1)) {
                            c.x-=100;
                            c.y-=100;
                            coord_rect_extend(&map_search->rect_new,&c);
                            c.x+=200;
                            c.y+=200;
                            coord_rect_extend(&map_search->rect_new,&c);
                        }
                    }
                }
                continue;
            default:
                return NULL;
            }
        }
        if(map_search->search.type==attr_house_number && map_search->mode==2 && map_search->parent_name) {
            /* For unindexed house number search, check if street segments extending possible housenumber locations were found */
            if(map_search->ms.u.c_rect.lu.x!=map_search->rect_new.lu.x || map_search->ms.u.c_rect.lu.y!=map_search->rect_new.lu.y ||
                    map_search->ms.u.c_rect.rl.x!=map_search->rect_new.rl.x || map_search->ms.u.c_rect.rl.y!=map_search->rect_new.rl.y) {
                map_search->ms.u.c_rect=map_search->rect_new;
                map_rect_destroy_binfile(map_search->mr);
                map_search->mr=map_rect_new_binfile(map_search->map, &map_search->ms);
                dbg(lvl_debug,"Extended house number search region to %d x %d, restarting...",
                    map_search->ms.u.c_rect.rl.x - map_search->ms.u.c_rect.lu.x, map_search->ms.u.c_rect.lu.y-map_search->ms.u.c_rect.rl.y);
                continue;
            }
        }
        if (!map_search->mr_item)
            return NULL;
        map_rect_destroy_binfile(map_search->mr);
        if (!binmap_search_by_index(map_search->map, map_search->item, &map_search->mr))
            return NULL;
    }
}


static void binmap_search_destroy(struct map_search_priv *ms) {
    if (ms->search_results)
        g_hash_table_destroy(ms->search_results);
    if(ATTR_IS_STRING(ms->search.type))
        g_free(ms->search.u.str);
    if(ms->parent_name)
        g_free(ms->parent_name);
    if (ms->mr_item)
        map_rect_destroy_binfile(ms->mr_item);
    if (ms->mr)
        map_rect_destroy_binfile(ms->mr);
    while(ms->boundaries) {
        geom_poly_segment_destroy(ms->boundaries->data);
        ms->boundaries=g_list_delete_link(ms->boundaries,ms->boundaries);
    }
    g_free(ms);
}

static int binmap_get_attr(struct map_priv *m, enum attr_type type, struct attr *attr) {
    attr->type=type;
    switch (type) {
    case attr_map_release:
        if (m->map_release) {
            attr->u.str=m->map_release;
            return 1;
        }
        break;
    case attr_progress:
        if (m->progress) {
            attr->u.str=m->progress;
            return 1;
        }
    default:
        break;
    }
    return 0;
}

static int binmap_set_attr(struct map_priv *map, struct attr *attr) {
    switch (attr->type) {
    case attr_update:
        map->download_enabled = attr->u.num;
        return 1;
    default:
        return 0;
    }

}

static struct map_methods map_methods_binfile = {
    projection_mg,
    "utf-8",
    map_destroy_binfile,
    map_rect_new_binfile,
    map_rect_destroy_binfile,
    map_rect_get_item_binfile,
    map_rect_get_item_byid_binfile,
    binmap_search_new,
    binmap_search_destroy,
    binmap_search_get_item,
    NULL,
    binmap_get_attr,
    binmap_set_attr,
};

static int binfile_get_index(struct map_priv *m) {
    int len;
    int cde_index_size;
    int offset;
    struct zip_cd *cd;

    len = strlen("index");
    cde_index_size = sizeof(struct zip_cd)+len;
    if (m->eoc64)
        offset = m->eoc64->zip64ecsz-cde_index_size;
    else
        offset = m->eoc->zipecsz-cde_index_size;
    cd = binfile_read_cd(m, offset, len);

    if (!cd) {
        cde_index_size+=sizeof(struct zip_cd_ext);
        if (m->eoc64)
            offset = m->eoc64->zip64ecsz-cde_index_size;
        else
            offset = m->eoc->zipecsz-cde_index_size;
        cd = binfile_read_cd(m, offset, len+sizeof(struct zip_cd_ext));
    }
    if (cd) {
        if (cd->zipcfnl == len && !strncmp(cd->zipcfn, "index", len)) {
            m->index_offset=offset;
            m->index_cd=cd;
            return 1;
        }
    }
    offset=binfile_search_cd(m, 0, "index", 0, 0);
    if (offset == -1)
        return 0;
    cd=binfile_read_cd(m, offset, -1);
    if (!cd)
        return 0;
    m->index_offset=offset;
    m->index_cd=cd;
    return 1;
}

static int map_binfile_zip_setup(struct map_priv *m, char *filename, int mmap) {
    struct zip_cd *first_cd;
    int i;
    if (!(m->eoc=binfile_read_eoc(m->fi))) {
        dbg(lvl_error,"map file %s: unable to read eoc", filename);
        return 0;
    }
    dbg_assert(m->eoc->zipedsk == m->eoc->zipecen);
    if (m->eoc->zipedsk && strlen(filename) > 3) {
        char *tmpfilename=g_strdup(filename),*ext=tmpfilename+strlen(tmpfilename)-3;
        m->fis=g_new(struct file *,m->eoc->zipedsk);
        for (i = 0 ; i < m->eoc->zipedsk-1 ; i++) {
            sprintf(ext,"b%02d",i+1);
            m->fis[i]=file_create(tmpfilename, 0);
            if (mmap)
                file_mmap(m->fis[i]);
        }
        m->fis[m->eoc->zipedsk-1]=m->fi;
        g_free(tmpfilename);
    }
    dbg(lvl_debug,"num_disk %d",m->eoc->zipedsk);
    m->eoc64=binfile_read_eoc64(m->fi);
    if (!binfile_get_index(m)) {
        dbg(lvl_error,"map file %s: no index found", filename);
        return 0;
    }
    if (!(first_cd=binfile_read_cd(m, 0, 0))) {
        dbg(lvl_error,"map file %s: unable to get first cd", filename);
        return 0;
    }
    m->cde_size=sizeof(struct zip_cd)+first_cd->zipcfnl+first_cd->zipcxtl;
    m->zip_members=m->index_offset/m->cde_size+1;
    dbg(lvl_debug,"cde_size %d", m->cde_size);
    dbg(lvl_debug,"members %d",m->zip_members);
    file_data_free(m->fi, (unsigned char *)first_cd);
    if (mmap)
        file_mmap(m->fi);
    return 1;
}


#if 0
static int map_binfile_download_initial(struct map_priv *m) {
    struct attr readwrite= {attr_readwrite,{(void *)1}};
    struct attr create= {attr_create,{(void *)1}};
    struct attr *attrs[4];
    struct file *out;
    long long woffset=0,planet_size;
    int size_ret;
    int cd1size,cdisize;
    long long cd1offset,cdioffset;
    struct zip64_eoc *zip64_eoc;
    struct zip64_eocl *zip64_eocl;
    struct zip_eoc *zip_eoc;
    struct zip_cd *cd1,*cdn,*cdi;
    int count,chunk,cdoffset=0;
    int mode=1;
    struct map_download *download=g_new0(struct map_download, 1);

    attrs[0]=&readwrite;
    attrs[1]=&create;
    attrs[2]=NULL;
    download->file=file_create(m->filename,attrs);
    download->m=m;
    download_planet_size(download);
    download_eoc(download);
    download_directory_start(download);
    while (download_directory_do(download));
    download_directory_finish(download);
    download_initial_finish(download);
    m->fi=download->file;
    g_free(download);
    return 1;


    cd1size=sizeof(*cd1);
    cd1offset=zip64_eoc->zip64eofst;
    cd1=(struct zip_cd *)map_binfile_download_range(m, cd1offset, cd1size);
    if (!cd1)
        return 0;
    cd1size=sizeof(*cd1)+binfile_cd_extra(cd1);
    g_free(cd1);
    cd1=(struct zip_cd *)map_binfile_download_range(m, cd1offset, cd1size);
    if (!cd1)
        return 0;
    cd1->zipcunc=0;
    cdisize=sizeof(*cdi)+strlen("index")+cd1->zipcxtl;
    cdioffset=zip64_eoc->zip64eofst+zip64_eoc->zip64ecsz-cdisize;
    cdi=(struct zip_cd *)map_binfile_download_range(m, cdioffset, cdisize);
    if (!cdi) {
        g_free(cd1);
        return 0;
    }
    cdi->zipcunc=0;
    cdn=g_malloc0(cd1size*256);

    file_data_write(out, woffset, sizeof(*zip64_eoc), (unsigned char *)zip64_eoc);
    woffset+=sizeof(*zip64_eoc);
    cdoffset=woffset;

    file_data_write(out, woffset, cd1size, (unsigned char *)cd1);
    woffset+=cd1size;
    count=(cdioffset-cd1offset)/cd1size-1;
    while (count > 0) {
        if (count > 256)
            chunk=256;
        else
            chunk=count;
        file_data_write(out, woffset, cd1size*chunk, (unsigned char *)cdn);
        woffset+=cd1size*chunk;
        count-=chunk;
    }
    g_free(cdn);
    g_free(cd1);
    file_data_write(out, woffset, cdisize, (unsigned char *)cdi);
    woffset+=cdisize;

}
#endif

static int map_binfile_open(struct map_priv *m) {
    int *magic;
    struct map_rect_priv *mr;
    struct item *item;
    struct attr attr;
    struct attr readwrite= {attr_readwrite, {(void *)1}};
    struct attr *attrs[]= {&readwrite, NULL};

    dbg(lvl_debug,"file_create %s", m->filename);
    m->fi=file_create(m->filename, m->url?attrs:NULL);
    if (! m->fi && m->url)
        return 0;
    if (! m->fi) {
        dbg(lvl_error,"Failed to load '%s'", m->filename);
        return 0;
    }
    if (m->check_version)
        m->version=file_version(m->fi, m->check_version);
    magic=(int *)file_data_read(m->fi, 0, 4);
    if (!magic) {
        file_destroy(m->fi);
        m->fi=NULL;
        return 0;
    }
    *magic = le32_to_cpu(*magic);
    if (*magic == zip_lfh_sig || *magic == zip_split_sig || *magic == zip_cd_sig || *magic == zip64_eoc_sig) {
        if (!map_binfile_zip_setup(m, m->filename, m->flags & 1)) {
            dbg(lvl_error,"invalid file format for '%s'", m->filename);
            file_destroy(m->fi);
            m->fi=NULL;
            return 0;
        }
    } else if (*magic == zip_lfh_sig_rev || *magic == zip_split_sig_rev || *magic == zip_cd_sig_rev
               || *magic == zip64_eoc_sig_rev) {
        dbg(lvl_error,"endianness mismatch for '%s'", m->filename);
        file_destroy(m->fi);
        m->fi=NULL;
        return 0;
    } else
        file_mmap(m->fi);
    file_data_free(m->fi, (unsigned char *)magic);
    m->cachedir=g_strdup("/tmp/navit");
    m->map_version=0;
    mr=map_rect_new_binfile(m, NULL);
    if (mr) {
        while ((item=map_rect_get_item_binfile(mr)) == &busy_item);
        if (item && item->type == type_map_information)  {
            if (binfile_attr_get(item->priv_data, attr_version, &attr))
                m->map_version=attr.u.num;
            if (binfile_attr_get(item->priv_data, attr_map_release, &attr))
                m->map_release=g_strdup(attr.u.str);
            if (m->url && binfile_attr_get(item->priv_data, attr_url, &attr)) {
                dbg(lvl_debug,"url config %s map %s",m->url,attr.u.str);
                if (strcmp(m->url, attr.u.str))
                    m->update_available=1;
                g_free(m->url);
                m->url=g_strdup(attr.u.str);
            }
        }
        map_rect_destroy_binfile(mr);
        if (m->map_version >= 16) {
            dbg(lvl_error,"%s: This map is incompatible with your navit version. Please update navit. (map version %d)",
                m->filename, m->map_version);
            return 0;
        }
    }
    return 1;
}

static void map_binfile_close(struct map_priv *m) {
    int i;
    file_data_free(m->fi, (unsigned char *)m->index_cd);
    file_data_free(m->fi, (unsigned char *)m->eoc);
    file_data_free(m->fi, (unsigned char *)m->eoc64);
    g_free(m->cachedir);
    g_free(m->map_release);
    if (m->fis) {
        for (i = 0 ; i < m->eoc->zipedsk ; i++) {
            file_destroy(m->fis[i]);
        }
    } else
        file_destroy(m->fi);
}

static void map_binfile_destroy(struct map_priv *m) {
    g_free(m->filename);
    g_free(m->url);
    g_free(m->progress);
    g_free(m);
}


static void binfile_check_version(struct map_priv *m) {
    int version=-1;
    if (!m->check_version)
        return;
    if (m->fi)
        version=file_version(m->fi, m->check_version);
    if (version != m->version) {
        if (m->fi)
            map_binfile_close(m);
        map_binfile_open(m);
    }
}


static struct map_priv *map_new_binfile(struct map_methods *meth, struct attr **attrs, struct callback_list *cbl) {
    struct map_priv *m;
    struct attr *data=attr_search(attrs, NULL, attr_data);
    struct attr *check_version,*flags,*url,*download_enabled;
    struct file_wordexp *wexp;
    char **wexp_data;
    if (! data)
        return NULL;

    wexp=file_wordexp_new(data->u.str);
    wexp_data=file_wordexp_get_array(wexp);
    dbg(lvl_debug,"map_new_binfile %s", data->u.str);
    *meth=map_methods_binfile;

    m=g_new0(struct map_priv, 1);
    m->cbl=cbl;
    m->id=++map_id;
    m->filename=g_strdup(wexp_data[0]);
    file_wordexp_destroy(wexp);
    check_version=attr_search(attrs, NULL, attr_check_version);
    if (check_version)
        m->check_version=check_version->u.num;
    flags=attr_search(attrs, NULL, attr_flags);
    if (flags)
        m->flags=flags->u.num;
    url=attr_search(attrs, NULL, attr_url);
    if (url)
        m->url=g_strdup(url->u.str);
    download_enabled = attr_search(attrs, NULL, attr_update);
    if (download_enabled)
        m->download_enabled=download_enabled->u.num;

    if (!map_binfile_open(m) && !m->check_version && !m->url) {
        map_binfile_destroy(m);
        m=NULL;
    } else {
        load_changes(m);
    }
    return m;
}

void plugin_init(void) {
    dbg(lvl_debug,"binfile: plugin_init");
    if (sizeof(struct zip_cd) != 46) {
        dbg(lvl_error,"error: sizeof(struct zip_cd)=%zu",sizeof(struct zip_cd));
    }
    plugin_register_category_map("binfile", map_new_binfile);
}

