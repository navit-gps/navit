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

#include <Python.h>
#include "debug.h"

#if defined(MS_WINDOWS) || defined(__CYGWIN__)
#define Obj_HEAD PyObject_HEAD_INIT(NULL);
#else
#define Obj_HEAD PyObject_HEAD_INIT(&PyType_Type)
#endif

struct navit;

PyObject * main_py(PyObject *self, PyObject *args);
PyObject * navit_py(PyObject *self, PyObject *args);
PyObject * navit_py_ref(struct navit *navit);
extern PyTypeObject pcoord_Type;
PyObject * pcoord_py(PyObject *self, PyObject *args);
struct pcoord *pcoord_py_get(PyObject *self);
