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

#include <glib.h>
#include <stdlib.h>
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
#include "transform.h"
#include "file.h"
#include "shapefil.h"


struct map_priv {
	int id;
	char *filename;
	char *charset;
	SHPHandle hSHP;
	DBFHandle hDBF;
	int nShapeType, nEntities, nFields;
	double adfMinBound[4], adfMaxBound[4];
};


struct map_rect_priv {
	struct map_selection *sel;
	struct map_priv *m;
	struct item item;
	int idx;
	int cidx;
	int aidx;
	enum attr_type anext;
	SHPObject *psShape;
	char *str;
};

static void
map_destroy_shapefile(struct map_priv *m)
{
	dbg(1,"map_destroy_shapefile\n");
	g_free(m);
}

static void
shapefile_coord_rewind(void *priv_data)
{
	struct map_rect_priv *mr=priv_data;
	mr->cidx=0;
}

static int
shapefile_coord_get(void *priv_data, struct coord *c, int count)
{
	struct map_rect_priv *mr=priv_data;
	int ret=0;
	int idx;
	struct coord_geo g;
	SHPObject *psShape=mr->psShape;
	while (count) {
		idx=mr->cidx;
		if (idx >= psShape->nVertices)
			break;
		g.lng=psShape->padfX[idx];
		g.lat=psShape->padfY[idx];
		transform_from_geo(projection_mg, &g, c);
		mr->cidx++;
		ret++;
		c++;
		count--;
	}
	return ret;
}

static void
shapefile_attr_rewind(void *priv_data)
{
	struct map_rect_priv *mr=priv_data;
	mr->aidx=0;
	mr->anext=attr_debug;
}

static int
shapefile_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr)
{	
	struct map_rect_priv *mr=priv_data;
	struct map_priv *m=mr->m;
	char szTitle[12],*pszTypeName,*str;
	int ret, nWidth, nDecimals;

	attr->type=attr_type;
	switch (attr_type) {
	case attr_any:
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
			str=g_strdup(DBFReadStringAttribute( m->hDBF, mr->item.id_lo, mr->aidx ));
			break;
		case FTInteger:
			pszTypeName = "Integer";
			str=g_strdup_printf("%d",DBFReadIntegerAttribute( m->hDBF, mr->item.id_lo, mr->aidx ));
			break;
		case FTDouble:
			pszTypeName = "Double";
			str=g_strdup_printf("%lf",DBFReadDoubleAttribute( m->hDBF, mr->item.id_lo, mr->aidx ));
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
		return 0;
	}
}

static struct item_methods methods_shapefile = {
        shapefile_coord_rewind,
        shapefile_coord_get,
        shapefile_attr_rewind,
        shapefile_attr_get,
};

static struct map_rect_priv *
map_rect_new_shapefile(struct map_priv *map, struct map_selection *sel)
{
	struct map_rect_priv *mr;

	dbg(1,"map_rect_new_shapefile\n");
	mr=g_new0(struct map_rect_priv, 1);
	mr->m=map;
	mr->idx=0;
	mr->item.id_lo=0;
	mr->item.id_hi=0;
	mr->item.meth=&methods_shapefile;
	mr->item.priv_data=mr;
	return mr;
}


static void
map_rect_destroy_shapefile(struct map_rect_priv *mr)
{
	if (mr->psShape)
		SHPDestroyObject(mr->psShape);
	g_free(mr->str);
        g_free(mr);
}

static struct item *
map_rect_get_item_shapefile(struct map_rect_priv *mr)
{
	struct map_priv *m=mr->m;
	if (mr->idx >= m->nEntities)
		return NULL;
	mr->item.type=type_street_unkn;
	mr->item.id_lo=mr->idx;
	if (mr->psShape)
		SHPDestroyObject(mr->psShape);
	mr->psShape=SHPReadObject(m->hSHP, mr->idx);
	mr->idx++;
	shapefile_coord_rewind(mr);
	shapefile_attr_rewind(mr);
	return &mr->item;
}

static struct item *
map_rect_get_item_byid_shapefile(struct map_rect_priv *mr, int id_hi, int id_lo)
{
	mr->idx=id_lo;
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

static struct map_priv *
map_new_shapefile(struct map_methods *meth, struct attr **attrs)
{
	struct map_priv *m;
	struct attr *data=attr_search(attrs, NULL, attr_data);
	struct attr *charset=attr_search(attrs, NULL, attr_charset);
	struct file_wordexp *wexp;
	char *wdata;
	char **wexp_data;
	char *shapefile,*dbffile;
	if (! data)
		return NULL;
	dbg(1,"map_new_shapefile %s\n", data->u.str);	
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
	dbg(1,"map_new_shapefile %s %s\n", m->filename, wdata);
	if (charset) {
		m->charset=g_strdup(charset->u.str);
		meth->charset=m->charset;
	}
	file_wordexp_destroy(wexp);
	return m;
}

void
plugin_init(void)
{
	dbg(1,"shapefile: plugin_init\n");
	plugin_register_map_type("shapefile", map_new_shapefile);
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
	dbg(0,"error:%s\n", message);
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


