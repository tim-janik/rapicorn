/* Rapicorn
 * Copyright (C) 2006-2007 Tim Janik
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
#ifndef __RAPICORN_LOOP_HH__
#define __RAPICORN_LOOP_HH__

#include <ui/primitives.hh>

namespace Rapicorn {

struct PollFD { ///< Mirrors struct pollfd for poll(3posix)
  int           fd;
  uint16        events;
  uint16        revents;
  enum {
    /* Event types that can be polled for, set in .events, updated in .revents */
    IN          = RAPICORN_SYSVAL_POLLIN,         /* RDNORM || RDBAND */
    PRI         = RAPICORN_SYSVAL_POLLPRI,        /* urgent data available */
    OUT         = RAPICORN_SYSVAL_POLLOUT,        /* writing data will not block */
    RDNORM      = RAPICORN_SYSVAL_POLLRDNORM,     /* reading data will not block */
    RDBAND      = RAPICORN_SYSVAL_POLLRDBAND,     /* reading priority data will not block */
    WRNORM      = RAPICORN_SYSVAL_POLLWRNORM,     /* writing data will not block */
    WRBAND      = RAPICORN_SYSVAL_POLLWRBAND,     /* writing priority data will not block */
    /* Event types updated in .revents regardlessly */
    ERR         = RAPICORN_SYSVAL_POLLERR,        /* error condition */
    HUP         = RAPICORN_SYSVAL_POLLHUP,        /* file descriptor closed */
    NVAL        = RAPICORN_SYSVAL_POLLNVAL,       /* invalid PollFD */
  };
};

class EventFd { ///< Wakeup facility for IPC.
  int      fds[2];
public:
  explicit EventFd   ();
  int      open      (); ///< @Returns -errno.
  void     wakeup    (); ///< Wakeup polling end.
  int      inputfd   (); ///< @Returns fd for POLLIN.
  void     flush     (); ///< Clear pending wakeups.
  /*Des*/ ~EventFd   ();
};

// === EventLoop
class EventLoop : public virtual BaseObject {
  friend class RapicornTester;
  class TimedSource;
  class PollFDSource;
protected:
  typedef Signals::Slot1<void,PollFD&> VPfdSlot;
  typedef Signals::Slot1<bool,PollFD&> BPfdSlot;
  explicit         EventLoop  ();
  virtual         ~EventLoop  ();
  static uint64    get_current_time_usecs();    // FIXME: current_time should move to utilities.hh
public:
  static const int PRIORITY_NOW        = -1073741824;   /* most important, used for immediate async execution (MAXINT/2) */
  static const int PRIORITY_HIGH       = -100 - 10;     /* very important, used for io handlers (G*HIGH) */
  static const int PRIORITY_NEXT       = -100 - 5;      /* still very important, used for need-to-be-async operations (G*HIGH) */
  static const int PRIORITY_NOTIFY     =    0 - 1;      /* important, delivers async signals (G*DEFAULT) */
  static const int PRIORITY_NORMAL     =    0;          /* normal importantance, interfaces to all layers (G*DEFAULT) */
  static const int PRIORITY_UPDATE     = +100 + 5;      /* mildly important, used for GUI updates or user information (G*HIGH_IDLE) */
  static const int PRIORITY_IDLE       = +200;          /* mildly important, used for GUI updates or user information (G*DEFAULT_IDLE) */
  static const int PRIORITY_BACKGROUND = +300 + 500;    /* unimportant, used when everything else done (G*LOW) */
  static EventLoop* create      ();
  /* run state */
  static bool   iterate_loops   (bool may_block,
                                 bool may_dispatch);
  static void   kill_loops      ();
  static bool   loops_exitable  ();
  virtual void  kill_sources    (void) = 0;
  virtual bool  has_primary     (void) = 0;
  virtual void  wakeup          (void) = 0;             /* thread-safe, as long as loop is undestructed */
  /* source handling */
  class Source;
  virtual uint  add_source      (Source         *loop_source,
                                 int             priority = PRIORITY_IDLE) = 0;
  virtual bool  try_remove      (uint            id) = 0;
  void          remove          (uint            id);
  uint          exec_now        (const VoidSlot &sl);
  uint          exec_now        (const BoolSlot &sl);
  uint          exec_next       (const VoidSlot &sl);
  uint          exec_next       (const BoolSlot &sl);
  uint          exec_notify     (const VoidSlot &sl);
  uint          exec_notify     (const BoolSlot &sl);
  uint          exec_normal     (const VoidSlot &sl);
  uint          exec_normal     (const BoolSlot &sl);
  uint          exec_update     (const VoidSlot &sl);
  uint          exec_update     (const BoolSlot &sl);
  uint          exec_background (const VoidSlot &sl);
  uint          exec_background (const BoolSlot &sl);
  uint          exec_timer      (uint            timeout_ms,
                                 const VoidSlot &sl,
                                 int             priority = PRIORITY_NEXT);
  uint          exec_timer      (uint            initial_timeout_ms,
                                 uint            repeat_timeout_ms,
                                 const BoolSlot &sl,
                                 int             priority = PRIORITY_NEXT);
  uint          exec_io_handler (const VPfdSlot &sl,
                                 int             fd,
                                 const String   &mode,
                                 int             priority = PRIORITY_NORMAL);
  uint          exec_io_handler (const BPfdSlot &sl,
                                 int             fd,
                                 const String   &mode,
                                 int             priority = PRIORITY_NORMAL);
};

/* --- EventLoop::Source --- */
class EventLoop::Source : public virtual ReferenceCountable, protected NonCopyable {
  friend       class EventLoopImpl;
  EventLoop    *m_main_loop;
  struct {
    PollFD    *pfd;
    uint       idx;
  }           *m_pfds;
  uint         m_id;
  int          m_priority;
  uint16       m_loop_state;
  uint         m_may_recurse : 1;
  uint         m_dispatching : 1;
  uint         m_was_dispatching : 1;
  uint         m_primary : 1;
  uint         n_pfds      ();
protected:
  explicit     Source      ();
  uint         source_id   () { return m_main_loop ? m_id : 0; }
  void         loop_remove ();
public:
  virtual     ~Source      ();
  virtual bool prepare     (uint64 current_time_usecs,
                            int64 *timeout_usecs_p) = 0;
  virtual bool check       (uint64 current_time_usecs) = 0;
  virtual bool dispatch    () = 0;
  virtual void destroy     ();
  bool         may_recurse () const;
  void         may_recurse (bool           may_recurse);
  bool         primary     () const;
  void         primary     (bool           is_primary);
  bool         recursion   () const;
  void         add_poll    (PollFD * const pfd);
  void         remove_poll (PollFD * const pfd);
};

/* --- EventLoop::TimedSource --- */
class EventLoop::TimedSource : public virtual EventLoop::Source {
  uint64     m_expiration_usecs;
  uint       m_interval_msecs;
  bool       m_first_interval;
  const bool m_oneshot;
  union {
    Signals::Trampoline0<bool> *m_btrampoline;
    Signals::Trampoline0<void> *m_vtrampoline;
  };
protected:
  virtual     ~TimedSource  ();
  virtual bool prepare      (uint64 current_time_usecs,
                             int64 *timeout_usecs_p);
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

/* --- EventLoop::PollFDSource --- */
class EventLoop::PollFDSource : public virtual EventLoop::Source {
protected:
  void          construct       (const String &mode);
  virtual      ~PollFDSource    ();
  virtual bool  prepare         (uint64 current_time_usecs,
                                 int64 *timeout_usecs_p);
  virtual bool  check           (uint64 current_time_usecs);
  virtual bool  dispatch        ();
  virtual void  destroy         ();
  PollFD        m_pfd;
  /* w - poll writable
   * r - poll readable
   * p - poll urgent redable
   * b - blocking
   * E - ignore erros (or auto destroy)
   * H - ignore hangup (or auto destroy)
   * C - prevent auto close on destroy
   */
  uint          m_ignore_errors : 1;    // 'E'
  uint          m_ignore_hangup : 1;    // 'H'
  uint          m_never_close : 1;      // 'C'
private:
  const uint    m_oneshot : 1;
  union {
    Signals::Trampoline1<bool,PollFD&> *m_btrampoline;
    Signals::Trampoline1<void,PollFD&> *m_vtrampoline;
  };
public:
  explicit      PollFDSource    (Signals::Trampoline1<bool,PollFD&> &bt,
                                 int                                 fd,
                                 const String                       &mode);
  explicit      PollFDSource    (Signals::Trampoline1<void,PollFD&> &vt,
                                 int                                 fd,
                                 const String                       &mode);
};

/* --- EventLoop methods --- */
inline uint
EventLoop::exec_now (const VoidSlot &sl)
{
  return add_source (new TimedSource (*sl.get_trampoline()), PRIORITY_NOW);
}

inline uint
EventLoop::exec_now (const BoolSlot &sl)
{
  return add_source (new TimedSource (*sl.get_trampoline()), PRIORITY_NOW);
}

inline uint
EventLoop::exec_next (const VoidSlot &sl)
{
  return add_source (new TimedSource (*sl.get_trampoline()), PRIORITY_NEXT);
}

inline uint
EventLoop::exec_next (const BoolSlot &sl)
{
  return add_source (new TimedSource (*sl.get_trampoline()), PRIORITY_NEXT);
}

inline uint
EventLoop::exec_notify (const VoidSlot &sl)
{
  return add_source (new TimedSource (*sl.get_trampoline()), PRIORITY_NOTIFY);
}

inline uint
EventLoop::exec_notify (const BoolSlot &sl)
{
  return add_source (new TimedSource (*sl.get_trampoline()), PRIORITY_NOTIFY);
}

inline uint
EventLoop::exec_normal (const VoidSlot &sl)
{
  return add_source (new TimedSource (*sl.get_trampoline()), PRIORITY_NORMAL);
}

inline uint
EventLoop::exec_normal (const BoolSlot &sl)
{
  return add_source (new TimedSource (*sl.get_trampoline()), PRIORITY_NORMAL);
}

inline uint
EventLoop::exec_update (const VoidSlot &sl)
{
  return add_source (new TimedSource (*sl.get_trampoline()), PRIORITY_UPDATE);
}

inline uint
EventLoop::exec_update (const BoolSlot &sl)
{
  return add_source (new TimedSource (*sl.get_trampoline()), PRIORITY_UPDATE);
}

inline uint
EventLoop::exec_background (const VoidSlot &sl)
{
  return add_source (new TimedSource (*sl.get_trampoline()), PRIORITY_BACKGROUND);
}

inline uint
EventLoop::exec_background (const BoolSlot &sl)
{
  return add_source (new TimedSource (*sl.get_trampoline()), PRIORITY_BACKGROUND);
}

inline uint
EventLoop::exec_timer (uint            timeout_ms,
                       const VoidSlot &sl,
                       int             priority)
{
  return add_source (new TimedSource (*sl.get_trampoline(), timeout_ms, timeout_ms), priority);
}

inline uint
EventLoop::exec_timer (uint            initial_timeout_ms,
                       uint            repeat_timeout_ms,
                       const BoolSlot &sl,
                       int             priority)
{
  return add_source (new TimedSource (*sl.get_trampoline(), initial_timeout_ms, repeat_timeout_ms), priority);
}

inline uint
EventLoop::exec_io_handler (const VPfdSlot &sl,
                            int             fd,
                            const String   &mode,
                            int             priority)
{
  return add_source (new PollFDSource (*sl.get_trampoline(), fd, mode), priority);
}

inline uint
EventLoop::exec_io_handler (const BPfdSlot &sl,
                            int             fd,
                            const String   &mode,
                            int             priority)
{
  return add_source (new PollFDSource (*sl.get_trampoline(), fd, mode), priority);
}

} // Rapicorn

#endif  /* __RAPICORN_LOOP_HH__ */
