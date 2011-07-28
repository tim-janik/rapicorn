// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_LOOP_HH__
#define __RAPICORN_LOOP_HH__

#include <ui/primitives.hh>

namespace Rapicorn {

// === PollFD ===
struct PollFD   /// Mirrors struct pollfd for poll(3posix)
{
  int           fd;
  uint16        events;
  uint16        revents;
  /// Event types that can be polled for, set in .events, updated in .revents
  enum {
    IN          = RAPICORN_SYSVAL_POLLIN,       ///< RDNORM || RDBAND
    PRI         = RAPICORN_SYSVAL_POLLPRI,      ///< urgent data available
    OUT         = RAPICORN_SYSVAL_POLLOUT,      ///< writing data will not block
    RDNORM      = RAPICORN_SYSVAL_POLLRDNORM,   ///< reading data will not block
    RDBAND      = RAPICORN_SYSVAL_POLLRDBAND,   ///< reading priority data will not block
    WRNORM      = RAPICORN_SYSVAL_POLLWRNORM,   ///< writing data will not block
    WRBAND      = RAPICORN_SYSVAL_POLLWRBAND,   ///< writing priority data will not block
    /* Event types updated in .revents regardlessly */
    ERR         = RAPICORN_SYSVAL_POLLERR,      ///< error condition
    HUP         = RAPICORN_SYSVAL_POLLHUP,      ///< file descriptor closed
    NVAL        = RAPICORN_SYSVAL_POLLNVAL,     ///< invalid PollFD
  };
};

// === EventFd ===
class EventFd   /// Wakeup facility for IPC.
{
  int      fds[2];
public:
  explicit EventFd   ();
  int      open      (); ///< Opens the eventfd and returns -errno.
  bool     opened    (); ///< Indicates whether eventfd has been opened.
  void     wakeup    (); ///< Wakeup polling end.
  int      inputfd   (); ///< Returns the file descriptor for POLLIN.
  bool     pollin    (); ///< Checks whether events are pending.
  void     flush     (); ///< Clear pending wakeups.
  /*Des*/ ~EventFd   ();
};

// === EventLoop ===
class MainLoop;
class EventLoop : public virtual BaseObject /// Loop object, polling for events and executing callbacks in accordance.
{
  class TimedSource;
  class PollFDSource;
  friend           class MainLoop;
public:
  class Source;
protected:
  typedef std::list<Source*>        SourceList;
  typedef std::map<int, SourceList> SourceListMap;
  MainLoop     &m_main_loop;
  SourceListMap m_sources;
  explicit      EventLoop        (MainLoop&);
  virtual      ~EventLoop        ();
  Source*       find_first_L     ();
  Source*       find_source_L    (uint id);
  bool          has_primary_L    (void);
  void          remove_source_Lm (Source *source);
  void          kill_sources_Lm  (void);
  bool          prepare_sources_Lm  (int*, vector<PollFD>&, int64*);
  bool          check_sources_Lm    (int*, const vector<PollFD>&);
  void          dispatch_sources_Lm (int);
  static uint64 get_current_time_usecs();
  typedef Signals::Slot1<void,PollFD&> VPfdSlot;
  typedef Signals::Slot1<bool,PollFD&> BPfdSlot;
public:
  static const int PRIORITY_NOW        = -1073741824;   ///< Most important, used for immediate async execution (MAXINT/2)
  static const int PRIORITY_HIGH       = -100 - 10;     ///< Very important, used for io handlers (G*HIGH)
  static const int PRIORITY_NEXT       = -100 - 5;      ///< Still very important, used for need-to-be-async operations (G*HIGH)
  static const int PRIORITY_NOTIFY     =    0 - 1;      ///< Important, delivers async signals (G*DEFAULT)
  static const int PRIORITY_NORMAL     =    0;          ///< Normal importantance, interfaces to all layers (G*DEFAULT)
  static const int PRIORITY_UPDATE     = +100 + 5;      ///< Mildly important, used for GUI updates or user information (G*HIGH_IDLE)
  static const int PRIORITY_IDLE       = +200;          ///< Mildly important, used for GUI updates or user information (G*DEFAULT_IDLE)
  static const int PRIORITY_BACKGROUND = +300 + 500;    ///< Unimportant, used when everything else done (G*LOW)
  void wakeup   ();                                     ///< Wakeup loop from polling.
  // source handling
  uint add      (Source *loop_source,
                 int priority = PRIORITY_IDLE); ///< Adds a new source to the loop with custom priority.
  bool try_remove      (uint            id);    ///< Tries to remove a source, returns if successfull.
  void remove          (uint            id);    ///< Removes a source from loop, the source must be present.
  void kill_sources    (void);                  ///< Remove all sources from this loop, prevents all further execution.
  bool has_primary     (void);                  ///< Indicates whether loop contains primary sources.
  uint exec_now        (const VoidSlot &sl);    ///< Execute a callback with priority "now" (highest).
  uint exec_now        (const BoolSlot &sl);    ///< Executes callback as exec_now(), returning true repeats callback.
  uint exec_next       (const VoidSlot &sl);    ///< Execute a callback with priority "next" (very important).
  uint exec_next       (const BoolSlot &sl);    ///< Executes callback as exec_next(), returning true repeats callback.
  uint exec_notify     (const VoidSlot &sl);    ///< Execute a callback with priority "notify" (important, for async signals).
  uint exec_notify     (const BoolSlot &sl);    ///< Executes callback as exec_notify(), returning true repeats callback.
  uint exec_normal     (const VoidSlot &sl);    ///< Execute a callback with normal priority (round-robin for all events and requests).
  uint exec_normal     (const BoolSlot &sl);    ///< Executes callback as exec_normal(), returning true repeats callback.
  uint exec_update     (const VoidSlot &sl);    ///< Execute a callback with priority "update" (important idle).
  uint exec_update     (const BoolSlot &sl);    ///< Executes callback as exec_update(), returning true repeats callback.
  uint exec_background (const VoidSlot &sl);    ///< Execute a callback with background priority (when idle).
  uint exec_background (const BoolSlot &sl);    ///< Executes callback as exec_background(), returning true repeats callback.
  uint exec_timer      (uint            timeout_ms,
                        const VoidSlot &sl,
                        int             priority = PRIORITY_NEXT); ///< Execute a callback after a specified timeout.
  uint exec_timer      (uint            initial_timeout_ms,
                        uint            repeat_timeout_ms,
                        const BoolSlot &sl,
                        int             priority = PRIORITY_NEXT); ///< Add exec_timer() callback, returning true repeats callback.
  uint exec_io_handler (const VPfdSlot &sl,
                        int             fd,
                        const String   &mode,
                        int             priority = PRIORITY_NORMAL); ///< Execute a callback after polling for mode on fd.
  uint exec_io_handler (const BPfdSlot &sl,
                        int             fd,
                        const String   &mode,
                        int             priority = PRIORITY_NORMAL); ///< Add exec_io_handler() callback, returning true repeats callback.
};

// === MainLoop ===
class MainLoop : public EventLoop /// An EventLoop implementation that offers public API for running the loop.
{
  friend                class EventLoop;
  friend                class SlaveLoop;
  vector<EventLoop*>    m_loops;
  EventFd               m_eventfd;
  uint                  m_generation;
  Mutex                 m_mutex;
  void                  wakeup_poll         ();                 ///< Wakeup main loop from polling.
  void                  add_loop_L          (EventLoop &loop);  ///< Adds a slave loop to this main loop.
  void                  remove_loop_L       (EventLoop &loop);  ///< Removes a slave loop from this main loop.
  bool                  iterate_loops       (bool b, bool d);
  explicit              MainLoop            ();
public:
  virtual      ~MainLoop        ();
  void          kill_loops      (); ///< Kill all sources in this loop and all slave loops.
  bool          exitable        (); ///< Indicates wether this loop can be auto-exited, i.e. no primary sources are present.
  void          iterate         (bool block = true); ///< Perform pending() & dispatch() in onse step.
  bool          pending         (bool block = true); ///< Checks whether the main or any slave loop need dispatching, blocks if permitted.
  void          dispatch        (); ///< Dispatches all pending events in the main or any slave loop.
  EventLoop*    new_slave       (); ///< Creates a new slave loop that is run as part of this main loop.
  static MainLoop*  _new        (); ///< Creates a new main loop object, users can run an exit this loop directly.
  inline Mutex& mutex           () { return m_mutex; } ///< mutex associated with this main loop.
};

// === EventLoop::Source ===
class EventLoop::Source : public virtual ReferenceCountable, protected NonCopyable /// EventLoop source for callback execution.
{
  friend        class EventLoop;
protected:
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
  explicit     Source      ();
  uint         source_id   () { return m_main_loop ? m_id : 0; }
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
  void         loop_remove ();
};

// === EventLoop::TimedSource ===
class EventLoop::TimedSource : public virtual EventLoop::Source /// EventLoop source for timer execution.
{
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

// === EventLoop::PollFDSource ===
class EventLoop::PollFDSource : public virtual EventLoop::Source /// EventLoop source for IO callbacks.
{
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

// === EventLoop methods ===
inline uint
EventLoop::exec_now (const VoidSlot &sl)
{
  return add (new TimedSource (*sl.get_trampoline()), PRIORITY_NOW);
}

inline uint
EventLoop::exec_now (const BoolSlot &sl)
{
  return add (new TimedSource (*sl.get_trampoline()), PRIORITY_NOW);
}

inline uint
EventLoop::exec_next (const VoidSlot &sl)
{
  return add (new TimedSource (*sl.get_trampoline()), PRIORITY_NEXT);
}

inline uint
EventLoop::exec_next (const BoolSlot &sl)
{
  return add (new TimedSource (*sl.get_trampoline()), PRIORITY_NEXT);
}

inline uint
EventLoop::exec_notify (const VoidSlot &sl)
{
  return add (new TimedSource (*sl.get_trampoline()), PRIORITY_NOTIFY);
}

inline uint
EventLoop::exec_notify (const BoolSlot &sl)
{
  return add (new TimedSource (*sl.get_trampoline()), PRIORITY_NOTIFY);
}

inline uint
EventLoop::exec_normal (const VoidSlot &sl)
{
  return add (new TimedSource (*sl.get_trampoline()), PRIORITY_NORMAL);
}

inline uint
EventLoop::exec_normal (const BoolSlot &sl)
{
  return add (new TimedSource (*sl.get_trampoline()), PRIORITY_NORMAL);
}

inline uint
EventLoop::exec_update (const VoidSlot &sl)
{
  return add (new TimedSource (*sl.get_trampoline()), PRIORITY_UPDATE);
}

inline uint
EventLoop::exec_update (const BoolSlot &sl)
{
  return add (new TimedSource (*sl.get_trampoline()), PRIORITY_UPDATE);
}

inline uint
EventLoop::exec_background (const VoidSlot &sl)
{
  return add (new TimedSource (*sl.get_trampoline()), PRIORITY_BACKGROUND);
}

inline uint
EventLoop::exec_background (const BoolSlot &sl)
{
  return add (new TimedSource (*sl.get_trampoline()), PRIORITY_BACKGROUND);
}

inline uint
EventLoop::exec_timer (uint            timeout_ms,
                       const VoidSlot &sl,
                       int             priority)
{
  return add (new TimedSource (*sl.get_trampoline(), timeout_ms, timeout_ms), priority);
}

inline uint
EventLoop::exec_timer (uint            initial_timeout_ms,
                       uint            repeat_timeout_ms,
                       const BoolSlot &sl,
                       int             priority)
{
  return add (new TimedSource (*sl.get_trampoline(), initial_timeout_ms, repeat_timeout_ms), priority);
}

inline uint
EventLoop::exec_io_handler (const VPfdSlot &sl,
                            int             fd,
                            const String   &mode,
                            int             priority)
{
  return add (new PollFDSource (*sl.get_trampoline(), fd, mode), priority);
}

inline uint
EventLoop::exec_io_handler (const BPfdSlot &sl,
                            int             fd,
                            const String   &mode,
                            int             priority)
{
  return add (new PollFDSource (*sl.get_trampoline(), fd, mode), priority);
}

} // Rapicorn

#endif  /* __RAPICORN_LOOP_HH__ */
