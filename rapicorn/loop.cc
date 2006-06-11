/* Rapicorn
 * Copyright (C) 2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "loop.hh"
#include <sys/poll.h>
#include <errno.h>

namespace Rapicorn {

uint64
MainLoop::get_current_time_usecs ()
{
  GTimeVal current_time;
  g_get_current_time (&current_time);
  return current_time.tv_sec * 1000000ULL + current_time.tv_usec;
}

void
MainLoop::remove (uint   id)
{
  if (!try_remove (id))
    g_warning ("%s: failed to remove loop source: %u", G_STRFUNC, id);
}

class MainLoopPoolThread : public virtual Thread, public virtual MainLoopPool {
  bool iterate();
public:
  MainLoopPoolThread (const String &name) :
    Thread (name)
  {}
  virtual void
  run ()
  {
    do
      {
        if (!iterate())
          {
            if (!Self::sleep (100 * 1000))
              break;
          }
      }
    while (!aborted());
#if 0 // FIXME
    do
      {
        MainLoop *loop = pop_loop();
        if (loop)
          {
            if (loop->pending())
              loop->iteration();
            else
              loop->iteration (true);
          }
        else if (!Self::sleep (100 * 1000))
          break;
        push_loop (loop);
      }
    while (!aborted());
#endif
  }
};

class MainLoopPoolSingleton : public virtual MainLoopPool::Singleton {
  std::list<MainLoop*> loops;
  Thread              *worker;
public:
  MainLoopPoolSingleton() :
    worker (NULL)
  {}
protected:
  virtual MainLoop*
  pop_loop ()
  {
    if (loops.empty())
      return NULL;
    MainLoop *loop = loops.front();
    loops.pop_front();
    return loop;
  }
  virtual void
  push_loop (MainLoop *mloop)
  {
    loops.push_back (mloop);
  }
  virtual void
  add_loop (MainLoop *mloop)
  {
    loops.push_back (mloop);
    set_n_threads (loops.size());
    if (worker)
      worker->wakeup();
  }
  virtual void
  set_n_threads (uint n)
  {
    if (n && !worker)
      {
        worker = new MainLoopPoolThread ("RapicornWorker");
        worker->start();
      }
    if (!n && worker)
      {
        worker->abort();
        worker->wait_for_exit();
        delete worker;
        worker = NULL;
      }
  }
  virtual uint
  get_n_threads ()
  {
    return worker ? 1 : 0;
  }
  virtual void
  quit_loops ()
  {
    std::list<MainLoop*>::iterator it;
    for (it = loops.begin(); it != loops.end(); it++)
      (*it)->quit();
  }
};

MainLoopPool::Singleton *MainLoopPool::m_singleton = NULL;
Mutex                    MainLoopPool::m_mutex;
void
MainLoopPool::rapicorn_init ()
{
  g_assert (!m_singleton);
  m_singleton = new MainLoopPoolSingleton();
}

/* --- GLibSourceBase --- */
class GLibSourceBase {
  static gboolean
  gsource_prepare (GSource    *gsource,
                   gint       *timeout_p)
  {
    GLibSourceBase *self = source_cast<GLibSourceBase*> (gsource);
    return self->prepare (timeout_p);
  }
  static gboolean
  gsource_check (GSource *gsource)
  {
    GLibSourceBase *self = source_cast<GLibSourceBase*> (gsource);
    return self->check();
  }
  static gboolean
  gsource_dispatch (GSource    *gsource,
                    GSourceFunc callback,
                    gpointer    user_data)
  {
    GLibSourceBase *self = source_cast<GLibSourceBase*> (gsource);
    return self->dispatch();
  }
  static void
  gsource_finalize (GSource *gsource)
  {
    /* the corresponding GMainContext mutex is currently locked */
    GLibSourceBase *self = source_cast<GLibSourceBase*> (gsource);
    self->~GLibSourceBase();
  }
  GSource *m_gsource;
protected:
  virtual bool prepare  (int *timeout_p) = 0;
  virtual bool check    () = 0;
  virtual bool dispatch () = 0;
  virtual ~GLibSourceBase()
  {
    /* the corresponding GMainContext mutex is currently locked */
  }
  template<class T> static T*
  create_source()
  {
    static GSourceFuncs source_funcs = {
      gsource_prepare, gsource_check,
      gsource_dispatch, gsource_finalize,
      NULL, NULL,
    };
    GSource *gsource = g_source_new (&source_funcs, sizeof (GSource) + sizeof (T));
    void *ptr = &gsource[1];
    T *self = new (ptr) T (gsource);
    GLibSourceBase *base = dynamic_cast<GLibSourceBase*> (self);
    g_assert (base != NULL);
    g_assert (gsource->callback_data == NULL && gsource->callback_funcs == NULL);
    gsource->callback_data = base;
    return self;
  }
  template<class Tp> static Tp
  source_cast (GSource *gsource)
  {
    if (gsource && gsource->source_funcs &&
        gsource->source_funcs->finalize == gsource_finalize)
      {
        g_assert (gsource->callback_data != NULL && gsource->callback_funcs == NULL);
        GLibSourceBase *base = (GLibSourceBase*) gsource->callback_data;
        return dynamic_cast<Tp> (base);
      }
    return NULL;
  }
  GSource*
  get_source()
  {
    return m_gsource;
  }
  explicit GLibSourceBase (GSource *gsrc) :
    m_gsource (gsrc)
  {
    GSource *gsource = get_source();
    g_source_set_can_recurse (gsource, FALSE);
  }
public:
  static GLibSourceBase*
  find_source (GMainContext *context,
               uint          id)
  {
    GSource *gsource = g_main_context_find_source_by_id (context, id);
    if (gsource)
      return source_cast<GLibSourceBase*> (gsource);
    return NULL;
  }
  void
  set_priority (int priority)
  {
    GSource *gsource = get_source();
    g_source_set_priority (gsource, priority);
  }
  void
  attach (GMainContext *context)
  {
    GSource *gsource = get_source();
    g_source_attach (gsource, context);
  }
  uint64
  get_current_time()
  {
    GSource *gsource = get_source();
    GTimeVal current_time;
    g_source_get_current_time (gsource, &current_time);
    return current_time.tv_sec * 1000000ULL + current_time.tv_usec;
  }
  uint
  get_id ()
  {
    GSource *gsource = get_source();
    return g_source_get_id (gsource);
  }
  virtual void
  destroy()
  {
    GSource *gsource = get_source();
    g_source_destroy (gsource);
  }
};

/* --- GLibSourceSource --- */
class GLibSourceSource : public GLibSourceBase {
  MainLoop::Source *loop_source;
  virtual
  ~GLibSourceSource()
  {
    /* the corresponding GMainContext mutex is currently locked */
    g_assert (loop_source == NULL);
  }
  virtual bool
  prepare (int *timeout_p)
  {
    if (loop_source)
      return loop_source->prepare (get_current_time(), timeout_p);
    return false;
  }
  virtual bool
  check()
  {
    if (loop_source)
      return loop_source->check (get_current_time());
    return false;
  }
  virtual bool
  dispatch ()
  {
    if (loop_source)
      return loop_source->dispatch();
    return false;
  }
  virtual void
  destroy()
  {
    MainLoop::Source *old_loop_source = loop_source;
    loop_source = NULL;
    if (old_loop_source)
      delete old_loop_source;
    GLibSourceBase::destroy(); /* chain parent */
  }
public:
  GLibSourceSource (GSource *gsrc) :
    GLibSourceBase (gsrc),
    loop_source (NULL)
  {}
  static GLibSourceSource*
  create_source (MainLoop::Source *loop_source)
  {
    GLibSourceSource *self = GLibSourceBase::create_source<GLibSourceSource>();
    g_assert (loop_source != NULL);
    g_assert (self->loop_source == NULL);
    self->loop_source = loop_source;
    return self;
  }
};

class GLibMainLoop : public virtual MainLoop {
  GMainContext *m_context;
  Mutex         m_mutex;
  bool          inactive;
  GLibMainLoop () :
    m_context (g_main_context_new()),
    inactive (true)
  {}
  ~GLibMainLoop()
  {
    AutoLocker locker (m_mutex);
    // FIXME: we should destroy all sources here
    g_main_context_unref (m_context);
  }
  virtual void
  quit (void)
  {
    AutoLocker locker (m_mutex);
    // FIXME: we should destroy all sources here
    g_main_context_unref (m_context);
    m_context = g_main_context_new();
    inactive = true;
  }
  virtual bool
  running (void)
  {
    AutoLocker locker (m_mutex);
    return !inactive;
  }
  virtual uint
  add_source (int       priority,
              Source   *loop_source)
  {
    AutoLocker locker (m_mutex);
    GLibSourceSource *source = GLibSourceSource::create_source (loop_source);
    source->set_priority (priority);
    source->attach (m_context);
    inactive = false;
    return source->get_id();
  }
  virtual bool
  try_remove (uint id)
  {
    AutoLocker locker (m_mutex);
    GLibSourceBase *source = GLibSourceBase::find_source (m_context, id);
    if (source)
      {
        source->destroy();
        return true;
      }
    return false;
  }
  virtual bool
  pending ()
  {
    /* not locking */
    return g_main_context_pending (m_context);
  }
  virtual bool
  iteration (bool may_block = false)
  {
    /* not locking */
    return g_main_context_iteration (m_context, may_block);
  }
  virtual bool
  acquire ()
  {
    AutoLocker locker (m_mutex);
    return g_main_context_acquire (m_context);
  }
  virtual bool
  prepare (int     *priority,
           int     *timeout)
  {
    /* not locking, should have acquired thread */
    return g_main_context_prepare (m_context, priority);
  }
  virtual bool
  query (int      max_priority,
         int     *timeout,
         int      n_pfds,
         GPollFD *pfds)
  {
    /* not locking, should have acquired thread */
    return g_main_context_query (m_context, max_priority, timeout, pfds, n_pfds);
  }
  virtual bool
  check     (int      max_priority,
             int      n_pfds,
             GPollFD *pfds)
  {
    /* not locking, should have acquired thread */
    return g_main_context_check (m_context, max_priority, pfds, n_pfds);
  }
  virtual void
  dispatch ()
  {
    /* not locking, should have acquired thread */
    return g_main_context_dispatch (m_context);
  }
  virtual void
  release ()
  {
    AutoLocker locker (m_mutex);
    return g_main_context_release (m_context);
  }
public:
  static GLibMainLoop*
  create_loop_context()
  {
    return new GLibMainLoop();
  }
};

MainLoop*
glib_loop_create (void) // FIXME
{
  return GLibMainLoop::create_loop_context();
}

/* --- main loop iteration --- */
bool
MainLoopPoolThread::iterate()
{
  const uint max_loops = 3;
  MainLoop *loops[max_loops];
  int priorities[max_loops];
  bool dispatch_flag[max_loops] = { 0, };
  std::list<MainLoop*> busy_loops;
  uint n = 0;
  /* collect loop and acquire thread */
  while (n < max_loops)
    {
      loops[n] = pop_loop();
      if (!loops[n])
        break;
      if (!loops[n]->acquire())
        {
          busy_loops.push_back (loops[n]);
          continue;
        }
      priorities[n] = G_MAXINT;
      n++;
    }
  while (!busy_loops.empty())
    {
      push_loop (busy_loops.front());
      busy_loops.pop_front();
    }
  /* prepare all loops */
  int timeout = G_MAXINT;
  for (uint i = 0; i < n; i++)
    {
      int ptimeout = -1;
      if (loops[i]->prepare (&priorities[i], &ptimeout) ||
          ptimeout == 0)
        {
          dispatch_flag[i] = true;
          timeout = 0;
        }
      else if (ptimeout > 0)
        timeout = MIN (timeout, ptimeout);
    }
  /* collect poll-fds from all loops */
  uint max_pfds = 0;
  uint n_pfds = 0;
  GPollFD *pfds = NULL;
  uint pfd_index[max_loops + 1] = { 0, };
  for (uint i = 0; i < n; i++)
    {
      uint need_fds;
      pfd_index[i] = n_pfds;
      int qtimeout = -1;
      while ((need_fds = loops[i]->query (priorities[i], &qtimeout,
                                          max_pfds - pfd_index[i],
                                          pfds + pfd_index[i])) > max_pfds - pfd_index[i])
        {
          max_pfds += need_fds - (max_pfds - pfd_index[i]);
          pfds = g_renew (GPollFD, pfds, max_pfds);
        }
      n_pfds += need_fds;
      if (qtimeout == 0)
        dispatch_flag[i] = true;
      else if (qtimeout > 0)
        timeout = MIN (timeout, qtimeout);
    }
  pfd_index[n] = n_pfds;
  /* poll on the pfds */
  int err = poll ((pollfd*) pfds, n_pfds, timeout);
  if (err < 0 && errno != EINTR)
    g_warning ("%s: failed to poll(): %s", G_STRFUNC, g_strerror (errno));
  /* check the pfds */
  for (uint i = 0; i < n; i++)
    if (loops[i]->check (priorities[i], pfd_index[i + 1] - pfd_index[i], pfds + pfd_index[i]))
      dispatch_flag[i] = true;
  g_free (pfds);
  /* dispatch the loops */
  bool some_dispatched = false;
  for (uint i = 0; i < n; i++)
    if (dispatch_flag[i])
      {
        loops[i]->dispatch();
        some_dispatched = true;
      }
  /* return loops and release thread */
  for (uint i = 0; i < n; i++)
    {
      loops[i]->release();
      push_loop (loops[i]);
    }
  return some_dispatched || timeout > 0;
}

} // Rapicorn
