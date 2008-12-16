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
#include "main.h"

typedef struct {
	PyObject_HEAD
} mainObject;

static PyObject *
main_navit(PyObject *self, PyObject *args)
{
	struct navit *navit;
	const char *file;
	int ret=0;
	navit=main_get_navit(NULL);
	return navit_py_ref(navit);
}



static PyMethodDef main_methods[] = {
	{"navit",	(PyCFunction) main_navit, METH_VARARGS },
	{NULL, NULL },
};


static PyObject *
main_getattr_py(PyObject *self, char *name)
{
	return Py_FindMethod(main_methods, self, name);
}

static void
main_destroy_py(mainObject *self)
{
}

PyTypeObject main_Type = {
	Obj_HEAD
	.tp_name="main",
	.tp_basicsize=sizeof(mainObject),
	.tp_dealloc=(destructor)main_destroy_py,
	.tp_getattr=main_getattr_py,
};

PyObject *
main_py(PyObject *self, PyObject *args)
{
	mainObject *ret;

	dbg(0,"enter\n");	
	ret=PyObject_NEW(mainObject, &main_Type);
	return (PyObject *)ret;
}

