/* pyRapicorn
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
#include <Python.h> // must be included first to configure std headers

#include <rapicorn/rapicorn.hh>
using namespace Rapicorn;

// Makefile.am: -DPYRAPICORN=pyRapicorn0001
#define _CPPPASTE(a,b)  a ## b
#define _CPPSTRING(s)   #s
#define CPPPASTE(a,b)   _CPPPASTE (a, b)
#define CPPSTRING(s)    _CPPSTRING (s)
#define PYRAPICORNSTR   CPPSTRING (PYRAPICORN)          // "pyRapicorn0001"
#define initpyRapicorn  CPPPASTE (init, PYRAPICORN)     // initpyRapicorn0001

#define rpy_INCREF_None()       ({ Py_INCREF (Py_None); Py_None; })

static PyObject *rapicorn_exception = NULL;

static PyObject*
rpy_printout (PyObject *self,
              PyObject *args)
{
  const char *stringarg = NULL;
  if (!PyArg_ParseTuple (args, "s", &stringarg))
    return NULL;
  printout (stringarg);
  return rpy_INCREF_None();
}

static PyMethodDef rapicorn_vtable[] = {
  { "printout",         rpy_printout,           METH_VARARGS,
    "Rapicorn::printout() - print to stdout." },
  { NULL, } // sentinel
};
static const char rapicorn_doc[] = "Rapicorn C++ glue module.";

PyMODINIT_FUNC
initpyRapicorn (void) // conventional dlmodule initializer
{
  PyObject *m = Py_InitModule3 (PYRAPICORNSTR, rapicorn_vtable, rapicorn_doc);
  if (!m)
    return;
  if (!rapicorn_exception)
    rapicorn_exception = PyErr_NewException ((char*) PYRAPICORNSTR ".exception", NULL, NULL);
  if (!rapicorn_exception)
    return;
  Py_INCREF (rapicorn_exception);
  PyModule_AddObject (m, "exception", rapicorn_exception);
}
