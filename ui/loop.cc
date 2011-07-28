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

/* --- assert poll constants --- */
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

/* --- assert pollfd struct --- */
RAPICORN_STATIC_ASSERT (sizeof   (PollFD)               == sizeof   (struct pollfd));
RAPICORN_STATIC_ASSERT (offsetof (PollFD, fd)           == offsetof (struct pollfd, fd));
RAPICORN_STATIC_ASSERT (sizeof (((PollFD*) 0)->fd)      == sizeof (((struct pollfd*) 0)->fd));
RAPICORN_STATIC_ASSERT (offsetof (PollFD, events)       == offsetof (struct pollfd, events));
RAPICORN_STATIC_ASSERT (sizeof (((PollFD*) 0)->events)  == sizeof (((struct pollfd*) 0)->events));
RAPICORN_STATIC_ASSERT (offsetof (PollFD, revents)      == offsetof (struct pollfd, revents));
RAPICORN_STATIC_ASSERT (sizeof (((PollFD*) 0)->revents) == sizeof (((struct pollfd*) 0)->revents));

enum {
  UNCHECKED             = 0,
  PREPARED,
  NEEDS_DISPATCH,
  _DUMMY = UINT_MAX
};

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
  m_main_loop (main)
{
  // we cannot *use* m_main_loop yet, because we might be called from within MainLoop::MainLoop(), see SlaveLoop()
}

EventLoop::~EventLoop ()
{
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
  source->m_loop_state = UNCHECKED;
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
  source->m_loop_state = UNCHECKED;
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
  EventLoop (*this) // sets *this as MainLoop on self
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

bool
MainLoop::exitable()
{
  ScopedLock<Mutex> locker (m_mutex);
  const uint generation = m_generation;
  // loop list shouldn't be modified by querying exitable state
  bool found_primary = false;
  for (size_t i = 0; !found_primary && i < m_loops.size(); i++)
    if (m_loops[i]->has_primary_L())
      found_primary = true;
  return_val_if_fail (generation == m_generation, false);
  return !found_primary; // exitable if no primary sources remain
}

void
MainLoop::iterate (bool may_block)
{
  iterate_loops (may_block, true);
}

bool
MainLoop::pending (bool may_block)
{
  return iterate_loops (may_block, false);
}

void
MainLoop::dispatch ()
{
  iterate_loops (false, true);
}

bool
EventLoop::prepare_sources_Lm (int            *max_priority,
                               vector<PollFD> &pfds,
                               int64          *timeout_usecs)
{
  ScopedLock<Mutex> locker (m_main_loop.mutex(), BALANCED);
  bool must_dispatch = false;
  // create source list to operate on
  SourceList sources;
  for (SourceListMap::iterator it = m_sources.begin(); it != m_sources.end(); it++)
    {
      SourceList &slist = (*it).second;
      for (SourceList::iterator lit = slist.begin(); lit != slist.end(); lit++)
        if ((*lit)->m_main_loop == this && /* test undestroyed */
            (!(*lit)->m_dispatching || (*lit)->m_may_recurse))
          {
            (*lit)->m_loop_state = UNCHECKED;
            sources.push_back (ref (*lit));
          }
    }
  // prepare sources, up to NEEDS_DISPATCH priority
  uint64 current_time_usecs = EventLoop::get_current_time_usecs();
  for (SourceList::iterator lit = sources.begin(); lit != sources.end(); lit++)
    if ((*lit)->m_main_loop == this &&
        *max_priority >= (*lit)->m_priority)
      {
        Source &source = **lit;
        int64 timeout = -1;
        locker.unlock();
        const bool need_dispatch = source.prepare (current_time_usecs, &timeout);
        locker.lock();
        if (source.m_main_loop != this)
          continue;
        if (need_dispatch)
          {
            *max_priority = source.m_priority; // starve lower priority sources
            source.m_loop_state = NEEDS_DISPATCH;
            must_dispatch = true;
          }
        else
          source.m_loop_state = PREPARED;
        if (timeout >= 0)
          *timeout_usecs = MIN (*timeout_usecs, timeout);
        uint npfds = source.n_pfds();
        for (uint i = 0; i < npfds; i++)
          {
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
      }
  if (must_dispatch)
    *timeout_usecs = 0;
  // clean up
  locker.unlock();
  for (SourceList::iterator lit = sources.begin(); lit != sources.end(); lit++)
    unref (*lit); // unlocked
  locker.lock();
  return must_dispatch;
}

bool
EventLoop::check_sources_Lm (int                  *max_priority,
                             const vector<PollFD> &pfds)
{
  ScopedLock<Mutex> locker (m_main_loop.mutex(), BALANCED);
  bool must_dispatch = false;
  // create source list to operate on
  SourceList sources;
  for (SourceListMap::iterator it = m_sources.begin(); it != m_sources.end(); it++)
    {
      SourceList &slist = (*it).second;
      for (SourceList::iterator lit = slist.begin(); lit != slist.end(); lit++)
        if ((*lit)->m_main_loop == this && /* test undestroyed */
            ((*lit)->m_loop_state == PREPARED ||
             (*lit)->m_loop_state == NEEDS_DISPATCH))
          sources.push_back (ref (*lit));
    }
  // check polled sources
  uint64 current_time_usecs = EventLoop::get_current_time_usecs();
  for (SourceList::iterator lit = sources.begin(); lit != sources.end(); lit++)
    if ((*lit)->m_main_loop == this &&
        *max_priority >= (*lit)->m_priority)
      {
        Source &source = **lit;
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
        if (source.m_loop_state == PREPARED)
          {
            Source &source = **lit;
            locker.unlock();
            bool need_dispatch = source.check (current_time_usecs);
            locker.lock();
            if (source.m_main_loop == this && need_dispatch)
              {
                *max_priority = source.m_priority; // starve lower priority sources
                source.m_loop_state = NEEDS_DISPATCH;
                must_dispatch = true;
              }
          }
        else if (source.m_loop_state == NEEDS_DISPATCH)
          must_dispatch = true;
      }
  // clean up
  locker.unlock();
  for (SourceList::iterator lit = sources.begin(); lit != sources.end(); lit++)
    unref (*lit); // unlocked
  locker.lock();
  return must_dispatch;
}

void
EventLoop::dispatch_sources_Lm (const int  max_priority)
{
  ScopedLock<Mutex> locker (m_main_loop.mutex(), BALANCED);
  // create source list to operate on
  SourceList sources;
  for (SourceListMap::iterator it = m_sources.begin(); it != m_sources.end(); it++)
    {
      SourceList &slist = (*it).second;
      for (SourceList::iterator lit = slist.begin(); lit != slist.end(); lit++)
        if ((*lit)->m_main_loop == this && /* test undestroyed */
            (*lit)->m_loop_state == NEEDS_DISPATCH)
          sources.push_back (ref (*lit));
    }
  // dispatch sources
  for (SourceList::iterator lit = sources.begin(); lit != sources.end(); lit++)
    if ((*lit)->m_main_loop == this &&
        max_priority >= (*lit)->m_priority)
      {
        Source &source = **lit;
        if (source.m_loop_state == NEEDS_DISPATCH)
          {
            source.m_loop_state = UNCHECKED;
            bool was_dispatching = source.m_was_dispatching;
            source.m_was_dispatching = source.m_dispatching;
            source.m_dispatching = true;
            locker.unlock();
            bool keep_alive = source.dispatch ();
            locker.lock();
            source.m_dispatching = source.m_was_dispatching;
            source.m_was_dispatching = was_dispatching;
            if (source.m_main_loop == this && !keep_alive)
              remove_source_Lm (&source);
          }
      }
  // clean up
  locker.unlock();
  for (SourceList::iterator lit = sources.begin(); lit != sources.end(); lit++)
    unref (*lit); // unlocked
  locker.lock();
}

bool
MainLoop::iterate_loops (bool may_block, bool may_dispatch)
{
  ScopedLock<Mutex> locker (m_mutex);
  int err = m_eventfd.open();
  if (err < 0)
    fatal ("MainLoop: failed to create wakeup pipe: %s", strerror (-err));
  int64 timeout_usecs = INT64_MAX;
  bool seen_must_dispatch = false;
  vector<PollFD> pfds;
  pfds.reserve (7); // profiled optimization
  // create referenced loop list
  std::list<EventLoop*> loops;
  for (size_t i = 0; i < m_loops.size(); i++)
    loops.push_back (ref (m_loops[i]));
  // prepare
  int max_priority = INT_MAX;
  for (std::list<EventLoop*>::iterator lit = loops.begin(); lit != loops.end(); lit++)
    {
      EventLoop &loop = **lit;
      const bool must_dispatch = loop.prepare_sources_Lm (&max_priority, pfds, &timeout_usecs);
      seen_must_dispatch |= must_dispatch;
    }
  // allow poll wakeups
  PollFD wakeup = { m_eventfd.inputfd(), PollFD::IN, 0 };
  uint wakeup_idx = pfds.size();
  pfds.push_back (wakeup);
  // poll file descriptors
  int64 timeout_msecs = timeout_usecs / 1000;
  if (timeout_usecs > 0 && timeout_msecs <= 0)
    timeout_msecs = 1;
  if (!may_block || seen_must_dispatch)
    timeout_msecs = 0;
  int presult;
  do
    {
      const bool thread_entered = rapicorn_thread_entered();
      if (thread_entered)
        rapicorn_thread_leave();
      presult = poll ((struct pollfd*) &pfds[0], pfds.size(), MIN (timeout_msecs, INT_MAX));
      if (thread_entered)
        rapicorn_thread_enter();
    }
  while (presult < 0 && (errno == EAGAIN || errno == EINTR));
  if (presult < 0)
    pcritical ("MainLoop: poll() failed");
  else if (pfds[wakeup_idx].revents)
    m_eventfd.flush(); // restart queueing wakeups, possibly triggered by dispatching
  // check
  for (std::list<EventLoop*>::iterator lit = loops.begin(); lit != loops.end(); lit++)
    {
      EventLoop &loop = **lit;
      const bool must_dispatch = loop.check_sources_Lm (&max_priority, pfds);
      seen_must_dispatch |= must_dispatch;
    }
  // dispatch
  if (may_dispatch && seen_must_dispatch)
    for (std::list<EventLoop*>::iterator lit = loops.begin(); lit != loops.end(); lit++)
      {
        EventLoop &loop = **lit;
        loop.dispatch_sources_Lm (max_priority);
      }
  // cleanup
  locker.unlock();
  for (std::list<EventLoop*>::iterator lit = loops.begin(); lit != loops.end(); lit++)
    unref (*lit); // unlocked
  locker.lock();
  return seen_must_dispatch && !may_dispatch;
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
