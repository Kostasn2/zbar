/*------------------------------------------------------------------------
 *  Copyright 2009-2010 (c) Jeff Brown <spadix@users.sourceforge.net>
 *
 *  This file is part of the ZBar Bar Code Reader.
 *
 *  The ZBar Bar Code Reader is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU Lesser Public License as
 *  published by the Free Software Foundation; either version 2.1 of
 *  the License, or (at your option) any later version.
 *
 *  The ZBar Bar Code Reader is distributed in the hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 *  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser Public License
 *  along with the ZBar Bar Code Reader; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301  USA
 *
 *  http://sourceforge.net/projects/zbar
 *------------------------------------------------------------------------*/

#include "zbarmodule.h"

static char enumitem_doc[] =
    PyDoc_STR("simple enumeration item.\n"
	      "\n"
	      "associates an int value with a name for printing.");

static zbarEnumItem *enumitem_new(PyTypeObject *type, PyObject *args,
				  PyObject *kwds)
{
    int val		  = 0;
    PyObject *name	  = NULL;
    static char *kwlist[] = { "value", "name", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iS", kwlist, &val, &name))
	return (NULL);

    zbarEnumItem *self = (zbarEnumItem *)type->tp_alloc(type, 0);
    if (!self)
	return (NULL);

#if PY_MAJOR_VERSION >= 3
    PyLongObject *longval = (PyLongObject *)PyLong_FromLong(val);
    if (!longval) {
	Py_DECREF(self);
	return (NULL);
    }

    /* we assume the "fast path" for a single-digit ints (see longobject.c) */
    /* this also holds if we get a small_int preallocated long */
#if PY_VERSION_HEX >= 0x030900A4
    Py_SET_SIZE(&self->val, Py_SIZE(longval));
#else
    Py_SIZE(&self->val)	  = Py_SIZE(longval);
#endif
    self->val.ob_digit[0] = longval->ob_digit[0];
    Py_DECREF(longval);
#else
    self->val.ob_ival = val;
#endif
    self->name = name;
    return (self);
}

static void enumitem_dealloc(zbarEnumItem *self)
{
    Py_CLEAR(self->name);
    ((PyObject *)self)->ob_type->tp_free((PyObject *)self);
}

static PyObject *enumitem_str(zbarEnumItem *self)
{
    Py_INCREF(self->name);
    return (self->name);
}

#if PY_MAJOR_VERSION < 3
/* tp_print was dropped on Python 3.9 */
static int enumitem_print(zbarEnumItem *self, FILE *fp, int flags)
{
    return (self->name->ob_type->tp_print(self->name, fp, flags));
}
#endif

static PyObject *enumitem_repr(zbarEnumItem *self)
{
    PyObject *name = PyObject_Repr(self->name);
    if (!name)
	return (NULL);
#if PY_MAJOR_VERSION >= 3
    PyObject *repr = PyUnicode_FromFormat("%s(%ld, %U)",
					  ((PyObject *)self)->ob_type->tp_name,
					  PyLong_AsLong((PyObject *)self),
					  name);
#else
    char *namestr  = PyString_AsString(name);
    PyObject *repr = PyString_FromFormat("%s(%ld, %s)",
					 ((PyObject *)self)->ob_type->tp_name,
					 self->val.ob_ival, namestr);
#endif
    Py_DECREF(name);
    return ((PyObject *)repr);
}

PyTypeObject zbarEnumItem_Type = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "zbar.EnumItem",
    .tp_doc				   = enumitem_doc,
    .tp_basicsize			   = sizeof(zbarEnumItem),
    .tp_flags	= Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new	= (newfunc)enumitem_new,
    .tp_dealloc = (destructor)enumitem_dealloc,
    .tp_str	= (reprfunc)enumitem_str,
#if PY_MAJOR_VERSION < 3
    .tp_print = (printfunc)enumitem_print,
#endif
    .tp_repr = (reprfunc)enumitem_repr,
};

zbarEnumItem *zbarEnumItem_New(PyObject *byname, PyObject *byvalue, int val,
			       const char *name)
{
    zbarEnumItem *self = PyObject_New(zbarEnumItem, &zbarEnumItem_Type);
    if (!self)
	return (NULL);
#if PY_MAJOR_VERSION >= 3
    PyLongObject *longval = (PyLongObject *)PyLong_FromLong(val);
    if (!longval) {
	Py_DECREF(self);
	return (NULL);
    }

    /* we assume the "fast path" for a single-digit ints (see longobject.c) */
    /* this also holds if we get a small_int preallocated long */
#if PY_VERSION_HEX >= 0x030900A4
    Py_SET_SIZE(&self->val, Py_SIZE(longval));
#else
    Py_SIZE(&self->val)	  = Py_SIZE(longval);
#endif
    self->val.ob_digit[0] = longval->ob_digit[0];
    Py_DECREF(longval);

    self->name = PyUnicode_FromString(name);
#else
    self->val.ob_ival = val;
    self->name	      = PyString_FromString(name);
#endif
    if (!self->name ||
	(byname && PyDict_SetItem(byname, self->name, (PyObject *)self)) ||
	(byvalue &&
	 PyDict_SetItem(byvalue, (PyObject *)self, (PyObject *)self))) {
	Py_DECREF((PyObject *)self);
	return (NULL);
    }
    return (self);
}

static char enum_doc[] = PyDoc_STR("enumeration container for EnumItems.\n"
				   "\n"
				   "exposes items as read-only attributes");

/* FIXME add iteration */

static int enum_traverse(zbarEnum *self, visitproc visit, void *arg)
{
    Py_VISIT(self->byname);
    Py_VISIT(self->byvalue);
    return (0);
}

static int enum_clear(zbarEnum *self)
{
    Py_CLEAR(self->byname);
    Py_CLEAR(self->byvalue);
    return (0);
}

static void enum_dealloc(zbarEnum *self)
{
    enum_clear(self);
    ((PyObject *)self)->ob_type->tp_free((PyObject *)self);
}

PyTypeObject zbarEnum_Type = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "zbar.Enum",

    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,

    .tp_doc	   = enum_doc,
    .tp_basicsize  = sizeof(zbarEnum),
    .tp_dictoffset = offsetof(zbarEnum, byname),
    .tp_traverse   = (traverseproc)enum_traverse,
    .tp_clear	   = (inquiry)enum_clear,
    .tp_dealloc	   = (destructor)enum_dealloc,
};

zbarEnum *zbarEnum_New()
{
    zbarEnum *self = PyObject_GC_New(zbarEnum, &zbarEnum_Type);
    if (!self)
	return (NULL);
    self->byname  = PyDict_New();
    self->byvalue = PyDict_New();
    if (!self->byname || !self->byvalue) {
	Py_DECREF(self);
	return (NULL);
    }
    return (self);
}

int zbarEnum_Add(zbarEnum *self, int val, const char *name)
{
    zbarEnumItem *item;
    item = zbarEnumItem_New(self->byname, self->byvalue, val, name);
    if (!item)
	return (-1);
    return (0);
}

zbarEnumItem *zbarEnum_LookupValue(zbarEnum *self, int val)
{
#if PY_MAJOR_VERSION >= 3
    PyObject *key = PyLong_FromLong(val);
#else
    PyObject *key = PyInt_FromLong(val);
#endif
    zbarEnumItem *e = (zbarEnumItem *)PyDict_GetItem(self->byvalue, key);
    if (!e)
	return ((zbarEnumItem *)key);
    Py_INCREF((PyObject *)e);
    Py_DECREF(key);
    return (e);
}

PyObject *zbarEnum_SetFromMask(zbarEnum *self, unsigned int mask)
{
    PyObject *result = PySet_New(NULL);
    PyObject *key, *item;
    Py_ssize_t i = 0;
    while (PyDict_Next(self->byvalue, &i, &key, &item)) {
#if PY_MAJOR_VERSION >= 3
	unsigned long val = (unsigned long)PyLong_AsLong(item);
#else
	int val = PyInt_AsLong(item);
#endif
	if (val < sizeof(mask) * 8 && ((mask >> val) & 1))
	    PySet_Add(result, item);
    }
    return (result);
}
