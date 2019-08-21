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

#ifndef NAVIT_XMLCONFIG_H
#define NAVIT_XMLCONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"

#ifndef USE_EZXML
#ifdef HAVE_GLIB
#define USE_EZXML 0
#else
#define USE_EZXML 1
#endif
#endif

#if !USE_EZXML
#define XML_ATTR_DISTANCE 1
typedef GMarkupParseContext xml_context;
#else
#include "ezxml.h"
#define XML_ATTR_DISTANCE 2
#undef G_MARKUP_ERROR
#undef G_MARKUP_ERROR_INVALID_CONTENT
#undef G_MARKUP_ERROR_PARSE
#undef G_MARKUP_ERROR_UNKNOWN_ELEMENT
#define G_MARKUP_ERROR 0
#define G_MARKUP_ERROR_INVALID_CONTENT 0
#define G_MARKUP_ERROR_PARSE 0
#define G_MARKUP_ERROR_UNKNOWN_ELEMENT 0
typedef void * xml_context;
#endif

typedef void *(*object_func_new)(struct attr *parent, struct attr **attrs);
typedef int (*object_func_get_attr)(void *, enum attr_type type, struct attr *attr, struct attr_iter *iter);
typedef struct attr_iter *(*object_func_iter_new)(void *);
typedef void (*object_func_iter_destroy)(struct attr_iter *);
typedef int (*object_func_set_attr)(void *, struct attr *attr);
typedef int (*object_func_add_attr)(void *, struct attr *attr);
typedef int (*object_func_remove_attr)(void *, struct attr *attr);
typedef int (*object_func_init)(void *);
typedef void (*object_func_destroy)(void *);
typedef void *(*object_func_dup)(void *);
typedef void *(*object_func_ref)(void *);
typedef void *(*object_func_unref)(void *);


/**
 * @brief Basic functions for Navit objects
 *
 * This is the minimal list of functions which is supported by every Navit object.
 *
 * Some members can be NULL for certain object types: while Navit does not mandate this function to be
 * implemented for every object class, the function may need to be defined for some object classes.
 *
 * Default implementations are available for every function in the list except `create`, `init` and
 * `dup`. These can be set directly, or type-specific implementations can call through to them as they
 * see fit.
 */
struct object_func {
    enum attr_type type;                           /**< The object type */
    void *(*create)(struct attr *parent, struct attr **attrs); /**< Function to create a new object instance */
    int (*get_attr)(void *, enum attr_type type, struct attr *attr, struct attr_iter *iter); /**< Function
	                                                 *  to get an attribute of the object,
	                                                 *  set to `navit_object_get_attr` for default behavior */
    struct attr_iter *(*iter_new)(void *);         /**< Function to obtain a new attribute iterator,
	                                                 *  set to `navit_object_attr_iter_new` for default
	                                                 *  behavior, can be NULL for some object types */
    void (*iter_destroy)(struct attr_iter *);      /**< Function to destroy an attribute iterator,
	                                                 *  set to `navit_object_attr_iter_destroy` for default
	                                                 *  behavior, can be NULL for some object types */
    int (*set_attr)(void *, struct attr *attr);    /**< Function to set an attribute,
	                                                 *  set to `navit_object_set_attr` for default behavior,
	                                                 *  can be NULL for some object types */
    int (*add_attr)(void *, struct attr *attr);    /**< Function to add an attribute,
	                                                 *  set to `navit_object_add_attr` for default behavior,
	                                                 *  can be NULL for some object types */
    int (*remove_attr)(void *, struct attr *attr); /**< Function to remove an attribute,
	                                                 *  set to `navit_object_remove_attr` for default behavior,
	                                                 *  can be NULL for some object types */
    int (*init)(void *);                           /**< TODO,
	                                                 *  can be NULL for some object types */
    void (*destroy)(void *);                       /**< Function to destroy an object instance,
	                                                 *  set to `navit_object_destroy` for default behavior,
	                                                 *  can be NULL */
    void *(*dup)(void *);                          /**< Function to create a copy of an object instance */
    void *(*ref)(void *);                          /**< Function to increase the reference count for an
	                                                 *  object instance, set to `navit_object_ref` for
	                                                 *  default behavior, can be NULL for some object types */
    void *(*unref)(void *);                        /**< Function to decrease the reference count for an
	                                                 *  object instance, set to `navit_object_unref` for
	                                                 *  default behavior, can be NULL for some object types */
};

extern struct object_func map_func, mapset_func, navit_func, osd_func, tracking_func, vehicle_func, maps_func,
           layout_func, roadprofile_func, vehicleprofile_func, layer_func, config_func, profile_option_func, script_func, log_func,
           speech_func, navigation_func, route_func, traffic_func;

#define HAS_OBJECT_FUNC(x) ((x) == attr_map || (x) == attr_mapset || (x) == attr_navit || (x) == attr_osd || (x) == attr_trackingo || (x) == attr_vehicle || (x) == attr_maps || (x) == attr_layout || (x) == attr_roadprofile || (x) == attr_vehicleprofile || (x) == attr_layer || (x) == attr_config || (x) == attr_profile_option || (x) == attr_script || (x) == attr_log || (x) == attr_speech || (x) == attr_navigation || (x) == attr_route)

#define NAVIT_OBJECT struct object_func *func; int refcount; struct attr **attrs;
struct navit_object {
    NAVIT_OBJECT
};

int navit_object_set_methods(void *in, int in_size, void *out, int out_size);
struct navit_object *navit_object_new(struct attr **attrs, struct object_func *func, int size);
struct navit_object *navit_object_ref(struct navit_object *obj);
void* navit_object_unref(struct navit_object *obj);
struct attr_iter * navit_object_attr_iter_new(void * unused);
void navit_object_attr_iter_destroy(struct attr_iter *iter);
int navit_object_get_attr(struct navit_object *obj, enum attr_type type, struct attr *attr, struct attr_iter *iter);
void navit_object_callbacks(struct navit_object *obj, struct attr *attr);
int navit_object_set_attr(struct navit_object *obj, struct attr *attr);
int navit_object_add_attr(struct navit_object *obj, struct attr *attr);
int navit_object_remove_attr(struct navit_object *obj, struct attr *attr);
void navit_object_destroy(struct navit_object *obj);

typedef GError xmlerror;

/* prototypes */
enum attr_type;
struct object_func *object_func_lookup(enum attr_type type);
int xml_parse_file(char *filename, void *data,
                   void (*start)(xml_context *, const char *, const char **, const char **, void *, GError **),
                   void (*end)(xml_context *, const char *, void *, GError **),
                   void (*text)(xml_context *, const char *, gsize, void *, GError **));
int xml_parse_text(const char *document, void *data, void (*start)(xml_context *, const char *, const char **,
                   const char **, void *, GError **), void (*end)(xml_context *, const char *, void *, GError **),
                   void (*text)(xml_context*, const char *, gsize, void *, GError **));
gboolean config_load(const char *filename, xmlerror **error);
//static void xinclude(GMarkupParseContext *context, const gchar **attribute_names, const gchar **attribute_values, struct xmldocument *doc_old, xmlerror **error);

/* end of prototypes */

#ifdef __cplusplus
}
#endif

#endif
