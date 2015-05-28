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
struct QuickSourcePArray : public QuickArray<EventSourceP*> {
  QuickSourcePArray (uint n_reserved, EventSourceP **reserved) : QuickArray (n_reserved, reserved) {}
};

// === EventLoop ===
EventLoop::EventLoop (MainLoop &main) :
  main_loop_ (&main), dispatch_priority_ (0), primary_ (false)
{
  poll_sources_.reserve (7);
  // we cannot *use* main_loop_ yet, because we might be called from within MainLoop::MainLoop(), see SlaveLoop()
  assert_return (main_loop_ && main_loop_->main_loop_); // sanity checks
}

EventLoop::~EventLoop ()
{
  unpoll_sources_U();
  // we cannot *use* main_loop_ anymore, because we might be called from within MainLoop::MainLoop(), see ~SlaveLoop()
}

inline EventSourceP&
EventLoop::find_first_L()
{
  static EventSourceP null_source;
  return sources_.empty() ? null_source : sources_[0];
}

inline EventSourceP&
EventLoop::find_source_L (uint id)
{
  for (SourceList::iterator lit = sources_.begin(); lit != sources_.end(); lit++)
    if (id == (*lit)->id_)
      return *lit;
  static EventSourceP null_source;
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
  ScopedLock<Mutex> locker (main_loop_->mutex());
  return has_primary_L();
}

bool
EventLoop::flag_primary (bool on)
{
  ScopedLock<Mutex> locker (main_loop_->mutex());
  const bool was_primary = primary_;
  primary_ = on;
  if (primary_ != was_primary)
    wakeup();
  return was_primary;
}

static const int16 UNDEFINED_PRIORITY = -32768;

uint
EventLoop::add (EventSourceP source, int priority)
{
  static_assert (UNDEFINED_PRIORITY < 1, "");
  assert_return (priority >= 1 && priority <= PRIORITY_CEILING, 0);
  ScopedLock<Mutex> locker (main_loop_->mutex());
  assert_return (source != NULL, 0);
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
EventLoop::remove_source_Lm (EventSourceP source)
{
  ScopedLock<Mutex> locker (main_loop_->mutex(), BALANCED_LOCK);
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
  ScopedLock<Mutex> locker (main_loop_->mutex());
  EventSourceP &source = find_source_L (id);
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

/* void EventLoop::change_priority (EventSource *source, int priority) {
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
      EventSourceP &source = find_first_L();
      if (source == NULL)
        break;
      remove_source_Lm (source);
    }
  ScopedLock<Mutex> locker (main_loop_->mutex(), BALANCED_LOCK);
  locker.unlock();
  unpoll_sources_U(); // unlocked
  locker.lock();
}

/** Remove all sources from a loop and prevent any further execution.
 * The destroy_loop() method removes all sources from a loop and in
 * case of a slave EventLoop (see create_slave()) removes it from its
 * associated main loop. Calling destroy_loop() on a main loop also
 * calls destroy_loop() for all its slave loops.
 * Note that MainLoop objects are artificially kept alive until
 * MainLoop::destroy_loop() is called, so calling destroy_loop() is
 * mandatory for MainLoop objects to prevent object leaks.
 * This method must be called only once on a loop.
 */
void
EventLoop::destroy_loop()
{
  assert_return (main_loop_ != NULL);
  // guard main_loop_ pointer *before* locking, so dtor is called after unlock
  EventLoopP main_loop_guard = shared_ptr_cast<EventLoop*> (main_loop_);
  ScopedLock<Mutex> locker (main_loop_->mutex());
  if (this != main_loop_)
    main_loop_->kill_loop_Lm (*this);
  else
    main_loop_->kill_loops_Lm();
  assert_return (main_loop_ == NULL);
}

void
EventLoop::wakeup ()
{
  // this needs to work unlocked
  main_loop_->wakeup_poll();
}

// === MainLoop ===
MainLoop::MainLoop() :
  EventLoop (*this), // sets *this as MainLoop on self
  rr_index_ (0), running_ (false), has_quit_ (false), quit_code_ (0)
{
  ScopedLock<Mutex> locker (main_loop_->mutex());
  const int err = eventfd_.open();
  if (err < 0)
    fatal ("MainLoop: failed to create wakeup pipe: %s", strerror (-err));
  // has_quit_ and eventfd_ need to be setup here, so calling quit() before run() works
}

/** Create a new main loop object, users can run or iterate this loop directly.
 * Note that MainLoop objects have special lifetime semantics that keep them
 * alive until they are explicitely destroyed with destroy_loop().
 */
MainLoopP
MainLoop::create ()
{
  MainLoopP main_loop = FriendAllocator<MainLoop>::make_shared();
  ScopedLock<Mutex> locker (main_loop->mutex());
  main_loop->add_loop_L (*main_loop);
  return main_loop;
}

MainLoop::~MainLoop()
{
  ScopedLock<Mutex> locker (mutex_);
  if (main_loop_)
    kill_loops_Lm();
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
  assert_return (this == loop.main_loop_);
  loops_.push_back (shared_ptr_cast<EventLoop> (&loop));
  wakeup_poll();
}

void
MainLoop::kill_loop_Lm (EventLoop &loop)
{
  assert_return (this == loop.main_loop_);
  loop.kill_sources_Lm();
  if (loop.main_loop_) // guard against nested kill_loop_Lm (same) calls
    {
      loop.main_loop_ = NULL;
      vector<EventLoopP>::iterator it = std::find_if (loops_.begin(), loops_.end(),
                                                      [&loop] (EventLoopP &lp) { return lp.get() == &loop; });
      if (this == &loop) // MainLoop->destroy_loop()
        {
          if (loops_.size()) // MainLoop must be the last loop to be destroyed
            assert_return (loops_[0].get() == this);
        }
      else
        assert_return (it != loops_.end());
      if (it != loops_.end())
        loops_.erase (it);
      wakeup_poll();
    }
}

void
MainLoop::kill_loops_Lm()
{
  while (loops_.size() > 1 || loops_[0].get() != this)
    {
      EventLoopP loop = loops_[0].get() != this ? loops_[0] : loops_[loops_.size() - 1];
      kill_loop_Lm (*loop);
    }
  kill_loop_Lm (*this);
}

int
MainLoop::run ()
{
  EventLoopP main_loop_guard = shared_ptr_cast<EventLoop> (this);
  ScopedLock<Mutex> locker (mutex_);
  LoopState state;
  running_ = !has_quit_;
  while (ISLIKELY (running_))
    iterate_loops_Lm (state, true, true);
  const int last_quit_code = quit_code_;
  running_ = false;
  has_quit_ = false;    // allow loop resumption
  quit_code_ = 0;       // allow loop resumption
  return last_quit_code;
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
  has_quit_ = true;     // cancel run() upfront, or
  running_ = false;     // interrupt current iteration
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
  EventLoopP main_loop_guard = shared_ptr_cast<EventLoop> (this);
  ScopedLock<Mutex> locker (mutex_);
  LoopState state;
  const bool was_running = running_;    // guard for recursion
  running_ = true;
  const bool sources_pending = iterate_loops_Lm (state, may_block, true);
  running_ = was_running && !has_quit_;
  return sources_pending;
}

void
MainLoop::iterate_pending()
{
  EventLoopP main_loop_guard = shared_ptr_cast<EventLoop> (this);
  ScopedLock<Mutex> locker (mutex_);
  LoopState state;
  const bool was_running = running_;    // guard for recursion
  running_ = true;
  while (ISLIKELY (running_))
    if (!iterate_loops_Lm (state, false, true))
      break;
  running_ = was_running && !has_quit_;
}

bool
MainLoop::pending()
{
  ScopedLock<Mutex> locker (mutex_);
  LoopState state;
  EventLoopP main_loop_guard = shared_ptr_cast<EventLoop> (this);
  return iterate_loops_Lm (state, false, false);
}

void
EventLoop::unpoll_sources_U() // must be unlocked!
{
  // clear poll sources
  poll_sources_.resize (0);
}

void
EventLoop::collect_sources_Lm (LoopState &state)
{
  // enforce clean slate
  if (UNLIKELY (!poll_sources_.empty()))
    {
      Mutex &main_mutex = main_loop_->mutex();
      main_mutex.unlock();
      unpoll_sources_U(); // unlocked
      main_mutex.lock();
      assert (poll_sources_.empty());
    }
  if (UNLIKELY (!state.seen_primary && primary_))
    state.seen_primary = true;
  EventSourceP* arraymem[7]; // using a vector+malloc here shows up in the profiles
  QuickSourcePArray poll_candidates (ARRAY_SIZE (arraymem), arraymem);
  // determine dispatch priority & collect sources for preparing
  dispatch_priority_ = UNDEFINED_PRIORITY; // initially, consider sources at *all* priorities
  for (SourceList::iterator lit = sources_.begin(); lit != sources_.end(); lit++)
    {
      EventSource &source = **lit;
      if (UNLIKELY (!state.seen_primary && source.primary_))
        state.seen_primary = true;
      if (source.loop_ != this ||                               // ignore destroyed and
          (source.dispatching_ && !source.may_recurse_))        // avoid unallowed recursion
        continue;
      if (source.priority_ > dispatch_priority_ &&              // ignore lower priority sources
          source.loop_state_ == NEEDS_DISPATCH)                 // if NEEDS_DISPATCH sources remain
        dispatch_priority_ = source.priority_;                  // so raise dispatch_priority_
      if (source.priority_ > dispatch_priority_ ||              // add source if it is an eligible
          (source.priority_ == dispatch_priority_ &&            // candidate, baring future raises
           source.loop_state_ == NEEDS_DISPATCH))               // of dispatch_priority_...
        poll_candidates.push (&*lit);                           // collect only, adding ref() later
    }
  // ensure ref counts on all prepare sources
  assert (poll_sources_.empty());
  for (size_t i = 0; i < poll_candidates.size(); i++)
    if ((*poll_candidates[i])->priority_ > dispatch_priority_ || // throw away lower priority sources
        ((*poll_candidates[i])->priority_ == dispatch_priority_ &&
         (*poll_candidates[i])->loop_state_ == NEEDS_DISPATCH)) // re-poll sources that need dispatching
      poll_sources_.push_back (*poll_candidates[i]);
  /* here, poll_sources_ contains either all sources, or only the highest priority
   * NEEDS_DISPATCH sources plus higher priority sources. giving precedence to the
   * remaining NEEDS_DISPATCH sources ensures round-robin processing.
   */
}

bool
EventLoop::prepare_sources_Lm (LoopState &state, int64 *timeout_usecs, QuickPfdArray &pfda)
{
  Mutex &main_mutex = main_loop_->mutex();
  // prepare sources, up to NEEDS_DISPATCH priority
  for (auto lit = poll_sources_.begin(); lit != poll_sources_.end(); lit++)
    {
      EventSource &source = **lit;
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
          dispatch_priority_ = MAX (dispatch_priority_, source.priority_); // upgrade dispatch priority
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
  return dispatch_priority_ > UNDEFINED_PRIORITY;
}

bool
EventLoop::check_sources_Lm (LoopState &state, const QuickPfdArray &pfda)
{
  Mutex &main_mutex = main_loop_->mutex();
  // check polled sources
  for (auto lit = poll_sources_.begin(); lit != poll_sources_.end(); lit++)
    {
      EventSource &source = **lit;
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
          dispatch_priority_ = MAX (dispatch_priority_, source.priority_); // upgrade dispatch priority
          source.loop_state_ = NEEDS_DISPATCH;
        }
      else
        source.loop_state_ = WAITING;
    }
  return dispatch_priority_ > UNDEFINED_PRIORITY;
}

void
EventLoop::dispatch_source_Lm (LoopState &state)
{
  Mutex &main_mutex = main_loop_->mutex();
  // find a source to dispatch at dispatch_priority_
  EventSourceP dispatch_source = NULL;                  // shared_ptr to keep alive even if everything else is destroyed
  for (auto lit = poll_sources_.begin(); lit != poll_sources_.end(); lit++)
    {
      EventSourceP &source = *lit;
      if (source->loop_ == this &&                      // test undestroyed
          source->priority_ == dispatch_priority_ &&    // only dispatch at dispatch priority
          source->loop_state_ == NEEDS_DISPATCH)
        {
          dispatch_source = source;
          break;
        }
    }
  dispatch_priority_ = UNDEFINED_PRIORITY;
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
}

bool
MainLoop::iterate_loops_Lm (LoopState &state, bool may_block, bool may_dispatch)
{
  assert_return (state.phase == state.NONE, false);
  Mutex &main_mutex = main_loop_->mutex();
  int64 timeout_usecs = INT64_MAX;
  PollFD reserved_pfd_mem[7];   // store PollFD array in stack memory, to reduce malloc overhead
  QuickPfdArray pfda (ARRAY_SIZE (reserved_pfd_mem), reserved_pfd_mem); // pfda.size() == 0
  // allow poll wakeups
  const PollFD wakeup = { eventfd_.inputfd(), PollFD::IN, 0 };
  const uint wakeup_idx = 0; // wakeup_idx = pfda.size();
  pfda.push (wakeup);
  // create pollable loop list
  const size_t nloops = loops_.size();
  EventLoopP loops[nloops];
  for (size_t i = 0; i < nloops; i++)
    loops[i] = loops_[i];
  // collect
  state.phase = state.COLLECT;
  state.seen_primary = false;
  for (size_t i = 0; i < nloops; i++)
    loops[i]->collect_sources_Lm (state);
  // prepare
  bool any_dispatchable = false;
  bool priority_ascension = false;      // flag for priority elevation between loops
  state.phase = state.PREPARE;
  state.current_time_usecs = timestamp_realtime();
  bool dispatchable[nloops];
  for (size_t i = 0; i < nloops; i++)
    {
      dispatchable[i] = loops[i]->prepare_sources_Lm (state, &timeout_usecs, pfda);
      any_dispatchable |= dispatchable[i];
      if (UNLIKELY (loops[i]->dispatch_priority_ >= PRIORITY_ASCENT) && dispatchable[i])
        priority_ascension = true;
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
  if (presult < 0 && errno != EINTR)
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
      if (UNLIKELY (loops[i]->dispatch_priority_ >= PRIORITY_ASCENT) && dispatchable[i])
        priority_ascension = true;
    }
  // dispatch
  if (may_dispatch && any_dispatchable)
    {
      size_t index, i = nloops;
      do        // find next dispatchable loop in round-robin fashion
        index = rr_index_++ % nloops;
      while ((!dispatchable[index] ||
              (priority_ascension && loops[index]->dispatch_priority_ < PRIORITY_ASCENT)) && i--);
      state.phase = state.DISPATCH;
      loops[index]->dispatch_source_Lm (state); // passes on shared_ptr to keep alive wihle locked
    }
  // cleanup
  state.phase = state.NONE;
  main_mutex.unlock();
  for (size_t i = 0; i < nloops; i++)
    {
      loops[i]->unpoll_sources_U(); // unlocked
      loops[i] = NULL; // dtor, unlocked
    }
  main_mutex.lock();
  return any_dispatchable; // need to dispatch or recheck
}

struct SlaveLoop : public EventLoop {
  friend class FriendAllocator<SlaveLoop>;
  SlaveLoop (MainLoopP main) :
    EventLoop (*main)
  {}
  ~SlaveLoop()
  {
    assert_return (main_loop_ == NULL);
  }
};

EventLoopP
MainLoop::create_slave()
{
  ScopedLock<Mutex> locker (mutex());
  EventLoopP slave_loop = FriendAllocator<SlaveLoop>::make_shared (shared_ptr_cast<MainLoop> (this));
  this->add_loop_L (*slave_loop);
  return slave_loop;
}

// === EventLoop::LoopState ===
LoopState::LoopState() :
  current_time_usecs (0), phase (NONE), seen_primary (false)
{}

// === EventSource ===
EventSource::EventSource () :
  loop_ (NULL),
  pfds_ (NULL),
  id_ (0),
  priority_ (UNDEFINED_PRIORITY),
  loop_state_ (0),
  may_recurse_ (0),
  dispatching_ (0),
  was_dispatching_ (0),
  primary_ (0)
{}

uint
EventSource::n_pfds ()
{
  uint i = 0;
  if (pfds_)
    while (pfds_[i].pfd)
      i++;
  return i;
}

void
EventSource::may_recurse (bool may_recurse)
{
  may_recurse_ = may_recurse;
}

bool
EventSource::may_recurse () const
{
  return may_recurse_;
}

bool
EventSource::primary () const
{
  return primary_;
}

void
EventSource::primary (bool is_primary)
{
  primary_ = is_primary;
}

bool
EventSource::recursion () const
{
  return dispatching_ && was_dispatching_;
}

void
EventSource::add_poll (PollFD *const pfd)
{
  const uint idx = n_pfds();
  uint npfds = idx + 1;
  pfds_ = (typeof (pfds_)) realloc (pfds_, sizeof (pfds_[0]) * (npfds + 1));
  if (!pfds_)
    fatal ("EventSource: out of memory");
  pfds_[npfds].idx = UINT_MAX;
  pfds_[npfds].pfd = NULL;
  pfds_[idx].idx = UINT_MAX;
  pfds_[idx].pfd = pfd;
}

void
EventSource::remove_poll (PollFD *const pfd)
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
    critical ("EventSource: unremovable PollFD: %p (fd=%d)", pfd, pfd->fd);
}

void
EventSource::destroy ()
{}

void
EventSource::loop_remove ()
{
  if (loop_)
    loop_->try_remove (source_id());
}

EventSource::~EventSource ()
{
  RAPICORN_ASSERT (loop_ == NULL);
  if (pfds_)
    free (pfds_);
}

// == DispatcherSource ==
DispatcherSource::DispatcherSource (const DispatcherSlot &slot) :
  slot_ (slot)
{}

DispatcherSource::~DispatcherSource ()
{
  slot_ = NULL;
}

bool
DispatcherSource::prepare (const LoopState &state, int64 *timeout_usecs_p)
{
  return slot_ (state);
}

bool
DispatcherSource::check (const LoopState &state)
{
  return slot_ (state);
}

bool
DispatcherSource::dispatch (const LoopState &state)
{
  return slot_ (state);
}

void
DispatcherSource::destroy()
{
  LoopState state;
  state.phase = state.DESTROY;
  slot_ (state);
}

// == TimedSource ==
TimedSource::TimedSource (const VoidSlot &slot, uint initial_interval_msecs, uint repeat_interval_msecs) :
  expiration_usecs_ (timestamp_realtime() + 1000ULL * initial_interval_msecs),
  interval_msecs_ (repeat_interval_msecs), first_interval_ (true),
  oneshot_ (true), void_slot_ (slot)
{}

TimedSource::TimedSource (const BoolSlot &slot, uint initial_interval_msecs, uint repeat_interval_msecs) :
  expiration_usecs_ (timestamp_realtime() + 1000ULL * initial_interval_msecs),
  interval_msecs_ (repeat_interval_msecs), first_interval_ (true),
  oneshot_ (false), bool_slot_ (slot)
{}

bool
TimedSource::prepare (const LoopState &state, int64 *timeout_usecs_p)
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
TimedSource::check (const LoopState &state)
{
  return state.current_time_usecs >= expiration_usecs_;
}

bool
TimedSource::dispatch (const LoopState &state)
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

TimedSource::~TimedSource ()
{
  if (oneshot_)
    void_slot_.~VoidSlot();
  else
    bool_slot_.~BoolSlot();
}

// == PollFDSource ==
/*! @class PollFDSource
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
PollFDSource::PollFDSource (const BPfdSlot &slot, int fd, const String &mode) :
  pfd_ ((PollFD) { fd, 0, 0 }),
  ignore_errors_ (strchr (mode.c_str(), 'E') != NULL),
  ignore_hangup_ (strchr (mode.c_str(), 'H') != NULL),
  never_close_ (strchr (mode.c_str(), 'C') != NULL),
  oneshot_ (false), bool_poll_slot_ (slot)
{
  construct (mode);
}

PollFDSource::PollFDSource (const VPfdSlot &slot, int fd, const String &mode) :
  pfd_ ((PollFD) { fd, 0, 0 }),
  ignore_errors_ (strchr (mode.c_str(), 'E') != NULL),
  ignore_hangup_ (strchr (mode.c_str(), 'H') != NULL),
  never_close_ (strchr (mode.c_str(), 'C') != NULL),
  oneshot_ (true), void_poll_slot_ (slot)
{
  construct (mode);
}

void
PollFDSource::construct (const String &mode)
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
PollFDSource::prepare (const LoopState &state, int64 *timeout_usecs_p)
{
  pfd_.revents = 0;
  return pfd_.fd < 0;
}

bool
PollFDSource::check (const LoopState &state)
{
  return pfd_.fd < 0 || pfd_.revents != 0;
}

bool
PollFDSource::dispatch (const LoopState &state)
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
PollFDSource::destroy()
{
  /* close down */
  if (!never_close_ && pfd_.fd >= 0)
    close (pfd_.fd);
  pfd_.fd = -1;
}

PollFDSource::~PollFDSource ()
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
  @li The main loop and its slave loops are handled in round-robin fahsion, priorities below Rapicorn::EventLoop::PRIORITY_ASCENT only apply internally to a loop.
  @li Loops are thread safe, so any thready may add or remove sources to a loop at any time, regardless of which thread
  is currently running the loop.
  @li Sources added to a loop may be flagged as "primary" (see Rapicorn::EventSource::primary()),
  to keep the loop from exiting. This is used to distinguish background jobs, e.g. updating a window's progress bar,
  from primary jobs, like processing events on the main window.
  Sticking with the example, a window's event loop should be exited if the window vanishes, but not when it's
  progress bar stoped updating.

  Loop integration of a Rapicorn::EventSource class:
  @li First, prepare() is called on a source, returning true here flags the source to be ready for immediate dispatching.
  @li Second, poll(2) monitors all PollFD file descriptors of the source (see Rapicorn::EventSource::add_poll()).
  @li Third, check() is called for the source to check whether dispatching is needed depending on PollFD states.
  @li Fourth, the source is dispatched if it returened true from either prepare() or check(). If multiple sources are
  ready to be dispatched, the entire process may be repeated several times (after dispatching other sources),
  starting with a new call to prepare() before a particular source is finally dispatched.
 */
