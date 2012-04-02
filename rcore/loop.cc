// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "loop.hh"
#include "strings.hh"
#include <sys/poll.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <algorithm>
#include <list>
#include <cstring>
#if __GLIBC__ == 2 && __GLIBC_MINOR__ >= 10
#  include <sys/eventfd.h>              // defines EFD_SEMAPHORE
#endif

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

// === EventFd ===
EventFd::EventFd ()
{
  fds[0] = -1;
  fds[1] = -1;
}

bool
EventFd::opened ()
{
  return fds[0] >= 0;
}

int
EventFd::open ()
{
  if (opened())
    return 0;
  int err;
  long nflags;
#ifdef EFD_SEMAPHORE
  do
    fds[0] = eventfd (0 /*initval*/, 0 /*flags*/);
  while (fds[0] < 0 && (errno == EAGAIN || errno == EINTR));
#else
  do
    err = pipe (fds);
  while (err < 0 && (errno == EAGAIN || errno == EINTR));
  if (fds[1] >= 0)
    {
      nflags = fcntl (fds[1], F_GETFL, 0);
      nflags |= O_NONBLOCK;
      do
        err = fcntl (fds[1], F_SETFL, nflags);
      while (err < 0 && (errno == EINTR || errno == EAGAIN));
    }
#endif
  if (fds[0] >= 0)
    {
      nflags = fcntl (fds[0], F_GETFL, 0);
      nflags |= O_NONBLOCK;
      do
        err = fcntl (fds[0], F_SETFL, nflags);
      while (err < 0 && (errno == EINTR || errno == EAGAIN));
      return 0;
    }
  return -errno;
}

void
EventFd::wakeup () // wakeup polling end
{
  int err;
#ifdef EFD_SEMAPHORE
  do
    err = eventfd_write (fds[0], 1);
  while (err < 0 && errno == EINTR);
#else
  char w = 'w';
  do
    err = write (fds[1], &w, 1);
  while (err < 0 && errno == EINTR);
#endif
  // EAGAIN occours if too many wakeups are pending
}

int
EventFd::inputfd () // fd for POLLIN
{
  return fds[0];
}

bool
EventFd::pollin ()
{
  struct pollfd pfds[1] = { { inputfd(), PollFD::IN, 0 }, };
  int presult;
  do
    presult = poll (&pfds[0], 1, -1);
  while (presult < 0 && (errno == EAGAIN || errno == EINTR));
  return pfds[0].revents != 0;
}

void
EventFd::flush () // clear pending wakeups
{
  int err;
#ifdef EFD_SEMAPHORE
  eventfd_t bytes8;
  do
    err = eventfd_read (fds[0], &bytes8);
  while (err < 0 && errno == EINTR);
#else
  char buffer[512]; // 512 is posix pipe atomic read/write size
  do
    err = read (fds[0], buffer, sizeof (buffer));
  while (err < 0 && errno == EINTR);
#endif
  // EAGAIN occours if no wakeups are pending
}

EventFd::~EventFd ()
{
#ifdef EFD_SEMAPHORE
  close (fds[0]);
#else
  close (fds[0]);
  close (fds[1]);
#endif
}

// === Stupid ID allocator ===
static volatile uint static_id_counter = 65536;
static uint
alloc_id ()
{
  uint id = Atomic::add (&static_id_counter, 1);
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
  Data  *m_data;
  uint   m_n_elements;
  uint   m_n_reserved;
  Data  *m_reserved;
  template<class D> void swap (D &a, D &b) { D t = a; a = b; b = t; }
public:
  typedef Data* iterator;
  QuickArray (uint n_reserved, Data *reserved) : m_data (reserved), m_n_elements (0), m_n_reserved (n_reserved), m_reserved (reserved) {}
  ~QuickArray()                         { if (LIKELY (m_data) && UNLIKELY (m_data != m_reserved)) free (m_data); }
  uint        size       () const       { return m_n_elements; }
  bool        empty      () const       { return m_n_elements == 0; }
  Data*       data       () const       { return m_data; }
  Data&       operator[] (uint n)       { return m_data[n]; }
  const Data& operator[] (uint n) const { return m_data[n]; }
  iterator    begin      ()             { return &m_data[0]; }
  iterator    end        ()             { return &m_data[m_n_elements]; }
  void        shrink     (uint n)       { m_n_elements = MIN (m_n_elements, n); }
  void        swap       (QuickArray &o)
  {
    swap (m_data,       o.m_data);
    swap (m_n_elements, o.m_n_elements);
    swap (m_n_reserved, o.m_n_reserved);
    swap (m_reserved,   o.m_reserved);
  }
  void        push       (const Data &d)
  {
    const uint idx = m_n_elements++;
    if (UNLIKELY (m_n_elements > m_n_reserved))                 // reserved memory exceeded
      {
        const size_t sz = m_n_elements * sizeof (Data);
        const bool migrate = UNLIKELY (m_data == m_reserved);   // migrate from reserved to malloced
        Data *mem = (Data*) (migrate ? malloc (sz) : realloc (m_data, sz));
        if (UNLIKELY (!mem))
          fatal ("OOM");
        if (migrate)
          {
            memcpy (mem, m_data, sz - 1 * sizeof (Data));
            m_reserved = NULL;
            m_n_reserved = 0;
          }
        m_data = mem;
      }
    m_data[idx] = d;
  }
};
struct EventLoop::QuickPfdArray : public QuickArray<PollFD> {
  QuickPfdArray (uint n_reserved, PollFD *reserved) : QuickArray (n_reserved, reserved) {}
};
struct EventLoop::QuickSourceArray : public QuickArray<Source*> {
  QuickSourceArray (uint n_reserved, Source **reserved) : QuickArray (n_reserved, reserved) {}
};

// === EventLoop ===
EventLoop::EventLoop (MainLoop &main) :
  m_main_loop (main), m_dispatch_priority (0), m_poll_sources (*new (pollmem1) QuickSourceArray (0, NULL))
{
  RAPICORN_STATIC_ASSERT (sizeof (QuickSourceArray) <= sizeof (EventLoop::pollmem1));
  // we cannot *use* m_main_loop yet, because we might be called from within MainLoop::MainLoop(), see SlaveLoop()
}

EventLoop::~EventLoop ()
{
  unpoll_sources_U();
  m_poll_sources.~QuickSourceArray();
  // we cannot *use* m_main_loop anymore, because we might be called from within MainLoop::MainLoop(), see ~SlaveLoop()
}

inline EventLoop::Source*
EventLoop::find_first_L()
{
  return m_sources.empty() ? NULL : m_sources[0];
}

inline EventLoop::Source*
EventLoop::find_source_L (uint id)
{
  for (SourceList::iterator lit = m_sources.begin(); lit != m_sources.end(); lit++)
    if (id == (*lit)->m_id)
      return *lit;
  return NULL;
}

bool
EventLoop::has_primary_L()
{
  for (SourceList::iterator lit = m_sources.begin(); lit != m_sources.end(); lit++)
    if ((*lit)->primary())
      return true;
  return false;
}

bool
EventLoop::has_primary()
{
  ScopedLock<Mutex> locker (m_main_loop.mutex());
  return has_primary_L();
}

uint
EventLoop::add (Source *source,
                int     priority)
{
  ScopedLock<Mutex> locker (m_main_loop.mutex());
  assert_return (source->m_loop == NULL, 0);
  source->m_loop = this;
  ref_sink (source);
  source->m_id = alloc_id();
  source->m_loop_state = WAITING;
  source->m_priority = priority;
  m_sources.push_back (source);
  locker.unlock();
  wakeup();
  return source->m_id;
}

void
EventLoop::remove_source_Lm (Source *source)
{
  ScopedLock<Mutex> locker (m_main_loop.mutex(), BALANCED);
  assert_return (source->m_loop == this);
  source->m_loop = NULL;
  source->m_loop_state = WAITING;
  m_sources.erase (find (m_sources.begin(), m_sources.end(), source));
  release_id (source->m_id);
  source->m_id = 0;
  locker.unlock();
  source->destroy();
  unref (source);
  locker.lock();
}

bool
EventLoop::try_remove (uint id)
{
  ScopedLock<Mutex> locker (m_main_loop.mutex());
  Source *source = find_source_L (id);
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
  for (Source *source = find_first_L(); source != NULL; source = find_first_L())
    remove_source_Lm (source);
  ScopedLock<Mutex> locker (m_main_loop.mutex(), BALANCED);
  locker.unlock();
  unpoll_sources_U(); // unlocked
  locker.lock();
}

void
EventLoop::kill_sources()
{
  ScopedLock<Mutex> locker (m_main_loop.mutex());
  kill_sources_Lm();
  locker.unlock();
  wakeup();
}

void
EventLoop::wakeup ()
{
  // this needs to work unlocked
  m_main_loop.wakeup_poll();
}

// === MainLoop ===
static bool dummy_sense() { return false; }
static void dummy_nop()   {}
static const MainLoop::LockHooks dummy_hooks = { dummy_sense, dummy_nop, dummy_nop };

MainLoop::MainLoop() :
  EventLoop (*this), // sets *this as MainLoop on self
  m_rr_index (0), m_running (true), m_quit_code (0), m_lock_hooks (dummy_hooks)
{
  ScopedLock<Mutex> locker (m_main_loop.mutex());
  add_loop_L (*this);
  int err = m_eventfd.open();
  if (err < 0)
    fatal ("MainLoop: failed to create wakeup pipe: %s", strerror (-err));
  // m_running and m_eventfd need to be setup here, so calling quit() before run() works
}

MainLoop::~MainLoop()
{
  kill_loops(); // this->kill_sources_Lm()
  ScopedLock<Mutex> locker (m_mutex);
  remove_loop_L (*this);
  assert_return (m_loops.empty() == true);
}

void
MainLoop::set_lock_hooks (const LockHooks &hooks)
{
  ScopedLock<Mutex> locker (m_mutex);
  if (hooks.sense)
    {
      assert_return (hooks.lock != NULL && hooks.unlock != NULL);
      m_lock_hooks = hooks;
    }
  else
    m_lock_hooks = dummy_hooks;
}

void
MainLoop::wakeup_poll()
{
  if (m_eventfd.opened())
    m_eventfd.wakeup();
}

void
MainLoop::add_loop_L (EventLoop &loop)
{
  assert_return (this == &loop.m_main_loop);
  m_loops.push_back (&loop);
  wakeup_poll();
}

void
MainLoop::remove_loop_L (EventLoop &loop)
{
  assert_return (this == &loop.m_main_loop);
  vector<EventLoop*>::iterator it = std::find (m_loops.begin(), m_loops.end(), &loop);
  assert_return (it != m_loops.end());
  m_loops.erase (it);
  wakeup_poll();
}

void
MainLoop::kill_loops()
{
  ScopedLock<Mutex> locker (m_mutex);
  EventLoop *spare = this;
  // create referenced loop list
  std::list<EventLoop*> loops;
  for (size_t i = 0; i < m_loops.size(); i++)
    {
      loops.push_back (m_loops[i]);
      if (spare != m_loops[i]) // avoid ref(this) during dtor
        ref (m_loops[i]);
    }
  // kill loops
  for (std::list<EventLoop*>::iterator lit = loops.begin(); lit != loops.end(); lit++)
    {
      EventLoop &loop = **lit;
      if (this == &loop.m_main_loop)
        loop.kill_sources_Lm();
    }
  this->kill_sources_Lm();
  // cleanup
  locker.unlock();
  for (std::list<EventLoop*>::iterator lit = loops.begin(); lit != loops.end(); lit++)
    {
      if (spare != *lit) // avoid unref(this) during dtor
        unref (*lit); // unlocked
    }
  locker.lock();
  wakeup_poll();
}

int
MainLoop::run ()
{
  ScopedLock<Mutex> locker (m_mutex);
  State state;
  while (ISLIKELY (m_running))
    iterate_loops_Lm (state, true, true);
  return m_quit_code;
}

void
MainLoop::quit (int quit_code)
{
  ScopedLock<Mutex> locker (m_mutex);
  m_quit_code = quit_code;
  m_running = false;
  wakeup();
}

bool
MainLoop::finishable_L()
{
  // loop list shouldn't be modified by querying finishable state
  bool found_primary = false;
  for (size_t i = 0; !found_primary && i < m_loops.size(); i++)
    if (m_loops[i]->has_primary_L())
      found_primary = true;
  return !found_primary; // finishable if no primary sources remain
}

bool
MainLoop::finishable()
{
  ScopedLock<Mutex> locker (m_mutex);
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
  ScopedLock<Mutex> locker (m_mutex);
  State state;
  return iterate_loops_Lm (state, may_block, true);
}

void
MainLoop::iterate_pending()
{
  ScopedLock<Mutex> locker (m_mutex);
  State state;
  while (iterate_loops_Lm (state, false, true));
}

void
EventLoop::unpoll_sources_U() // must be unlocked!
{
  QuickSourceArray sources (0, NULL);
  // clear poll sources
  sources.swap (m_poll_sources);
  for (QuickSourceArray::iterator lit = sources.begin(); lit != sources.end(); lit++)
    unref (*lit); // unlocked
}

static const int64 supraint_priobase = 2147483648LL; // INT32_MAX + 1, above all possible int32 values

void
EventLoop::collect_sources_Lm (State &state)
{
  // enforce clean slate
  if (UNLIKELY (!m_poll_sources.empty()))
    {
      Mutex &main_mutex = m_main_loop.mutex();
      main_mutex.unlock();
      unpoll_sources_U(); // unlocked
      main_mutex.lock();
      assert (m_poll_sources.empty());
    }
  // since m_poll_sources is empty, poll_candidates can reuse pollmem2
  QuickSourceArray poll_candidates (ARRAY_SIZE (pollmem2), pollmem2);
  // determine dispatch priority & collect sources for preparing
  m_dispatch_priority = supraint_priobase; // dispatch priority, cover full int32 range initially
  for (SourceList::iterator lit = m_sources.begin(); lit != m_sources.end(); lit++)
    {
      Source &source = **lit;
      if (UNLIKELY (!state.seen_primary && source.m_primary))
        state.seen_primary = true;
      if (source.m_loop != this ||                              // consider undestroyed
          (source.m_dispatching && !source.m_may_recurse))      // avoid unallowed recursion
        continue;
      if (source.m_priority < m_dispatch_priority &&
          source.m_loop_state == NEEDS_DISPATCH)                // dispatch priority needs adjusting
        m_dispatch_priority = source.m_priority;                // upgrade dispatch priority
      if (source.m_priority < m_dispatch_priority ||            // prepare preempting sources
          (source.m_priority == m_dispatch_priority &&
           source.m_loop_state == NEEDS_DISPATCH))              // re-poll sources that need dispatching
        poll_candidates.push (&source);                         // collect only, adding ref() next
    }
  // ensure ref counts on all prepare sources
  uint j = 0;
  for (uint i = 0; i < poll_candidates.size(); i++)
    if (poll_candidates[i]->m_priority < m_dispatch_priority || // throw away lower priority sources
        (poll_candidates[i]->m_priority == m_dispatch_priority &&
         poll_candidates[i]->m_loop_state == NEEDS_DISPATCH))   // re-poll sources that need dispatching
      poll_candidates[j++] = ref (poll_candidates[i]);
  poll_candidates.shrink (j);
  m_poll_sources.swap (poll_candidates);                // ref()ed sources <= dispatch priority
  assert (poll_candidates.empty());
}

bool
EventLoop::prepare_sources_Lm (State          &state,
                               int64          *timeout_usecs,
                               QuickPfdArray  &pfda)
{
  Mutex &main_mutex = m_main_loop.mutex();
  // prepare sources, up to NEEDS_DISPATCH priority
  for (QuickSourceArray::iterator lit = m_poll_sources.begin(); lit != m_poll_sources.end(); lit++)
    {
      Source &source = **lit;
      if (source.m_loop != this) // test undestroyed
        continue;
      int64 timeout = -1;
      main_mutex.unlock();
      const bool need_dispatch = source.prepare (state, &timeout);
      main_mutex.lock();
      if (source.m_loop != this)
        continue; // ignore newly destroyed sources
      if (need_dispatch)
        {
          m_dispatch_priority = MIN (m_dispatch_priority, source.m_priority); // upgrade dispatch priority
          source.m_loop_state = NEEDS_DISPATCH;
          continue;
        }
      source.m_loop_state = PREPARED;
      if (timeout >= 0)
        *timeout_usecs = MIN (*timeout_usecs, timeout);
      uint npfds = source.n_pfds();
      for (uint i = 0; i < npfds; i++)
        if (source.m_pfds[i].pfd->fd >= 0)
          {
            uint idx = pfda.size();
            source.m_pfds[i].idx = idx;
            pfda.push (*source.m_pfds[i].pfd);
            pfda[idx].revents = 0;
          }
        else
          source.m_pfds[i].idx = UINT_MAX;
    }
  return m_dispatch_priority < supraint_priobase;
}

bool
EventLoop::check_sources_Lm (State               &state,
                             const QuickPfdArray &pfda)
{
  Mutex &main_mutex = m_main_loop.mutex();
  // check polled sources
  for (QuickSourceArray::iterator lit = m_poll_sources.begin(); lit != m_poll_sources.end(); lit++)
    {
      Source &source = **lit;
      if (source.m_loop != this && // test undestroyed
          source.m_loop_state != PREPARED)
        continue; // only check prepared sources
      uint npfds = source.n_pfds();
      for (uint i = 0; i < npfds; i++)
        {
          uint idx = source.m_pfds[i].idx;
          if (idx < pfda.size() &&
              source.m_pfds[i].pfd->fd == pfda[idx].fd)
            source.m_pfds[i].pfd->revents = pfda[idx].revents;
          else
            source.m_pfds[i].idx = UINT_MAX;
        }
      main_mutex.unlock();
      bool need_dispatch = source.check (state);
      main_mutex.lock();
      if (source.m_loop != this)
        continue; // ignore newly destroyed sources
      if (need_dispatch)
        {
          m_dispatch_priority = MIN (m_dispatch_priority, source.m_priority); // upgrade dispatch priority
          source.m_loop_state = NEEDS_DISPATCH;
        }
      else
        source.m_loop_state = WAITING;
    }
  return m_dispatch_priority < supraint_priobase;
}

EventLoop::Source*
EventLoop::dispatch_source_Lm (State &state)
{
  Mutex &main_mutex = m_main_loop.mutex();
  // find a source to dispatch at m_dispatch_priority
  Source *dispatch_source = NULL;
  for (QuickSourceArray::iterator lit = m_poll_sources.begin(); lit != m_poll_sources.end(); lit++)
    {
      Source &source = **lit;
      if (source.m_loop == this &&                    // test undestroyed
          source.m_priority == m_dispatch_priority && // only dispatch at dispatch priority
          source.m_loop_state == NEEDS_DISPATCH)
        {
          dispatch_source = &source;
          break;
        }
    }
  m_dispatch_priority = supraint_priobase;
  // dispatch single source
  if (dispatch_source)
    {
      dispatch_source->m_loop_state = WAITING;
      const bool old_was_dispatching = dispatch_source->m_was_dispatching;
      dispatch_source->m_was_dispatching = dispatch_source->m_dispatching;
      dispatch_source->m_dispatching = true;
      ref (dispatch_source);    // ref() to keep alive even if everything else is destroyed
      main_mutex.unlock();
      const bool keep_alive = dispatch_source->dispatch (state);
      main_mutex.lock();
      dispatch_source->m_dispatching = dispatch_source->m_was_dispatching;
      dispatch_source->m_was_dispatching = old_was_dispatching;
      if (dispatch_source->m_loop == this && !keep_alive)
        remove_source_Lm (dispatch_source);
    }
  return dispatch_source;       // unref() carried out by caller
}

bool
MainLoop::iterate_loops_Lm (State &state, bool may_block, bool may_dispatch)
{
  Mutex &main_mutex = m_main_loop.mutex();
  int64 timeout_usecs = INT64_MAX;
  bool any_dispatchable = false;
  PollFD reserved_pfd_mem[7];   // store PollFD array in stack memory, to reduce malloc overhead
  QuickPfdArray pfda (ARRAY_SIZE (reserved_pfd_mem), reserved_pfd_mem); // pfda.size() == 0
  // allow poll wakeups
  const PollFD wakeup = { m_eventfd.inputfd(), PollFD::IN, 0 };
  const uint wakeup_idx = 0; // wakeup_idx = pfda.size();
  pfda.push (wakeup);
  // create referenced loop list
  const uint nloops = m_loops.size();
  EventLoop* loops[nloops];
  bool dispatchable[nloops];
  for (size_t i = 0; i < nloops; i++)
    loops[i] = ref (m_loops[i]);
  // collect
  state.seen_primary = false;
  for (size_t i = 0; i < nloops; i++)
    loops[i]->collect_sources_Lm (state);
  // prepare
  state.current_time_usecs = timestamp_realtime();
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
  LockHooks hooks = m_lock_hooks;
  int presult;
  main_mutex.unlock();
  const bool needs_locking = hooks.sense();
  if (needs_locking)
    hooks.unlock();
  do
    presult = poll ((struct pollfd*) &pfda[0], pfda.size(), MIN (timeout_msecs, INT_MAX));
  while (presult < 0 && errno == EAGAIN); // EINTR may indicate a signal
  if (needs_locking)
    hooks.lock();
  main_mutex.lock();
  if (presult < 0)
    critical ("MainLoop: poll() failed: %s", strerror());
  else if (pfda[wakeup_idx].revents)
    m_eventfd.flush(); // restart queueing wakeups, possibly triggered by dispatching
  // check
  state.current_time_usecs = timestamp_realtime();
  for (size_t i = 0; i < nloops; i++)
    {
      dispatchable[i] |= loops[i]->check_sources_Lm (state, pfda);
      any_dispatchable |= dispatchable[i];
    }
  // dispatch
  Source *unref_source = NULL;
  if (may_dispatch && any_dispatchable)
    {
      size_t index, i = nloops;
      do        // find next dispatchable loop in round-robin fashion
        index = m_rr_index++ % nloops;
      while (!dispatchable[index] && i--);
      unref_source = loops[index]->dispatch_source_Lm (state); // passes on dispatch_source reference
    }
  // cleanup
  main_mutex.unlock();
  if (unref_source)
    unref (unref_source); // unlocked
  for (size_t i = 0; i < nloops; i++)
    {
      loops[i]->unpoll_sources_U(); // unlocked
      unref (loops[i]); // unlocked
    }
  main_mutex.lock();
  return any_dispatchable; // need to dispatch or recheck
}

struct SlaveLoop : public EventLoop {
  SlaveLoop (MainLoop &main) :
    EventLoop (main)
  {
    ScopedLock<Mutex> locker (m_main_loop.mutex());
    ref (m_main_loop);
    m_main_loop.add_loop_L (*this);
  }
  ~SlaveLoop()
  {
    ScopedLock<Mutex> locker (m_main_loop.mutex());
    kill_sources_Lm();
    m_main_loop.remove_loop_L (*this);
    locker.unlock();
    unref (m_main_loop); // unlocked
  }
};

EventLoop*
MainLoop::new_slave ()
{
  return new SlaveLoop (*this);
}

MainLoop*
MainLoop::_new ()
{
  return new MainLoop();
}

// === EventLoop::State ===
EventLoop::State::State() :
  current_time_usecs (0),
  seen_primary (false)
{}

// === EventLoop::Source ===
EventLoop::Source::Source () :
  m_loop (NULL),
  m_pfds (NULL),
  m_id (0),
  m_priority (INT_MAX),
  m_loop_state (0),
  m_may_recurse (0),
  m_dispatching (0),
  m_was_dispatching (0),
  m_primary (0)
{}

uint
EventLoop::Source::n_pfds ()
{
  uint i = 0;
  if (m_pfds)
    while (m_pfds[i].pfd)
      i++;
  return i;
}

void
EventLoop::Source::may_recurse (bool may_recurse)
{
  m_may_recurse = may_recurse;
}

bool
EventLoop::Source::may_recurse () const
{
  return m_may_recurse;
}

bool
EventLoop::Source::primary () const
{
  return m_primary;
}

void
EventLoop::Source::primary (bool is_primary)
{
  m_primary = is_primary;
}

bool
EventLoop::Source::recursion () const
{
  return m_dispatching && m_was_dispatching;
}

void
EventLoop::Source::add_poll (PollFD *const pfd)
{
  const uint idx = n_pfds();
  uint npfds = idx + 1;
  m_pfds = (typeof (m_pfds)) realloc (m_pfds, sizeof (m_pfds[0]) * (npfds + 1));
  if (!m_pfds)
    fatal ("EventLoopSource: out of memory");
  m_pfds[npfds].idx = UINT_MAX;
  m_pfds[npfds].pfd = NULL;
  m_pfds[idx].idx = UINT_MAX;
  m_pfds[idx].pfd = pfd;
}

void
EventLoop::Source::remove_poll (PollFD *const pfd)
{
  uint idx, npfds = n_pfds();
  for (idx = 0; idx < npfds; idx++)
    if (m_pfds[idx].pfd == pfd)
      break;
  if (idx < npfds)
    {
      m_pfds[idx].idx = UINT_MAX;
      m_pfds[idx].pfd = m_pfds[npfds - 1].pfd;
      m_pfds[idx].idx = m_pfds[npfds - 1].idx;
      m_pfds[npfds - 1].idx = UINT_MAX;
      m_pfds[npfds - 1].pfd = NULL;
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
  if (m_loop)
    m_loop->try_remove (source_id());
}

EventLoop::Source::~Source ()
{
  RAPICORN_ASSERT (m_loop == NULL);
  if (m_pfds)
    free (m_pfds);
}

/* --- EventLoop::TimedSource --- */
EventLoop::TimedSource::TimedSource (Signals::Trampoline0<void> &vt,
                                     uint initial_interval_msecs,
                                     uint repeat_interval_msecs) :
  m_expiration_usecs (timestamp_realtime() + 1000ULL * initial_interval_msecs),
  m_interval_msecs (repeat_interval_msecs), m_first_interval (true),
  m_oneshot (true), m_vtrampoline (ref_sink (&vt))
{}

EventLoop::TimedSource::TimedSource (Signals::Trampoline0<bool> &bt,
                                     uint initial_interval_msecs,
                                     uint repeat_interval_msecs) :
  m_expiration_usecs (timestamp_realtime() + 1000ULL * initial_interval_msecs),
  m_interval_msecs (repeat_interval_msecs), m_first_interval (true),
  m_oneshot (false), m_btrampoline (ref_sink (&bt))
{}

bool
EventLoop::TimedSource::prepare (const State &state,
                                 int64 *timeout_usecs_p)
{
  if (state.current_time_usecs >= m_expiration_usecs)
    return true;                                            /* timeout expired */
  if (!m_first_interval)
    {
      uint64 interval = m_interval_msecs * 1000ULL;
      if (state.current_time_usecs + interval < m_expiration_usecs)
        m_expiration_usecs = state.current_time_usecs + interval; /* clock warped back in time */
    }
  *timeout_usecs_p = MIN (INT_MAX, m_expiration_usecs - state.current_time_usecs);
  return 0 == *timeout_usecs_p;
}

bool
EventLoop::TimedSource::check (const State &state)
{
  return state.current_time_usecs >= m_expiration_usecs;
}

bool
EventLoop::TimedSource::dispatch (const State &state)
{
  bool repeat = false;
  m_first_interval = false;
  if (m_oneshot && m_vtrampoline->callable())
    (*m_vtrampoline) ();
  else if (!m_oneshot && m_btrampoline->callable())
    repeat = (*m_btrampoline) ();
  if (repeat)
    m_expiration_usecs = timestamp_realtime() + 1000ULL * m_interval_msecs;
  return repeat;
}

EventLoop::TimedSource::~TimedSource ()
{
  if (m_oneshot)
    unref (m_vtrampoline);
  else
    unref (m_btrampoline);
}

// == EventLoop::PollFDSource ==
/*! @class EventLoop::PollFDSource
 * A PollFDSource can be used to execute a callback function from the main loop,
 * depending on certain file descriptor states.
 * The modes supported for polling the file descriptor are as follows:
 * @li @c "w" - poll writable
 * @li @c "r" - poll readable
 * @li @c "p" - poll urgent redable
 * @li @c "b" - blocking
 * @li @c "E" - ignore erros (or auto destroy)
 * @li @c "H" - ignore hangup (or auto destroy)
 * @li @c "C" - prevent auto close on destroy
 */
EventLoop::PollFDSource::PollFDSource (Signals::Trampoline1<bool,PollFD&> &bt,
                                       int                                 fd,
                                       const String                       &mode) :
  m_pfd ((PollFD) { fd, 0, 0 }),
  m_ignore_errors (strchr (mode.c_str(), 'E') != NULL),
  m_ignore_hangup (strchr (mode.c_str(), 'H') != NULL),
  m_never_close (strchr (mode.c_str(), 'C') != NULL),
  m_oneshot (false), m_btrampoline (ref_sink (&bt))
{
  construct (mode);
}

EventLoop::PollFDSource::PollFDSource (Signals::Trampoline1<void,PollFD&> &vt,
                                       int                                 fd,
                                       const String                       &mode) :
  m_pfd ((PollFD) { fd, 0, 0 }),
  m_ignore_errors (strchr (mode.c_str(), 'E') != NULL),
  m_ignore_hangup (strchr (mode.c_str(), 'H') != NULL),
  m_never_close (strchr (mode.c_str(), 'C') != NULL),
  m_oneshot (true), m_vtrampoline (ref_sink (&vt))
{
  construct (mode);
}

void
EventLoop::PollFDSource::construct (const String &mode)
{
  add_poll (&m_pfd);
  m_pfd.events |= strchr (mode.c_str(), 'w') ? PollFD::OUT : 0;
  m_pfd.events |= strchr (mode.c_str(), 'r') ? PollFD::IN : 0;
  m_pfd.events |= strchr (mode.c_str(), 'p') ? PollFD::PRI : 0;
  if (m_pfd.fd >= 0)
    {
      const long lflags = fcntl (m_pfd.fd, F_GETFL, 0);
      long nflags = lflags;
      if (strchr (mode.c_str(), 'b'))
        nflags &= ~(long) O_NONBLOCK;
      else
        nflags |= O_NONBLOCK;
      if (nflags != lflags)
        {
          int err;
          do
            err = fcntl (m_pfd.fd, F_SETFL, nflags);
          while (err < 0 && (errno == EINTR || errno == EAGAIN));
        }
    }
}

bool
EventLoop::PollFDSource::prepare (const State &state,
                                  int64 *timeout_usecs_p)
{
  m_pfd.revents = 0;
  return m_pfd.fd < 0;
}

bool
EventLoop::PollFDSource::check (const State &state)
{
  return m_pfd.fd < 0 || m_pfd.revents != 0;
}

bool
EventLoop::PollFDSource::dispatch (const State &state)
{
  bool keep_alive = false;
  if (m_pfd.fd >= 0 && (m_pfd.revents & PollFD::NVAL))
    ; // close down
  else if (m_pfd.fd >= 0 && !m_ignore_errors && (m_pfd.revents & PollFD::ERR))
    ; // close down
  else if (m_pfd.fd >= 0 && !m_ignore_hangup && (m_pfd.revents & PollFD::HUP))
    ; // close down
  else if (m_oneshot && m_vtrampoline->callable())
    (*m_vtrampoline) (m_pfd);
  else if (!m_oneshot && m_btrampoline->callable())
    keep_alive = (*m_btrampoline) (m_pfd);
  /* close down */
  if (!keep_alive)
    {
      if (!m_never_close && m_pfd.fd >= 0)
        close (m_pfd.fd);
      m_pfd.fd = -1;
    }
  return keep_alive;
}

void
EventLoop::PollFDSource::destroy()
{
  /* close down */
  if (!m_never_close && m_pfd.fd >= 0)
    close (m_pfd.fd);
  m_pfd.fd = -1;
}

EventLoop::PollFDSource::~PollFDSource ()
{
  if (m_oneshot)
    unref (m_vtrampoline);
  else
    unref (m_btrampoline);
}

} // Rapicorn

// == Loop Description ==
/*! @page EventLoops    Event Loops and Event Sources
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
