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
#ifndef __RAPICORN_LOOP_HH__
#define __RAPICORN_LOOP_HH__

#include <rapicorn/primitives.hh>

namespace Rapicorn {

class MainLoop : public virtual ReferenceCountImpl {
public:
  class Source {
  public:
    virtual      ~Source  ()                            {}
    virtual bool prepare  (uint64 current_time_usecs,
                           int   *timeout_msecs_p) = 0;
    virtual bool check    (uint64 current_time_usecs) = 0;
    virtual bool dispatch () = 0;
  };
protected:
  explicit     MainLoop  () {}
  virtual      ~MainLoop () {}
  virtual bool pending   () = 0;
  virtual bool iteration (bool     may_block = false) = 0;
  virtual bool acquire   () = 0;
  virtual bool prepare   (int     *priority,
                          int     *timeout) = 0;
  virtual bool query     (int      max_priority,
                          int     *timeout,
                          int      n_pfds,
                          GPollFD *pfds) = 0;
  virtual bool check     (int      max_priority,
                          int      n_pfds,
                          GPollFD *pfds) = 0;
  virtual void dispatch  () = 0;
  virtual void release   () = 0;
  friend class MainLoopPoolThread;
private:
  template<typename Func>
  class IdleSource : public virtual Source {
    Func m_func;
  protected:
    virtual bool prepare    (uint64 current_time_usecs,
                             int   *timeout_msecs_p)    { return true; }
    virtual bool check      (uint64 current_time_usecs) { return true; }
    virtual bool dispatch   ()                          { return 0 != m_func (); }
  public:
    explicit     IdleSource (Func func) : m_func (func) {}
  };
  template<typename Func>
  class TimedSource : public virtual IdleSource<Func> {
    uint   m_interval_msecs;
    uint64 m_expiration_usecs;
    virtual bool prepare  (uint64 current_time_usecs,
                           int   *timeout_msecs_p);
    virtual bool check    (uint64 current_time_usecs);
    virtual bool dispatch ();
  public:
    explicit TimedSource (Func func,
                          uint interval_msecs);
  };
  static const uint PRIORITY_NOW        = -G_MAXINT / 2;                /* most important, used for immediate async execution */
  static const uint PRIORITY_HIGH       = G_PRIORITY_HIGH - 10;         /* very important, used for io handlers */
  static const uint PRIORITY_NEXT       = G_PRIORITY_HIGH - 5;          /* still very important, used for need-to-be-async operations */
  static const uint PRIORITY_NOTIFY     = G_PRIORITY_DEFAULT - 1;       /* important, delivers async signals */
  static const uint PRIORITY_NORMAL     = G_PRIORITY_DEFAULT;           /* normal importantance, interfaces to all layers */
  static const uint PRIORITY_UPDATE     = G_PRIORITY_HIGH_IDLE + 5;     /* mildly important, used for GUI updates or user information */
  static const uint PRIORITY_BACKGROUND = G_PRIORITY_LOW + 500;         /* unimportant, used when everything else done */
public:
  static uint64                 get_current_time_usecs(); // FIXME: should move this to birnetutilsxx.hh
  virtual void                  quit            (void) = 0;
  virtual bool                  running         (void) = 0;
  virtual uint                  add_source      (int     priority,
                                                 Source *loop_source) = 0;
  virtual bool                  try_remove      (uint   id) = 0;
  void                          remove          (uint   id);
  template<typename Func> uint  idle_timed      (uint   timeout_ms,
                                                 Func   func_obj)  { return add_source (PRIORITY_NEXT,       new TimedSource<Func> (func_obj, timeout_ms)); }
  template<typename Func> uint  idle_now        (Func   func_obj)  { return add_source (PRIORITY_HIGH,       new IdleSource<Func> (func_obj)); }
  template<typename Func> uint  idle_next       (Func   func_obj)  { return add_source (PRIORITY_NEXT,       new IdleSource<Func> (func_obj)); }
  template<typename Func> uint  idle_notify     (Func   func_obj)  { return add_source (PRIORITY_NOTIFY,     new IdleSource<Func> (func_obj)); }
  template<typename Func> uint  idle_normal     (Func   func_obj)  { return add_source (PRIORITY_NORMAL,     new IdleSource<Func> (func_obj)); }
  template<typename Func> uint  idle_update     (Func   func_obj)  { return add_source (PRIORITY_UPDATE,     new IdleSource<Func> (func_obj)); }
  template<typename Func> uint  idle_background (Func   func_obj)  { return add_source (PRIORITY_BACKGROUND, new IdleSource<Func> (func_obj)); }
};

class MainLoopPool {
  static void           rapicorn_init   ();
  friend void           rapicorn_init   ();
  BIRNET_PRIVATE_CLASS_COPY (MainLoopPool);
public:
  class Singleton {
  public:
    virtual void        add_loop        (MainLoop *mloop) = 0;
    virtual void        set_n_threads   (uint      n) = 0;
    virtual uint        get_n_threads   (void) = 0;
    virtual void        quit_loops      () = 0;
    virtual MainLoop*   pop_loop        () = 0;
    virtual void        push_loop       (MainLoop *mloop) = 0;
  };
protected:
  explicit              MainLoopPool    ()                      {}
  static Singleton     *m_singleton;
  static Mutex         *m_mutex;
  static MainLoop*      pop_loop        ()                      { AutoLocker locker (*m_mutex); return m_singleton->pop_loop(); }
  static void           push_loop       (MainLoop *mloop)       { AutoLocker locker (*m_mutex); return m_singleton->push_loop (mloop); }
public:
  static void           add_loop        (MainLoop *mloop)       { AutoLocker locker (*m_mutex); return m_singleton->add_loop (mloop); }
  static void           set_n_threads   (uint      n)           { AutoLocker locker (*m_mutex); return m_singleton->set_n_threads (n); }
  static uint           get_n_threads   (void)                  { AutoLocker locker (*m_mutex); return m_singleton->get_n_threads(); }
  static void           quit_loops      ()                      { AutoLocker locker (*m_mutex); return m_singleton->quit_loops(); }
};

/* --- implementation details --- */
template<typename Func> bool
MainLoop::TimedSource<Func>::prepare (uint64 current_time_usecs,
                                      int   *timeout_msecs_p)
{
  if (current_time_usecs >= m_expiration_usecs)
    return true;                                            /* timeout expired */
  uint64 interval = m_interval_msecs * 1000ULL;
  if (current_time_usecs + interval < m_expiration_usecs)
    m_expiration_usecs = current_time_usecs + interval;     /* clock warped back in time */
  *timeout_msecs_p = MIN (G_MAXINT, (m_expiration_usecs - current_time_usecs) / 1000ULL);
  return 0 == *timeout_msecs_p;
}

template<typename Func> bool
MainLoop::TimedSource<Func>::check (uint64 current_time_usecs)
{
  return current_time_usecs >= m_expiration_usecs;
}

template<typename Func> bool
MainLoop::TimedSource<Func>::dispatch()
{
  if (!IdleSource<Func>::dispatch())
    return false;
  m_expiration_usecs = MainLoop::get_current_time_usecs() + 1000ULL * m_interval_msecs;
  return true;
}

template<typename Func>
MainLoop::TimedSource<Func>::TimedSource (Func func,
                                          uint interval_msecs) :
  IdleSource<Func> (func),
  m_interval_msecs (interval_msecs),
  m_expiration_usecs (MainLoop::get_current_time_usecs() + 1000ULL * interval_msecs)
{}

// FIXME:
MainLoop*    glib_loop_create (void);


} // Rapicorn

#endif  /* __RAPICORN_LOOP_HH__ */
