/* Rapicorn
 * Copyright (C) 2008-2010 Tim Janik
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
#include "loop.hh"

namespace Rapicorn {

class DispatchSource : public virtual EventLoop::Source {
  RAPICORN_PRIVATE_CLASS_COPY (DispatchSource);
protected:
  virtual bool
  prepare  (uint64 current_time_usecs,
            int64 *timeout_usecs_p)
  {
    return Plic::DispatcherRegistry::check_dispatch();
  }
  virtual bool
  check    (uint64 current_time_usecs)
  {
    return Plic::DispatcherRegistry::check_dispatch();
  }
  virtual bool
  dispatch ()
  {
    Plic::DispatcherRegistry::dispatch();
    return true; // keep alive
  }
public:
  DispatchSource() {}
};

struct Initializer {
  String         application_name;
  vector<String> cmdline_args;
  int            cpu;
  Mutex          mutex;
  Cond           cond;
  uint64         app_id;
  Initializer() : cpu (-1), app_id (0) {}
};

class RopeThread : public Thread {
  Initializer *m_init;
  EventLoop   *m_loop;
  ~RopeThread()
  {
    m_init = NULL;
    if (m_loop)
      unref (m_loop);
    m_loop = NULL;
  }
public:
  RopeThread (const String &name,
              Initializer  *init) :
    Thread (name),
    m_init (init),
    m_loop (NULL)
  {}
  void
  wakeup_loop ()
  {
    if (m_loop)
      m_loop->wakeup();
  }
private:
  virtual void
  run ()
  {
    affinity (m_init->cpu);
    // rapicorn_init_core() already called
    Rapicorn::StringList slist;
    slist.strings = m_init->cmdline_args;
    App.init_with_x11 (m_init->application_name, slist);
    m_loop = ref_sink (EventLoop::create());
    EventLoop::Source *esource = new DispatchSource();
    (*m_loop).add_source (esource, MAXINT);
    esource->exitable (false);
    m_init->mutex.lock();
    Locatable *lapp = &App;
    m_init->app_id = lapp->locatable_id();
    m_init->cond.signal();
    m_init->mutex.unlock();
    m_init = NULL;
    App.execute_loops();
  }
};

static RopeThread *rope_thread = NULL;
static uint64      app_id = 0;

uint64
rope_thread_start (const String              &application_name,
                   const std::vector<String> &cmdline_args,
                   int                        cpu)
{
  return_val_if_fail (rope_thread == NULL, 0);
  static volatile size_t initialized = 0;
  if (once_enter (&initialized))
    {
      /* start parallel thread */
      Initializer init;
      init.application_name = application_name;
      init.cmdline_args = cmdline_args;
      init.cpu = cpu;
      rope_thread = new RopeThread ("RapicornUI", &init);
      ref_sink (rope_thread);
      rope_thread->start();
      init.mutex.lock();
      while (init.app_id == 0)
        init.cond.wait (init.mutex);
      app_id = init.app_id;
      init.mutex.unlock();
      once_leave (&initialized, 1);
    }
  return app_id;
}

FieldBuffer*
rope_thread_call (FieldBuffer *call)
{
  return_val_if_fail (rope_thread != NULL, NULL);

  const int64 call_id = call->first_id();
  bool mayblock = Plic::DispatcherRegistry::push_call (call); // deletes call
  rope_thread->wakeup_loop();
  if (mayblock)
    Thread::Self::yield(); // allow fast return value handling on single core
  FieldBuffer *fr = NULL;
  if (Plic::is_callid_twoway (call_id))
    fr = Plic::DispatcherRegistry::fetch_return();
  return fr;
}

} // Rapicorn
