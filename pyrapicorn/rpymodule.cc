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

/* construct conventional Python module initializer name */
#define initpyRapicorn RAPICORN_CPP_PASTE2 (init, PYRAPICORN)  // initpyRapicorn0001


static PyObject *rapicorn_exception = NULL;

static PyObject*
rapicorn_trampoline (PyObject *self,
                     PyObject *args)
{
  uint32 ui = 0;
  if (PyTuple_Check (args) && PyTuple_Size (args) > 0)
    {
      PyObject *first = PyTuple_GetItem (args, 0);
      if (PyInt_Check (first))
        ui = PyInt_AsLong (first);
    }
  switch (ui)
    {
      const char *string1;
    case 0xA001:
      if (PyArg_ParseTuple (args, "Is", &ui, &string1))
        {
          printout ("createwindow: %s\n", string1);
          Window w = Factory::create_window (string1);
          PyObject *po = rpy_window_create (w);
          return po;
        }
      else
        return NULL;
    case 0xA002:
      Application::execute_loops();
      return rpy_incref_None();
    default:
      return PyErr_Format (PyExc_NotImplementedError, "invalid rapicorn trampoline invokation (code=%u)", ui);
    }
}

static PyObject*
rpy_printout (PyObject *self,
              PyObject *args)
{
  const char *stringarg = NULL;
  if (!PyArg_ParseTuple (args, "s", &stringarg))
    return NULL;
  printout (stringarg);
  return rpy_incref_None();
}

static PyMethodDef rapicorn_vtable[] = {
  { "printout",         rpy_printout,           METH_VARARGS,
    "Rapicorn::printout() - print to stdout." },
  { "__rapicorn_trampoline__",  rapicorn_trampoline,        METH_VARARGS,
    "Rapicorn function invokation trampoline." },
  { NULL, } // sentinel
};
static const char rapicorn_doc[] = "Rapicorn C++ glue module.";

PyMODINIT_FUNC
initpyRapicorn (void) // conventional dlmodule initializer
{
  PyObject *m = Py_InitModule3 (PYRAPICORNSTR, rapicorn_vtable, (char*) rapicorn_doc);
  if (!m)
    return;
  if (!rapicorn_exception)
    rapicorn_exception = PyErr_NewException ((char*) PYRAPICORNSTR ".exception", NULL, NULL);
  if (!rapicorn_exception)
    return;
  Py_INCREF (rapicorn_exception);
  PyModule_AddObject (m, "exception", rapicorn_exception);
  rpy_types_init (m);

  char *argv0;
  { // retrieve argv[0]
    PyObject *sysmod = PyImport_ImportModule ("sys");
    PyObject *astr   = sysmod ? PyObject_GetAttrString (sysmod, "argv") : NULL;
    PyObject *arg0   = astr ? PySequence_GetItem (astr, 0) : NULL;
    argv0            = arg0 ? strdup (PyString_AsString (arg0)) : NULL;
    Py_XDECREF (sysmod);
    Py_XDECREF (astr);
    Py_XDECREF (arg0);
    if (!argv0)
      return; // exception set
  }
  { // initialize Rapicorn with dummy argv, hardcode X11 temporarily
    int dummyargc = 1;
    char *dummyargs[] = { NULL, NULL };
    dummyargs[0] = argv0[0] ? argv0 : (char*) "Python>>>";
    char **dummyargv = dummyargs;
    Application::init_with_x11 (&dummyargc, &dummyargv, dummyargs[0]);
  }
  free (argv0);
}
