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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "debug.h"
#include "plugin.h"
#include "projection.h"
#include "item.h"
#include "map.h"
#include "maptype.h"
#include "attr.h"
#include "transform.h"
#include "file.h"
#include "shapefil.h"


#define IS_ARC(x) ((x).nSHPType == SHPT_ARC || (x).nSHPType == SHPT_ARCZ || (x).nSHPType == SHPT_ARCM)
#define IS_POLYGON(x) ((x).nSHPType == SHPT_POLYGON || (x).nSHPType == SHPT_POLYGONZ || (x).nSHPType == SHPT_POLYGONM)

struct map_priv {
    int id;
    char *filename;
    char *charset;
    SHPHandle hSHP;
    DBFHandle hDBF;
    int nShapeType, nEntities, nFields;
    double adfMinBound[4], adfMaxBound[4];
    struct longest_match *lm;
    char *dbfmap_data;
    struct coord offset;
    enum projection pro;
    int flags;
};


struct map_rect_priv {
    struct map_selection *sel;
    struct map_priv *m;
    struct item item;
    int idx;
    int cidx,cidx_rewind;
    int part,part_rewind;
    int aidx;
    enum attr_type anext;
    SHPObject *psShape;
    char *str;
    char *line;
    int attr_pos;
    struct attr *attr;
};

static void map_destroy_shapefile(struct map_priv *m) {
    dbg(lvl_debug,"map_destroy_shapefile");
    g_free(m);
}

static void shapefile_coord_rewind(void *priv_data) {
    struct map_rect_priv *mr=priv_data;
    mr->cidx=mr->cidx_rewind;
    mr->part=mr->part_rewind;
}

static void shapefile_coord(struct map_rect_priv *mr, int idx, struct coord *c) {
    SHPObject *psShape=mr->psShape;
    struct coord cs;
    struct coord_geo g;

    if (!mr->m->pro) {
        g.lng=psShape->padfX[idx]+mr->m->offset.x;
        g.lat=psShape->padfY[idx]+mr->m->offset.y;
        transform_from_geo(projection_mg, &g, c);
    } else {
        cs.x=psShape->padfX[idx]+mr->m->offset.x;
        cs.y=psShape->padfY[idx]+mr->m->offset.y;
        transform_from_to(&cs, mr->m->pro, c, projection_mg);
    }
}

static int shapefile_coord_get(void *priv_data, struct coord *c, int count) {
    struct map_rect_priv *mr=priv_data;
    int ret=0;
    int idx;

    SHPObject *psShape=mr->psShape;
    while (count) {
        idx=mr->cidx;
        if (idx >= psShape->nVertices)
            break;
        if (mr->part+1 < psShape->nParts && idx == psShape->panPartStart[mr->part+1]) {
            if (IS_POLYGON(*psShape)) {
                mr->part++;
                shapefile_coord(mr, 0, c);
            } else if (IS_ARC(*psShape)) {
                break;
            } else {
                dbg_assert("Neither POLYGON or ARC and has parts" == NULL);
            }
        } else {
            shapefile_coord(mr, idx, c);
            mr->cidx++;
        }
        ret++;
        c++;
        count--;
    }
    return ret;
}

static void shapefile_attr_rewind(void *priv_data) {
    struct map_rect_priv *mr=priv_data;
    mr->aidx=0;
    mr->attr_pos=0;
    if (mr->m->flags & 1)
        mr->anext=attr_none;
    else
        mr->anext=attr_debug;
}


struct longest_match_list_item {
    void *data;
    int match_idx_count;
    int *match_idx;
};

struct longest_match_list {
    GList *longest_match_list_items;
};


struct longest_match {
    GHashTable *match_hash;
    char *match_present;
    int match_present_count;
    GList *longest_match_lists;
    int longest_match_list_count;
};

static void longest_match_add_match(struct longest_match *lm, struct longest_match_list_item *lmi, char *match) {
    int idx;
    if (!(idx=(int)(long)g_hash_table_lookup(lm->match_hash, match))) {
        idx=lm->match_present_count++;
        lm->match_present=g_renew(char, lm->match_present, lm->match_present_count);
        g_hash_table_insert(lm->match_hash, g_strdup(match), (gpointer)(long)idx);
    }
    lmi->match_idx=g_renew(int, lmi->match_idx, lmi->match_idx_count+1);
    lmi->match_idx[lmi->match_idx_count++]=idx;
}

static void longest_match_item_destroy(struct longest_match_list_item *lmi, gpointer p_flags) {
    long flags = (long) p_flags;
    if (!lmi)
        return;
    if (flags & 2) {
        g_free(lmi->data);
    }
    g_free(lmi->match_idx);
    g_free(lmi);
}

static struct longest_match_list_item *longest_match_item_new(struct longest_match_list *lml, void *data) {
    struct longest_match_list_item *ret=g_new0(struct longest_match_list_item,1);
    lml->longest_match_list_items=g_list_append(lml->longest_match_list_items, ret);
    ret->data=data;

    return ret;
}

static struct longest_match_list *longest_match_list_new(struct longest_match *lm) {
    struct longest_match_list *ret=g_new0(struct longest_match_list,1);
    lm->longest_match_lists=g_list_append(lm->longest_match_lists, ret);
    return ret;
}

static void longest_match_list_destroy(struct longest_match_list *lml, gpointer p_flags) {
    long flags = (long) p_flags;
    if (!lml)
        return;
    if (flags & 1) {
        g_list_foreach(lml->longest_match_list_items, (GFunc)longest_match_item_destroy, (gpointer)flags);
        g_list_free(lml->longest_match_list_items);
    }
    g_free(lml);
}

static struct longest_match_list *longest_match_get_list(struct longest_match *lm, int list) {
    GList *l=lm->longest_match_lists;
    while (l && list > 0) {
        l=g_list_next(l);
        list++;
    }
    if (l)
        return l->data;
    return NULL;
}

static struct longest_match *longest_match_new(void) {
    struct longest_match *ret=g_new0(struct longest_match,1);
    ret->match_hash=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    ret->match_present_count=1;
    return ret;
}

static void longest_match_destroy(struct longest_match *lm, long flags) {
    if (!lm)
        return;
    if (flags & 1) {
        g_list_foreach(lm->longest_match_lists, (GFunc)longest_match_list_destroy, (gpointer)flags);
        g_list_free(lm->longest_match_lists);
    }
    g_hash_table_destroy(lm->match_hash);
    g_free(lm->match_present);
    g_free(lm);
}

static void longest_match_clear(struct longest_match *lm) {
    if (lm->match_present)
        memset(lm->match_present, 0, lm->match_present_count);
}

static void longest_match_add_key_value(struct longest_match *lm, char *k, char *v) {
    char* buffer=g_alloca(strlen(k)+strlen(v)+2);
    int idx;

    strcpy(buffer,"*=*");
    if ((idx=(int)(long)g_hash_table_lookup(lm->match_hash, buffer)))
        lm->match_present[idx]=1;

    sprintf(buffer,"%s=*", k);
    if ((idx=(int)(long)g_hash_table_lookup(lm->match_hash, buffer)))
        lm->match_present[idx]=2;

    sprintf(buffer,"*=%s", v);
    if ((idx=(int)(long)g_hash_table_lookup(lm->match_hash, buffer)))
        lm->match_present[idx]=2;

    sprintf(buffer,"%s=%s", k, v);
    if ((idx=(int)(long)g_hash_table_lookup(lm->match_hash, buffer)))
        lm->match_present[idx]=4;
}

static int longest_match_list_find(struct longest_match *lm, struct longest_match_list *lml, void **list,
                                   int max_list_len) {
    int j,longest=0,ret=0,sum,val;
    struct longest_match_list_item *curr;
    GList *l=lml->longest_match_list_items;


    while (l) {
        sum=0;
        curr=l->data;
        for (j = 0 ; j < curr->match_idx_count ; j++) {
            val=lm->match_present[curr->match_idx[j]];
            if (val)
                sum+=val;
            else {
                sum=-1;
                break;
            }
        }
        if (sum > longest) {
            longest=sum;
            ret=0;
        }
        if (sum > 0 && sum == longest && ret < max_list_len)
            list[ret++]=curr->data;
        l=g_list_next(l);
    }
    return ret;
}


static void build_match(struct longest_match *lm, struct longest_match_list *lml, char *line) {
    struct longest_match_list_item *lmli;
    char *kvl=NULL,*i=NULL,*p,*kv;
    dbg(lvl_debug,"line=%s",line);
    kvl=line;
    p=strchr(line,'\t');
    if (p) {
        while (*p == '\t')
            *p++='\0';
        i=p;
    }
    lmli=longest_match_item_new(lml,g_strdup(i));
    while ((kv=strtok(kvl, ","))) {
        kvl=NULL;
        longest_match_add_match(lm, lmli, kv);
    }
}

static void build_matches(struct map_priv *m,char *matches) {
    char *matches2=g_strdup(matches);
    char *l=matches2,*p;
    struct longest_match_list *lml;

    m->lm=longest_match_new();
    lml=longest_match_list_new(m->lm);
    while (l) {
        p=strchr(l,'\n');
        if (p)
            *p++='\0';
        if (strlen(l))
            build_match(m->lm,lml,l);
        l=p;
    }
    g_free(matches2);
}

static void process_fields(struct map_priv *m, int id) {
    int i;
    char szTitle[12],*str;
    int nWidth, nDecimals;

    for (i = 0 ; i < m->nFields ; i++) {

        switch (DBFGetFieldInfo(m->hDBF, i, szTitle, &nWidth, &nDecimals )) {
        case FTString:
            str=g_strdup(DBFReadStringAttribute( m->hDBF, id, i ));
            break;
        case FTInteger:
            str=g_strdup_printf("%d",DBFReadIntegerAttribute( m->hDBF, id, i ));
            break;
        case FTDouble:
            str=g_strdup_printf("%lf",DBFReadDoubleAttribute( m->hDBF, id, i ));
            break;
        case FTInvalid:
            str=NULL;
            break;
        default:
            str=NULL;
        }
        longest_match_add_key_value(m->lm, szTitle, str);
    }
}

static int attr_resolve(struct map_rect_priv *mr, enum attr_type attr_type, struct attr *attr) {
    char name[1024];
    char value[1024];
    char szTitle[12];
    const char *str;
    char *col,*type=NULL;
    int i,len, nWidth, nDecimals;
    if (!mr->line)
        return 0;
    if (attr_type != attr_any)
        type=attr_to_name(attr_type);
    if (attr_from_line(mr->line,type,&mr->attr_pos,value,name)) {
        len=strlen(value);
        if (value[0] == '$' && value[1] == '{' && value[len-1] == '}') {
            int found=0;
            value[len-1]='\0';
            col=value+2;
            for (i = 0 ; i < mr->m->nFields ; i++) {
                if (DBFGetFieldInfo(mr->m->hDBF, i, szTitle, &nWidth, &nDecimals ) == FTString && !strcmp(szTitle,col)) {
                    str=DBFReadStringAttribute( mr->m->hDBF, mr->item.id_hi, i);
                    strcpy(value,str);
                    found=1;
                    break;
                }
            }
            if (!found)
                value[0]='\0';
        }
        if (!value[0])
            return -1;
        dbg(lvl_debug,"name=%s value=%s",name,value);
        attr_free(mr->attr);
        mr->attr=attr_new_from_text(name,value);
        if (mr->attr) {
            *attr=*mr->attr;
            return 1;
        }
        return -1;
    }
    return 0;
}

static int shapefile_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr) {
    struct map_rect_priv *mr=priv_data;
    struct map_priv *m=mr->m;
    char szTitle[12],*pszTypeName, *str;
    int code, ret, nWidth, nDecimals;

    attr->type=attr_type;
    switch (attr_type) {
    case attr_any:
        while ((code=attr_resolve(mr, attr_type, attr))) {
            if (code == 1)
                return 1;
        }
        while (mr->anext != attr_none) {
            ret=shapefile_attr_get(priv_data, mr->anext, attr);
            if (ret)
                return ret;
        }
        return 0;
    case attr_debug:
        if (mr->aidx >= m->nFields) {
            mr->anext=attr_none;
            return 0;
        }
        switch (DBFGetFieldInfo(m->hDBF, mr->aidx, szTitle, &nWidth, &nDecimals )) {
        case FTString:
            pszTypeName = "String";
            str=g_strdup(DBFReadStringAttribute( m->hDBF, mr->item.id_hi, mr->aidx ));
            break;
        case FTInteger:
            pszTypeName = "Integer";
            str=g_strdup_printf("%d",DBFReadIntegerAttribute( m->hDBF, mr->item.id_hi, mr->aidx ));
            break;
        case FTDouble:
            pszTypeName = "Double";
            str=g_strdup_printf("%lf",DBFReadDoubleAttribute( m->hDBF, mr->item.id_hi, mr->aidx ));
            break;
        case FTInvalid:
            pszTypeName = "Invalid";
            str=NULL;
            break;
        default:
            pszTypeName = "Unknown";
            str=NULL;
        }
        g_free(mr->str);
        mr->str=attr->u.str=g_strdup_printf("%s=%s(%s)",szTitle,str,pszTypeName);
        g_free(str);
        mr->aidx++;
        return 1;
    default:
        return (attr_resolve(mr, attr_type, attr) == 1);
    }
}

static struct item_methods methods_shapefile = {
    shapefile_coord_rewind,
    shapefile_coord_get,
    shapefile_attr_rewind,
    shapefile_attr_get,
};

static struct map_rect_priv *map_rect_new_shapefile(struct map_priv *map, struct map_selection *sel) {
    struct map_rect_priv *mr;
    char *dbfmapfile=g_strdup_printf("%s.dbfmap", map->filename);
    void *data;
    struct file *file;
    int size;
    int changed=0;
    if ((file=file_create(dbfmapfile, 0))) {
        size=file_size(file);
        data=file_data_read_all(file);
        if (data) {
            if (!map->dbfmap_data || size != strlen(map->dbfmap_data) || strncmp(data,map->dbfmap_data,size)) {
                g_free(map->dbfmap_data);
                map->dbfmap_data=g_malloc(size+1);
                memcpy(map->dbfmap_data, data, size);
                map->dbfmap_data[size]='\0';
                changed=1;
            }
            file_data_free(file, data);
        }
        file_destroy(file);
    } else {
        dbg(lvl_error,"Failed to open %s",dbfmapfile);
        if (map->dbfmap_data) {
            changed=1;
            g_free(map->dbfmap_data);
            map->dbfmap_data=NULL;
        }
    }
    dbg(lvl_debug,"%s changed %d old %p",dbfmapfile,changed,map->dbfmap_data);
    if (changed) {
        longest_match_destroy(map->lm,1);
        map->lm=NULL;
        if (map->dbfmap_data) {
            build_matches(map,map->dbfmap_data);
        }
    }
    dbg(lvl_debug,"map_rect_new_shapefile");
    mr=g_new0(struct map_rect_priv, 1);
    mr->m=map;
    mr->idx=0;
    mr->item.id_lo=0;
    mr->item.id_hi=0;
    mr->item.meth=&methods_shapefile;
    mr->item.priv_data=mr;
    g_free(dbfmapfile);
    return mr;
}


static void map_rect_destroy_shapefile(struct map_rect_priv *mr) {
    if (mr->psShape)
        SHPDestroyObject(mr->psShape);
    attr_free(mr->attr);
    g_free(mr->str);
    g_free(mr);
}

static struct item *map_rect_get_item_shapefile(struct map_rect_priv *mr) {
    struct map_priv *m=mr->m;
    void *lines[5];
    struct longest_match_list *lml;
    int count;
    char type[1024];

    if (mr->psShape && IS_ARC(*mr->psShape) && mr->part+1 < mr->psShape->nParts) {
        mr->part++;
        mr->part_rewind=mr->part;
        mr->cidx_rewind=mr->psShape->panPartStart[mr->part];
    } else {
        if (mr->idx >= m->nEntities)
            return NULL;
        mr->item.id_hi=mr->idx;
        if (mr->psShape)
            SHPDestroyObject(mr->psShape);
        mr->psShape=SHPReadObject(m->hSHP, mr->idx);
        if (mr->psShape->nVertices > 1)
            mr->item.type=type_street_unkn;
        else
            mr->item.type=type_point_unkn;
        if (m->lm) {
            longest_match_clear(m->lm);
            process_fields(m, mr->idx);

            lml=longest_match_get_list(m->lm, 0);
            count=longest_match_list_find(m->lm, lml, lines, sizeof(lines)/sizeof(void *));
            if (count) {
                mr->line=lines[0];
                if (attr_from_line(mr->line,"type",NULL,type,NULL)) {
                    dbg(lvl_debug,"type='%s'", type);
                    mr->item.type=item_from_name(type);
                    if (mr->item.type == type_none && strcmp(type,"none"))
                        dbg(lvl_error,"Warning: type '%s' unknown", type);
                } else {
                    dbg(lvl_debug,"failed to get attribute type");
                }
            } else
                mr->line=NULL;
        }
        mr->idx++;
        mr->part_rewind=0;
        mr->cidx_rewind=0;
    }
    shapefile_coord_rewind(mr);
    shapefile_attr_rewind(mr);
    return &mr->item;
}

static struct item *map_rect_get_item_byid_shapefile(struct map_rect_priv *mr, int id_hi, int id_lo) {
    mr->idx=id_hi;
    while (id_lo--) {
        if (!map_rect_get_item_shapefile(mr))
            return NULL;
    }
    return map_rect_get_item_shapefile(mr);
}

static struct map_methods map_methods_shapefile = {
    projection_mg,
    "iso8859-1",
    map_destroy_shapefile,
    map_rect_new_shapefile,
    map_rect_destroy_shapefile,
    map_rect_get_item_shapefile,
    map_rect_get_item_byid_shapefile,
};

static struct map_priv *map_new_shapefile(struct map_methods *meth, struct attr **attrs, struct callback_list *cbl) {
    struct map_priv *m;
    struct attr *data=attr_search(attrs, NULL, attr_data);
    struct attr *charset=attr_search(attrs, NULL, attr_charset);
    struct attr *projectionname=attr_search(attrs, NULL, attr_projectionname);
    struct attr *flags=attr_search(attrs, NULL, attr_flags);
    struct file_wordexp *wexp;
    char *wdata;
    char **wexp_data;
    char *shapefile,*dbffile;
    if (! data)
        return NULL;
    dbg(lvl_debug,"map_new_shapefile %s", data->u.str);
    wdata=g_strdup(data->u.str);
    wexp=file_wordexp_new(wdata);
    wexp_data=file_wordexp_get_array(wexp);
    *meth=map_methods_shapefile;

    m=g_new0(struct map_priv, 1);
    m->filename=g_strdup(wexp_data[0]);
    shapefile=g_strdup_printf("%s.shp", m->filename);
    m->hSHP=SHPOpen(shapefile, "rb" );
    SHPGetInfo( m->hSHP, &m->nEntities, &m->nShapeType, m->adfMinBound, m->adfMaxBound );
    g_free(shapefile);
    dbffile=g_strdup_printf("%s.dbf", m->filename);
    m->hDBF=DBFOpen(dbffile, "rb");
    m->nFields=DBFGetFieldCount(m->hDBF);
    g_free(dbffile);
    dbg(lvl_debug,"map_new_shapefile %s %s", m->filename, wdata);
    if (charset) {
        m->charset=g_strdup(charset->u.str);
        meth->charset=m->charset;
    }
    if (projectionname)
        m->pro=projection_from_name(projectionname->u.str, &m->offset);
    if (flags)
        m->flags=flags->u.num;
    file_wordexp_destroy(wexp);
    return m;
}

void plugin_init(void) {
    dbg(lvl_debug,"shapefile: plugin_init");
    plugin_register_category_map("shapefile", map_new_shapefile);
}

/************************************************************************/
/*                            VSI_SHP_Open()                            */
/************************************************************************/

static SAFile VSI_SHP_Open( const char *pszFilename, const char *pszAccess )

{
    return (SAFile)fopen(pszFilename, pszAccess);
}

/************************************************************************/
/*                            VSI_SHP_Read()                            */
/************************************************************************/

static SAOffset VSI_SHP_Read( void *p, SAOffset size, SAOffset nmemb, SAFile file )

{
    return fread(p, size, nmemb, (FILE *)file);
}

/************************************************************************/
/*                           VSI_SHP_Write()                            */
/************************************************************************/

static SAOffset VSI_SHP_Write( void *p, SAOffset size, SAOffset nmemb, SAFile file )

{
    return fwrite(p, size, nmemb, (FILE *)file);
}

/************************************************************************/
/*                            VSI_SHP_Seek()                            */
/************************************************************************/

static SAOffset VSI_SHP_Seek( SAFile file, SAOffset offset, int whence )

{
    return fseek((FILE *)file, offset, whence);
}

/************************************************************************/
/*                            VSI_SHP_Tell()                            */
/************************************************************************/

static SAOffset VSI_SHP_Tell( SAFile file )

{
    return ftell((FILE *)file);
}


static int VSI_SHP_Flush( SAFile file )

{
    return fflush((FILE *)file);
}

/************************************************************************/
/*                           VSI_SHP_Close()                            */
/************************************************************************/

static int VSI_SHP_Close( SAFile file )

{
    return fclose((FILE *)file);
}

/************************************************************************/
/*                              SADError()                              */
/************************************************************************/

static void VSI_SHP_Error( const char *message )

{
    dbg(lvl_error,"error:%s", message);
}

/************************************************************************/
/*                           VSI_SHP_Remove()                           */
/************************************************************************/

static int VSI_SHP_Remove( const char *pszFilename )

{
    return unlink(pszFilename);
}

/************************************************************************/
/*                        SASetupDefaultHooks()                         */
/************************************************************************/

void SASetupDefaultHooks( SAHooks *psHooks )

{
    psHooks->FOpen   = VSI_SHP_Open;
    psHooks->FRead   = VSI_SHP_Read;
    psHooks->FWrite  = VSI_SHP_Write;
    psHooks->FSeek   = VSI_SHP_Seek;
    psHooks->FTell   = VSI_SHP_Tell;
    psHooks->FFlush  = VSI_SHP_Flush;
    psHooks->FClose  = VSI_SHP_Close;

    psHooks->Remove  = VSI_SHP_Remove;
    psHooks->Atof    = atof;

    psHooks->Error   = VSI_SHP_Error;
}


