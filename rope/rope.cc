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
#include "rope.hh" // must be included first to configure std headers
#include <deque>


// --- conventional Python module initializers ---
#define MODULE_NAME             pyRapicorn
#define MODULE_NAME_STRING      STRINGIFY (MODULE_NAME)
#define MODULE_INIT_FUNCTION    RAPICORN_CPP_PASTE2 (init, MODULE_NAME)

static int cpu_affinity (int cpu); // FIXME

// --- rope2cxx stubs (generated) ---
#include "rope2cxx.cc"

// --- Anonymous namespacing
namespace {

// --- cpy2rope stubs (generated) ---
#define HAVE_PLIC_CALL_REMOTE   1
static Rapicorn::Plic::FieldBuffer* plic_call_remote    (Rapicorn::Plic::FieldBuffer*);
static PyObject*                    plic_cpy_trampoline (PyObject *pyself, PyObject *pyargs); // generated
#include "cpy2rope.cc"
typedef Rapicorn::Plic::FieldBuffer FieldBuffer;

// --- globals ---
static PyObject *rapicorn_exception = NULL;

// --- rapicorn thread ---
class UIThread : public Thread {
  EventLoop * volatile m_loop;
  virtual void
  run ()
  {
    cpu_affinity (m_cpu);
    // rapicorn_init_core() already called
    Rapicorn::StringList slist;
    slist.strings = this->cmdline_args;
    App.init_with_x11 (this->application_name, slist);
    m_loop = ref_sink (EventLoop::create());
    EventLoop::Source *esource = new ProcSource (*this);
    (*m_loop).add_source (esource, MAXINT);
    esource->exitable (false);
    {
      FieldBuffer *fb = new FieldBuffer (2);
      fb->add_int64 (0x02000000); // proc_id
      Deletable *dapp = &App;
      fb->add_string (dapp->object_url());
      push_return (fb);
    }
    App.execute_loops();
  }
  bool
  dispatch ()
  {
    FieldBuffer *call = pop_proc();
    if (call)
      {
        if (0)
          {
#if 0
            std::string string;
            if (!google::protobuf::TextFormat::PrintToString (*proc, &string))
              string = "{*protobuf::TextFormat ERROR*}";
            printerr ("Rapicorn::UIThread::call:\n%s\n", string.c_str());
#endif
          }
        FieldBuffer fdummy (2), *fr;
        if (call->first_id() >= 0x02000000)
          fr = new FieldBuffer (2);
        else
          fr = &fdummy;
        bool success = plic_call_wrapper_switch (*call, *fr);
        if (!success)
          {
            printerr ("UIThread::call error (see logs)\n"); // FIXME
            if (call->first_id() >= 0x02000000)
              {
                FieldBuffer *fb = new FieldBuffer (2);
                fb->add_int64 (0x02000000); // proc_id
                fb->add_string ("*ERROR*"); // FIXME
                push_return (fb);
              }
            if (call->first_id() >= 0x02000000)
              delete fr;
          }
        else if (call->first_id() >= 0x02000000)
            push_return (fr);
        delete call;
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
  int            m_cpu;
  String         application_name;
  vector<String> cmdline_args;
  UIThread (const String &name) :
    Thread (name),
    m_loop (NULL),
    m_cpu (-1),
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
  vector<FieldBuffer*>          rrv, rri, rro;
  size_t                        rrx;
public:
  void
  push_return (FieldBuffer *rret)
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
  FieldBuffer*
  fetch_return (void)
  {
    if (rrx >= rro.size())
      {
        if (rrx)
          rro.resize (0);
        /* fetch result */
        rrm.lock();
        if (rrv.size())
          rrv.swap (rro);
        rrm.unlock();
        if (rro.size() == 0) // no result yet
          {
            Thread::Self::yield(); // give way to rapicorn thread on single core
            rrm.lock();
            while (rrv.size() == 0)
              rrc.wait (rrm);
            rrv.swap (rro);
            rrm.unlock();
          }
        rrx = 0;
      }
    // rrx < rro.size()
    return rro[rrx++];
  }
private:
  SpinLock /*Mutex*/            rps;
  vector<FieldBuffer*>          rpv, rpi, rpo;
  size_t                        rpx;
public:
  void
  push_proc (FieldBuffer *proc)
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
  FieldBuffer*
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
  static String
  ui_thread_create (const String              &application_name,
                    const std::vector<String> &cmdline_args)
  {
    return_val_if_fail (ui_thread == NULL, "");
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
    ui_thread->m_cpu = cpu_affinity (-1);
    ui_thread->start();
    FieldBuffer *rpret = ui_thread->fetch_return();
    String appurl;
    if (rpret && rpret->first_id() == 0x02000000)
      {
        Rapicorn::Plic::FieldBufferReader rpr (*rpret);
        rpr.skip(); // proc_id
        if (rpr.remaining() > 0 && rpr.get_type() == Rapicorn::Plic::STRING)
          appurl = rpr.pop_string();
      }
    delete rpret;
    return appurl;
  }
  static FieldBuffer*
  ui_thread_call_remote (FieldBuffer *call)
  {
    if (!ui_thread)
      return false;
    int64 call_id = call->first_id();
    ui_thread->push_proc (call); // deletes call
    if (0) // debug
      printf ("Remote Procedure Call, id=0x%08llx (%s-way)\n",
              call_id, call_id >= 0x02000000 ? "two" : "one");
    if (call_id >= 0x02000000)
      {
        FieldBuffer *rpret = ui_thread->fetch_return();
        if (0)
          printerr ("Remote Procedure Return: 0x%08llx\n", rpret->first_id());
        return rpret;
      }
    return NULL;
  }
};
UIThread *UIThread::ui_thread = NULL;

static Rapicorn::Plic::FieldBuffer*
plic_call_remote (Rapicorn::Plic::FieldBuffer *call)
{
  return UIThread::ui_thread_call_remote (call);
}

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
  String appurl = UIThread::ui_thread_create (String (ns, nl), strv);
  if (appurl.size() == 0)
    ; // FIXME: throw exception
  return PyString_FromStringAndSize (appurl.data(), appurl.size());
}

} // Anon

// --- Python module definitions (global namespace) ---
static PyMethodDef rope_vtable[] = {
  { "_init_dispatcher",         rope_init_dispatcher,         METH_VARARGS,
    "Rapicorn::_init_dispatcher() - initial setup." },
  { "__rope_pytrampoline__",    plic_cpy_trampoline,          METH_VARARGS,
    "Rapicorn function invokation trampoline." },
  { "printout",                 rope_printout,                METH_VARARGS,
    "Rapicorn::printout() - print to stdout." },
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
    // int dummyargc = 1;
    char *dummyargs[] = { NULL, NULL };
    dummyargs[0] = argv0[0] ? argv0 : (char*) "Python>>>";
    // char **dummyargv = dummyargs;
    // FIXME: // App.init_with_x11 (&dummyargc, &dummyargv, dummyargs[0]);
  }
  free (argv0);
}
// using global namespace for Python module initialization



// FIXME: move...
//#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

static int
cpu_affinity (int cpu)
{
  pthread_t thread = pthread_self();
  cpu_set_t cpuset;

  if (cpu >= 0 && cpu < CPU_SETSIZE)
    {
      CPU_ZERO (&cpuset);
      CPU_SET (cpu, &cpuset);
      if (pthread_setaffinity_np (thread, sizeof (cpu_set_t), &cpuset) != 0)
        perror ("pthread_setaffinity_np");
    }

  if (pthread_getaffinity_np (thread, sizeof (cpu_set_t), &cpuset) != 0)
    perror ("pthread_getaffinity_np");
  printf ("Affinity(%ld/%d cpus): thread=%p", sysconf (_SC_NPROCESSORS_ONLN), CPU_SETSIZE, (void*)thread);
  for (int j = 0; j < CPU_SETSIZE; j++)
    if (CPU_ISSET (j, &cpuset))
      {
        printf ("    CPU %d\n", j);
        return j;
      }
  return -1;
}
