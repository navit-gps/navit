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
#include "coord.h"

typedef struct {
    PyObject_HEAD
    struct pcoord pc;
} pcoordObject;

static PyObject *pcoord_func(pcoordObject *self, PyObject *args) {
    const char *file;
    int ret=0;
    if (!PyArg_ParseTuple(args, "s", &file))
        return NULL;
    return Py_BuildValue("i",ret);
}



static PyMethodDef pcoord_methods[] = {
    {"func",	(PyCFunction) pcoord_func, METH_VARARGS },
    {NULL, NULL },
};


static PyObject *pcoord_getattr_py(PyObject *self, char *name) {
    return Py_FindMethod(pcoord_methods, self, name);
}

static void pcoord_destroy_py(pcoordObject *self) {
}

PyTypeObject pcoord_Type = {
    Obj_HEAD
    .tp_name="pcoord",
    .tp_basicsize=sizeof(pcoordObject),
    .tp_dealloc=(destructor)pcoord_destroy_py,
    .tp_getattr=pcoord_getattr_py,
};

PyObject *pcoord_py(PyObject *self, PyObject *args) {
    pcoordObject *ret;
    const char *str;
    enum projection pro;
    struct coord c;
    if (!PyArg_ParseTuple(args, "si", &str, &pro))
        return NULL;
    ret=PyObject_NEW(pcoordObject, &pcoord_Type);
    coord_parse(str, pro, &c);
    ret->pc.pro=pro;
    ret->pc.x=c.x;
    ret->pc.y=c.y;
    dbg(lvl_debug,"0x%x,0x%x", c.x, c.y);
    return (PyObject *)ret;
}

struct pcoord *
pcoord_py_get(PyObject *self) {
    return &((pcoordObject *)self)->pc;
}

