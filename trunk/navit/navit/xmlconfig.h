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

struct object_func {
	enum attr_type type;
	void *(*new)(struct attr *parent, struct attr **attrs);
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
struct container;
gboolean config_load(const char *filename, xmlerror **error);
struct object_func *object_func_lookup(enum attr_type type);

#endif

