// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_LOOP_HH__
#define __RAPICORN_LOOP_HH__

#include <rcore/utilities.hh>
#include <rcore/rapicornthread.hh>
#include <rcore/rapicornsignal.hh>


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
  class DispatcherSource;
  class QuickPfdArray;          // pseudo vector<PollFD>
  class QuickSourceArray;       // pseudo vector<Source*>
  friend class MainLoop;
public:
  class Source;
  class State;
protected:
  typedef std::vector<Source*>    SourceList;
  MainLoop     &m_main_loop;
  SourceList    m_sources;
  int64         m_dispatch_priority;
  QuickSourceArray &m_poll_sources;
  uint64        pollmem1[3];
  Source*       pollmem2[7];
  bool          m_primary;
  explicit      EventLoop        (MainLoop&);
  virtual      ~EventLoop        ();
  Source*       find_first_L     ();
  Source*       find_source_L    (uint id);
  bool          has_primary_L    (void);
  void          remove_source_Lm (Source *source);
  void          kill_sources_Lm  (void);
  void          unpoll_sources_U    ();
  void          collect_sources_Lm  (State&);
  bool          prepare_sources_Lm  (State&, int64*, QuickPfdArray&);
  bool          check_sources_Lm    (State&, const QuickPfdArray&);
  Source*       dispatch_source_Lm  (State&);
  typedef Signals::Slot1<void,PollFD&>      VPfdSlot;
  typedef Signals::Slot1<bool,PollFD&>      BPfdSlot;
  typedef Signals::Slot1<bool,const State&> DispatcherSlot;
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
  void remove          (uint            id);    ///< Removes a source from loop, the source must be present.
  bool try_remove      (uint            id);    ///< Tries to remove a source, returns if successfull.
  void kill_sources    (void);                  ///< Remove all sources from this loop, prevents all further execution.
  bool has_primary     (void);                  ///< Indicates whether loop contains primary sources.
  bool flag_primary    (bool            on);
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
  uint exec_dispatcher (const DispatcherSlot &sl,
                        int    priority = PRIORITY_NORMAL); ///< Execute a single dispatcher callback for prepare, check, dispatch.
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
  MainLoop* main_loop  () const { return &m_main_loop; } ///< Get the main loop for this loop.
};

// === MainLoop ===
class MainLoop : public EventLoop /// An EventLoop implementation that offers public API for running the loop.
{
  friend                class EventLoop;
  friend                class SlaveLoop;
  Mutex                 m_mutex;
  vector<EventLoop*>    m_loops;
  EventFd               m_eventfd;
  uint                  m_rr_index;
  bool                  m_running;
  int                   m_quit_code;
  bool                  finishable_L        ();
  void                  wakeup_poll         ();                 ///< Wakeup main loop from polling.
  void                  add_loop_L          (EventLoop &loop);  ///< Adds a slave loop to this main loop.
  void                  remove_loop_L       (EventLoop &loop);  ///< Removes a slave loop from this main loop.
  bool                  iterate_loops_Lm    (State&, bool b, bool d);
  explicit              MainLoop            ();
public:
  virtual   ~MainLoop        ();
  int        run             (); ///< Run loop iterations until a call to quit() or finishable becomes true.
  void       quit            (int quit_code = 0); ///< Cause run() to return with @a quit_code.
  bool       finishable      (); ///< Indicates wether this loop has no primary sources left to process.
  bool       iterate         (bool block); ///< Perform one loop iteration and return whether more iterations are needed.
  void       iterate_pending (); ///< Call iterate() until no immediate dispatching is needed.
  void       kill_loops      (); ///< Kill all sources in this loop and all slave loops.
  EventLoop* new_slave       (); ///< Creates a new slave loop that is run as part of this main loop.
  static MainLoop*  _new     (); ///< Creates a new main loop object, users can run or iterate this loop directly.
  inline Mutex&     mutex    () { return m_mutex; } ///< Provide access to the mutex associated with this main loop.
  ///@cond
  struct LockHooks { bool (*sense) (); void (*lock) (); void (*unlock) (); };
  void              set_lock_hooks      (const LockHooks &hooks);
private: LockHooks  m_lock_hooks;
  ///@endcond
};

// === EventLoop::State ===
struct EventLoop::State {
  enum Phase { NONE, COLLECT, PREPARE, CHECK, DISPATCH, DESTROY };
  uint64 current_time_usecs;
  Phase  phase;
  bool   seen_primary; ///< Useful as hint for primary source presence, MainLoop::finishable() checks exhaustively
  State();
};

// === EventLoop::Source ===
class EventLoop::Source : public virtual ReferenceCountable, protected NonCopyable /// EventLoop source for callback execution.
{
  friend        class EventLoop;
protected:
  EventLoop   *m_loop;
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
  uint         source_id   () { return m_loop ? m_id : 0; }
public:
  virtual     ~Source      ();
  virtual bool prepare     (const State &state,
                            int64 *timeout_usecs_p) = 0;    ///< Prepare the source for dispatching (true return) or polling (false).
  virtual bool check       (const State &state) = 0;        ///< Check the source and its PollFD descriptors for dispatching (true return).
  virtual bool dispatch    (const State &state) = 0;        ///< Dispatch source, returns if it should be kept alive.
  virtual void destroy     ();
  bool         recursion   () const;                        ///< Indicates wether the source is currently in recursion.
  bool         may_recurse () const;                        ///< Indicates if this source may recurse.
  void         may_recurse (bool           may_recurse);    ///< Dispatch this source if its running recursively.
  bool         primary     () const;                        ///< Indicate whether this source is primary.
  void         primary     (bool           is_primary);     ///< Set whether this source prevents its loop from exiting.
  void         add_poll    (PollFD * const pfd);            ///< Add a PollFD descriptors for poll(2) and check().
  void         remove_poll (PollFD * const pfd);            ///< Remove a previously added PollFD.
  void         loop_remove ();                              ///< Remove this source from its event loop if any.
  MainLoop*    main_loop   () const { return m_loop ? m_loop->main_loop() : NULL; } ///< Get the main loop for this source.
};

// === EventLoop::DispatcherSource ===
class EventLoop::DispatcherSource : public virtual EventLoop::Source /// EventLoop source for timer execution.
{
  Signals::Trampoline1<bool,const State&> *m_trampoline;
protected:
  virtual     ~DispatcherSource ();
  virtual bool prepare          (const State &state, int64 *timeout_usecs_p);
  virtual bool check            (const State &state);
  virtual bool dispatch         (const State &state);
  virtual void destroy          ();
public:
  explicit     DispatcherSource (Signals::Trampoline1<bool,const State&> &tr);
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
  virtual bool prepare      (const State &state,
                             int64 *timeout_usecs_p);
  virtual bool check        (const State &state);
  virtual bool dispatch     (const State &state);
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
  virtual bool  prepare         (const State &state,
                                 int64 *timeout_usecs_p);
  virtual bool  check           (const State &state);
  virtual bool  dispatch        (const State &state);
  virtual void  destroy         ();
  PollFD        m_pfd;
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
EventLoop::exec_dispatcher (const DispatcherSlot &sl, int priority)
{
  return add (new DispatcherSource (*sl.get_trampoline()), priority);
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
