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
  class TimedSource : public virtual Source {
    uint64     m_expiration_usecs;
    uint       m_interval_msecs;
    const bool m_oneshot;
    bool       m_first_interval;
    union {
      Signals::Trampoline0<bool> *m_btrampoline;
      Signals::Trampoline0<void> *m_vtrampoline;
    };
  protected:
    virtual      ~TimedSource ();
    virtual bool prepare      (uint64 current_time_usecs,
                               int   *timeout_msecs_p);
    virtual bool check        (uint64 current_time_usecs);
    virtual bool dispatch     ();
  public:
    explicit     TimedSource (Signals::Trampoline0<bool> &bt,
                              uint initial_interval_msecs = 0,
                              uint repeat_interval_msecs = 0);
    explicit     TimedSource (Signals::Trampoline0<void> &vt,
                              uint initial_interval_msecs = 0,
                              uint repeat_interval_msecs = 0);
  };
public:
  static const  int PRIORITY_NOW        = -G_MAXINT / 2;                /* most important, used for immediate async execution */
  static const  int PRIORITY_HIGH       = G_PRIORITY_HIGH - 10;         /* very important, used for io handlers */
  static const  int PRIORITY_NEXT       = G_PRIORITY_HIGH - 5;          /* still very important, used for need-to-be-async operations */
  static const  int PRIORITY_NOTIFY     = G_PRIORITY_DEFAULT - 1;       /* important, delivers async signals */
  static const  int PRIORITY_NORMAL     = G_PRIORITY_DEFAULT;           /* normal importantance, interfaces to all layers */
  static const  int PRIORITY_UPDATE     = G_PRIORITY_HIGH_IDLE + 5;     /* mildly important, used for GUI updates or user information */
  static const  int PRIORITY_BACKGROUND = G_PRIORITY_LOW + 500;         /* unimportant, used when everything else done */
  static uint64 get_current_time_usecs(); // FIXME: should move this to birnetutilsxx.hh
  virtual void  quit            (void) = 0;
  virtual void  wakeup          (void) = 0;
  virtual bool  running         (void) = 0;
  virtual uint  add_source      (int             priority,
                                 Source         *loop_source) = 0;
  virtual bool  try_remove      (uint            id) = 0;
  void          remove          (uint            id);
  uint          exec_now        (const VoidSlot &sl) { return add_source (PRIORITY_HIGH,       new TimedSource (*sl.get_trampoline())); }
  uint          exec_now        (const BoolSlot &sl) { return add_source (PRIORITY_HIGH,       new TimedSource (*sl.get_trampoline())); }
  uint          exec_next       (const VoidSlot &sl) { return add_source (PRIORITY_NEXT,       new TimedSource (*sl.get_trampoline())); }
  uint          exec_next       (const BoolSlot &sl) { return add_source (PRIORITY_NEXT,       new TimedSource (*sl.get_trampoline())); }
  uint          exec_notify     (const VoidSlot &sl) { return add_source (PRIORITY_NOTIFY,     new TimedSource (*sl.get_trampoline())); }
  uint          exec_notify     (const BoolSlot &sl) { return add_source (PRIORITY_NOTIFY,     new TimedSource (*sl.get_trampoline())); }
  uint          exec_normal     (const VoidSlot &sl) { return add_source (PRIORITY_NORMAL,     new TimedSource (*sl.get_trampoline())); }
  uint          exec_normal     (const BoolSlot &sl) { return add_source (PRIORITY_NORMAL,     new TimedSource (*sl.get_trampoline())); }
  uint          exec_update     (const VoidSlot &sl) { return add_source (PRIORITY_UPDATE,     new TimedSource (*sl.get_trampoline())); }
  uint          exec_update     (const BoolSlot &sl) { return add_source (PRIORITY_UPDATE,     new TimedSource (*sl.get_trampoline())); }
  uint          exec_background (const VoidSlot &sl) { return add_source (PRIORITY_BACKGROUND, new TimedSource (*sl.get_trampoline())); }
  uint          exec_background (const BoolSlot &sl) { return add_source (PRIORITY_BACKGROUND, new TimedSource (*sl.get_trampoline())); }
  uint          exec_timer      (uint            initial_timeout_ms,
                                 uint            repeat_timeout_ms,
                                 const VoidSlot &sl) { return add_source (PRIORITY_NEXT,       new TimedSource (*sl.get_trampoline(), initial_timeout_ms, repeat_timeout_ms)); }
  uint          exec_timer      (uint            initial_timeout_ms,
                                 uint            repeat_timeout_ms,
                                 const BoolSlot &sl) { return add_source (PRIORITY_NEXT,       new TimedSource (*sl.get_trampoline(), initial_timeout_ms, repeat_timeout_ms)); }
};

class MainLoopPool {
  static void           rapicorn_init   ();
  friend void           rapicorn_init   (int*, char***, const char*);
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
  static Mutex          m_mutex;
  static MainLoop*      pop_loop        ()                      { AutoLocker locker (m_mutex); return m_singleton->pop_loop(); }
  static void           push_loop       (MainLoop *mloop)       { AutoLocker locker (m_mutex); return m_singleton->push_loop (mloop); }
public:
  static void           add_loop        (MainLoop *mloop)       { AutoLocker locker (m_mutex); return m_singleton->add_loop (mloop); }
  static void           set_n_threads   (uint      n)           { AutoLocker locker (m_mutex); return m_singleton->set_n_threads (n); }
  static uint           get_n_threads   (void)                  { AutoLocker locker (m_mutex); return m_singleton->get_n_threads(); }
  static void           quit_loops      ()                      { AutoLocker locker (m_mutex); return m_singleton->quit_loops(); }
};

// FIXME:
MainLoop*    glib_loop_create (void);


} // Rapicorn

#endif  /* __RAPICORN_LOOP_HH__ */
