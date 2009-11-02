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

#include "config.h"
#include <glib.h>
#include "common.h"
#include <fcntl.h>
#include "coord.h"
#include "projection.h"
#include "debug.h"
#include "item.h"
#include "map.h"
#include "mapset.h"
#include "plugin.h"
#include "debug.h"
#include "item.h"
#include "attr.h"
#include "xmlconfig.h"

#if defined(MS_WINDOWS) || defined(__CYGWIN__)
#define Obj_HEAD PyObject_HEAD_INIT(NULL);
#else
#define Obj_HEAD PyObject_HEAD_INIT(&PyType_Type)
#endif

/* *** coord *** */

typedef struct {
	PyObject_HEAD
	struct coord *c;
} coordObject;

static void coord_destroy_py(coordObject *self);

PyTypeObject coord_Type = {
	Obj_HEAD
	.tp_name="coord",
	.tp_basicsize=sizeof(coordObject),
	.tp_dealloc=(destructor)coord_destroy_py,
};


/* *** map *** */

typedef struct {
	PyObject_HEAD
	int ref;
	struct map *m;
} mapObject;

static void map_destroy_py(mapObject *self);
static PyObject *map_getattr_py(PyObject *self, char *name);

PyTypeObject map_Type = {
	Obj_HEAD
	.tp_name="map",
	.tp_basicsize=sizeof(mapObject),
	.tp_dealloc=(destructor)map_destroy_py,
	.tp_getattr=map_getattr_py,
};

/* *** IMPLEMENTATIONS *** */

/* *** coord *** */

static PyObject *
coord_new_py(PyObject *self, PyObject *args)
{
	coordObject *ret;
	int x,y;
	if (!PyArg_ParseTuple(args, "ii:navit.coord",&x,&y))
		return NULL;
	ret=PyObject_NEW(coordObject, &coord_Type);
	ret->c=coord_new(x,y);
	return (PyObject *)ret;
}

static void
coord_destroy_py(coordObject *self)
{
		coord_destroy(self->c);
}

/* *** coord_rect *** */

typedef struct {
	PyObject_HEAD
	struct coord_rect *r;
} coord_rectObject;


static void coord_rect_destroy_py(coord_rectObject *self);

PyTypeObject coord_rect_Type = {
#if defined(MS_WINDOWS) || defined(__CYGWIN__)
	PyObject_HEAD_INIT(NULL);
#else
	PyObject_HEAD_INIT(&PyType_Type)
#endif
	.tp_name="coord_rect",
	.tp_basicsize=sizeof(coord_rectObject),
	.tp_dealloc=(destructor)coord_rect_destroy_py,
};

static PyObject *
coord_rect_new_py(PyObject *self, PyObject *args)
{
	coord_rectObject *ret;
	coordObject *lu,*rd;
	if (!PyArg_ParseTuple(args, "O!O!:navit.coord_rect_rect",&coord_Type,&lu,&coord_Type,&rd))
		return NULL;
	ret=PyObject_NEW(coord_rectObject, &coord_rect_Type);
	ret->r=coord_rect_new(lu->c,rd->c);
	return (PyObject *)ret;
}

static void
coord_rect_destroy_py(coord_rectObject *self)
{
	coord_rect_destroy(self->r);
}

/* *** map_rect *** */

typedef struct {
	PyObject_HEAD
	struct map_rect *mr;
} map_rectObject;


static void map_rect_destroy_py(map_rectObject *self);

PyTypeObject map_rect_Type = {
#if defined(MS_WINDOWS) || defined(__CYGWIN__)
	PyObject_HEAD_INIT(NULL);
#else
	PyObject_HEAD_INIT(&PyType_Type)
#endif
	.tp_name="map_rect",
	.tp_basicsize=sizeof(map_rectObject),
	.tp_dealloc=(destructor)map_rect_destroy_py,
};

static PyObject *
map_rect_new_py(mapObject *self, PyObject *args)
{
	map_rectObject *ret;
	coord_rectObject *r;
	if (!PyArg_ParseTuple(args, "O!:navit.map_rect_rect",&coord_rect_Type,&r))
		return NULL;
	ret=PyObject_NEW(map_rectObject, &map_rect_Type);
	ret->mr=map_rect_new(self->m, NULL);
	return (PyObject *)ret;
}

static void
map_rect_destroy_py(map_rectObject *self)
{
	map_rect_destroy(self->mr);
}


/* *** map *** */

static PyObject *
map_dump_file_py(mapObject *self, PyObject *args)
{
	const char *s;
	if (!PyArg_ParseTuple(args, "s",&s))
		return NULL;
	map_dump_file(self->m, s);
	Py_RETURN_NONE;
}


static PyObject *
map_set_attr_py(mapObject *self, PyObject *args)
{
	PyObject *attr;
	if (!PyArg_ParseTuple(args, "O!", &attr_Type, &attr))
		return NULL;
	map_set_attr(self->m, attr_py_get(attr));
	Py_RETURN_NONE;
}



static PyMethodDef map_methods[] = {
	{"dump_file",		(PyCFunction) map_dump_file_py, METH_VARARGS },
	{"map_rect_new",	(PyCFunction) map_rect_new_py, METH_VARARGS },
	{"set_attr",		(PyCFunction) map_set_attr_py, METH_VARARGS },
	{NULL, NULL },
};

static PyObject *
map_getattr_py(PyObject *self, char *name)
{
	return Py_FindMethod(map_methods, self, name);
}


static PyObject *
map_new_py(PyObject *self, PyObject *args)
{
	mapObject *ret;
	char *type, *filename;
	
	if (!PyArg_ParseTuple(args, "ss:navit.map", &type, &filename))
		return NULL;
	ret=PyObject_NEW(mapObject, &map_Type);
	ret->m=map_new(NULL,NULL);
	ret->ref=0;
	return (PyObject *)ret;
}

PyObject *
map_py_ref(struct map *map)
{
	mapObject *ret;
	ret=PyObject_NEW(mapObject, &map_Type);
	ret->m=map;
	ret->ref=1;
	return (PyObject *)ret;
}

static void
map_destroy_py(mapObject *self)
{
	if (!self->ref)
		map_destroy(self->m);
}

/* *** mapset *** */


typedef struct {
	PyObject_HEAD
	struct mapset *ms;
} mapsetObject;


static void mapset_destroy_py(mapsetObject *self);
static PyObject *mapset_getattr_py(PyObject *self, char *name);

PyTypeObject mapset_Type = {
#if defined(MS_WINDOWS) || defined(__CYGWIN__)
	PyObject_HEAD_INIT(NULL);
#else
	PyObject_HEAD_INIT(&PyType_Type)
#endif
	.tp_name="mapset",
	.tp_basicsize=sizeof(mapsetObject),
	.tp_dealloc=(destructor)mapset_destroy_py,
	.tp_getattr=mapset_getattr_py,
};

static PyObject *
mapset_add_py(mapsetObject *self, PyObject *args)
{
	mapObject *map;
	if (!PyArg_ParseTuple(args, "O:navit.mapset", &map))
		return NULL;
	Py_INCREF(map);
	mapset_add_attr(self->ms, &(struct attr){attr_map,.u.map=map->m});
	return Py_BuildValue("");
}

static PyMethodDef mapset_methods[] = {
	{"add",	(PyCFunction) mapset_add_py, METH_VARARGS },
	{NULL, NULL },
};

static PyObject *
mapset_getattr_py(PyObject *self, char *name)
{
	return Py_FindMethod(mapset_methods, self, name);
}

static PyObject *
mapset_new_py(PyObject *self, PyObject *args)
{
	mapsetObject *ret;
	if (!PyArg_ParseTuple(args, ":navit.mapset"))
		return NULL;
	ret=PyObject_NEW(mapsetObject, &mapset_Type);
	ret->ms=mapset_new(NULL,NULL);
	return (PyObject *)ret;
}

static void
mapset_destroy_py(mapsetObject *self)
{
	mapset_destroy(self->ms);
}

static PyObject *
config_load_py(PyObject *self, PyObject *args)
{
	const char *file;
	int ret;
	xmlerror *error;
	if (!PyArg_ParseTuple(args, "s", &file))
		return NULL;
	ret=config_load(file, &error);
	return Py_BuildValue("i",ret);
}

static PyMethodDef navitMethods[]={
	{"attr", attr_new_py, METH_VARARGS},
	{"coord", coord_new_py, METH_VARARGS, "Create a new coordinate point."},
	{"coord_rect", coord_rect_new_py, METH_VARARGS, "Create a new coordinate rectangle."},
	{"map", map_new_py, METH_VARARGS, "Create a new map."},
	{"mapset", mapset_new_py, METH_VARARGS, "Create a new mapset."},
	{"config_load", config_load_py, METH_VARARGS, "Load a config"},
	{"config", config_py, METH_VARARGS, "Get Config Object"},
	{"pcoord", pcoord_py, METH_VARARGS},
	{NULL, NULL, 0, NULL}
};


PyObject *
python_object_from_attr(struct attr *attr)
{
	switch (attr->type) {
	case attr_navigation:
		return navigation_py_ref(attr->u.navigation);
	case attr_route:
		return route_py_ref(attr->u.route);
	default:
		return NULL;
	}
	return NULL;
}


void
plugin_init(void)
{
	int fd,size;
	char buffer[65536];

	Py_Initialize();
	Py_InitModule("navit", navitMethods);
	fd=open("startup.py",O_RDONLY);
	if (fd >= 0) {
		size=read(fd, buffer, 65535);
		if (size > 0) {
			buffer[size]='\0';
			PyRun_SimpleString(buffer);
		}
	}
	
	Py_Finalize();
}
