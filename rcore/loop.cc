// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#include "loop.hh"
#include "strings.hh"
#include <sys/poll.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <algorithm>
#include <list>
#include <cstring>

namespace Rapicorn {

enum {
  WAITING             = 0,
  PREPARED,
  NEEDS_DISPATCH,
};

// == PollFD invariants ==
RAPICORN_STATIC_ASSERT (PollFD::IN     == POLLIN);
RAPICORN_STATIC_ASSERT (PollFD::PRI    == POLLPRI);
RAPICORN_STATIC_ASSERT (PollFD::OUT    == POLLOUT);
RAPICORN_STATIC_ASSERT (PollFD::RDNORM == POLLRDNORM);
RAPICORN_STATIC_ASSERT (PollFD::RDBAND == POLLRDBAND);
RAPICORN_STATIC_ASSERT (PollFD::WRNORM == POLLWRNORM);
RAPICORN_STATIC_ASSERT (PollFD::WRBAND == POLLWRBAND);
RAPICORN_STATIC_ASSERT (PollFD::ERR    == POLLERR);
RAPICORN_STATIC_ASSERT (PollFD::HUP    == POLLHUP);
RAPICORN_STATIC_ASSERT (PollFD::NVAL   == POLLNVAL);
RAPICORN_STATIC_ASSERT (sizeof   (PollFD)               == sizeof   (struct pollfd));
RAPICORN_STATIC_ASSERT (offsetof (PollFD, fd)           == offsetof (struct pollfd, fd));
RAPICORN_STATIC_ASSERT (sizeof (((PollFD*) 0)->fd)      == sizeof (((struct pollfd*) 0)->fd));
RAPICORN_STATIC_ASSERT (offsetof (PollFD, events)       == offsetof (struct pollfd, events));
RAPICORN_STATIC_ASSERT (sizeof (((PollFD*) 0)->events)  == sizeof (((struct pollfd*) 0)->events));
RAPICORN_STATIC_ASSERT (offsetof (PollFD, revents)      == offsetof (struct pollfd, revents));
RAPICORN_STATIC_ASSERT (sizeof (((PollFD*) 0)->revents) == sizeof (((struct pollfd*) 0)->revents));

// === Stupid ID allocator ===
static Atomic<uint> static_id_counter = 65536;
static uint
alloc_id ()
{
  uint id = static_id_counter++;
  if (!id)
    fatal ("EventLoop: id counter overflow, please report"); // FIXME
  return id;
}

static void
release_id (uint id)
{
  assert_return (id != 0);
  // FIXME: ned proper ID allocator
}

// === QuickArray ===
template<class Data>
class QuickArray {
  Data  *data_;
  uint   n_elements_;
  uint   n_reserved_;
  Data  *reserved_;
  template<class D> void swap (D &a, D &b) { D t = a; a = b; b = t; }
public:
  typedef Data* iterator;
  QuickArray (uint n_reserved, Data *reserved) : data_ (reserved), n_elements_ (0), n_reserved_ (n_reserved), reserved_ (reserved) {}
  ~QuickArray()                         { if (LIKELY (data_) && UNLIKELY (data_ != reserved_)) free (data_); }
  uint        size       () const       { return n_elements_; }
  bool        empty      () const       { return n_elements_ == 0; }
  Data*       data       () const       { return data_; }
  Data&       operator[] (uint n)       { return data_[n]; }
  const Data& operator[] (uint n) const { return data_[n]; }
  iterator    begin      ()             { return &data_[0]; }
  iterator    end        ()             { return &data_[n_elements_]; }
  void        shrink     (uint n)       { n_elements_ = MIN (n_elements_, n); }
  void        swap       (QuickArray &o)
  {
    swap (data_,       o.data_);
    swap (n_elements_, o.n_elements_);
    swap (n_reserved_, o.n_reserved_);
    swap (reserved_,   o.reserved_);
  }
  void        push       (const Data &d)
  {
    const uint idx = n_elements_++;
    if (UNLIKELY (n_elements_ > n_reserved_))                 // reserved memory exceeded
      {
        const size_t sz = n_elements_ * sizeof (Data);
        const bool migrate = UNLIKELY (data_ == reserved_);   // migrate from reserved to malloced
        Data *mem = (Data*) (migrate ? malloc (sz) : realloc (data_, sz));
        if (UNLIKELY (!mem))
          fatal ("OOM");
        if (migrate)
          {
            memcpy (mem, data_, sz - 1 * sizeof (Data));
            reserved_ = NULL;
            n_reserved_ = 0;
          }
        data_ = mem;
      }
    data_[idx] = d;
  }
};
struct EventLoop::QuickPfdArray : public QuickArray<PollFD> {
  QuickPfdArray (uint n_reserved, PollFD *reserved) : QuickArray (n_reserved, reserved) {}
};
struct QuickSourcePArray : public QuickArray<EventLoop::SourceP*> {
  QuickSourcePArray (uint n_reserved, EventLoop::SourceP **reserved) : QuickArray (n_reserved, reserved) {}
};

// === EventLoop ===
EventLoop::EventLoop (MainLoop &main) :
  main_loop_ (main), dispatch_priority_ (0), primary_ (false)
{
  poll_sources_.reserve (7);
  // we cannot *use* main_loop_ yet, because we might be called from within MainLoop::MainLoop(), see SlaveLoop()
}

EventLoop::~EventLoop ()
{
  unpoll_sources_U();
  // we cannot *use* main_loop_ anymore, because we might be called from within MainLoop::MainLoop(), see ~SlaveLoop()
}

inline EventLoop::SourceP&
EventLoop::find_first_L()
{
  static SourceP null_source;
  return sources_.empty() ? null_source : sources_[0];
}

inline EventLoop::SourceP&
EventLoop::find_source_L (uint id)
{
  for (SourceList::iterator lit = sources_.begin(); lit != sources_.end(); lit++)
    if (id == (*lit)->id_)
      return *lit;
  static SourceP null_source;
  return null_source;
}

bool
EventLoop::has_primary_L()
{
  if (primary_)
    return true;
  for (SourceList::iterator lit = sources_.begin(); lit != sources_.end(); lit++)
    if ((*lit)->primary())
      return true;
  return false;
}

bool
EventLoop::has_primary()
{
  ScopedLock<Mutex> locker (main_loop_.mutex());
  return has_primary_L();
}

bool
EventLoop::flag_primary (bool on)
{
  ScopedLock<Mutex> locker (main_loop_.mutex());
  const bool was_primary = primary_;
  primary_ = on;
  if (primary_ != was_primary)
    wakeup();
  return was_primary;
}

uint
EventLoop::add (SourceP source, int priority)
{
  ScopedLock<Mutex> locker (main_loop_.mutex());
  assert_return (source->loop_ == NULL, 0);
  source->loop_ = this;
  source->id_ = alloc_id();
  source->loop_state_ = WAITING;
  source->priority_ = priority;
  sources_.push_back (source);
  locker.unlock();
  wakeup();
  return source->id_;
}

void
EventLoop::remove_source_Lm (SourceP source)
{
  ScopedLock<Mutex> locker (main_loop_.mutex(), BALANCED_LOCK);
  assert_return (source->loop_ == this);
  source->loop_ = NULL;
  source->loop_state_ = WAITING;
  auto pos = find (sources_.begin(), sources_.end(), source);
  assert (pos != sources_.end());
  sources_.erase (pos);
  release_id (source->id_);
  source->id_ = 0;
  locker.unlock();
  source->destroy();
  locker.lock();
}

bool
EventLoop::try_remove (uint id)
{
  ScopedLock<Mutex> locker (main_loop_.mutex());
  SourceP &source = find_source_L (id);
  if (source)
    {
      remove_source_Lm (source);
      locker.unlock();
      wakeup();
      return true;
    }
  return false;
}

void
EventLoop::remove (uint id)
{
  if (!try_remove (id))
    critical ("%s: failed to remove loop source: %u", RAPICORN_SIMPLE_FUNCTION, id);
}

/* void EventLoop::change_priority (Source *source, int priority) {
 * // ensure that source belongs to this
 * // reset all source->pfds[].idx = UINT_MAX
 * // unlink source
 * // poke priority
 * // re-add source
 */

void
EventLoop::kill_sources_Lm()
{
  for (;;)
    {
      SourceP &source = find_first_L();
      if (source == NULL)
        break;
      remove_source_Lm (source);
    }
  ScopedLock<Mutex> locker (main_loop_.mutex(), BALANCED_LOCK);
  locker.unlock();
  unpoll_sources_U(); // unlocked
  locker.lock();
}

void
EventLoop::kill_sources()
{
  ScopedLock<Mutex> locker (main_loop_.mutex());
  kill_sources_Lm();
  locker.unlock();
  wakeup();
}

void
EventLoop::wakeup ()
{
  // this needs to work unlocked
  main_loop_.wakeup_poll();
}

// === MainLoop ===
MainLoop::MainLoop() :
  EventLoop (*this), // sets *this as MainLoop on self
  rr_index_ (0), running_ (true), quit_code_ (0)
{
  ScopedLock<Mutex> locker (main_loop_.mutex());
  add_loop_L (*this);
  const int err = eventfd_.open();
  if (err < 0)
    fatal ("MainLoop: failed to create wakeup pipe: %s", strerror (-err));
  // running_ and eventfd_ need to be setup here, so calling quit() before run() works
}

MainLoopP
MainLoop::create ()
{
  MainLoopP mloop = FriendAllocator<MainLoop>::make_shared();
  return mloop;
}

MainLoop::~MainLoop()
{
  // slave loops keep MainLoop alive, so here we should only have one loops_ element: this
  kill_loops(); // essentially just: this->kill_sources_Lm()
  ScopedLock<Mutex> locker (mutex_);
  assert (loops_.size() == 1 && loops_[0] == this);
  loops_.resize (0);
  assert_return (loops_.empty() == true);
}

void
MainLoop::wakeup_poll()
{
  if (eventfd_.opened())
    eventfd_.wakeup();
}

void
MainLoop::add_loop_L (EventLoop &loop)
{
  assert_return (this == &loop.main_loop_);
  loops_.push_back (&loop);
  wakeup_poll();
}

void
MainLoop::remove_loop_L (EventLoop &loop)
{
  assert_return (this == &loop.main_loop_);
  vector<EventLoop*>::iterator it = std::find (loops_.begin(), loops_.end(), &loop);
  assert_return (it != loops_.end());
  loops_.erase (it);
  wakeup_poll();
}

void
MainLoop::kill_loops()
{
  vector<EventLoopP> loops; // initialize *before* locker, so loops->dtor is called last
  ScopedLock<Mutex> locker (mutex_);
  for (size_t i = 0; i < loops_.size(); i++)
    if (loops_[i] != this)
      {
        EventLoopP loop = shared_ptr_cast<EventLoop*> (loops_[i]); // may yield NULL during loop->dtor
        if (loop)
          loops.push_back (loop);
      }
  for (auto loop : loops)
    loop->kill_sources_Lm();
  this->kill_sources_Lm();
  // cleanup
  locker.unlock();
  // here, loops is destroyed and releases the shared_ptrs *after* locker was released
}

int
MainLoop::run ()
{
  ScopedLock<Mutex> locker (mutex_);
  State state;
  while (ISLIKELY (running_))
    iterate_loops_Lm (state, true, true);
  return quit_code_;
}

bool
MainLoop::running ()
{
  ScopedLock<Mutex> locker (mutex_);
  return running_;
}

void
MainLoop::quit (int quit_code)
{
  ScopedLock<Mutex> locker (mutex_);
  quit_code_ = quit_code;
  running_ = false;
  wakeup();
}

bool
MainLoop::finishable_L()
{
  // loop list shouldn't be modified by querying finishable state
  bool found_primary = primary_;
  for (size_t i = 0; !found_primary && i < loops_.size(); i++)
    if (loops_[i]->has_primary_L())
      found_primary = true;
  return !found_primary; // finishable if no primary sources remain
}

bool
MainLoop::finishable()
{
  ScopedLock<Mutex> locker (mutex_);
  return finishable_L();
}

/**
 * @param may_block     If true, iterate() will wait for events occour.
 *
 * MainLoop::iterate() is the heart of the main event loop. For loop iteration,
 * all event sources are polled for incoming events. Then dispatchable sources are
 * picked one per iteration and dispatched in round-robin fashion.
 * If no sources need immediate dispatching and @a may_block is true, iterate() will
 * wait for events to become available.
 * @returns Whether more sources need immediate dispatching.
 */
bool
MainLoop::iterate (bool may_block)
{
  ScopedLock<Mutex> locker (mutex_);
  State state;
  return iterate_loops_Lm (state, may_block, true);
}

void
MainLoop::iterate_pending()
{
  ScopedLock<Mutex> locker (mutex_);
  State state;
  while (iterate_loops_Lm (state, false, true));
}

void
EventLoop::unpoll_sources_U() // must be unlocked!
{
  // clear poll sources
  poll_sources_.resize (0);
}

static const int64 supraint_priobase = 2147483648LL; // INT32_MAX + 1, above all possible int32 values

void
EventLoop::collect_sources_Lm (State &state)
{
  // enforce clean slate
  if (UNLIKELY (!poll_sources_.empty()))
    {
      Mutex &main_mutex = main_loop_.mutex();
      main_mutex.unlock();
      unpoll_sources_U(); // unlocked
      main_mutex.lock();
      assert (poll_sources_.empty());
    }
  if (UNLIKELY (!state.seen_primary && primary_))
    state.seen_primary = true;
  SourceP* arraymem[7]; // using a vector+malloc here shows up in the profiles
  QuickSourcePArray poll_candidates (ARRAY_SIZE (arraymem), arraymem);
  // determine dispatch priority & collect sources for preparing
  dispatch_priority_ = supraint_priobase; // dispatch priority, cover full int32 range initially
  for (SourceList::iterator lit = sources_.begin(); lit != sources_.end(); lit++)
    {
      Source &source = **lit;
      if (UNLIKELY (!state.seen_primary && source.primary_))
        state.seen_primary = true;
      if (source.loop_ != this ||                               // consider undestroyed
          (source.dispatching_ && !source.may_recurse_))        // avoid unallowed recursion
        continue;
      if (source.priority_ < dispatch_priority_ &&
          source.loop_state_ == NEEDS_DISPATCH)                 // dispatch priority needs adjusting
        dispatch_priority_ = source.priority_;                  // upgrade dispatch priority
      if (source.priority_ < dispatch_priority_ ||              // prepare preempting sources
          (source.priority_ == dispatch_priority_ &&
           source.loop_state_ == NEEDS_DISPATCH))               // re-poll sources that need dispatching
        poll_candidates.push (&*lit);                           // collect only, adding ref() next
    }
  // ensure ref counts on all prepare sources
  assert (poll_sources_.empty());
  for (size_t i = 0; i < poll_candidates.size(); i++)
    if ((*poll_candidates[i])->priority_ < dispatch_priority_ || // throw away lower priority sources
        ((*poll_candidates[i])->priority_ == dispatch_priority_ &&
         (*poll_candidates[i])->loop_state_ == NEEDS_DISPATCH)) // re-poll sources that need dispatching
      poll_sources_.push_back (*poll_candidates[i]);
}

bool
EventLoop::prepare_sources_Lm (State          &state,
                               int64          *timeout_usecs,
                               QuickPfdArray  &pfda)
{
  Mutex &main_mutex = main_loop_.mutex();
  // prepare sources, up to NEEDS_DISPATCH priority
  for (auto lit = poll_sources_.begin(); lit != poll_sources_.end(); lit++)
    {
      Source &source = **lit;
      if (source.loop_ != this) // test undestroyed
        continue;
      int64 timeout = -1;
      main_mutex.unlock();
      const bool need_dispatch = source.prepare (state, &timeout);
      main_mutex.lock();
      if (source.loop_ != this)
        continue; // ignore newly destroyed sources
      if (need_dispatch)
        {
          dispatch_priority_ = MIN (dispatch_priority_, source.priority_); // upgrade dispatch priority
          source.loop_state_ = NEEDS_DISPATCH;
          continue;
        }
      source.loop_state_ = PREPARED;
      if (timeout >= 0)
        *timeout_usecs = MIN (*timeout_usecs, timeout);
      uint npfds = source.n_pfds();
      for (uint i = 0; i < npfds; i++)
        if (source.pfds_[i].pfd->fd >= 0)
          {
            uint idx = pfda.size();
            source.pfds_[i].idx = idx;
            pfda.push (*source.pfds_[i].pfd);
            pfda[idx].revents = 0;
          }
        else
          source.pfds_[i].idx = UINT_MAX;
    }
  return dispatch_priority_ < supraint_priobase;
}

bool
EventLoop::check_sources_Lm (State               &state,
                             const QuickPfdArray &pfda)
{
  Mutex &main_mutex = main_loop_.mutex();
  // check polled sources
  for (auto lit = poll_sources_.begin(); lit != poll_sources_.end(); lit++)
    {
      Source &source = **lit;
      if (source.loop_ != this && // test undestroyed
          source.loop_state_ != PREPARED)
        continue; // only check prepared sources
      uint npfds = source.n_pfds();
      for (uint i = 0; i < npfds; i++)
        {
          uint idx = source.pfds_[i].idx;
          if (idx < pfda.size() &&
              source.pfds_[i].pfd->fd == pfda[idx].fd)
            source.pfds_[i].pfd->revents = pfda[idx].revents;
          else
            source.pfds_[i].idx = UINT_MAX;
        }
      main_mutex.unlock();
      bool need_dispatch = source.check (state);
      main_mutex.lock();
      if (source.loop_ != this)
        continue; // ignore newly destroyed sources
      if (need_dispatch)
        {
          dispatch_priority_ = MIN (dispatch_priority_, source.priority_); // upgrade dispatch priority
          source.loop_state_ = NEEDS_DISPATCH;
        }
      else
        source.loop_state_ = WAITING;
    }
  return dispatch_priority_ < supraint_priobase;
}

EventLoop::SourceP
EventLoop::dispatch_source_Lm (State &state)
{
  Mutex &main_mutex = main_loop_.mutex();
  // find a source to dispatch at dispatch_priority_
  SourceP dispatch_source = NULL;       // shared_ptr to keep alive even if everything else is destroyed
  for (auto lit = poll_sources_.begin(); lit != poll_sources_.end(); lit++)
    {
      SourceP &source = *lit;
      if (source->loop_ == this &&                    // test undestroyed
          source->priority_ == dispatch_priority_ &&  // only dispatch at dispatch priority
          source->loop_state_ == NEEDS_DISPATCH)
        {
          dispatch_source = source;
          break;
        }
    }
  dispatch_priority_ = supraint_priobase;
  // dispatch single source
  if (dispatch_source)
    {
      dispatch_source->loop_state_ = WAITING;
      const bool old_was_dispatching = dispatch_source->was_dispatching_;
      dispatch_source->was_dispatching_ = dispatch_source->dispatching_;
      dispatch_source->dispatching_ = true;
      main_mutex.unlock();
      const bool keep_alive = dispatch_source->dispatch (state);
      main_mutex.lock();
      dispatch_source->dispatching_ = dispatch_source->was_dispatching_;
      dispatch_source->was_dispatching_ = old_was_dispatching;
      if (dispatch_source->loop_ == this && !keep_alive)
        remove_source_Lm (dispatch_source);
    }
  return dispatch_source;       // keep alive when passing to caller
}

bool
MainLoop::iterate_loops_Lm (State &state, bool may_block, bool may_dispatch)
{
  assert_return (state.phase == state.NONE, false);
  Mutex &main_mutex = main_loop_.mutex();
  int64 timeout_usecs = INT64_MAX;
  bool any_dispatchable = false;
  PollFD reserved_pfd_mem[7];   // store PollFD array in stack memory, to reduce malloc overhead
  QuickPfdArray pfda (ARRAY_SIZE (reserved_pfd_mem), reserved_pfd_mem); // pfda.size() == 0
  // allow poll wakeups
  const PollFD wakeup = { eventfd_.inputfd(), PollFD::IN, 0 };
  const uint wakeup_idx = 0; // wakeup_idx = pfda.size();
  pfda.push (wakeup);
  // create pollable loop list
  EventLoopP loops[loops_.size()];
  size_t j = 0;
  for (size_t i = 0; i < loops_.size(); i++)
    {
      EventLoopP loop = shared_ptr_cast<EventLoop*> (loops_[i]); // may yield NULL during loop->dtor
      if (loop)
        loops[j++] = loop;
    }
  const size_t nloops = j;
  // collect
  state.phase = state.COLLECT;
  state.seen_primary = false;
  for (size_t i = 0; i < nloops; i++)
    loops[i]->collect_sources_Lm (state);
  // prepare
  state.phase = state.PREPARE;
  state.current_time_usecs = timestamp_realtime();
  bool dispatchable[nloops];
  for (size_t i = 0; i < nloops; i++)
    {
      dispatchable[i] = loops[i]->prepare_sources_Lm (state, &timeout_usecs, pfda);
      any_dispatchable |= dispatchable[i];
    }
  // poll file descriptors
  int64 timeout_msecs = timeout_usecs / 1000;
  if (timeout_usecs > 0 && timeout_msecs <= 0)
    timeout_msecs = 1;
  if (!may_block || any_dispatchable)
    timeout_msecs = 0;
  main_mutex.unlock();
  int presult;
  do
    presult = poll ((struct pollfd*) &pfda[0], pfda.size(), MIN (timeout_msecs, INT_MAX));
  while (presult < 0 && errno == EAGAIN); // EINTR may indicate a signal
  main_mutex.lock();
  if (presult < 0)
    critical ("MainLoop: poll() failed: %s", strerror());
  else if (pfda[wakeup_idx].revents)
    eventfd_.flush(); // restart queueing wakeups, possibly triggered by dispatching
  // check
  state.phase = state.CHECK;
  state.current_time_usecs = timestamp_realtime();
  for (size_t i = 0; i < nloops; i++)
    {
      dispatchable[i] |= loops[i]->check_sources_Lm (state, pfda);
      any_dispatchable |= dispatchable[i];
    }
  // dispatch
  SourceP dispatched_source = NULL;
  if (may_dispatch && any_dispatchable)
    {
      size_t index, i = nloops;
      do        // find next dispatchable loop in round-robin fashion
        index = rr_index_++ % nloops;
      while (!dispatchable[index] && i--);
      state.phase = state.DISPATCH;
      dispatched_source = loops[index]->dispatch_source_Lm (state); // passes on shared_ptr to keep alive wihle locked
    }
  // cleanup
  state.phase = state.NONE;
  main_mutex.unlock();
  dispatched_source = NULL; // release after mutex unlocked
  for (size_t i = 0; i < nloops; i++)
    {
      loops[i]->unpoll_sources_U(); // unlocked
      loops[i] = NULL; // unlocked
    }
  main_mutex.lock();
  return any_dispatchable; // need to dispatch or recheck
}

struct SlaveLoop : public EventLoop {
  friend class FriendAllocator<SlaveLoop>;
  const MainLoopP main_loop_ref_;
  SlaveLoop (MainLoopP main) :
    EventLoop (*main), main_loop_ref_ (main)
  {
    ScopedLock<Mutex> locker (main_loop_.mutex());
    main_loop_.add_loop_L (*this);
  }
  ~SlaveLoop()
  {
    ScopedLock<Mutex> locker (main_loop_.mutex());
    kill_sources_Lm();
    main_loop_.remove_loop_L (*this);
  }
};

EventLoopP
MainLoop::create_slave()
{
  return FriendAllocator<SlaveLoop>::make_shared (shared_ptr_cast<MainLoop> (this));
}

// === EventLoop::State ===
EventLoop::State::State() :
  current_time_usecs (0), phase (NONE), seen_primary (false)
{}

// === EventLoop::Source ===
EventLoop::Source::Source () :
  loop_ (NULL),
  pfds_ (NULL),
  id_ (0),
  priority_ (INT_MAX),
  loop_state_ (0),
  may_recurse_ (0),
  dispatching_ (0),
  was_dispatching_ (0),
  primary_ (0)
{}

uint
EventLoop::Source::n_pfds ()
{
  uint i = 0;
  if (pfds_)
    while (pfds_[i].pfd)
      i++;
  return i;
}

void
EventLoop::Source::may_recurse (bool may_recurse)
{
  may_recurse_ = may_recurse;
}

bool
EventLoop::Source::may_recurse () const
{
  return may_recurse_;
}

bool
EventLoop::Source::primary () const
{
  return primary_;
}

void
EventLoop::Source::primary (bool is_primary)
{
  primary_ = is_primary;
}

bool
EventLoop::Source::recursion () const
{
  return dispatching_ && was_dispatching_;
}

void
EventLoop::Source::add_poll (PollFD *const pfd)
{
  const uint idx = n_pfds();
  uint npfds = idx + 1;
  pfds_ = (typeof (pfds_)) realloc (pfds_, sizeof (pfds_[0]) * (npfds + 1));
  if (!pfds_)
    fatal ("EventLoopSource: out of memory");
  pfds_[npfds].idx = UINT_MAX;
  pfds_[npfds].pfd = NULL;
  pfds_[idx].idx = UINT_MAX;
  pfds_[idx].pfd = pfd;
}

void
EventLoop::Source::remove_poll (PollFD *const pfd)
{
  uint idx, npfds = n_pfds();
  for (idx = 0; idx < npfds; idx++)
    if (pfds_[idx].pfd == pfd)
      break;
  if (idx < npfds)
    {
      pfds_[idx].idx = UINT_MAX;
      pfds_[idx].pfd = pfds_[npfds - 1].pfd;
      pfds_[idx].idx = pfds_[npfds - 1].idx;
      pfds_[npfds - 1].idx = UINT_MAX;
      pfds_[npfds - 1].pfd = NULL;
    }
  else
    critical ("EventLoopSource: unremovable PollFD: %p (fd=%d)", pfd, pfd->fd);
}

void
EventLoop::Source::destroy ()
{}

void
EventLoop::Source::loop_remove ()
{
  if (loop_)
    loop_->try_remove (source_id());
}

EventLoop::Source::~Source ()
{
  RAPICORN_ASSERT (loop_ == NULL);
  if (pfds_)
    free (pfds_);
}

// == EventLoop::DispatcherSource ==
EventLoop::DispatcherSource::DispatcherSource (const DispatcherSlot &slot) :
  slot_ (slot)
{}

EventLoop::DispatcherSource::~DispatcherSource ()
{
  slot_ = NULL;
}

bool
EventLoop::DispatcherSource::prepare (const State &state, int64 *timeout_usecs_p)
{
  return slot_ (state);
}

bool
EventLoop::DispatcherSource::check (const State &state)
{
  return slot_ (state);
}

bool
EventLoop::DispatcherSource::dispatch (const State &state)
{
  return slot_ (state);
}

void
EventLoop::DispatcherSource::destroy()
{
  State state;
  state.phase = state.DESTROY;
  slot_ (state);
}

// == EventLoop::TimedSource ==
EventLoop::TimedSource::TimedSource (const VoidSlot &slot, uint initial_interval_msecs, uint repeat_interval_msecs) :
  expiration_usecs_ (timestamp_realtime() + 1000ULL * initial_interval_msecs),
  interval_msecs_ (repeat_interval_msecs), first_interval_ (true),
  oneshot_ (true), void_slot_ (slot)
{}

EventLoop::TimedSource::TimedSource (const BoolSlot &slot, uint initial_interval_msecs, uint repeat_interval_msecs) :
  expiration_usecs_ (timestamp_realtime() + 1000ULL * initial_interval_msecs),
  interval_msecs_ (repeat_interval_msecs), first_interval_ (true),
  oneshot_ (false), bool_slot_ (slot)
{}

bool
EventLoop::TimedSource::prepare (const State &state,
                                 int64 *timeout_usecs_p)
{
  if (state.current_time_usecs >= expiration_usecs_)
    return true;                                            /* timeout expired */
  if (!first_interval_)
    {
      uint64 interval = interval_msecs_ * 1000ULL;
      if (state.current_time_usecs + interval < expiration_usecs_)
        expiration_usecs_ = state.current_time_usecs + interval; /* clock warped back in time */
    }
  *timeout_usecs_p = MIN (INT_MAX, expiration_usecs_ - state.current_time_usecs);
  return 0 == *timeout_usecs_p;
}

bool
EventLoop::TimedSource::check (const State &state)
{
  return state.current_time_usecs >= expiration_usecs_;
}

bool
EventLoop::TimedSource::dispatch (const State &state)
{
  bool repeat = false;
  first_interval_ = false;
  if (oneshot_ && void_slot_ != NULL)
    void_slot_ ();
  else if (!oneshot_ && bool_slot_ != NULL)
    repeat = bool_slot_ ();
  if (repeat)
    expiration_usecs_ = timestamp_realtime() + 1000ULL * interval_msecs_;
  return repeat;
}

EventLoop::TimedSource::~TimedSource ()
{
  if (oneshot_)
    void_slot_.~VoidSlot();
  else
    bool_slot_.~BoolSlot();
}

// == EventLoop::PollFDSource ==
/*! @class EventLoop::PollFDSource
 * A PollFDSource can be used to execute a callback function from the main loop,
 * depending on certain file descriptor states.
 * The modes supported for polling the file descriptor are as follows:
 * @li @c "w" - poll writable
 * @li @c "r" - poll readable
 * @li @c "p" - poll urgent redable
 * @li @c "b" - set fd blocking
 * @li @c "B" - set fd non-blocking
 * @li @c "E" - ignore erros (or auto destroy)
 * @li @c "H" - ignore hangup (or auto destroy)
 * @li @c "C" - prevent auto close on destroy
 */
EventLoop::PollFDSource::PollFDSource (const BPfdSlot &slot, int fd, const String &mode) :
  pfd_ ((PollFD) { fd, 0, 0 }),
  ignore_errors_ (strchr (mode.c_str(), 'E') != NULL),
  ignore_hangup_ (strchr (mode.c_str(), 'H') != NULL),
  never_close_ (strchr (mode.c_str(), 'C') != NULL),
  oneshot_ (false), bool_poll_slot_ (slot)
{
  construct (mode);
}

EventLoop::PollFDSource::PollFDSource (const VPfdSlot &slot, int fd, const String &mode) :
  pfd_ ((PollFD) { fd, 0, 0 }),
  ignore_errors_ (strchr (mode.c_str(), 'E') != NULL),
  ignore_hangup_ (strchr (mode.c_str(), 'H') != NULL),
  never_close_ (strchr (mode.c_str(), 'C') != NULL),
  oneshot_ (true), void_poll_slot_ (slot)
{
  construct (mode);
}

void
EventLoop::PollFDSource::construct (const String &mode)
{
  add_poll (&pfd_);
  pfd_.events |= strchr (mode.c_str(), 'w') ? PollFD::OUT : 0;
  pfd_.events |= strchr (mode.c_str(), 'r') ? PollFD::IN : 0;
  pfd_.events |= strchr (mode.c_str(), 'p') ? PollFD::PRI : 0;
  if (pfd_.fd >= 0)
    {
      const long lflags = fcntl (pfd_.fd, F_GETFL, 0);
      long nflags = lflags;
      if (strchr (mode.c_str(), 'b'))
        nflags &= ~long (O_NONBLOCK);
      else if (strchr (mode.c_str(), 'B'))
        nflags |= O_NONBLOCK;
      if (nflags != lflags)
        {
          int err;
          do
            err = fcntl (pfd_.fd, F_SETFL, nflags);
          while (err < 0 && (errno == EINTR || errno == EAGAIN));
        }
    }
}

bool
EventLoop::PollFDSource::prepare (const State &state,
                                  int64 *timeout_usecs_p)
{
  pfd_.revents = 0;
  return pfd_.fd < 0;
}

bool
EventLoop::PollFDSource::check (const State &state)
{
  return pfd_.fd < 0 || pfd_.revents != 0;
}

bool
EventLoop::PollFDSource::dispatch (const State &state)
{
  bool keep_alive = false;
  if (pfd_.fd >= 0 && (pfd_.revents & PollFD::NVAL))
    ; // close down
  else if (pfd_.fd >= 0 && !ignore_errors_ && (pfd_.revents & PollFD::ERR))
    ; // close down
  else if (pfd_.fd >= 0 && !ignore_hangup_ && (pfd_.revents & PollFD::HUP))
    ; // close down
  else if (oneshot_ && void_poll_slot_ != NULL)
    void_poll_slot_ (pfd_);
  else if (!oneshot_ && bool_poll_slot_ != NULL)
    keep_alive = bool_poll_slot_ (pfd_);
  /* close down */
  if (!keep_alive)
    {
      if (!never_close_ && pfd_.fd >= 0)
        close (pfd_.fd);
      pfd_.fd = -1;
    }
  return keep_alive;
}

void
EventLoop::PollFDSource::destroy()
{
  /* close down */
  if (!never_close_ && pfd_.fd >= 0)
    close (pfd_.fd);
  pfd_.fd = -1;
}

EventLoop::PollFDSource::~PollFDSource ()
{
  if (oneshot_)
    void_poll_slot_.~VPfdSlot();
  else
    bool_poll_slot_.~BPfdSlot();
}

} // Rapicorn

// == Loop Description ==
/*! @page eventloops    Event Loops and Event Sources
  Rapicorn <a href="http://en.wikipedia.org/wiki/Event_loop">event loops</a>
  are a programming facility to execute callback handlers (dispatch event sources) according to expiring Timers,
  IO events or arbitrary other conditions.
  A Rapicorn::EventLoop is created with Rapicorn::MainLoop::_new() or Rapicorn::MainLoop::new_slave(). Callbacks or other
  event sources are added to it via Rapicorn::EventLoop::add(), Rapicorn::EventLoop::exec_normal() and related functions.
  Once a main loop is created and its callbacks are added, it can be run as: @code
  * while (!loop.finishable())
  *   loop.iterate (true);
  @endcode
  Rapicorn::MainLoop::iterate() finds a source that immediately need dispatching and starts to dispatch it.
  If no source was found, it monitors the source list's PollFD descriptors for events, and finds dispatchable
  sources based on newly incoming events on the descriptors.
  If multiple sources need dispatching, they are handled according to their priorities (see Rapicorn::EventLoop::add())
  and at the same priority, sources are dispatched in round-robin fashion.
  Calling Rapicorn::MainLoop::iterate() also iterates over its slave loops, which allows to handle sources
  on several independently running loops within the same thread, usually used to associate one event loop with one window.

  Traits of the Rapicorn::EventLoop class:
  @li The main loop and its slave loops are handled in round-robin fahsion, priorities only apply internally to a loop.
  @li Loops are thread safe, so any thready may add or remove sources to a loop at any time, regardless of which thread
  is currently running the loop.
  @li Sources added to a loop may be flagged as "primary" (see Rapicorn::EventLoop::Source::primary()),
  to keep the loop from exiting. This is used to distinguish background jobs, e.g. updating a window's progress bar,
  from primary jobs, like processing events on the main window.
  Sticking with the example, a window's event loop should be exited if the window vanishes, but not when it's
  progress bar stoped updating.

  Loop integration of a Rapicorn::EventLoop::Source class:
  @li First, prepare() is called on a source, returning true here flags the source to be ready for immediate dispatching.
  @li Second, poll(2) monitors all PollFD file descriptors of the source (see Rapicorn::EventLoop::Source::add_poll()).
  @li Third, check() is called for the source to check whether dispatching is needed depending on PollFD states.
  @li Fourth, the source is dispatched if it returened true from either prepare() or check(). If multiple sources are
  ready to be dispatched, the entire process may be repeated several times (after dispatching other sources),
  starting with a new call to prepare() before a particular source is finally dispatched.
 */
