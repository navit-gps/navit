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
#include "template.h"

typedef struct {
    PyObject_HEAD
    int ref;
    struct template *template;
} templateObject;

static PyObject *template_func(templateObject *self, PyObject *args) {
    const char *file;
    int ret;
    if (!PyArg_ParseTuple(args, "s", &file))
        return NULL;
    ret=0;
    return Py_BuildValue("i",ret);
}



static PyMethodDef template_methods[] = {
    {"func",	(PyCFunction) template_func, METH_VARARGS },
    {NULL, NULL },
};


static PyObject *template_getattr_py(PyObject *self, char *name) {
    return Py_FindMethod(template_methods, self, name);
}

static void template_destroy_py(templateObject *self) {
    if (! self->ref)
        template_destroy(self->template);
}

PyTypeObject template_Type = {
    Obj_HEAD
    .tp_name="template",
    .tp_basicsize=sizeof(templateObject),
    .tp_dealloc=(destructor)template_destroy_py,
    .tp_getattr=template_getattr_py,
};

struct template *
template_py_get(PyObject *self) {
    return ((templateObject *)self)->template;
}

PyObject *template_new_py(PyObject *self, PyObject *args) {
    templateObject *ret;

    ret=PyObject_NEW(templateObject, &template_Type);
    ret->template=template_new();
    ret->ref=0;
    return (PyObject *)ret;
}

PyObject *template_new_py_ref(struct template *template) {
    templateObject *ret;

    ret=PyObject_NEW(templateObject, &template_Type);
    ret->ref=1;
    ret->template=template;
    return (PyObject *)ret;
}

