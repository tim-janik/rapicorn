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

// --- cpy2rope stubs (generated) ---
#define PLIC_COUPLER() (*rope_thread_coupler())
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

static PyObject*
rope_init_dispatcher (PyObject *self,
                      PyObject *args)
{
  const char *ns = NULL;
  unsigned int nl = 0;
  PyObject *list;
  if (!PyArg_ParseTuple (args, "s#O", &ns, &nl, &list))
    return NULL;
  const ssize_t len = PyList_Size (list);
  if (len < 0)
    return NULL;
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
  /* initialize core */
  RapicornInitValue ivalues[] = { { NULL } };
  int argc = 1;
  char *argv[1], **ap = argv;
  argv[0] = (char*) String (ns, nl).c_str(); // application_name
  rapicorn_init_core (&argc, ap, NULL, ivalues);
  int cpu = Thread::Self::affinity (1);
  uint64 app_id = rope_thread_start (String (ns, nl), strv, cpu);
  if (app_id == 0)
    ; // FIXME: throw exception
  return PyLong_FromUnsignedLongLong (app_id);
}

static PyObject*
rope_event_fd (PyObject *self, PyObject *args)
{
  PyObject *tuple = PyTuple_New (2);
  PyTuple_SET_ITEM (tuple, 0, PyLong_FromLongLong (rope_thread_inputfd()));
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
  Plic::Coupler &cpl = PLIC_COUPLER();
  bool hasevent = cpl.has_event();
  PyObject *pybool = hasevent ? Py_True : Py_False;
  Py_INCREF (pybool);
  return pybool;
}

static PyObject*
rope_event_dispatch (PyObject *self, PyObject *args)
{
  if (self || PyTuple_Size (args) != 0)
    { PyErr_Format (PyExc_TypeError, "no arguments expected"); return NULL; }
  rope_thread_flush_input();
  Plic::Coupler &cpl = PLIC_COUPLER();
  FieldBuffer *fr = NULL, *fb = cpl.pop_event();
  if (!fb)
    return None_INCREF();
  Plic::FieldBufferReader &fbr = cpl.reader;
  fbr.reset (*fb);
  uint64 msgid = fbr.pop_int64();
  switch (msgid >> PLIC_MSGID_SHIFT)
    {
    case Plic::msgid_event >> PLIC_MSGID_SHIFT:
      {
        uint64 handler_id = fbr.pop_int64();
        Plic::EventDispatcher *evd = cpl.dispatcher_lookup (uint (handler_id));
        if (evd)
          fr = evd->dispatch_event (cpl); // continues to use cpl.reader
        else
          fr = FieldBuffer::new_error (string_printf ("invalid signal handler id in %s: %u", "event", uint (handler_id)), "PLIC");
      }
      break;
    case Plic::msgid_discon >> PLIC_MSGID_SHIFT:
      {
        uint64 handler_id = fbr.pop_int64();
        bool deleted = cpl.dispatcher_delete (uint (handler_id));
        if (!deleted)
          fr = FieldBuffer::new_error (string_printf ("invalid signal handler id in %s: %u", "disconnect", uint (handler_id)), "PLIC");
      }
      break;
    default:
      fr = FieldBuffer::new_error (string_printf ("unhandled message id: 0x%x", uint (msgid >> PLIC_MSGID_SHIFT)), "PLIC");
      break;
    }
  cpl.reader.reset();
  delete fb;
  if (fr)
    {
      Plic::FieldBufferReader frr (*fr);
      uint64 frid = frr.pop_int64();
      if (Plic::is_msgid_error (frid))
        {
          String msg = frr.pop_string(), domain = frr.pop_string();
          if (domain.size())
            domain += ": ";
          msg = domain + msg;
          PyErr_Format (PyExc_RuntimeError, "%s", msg.c_str());
        }
      delete fr;
    }
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
    return;

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
    // int dummyargc = 1;
    char *dummyargs[] = { NULL, NULL };
    dummyargs[0] = argv0[0] ? argv0 : (char*) "Python>>>";
    // char **dummyargv = dummyargs;
    // FIXME: // App.init_with_x11 (&dummyargc, &dummyargv, dummyargs[0]);
  }
  free (argv0);
}
// using global namespace for Python module initialization
