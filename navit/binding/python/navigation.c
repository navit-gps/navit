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
#include "navigation.h"

typedef struct {
    PyObject_HEAD
    struct navigation *navigation;
} navigationObject;

static PyObject *navigation_get_map_py(navigationObject *self, PyObject *args) {
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    return map_py_ref(navigation_get_map(self->navigation));
}



static PyMethodDef navigation_methods[] = {
    {"get_map",	(PyCFunction) navigation_get_map_py, METH_VARARGS },
    {NULL, NULL },
};


static PyObject *navigation_getattr_py(PyObject *self, char *name) {
    return Py_FindMethod(navigation_methods, self, name);
}

static void navigation_destroy_py(navigationObject *self) {
}

PyTypeObject navigation_Type = {
    Obj_HEAD
    .tp_name="navigation",
    .tp_basicsize=sizeof(navigationObject),
    .tp_dealloc=(destructor)navigation_destroy_py,
    .tp_getattr=navigation_getattr_py,
};

PyObject *navigation_py(PyObject *self, PyObject *args) {
    navigationObject *ret;

    ret=PyObject_NEW(navigationObject, &navigation_Type);
    return (PyObject *)ret;
}

PyObject *navigation_py_ref(struct navigation *navigation) {
    navigationObject *ret;

    ret=PyObject_NEW(navigationObject, &navigation_Type);
    ret->navigation=navigation;
    return (PyObject *)ret;
}


