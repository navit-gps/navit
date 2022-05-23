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

#include "common.h"
#include "item.h"
#include "coord.h"
#include "route.h"

typedef struct {
    PyObject_HEAD
    struct route *route;
} routeObject;

static PyObject *route_get_map_py(routeObject *self, PyObject *args) {
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    return map_py_ref(route_get_map(self->route));
}



static PyMethodDef route_methods[] = {
    {"get_map",	(PyCFunction) route_get_map_py, METH_VARARGS },
    {NULL, NULL },
};


static PyObject *route_getattr_py(PyObject *self, char *name) {
    return Py_FindMethod(route_methods, self, name);
}

static void route_destroy_py(routeObject *self) {
}

PyTypeObject route_Type = {
    Obj_HEAD
    .tp_name="route",
    .tp_basicsize=sizeof(routeObject),
    .tp_dealloc=(destructor)route_destroy_py,
    .tp_getattr=route_getattr_py,
};

PyObject *route_py(PyObject *self, PyObject *args) {
    routeObject *ret;

    ret=PyObject_NEW(routeObject, &route_Type);
    return (PyObject *)ret;
}

PyObject *route_py_ref(struct route *route) {
    routeObject *ret;

    ret=PyObject_NEW(routeObject, &route_Type);
    ret->route=route;
    return (PyObject *)ret;
}


