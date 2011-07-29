// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "loop.hh"
#include "commands.hh"
#include <sys/poll.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <algorithm>
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
  return_if_fail (id != 0);
  // FIXME: ned proper ID allocator
}

// === EventLoop ===
EventLoop::EventLoop (MainLoop &main) :
  m_main_loop (main), m_dispatch_priority (0)
{
  // we cannot *use* m_main_loop yet, because we might be called from within MainLoop::MainLoop(), see SlaveLoop()
}

EventLoop::~EventLoop ()
{
  unpoll_sources_U();
  // we cannot *use* m_main_loop anymore, because we might be called from within MainLoop::MainLoop(), see ~SlaveLoop()
}

uint64
EventLoop::get_current_time_usecs ()
{
  struct timeval tv;
  gettimeofday (&tv, NULL);
  uint64 usecs = tv.tv_sec; // promote to 64 bit
  return usecs * 1000000 + tv.tv_usec;
}

inline EventLoop::Source*
EventLoop::find_first_L()
{
  SourceListMap::iterator it = m_sources.begin();
  if (it != m_sources.end())
    {
      SourceList &slist = (*it).second;
      SourceList::iterator lit = slist.begin();
      if (lit != slist.end())
        return *lit;
    }
  return NULL;
}

inline EventLoop::Source*
EventLoop::find_source_L (uint id)
{
  for (SourceListMap::iterator it = m_sources.begin(); it != m_sources.end(); it++)
    {
      SourceList &slist = (*it).second;
      for (SourceList::iterator lit = slist.begin(); lit != slist.end(); lit++)
        if (id == (*lit)->m_id)
          return *lit;
    }
  return NULL;
}

bool
EventLoop::has_primary_L()
{
  for (SourceListMap::iterator it = m_sources.begin(); it != m_sources.end(); it++)
    {
      SourceList &slist = (*it).second;
      for (SourceList::iterator lit = slist.begin(); lit != slist.end(); lit++)
        if ((*lit)->primary())
          return true;
    }
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
  return_val_if_fail (source->m_main_loop == NULL, 0);
  source->m_main_loop = this;
  ref_sink (source);
  source->m_id = alloc_id();
  source->m_loop_state = WAITING;
  source->m_priority = priority;
  SourceList &slist = m_sources[priority];
  slist.push_back (source);
  locker.unlock();
  wakeup();
  return source->m_id;
}

void
EventLoop::remove_source_Lm (Source *source)
{
  ScopedLock<Mutex> locker (m_main_loop.mutex(), BALANCED);
  return_if_fail (source->m_main_loop == this);
  source->m_main_loop = NULL;
  source->m_loop_state = WAITING;
  SourceList &slist = m_sources[source->m_priority];
  slist.remove (source);
  if (slist.empty())
    m_sources.erase (m_sources.find (source->m_priority));
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
MainLoop::MainLoop() :
  EventLoop (*this), // sets *this as MainLoop on self
  m_rr_index (0), m_auto_finish (true), m_running (false), m_quit_code (0)
{
  ScopedLock<Mutex> locker (m_main_loop.mutex());
  add_loop_L (*this);
}

MainLoop::~MainLoop()
{
  kill_loops(); // this->kill_sources_Lm()
  ScopedLock<Mutex> locker (m_mutex);
  remove_loop_L (*this);
  return_if_fail (m_loops.empty() == true);
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
  return_if_fail (this == &loop.m_main_loop);
  m_loops.push_back (&loop);
  wakeup_poll();
}

void
MainLoop::remove_loop_L (EventLoop &loop)
{
  return_if_fail (this == &loop.m_main_loop);
  vector<EventLoop*>::iterator it = std::find (m_loops.begin(), m_loops.end(), &loop);
  return_if_fail (it != m_loops.end());
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
  m_quit_code = 0;
  m_running = true;
  bool seen_primary = false; // hint towards not being finishable
  iterate_loops_Lm (false, false, &seen_primary);   // check sources
  while (m_running)
    {
      if (m_auto_finish && !seen_primary)
        {
          // really determine if we're finishable, seen_primary is merely a hint
          locker.unlock();
          bool finished = finishable(); // unlocked
          locker.lock();
          if (finished)
            break;
        }
      iterate_loops_Lm (true, true, &seen_primary); // poll & dispatch
    }
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
MainLoop::auto_finish ()
{
  ScopedLock<Mutex> locker (m_mutex);
  return m_auto_finish;
}

void
MainLoop::auto_finish (bool autofinish)
{
  ScopedLock<Mutex> locker (m_mutex);
  m_auto_finish = autofinish;
  if (m_auto_finish)
    wakeup();
}

bool
MainLoop::finishable()
{
  ScopedLock<Mutex> locker (m_mutex);
  // loop list shouldn't be modified by querying finishable state
  bool found_primary = false;
  for (size_t i = 0; !found_primary && i < m_loops.size(); i++)
    if (m_loops[i]->has_primary_L())
      found_primary = true;
  return !found_primary; // finishable if no primary sources remain
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
  bool seen_primary = false;
  return iterate_loops_Lm (may_block, true, &seen_primary);
}

void
MainLoop::iterate_pending()
{
  ScopedLock<Mutex> locker (m_mutex);
  bool seen_primary = false;
  while (iterate_loops_Lm (false, true, &seen_primary));
}

void
EventLoop::unpoll_sources_U() // must be unlocked!
{
  SourceList sources;
  // clear poll sources
  sources.swap (m_poll_sources);
  for (SourceList::iterator lit = sources.begin(); lit != sources.end(); lit++)
    unref (*lit); // unlocked
}

static const int64 supraint_priobase = 2147483648LL; // INT32_MAX + 1, above all possible int32 values

bool
EventLoop::sense_sources_L (bool *seen_primary)
{
  m_dispatch_priority = supraint_priobase; // dispatch priority, cover full int32 range initially
  // find sources that need dispatching
  for (SourceListMap::iterator it = m_sources.begin(); it != m_sources.end(); it++)
    {
      SourceList &slist = (*it).second;
      for (SourceList::iterator lit = slist.begin(); lit != slist.end(); lit++)
        {
          Source &source = **lit;
          *seen_primary |= source.m_primary;
          if (source.m_main_loop == this &&             // test undestroyed
              source.m_loop_state == NEEDS_DISPATCH &&  // find NEEDS_DISPATCH sources
              (!source.m_dispatching || source.m_may_recurse))
            m_dispatch_priority = MIN (m_dispatch_priority, source.m_priority);
        }
    }
  return m_dispatch_priority < supraint_priobase;
}

bool
EventLoop::prepare_sources_Lm (vector<PollFD> &pfds,
                               int64          *timeout_usecs)
{
  ScopedLock<Mutex> locker (m_main_loop.mutex(), BALANCED);
  // enforce clean slate
  if (!m_poll_sources.empty())
    {
      locker.unlock();
      unpoll_sources_U(); // unlocked
      locker.lock();
    }
  // collect sources that need preparing
  for (SourceListMap::iterator it = m_sources.begin(); it != m_sources.end(); it++)
    {
      SourceList &slist = (*it).second;
      for (SourceList::iterator lit = slist.begin(); lit != slist.end(); lit++)
        {
          Source &source = **lit;
          if (source.m_main_loop == this &&               // test undestroyed
              (!source.m_dispatching || source.m_may_recurse) &&
              (source.m_priority < m_dispatch_priority || // only prepare preempting sources
               (source.m_priority == m_dispatch_priority &&
                source.m_loop_state == NEEDS_DISPATCH)))  // re-poll sources that need dispatching
            m_poll_sources.push_back (ref (&source));
        }
    }
  // prepare sources, up to NEEDS_DISPATCH priority
  uint64 current_time_usecs = EventLoop::get_current_time_usecs();
  for (SourceList::iterator lit = m_poll_sources.begin(); lit != m_poll_sources.end(); lit++)
    {
      Source &source = **lit;
      if (source.m_main_loop != this) // test undestroyed
        continue;
      int64 timeout = -1;
      locker.unlock();
      const bool need_dispatch = source.prepare (current_time_usecs, &timeout);
      locker.lock();
      if (source.m_main_loop != this)
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
            uint idx = pfds.size();
            source.m_pfds[i].idx = idx;
            pfds.push_back (*source.m_pfds[i].pfd);
            pfds[idx].revents = 0;
          }
        else
          source.m_pfds[i].idx = UINT_MAX;
    }
  return m_dispatch_priority < supraint_priobase;
}

bool
EventLoop::check_sources_Lm (const vector<PollFD> &pfds)
{
  ScopedLock<Mutex> locker (m_main_loop.mutex(), BALANCED);
  // check polled sources
  uint64 current_time_usecs = EventLoop::get_current_time_usecs();
  for (SourceList::iterator lit = m_poll_sources.begin(); lit != m_poll_sources.end(); lit++)
    {
      Source &source = **lit;
      if (source.m_main_loop != this && // test undestroyed
          source.m_loop_state != PREPARED)
        continue; // only check prepared sources
      uint npfds = source.n_pfds();
      for (uint i = 0; i < npfds; i++)
        {
          uint idx = source.m_pfds[i].idx;
          if (idx < pfds.size() &&
              source.m_pfds[i].pfd->fd == pfds[idx].fd)
            source.m_pfds[i].pfd->revents = pfds[idx].revents;
          else
            source.m_pfds[i].idx = UINT_MAX;
        }
      locker.unlock();
      bool need_dispatch = source.check (current_time_usecs);
      locker.lock();
      if (source.m_main_loop != this)
        continue; // ignore newly destroyed sources
      if (need_dispatch)
        {
          m_dispatch_priority = MIN (m_dispatch_priority, source.m_priority); // upgrade dispatch priority
          source.m_loop_state = NEEDS_DISPATCH;
        }
      else
        source.m_loop_state = WAITING;
    }
  // clean up
  if (!m_poll_sources.empty())
    {
      locker.unlock();
      unpoll_sources_U(); // unlocked
      locker.lock();
    }
  return m_dispatch_priority < supraint_priobase;
}

void
EventLoop::dispatch_source_Lm ()
{
  ScopedLock<Mutex> locker (m_main_loop.mutex(), BALANCED);
  // find a source to dispatch at m_dispatch_priority
  Source *dispatch_source = NULL;
  for (SourceListMap::iterator it = m_sources.begin(); it != m_sources.end() && !dispatch_source; it++)
    {
      SourceList &slist = (*it).second;
      for (SourceList::iterator lit = slist.begin(); lit != slist.end() && !dispatch_source; lit++)
        {
          Source &source = **lit;
          if (source.m_main_loop == this &&               // test undestroyed
              source.m_priority == m_dispatch_priority && // only dispatch at dispatch priority
              source.m_loop_state == NEEDS_DISPATCH)
            dispatch_source = ref (&source);
        }
    }
  m_dispatch_priority = supraint_priobase;
  // dispatch single source
  if (dispatch_source)
    {
      Source &source = *dispatch_source;
      source.m_loop_state = WAITING;
      const bool old_was_dispatching = source.m_was_dispatching;
      source.m_was_dispatching = source.m_dispatching;
      source.m_dispatching = true;
      locker.unlock();
      const bool keep_alive = source.dispatch ();
      locker.lock();
      source.m_dispatching = source.m_was_dispatching;
      source.m_was_dispatching = old_was_dispatching;
      if (source.m_main_loop == this && !keep_alive)
        remove_source_Lm (&source);
      // clean up
      locker.unlock();
      unref (dispatch_source); // unlocked
      locker.lock();
    }
}

bool
MainLoop::iterate_loops_Lm (bool may_block, bool may_dispatch, bool *seen_primary)
{
  ScopedLock<Mutex> locker (m_main_loop.mutex(), BALANCED);
  int err = m_eventfd.open();
  if (err < 0)
    fatal ("MainLoop: failed to create wakeup pipe: %s", strerror (-err));
  int64 timeout_usecs = INT64_MAX;
  bool any_dispatchable = false;
  vector<PollFD> pfds;
  pfds.reserve (7); // profiled optimization
  // create referenced loop list
  const uint nloops = m_loops.size();
  EventLoop* loops[nloops];
  bool dispatchable[nloops];
  for (size_t i = 0; i < nloops; i++)
    loops[i] = ref (m_loops[i]);
  // prepare
  for (uint i = 0; i < nloops; i++)
    {
      loops[i]->sense_sources_L (seen_primary); // sense m_dispatch_priority
      dispatchable[i] = loops[i]->prepare_sources_Lm (pfds, &timeout_usecs);
      any_dispatchable |= dispatchable[i];
    }
  // allow poll wakeups
  PollFD wakeup = { m_eventfd.inputfd(), PollFD::IN, 0 };
  uint wakeup_idx = pfds.size();
  pfds.push_back (wakeup);
  // poll file descriptors
  int64 timeout_msecs = timeout_usecs / 1000;
  if (timeout_usecs > 0 && timeout_msecs <= 0)
    timeout_msecs = 1;
  if (!may_block || any_dispatchable)
    timeout_msecs = 0;
  int presult;
  locker.unlock();
  const bool thread_entered = rapicorn_thread_entered();
  if (thread_entered)
    rapicorn_thread_leave();
  do
    presult = poll ((struct pollfd*) &pfds[0], pfds.size(), MIN (timeout_msecs, INT_MAX));
  while (presult < 0 && (errno == EAGAIN || errno == EINTR));
  if (thread_entered)
    rapicorn_thread_enter();
  locker.lock();
  if (presult < 0)
    pcritical ("MainLoop: poll() failed");
  else if (pfds[wakeup_idx].revents)
    m_eventfd.flush(); // restart queueing wakeups, possibly triggered by dispatching
  // check
  for (uint i = 0; i < nloops; i++)
    {
      dispatchable[i] |= loops[i]->check_sources_Lm (pfds);
      any_dispatchable |= dispatchable[i];
    }
  // dispatch
  if (may_dispatch && any_dispatchable)
    {
      uint index, i = nloops;
      do        // find next dispatchable loop in round-robin fashion
        index = m_rr_index++ % nloops;
      while (!dispatchable[index] && i--);
      loops[index]->dispatch_source_Lm();
    }
  // cleanup
  locker.unlock();
  for (uint i = 0; i < nloops; i++)
    {
      loops[i]->unpoll_sources_U(); // unlocked
      unref (loops[i]); // unlocked
    }
  locker.lock();
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

// === EventLoop::Source ===
EventLoop::Source::Source () :
  m_main_loop (NULL),
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
  if (m_main_loop)
    m_main_loop->try_remove (source_id());
}

EventLoop::Source::~Source ()
{
  RAPICORN_ASSERT (m_main_loop == NULL);
  if (m_pfds)
    free (m_pfds);
}

/* --- EventLoop::TimedSource --- */
EventLoop::TimedSource::TimedSource (Signals::Trampoline0<void> &vt,
                                     uint initial_interval_msecs,
                                     uint repeat_interval_msecs) :
  m_expiration_usecs (EventLoop::get_current_time_usecs() + 1000ULL * initial_interval_msecs),
  m_interval_msecs (repeat_interval_msecs), m_first_interval (true),
  m_oneshot (true), m_vtrampoline (ref_sink (&vt))
{}

EventLoop::TimedSource::TimedSource (Signals::Trampoline0<bool> &bt,
                                     uint initial_interval_msecs,
                                     uint repeat_interval_msecs) :
  m_expiration_usecs (EventLoop::get_current_time_usecs() + 1000ULL * initial_interval_msecs),
  m_interval_msecs (repeat_interval_msecs), m_first_interval (true),
  m_oneshot (false), m_btrampoline (ref_sink (&bt))
{}

bool
EventLoop::TimedSource::prepare (uint64 current_time_usecs,
                                 int64 *timeout_usecs_p)
{
  if (current_time_usecs >= m_expiration_usecs)
    return true;                                            /* timeout expired */
  if (!m_first_interval)
    {
      uint64 interval = m_interval_msecs * 1000ULL;
      if (current_time_usecs + interval < m_expiration_usecs)
        m_expiration_usecs = current_time_usecs + interval; /* clock warped back in time */
    }
  *timeout_usecs_p = MIN (INT_MAX, m_expiration_usecs - current_time_usecs);
  return 0 == *timeout_usecs_p;
}

bool
EventLoop::TimedSource::check (uint64 current_time_usecs)
{
  return current_time_usecs >= m_expiration_usecs;
}

bool
EventLoop::TimedSource::dispatch()
{
  bool repeat = false;
  m_first_interval = false;
  if (m_oneshot && m_vtrampoline->callable())
    (*m_vtrampoline) ();
  else if (!m_oneshot && m_btrampoline->callable())
    repeat = (*m_btrampoline) ();
  if (repeat)
    m_expiration_usecs = EventLoop::get_current_time_usecs() + 1000ULL * m_interval_msecs;
  return repeat;
}

EventLoop::TimedSource::~TimedSource ()
{
  if (m_oneshot)
    unref (m_vtrampoline);
  else
    unref (m_btrampoline);
}

/* --- EventLoop::PollFDSource --- */
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
EventLoop::PollFDSource::prepare (uint64 current_time_usecs,
                                  int64 *timeout_usecs_p)
{
  m_pfd.revents = 0;
  return m_pfd.fd < 0;
}

bool
EventLoop::PollFDSource::check (uint64 current_time_usecs)
{
  return m_pfd.fd < 0 || m_pfd.revents != 0;
}

bool
EventLoop::PollFDSource::dispatch()
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
