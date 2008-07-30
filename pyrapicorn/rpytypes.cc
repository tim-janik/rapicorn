/* Rapicorn-Python Bindings
 * Copyright (C) 2008 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "rpy.hh" // must be included first to configure std headers

#include <rapicorn/rapicorn.hh>
using namespace Rapicorn;

#define rpy_is_pywindow(pyval)  PyObject_TypeCheck (pyval, &rpy_window_type)

struct PyWindow {
  PyObject_HEAD
  Window window;
};

static PyObject*
rpy_window_new (PyTypeObject *type,
                PyObject     *args,
                PyObject     *kwds)
{
  Window *tmpwin = NULL;
  if (PyTuple_Check (args) && PyTuple_Size (args) == 1)
    {
      PyObject *first = PyTuple_GetItem (args, 0);
      if (!first)
        return NULL;
      if (PyString_Check (first))
        {
          const char *rooturl = PyString_AsString (first);
          if (!rooturl)
            return NULL;
          tmpwin = Window::from_object_url (rooturl);
        }
    }
  if (!tmpwin)
    return PyErr_Format (PyExc_RuntimeError, "invalid instantiation of internal Rapicorn.Window type");
  PyWindow *self = (PyWindow*) type->tp_alloc (type, 0);
  if (self == NULL)
    return NULL;
  new (&self->window) Window (*tmpwin);
  delete tmpwin;
  printout ("Window-new(%p)\n", self);
  return PYWO (self);
}

static void
rpy_window_dealloc (PyWindow *self)
{
  printout ("Window-del(%p)\n", self);
  self->window.~Window();
  self->ob_type->tp_free (PYWO (self));
}

static PyObject*
pywindow_show (PyWindow *self)
{
  self->window.show();
  return rpy_incref_None();
}

static PyMethodDef pywindow_methods[] = {
  { "show", PYCF (pywindow_show), METH_NOARGS, "display window on screen" },
  { NULL, NULL, },
};

static PyTypeObject rpy_window_type = {
  PyObject_HEAD_INIT (NULL)
  0,                                    // ob_size
  PYRAPICORNSTR ".Window",              // tp_name
  sizeof (PyWindow),                    // tp_basicsize
  0,                                    // tp_itemsize
  (destructor) rpy_window_dealloc,      // tp_dealloc
  0,                                    // tp_print
  0,                                    // tp_getattr
  0,                                    // tp_setattr
  0,                                    // tp_compare
  0,                                    // tp_repr
  0,                                    // tp_as_number
  0,                                    // tp_as_sequence
  0,                                    // tp_as_mapping
  0,                                    // tp_hash
  0,                                    // tp_call
  0,                                    // tp_str
  0,                                    // tp_getattro
  0,                                    // tp_setattro
  0,                                    // tp_as_buffer
  Py_TPFLAGS_DEFAULT,                   // tp_flags
  "Rapicorn Window object",             // tp_doc
  0,                                    // tp_traverse
  0,                                    // tp_clear
  0,                                    // tp_richcompare
  0,                                    // tp_weaklistoffset
  0,                                    // tp_iter
  0,                                    // tp_iternext
  pywindow_methods,                     // tp_methods
  0,                                    // tp_members
  0,                                    // tp_getset
  0,                                    // tp_base
  0,                                    // tp_dict
  0,                                    // tp_descr_get
  0,                                    // tp_descr_set
  0,                                    // tp_dictoffset
  0,                                    // tp_init
  0,                                    // tp_alloc
  rpy_window_new,                       // tp_new
};

PyObject*
rpy_window_create (Window &window)
{
  PyObject *obj = PyObject_CallFunction (PYTO (&rpy_window_type),
                                         PYS ("s"), window.root().object_url().c_str());
  return obj;
}

static void
pymodule_add_type (PyObject     *module,
                   PyTypeObject *ptyobj)
{
  char *objname = strrchr (ptyobj->tp_name, '.');
  assert (objname != NULL);
  PyModule_AddObject (module, objname + 1, (PyObject*) ptyobj);
}

void
rpy_types_init (PyObject *module)
{
  if (PyType_Ready (&rpy_window_type) < 0)
    return;
  Py_INCREF (&rpy_window_type);
  // pymodule_add_type (module, &rpy_window_type);
}

