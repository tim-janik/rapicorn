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
#include "py-rope.hh" // must be included first to configure std headers
#include <deque>

// --- conventional Python module initializers ---
#define MODULE_NAME             pyRapicorn
#define MODULE_NAME_STRING      STRINGIFY (MODULE_NAME)
#define MODULE_INIT_FUNCTION    RAPICORN_CPP_PASTE2 (init, MODULE_NAME)

// --- Anonymous namespacing
namespace {

static Plic::ClientConnection pyrope_connection;
#define PLIC_CONNECTION()    (pyrope_connection)

// --- cpy2rope stubs (generated) ---
#include "cpy2rope.cc"

// --- PyC functions ---
static PyObject*
rope_printout (PyObject *self,
               PyObject *args)
{
  const char *ns = NULL;
  unsigned int nl = 0;
  if (!PyArg_ParseTuple (args, "s#", &ns, &nl))
    return NULL;
  String str = String (ns, nl);
  printout ("%s", str.c_str());
  return None_INCREF();
}

static void
shutdown_rapicorn_atexit (void)
{
  ApplicationH::shutdown();
}

static PyObject*
rope_init_dispatcher (PyObject *self, PyObject *args)
{
  assert_return (pyrope_connection.is_null(), NULL);
  // parse args: application_name, cmdline_args
  const char *ns = NULL;
  unsigned int nl = 0;
  PyObject *list;
  if (!PyArg_ParseTuple (args, "s#O", &ns, &nl, &list))
    return NULL;
  String application_name (ns, nl); // first argument
  const ssize_t len = PyList_Size (list);
  if (len < 0)
    return NULL;
  // convert args to string vector
  std::vector<String> strv;
  for (ssize_t k = 0; k < len; k++)
    {
      PyObject *item = PyList_GET_ITEM (list, k);
      char *as = NULL;
      Py_ssize_t al = 0;
      if (PyString_AsStringAndSize (item, &as, &al) < 0)
        return NULL;
      strv.push_back (String (as, al));
    }
  if (PyErr_Occurred())
    return NULL;
  // reconstruct argv array
  char *argv[1 + len + 1];
  argv[0] = (char*) application_name.c_str();
  for (ssize_t k = 0; k < len; k++)
    argv[1 + k] = (char*) strv[k].c_str();
  argv[1 + len] = NULL;
  int argc = 1 + len;
  // construct internal arguments
  StringVector iargs;
  iargs.push_back (string_printf ("cpu-affinity=%d", !ThisThread::affinity()));
  // initialize core
  ApplicationH app = init_app (application_name, &argc, argv, iargs);
  uint64 app_id = app._rpc_id();
  if (app_id == 0)
    ; // FIXME: throw exception
  pyrope_connection = app.ipc_connection();
  atexit (shutdown_rapicorn_atexit);
  return PyLong_FromUnsignedLongLong (app_id);
}

static PyObject*
rope_event_fd (PyObject *self, PyObject *args)
{
  PyObject *tuple = PyTuple_New (2);
  PyTuple_SET_ITEM (tuple, 0, PyLong_FromLongLong (PLIC_CONNECTION().notify_fd()));
  PyTuple_SET_ITEM (tuple, 1, PyString_FromString ("i")); // POLLIN
  if (PyErr_Occurred())
    {
      Py_DECREF (tuple);
      tuple = NULL;
    }
  return tuple;
}

static PyObject*
rope_event_check (PyObject *self, PyObject *args)
{
  if (self || PyTuple_Size (args) != 0)
    { PyErr_Format (PyExc_TypeError, "no arguments expected"); return NULL; }
  bool hasevent = PLIC_CONNECTION().pending();
  PyObject *pybool = hasevent ? Py_True : Py_False;
  Py_INCREF (pybool);
  return pybool;
}

static PyObject*
rope_event_dispatch (PyObject *self, PyObject *args)
{
  if (self || PyTuple_Size (args) != 0)
    { PyErr_Format (PyExc_TypeError, "no arguments expected"); return NULL; }
  PLIC_CONNECTION().dispatch();
  return PyErr_Occurred() ? NULL : None_INCREF();
}

} // Anon

// --- Python module definitions (global namespace) ---
static PyMethodDef rope_vtable[] = {
  { "_init_dispatcher",         rope_init_dispatcher,         METH_VARARGS,
    "Run initial Rapicorn setup code." },
  { "_event_fd",                rope_event_fd,                METH_VARARGS,
    "Rapicorn event notification file descriptor." },
  { "_event_check",             rope_event_check,             METH_VARARGS,
    "Check for pending Rapicorn events." },
  { "_event_dispatch",          rope_event_dispatch,          METH_VARARGS,
    "Dispatch pending Rapicorn events." },
  { "printout",                 rope_printout,                METH_VARARGS,
    "Rapicorn::printout() - print to stdout." },
  PLIC_PYSTUB_METHOD_DEFS(),
  { NULL, } // sentinel
};
static const char rapicorn_doc[] = "Rapicorn Python Language Binding Module.";

PyMODINIT_FUNC
MODULE_INIT_FUNCTION (void) // conventional dlmodule initializer
{
  // register module
  PyObject *m = Py_InitModule3 (MODULE_NAME_STRING, rope_vtable, (char*) rapicorn_doc);
  if (!m)
    return; // exception
}
// using global namespace for Python module initialization
