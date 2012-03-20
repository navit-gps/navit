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


struct object_func {
	enum attr_type type;
	void *(*create)(struct attr *parent, struct attr **attrs);
	int (*get_attr)(void *, enum attr_type type, struct attr *attr, struct attr_iter *iter);
	struct attr_iter *(*iter_new)(void *);
	void (*iter_destroy)(struct attr_iter *);
	int (*set_attr)(void *, struct attr *attr);
	int (*add_attr)(void *, struct attr *attr);
	int (*remove_attr)(void *, struct attr *attr);
	int (*init)(void *);
	void (*destroy)(void *);
	void *(*dup)(void *);
	void *(*ref)(void *);
	void *(*unref)(void *);
};

extern struct object_func map_func, mapset_func, navit_func, tracking_func, vehicle_func;

#define HAS_OBJECT_FUNC(x) ((x) == attr_map || (x) == attr_mapset || (x) == attr_navit || (x) == attr_trackingo || (x) == attr_vehicle)

struct navit_object {
	struct object_func *func;
	int refcount;
};


typedef GError xmlerror;

extern const int xml_attr_distance;

/* prototypes */
enum attr_type;
struct object_func *object_func_lookup(enum attr_type type);
void xml_parse_text(const char *document, void *data, void (*start)(void *, const char *, const char **, const char **, void *, void *), void (*end)(void *, const char *, void *, void *), void (*text)(void *, const char *, int, void *, void *));
gboolean config_load(const char *filename, xmlerror **error);
//static void xinclude(GMarkupParseContext *context, const gchar **attribute_names, const gchar **attribute_values, struct xmldocument *doc_old, xmlerror **error);

/* end of prototypes */

#ifdef __cplusplus
}
#endif

#endif
