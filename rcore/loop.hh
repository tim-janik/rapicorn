// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_LOOP_HH__
#define __RAPICORN_LOOP_HH__

#include <rcore/objects.hh>
#include <rcore/thread.hh>


namespace Rapicorn {

using Aida::EventFd;

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

// === EventLoop ===
class EventLoop;
typedef std::shared_ptr<EventLoop> EventLoopP;
class MainLoop;
typedef std::shared_ptr<MainLoop> MainLoopP;
/// Loop object, polling for events and executing callbacks in accordance.
class EventLoop : public virtual std::enable_shared_from_this<EventLoop>
{
  class TimedSource;
  typedef std::shared_ptr<TimedSource> TimedSourceP;
  class PollFDSource;
  typedef std::shared_ptr<PollFDSource> PollFDSourceP;
  class DispatcherSource;
  typedef std::shared_ptr<DispatcherSource> DispatcherSourceP;
  class QuickPfdArray;          // pseudo vector<PollFD>
  friend class MainLoop;
public:
  class State;
  class Source;
  typedef std::shared_ptr<Source> SourceP;
protected:
  typedef std::vector<SourceP>    SourceList;
  MainLoop     *main_loop_;
  SourceList    sources_;
  int64         dispatch_priority_;
  vector<SourceP> poll_sources_;
  bool          primary_;
  explicit      EventLoop        (MainLoop&);
  virtual      ~EventLoop        ();
  SourceP&      find_first_L     ();
  SourceP&      find_source_L    (uint id);
  bool          has_primary_L    (void);
  void          remove_source_Lm (SourceP source);
  void          kill_sources_Lm  (void);
  void          unpoll_sources_U    ();
  void          collect_sources_Lm  (State&);
  bool          prepare_sources_Lm  (State&, int64*, QuickPfdArray&);
  bool          check_sources_Lm    (State&, const QuickPfdArray&);
  void          dispatch_source_Lm  (State&);
public:
  typedef std::function<void (void)>         VoidSlot;
  typedef std::function<bool (void)>         BoolSlot;
  typedef std::function<void (PollFD&)>      VPfdSlot;
  typedef std::function<bool (PollFD&)>      BPfdSlot;
  typedef std::function<bool (const State&)> DispatcherSlot;
  static const int PRIORITY_NOW        = -1073741824;   ///< Most important, used for immediate async execution (MAXINT/2)
  static const int PRIORITY_HIGH       = -100 - 10;     ///< Very important, used for e.g. io handlers (G*HIGH)
  static const int PRIORITY_NEXT       = -100 - 5;      ///< Still very important, used for need-to-be-async operations (G*HIGH)
  static const int PRIORITY_NOTIFY     =    0 - 1;      ///< Important, delivers async callbacks (G*DEFAULT)
  static const int PRIORITY_NORMAL     =    0;          ///< Normal importantance, interfaces to all layers (G*DEFAULT)
  static const int PRIORITY_UPDATE     = +100 + 5;      ///< Mildly important, used for GUI updates or user information (G*HIGH_IDLE)
  static const int PRIORITY_IDLE       = +200;          ///< Mildly important, used for GUI updates or user information (G*DEFAULT_IDLE)
  static const int PRIORITY_BACKGROUND = +300 + 500;    ///< Unimportant, used when everything else done (G*LOW)
  void wakeup   ();                                     ///< Wakeup loop from polling.
  // source handling
  uint add             (SourceP loop_source, int priority
                        = PRIORITY_NORMAL);     ///< Adds a new source to the loop with custom priority.
  void remove          (uint            id);    ///< Removes a source from loop, the source must be present.
  bool try_remove      (uint            id);    ///< Tries to remove a source, returns if successfull.
  void destroy_loop    (void);
  bool has_primary     (void);                  ///< Indicates whether loop contains primary sources.
  bool flag_primary    (bool            on);
  template<class BoolVoidFunctor>
  uint exec_now        (BoolVoidFunctor &&bvf); ///< Execute a callback as primary source with priority "now" (highest), returning true repeats callback.
  template<class BoolVoidFunctor>
  uint exec_next       (BoolVoidFunctor &&bvf); ///< Execute a callback with priority "next" (very important), returning true repeats callback.
  template<class BoolVoidFunctor>
  uint exec_notify     (BoolVoidFunctor &&bvf); ///< Execute a callback with priority "notify" (important, for async callbacks), returning true repeats callback.
  template<class BoolVoidFunctor>
  uint exec_normal     (BoolVoidFunctor &&bvf); ///< Execute a callback with normal priority (round-robin for all events and requests), returning true repeats callback.
  template<class BoolVoidFunctor>
  uint exec_update     (BoolVoidFunctor &&bvf); ///< Execute a callback with priority "update" (important idle), returning true repeats callback.
  template<class BoolVoidFunctor>
  uint exec_background (BoolVoidFunctor &&bvf); ///< Execute a callback with background priority (when idle), returning true repeats callback.
  MainLoop* main_loop  () const { return main_loop_; }  ///< Get the main loop for this loop.
  /// Execute a single dispatcher callback for prepare, check, dispatch.
  uint exec_dispatcher (const DispatcherSlot &sl, int priority = PRIORITY_NORMAL);
  /// Execute a callback after a specified timeout, returning true repeats callback.
  template<class BoolVoidFunctor>
  uint exec_timer      (uint timeout_ms, BoolVoidFunctor &&bvf, int priority = PRIORITY_NORMAL);
  /// Execute a callback after a specified timeout with adjustable initial timeout, returning true repeats callback.
  template<class BoolVoidFunctor>
  uint exec_timer      (uint initial_timeout_ms, uint repeat_timeout_ms, BoolVoidFunctor &&bvf, int priority = PRIORITY_NORMAL);
  /// Execute a callback after polling for mode on fd, returning true repeats callback.
  template<class BoolVoidPollFunctor>
  uint exec_io_handler (BoolVoidPollFunctor &&bvf, int fd, const String &mode, int priority = PRIORITY_NORMAL);
};

// === MainLoop ===
/// An EventLoop implementation that offers public API for running the loop.
class MainLoop : public EventLoop
{
  friend                class FriendAllocator<MainLoop>;
  friend                class EventLoop;
  friend                class SlaveLoop;
  Mutex                 mutex_;
  vector<EventLoopP>    loops_;
  EventFd               eventfd_;
  uint                  rr_index_;
  bool                  running_;
  int                   quit_code_;
  bool                  finishable_L        ();
  void                  wakeup_poll         ();                 ///< Wakeup main loop from polling.
  void                  add_loop_L          (EventLoop &loop);  ///< Adds a slave loop to this main loop.
  void                  kill_loop_Lm        (EventLoop &loop);  ///< Destroy a slave loop and all its sources.
  void                  kill_loops_Lm       ();                 ///< Destroy this loop and all slave loops.
  bool                  iterate_loops_Lm    (State&, bool b, bool d);
  explicit              MainLoop            ();
public:
  virtual   ~MainLoop        ();
  int        run             (); ///< Run loop iterations until a call to quit() or finishable becomes true.
  bool       running         (); ///< Indicates if quit() has been called already.
  void       quit            (int quit_code = 0); ///< Cause run() to return with @a quit_code.
  bool       finishable      (); ///< Indicates wether this loop has no primary sources left to process.
  bool       iterate         (bool block); ///< Perform one loop iteration and return whether more iterations are needed.
  void       iterate_pending (); ///< Call iterate() until no immediate dispatching is needed.
  EventLoopP create_slave    (); ///< Creates a new slave loop that is run as part of this main loop.
  static MainLoopP  create   ();
  inline Mutex&     mutex    () { return mutex_; } ///< Provide access to the mutex associated with this main loop.
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
class EventLoop::Source /// EventLoop source for callback execution.
{
  friend        class EventLoop;
  RAPICORN_CLASS_NON_COPYABLE (Source);
protected:
  EventLoop   *loop_;
  struct {
    PollFD    *pfd;
    uint       idx;
  }           *pfds_;
  uint         id_;
  int          priority_;
  uint16       loop_state_;
  uint         may_recurse_ : 1;
  uint         dispatching_ : 1;
  uint         was_dispatching_ : 1;
  uint         primary_ : 1;
  uint         n_pfds      ();
  explicit     Source      ();
  uint         source_id   () { return loop_ ? id_ : 0; }
  virtual     ~Source      ();
public:
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
  MainLoop*    main_loop   () const { return loop_ ? loop_->main_loop() : NULL; } ///< Get the main loop for this source.
};

// === EventLoop::DispatcherSource ===
class EventLoop::DispatcherSource : public virtual EventLoop::Source /// EventLoop source for timer execution.
{
  friend class FriendAllocator<DispatcherSource>;
  DispatcherSlot slot_;
protected:
  virtual     ~DispatcherSource ();
  virtual bool prepare          (const State &state, int64 *timeout_usecs_p);
  virtual bool check            (const State &state);
  virtual bool dispatch         (const State &state);
  virtual void destroy          ();
  explicit     DispatcherSource (const DispatcherSlot &slot);
public:
  static DispatcherSourceP create (const DispatcherSlot &slot)
  { return FriendAllocator<DispatcherSource>::make_shared (slot); }
};

// === EventLoop::TimedSource ===
class EventLoop::TimedSource : public virtual EventLoop::Source /// EventLoop source for timer execution.
{
  friend class FriendAllocator<TimedSource>;
  uint64     expiration_usecs_;
  uint       interval_msecs_;
  bool       first_interval_;
  const bool oneshot_;
  union {
    BoolSlot bool_slot_;
    VoidSlot void_slot_;
  };
protected:
  virtual     ~TimedSource  ();
  virtual bool prepare      (const State &state,
                             int64 *timeout_usecs_p);
  virtual bool check        (const State &state);
  virtual bool dispatch     (const State &state);
  explicit     TimedSource (const BoolSlot &slot, uint initial_interval_msecs, uint repeat_interval_msecs);
  explicit     TimedSource (const VoidSlot &slot, uint initial_interval_msecs, uint repeat_interval_msecs);
public:
  static TimedSourceP create (const BoolSlot &slot, uint initial_interval_msecs = 0, uint repeat_interval_msecs = 0)
  { return FriendAllocator<TimedSource>::make_shared (slot, initial_interval_msecs, repeat_interval_msecs); }
  static TimedSourceP create (const VoidSlot &slot, uint initial_interval_msecs = 0, uint repeat_interval_msecs = 0)
  { return FriendAllocator<TimedSource>::make_shared (slot, initial_interval_msecs, repeat_interval_msecs); }
};

// === EventLoop::PollFDSource ===
class EventLoop::PollFDSource : public virtual EventLoop::Source /// EventLoop source for IO callbacks.
{
  friend class FriendAllocator<PollFDSource>;
protected:
  void          construct       (const String &mode);
  virtual      ~PollFDSource    ();
  virtual bool  prepare         (const State &state,
                                 int64 *timeout_usecs_p);
  virtual bool  check           (const State &state);
  virtual bool  dispatch        (const State &state);
  virtual void  destroy         ();
  PollFD        pfd_;
  uint          ignore_errors_ : 1;    // 'E'
  uint          ignore_hangup_ : 1;    // 'H'
  uint          never_close_ : 1;      // 'C'
private:
  const uint    oneshot_ : 1;
  union {
    BPfdSlot bool_poll_slot_;
    VPfdSlot void_poll_slot_;
  };
  explicit      PollFDSource    (const BPfdSlot &slot, int fd, const String &mode);
  explicit      PollFDSource    (const VPfdSlot &slot, int fd, const String &mode);
public:
  static PollFDSourceP create (const BPfdSlot &slot, int fd, const String &mode)
  { return FriendAllocator<PollFDSource>::make_shared (slot, fd, mode); }
  static PollFDSourceP create (const VPfdSlot &slot, int fd, const String &mode)
  { return FriendAllocator<PollFDSource>::make_shared (slot, fd, mode); }
};

// === EventLoop methods ===
template<class BoolVoidFunctor> uint
EventLoop::exec_now (BoolVoidFunctor &&bvf)
{
  typedef decltype (bvf()) ReturnType;
  std::function<ReturnType()> slot (bvf);
  TimedSourceP tsource = TimedSource::create (slot);
  tsource->primary (true);
  return add (tsource, PRIORITY_NOW);
}

template<class BoolVoidFunctor> uint
EventLoop::exec_next (BoolVoidFunctor &&bvf)
{
  typedef decltype (bvf()) ReturnType;
  std::function<ReturnType()> slot (bvf);
  return add (TimedSource::create (slot), PRIORITY_NEXT);
}

template<class BoolVoidFunctor> uint
EventLoop::exec_notify (BoolVoidFunctor &&bvf)
{
  typedef decltype (bvf()) ReturnType;
  std::function<ReturnType()> slot (bvf);
  return add (TimedSource::create (slot), PRIORITY_NOTIFY);
}

template<class BoolVoidFunctor> uint
EventLoop::exec_normal (BoolVoidFunctor &&bvf)
{
  typedef decltype (bvf()) ReturnType;
  std::function<ReturnType()> slot (bvf);
  return add (TimedSource::create (slot), PRIORITY_NORMAL);
}

template<class BoolVoidFunctor> uint
EventLoop::exec_update (BoolVoidFunctor &&bvf)
{
  typedef decltype (bvf()) ReturnType;
  std::function<ReturnType()> slot (bvf);
  return add (TimedSource::create (slot), PRIORITY_UPDATE);
}

template<class BoolVoidFunctor> uint
EventLoop::exec_background (BoolVoidFunctor &&bvf)
{
  typedef decltype (bvf()) ReturnType;
  std::function<ReturnType()> slot (bvf);
  return add (TimedSource::create (slot), PRIORITY_BACKGROUND);
}

inline uint
EventLoop::exec_dispatcher (const DispatcherSlot &slot, int priority)
{
  return add (DispatcherSource::create (slot), priority);
}

template<class BoolVoidFunctor> uint
EventLoop::exec_timer (uint timeout_ms, BoolVoidFunctor &&bvf, int priority)
{
  typedef decltype (bvf()) ReturnType;
  std::function<ReturnType()> slot (bvf);
  return add (TimedSource::create (slot, timeout_ms, timeout_ms), priority);
}

template<class BoolVoidFunctor> uint
EventLoop::exec_timer (uint initial_timeout_ms, uint repeat_timeout_ms, BoolVoidFunctor &&bvf, int priority)
{
  typedef decltype (bvf()) ReturnType;
  std::function<ReturnType()> slot (bvf);
  return add (TimedSource::create (slot, initial_timeout_ms, repeat_timeout_ms), priority);
}

template<class BoolVoidPollFunctor> uint
EventLoop::exec_io_handler (BoolVoidPollFunctor &&bvf, int fd, const String &mode, int priority)
{
  typedef decltype (bvf (*(PollFD*) 0)) ReturnType;
  std::function<ReturnType (PollFD&)> slot (bvf);
  return add (PollFDSource::create (slot, fd, mode), priority);
}

} // Rapicorn

#endif  /* __RAPICORN_LOOP_HH__ */
