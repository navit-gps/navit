/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifndef NAVIT_ATTR_H
#define NAVIT_ATTR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "projection.h"

enum item_type;

/**
 * Attribute type values, created using macro magic.
 */
enum attr_type {
#define ATTR2(x,y) attr_##y=x,
#define ATTR(x) attr_##x,

    /* Special macro for unused attribute types. Creates a placeholder entry
     * in the enum so the following values do not change. */
#define ATTR_UNUSED ATTR_UNUSED_L(__LINE__)
#define ATTR_UNUSED_L(x) ATTR_UNUSED_WITH_LINE_NUMBER(x)
#define ATTR_UNUSED_WITH_LINE_NUMBER(x) ATTR_UNUSED_##x,

#include "attr_def.h"

#undef ATTR_UNUSED_WITH_LINE_NUMBER
#undef ATTR_UNUSED_L
#undef ATTR_UNUSED

#undef ATTR2
#undef ATTR
};

enum attr_format {
    attr_format_default=0,
    attr_format_with_units=1,
};

#define AF_ONEWAY		(1<<0)
#define AF_ONEWAYREV		(1<<1)
#define AF_NOPASS		(AF_ONEWAY|AF_ONEWAYREV)
#define AF_ONEWAYMASK		(AF_ONEWAY|AF_ONEWAYREV)
#define AF_SEGMENTED		(1<<2)
#define AF_ROUNDABOUT 		(1<<3)
#define AF_ROUNDABOUT_VALID	(1<<4)
#define AF_ONEWAY_EXCEPTION	(1<<5)
#define AF_SPEED_LIMIT		(1<<6)
#define AF_RESERVED1		(1<<7)
#define AF_SIZE_OR_WEIGHT_LIMIT	(1<<8)
#define AF_THROUGH_TRAFFIC_LIMIT (1<<9)
#define AF_TOLL			(1<<10)
#define AF_SEASONAL		(1<<11)
#define AF_UNPAVED		(1<<12)
#define AF_FORD			(1<<13)
#define AF_UNDERGROUND		(1<<14)
#define AF_HIGH_OCCUPANCY_CAR_ONLY	(1<<18)
#define AF_DANGEROUS_GOODS	(1<<19)
#define AF_EMERGENCY_VEHICLES	(1<<20)
#define AF_TRANSPORT_TRUCK	(1<<21)
#define AF_DELIVERY_TRUCK	(1<<22)
#define AF_PUBLIC_BUS		(1<<23)
#define AF_TAXI			(1<<24)
#define AF_HIGH_OCCUPANCY_CAR	(1<<25)
#define AF_CAR			(1<<26)
#define AF_MOTORCYCLE		(1<<27)
#define AF_MOPED		(1<<28)
#define AF_HORSE		(1<<29)
#define AF_BIKE			(1<<30)
#define AF_PEDESTRIAN		(1<<31)

#define AF_PBH (AF_PEDESTRIAN|AF_BIKE|AF_HORSE)
#define AF_MOTORIZED_FAST (AF_MOTORCYCLE|AF_CAR|AF_HIGH_OCCUPANCY_CAR|AF_TAXI|AF_PUBLIC_BUS|AF_DELIVERY_TRUCK|AF_TRANSPORT_TRUCK|AF_EMERGENCY_VEHICLES)
#define AF_ALL (AF_PBH|AF_MOPED|AF_MOTORIZED_FAST)
#define AF_DISTORTIONMASK (AF_ALL|AF_ONEWAYMASK)


#define AF_DG_ANY		(1<<0)
#define AF_DG_WATER_HARMFUL	(1<<1)
#define AF_DG_EXPLOSIVE		(1<<2)
#define AF_DG_FLAMMABLE		(1<<3)

/*
 * Values for attributes that could carry relative values.
 * Some attributes allow both absolute and relative values. The value for these
 * attributes is stored as an int. Absolute values are stored as-is, relative
 * values are stored shifted by adding ATTR_REL_RELSHIFT.
 */
/** Minimum value for an absolute attribute value. */
#define ATTR_REL_MINABS		-0x40000000
/** Maximum value for an absolute attribute value. */
#define ATTR_REL_MAXABS		0x40000000
/** Minimum value for an relative attribute value (without value shift). */
#define ATTR_REL_MINREL		-0x1FFFFFFF
/** Maximum value for an relative attribute value (without value shift). */
#define ATTR_REL_MAXREL		0x20000000
/**
 * Value shift for relative values. This value is added to an attribute values to indicate
 * a relative value. */
#define ATTR_REL_RELSHIFT	0x60000000

/** Indicates whether a position is valid **/
enum attr_position_valid {
    attr_position_valid_invalid,              /**< The position is invalid and should be discarded. **/
    attr_position_valid_static,               /**< The position is valid but the vehicle is not moving, or moving very slowly.
	                                               Calculations that involve the difference between two consecutive positions,
	                                               such as bearing, may therefore be inaccurate. **/
    attr_position_valid_extrapolated_time,    /**< FIXME: this description is just my (mvglasow) guess; this value is not used anywhere as of r5957.
	                                               The position is the vehicle's last known position, and the consumer of the
	                                               information should be aware that the vehicle may have moved since. **/
    attr_position_valid_extrapolated_spatial, /**< FIXME: this description is just my (mvglasow) guess; this value is not used anywhere as of r5957.
	                                               The position is a prediction of the vehicle's current position, based on
	                                               its last known position, the time elapsed since it was obtained and possibly
	                                               other factors. This would be used for positions obtained through inertial
	                                               navigation. **/
    attr_position_valid_valid,                /**< The position is valid and can be used for all purposes. **/
};

#define ATTR_IS_INT(x) ((x) >= attr_type_int_begin && (x) <= attr_type_int_end)
#define ATTR_IS_DOUBLE(x) ((x) >= attr_type_double_begin && (x) <= attr_type_double_end)
#define ATTR_IS_STRING(x) ((x) >= attr_type_string_begin && (x) <= attr_type_string_end)
#define ATTR_IS_OBJECT(x) ((x) >= attr_type_object_begin && (x) <= attr_type_object_end)
#define ATTR_IS_ITEM(x) ((x) >= attr_type_item_begin && (x) <= attr_type_item_end)
#define ATTR_IS_COORD_GEO(x) ((x) >= attr_type_coord_geo_begin && (x) <= attr_type_coord_geo_end)
#define ATTR_IS_NUMERIC(x) (ATTR_IS_INT(x) || ATTR_IS_DOUBLE(x))
#define ATTR_IS_COLOR(x) ((x) >= attr_type_color_begin && (x) <= attr_type_color_end)
#define ATTR_IS_PCOORD(x) ((x) >= attr_type_pcoord_begin && (x) <= attr_type_pcoord_end)
#define ATTR_IS_COORD(x) ((x) >= attr_type_coord_begin && (x) <= attr_type_coord_end)
#define ATTR_IS_GROUP(x) ((x) >= attr_type_group_begin && (x) <= attr_type_group_end)

#define ATTR_INT(x,y) ((struct attr){attr_##x,{.num=y}})
#define ATTR_OBJECT(x,y) ((struct attr){attr_##x,{.navit=y}})

struct range {
    short min, max;
};

struct attr {
    enum attr_type type;
    union {
        char *str;
        void *data;
        long num;
        struct item *item;
        enum item_type item_type;
        enum projection projection;
        double * numd;
        struct color *color;
        struct coord_geo *coord_geo;
        struct navit *navit;
        struct callback *callback;
        struct callback_list *callback_list;
        struct vehicle *vehicle;
        struct layout *layout;
        struct layer *layer;
        struct map *map;
        struct mapset *mapset;
        struct log *log;
        struct route *route;
        struct navigation *navigation;
        struct coord *coord;
        struct pcoord *pcoord;
        struct gui *gui;
        struct graphics *graphics;
        struct tracking *tracking;
        struct itemgra *itemgra;
        struct plugin *plugin;
        struct plugins *plugins;
        struct polygon *polygon;
        struct polyline *polyline;
        struct circle *circle;
        struct text *text;
        struct icon *icon;
        struct image *image;
        struct arrows *arrows;
        struct element *element;
        struct speech *speech;
        struct cursor *cursor;
        struct displaylist *displaylist;
        struct transformation *transformation;
        struct vehicleprofile *vehicleprofile;
        struct roadprofile *roadprofile;
        struct bookmarks *bookmarks;
        struct config *config;
        struct osd *osd;
        struct range range;
        struct navit_object *navit_object;
        struct traffic *traffic;
        int *dash;
        enum item_type *item_types;
        enum attr_type *attr_types;
        long long *num64;
        struct attr *attrs;
        struct poly_hole *poly_hole;
    } u;
};

struct attr_iter;
/* prototypes */
void attr_create_hash(void);
void attr_destroy_hash(void);
enum attr_type attr_from_name(const char *name);
char *attr_to_name(enum attr_type attr);
struct attr *attr_new_from_text(const char *name, const char *value);
char *attr_to_text_ext(struct attr *attr, char *sep, enum attr_format fmt, enum attr_format def_fmt, struct map *map);
char *attr_to_text(struct attr *attr, struct map *map, int pretty);
struct attr *attr_search(struct attr **attrs, struct attr *last, enum attr_type attr);
int attr_generic_get_attr(struct attr **attrs, struct attr **def_attrs, enum attr_type type, struct attr *attr,
                          struct attr_iter *iter);
struct attr **attr_generic_set_attr(struct attr **attrs, struct attr *attr);
struct attr **attr_generic_add_attr(struct attr **attrs, struct attr *attr);
struct attr **attr_generic_add_attr_list(struct attr **attrs, struct attr **add);
struct attr **attr_generic_prepend_attr(struct attr **attrs, struct attr *attr);
struct attr **attr_generic_remove_attr(struct attr **attrs, struct attr *attr);
enum attr_type attr_type_begin(enum attr_type type);
int attr_data_size(struct attr *attr);
void *attr_data_get(struct attr *attr);
void attr_data_set(struct attr *attr, void *data);
void attr_data_set_le(struct attr *attr, void *data);
void attr_free_content(struct attr *attr);
void attr_free(struct attr *attr);
void attr_free_g(struct attr *attr, void * unused); /* to use as GFunc in glib context */
void attr_dup_content(struct attr *src, struct attr *dst);
struct attr *attr_dup(struct attr *attr);
void attr_list_free(struct attr **attrs);
struct attr **attr_list_dup(struct attr **attrs);
struct attr **attr_list_append(struct attr **attrs, struct attr *attr);
int attr_from_line(const char *line, const char *name, int *pos, char *val_ret, char *name_ret);
int attr_types_contains(enum attr_type *types, enum attr_type type);
int attr_types_contains_default(enum attr_type *types, enum attr_type type, int deflt);
int attr_rel2real(int attrval, int whole, int treat_neg_as_rel);
/* end of prototypes */
#ifdef __cplusplus
}
#endif

#endif
