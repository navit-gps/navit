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
};

typedef GError xmlerror;

extern const int xml_attr_distance;

/* prototypes */
enum attr_type;
struct object_func *object_func_lookup(enum attr_type type);
void xml_parse_text(const char *document, void *data, void (*start)(void *, const char *, const char **, const char **, void *, void *), void (*end)(void *, const char *, void *, void *), void (*text)(void *, const char *, int, void *, void *));
gboolean config_load(const char *filename, xmlerror **error);
static void xinclude(GMarkupParseContext *context, const gchar **attribute_names, const gchar **attribute_values, struct xmldocument *doc_old, xmlerror **error);

/* end of prototypes */

#ifdef __cplusplus
}
#endif

#endif
