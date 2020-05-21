/*
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

/** @file attr.c
 * @brief Attribute handling code
 *
 * Structures and functions for working with attributes.
 *
 * @author Navit Team
 * @date 2005-2014
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
#include "navigation.h"
#include "attr.h"
#include "map.h"
#include "config.h"
#include "endianess.h"
#include "util.h"
#include "types.h"
#include "xmlconfig.h"
#include "layout.h"

struct attr_name {
    enum attr_type attr;
    char *name;
};


/** List of attr_types with their names as strings. */
static struct attr_name attr_names[]= {
#define ATTR2(x,y) ATTR(y)
#define ATTR(x) { attr_##x, #x },

#define ATTR_UNUSED /* Unused attr_types not needed here.*/

#include "attr_def.h"

#undef ATTR_UNUSED

#undef ATTR2
#undef ATTR
};

static GHashTable *attr_hash;

void attr_create_hash(void) {
    int i;
    attr_hash=g_hash_table_new(g_str_hash, g_str_equal);
    for (i=0 ; i < sizeof(attr_names)/sizeof(struct attr_name) ; i++) {
        g_hash_table_insert(attr_hash, attr_names[i].name, GINT_TO_POINTER(attr_names[i].attr));
    }
}

void attr_destroy_hash(void) {
    g_hash_table_destroy(attr_hash);
    attr_hash=NULL;
}

/**
 * @brief Converts a string to an attr_type
 *
 * This function reads a string and returns the corresponding attr_type.
 *
 * @param name The attribute name
 * @return The corresponding {@code attr_type}, or {@code attr_none} if the string specifies a nonexistent or invalid attribute type.
 */
enum attr_type attr_from_name(const char *name) {
    int i;

    if (attr_hash)
        return GPOINTER_TO_INT(g_hash_table_lookup(attr_hash, name));
    for (i=0 ; i < sizeof(attr_names)/sizeof(struct attr_name) ; i++) {
        if (! strcmp(attr_names[i].name, name))
            return attr_names[i].attr;
    }
    return attr_none;
}


static int attr_match(enum attr_type search, enum attr_type found);



/**
 * @brief Converts an attr_type to a string
 *
 * @param attr The attribute type to be converted.
 * @return The attribute name, or NULL if an invalid value was passed as {@code attr}.
 * The calling function should create a copy of the string if it needs to alter it or relies on the
 * string being available permanently.
 */
char *attr_to_name(enum attr_type attr) {
    int i;

    for (i=0 ; i < sizeof(attr_names)/sizeof(struct attr_name) ; i++) {
        if (attr_names[i].attr == attr)
            return attr_names[i].name;
    }
    return NULL;
}

/**
 * @brief Creates an attribute from text information
 *
 * This function creates an attribute from two strings specifying the name and
 * the value.
 *
 * @param name The name of the new attribute
 * @param value The value of the new attribute
 * @return The new attribute
 */
struct attr *
attr_new_from_text(const char *name, const char *value) {
    enum attr_type attr;
    struct attr *ret;
    struct coord_geo *g;
    struct coord c;
    enum item_type item_type;
    char *pos,*type_str,*str,*tok;
    int min,max,count;

    ret=g_new0(struct attr, 1);
    dbg(lvl_debug,"enter name='%s' value='%s'", name, value);
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
            item_type=item_from_name(tok);
            if (item_type!=type_none) {
                ret->u.item_types[count++]=item_type;
                ret->u.item_types[count]=type_none;
            } else {
                dbg(lvl_error,"Unknown item type '%s' ignored.",tok);
            }
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
        if (attr >= attr_type_int_begin && attr < attr_type_rel_abs_begin) {
            char *tail;
            if (value[0] == '0' && value[1] == 'x')
                ret->u.num=strtoul(value, &tail, 0);
            else
                ret->u.num=strtol(value, &tail, 0);
            if (*tail) {
                dbg(lvl_error, "Incorrect value '%s' for attribute '%s';  expected a number. "
                    "Defaulting to 0.\n", value, name);
                ret->u.num=0;
            }
            break;
        }
        if (attr >= attr_type_rel_abs_begin && attr < attr_type_boolean_begin) {
            char *tail;
            int value_is_relative=0;
            ret->u.num=strtol(value, &tail, 0);
            if (*tail) {
                if (!strcmp(tail, "%")) {
                    value_is_relative=1;
                } else {
                    dbg(lvl_error, "Incorrect value '%s' for attribute '%s';  expected a number or a relative value in percent. "
                        "Defaulting to 0.\n", value, name);
                    ret->u.num=0;
                }
            }
            if (value_is_relative) {
                if ((ret->u.num > ATTR_REL_MAXREL) || (ret->u.num < ATTR_REL_MINREL)) {
                    dbg(lvl_error, "Relative possibly-relative attribute with value out of range: %li", ret->u.num);
                }

                ret->u.num += ATTR_REL_RELSHIFT;
            } else {
                if ((ret->u.num > ATTR_REL_MAXABS) || (ret->u.num < ATTR_REL_MINABS)) {
                    dbg(lvl_error, "Non-relative possibly-relative attribute with value out of range: %li", ret->u.num);
                }
            }
            break;
        }
        if (attr >= attr_type_boolean_begin && attr <=  attr_type_int_end) {
            if (!(g_ascii_strcasecmp(value,"no") && g_ascii_strcasecmp(value,"0") && g_ascii_strcasecmp(value,"false")))
                ret->u.num=0;
            else if (!(g_ascii_strcasecmp(value,"yes") && g_ascii_strcasecmp(value,"1") && g_ascii_strcasecmp(value,"true")))
                ret->u.num=1;
            else {
                dbg(lvl_error, "Incorrect value '%s' for attribute '%s';  expected a boolean (no/0/false or yes/1/true). "
                    "Defaulting to 'true'.\n", value, name);
                ret->u.num=1;
            }
            break;
        }
        if (attr >= attr_type_color_begin && attr <= attr_type_color_end) {
            struct color *color=g_new0(struct color, 1);
            int r,g,b,a;
            ret->u.color=color;
            if(strlen(value)==7) {
                sscanf(value,"#%02x%02x%02x", &r, &g, &b);
                color->r = (r << 8) | r;
                color->g = (g << 8) | g;
                color->b = (b << 8) | b;
                color->a = (65535);
            } else if(strlen(value)==9) {
                sscanf(value,"#%02x%02x%02x%02x", &r, &g, &b, &a);
                color->r = (r << 8) | r;
                color->g = (g << 8) | g;
                color->b = (b << 8) | b;
                color->a = (a << 8) | a;
            } else {
                dbg(lvl_error,"color %s has unknown format",value);
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
        dbg(lvl_debug,"unknown attribute");
        g_free(ret);
        ret=NULL;
    }
    return ret;
}

/**
 * @brief Converts access flags to a human-readable string.
 *
 * @param flags The flags as a number
 * @return The flags in human-readable form
 */
static char *flags_to_text(int flags) {
    char *ret=g_strdup_printf("0x%x:",flags);
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
    if (flags & AF_FORD) ret=g_strconcat_printf(ret,"%sAF_FORD",ret?"|":"");
    if (flags & AF_UNDERGROUND) ret=g_strconcat_printf(ret,"%sAF_UNDERGROUND",ret?"|":"");
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

/**
 * @brief Converts attribute data to human-readable text
 *
 * @param attr The attribute to be formatted
 * @param sep The separator to insert between a numeric value and its unit. If NULL, nothing will be inserted.
 * @param fmt Set to {@code attr_format_with_units} if {@code attr} is of type {@code attr_destination_length}
 * or {@code attr_destination_time} or a group type which might contain attributes of those types. Ignored for
 * all other attribute types.
 * @param def_fmt Not used
 * @param map The translation map. This is only needed for string type attributes or group type
 * attributes which might contain strings. It can be NULL for other attribute types. If a string
 * type attribute is encountered and {@code map} is NULL, the string will be returned unchanged.
 *
 * @return The attribute data in human-readable form. The caller is responsible for calling {@code g_free()} on
 * the result when it is no longer needed.
 */
char *attr_to_text_ext(struct attr *attr, char *sep, enum attr_format fmt, enum attr_format def_fmt, struct map *map) {
    char *ret;
    enum attr_type type=attr->type;

    if (!sep)
        sep="";

    if (type >= attr_type_item_begin && type <= attr_type_item_end) {
        struct item *item=attr->u.item;
        struct attr type, data;
        if (! item)
            return g_strdup("(nil)");
        if (! item->map || !map_get_attr(item->map, attr_type, &type, NULL))
            type.u.str="";
        if (! item->map || !map_get_attr(item->map, attr_data, &data, NULL))
            data.u.str="";
        return g_strdup_printf("type=0x%x id=0x%x,0x%x map=%p (%s:%s)", item->type, item->id_hi, item->id_lo, item->map,
                               type.u.str, data.u.str);
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
    if (type == attr_destination_length) {
        if (fmt == attr_format_with_units) {
            double distance=attr->u.num;
            if (distance > 10000)
                return g_strdup_printf("%.0f%skm",distance/1000,sep);
            return g_strdup_printf("%.1f%skm",distance/1000,sep);
        }
    }
    if (type == attr_destination_time) {
        if (fmt == attr_format_with_units) {
            int seconds=(attr->u.num+5)/10;
            int minutes=seconds/60;
            int hours=minutes/60;
            int days=hours/24;
            hours%=24;
            minutes%=60;
            seconds%=60;
            if (days)
                return g_strdup_printf("%d:%02d:%02d:%02d",days,hours,minutes,seconds);
            if (hours)
                return g_strdup_printf("%02d:%02d:%02d",hours,minutes,seconds);
            return g_strdup_printf("%02d:%02d",minutes,seconds);
        }
    }
    if (type >= attr_type_int_begin && type <= attr_type_int_end)
        return g_strdup_printf("%ld", attr->u.num);
    if (type >= attr_type_int64_begin && type <= attr_type_int64_end)
        return g_strdup_printf(LONGLONG_FMT, *attr->u.num64);
    if (type >= attr_type_double_begin && type <= attr_type_double_end)
        return g_strdup_printf("%f", *attr->u.numd);
    if (type >= attr_type_object_begin && type <= attr_type_object_end)
        return g_strdup_printf("(object[%s])", attr_to_name(type));
    if (type >= attr_type_color_begin && type <= attr_type_color_end) {
        if (attr->u.color->a != 65535)
            return g_strdup_printf("#%02x%02x%02x%02x", attr->u.color->r>>8,attr->u.color->g>>8,attr->u.color->b>>8,
                                   attr->u.color->a>>8);
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
    if (type == attr_item_types) {
        enum item_type *item_types=attr->u.item_types;
        char *sep="",*ret=NULL;
        while (item_types && *item_types) {
            ret=g_strconcat_printf(ret,"%s%s",sep,item_to_name(*item_types++));
            sep=",";
        }
        return ret;
    }
    if (type >= attr_type_group_begin && type <= attr_type_group_end) {
        int i=0;
        char *ret=g_strdup("");
        char *sep="";
        while (attr->u.attrs[i].type) {
            char *val=attr_to_text_ext(&attr->u.attrs[i], sep, fmt, def_fmt, map);
            ret=g_strconcat_printf(ret,"%s%s=%s",sep,attr_to_name(attr->u.attrs[i].type),val);
            g_free(val);
            sep=" ";
            i++;
        }
        return ret;
    }
    if (type >= attr_type_item_type_begin && type <= attr_type_item_type_end) {
        return g_strdup_printf("0x%ld[%s]",attr->u.num,item_to_name(attr->u.num));
    }
    if (type == attr_nav_status) {
        return nav_status_to_text(attr->u.num);
    }
    if (type == attr_poly_hole) {
        return g_strdup_printf("count=%d", attr->u.poly_hole->coord_count);
    }
    return g_strdup_printf("(no text[%s])", attr_to_name(type));
}

/**
 * @brief Converts an attribute to a string that can be displayed
 *
 * This function is just a wrapper around {@code attr_to_text_ext()}.
 *
 * @param attr The attribute to convert
 * @param map The translation map, see {@code attr_to_text_ext()}
 * @param pretty Not used
 */
char *attr_to_text(struct attr *attr, struct map *map, int pretty) {
    return attr_to_text_ext(attr, NULL, attr_format_default, attr_format_default, map);
}

/**
 * @brief Searches for an attribute of a given type
 *
 * This function searches an array of pointers to attributes for a given
 * attribute type and returns the first match.
 *
 * @param attrs Points to the array of attribute pointers to be searched
 * @param last Not used
 * @param attr_type The attribute type to search for. Generic types (such as
 * attr_any or attr_any_xml) are NOT supported.
 * @return Pointer to the first matching attribute, or NULL if no match was found.
 */
struct attr *
attr_search(struct attr **attrs, struct attr *last, enum attr_type attr) {
    dbg(lvl_info, "enter attrs=%p", attrs);
    while (*attrs) {
        dbg(lvl_debug,"*attrs=%p", *attrs);
        if ((*attrs)->type == attr) {
            return *attrs;
        }
        attrs++;
    }
    return NULL;
}

static int attr_match(enum attr_type search, enum attr_type found) {
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

/**
 * @brief Generic get function
 *
 * This function searches an attribute list for an attribute of a given type and
 * stores it in the attr parameter. If no match is found in attrs and the
 * def_attrs argument is supplied, def_attrs is searched for the attribute type
 * and the first match (if any) is returned.
 * <p>
 * Searching for attr_any or attr_any_xml is supported, but def_attrs will not
 * be searched in that case.
 * <p>
 * An iterator can be specified to get multiple attributes of the same type:
 * The first call will return the first match from attr; each subsequent call
 * with the same iterator will return the next match. After obtaining the last
 * match from attr, def_attrs is searched in the same manner. If no more matching
 * attributes are found in either of them, false is returned.
 *
 * @param attrs Points to the array of attribute pointers to be searched
 * @param def_attrs Points to a list of pointers to default attributes.
 * If an attribute is not found in attrs, the function will return the first
 * match from def_attrs. This parameter may be NULL.
 * @param type The attribute type to search for
 * @param attr Points to a {@code struct attr} which will receive the attribute
 * @param iter An iterator. This parameter may be NULL.
 * @return True if a matching attribute was found, false if not.
 */
int attr_generic_get_attr(struct attr **attrs, struct attr **def_attrs, enum attr_type type, struct attr *attr,
                          struct attr_iter *iter) {
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

/**
 * @brief Generic set function
 *
 * This function will search the list for the first attribute of the same type
 * as the supplied new one and replace it with that. If the list does not
 * contain an attribute whose type matches that of the new one, the new
 * attribute is inserted into the list.
 *
 * @param attrs Points to the array of attribute pointers to be updated (if NULL, this function will
 * create a new list containing only the new attribute)
 * @param attr The new attribute.
 * @return Pointer to the updated attribute list
 */
struct attr **
attr_generic_set_attr(struct attr **attrs, struct attr *attr) {
    struct attr **curr=attrs;
    int i,count=0;
    dbg(lvl_debug, "enter, attrs=%p, attr=%p (%s)", attrs, attr, attr_to_name(attr->type));
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

/**
 * @brief Generic add function
 *
 * This function will insert a new attribute into the list.
 *
 * @param attrs Points to the array of attribute pointers to be updated
 * @param attr The new attribute.
 * @return Pointer to the updated attribute list
 */
struct attr **
attr_generic_add_attr(struct attr **attrs, struct attr *attr) {
    struct attr **curr=attrs;
    int i,count=0;
    dbg(lvl_debug, "enter, attrs=%p, attr=%p (%s)", attrs, attr, attr_to_name(attr->type));
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
attr_generic_add_attr_list(struct attr **attrs, struct attr **add) {
    while (add && *add) {
        attrs=attr_generic_add_attr(attrs, *add);
        add++;
    }
    return attrs;
}

struct attr **
attr_generic_prepend_attr(struct attr **attrs, struct attr *attr) {
    struct attr **curr=attrs;
    int i,count=0;
    while (curr && *curr) {
        curr++;
        count++;
    }
    curr=g_new0(struct attr *, count+2);
    for (i = 0 ; i  < count ; i++)
        curr[i+1]=attrs[i];
    curr[0]=attr_dup(attr);
    curr[count+1]=NULL;
    g_free(attrs);
    return curr;
}

/**
 * @brief Removes an attribute from an attribute list.
 *
 * If `attrs` contains `attr`, a new attribute list is created (which contains all attributes, except
 * for `attr`) and both `attrs` (the original attribute list) and `attr` are freed.
 *
 * If `attrs` does not contain `attr`, this function is a no-op.
 *
 * @param attrs The attribute list
 * @param attr The attribute to remove from the list
 *
 * @return The new attribute list
 */
struct attr **
attr_generic_remove_attr(struct attr **attrs, struct attr *attr) {
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

enum attr_type attr_type_begin(enum attr_type type) {
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

int attr_data_size(struct attr *attr) {
    if (attr->type == attr_none)
        return 0;
    if (attr->type >= attr_type_string_begin && attr->type <= attr_type_string_end)
        return attr->u.str?strlen(attr->u.str)+1:0;
    if (attr->type >= attr_type_int_begin && attr->type <= attr_type_int_end)
        return sizeof(attr->u.num);
    if (attr->type >= attr_type_coord_geo_begin && attr->type <= attr_type_coord_geo_end)
        return sizeof(*attr->u.coord_geo);
    if (attr->type >= attr_type_color_begin && attr->type <= attr_type_color_end)
        return sizeof(*attr->u.color);
    if (attr->type >= attr_type_object_begin && attr->type <= attr_type_object_end)
        return sizeof(void *);
    if (attr->type >= attr_type_item_begin && attr->type <= attr_type_item_end)
        return sizeof(struct item);
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
    if (attr->type == attr_poly_hole) {
        return (sizeof(attr->u.poly_hole->coord_count) + (attr->u.poly_hole->coord_count * sizeof(*attr->u.poly_hole->coord)));
    }
    dbg(lvl_error,"size for %s unknown", attr_to_name(attr->type));
    return 0;
}

void *attr_data_get(struct attr *attr) {
    if ((attr->type >= attr_type_int_begin && attr->type <= attr_type_int_end) ||
            (attr->type >= attr_type_item_type_begin && attr->type <= attr_type_item_type_end))
        return &attr->u.num;
    if (attr->type == attr_order)
        return &attr->u.range;
    return attr->u.data;
}

void attr_data_set(struct attr *attr, void *data) {
    if ((attr->type >= attr_type_int_begin && attr->type <= attr_type_int_end) ||
            (attr->type >= attr_type_item_type_begin && attr->type <= attr_type_item_type_end))
        attr->u.num=*((int *)data);
    else
        attr->u.data=data;
}

void attr_data_set_le(struct attr * attr, void * data) {
    if ((attr->type >= attr_type_int_begin && attr->type <= attr_type_int_end) ||
            (attr->type >= attr_type_item_type_begin && attr->type <= attr_type_item_type_end))
        attr->u.num=le32_to_cpu(*((int *)data));
    else if (attr->type == attr_order) {
        attr->u.num=le32_to_cpu(*((int *)data));
        attr->u.range.min=le16_to_cpu(attr->u.range.min);
        attr->u.range.max=le16_to_cpu(attr->u.range.max);
    } else
        /* Fixme: Handle long long */
        attr->u.data=data;

}

static void attr_free_content_do(struct attr *attr) {
    if (!attr)
        return;
    if (HAS_OBJECT_FUNC(attr->type)) {
        struct navit_object *obj=attr->u.data;
        if (obj && obj->func && obj->func->unref)
            obj->func->unref(obj);
    }
    if (!(attr->type >= attr_type_int_begin && attr->type <= attr_type_int_end) &&
            !(attr->type >= attr_type_object_begin && attr->type <= attr_type_object_end) &&
            attr->type != attr_item_type)
        g_free(attr->u.data);
}

void attr_free_content(struct attr *attr) {
    attr_free_content_do(attr);
    memset(attr,0,sizeof(*attr));
}

void attr_free(struct attr *attr) {
    attr_free_content_do(attr);
    g_free(attr);
}

void attr_free_g(struct attr *attr, void * unused) {
    attr_free(attr);
}

void attr_dup_content(struct attr *src, struct attr *dst) {
    int size;
    dst->type=src->type;
    if (src->type >= attr_type_int_begin && src->type <= attr_type_int_end)
        dst->u.num=src->u.num;
    else if (src->type == attr_item_type)
        dst->u.item_type=src->u.item_type;
    else if (src->type >= attr_type_object_begin && src->type <= attr_type_object_end) {
        if (HAS_OBJECT_FUNC(src->type)) {
            struct navit_object *obj=src->u.data;
            if (obj && obj->func && obj->func->ref) {
                dst->u.data=obj->func->ref(obj);
            } else
                dst->u.data=obj;
        } else
            dst->u.data=src->u.data;
    } else {
        size=attr_data_size(src);
        if (size) {
            dst->u.data=g_malloc(size);
            memcpy(dst->u.data, src->u.data, size);
        }
    }
}

struct attr *
attr_dup(struct attr *attr) {
    struct attr *ret=g_new0(struct attr, 1);
    attr_dup_content(attr, ret);
    return ret;
}

/**
 * @brief Frees a list of attributes.
 *
 * This frees the pointer array as well as the attributes referenced by the pointers.
 *
 * It is safe to call this function with a NULL argument; doing so is a no-op.
 *
 * @param attrs The attribute list to free
 */
void attr_list_free(struct attr **attrs) {
    int count=0;
    while (attrs && attrs[count]) {
        attr_free(attrs[count++]);
    }
    g_free(attrs);
}

/**
 * @brief Duplicates a list of attributes.
 *
 * This creates a deep copy, i.e. the attributes in the list are copied as well.
 *
 * It is safe to call this function with a NULL argument; in this case, NULL will be returned.
 *
 * @param attrs The attribute list to copy
 *
 * @return The copy of the attribute list
 */
struct attr **
attr_list_dup(struct attr **attrs) {
    struct attr **ret;
    int i,count=0;

    if (!attrs)
        return NULL;

    while (attrs[count])
        count++;
    ret=g_new0(struct attr *, count+1);
    for (i = 0 ; i < count ; i++)
        ret[i]=attr_dup(attrs[i]);
    return ret;
}

/**
 * @brief Retrieves an attribute from a line in textfile format.
 *
 * If `name` is NULL, this function returns the first attribute found; otherwise it looks for an attribute
 * named `name`.
 *
 * If `pos` is specified, it acts as an offset pointer: Any data in `line` before `*pos` will be skipped.
 * When the function returns, the value pointed to by `pos` will be incremented by the number of characters
 * parsed. This can be used to iteratively retrieve all attributes: declare a local variable, set it to zero,
 * then iteratively call this function with a pointer to the same variable until it returns false.
 *
 * `val_ret` must be allocated (and later freed) by the caller, and have sufficient capacity to hold the
 * parsed value including the terminating NULL character. The minimum safe size is
 * `strlen(line) - strlen(name) - *pos` (assuming zero for NULL pointers).
 *
 * @param[in] line The line to parse, must be non-NULL and pointing to a NUL terminated string
 * @param[in] name The name of the attribute to retrieve; can be NULL (see description)
 * @param[in,out] pos As input, if pointer is non-NULL, this argument contains the character index inside @p line from which to start the search (see description)
 * @param[out] val_ret Points to a buffer which will receive the value as text
 * @param[out] name_ret Points to a buffer which will receive the actual name of the attribute found in the line, if NULL this argument won't be used. Note that the buffer provided here should be long enough to contain the attribute name + a terminating NUL character
 *
 * @return true if successful, false in case of failure
 */
int attr_from_line(const char *line, const char *name, int *pos, char *val_ret, char *name_ret) {
    int len=0,quoted,escaped;
    const char *p;
    char *e;
    const char *n;

    dbg(lvl_debug,"get_tag %s from %s", name, line);
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
        escaped=0;
        while (*p) {
            if (*p == ' ' && !quoted)
                break;
            if (*p == '"')
                quoted=1-quoted;
            if (*p == '\\') {	/* Next character is escaped */
                escaped++;
                if (*(p+1))	/* Make sure the string is not terminating just after this escape character */
                    p++;	/* if the string continues, skip the next character, whatever is is (space, double-quote or backslash) */
                else
                    dbg(lvl_warning, "Trailing backslash in input string \"%s\"", line);
            }
            p++;
        }
        if (name == NULL || (e-n == len && strncmp(n, name, len)==0)) {	/* We matched the searched attribute name */
            if (name_ret) {	/* If instructed to, store the actual name into the string pointed by name_ret */
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
            /* Note: in the strncpy* calls below, we give a max size_t argument exactly matching the string lengh we want to copy within the source string e, so no terminating NUL char will be appended */
            if (escaped)
                strncpy_unescape(val_ret, e, len-escaped);	/* Unescape if necessary */
            else
                strncpy(val_ret, e, len);
            /* Because no NUL terminating char was copied over, we manually append it here to terminate the C-string properly, just after the copied string */
            val_ret[len-escaped]='\0';
            if (pos)
                *pos=p-line;
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Checks if an enumeration of attribute types contains a specific attribute.
 *
 * @param types Points to a NULL-terminated array of pointers to {@code enum attr_type}, which will be searched
 * @param type The attr_type to be searched for
 *
 * @return True if the attribute type was found, false if it was not found or if {@code types} is empty
 */
int attr_types_contains(enum attr_type *types, enum attr_type type) {
    while (types && *types != attr_none) {
        if (*types == type)
            return 1;
        types++;
    }
    return 0;
}

/**
 * @brief Check if an enumeration of attribute types contains a specific attribute.
 *
 * This function is mostly identical to {@code attr_types_contains()} but returns the supplied default
 * value if {@code types} is empty.
 *
 * @param types Points to a NULL-terminated array of pointers to {@code enum attr_type}, which will be searched
 * @param type The attr_type to be searched for
 * @param deflt The default value to return if {@code types} is empty.
 *
 * @return True if the attribute type was found, false if it was not found, {@code deflt} if types is empty.
 */
int attr_types_contains_default(enum attr_type *types, enum attr_type type, int deflt) {
    if (!types) {
        return deflt;
    }
    return attr_types_contains(types, type);
}

/**
 * @brief Derive absolute value from relative attribute, given value of the whole range.
 *
 * @param attrval Value of u.num member of attribute capable of holding relative values.
 * @param whole Range counted as 100%.
 * @param treat_neg_as_rel Replace negative absolute values with whole+attr.u.num.
 *
 * @return Absolute value corresponding to given relative value.
 */
int attr_rel2real(int attrval, int whole, int treat_neg_as_rel) {
    if (attrval > ATTR_REL_MAXABS)
        return whole * (attrval - ATTR_REL_RELSHIFT)/100;
    if(treat_neg_as_rel && attrval<0 )
        return whole+attrval;
    return attrval;
}

