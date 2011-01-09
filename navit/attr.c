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
#include <string.h>
#include <stdio.h>
#include <glib.h>
#include "debug.h"
#include "item.h"
#include "coord.h"
#include "transform.h"
#include "color.h"
#include "attr.h"
#include "map.h"
#include "config.h"
#include "endianess.h"
#include "util.h"
#include "types.h"

struct attr_name {
	enum attr_type attr;
	char *name;
};


static struct attr_name attr_names[]={
#define ATTR2(x,y) ATTR(y)
#define ATTR(x) { attr_##x, #x },
#include "attr_def.h"
#undef ATTR2
#undef ATTR
};

enum attr_type
attr_from_name(const char *name)
{
	int i;

	for (i=0 ; i < sizeof(attr_names)/sizeof(struct attr_name) ; i++) {
		if (! strcmp(attr_names[i].name, name))
			return attr_names[i].attr;
	}
	return attr_none;
}


static int attr_match(enum attr_type search, enum attr_type found);



char *
attr_to_name(enum attr_type attr)
{
	int i;

	for (i=0 ; i < sizeof(attr_names)/sizeof(struct attr_name) ; i++) {
		if (attr_names[i].attr == attr)
			return attr_names[i].name;
	}
	return NULL; 
}

struct attr *
attr_new_from_text(const char *name, const char *value)
{
	enum attr_type attr;
	struct attr *ret;
	struct coord_geo *g;
	struct coord c;
	char *pos,*type_str,*str,*tok;
	int min,max,count;

	ret=g_new0(struct attr, 1);
	dbg(1,"enter name='%s' value='%s'\n", name, value);
	attr=attr_from_name(name);
	ret->type=attr;
	switch (attr) {
	case attr_item_type:
		ret->u.item_type=item_from_name(value);
		break;
	case attr_item_types:
		count=0;
		type_str=g_strdup(value);
		str=type_str;
		while ((tok=strtok(str, ","))) {
			ret->u.item_types=g_realloc(ret->u.item_types, (count+2)*sizeof(enum item_type));
			ret->u.item_types[count++]=item_from_name(tok);
			ret->u.item_types[count]=type_none;
        	        str=NULL;
        	}
		g_free(type_str);
		break;
	case attr_attr_types:
		count=0;
		type_str=g_strdup(value);
		str=type_str;
		while ((tok=strtok(str, ","))) {
			ret->u.attr_types=g_realloc(ret->u.attr_types, (count+2)*sizeof(enum attr_type));
			ret->u.attr_types[count++]=attr_from_name(tok);
			ret->u.attr_types[count]=attr_none;
        	        str=NULL;
        	}
		g_free(type_str);
		break;
	case attr_dash:
		count=0;
		type_str=g_strdup(value);
		str=type_str;
		while ((tok=strtok(str, ","))) {
			ret->u.dash=g_realloc(ret->u.dash, (count+2)*sizeof(int));
			ret->u.dash[count++]=g_ascii_strtoull(tok,NULL,0);
			ret->u.dash[count]=0;
        	        str=NULL;
        	}
		g_free(type_str);
		break;
	case attr_order:
	case attr_sequence_range:
	case attr_angle_range:
	case attr_speed_range:
		pos=strchr(value, '-');
		min=0;
		max=32767;
		if (! pos) {
                	sscanf(value,"%d",&min);
                	max=min;
		} else if (pos == value)
			sscanf(value,"-%d",&max);
		else
                	sscanf(value,"%d-%d",&min, &max);
		ret->u.range.min=min;
		ret->u.range.max=max;
		break;
	default:
		if (attr >= attr_type_string_begin && attr <= attr_type_string_end) {
			ret->u.str=g_strdup(value);
			break;
		}
		if (attr >= attr_type_int_begin && attr <= attr_type_int_end) {
			if (value[0] == '0' && value[1] == 'x')
				ret->u.num=strtoul(value, NULL, 0);
			else
				ret->u.num=strtol(value, NULL, 0);
			
			if ((attr >= attr_type_rel_abs_begin) && (attr < attr_type_boolean_begin)) {
				/* Absolute values are from -0x40000000 - 0x40000000, with 0x0 being 0 (who would have thought that?)
					 Relative values are from 0x40000001 - 0x80000000, with 0x60000000 being 0 */
				if (strchr(value, '%')) {
					if ((ret->u.num > 0x20000000) || (ret->u.num < -0x1FFFFFFF)) {
						dbg(0, "Relative possibly-relative attribute with invalid value %i\n", ret->u.num);
					}

					ret->u.num += 0x60000000;
				} else {
					if ((ret->u.num > 0x40000000) || (ret->u.num < -0x40000000)) {
						dbg(0, "Non-relative possibly-relative attribute with invalid value %i\n", ret->u.num);
					}
				}
			} else 	if (attr >= attr_type_boolean_begin) { // also check for yes and no
				if (g_ascii_strcasecmp(value,"no") && g_ascii_strcasecmp(value,"0") && g_ascii_strcasecmp(value,"false")) 
					ret->u.num=1;
				else
					ret->u.num=0;
			}
			break;
		}
		if (attr >= attr_type_color_begin && attr <= attr_type_color_end) {
			struct color *color=g_new0(struct color, 1);
			int r,g,b,a;
			ret->u.color=color;
			if(strlen(value)==7){
				sscanf(value,"#%02x%02x%02x", &r, &g, &b);
				color->r = (r << 8) | r;
				color->g = (g << 8) | g;
				color->b = (b << 8) | b;
				color->a = (65535);
			} else if(strlen(value)==9){
				sscanf(value,"#%02x%02x%02x%02x", &r, &g, &b, &a);
				color->r = (r << 8) | r;
				color->g = (g << 8) | g;
				color->b = (b << 8) | b;
				color->a = (a << 8) | a;
			} else {
				dbg(0,"color %s has unknown format\n",value);
			}
			break;
		}
		if (attr >= attr_type_coord_geo_begin && attr <= attr_type_coord_geo_end) {
			g=g_new(struct coord_geo, 1);
			ret->u.coord_geo=g;
			coord_parse(value, projection_mg, &c);
			transform_to_geo(projection_mg, &c, g);
			break;
		}
		dbg(1,"default\n");
		g_free(ret);
		ret=NULL;
	}
	return ret;
}

static char *
flags_to_text(int flags)
{
	char *ret=NULL;
	if (flags & AF_ONEWAY) ret=g_strconcat_printf(ret,"%sAF_ONEWAY",ret?"|":"");
	if (flags & AF_ONEWAYREV) ret=g_strconcat_printf(ret,"%sAF_ONEWAYREV",ret?"|":"");
	if (flags & AF_SEGMENTED) ret=g_strconcat_printf(ret,"%sAF_SEGMENTED",ret?"|":"");
	if (flags & AF_ROUNDABOUT) ret=g_strconcat_printf(ret,"%sAF_ROUNDABOUT",ret?"|":"");
	if (flags & AF_ROUNDABOUT_VALID) ret=g_strconcat_printf(ret,"%sAF_ROUNDABOUT_VALID",ret?"|":"");
	if (flags & AF_ONEWAY_EXCEPTION) ret=g_strconcat_printf(ret,"%sAF_ONEWAY_EXCEPTION",ret?"|":"");
	if (flags & AF_SPEED_LIMIT) ret=g_strconcat_printf(ret,"%sAF_SPEED_LIMIT",ret?"|":"");
	if (flags & AF_RESERVED1) ret=g_strconcat_printf(ret,"%sAF_RESERVED1",ret?"|":"");
	if (flags & AF_SIZE_OR_WEIGHT_LIMIT) ret=g_strconcat_printf(ret,"%sAF_SIZE_OR_WEIGHT_LIMIT",ret?"|":"");
	if (flags & AF_THROUGH_TRAFFIC_LIMIT) ret=g_strconcat_printf(ret,"%sAF_THROUGH_TRAFFIC_LIMIT",ret?"|":"");
	if (flags & AF_TOLL) ret=g_strconcat_printf(ret,"%sAF_TOLL",ret?"|":"");
	if (flags & AF_SEASONAL) ret=g_strconcat_printf(ret,"%sAF_SEASONAL",ret?"|":"");
	if (flags & AF_UNPAVED) ret=g_strconcat_printf(ret,"%sAF_UNPAVED",ret?"|":"");
	if (flags & AF_DANGEROUS_GOODS) ret=g_strconcat_printf(ret,"%sAF_DANGEROUS_GOODS",ret?"|":"");
	if ((flags & AF_ALL) == AF_ALL) 
		return g_strconcat_printf(ret,"%sAF_ALL",ret?"|":"");
	if ((flags & AF_ALL) == AF_MOTORIZED_FAST) 
		return g_strconcat_printf(ret,"%sAF_MOTORIZED_FAST",ret?"|":"");
	if (flags & AF_EMERGENCY_VEHICLES) ret=g_strconcat_printf(ret,"%sAF_EMERGENCY_VEHICLES",ret?"|":"");
	if (flags & AF_TRANSPORT_TRUCK) ret=g_strconcat_printf(ret,"%sAF_TRANSPORT_TRUCK",ret?"|":"");
	if (flags & AF_DELIVERY_TRUCK) ret=g_strconcat_printf(ret,"%sAF_DELIVERY_TRUCK",ret?"|":"");
	if (flags & AF_PUBLIC_BUS) ret=g_strconcat_printf(ret,"%sAF_PUBLIC_BUS",ret?"|":"");
	if (flags & AF_TAXI) ret=g_strconcat_printf(ret,"%sAF_TAXI",ret?"|":"");
	if (flags & AF_HIGH_OCCUPANCY_CAR) ret=g_strconcat_printf(ret,"%sAF_HIGH_OCCUPANCY_CAR",ret?"|":"");
	if (flags & AF_CAR) ret=g_strconcat_printf(ret,"%sAF_CAR",ret?"|":"");
	if (flags & AF_MOTORCYCLE) ret=g_strconcat_printf(ret,"%sAF_MOTORCYCLE",ret?"|":"");
	if (flags & AF_MOPED) ret=g_strconcat_printf(ret,"%sAF_MOPED",ret?"|":"");
	if (flags & AF_HORSE) ret=g_strconcat_printf(ret,"%sAF_HORSE",ret?"|":"");
	if (flags & AF_BIKE) ret=g_strconcat_printf(ret,"%sAF_BIKE",ret?"|":"");
	if (flags & AF_PEDESTRIAN) ret=g_strconcat_printf(ret,"%sAF_PEDESTRIAN",ret?"|":"");
	return ret;
}

char *
attr_to_text(struct attr *attr, struct map *map, int pretty)
{
	char *ret;
	enum attr_type type=attr->type;

	if (type >= attr_type_item_begin && type <= attr_type_item_end) {
		struct item *item=attr->u.item;
		struct attr type, data;
		if (! item)
			return g_strdup("(nil)");
		if (! item->map || !map_get_attr(item->map, attr_type, &type, NULL))
			type.u.str="";
		if (! item->map || !map_get_attr(item->map, attr_data, &data, NULL))
			data.u.str="";
		return g_strdup_printf("type=0x%x id=0x%x,0x%x map=%p (%s:%s)", item->type, item->id_hi, item->id_lo, item->map, type.u.str, data.u.str);
	}
	if (type >= attr_type_string_begin && type <= attr_type_string_end) {
		if (map) {
			char *mstr;
			if (attr->u.str) {
				mstr=map_convert_string(map, attr->u.str);
				ret=g_strdup(mstr);
				map_convert_free(mstr);
			} else
				ret=g_strdup("(null)");
			
		} else
			ret=g_strdup(attr->u.str);
		return ret;
	}
	if (type == attr_flags || type == attr_through_traffic_flags)
		return flags_to_text(attr->u.num);
	if (type >= attr_type_int_begin && type <= attr_type_int_end) 
		return g_strdup_printf("%d", attr->u.num);
	if (type >= attr_type_int64_begin && type <= attr_type_int64_end) 
		return g_strdup_printf(LONGLONG_FMT, *attr->u.num64);
	if (type >= attr_type_double_begin && type <= attr_type_double_end) 
		return g_strdup_printf("%f", *attr->u.numd);
	if (type >= attr_type_object_begin && type <= attr_type_object_end) 
		return g_strdup_printf("(object[%s])", attr_to_name(type));
	if (type >= attr_type_color_begin && type <= attr_type_color_end) {
		if (attr->u.color->a != 65535) 
			return g_strdup_printf("#%02x%02x%02x%02x", attr->u.color->r>>8,attr->u.color->g>>8,attr->u.color->b>>8, attr->u.color->a>>8);
		else
			return g_strdup_printf("#%02x%02x%02x", attr->u.color->r>>8,attr->u.color->g>>8,attr->u.color->b>>8);
	}
	if (type >= attr_type_coord_geo_begin && type <= attr_type_coord_geo_end) 
		return g_strdup_printf("%f %f",attr->u.coord_geo->lng,attr->u.coord_geo->lat);
	if (type == attr_zipfile_ref_block) {
		int *data=attr->u.data;
		return g_strdup_printf("0x%x,0x%x,0x%x",data[0],data[1],data[2]);
	}
	if (type == attr_item_id) {
		int *data=attr->u.data;
		return g_strdup_printf("0x%x,0x%x",data[0],data[1]);
	}
	if (type >= attr_type_group_begin && type <= attr_type_group_end) {
		int i=0;
		char *ret=g_strdup("");
		char *sep="";
		while (attr->u.attrs[i].type) {
			char *val=attr_to_text(&attr->u.attrs[i], map, pretty);
			ret=g_strconcat_printf(ret,"%s%s=%s",sep,attr_to_name(attr->u.attrs[i].type),val);
			g_free(val);
			sep=" ";
			i++;
		}
		return ret;
	}
	if (type >= attr_type_item_type_begin && type <= attr_type_item_type_end) {
		return g_strdup_printf("0x%x[%s]",attr->u.num,item_to_name(attr->u.num));
	}
	return g_strdup_printf("(no text[%s])", attr_to_name(type));	
}

struct attr *
attr_search(struct attr **attrs, struct attr *last, enum attr_type attr)
{
	dbg(1, "enter attrs=%p\n", attrs);
	while (*attrs) {
		dbg(1,"*attrs=%p\n", *attrs);
		if ((*attrs)->type == attr) {
			return *attrs;
		}
		attrs++;
	}
	return NULL;
}

static int
attr_match(enum attr_type search, enum attr_type found)
{
	switch (search) {
	case attr_any:
		return 1;
	case attr_any_xml:
		switch (found) {
		case attr_callback:
			return 0;
		default:
			return 1;
		}
	default:
		return search == found;
	}
}

int
attr_generic_get_attr(struct attr **attrs, struct attr **def_attrs, enum attr_type type, struct attr *attr, struct attr_iter *iter)
{
	while (attrs && *attrs) {
		if (attr_match(type,(*attrs)->type)) {
			*attr=**attrs;
			if (!iter)
				return 1;
			if (*((void **)iter) < (void *)attrs) {
				*((void **)iter)=(void *)attrs;
				return 1;
			}
		}
		attrs++;
	}
	if (type == attr_any || type == attr_any_xml)
		return 0;
	while (def_attrs && *def_attrs) {
		if ((*def_attrs)->type == type) {
			*attr=**def_attrs;
			return 1;
		}
		def_attrs++;
	}
	return 0;
}

struct attr **
attr_generic_set_attr(struct attr **attrs, struct attr *attr)
{
	struct attr **curr=attrs;
	int i,count=0;
	while (curr && *curr) {
		if ((*curr)->type == attr->type) {
			attr_free(*curr);
			*curr=attr_dup(attr);
			return attrs;
		}
		curr++;
		count++;
	}
	curr=g_new0(struct attr *, count+2);
	for (i = 0 ; i < count ; i++)
		curr[i]=attrs[i];
	curr[count]=attr_dup(attr);
	curr[count+1]=NULL;
	g_free(attrs);
	return curr;
}

struct attr **
attr_generic_add_attr(struct attr **attrs, struct attr *attr)
{
	struct attr **curr=attrs;
	int i,count=0;
	while (curr && *curr) {
		curr++;
		count++;
	}
	curr=g_new0(struct attr *, count+2);
	for (i = 0 ; i < count ; i++)
		curr[i]=attrs[i];
	curr[count]=attr_dup(attr);
	curr[count+1]=NULL;
	g_free(attrs);
	return curr;
}

struct attr **
attr_generic_remove_attr(struct attr **attrs, struct attr *attr)
{
	struct attr **curr=attrs;
	int i,j,count=0,found=0;
	while (curr && *curr) {
		if ((*curr)->type == attr->type && (*curr)->u.data == attr->u.data)
			found=1;
		curr++;
		count++;
	}
	if (!found)
		return attrs;
	curr=g_new0(struct attr *, count);
	j=0;
	for (i = 0 ; i < count ; i++) {
		if (attrs[i]->type != attr->type || attrs[i]->u.data != attr->u.data)
			curr[j++]=attrs[i];
		else
			attr_free(attrs[i]);
	}
	curr[j]=NULL;
	g_free(attrs);
	return curr;
}

enum attr_type
attr_type_begin(enum attr_type type)
{
	if (type < attr_type_item_begin)
		return attr_none;
	if (type < attr_type_int_begin)
		return attr_type_item_begin;
	if (type < attr_type_string_begin)
		return attr_type_int_begin;
	if (type < attr_type_special_begin)
		return attr_type_string_begin;
	if (type < attr_type_double_begin)
		return attr_type_special_begin;
	if (type < attr_type_coord_geo_begin)
		return attr_type_double_begin;
	if (type < attr_type_color_begin)
		return attr_type_coord_geo_begin;
	if (type < attr_type_object_begin)
		return attr_type_color_begin;
	if (type < attr_type_coord_begin)
		return attr_type_object_begin;
	if (type < attr_type_pcoord_begin)
		return attr_type_coord_begin;
	if (type < attr_type_callback_begin)
		return attr_type_pcoord_begin;
	if (type < attr_type_int64_begin)
		return attr_type_callback_begin;
	if (type <= attr_type_int64_end)
		return attr_type_int64_begin;
	return attr_none;
}

int
attr_data_size(struct attr *attr)
{
	if (attr->type >= attr_type_string_begin && attr->type <= attr_type_string_end) 
		return strlen(attr->u.str)+1;
	if (attr->type >= attr_type_int_begin && attr->type <= attr_type_int_end) 
		return sizeof(attr->u.num);
	if (attr->type >= attr_type_coord_geo_begin && attr->type <= attr_type_coord_geo_end) 
		return sizeof(*attr->u.coord_geo);
	if (attr->type >= attr_type_color_begin && attr->type <= attr_type_color_end) 
		return sizeof(*attr->u.color);
	if (attr->type >= attr_type_object_begin && attr->type <= attr_type_object_end) 
		return sizeof(void *);
	if (attr->type >= attr_type_int64_begin && attr->type <= attr_type_int64_end) 
		return sizeof(*attr->u.num64);
	if (attr->type == attr_order)
		return sizeof(attr->u.range);
	if (attr->type >= attr_type_double_begin && attr->type <= attr_type_double_end) 
		return sizeof(*attr->u.numd);
	if (attr->type == attr_item_types) {
		int i=0;
		while (attr->u.item_types[i++] != type_none);
		return i*sizeof(enum item_type);
	}
	if (attr->type >= attr_type_item_type_begin && attr->type <= attr_type_item_type_end)
		return sizeof(enum item_type);
	if (attr->type == attr_attr_types) {
		int i=0;
		while (attr->u.attr_types[i++] != attr_none);
		return i*sizeof(enum attr_type);
	}
	dbg(0,"size for %s unknown\n", attr_to_name(attr->type));
	return 0;
}

void *
attr_data_get(struct attr *attr)
{
	if ((attr->type >= attr_type_int_begin && attr->type <= attr_type_int_end) || 
	    (attr->type >= attr_type_item_type_begin && attr->type <= attr_type_item_type_end))
		return &attr->u.num;
	if (attr->type == attr_order)
		return &attr->u.range;
	return attr->u.data;
}

void
attr_data_set(struct attr *attr, void *data)
{
	if ((attr->type >= attr_type_int_begin && attr->type <= attr_type_int_end) || 
	    (attr->type >= attr_type_item_type_begin && attr->type <= attr_type_item_type_end))
		attr->u.num=*((int *)data);
	else
		attr->u.data=data;
}

void
attr_data_set_le(struct attr * attr, void * data)
{
	if ((attr->type >= attr_type_int_begin && attr->type <= attr_type_int_end) || 
	    (attr->type >= attr_type_item_type_begin && attr->type <= attr_type_item_type_end))
		attr->u.num=le32_to_cpu(*((int *)data));
	else if (attr->type == attr_order) {
		attr->u.num=le32_to_cpu(*((int *)data));
		attr->u.range.min=le16_to_cpu(attr->u.range.min);
		attr->u.range.max=le16_to_cpu(attr->u.range.max);
	}
	else
/* Fixme: Handle long long */
		attr->u.data=data;

}

void
attr_free(struct attr *attr)
{
	if (!attr)
		return;
	if (!(attr->type >= attr_type_int_begin && attr->type <= attr_type_int_end) && 
	    !(attr->type >= attr_type_object_begin && attr->type <= attr_type_object_end))
		g_free(attr->u.data);
	g_free(attr);
}

void
attr_dup_content(struct attr *src, struct attr *dst)
{
	int size;
	dst->type=src->type;
	if (src->type >= attr_type_int_begin && src->type <= attr_type_int_end) 
		dst->u.num=src->u.num;
	else if (src->type >= attr_type_object_begin && src->type <= attr_type_object_end) 
		dst->u.data=src->u.data;
	else {
		size=attr_data_size(src);
		if (size) {
			dst->u.data=g_malloc(size);
			memcpy(dst->u.data, src->u.data, size);
		}
	}
}

struct attr *
attr_dup(struct attr *attr)
{
	struct attr *ret=g_new0(struct attr, 1);
	attr_dup_content(attr, ret);
	return ret;
}

void
attr_list_free(struct attr **attrs)
{
	int count=0;
	while (attrs && attrs[count]) {
		attr_free(attrs[count++]);
	}
	g_free(attrs);
}

struct attr **
attr_list_dup(struct attr **attrs)
{
	struct attr **ret=attrs;
	int i,count=0;

	while (attrs[count])
		count++;
	ret=g_new0(struct attr *, count+1);
	for (i = 0 ; i < count ; i++)
		ret[i]=attr_dup(attrs[i]);
	return ret;
}


int
attr_from_line(char *line, char *name, int *pos, char *val_ret, char *name_ret)
{
	int len=0,quoted;
	char *p,*e,*n;

	dbg(1,"get_tag %s from %s\n", name, line); 
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
		e=strchr(p,'=');
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
			strncpy(val_ret, e, len);
			val_ret[len]='\0';
			if (pos)
				*pos=p-line;
			return 1;
		}
	}	
	return 0;
}

/**
 * Check if an enumeration of attribute types contains a specific attribute.
 *
 * @param types Pointer to the attr_type enumeration to be searched
 * @param type The attr_type to be searched for
 *
 * @return 1 if the attribute type was found, 0 if it was not found or if a null pointer was passed as types
 */
int
attr_types_contains(enum attr_type *types, enum attr_type type)
{
	while (types && *types != attr_none) {
		if (*types == type)
			return 1;
		types++;
	}
	return 0;
}

/**
 * Check if an enumeration of attribute types contains a specific attribute. 
 * It is different from attr_types_contains in that it returns a caller-defined value if the pointer to the enumeration is NULL.
 *
 * @param types Pointer to the attr_type enumeration to be searched
 * @param type The attr_type to be searched for
 * @param deflt The default value to return if types is NULL.
 *
 * @return 1 if the attribute type was found, 0 if it was not found, the value of the deflt argument if types is NULL.
 */
int
attr_types_contains_default(enum attr_type *types, enum attr_type type, int deflt)
{
	if (!types) {
		return deflt;
	}
	return attr_types_contains(types, type);	
}
