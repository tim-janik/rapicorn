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
#include <rapicorn.hh>
#include <deque>


// --- conventional Python module initializers ---
#define MODULE_NAME             pyRapicorn
#define MODULE_NAME_STRING      STRINGIFY (MODULE_NAME)
#define MODULE_INIT_FUNCTION    RAPICORN_CPP_PASTE2 (init, MODULE_NAME)

// --- protocol buffers (generated) ---
#include "protocol-pb2.cc"
#include <google/protobuf/text_format.h>

// --- Anonymous namespacing
namespace {

// --- py2rope stubs (generated) ---
static bool pyrope_trampoline_switch (unsigned int, PyObject*, PyObject*, PyObject**); // generated
static bool pyrope_send_message      (Rapicorn::Rope::RemoteProcedure &proc);
#define HAVE_PYROPE_SEND_MESSAGE 1
#include "pycstub.cc"
typedef RemoteProcedure RemoteReturn;

// --- prototypes ---
static bool pyrope_create_dispatch_thread (const String              &application_name,
                                           const std::vector<String> &cmdline_args);

// --- globals ---
static PyObject *rapicorn_exception = NULL;

// --- rapicorn thread ---
class UIThread : public Thread {
  EventLoop * volatile m_loop;
  virtual void
  run ()
  {
    // rapicorn_init_core() already called
    Rapicorn::StringList slist;
    slist.strings = this->cmdline_args;
    printerr ("UIThread::run(): initializing...\n");
    App.init_with_x11 (this->application_name, slist);
    m_loop = ref_sink (EventLoop::create());
    EventLoop::Source *esource = new ProcSource (*this);
    (*m_loop).add_source (esource, MAXINT);
    esource->exitable (false);
    {
      RemoteProcedure *rp = new RemoteProcedure();
      rp->set_proc_id (0x01000000);
      push_return (rp);
    }
    printerr ("UIThread::run(): execute_loops()...\n");
    App.execute_loops();
    printerr ("UIThread::run(): done, exiting.\n");
  }
  bool
  dispatch ()
  {
    RemoteProcedure *proc = pop_proc();
    printerr ("UIThread::dispatch()!\n");
    if (proc)
      {
        if (1)
          {
            std::string string;
            if (!google::protobuf::TextFormat::PrintToString (*proc, &string))
              string = "{*protobuf::TextFormat ERROR*}";
            printerr ("UIThread::call:\n%s\n", string.c_str());
          }
        delete proc;
      }
    return true;
  }
protected:
  class ProcSource : public virtual EventLoop::Source {
    UIThread &m_thread;
    RAPICORN_PRIVATE_CLASS_COPY (ProcSource);
  protected:
    /*Des*/     ~ProcSource() {}
    virtual bool prepare  (uint64 current_time_usecs,
                           int64 *timeout_usecs_p)      { return m_thread.check_dispatch(); }
    virtual bool check    (uint64 current_time_usecs)   { return m_thread.check_dispatch(); }
    virtual bool dispatch ()                            { return m_thread.dispatch(); }
  public:
    explicit     ProcSource (UIThread &thread) : m_thread (thread) {}
  };
public:
  String         application_name;
  vector<String> cmdline_args;
  UIThread (const String &name) :
    Thread (name),
    m_loop (NULL),
    rpx (0)
  {}
  ~UIThread()
  {
    unref (m_loop);
    m_loop = NULL;
  }
private:
  Mutex                         rrm;
  Cond                          rrc;
  vector<RemoteReturn*>         rrv, rri, rro;
  size_t                        rrx;
public:
  void
  push_return (RemoteReturn *rret)
  {
    rrm.lock();
    if (rrv.size() < rrv.capacity())
      rrv.push_back (rret);     // fast path, no realloc required
    else
      {
        rrv.swap (rri);
        rrm.unlock();
        rri.push_back (rret);   // realloc possible
        rrm.lock();
        rrv.swap (rri);
      }
    rrc.signal();
    rrm.unlock();
  }
  RemoteReturn*
  fetch_return (void)
  {
    if (rrx >= rro.size())
      {
        if (rrx)
          rro.resize (0);
        rrm.lock();
        while (rrv.size() == 0)
          rrc.wait (rrm);
        rrv.swap (rro);
        rrm.unlock();
        rrx = 0;
      }
    // rrx < rro.size()
    return rro[rrx++];
  }
private:
  SpinLock                      rps;
  vector<RemoteProcedure*>      rpv, rpi, rpo;
  size_t                        rpx;
public:
  void
  push_proc (RemoteProcedure *proc)
  {
    rps.lock();
    if (rpv.size() < rpv.capacity())
      rpv.push_back (proc);     // fast path, no realloc required
    else
      {
        rpv.swap (rpi);
        rps.unlock();
        rpi.push_back (proc);   // realloc possible
        rps.lock();
        rpv.swap (rpi);
      }
    rps.unlock();
    m_loop->wakeup();
  }
  RemoteProcedure*
  pop_proc (bool advance = true)
  {
    if (rpx >= rpo.size())
      {
        if (rpx)
          rpo.resize (0);
        rps.lock();
        rpv.swap (rpo);
        rps.unlock();
        rpx = 0;
      }
    if (rpx < rpo.size())
      {
        size_t indx = rpx;
        if (advance)
          rpx++;
        return rpo[indx];
      }
    return NULL;
  }
  bool
  check_dispatch ()
  {
    return pop_proc (false) != NULL;
  }
private:
  static UIThread *ui_thread;
public:
  static bool
  ui_thread_create (const String              &application_name,
                    const std::vector<String> &cmdline_args)
  {
    return_val_if_fail (ui_thread == NULL, false);
    /* initialize core */
    RapicornInitValue ivalues[] = {
      { NULL }
    };
    int argc = 0;
    char **argv = NULL;
    rapicorn_init_core (&argc, &argv, NULL, ivalues);
    /* start parallel thread */
    ui_thread = new UIThread ("RapicornUI");
    ref_sink (ui_thread);
    ui_thread->application_name = application_name;
    ui_thread->cmdline_args = cmdline_args;
    ui_thread->start();
    RemoteProcedure *rpret = ui_thread->fetch_return();
    printerr ("INIT-RETURN: 0x%08x\n", rpret->proc_id()); // 0x01000000
    delete rpret;
    return true;
  }
  static bool
  ui_thread_call_remote (Rapicorn::Rope::RemoteProcedure &proc)
  {
    if (!ui_thread)
      return false;
    ui_thread->push_proc (new RemoteProcedure (proc));
    printf ("Remote Procedure Call, id=0x%08x (%s-way)\n",
            proc.proc_id(), proc.proc_id() >= 0x02000000 ? "two" : "one");
    if (proc.proc_id() >= 0x02000000)
      {
        RemoteProcedure *rpret = ui_thread->fetch_return();
        printerr ("Remote Procedure Return: 0x%08x\n", rpret->proc_id());
      }
    return true;
  }
};
UIThread *UIThread::ui_thread = NULL;

static bool
pyrope_send_message (Rapicorn::Rope::RemoteProcedure &proc)
{
  bool success = UIThread::ui_thread_call_remote (proc);
  if (!success)
    return false;       // FIXME: throw python exception
  return true;
}

// --- PyC functions ---
static PyObject*
pyrope_trampoline (PyObject *self,
                   PyObject *args)
{
  PyObject *arg0 = NULL, *ret = NULL;
  uint32 method_id = 0;
  if (PyTuple_Size (args) < 1)
    goto error;
  arg0 = PyTuple_GET_ITEM (args, 0);
  if (!arg0)
    goto error;
  method_id = PyInt_AsLong (arg0);
  if (PyErr_Occurred())
    goto error;
  if (pyrope_trampoline_switch (method_id, self, args, &ret) && ret)
    return ret;
 error:
  if (PyErr_Occurred())
    return NULL;
  return PyErr_Format (PyExc_RuntimeError, "method call failed: 0x%08x (unimplemented?)", method_id);
}

static PyObject*
pyrope_printout (PyObject *self,
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
pyrope_init_dispatcher (PyObject *self,
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
  if (!UIThread::ui_thread_create (String (ns, nl), strv))
    ; // FIXME: throw exception
  return None_INCREF();
}

} // Anon

// --- Python module definitions (global namespace) ---
static PyMethodDef pyrope_vtable[] = {
  { "_init_dispatcher",         pyrope_init_dispatcher,         METH_VARARGS,
    "Rapicorn::_init_dispatcher() - initial setup." },
  { "__pyrope_trampoline__",    pyrope_trampoline,              METH_VARARGS,
    "Rapicorn function invokation trampoline." },
  { "printout",                 pyrope_printout,                METH_VARARGS,
    "Rapicorn::printout() - print to stdout." },
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
    // FIXME: // App.init_with_x11 (&dummyargc, &dummyargv, dummyargs[0]);
  }
  free (argv0);
}
// using global namespace for Python module initialization
