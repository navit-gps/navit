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
#include "attr.h"

typedef struct {
    PyObject_HEAD
    int ref;
    struct attr *attr;
} attrObject;

static PyObject *attr_func(attrObject *self, PyObject *args) {
    const char *file;
    int ret;
    if (!PyArg_ParseTuple(args, "s", &file))
        return NULL;
    ret=0;
    return Py_BuildValue("i",ret);
}



static PyMethodDef attr_methods[] = {
    {"func",	(PyCFunction) attr_func, METH_VARARGS },
    {NULL, NULL },
};


static PyObject *attr_getattr_py(PyObject *self, char *name) {
    return Py_FindMethod(attr_methods, self, name);
}

static void attr_destroy_py(attrObject *self) {
    if (! self->ref)
        attr_free(self->attr);
}

PyTypeObject attr_Type = {
    Obj_HEAD
    .tp_name="attr",
    .tp_basicsize=sizeof(attrObject),
    .tp_dealloc=(destructor)attr_destroy_py,
    .tp_getattr=attr_getattr_py,
};

struct attr *
attr_py_get(PyObject *self) {
    return ((attrObject *)self)->attr;
}

PyObject *attr_new_py(PyObject *self, PyObject *args) {
    attrObject *ret;
    const char *name,*value;
    if (!PyArg_ParseTuple(args, "ss", &name, &value))
        return NULL;
    ret=PyObject_NEW(attrObject, &attr_Type);
    ret->attr=attr_new_from_text(name, value);
    ret->ref=0;
    return (PyObject *)ret;
}

PyObject *attr_new_py_ref(struct attr *attr) {
    attrObject *ret;

    ret=PyObject_NEW(attrObject, &attr_Type);
    ret->ref=1;
    ret->attr=attr;
    return (PyObject *)ret;
}

