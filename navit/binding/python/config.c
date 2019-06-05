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
#include "config_.h"

typedef struct {
    PyObject_HEAD
} configObject;

static PyObject *config_navit(PyObject *self, PyObject *args) {
    struct attr navit;
    if (config_get_attr(config, attr_navit, &navit, NULL))
        return navit_py_ref(navit.u.navit);
    return NULL;
}



static PyMethodDef config_methods[] = {
    {"navit",	(PyCFunction) config_navit, METH_VARARGS },
    {NULL, NULL },
};


static PyObject *config_getattr_py(PyObject *self, char *name) {
    return Py_FindMethod(config_methods, self, name);
}

static void config_destroy_py(configObject *self) {
}

PyTypeObject config_Type = {
    Obj_HEAD
    .tp_name="config",
    .tp_basicsize=sizeof(configObject),
    .tp_dealloc=(destructor)config_destroy_py,
    .tp_getattr=config_getattr_py,
};

PyObject *config_py(PyObject *self, PyObject *args) {
    configObject *ret;

    dbg(lvl_debug,"enter");
    ret=PyObject_NEW(configObject, &config_Type);
    return (PyObject *)ret;
}

