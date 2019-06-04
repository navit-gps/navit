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
#include "navit.h"

typedef struct {
    PyObject_HEAD
    struct navit *navit;
} navitObject;

static PyObject *navit_set_attr_py(navitObject *self, PyObject *args) {
    PyObject *attr;
    if (!PyArg_ParseTuple(args, "O!", &attr_Type, &attr))
        return NULL;
    navit_set_attr(self->navit, attr_py_get(attr));
    Py_RETURN_NONE;
}

static PyObject *navit_get_attr_py(navitObject *self, PyObject *args) {
    char *name;
    struct attr attr;
    if (!PyArg_ParseTuple(args, "s", &name))
        return NULL;
    if (!navit_get_attr(self->navit, attr_from_name(name), &attr, NULL)) {
        dbg(lvl_error,"get_attr not ok");
        Py_RETURN_NONE;
    }
    dbg(lvl_debug,"get_attr ok");
    return python_object_from_attr(&attr);
}

static PyObject *navit_set_center_py(navitObject *self, PyObject *args) {
    PyObject *pcoord;
    if (!PyArg_ParseTuple(args, "O!", &pcoord_Type, &pcoord))
        return NULL;
    navit_set_center(self->navit, pcoord_py_get(pcoord), 0);
    Py_RETURN_NONE;
}

static PyObject *navit_set_destination_py(navitObject *self, PyObject *args) {
    PyObject *pcoord;
    const char *description;
    int async;
    if (!PyArg_ParseTuple(args, "O!si", &pcoord_Type, &pcoord, &description, &async))
        return NULL;
    navit_set_destination(self->navit, pcoord_py_get(pcoord), description, async);
    Py_RETURN_NONE;
}

static PyObject *navit_set_position_py(navitObject *self, PyObject *args) {
    PyObject *pcoord;
    if (!PyArg_ParseTuple(args, "O!", &pcoord_Type, &pcoord))
        return NULL;
    navit_set_position(self->navit, pcoord_py_get(pcoord));
    Py_RETURN_NONE;
}


static PyObject *navit_zoom_to_route_py(navitObject *self, PyObject *args) {
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    navit_zoom_to_route(self->navit,0);
    Py_RETURN_NONE;
}




static PyMethodDef navit_methods[] = {
    {"set_attr",		(PyCFunction) navit_set_attr_py, METH_VARARGS },
    {"get_attr",		(PyCFunction) navit_get_attr_py, METH_VARARGS },
    {"set_center",		(PyCFunction) navit_set_center_py, METH_VARARGS },
    {"set_destination",	(PyCFunction) navit_set_destination_py, METH_VARARGS },
    {"set_position",	(PyCFunction) navit_set_position_py, METH_VARARGS },
    {"zoom_to_route",	(PyCFunction) navit_zoom_to_route_py, METH_VARARGS },
    {NULL, NULL },
};


static PyObject *navit_getattr_py(PyObject *self, char *name) {
    return Py_FindMethod(navit_methods, self, name);
}

static void navit_destroy_py(navitObject *self) {
}

PyTypeObject navit_Type = {
    Obj_HEAD
    .tp_name="navit",
    .tp_basicsize=sizeof(navitObject),
    .tp_dealloc=(destructor)navit_destroy_py,
    .tp_getattr=navit_getattr_py,
};

PyObject *navit_py(PyObject *self, PyObject *args) {
    navitObject *ret;

    dbg(lvl_debug,"enter");
    ret=PyObject_NEW(navitObject, &navit_Type);
    return (PyObject *)ret;
}

PyObject *navit_py_ref(struct navit *navit) {
    dbg(lvl_debug,"navit=%p", navit);
    navitObject *ret=PyObject_NEW(navitObject, &navit_Type);
    ret->navit=navit;
    return (PyObject *)ret;
}
