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
#include "pyrope.hh" // must be included first to configure std headers

/* construct conventional Python module initializer name */
#define MODULE_NAME             pyRapicorn
#define MODULE_NAME_STRING      STRINGIFY (MODULE_NAME)
#define MODULE_INIT_FUNCTION    RAPICORN_CPP_PASTE2 (init, MODULE_NAME)


static PyObject *rapicorn_exception = NULL;

static PyObject*
pyrope_trampoline (PyObject *self,
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
          PyObject *po = None_INCREF();
          return po;
        }
      else
        return NULL;
    case 0xA002:
      Application::execute_loops();
      return None_INCREF();
    default:
      return PyErr_Format (PyExc_NotImplementedError, "invalid rapicorn trampoline invokation (code=%u)", ui);
    }
}

static PyObject*
pyrope_printout (PyObject *self,
                 PyObject *args)
{
  const char *stringarg = NULL;
  if (!PyArg_ParseTuple (args, "s", &stringarg))
    return NULL;
  printout ("%s", stringarg);
  return None_INCREF();
}

static PyMethodDef pyrope_vtable[] = {
  { "printout",                 pyrope_printout,                METH_VARARGS,
    "Rapicorn::printout() - print to stdout." },
  { "__pyrope_trampoline__",    pyrope_trampoline,              METH_VARARGS,
    "Rapicorn function invokation trampoline." },
  { NULL, } // sentinel
};
static const char rapicorn_doc[] = "Rapicorn Python Language Binding Module.";

PyMODINIT_FUNC
MODULE_INIT_FUNCTION (void) // conventional dlmodule initializer
{
  // register module
  PyObject *m = Py_InitModule3 (MODULE_NAME_STRING, pyrope_vtable, (char*) rapicorn_doc);
  if (!m)
    return;

  // register Rypicorn exception
  if (!rapicorn_exception)
    rapicorn_exception = PyErr_NewException ((char*) MODULE_NAME_STRING ".exception", NULL, NULL);
  if (!rapicorn_exception)
    return;
  Py_INCREF (rapicorn_exception);
  PyModule_AddObject (m, "exception", rapicorn_exception);

  // retrieve argv[0]
  char *argv0;
  {
    PyObject *sysmod = PyImport_ImportModule ("sys");
    PyObject *astr   = sysmod ? PyObject_GetAttrString (sysmod, "argv") : NULL;
    PyObject *arg0   = astr ? PySequence_GetItem (astr, 0) : NULL;
    argv0            = arg0 ? strdup (PyString_AsString (arg0)) : NULL;
    Py_XDECREF (arg0);
    Py_XDECREF (astr);
    Py_XDECREF (sysmod);
    if (!argv0)
      return; // exception set
  }

  // initialize Rapicorn with dummy argv, hardcode X11 temporarily
  {
    int dummyargc = 1;
    char *dummyargs[] = { NULL, NULL };
    dummyargs[0] = argv0[0] ? argv0 : (char*) "Python>>>";
    char **dummyargv = dummyargs;
    Application::init_with_x11 (&dummyargc, &dummyargv, dummyargs[0]);
  }
  free (argv0);
}
