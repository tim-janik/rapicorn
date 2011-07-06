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
#include "application.hh"

namespace Rapicorn {

class DispatchSource : public virtual EventLoop::Source {
  Plic::Coupler &coupler;
protected:
  virtual bool
  prepare  (uint64 current_time_usecs,
            int64 *timeout_usecs_p)
  {
    return coupler.check_dispatch();
  }
  virtual bool
  check    (uint64 current_time_usecs)
  {
    return coupler.check_dispatch();
  }
  virtual bool
  dispatch ()
  {
    coupler.dispatch();
    return true; // keep alive
  }
public:
  DispatchSource (Plic::Coupler    &_c,
                  EventLoop        &loop) : coupler (_c)
  {
    coupler.set_dispatcher_wakeup (loop, &EventLoop::wakeup);
  }
  virtual ~DispatchSource()
  {
    coupler.set_dispatcher_wakeup (NULL);
  }
};

struct Initializer {
  String              application_identifier;
  Mutex               mutex;
  Cond                cond;
  uint64              app_id;
  Plic::Coupler      *coupler;
  const StringVector *args;     // init-only
  int                *argcp;    // init-only
  char              **argv;     // init-only
  Initializer() : app_id (0), coupler (NULL), args (NULL), argcp (NULL), argv (NULL) {}
};

class RopeThread : public Thread {
  Initializer   *m_init;
  EventLoop     *m_loop;
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
private:
  virtual void
  run ()
  {
    affinity (string_to_int (string_vector_find (*m_init->args, "cpu-affinity=", "-1")));
    // init_core() already called
    init_app (m_init->application_identifier, m_init->argcp, m_init->argv, *m_init->args);
    m_init->argcp = NULL, m_init->argv = NULL, m_init->args = NULL; // becomes invalidated once app_id is set
    m_loop = ref_sink (EventLoop::create());
    EventLoop::Source *esource = new DispatchSource (*m_init->coupler, *m_loop);
    (*m_loop).add_source (esource, EventLoop::PRIORITY_NORMAL);
    esource->primary (false);
    m_init->mutex.lock();
    m_init->app_id = Application_SmartHandle (ApplicationImpl::the())._rpc_id();
    m_init->cond.signal();
    m_init->mutex.unlock();
    m_init = NULL;
    while (true) // !EventLoop::loops_exitable()
      EventLoop::iterate_loops (true, true);      // prepare/check/dispatch and may_block
  }
};

static RopeThread    *rope_thread = NULL;
static uint64         app_id = 0;

class RopeCoupler : public Plic::Coupler {
  virtual FieldBuffer*
  call_remote (FieldBuffer *fbcall)
  {
    return_val_if_fail (rope_thread != NULL, NULL);

    const int64 call_id = fbcall->first_id();
    bool mayblock = push_call (fbcall); // deletes fbcall
    if (mayblock)
      Thread::Self::yield(); // allow fast return value handling on single core
    FieldBuffer *fr = NULL;
    if (Plic::msgid_has_result (call_id))
      fr = pop_result();
    return fr;
  }
};
static RopeCoupler rope_coupler;

Plic::Coupler*
rope_thread_coupler ()
{
  return &rope_coupler;
}

static Plic::EventFd * volatile initevd = NULL;

int
rope_thread_inputfd ()
{
  if (once_enter (&initevd))
    {
      Plic::EventFd *evd = new Plic::EventFd();
      rope_coupler.set_event_wakeup (*evd, &Plic::EventFd::wakeup);
      int err = evd->open();
      if (err < 0)
        fatal ("failed to open EventFd: %s", string_from_errno (err).c_str());
      once_leave (&initevd, evd);
    }
  Plic::EventFd *e = initevd;
  return e->inputfd(); // fd for POLLIN
}

void
rope_thread_flush_input ()
{
  if (initevd)
    {
      Plic::EventFd *evd = initevd;
      evd->flush();
    }
}

uint64
rope_thread_start (const String       &app_ident,
                   int                *argcp,
                   char              **argv,
                   const StringVector &args)
{
  return_val_if_fail (rope_thread == NULL, 0);
  static volatile size_t initialized = 0;
  if (once_enter (&initialized))
    {
      /* start parallel thread */
      Initializer init;
      init.application_identifier = app_ident;
      init.argcp = argcp;
      init.argv = argv;
      init.args = &args;
      init.coupler = &rope_coupler;
      rope_thread = new RopeThread ("RapicornUI", &init);
      ref_sink (rope_thread);
      rope_thread->start();
      init.mutex.lock();
      while (init.app_id == 0)
        init.cond.wait (init.mutex);
      app_id = init.app_id;
      init.mutex.unlock();
      once_leave (&initialized, size_t (1));
    }
  return app_id;
}

} // Rapicorn
