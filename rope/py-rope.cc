// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#include "py-rope.hh" // must be included first to configure std headers
#include <deque>
using namespace Rapicorn;
using Rapicorn::Aida::RemoteHandle;

#define DEBUG_MODE  0

// --- conventional Python module initializers ---
#define MODULE_NAME             __pyrapicorn
#define MODULE_NAME_STRING      STRINGIFY (MODULE_NAME)
#define MODULE_INIT_FUNCTION    RAPICORN_CPP_PASTE2 (init, MODULE_NAME)

// --- Anonymous namespacing
namespace {

// == Generated C++ Stubs ==
static PyObject *global_rapicorn_module = NULL;
#define __AIDA_PYMODULE__OBJECT global_rapicorn_module
#include "pyrapicorn.cc"

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
  // parse args: application_name, cmdline_args
  const char *ns = NULL, *appclassstr = NULL;
  unsigned int nl = 0, appclasslen = 0;
  PyObject *list;
  if (!PyArg_ParseTuple (args, "s#Os#", &ns, &nl, &list, &appclassstr, &appclasslen))
    return NULL;
  String application_name (ns, nl);                     // first argument
  String appclass_name (appclassstr, appclasslen);      // third argument
  const ssize_t len = PyList_Size (list);
  if (len < 0)
    return NULL;
  // convert args to string vector
  std::vector<String> strv;
  for (ssize_t k = 0; k < len; k++)
    {
      PyObject *widget = PyList_GET_ITEM (list, k);
      char *as = NULL;
      Py_ssize_t al = 0;
      if (PyString_AsStringAndSize (widget, &as, &al) < 0)
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
  iargs.push_back (string_format ("cpu-affinity=%d", !ThisThread::affinity()));
  // initialize core
  ApplicationH app = init_app (application_name, &argc, argv, iargs);
  if (!app)
    return PyErr_Format (PyExc_RuntimeError, "Rapicorn initialization failed");
  atexit (shutdown_rapicorn_atexit);
  return __AIDA_pyfactory__create_handle (app, appclass_name.empty() ? NULL : appclass_name.c_str());
}

static PyObject*
rope_event_fd (PyObject *self, PyObject *args)
{
  PyObject *tuple = PyTuple_New (2);
  PyTuple_SET_ITEM (tuple, 0, PyLong_FromLongLong (__AIDA_local__client_connection->notify_fd()));
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
  bool hasevent = __AIDA_local__client_connection->pending();
  PyObject *pybool = hasevent ? Py_True : Py_False;
  Py_INCREF (pybool);
  return pybool;
}

static PyObject*
rope_event_dispatch (PyObject *self, PyObject *args)
{
  if (self || PyTuple_Size (args) != 0)
    { PyErr_Format (PyExc_TypeError, "no arguments expected"); return NULL; }
  __AIDA_local__client_connection->dispatch();
  return PyErr_Occurred() ? NULL : None_INCREF();
}

// == PyRemoteHandle ==
typedef Rapicorn::Aida::RemoteMember<RemoteHandle> RemoteMember;
typedef struct PyRemoteHandle PyRemoteHandle;
struct PyRemoteHandle {
  PyObject_HEAD;        // standard prologue
  RemoteMember remote;
};
#define PYRH(ooo)       ({ union { PyRemoteHandle *r; PyObject *o; } u; u.o = (ooo); u.r; })
#define PYRO(ooo)       ({ union { PyRemoteHandle *r; PyObject *o; } u; u.r = (ooo); u.o; })
#define PYTO(ooo)       ({ union { PyTypeObject *t; PyObject *o; } u; u.t = (ooo); u.o; })
#define PYS(cchr)       const_cast<char*> (cchr)

static int py_remote_handle_compare (PyObject *v, PyObject *w);

static RemoteMember next_remote_handle;

static PyObject*
py_remote_handle_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  PyRemoteHandle *self = PYRH (type->tp_alloc (type, 0));
  return_unless (self != NULL, NULL);
  new (&self->remote) RemoteMember (next_remote_handle);
  if (DEBUG_MODE)
    Rapicorn::printerr ("py_remote_handle_new: %p\n", self);
  return PYRO (self);
}

static void
py_remote_handle_dealloc (PyRemoteHandle *self)
{
  if (DEBUG_MODE)
    Rapicorn::printerr ("py_remote_handle_dealloc(%p)\n", self);
  self->remote.~RemoteMember();
  self->ob_type->tp_free (PYRO (self));
}

static long
py_remote_handle_hash (PyObject *v)
{
  PyTypeObject *tp = v->ob_type;
  (void) tp;                    // FIXME: check type
  uint64 u = PYRH (v)->remote.__aida_orbid__();
  u ^= (u * 0xa316eef5) >> 32;          // propagate high order bits into low order bits
  long h = u;                           // potentially shrinks 64 bit to 32 bit
  if (h == -1)                          // invalid for python hash to distinguish failures
    h = 1735524191;                     // random substitute
  return h;
}

static PyTypeObject py_remote_handle_type_object = {
  PyObject_HEAD_INIT (NULL)     		// standard prologue
  0,                            		// ob_size
  "Rapicorn.PyRemoteHandle",    		// tp_name
  sizeof (PyRemoteHandle),      		// tp_basicsize
  0,                            		// tp_itemsize
  (destructor) py_remote_handle_dealloc,        // tp_dealloc
  0,                            		// tp_print
  0,                            		// tp_getattr
  0,                            		// tp_setattr
  py_remote_handle_compare,     		// tp_compare
  0,                            		// tp_repr
  0,                            		// tp_as_number
  0,                            		// tp_as_sequence
  0,                            		// tp_as_mapping
  py_remote_handle_hash,        		// tp_hash
  0,                            		// tp_call
  0,                            		// tp_str
  0,                            		// tp_getattro
  0,                            		// tp_setattro
  0,                            		// tp_as_buffer
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,     // tp_flags
  "Rapicorn.PyRemoteHandle() -> "
  "Rapicorn wrapper object for remote objects", // tp_doc
  0,                                            // tp_traverse
  0,                                    	// tp_clear
  0,                                    	// tp_richcompare
  0,                                    	// tp_weaklistoffset
  0,                                    	// tp_iter
  0,                                    	// tp_iternext
  0,                                    	// tp_methods
  0,                                    	// tp_members
  0,                                    	// tp_getset
  0,                                    	// tp_base
  0,                                    	// tp_dict
  0,                                    	// tp_descr_get
  0,                                    	// tp_descr_set
  0,                                    	// tp_dictoffset
  0,                                    	// tp_init
  0,                                    	// tp_alloc
  py_remote_handle_new,                       	// tp_new
};

static int
py_remote_handle_compare (PyObject *v, PyObject *w)
{
  if (v == NULL || w == NULL ||
      !PyObject_TypeCheck (v, &py_remote_handle_type_object) ||
      !PyObject_TypeCheck (w, &py_remote_handle_type_object))
    {
      PyErr_Format (PyExc_RuntimeError, "%s:%d: bad arguments to py_remote_handle_compare", __FILE__, __LINE__);
      return -1;
    }
  if (v == w)
    return 0;
  const uint64 vi = PYRH (v)->remote.__aida_orbid__();
  const uint64 wi = PYRH (w)->remote.__aida_orbid__();
  return vi > wi ? +1 : vi < wi ? -1 : 0;
}

static void
register_py_remote_handle_type (PyObject *module)
{
  if (PyType_Ready (&py_remote_handle_type_object) < 0)
    return;
  Py_INCREF (&py_remote_handle_type_object);
  PyModule_AddObject (module, "PyRemoteHandle", (PyObject*) &py_remote_handle_type_object); // allows new from Python
}

} // Anon

void
py_remote_handle_push (const Rapicorn::Aida::RemoteHandle &rhandle)
{
  if (next_remote_handle)
    fatal ("py_remote_handle_push: stack overflow");
  next_remote_handle = rhandle;
}

void
py_remote_handle_pop (void)
{
  next_remote_handle = RemoteHandle::__aida_null_handle__();
}

Rapicorn::Aida::RemoteHandle
py_remote_handle_extract (PyObject *object)
{
  if (PyObject_TypeCheck (object, &py_remote_handle_type_object))
    return PYRH (object)->remote;
  else
    return Rapicorn::Aida::RemoteHandle::__aida_null_handle__();
}

Rapicorn::Aida::RemoteHandle
py_remote_handle_ensure (PyObject *object) // sets PyErr_Occurred() if null_handle
{
  Rapicorn::Aida::RemoteHandle remote = py_remote_handle_extract (object);
  if (!remote && !PyErr_Occurred())
    PyErr_Format (PyExc_RuntimeError, "invalid call to NULL handle instance of Rapicorn.PyRemoteHandle");
  return remote;
}

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
  AIDA_PYSTUB_METHOD_DEFS(),
  { NULL, } // sentinel
};
static const char rapicorn_doc[] = "Rapicorn Python Language Binding Module.";

PyMODINIT_FUNC MODULE_INIT_FUNCTION (void); // extra decl for gcc

PyMODINIT_FUNC
MODULE_INIT_FUNCTION (void) // conventional dlmodule initializer
{
  assert (global_rapicorn_module == NULL);
  // register module
  global_rapicorn_module = Py_InitModule3 (MODULE_NAME_STRING, rope_vtable, (char*) rapicorn_doc);
  if (!global_rapicorn_module)
    return; // exception
  // PyRemoteHandle
  register_py_remote_handle_type (global_rapicorn_module);
}
// using global namespace for Python module initialization
