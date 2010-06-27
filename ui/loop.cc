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
#include "loop.hh"
#include "commands.hh"
#include <sys/poll.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <algorithm>
#include <cstring>

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

/* --- EventLoop --- */
EventLoop::EventLoop ()
{}

EventLoop::~EventLoop ()
{}

uint64
EventLoop::get_current_time_usecs ()
{
  struct timeval tv;
  gettimeofday (&tv, NULL);
  uint64 usecs = tv.tv_sec; // promote to 64 bit
  return usecs * 1000000 + tv.tv_usec;
}

void
EventLoop::remove (uint   id)
{
  if (!try_remove (id))
    warning ("%s: failed to remove loop source: %u", RAPICORN_SIMPLE_FUNCTION, id);
}

class EventLoopImpl;
static vector<EventLoopImpl*> rapicorn_main_loops;
static Plic::EventFd          rapicorn_eventfd;

/* --- EventLoopImpl --- */
class EventLoopImpl : public virtual EventLoop {
  typedef std::list<Source*>        SourceList;
  typedef std::map<int, SourceList> SourceListMap;
  Mutex          m_mutex;
public:                                 // EventLoop::iterate_loops
  int            m_max_priority;        // EventLoop::iterate_loops
  bool           m_must_dispatch;       // EventLoop::iterate_loops
private:
  uint           m_counter;             // FIXME: need to handle wrapping at some point
  SourceListMap  m_sources;
  vector<PollFD> m_pfds;
  enum {
    WILL_CHECK,
    WILL_DISPATCH
  };
  uint8          m_istate;
  inline Source*
  find_first()
  {
    for (SourceListMap::iterator it = m_sources.begin(); it != m_sources.end(); it++)
      {
        SourceList &slist = (*it).second;
        for (SourceList::iterator lit = slist.begin(); lit != slist.end(); lit++)
          return *lit;
      }
    return NULL;
  }
  inline Source*
  find_source (uint id)
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
  void
  real_remove_Lm (Source *source)
  {
    assert (source->m_main_loop == this);
    SourceList &slist = m_sources[source->m_priority];
    slist.remove (source);
    if (slist.empty())
      m_sources.erase (m_sources.find (source->m_priority));
    bool needs_destroy = source->m_main_loop == this;
    source->m_main_loop = NULL;
    source->m_loop_state = UNCHECKED;
    m_mutex.unlock();
    if (needs_destroy)
      source->destroy();
    unref (source);
    m_mutex.lock();
  }
  void
  remove_loop_M ()
  {
    vector<EventLoopImpl*>::iterator it = find (rapicorn_main_loops.begin(), rapicorn_main_loops.end(), this);
    if (it != rapicorn_main_loops.end())
      rapicorn_main_loops.erase (it);
  }
public:
  EventLoopImpl () :
    m_max_priority (INT_MAX),
    m_must_dispatch (0),
    m_counter (1),
    m_istate (WILL_CHECK)
  {
    ScopedLock<Mutex> locker (m_mutex);
    return_if_fail (find (rapicorn_main_loops.begin(), rapicorn_main_loops.end(), this) == rapicorn_main_loops.end());
    assert (rapicorn_thread_entered());
    rapicorn_main_loops.push_back (this);
  }
  ~EventLoopImpl()
  {
    kill_sources();
    ScopedLock<Mutex> locker (m_mutex);
    remove_loop_M();
  }
  virtual bool prepare_sources  (int                  &max_priority,
                                 vector<PollFD>       &pfds,
                                 int64                &timeout_usecs);
  virtual bool check_sources    (int                  &max_priority,
                                 const vector<PollFD> &pfds);
  virtual void dispatch_sources (const int             max_priority);
  void
  wakeup_L (void)
  {
    rapicorn_eventfd.wakeup();
  }
  virtual void
  wakeup (void)
  {
    ScopedLock<Mutex> locker (m_mutex);
    wakeup_L();
  }
  virtual uint
  add_source (Source   *source,
              int       priority)
  {
    ScopedLock<Mutex> locker (m_mutex);
    return_val_if_fail (source->m_main_loop == NULL, 0);
    ref_sink (source);
    source->m_main_loop = this;
    source->m_id = m_counter++;
    if (!source->m_id) // FIXME: need simple counter/id allocation scheme
      error ("EventLoop::m_counter overflow, please report");
    source->m_loop_state = UNCHECKED;
    source->m_priority = priority;
    SourceList &slist = m_sources[priority];
    slist.push_back (source);
    wakeup_L();
    return source->m_id;
  }
  void
  change_priority (Source   *source,
                   int       priority)
  {
    // ensure that source belongs to this
    // reset all source->pfds[].idx = UINT_MAX
    // unlink
    // poke priority
    // relink
  }
  virtual bool
  try_remove (uint id)
  {
    ScopedLock<Mutex> locker (m_mutex);
    Source *source = find_source (id);
    if (source)
      {
        real_remove_Lm (source);
        return true;
      }
    return false;
  }
  virtual void
  kill_sources (void)
  {
    ScopedLock<Mutex> locker (m_mutex);
    bool keepref = !finalizing();
    if (keepref)
      ref (this);
    Source *source = find_first();
    while (source)
      {
        real_remove_Lm (source);
        source = find_first();
      }
    wakeup_L();
    if (keepref)
      unref (this);
  }
  virtual bool
  has_primary (void)
  {
    ScopedLock<Mutex> locker (m_mutex);
    for (SourceListMap::iterator it = m_sources.begin(); it != m_sources.end(); it++)
      {
        SourceList &slist = (*it).second;
        for (SourceList::iterator lit = slist.begin(); lit != slist.end(); lit++)
          if ((*lit)->primary())
            return true;
      }
    return false;
  }
};

/* --- EventLoopImpl iteration --- */
bool
EventLoopImpl::prepare_sources (int            &max_priority,
                                vector<PollFD> &pfds,
                                int64          &timeout_usecs)
{
  ScopedLock<Mutex> locker (m_mutex);
  bool must_dispatch = false;
  /* create source list to operate on */
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
  /* prepare sources, up to NEEDS_DISPATCH priority */
  uint64 current_time_usecs = EventLoop::get_current_time_usecs();
  for (SourceList::iterator lit = sources.begin(); lit != sources.end(); lit++)
    if ((*lit)->m_main_loop == this &&
        max_priority >= (*lit)->m_priority)
      {
        Source &source = **lit;
        int64 timeout = -1;
        locker.unlock();
        bool need_dispatch = source.prepare (current_time_usecs, &timeout);
        locker.lock();
        if (source.m_main_loop != this)
          continue;
        if (need_dispatch)
          {
            max_priority = source.m_priority; // starve lower priority sources
            source.m_loop_state = NEEDS_DISPATCH;
            must_dispatch = true;
          }
        else
          source.m_loop_state = PREPARED;
        if (timeout >= 0)
          timeout_usecs = MIN (timeout_usecs, timeout);
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
    timeout_usecs = 0;
  /* clean up */
  locker.unlock();                              /* unlocked */
  for (SourceList::iterator lit = sources.begin(); lit != sources.end(); lit++)
    unref (*lit);                               /* unlocked */
  return must_dispatch;
}

bool
EventLoopImpl::check_sources (int                  &max_priority,
                              const vector<PollFD> &pfds)
{
  ScopedLock<Mutex> locker (m_mutex);
  bool must_dispatch = false;
  /* create source list to operate on */
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
  /* check polled sources */
  uint64 current_time_usecs = EventLoop::get_current_time_usecs();
  for (SourceList::iterator lit = sources.begin(); lit != sources.end(); lit++)
    if ((*lit)->m_main_loop == this &&
        max_priority >= (*lit)->m_priority)
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
                max_priority = source.m_priority; // starve lower priority sources
                source.m_loop_state = NEEDS_DISPATCH;
                must_dispatch = true;
              }
          }
        else if (source.m_loop_state == NEEDS_DISPATCH)
          must_dispatch = true;
      }
  /* clean up */
  locker.unlock();                              /* unlocked */
  for (SourceList::iterator lit = sources.begin(); lit != sources.end(); lit++)
    unref (*lit);                               /* unlocked */
  return must_dispatch;
}

void
EventLoopImpl::dispatch_sources (const int max_priority)
{
  ScopedLock<Mutex> locker (m_mutex);
  /* create source list to operate on */
  SourceList sources;
  for (SourceListMap::iterator it = m_sources.begin(); it != m_sources.end(); it++)
    {
      SourceList &slist = (*it).second;
      for (SourceList::iterator lit = slist.begin(); lit != slist.end(); lit++)
        if ((*lit)->m_main_loop == this && /* test undestroyed */
            (*lit)->m_loop_state == NEEDS_DISPATCH)
          sources.push_back (ref (*lit));
    }
  /* dispatch sources */
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
              real_remove_Lm (&source);
          }
      }
  /* clean up */
  locker.unlock();                              /* unlocked */
  for (SourceList::iterator lit = sources.begin(); lit != sources.end(); lit++)
    unref (*lit);                               /* unlocked */
}

/* --- EventLoop running --- */
bool
EventLoop::loops_exitable ()
{
  assert (rapicorn_thread_entered());
  /* loop list shouldn't be modified by querying exitable state */
  for (uint i = 0; i < rapicorn_main_loops.size(); i++)
    if (rapicorn_main_loops[i]->has_primary())
      return false;
  return true;
}

void
EventLoop::kill_loops()
{
  assert (rapicorn_thread_entered());
  /* create referenced loop list */
  std::list<EventLoopImpl*> loops;
  for (uint i = 0; i < rapicorn_main_loops.size(); i++)
    loops.push_back (ref (rapicorn_main_loops[i]));
  /* kill loops */
  for (std::list<EventLoopImpl*>::iterator lit = loops.begin(); lit != loops.end(); lit++)
    {
      EventLoopImpl &loop = **lit;
      loop.kill_sources();
    }
  /* cleanup */
  for (std::list<EventLoopImpl*>::iterator lit = loops.begin(); lit != loops.end(); lit++)
    unref (*lit);
}

bool
EventLoop::iterate_loops (bool may_block,
                          bool may_dispatch)
{
  assert (rapicorn_thread_entered());   // acquire global lock
  int err = rapicorn_eventfd.open();
  if (err < 0)
    error ("EventLoop: failed to create wakeup pipe: %s", strerror (-err));
  vector<PollFD> pfds;
  pfds.reserve (7);
  int64 timeout_usecs = INT64_MAX;
  bool seen_must_dispatch = false;
  /* create referenced loop list */
  std::list<EventLoopImpl*> loops;
  for (uint i = 0; i < rapicorn_main_loops.size(); i++)
    loops.push_back (ref (rapicorn_main_loops[i]));
  /* prepare */
  for (std::list<EventLoopImpl*>::iterator lit = loops.begin(); lit != loops.end(); lit++)
    {
      EventLoopImpl &loop = **lit;
      ref (loop);
      loop.m_max_priority = INT_MAX;
      loop.m_must_dispatch = loop.prepare_sources (loop.m_max_priority, pfds, timeout_usecs);
      seen_must_dispatch |= loop.m_must_dispatch;
      unref (loop);
    }
  /* allow poll wakeups */
  PollFD wakeup = { rapicorn_eventfd.inputfd(), PollFD::IN, 0 };
  uint wakeup_idx = pfds.size();
  pfds.push_back (wakeup);
  /* poll file descriptors */
  int64 timeout_msecs = timeout_usecs / 1000;
  if (timeout_usecs > 0 && timeout_msecs <= 0)
    timeout_msecs = 1;
  if (!may_block || seen_must_dispatch)
    timeout_msecs = 0;
  int presult;
  do
    {
      rapicorn_thread_leave();          // release global lock
      presult = poll ((struct pollfd*) &pfds[0], pfds.size(), MIN (timeout_msecs, INT_MAX));
      rapicorn_thread_enter();          // re-acquire global lock
    }
  while (presult < 0 && (errno == EAGAIN || errno == EINTR));
  if (presult < 0)
    warning ("failure during main loop poll: %s", strerror (errno));
  else if (pfds[wakeup_idx].revents)
    {
      /* discard pending wakeups */
      rapicorn_eventfd.flush();
    }
  /* check */
  for (std::list<EventLoopImpl*>::iterator lit = loops.begin(); lit != loops.end(); lit++)
    {
      EventLoopImpl &loop = **lit;
      ref (loop);
      loop.m_must_dispatch |= loop.check_sources (loop.m_max_priority, pfds);
      seen_must_dispatch |= loop.m_must_dispatch;
      unref (loop);
    }
  /* dispatch */
  if (may_dispatch && seen_must_dispatch)
    for (std::list<EventLoopImpl*>::iterator lit = loops.begin(); lit != loops.end(); lit++)
      {
        EventLoopImpl &loop = **lit;
        ref (loop);
        loop.dispatch_sources (loop.m_max_priority);
        unref (loop);
      }
  /* cleanup */
  for (std::list<EventLoopImpl*>::iterator lit = loops.begin(); lit != loops.end(); lit++)
    unref (*lit);
  return seen_must_dispatch && !may_dispatch;
}

/* --- EventLoop::Source --- */
EventLoop*
EventLoop::create ()
{
  return new EventLoopImpl();
}

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
    error ("EventLoopSource: out of memory");
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
    warning ("EventLoopSource: unremovable PollFD: %p (fd=%d)", pfd, pfd->fd);
}

void
EventLoop::Source::destroy ()
{}

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
