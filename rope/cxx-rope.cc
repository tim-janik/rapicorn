/* Rapicorn C++ Remote Object Programming Extension
 * Copyright (C) 2010 Tim Janik
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
using namespace Rapicorn;

#define HAVE_PLIC_CALL_REMOTE   1

// --- rope2cxx stubs (generated) ---
#include "rope2cxx.cc"

// --- Anonymous namespacing
namespace {

// --- rapicorn thread ---
static uint  max_call_stack_size = 0;
static void print_max_call_stack_size()
{
  printerr ("DEBUG:atexit: max_call_stack_size: %u\n", max_call_stack_size);
}

class UIThread : public Thread {
  EventLoop * volatile m_loop;
  virtual void
  run ()
  {
    // rapicorn_init_core() already called
    Rapicorn::StringList slist;
    slist.strings = this->cmdline_args;
    App.init_with_x11 (this->application_name, slist);
    m_loop = ref_sink (EventLoop::create());
    EventLoop::Source *esource = new ProcSource (*this);
    (*m_loop).add_source (esource, MAXINT);
    esource->exitable (false);
    {
      FieldBuffer *fb = FieldBuffer::_new (2);
      fb->add_int64 (0x02000000); // proc_id
      Deletable *dapp = &App;
      fb->add_string (""); // FIXME: dapp->object_url()
      push_return (fb);
    }
    App.execute_loops();
  }
  bool
  dispatch ()
  {
    const FieldBuffer *call = pop_proc();
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
        FieldBuffer *fr;
        FieldBuffer8 fdummy (2);
        if (call->first_id() >= 0x02000000)
          fr = FieldBuffer::_new (2);
        else
          fr = &fdummy;
        bool success = true; // plic_call_wrapper_switch (*call, *fr);
        if (!success)
          {
            printerr ("UIThread::call error (see logs)\n"); // FIXME
            if (call->first_id() >= 0x02000000)
              {
                FieldBuffer *fb = FieldBuffer::_new (2);
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
    rrx (0),
    rpx (0)
  {
    rrv.reserve (1);
    atexit (print_max_call_stack_size);
  }
  ~UIThread()
  {
    unref (m_loop);
    m_loop = NULL;
  }
private:
  Mutex                         rrm;
  Cond                          rrc;
  vector<FieldBuffer*>          rrv, rro;
  size_t                        rrx;
public:
  void
  push_return (FieldBuffer *rret)
  {
    rrm.lock();
    rrv.push_back (rret);
    rrc.signal();
    rrm.unlock();
    Thread::Self::yield(); // allow fast return value handling on single core
  }
  FieldBuffer*
  fetch_return (void)
  {
    if (rrx >= rro.size())
      {
        if (rrx)
          rro.resize (0);
        rrm.lock();
        while (rrv.size() == 0)
          rrc.wait (rrm);
        rrv.swap (rro); // fetch result
        rrm.unlock();
        rrx = 0;
      }
    // rrx < rro.size()
    return rro[rrx++];
  }
private:
  Mutex                rps;
  vector<FieldBuffer*> rpv, rpo;
  size_t               rpx;
public:
  void
  push_proc (FieldBuffer *proc)
  {
    int64 call_id = proc->first_id();
    size_t sz;
    rps.lock();
    rpv.push_back (proc);
    sz = rpv.size();
    rps.unlock();
    m_loop->wakeup();
    if (call_id >= 0x02000000 || // minimize turn-around times for two-way calls
        sz >= 1009)              // allow batch processing of one-way call queue
      Thread::Self::yield();     // useless if threads have different CPU affinity
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
        max_call_stack_size = MAX (max_call_stack_size, rpo.size());
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
    ui_thread->m_cpu = -1;
    ui_thread->start();
    FieldBuffer *rpret = ui_thread->fetch_return();
    String appurl;
    if (rpret && rpret->first_id() == 0x02000000)
      {
        Plic::FieldBufferReader rpr (*rpret);
        rpr.skip(); // proc_id
        if (rpr.remaining() > 0 && rpr.get_type() == Plic::STRING)
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

static Plic::FieldBuffer*
plic_call_remote (Plic::FieldBuffer *call)
{
  return UIThread::ui_thread_call_remote (call);
}

/* --- initialization --- */
static void
cxxrope_init_dispatcher (const String         &appname,
                         const vector<String> &cmdline_args)
{
  const char *ns = NULL;
  unsigned int nl = 0;
  String appurl = UIThread::ui_thread_create (String (ns, nl), cmdline_args);
  if (appurl.size() == 0)
    ; // FIXME: throw exception
  printout ("APPURL: %s\n", appurl.c_str());
}

} // Anon

#include <stdio.h>
#include <stdint.h>
int
main (int   argc,
      char *argv[])
{
  cxxrope_init_dispatcher (argv[0], vector<String>());
  return 0;
}
