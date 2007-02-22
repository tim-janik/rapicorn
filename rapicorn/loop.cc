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
#include <sys/poll.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <glib.h> // FIXME: GTimeVal

namespace Rapicorn {
using namespace std;

/* --- assert poll constants --- */
BIRNET_STATIC_ASSERT (PollFD::IN     == POLLIN);
BIRNET_STATIC_ASSERT (PollFD::PRI    == POLLPRI);
BIRNET_STATIC_ASSERT (PollFD::OUT    == POLLOUT);
BIRNET_STATIC_ASSERT (PollFD::RDNORM == POLLRDNORM);
BIRNET_STATIC_ASSERT (PollFD::RDBAND == POLLRDBAND);
BIRNET_STATIC_ASSERT (PollFD::WRNORM == POLLWRNORM);
BIRNET_STATIC_ASSERT (PollFD::WRBAND == POLLWRBAND);
BIRNET_STATIC_ASSERT (PollFD::ERR    == POLLERR);
BIRNET_STATIC_ASSERT (PollFD::HUP    == POLLHUP);
BIRNET_STATIC_ASSERT (PollFD::NVAL   == POLLNVAL);

/* --- assert pollfd struct --- */
BIRNET_STATIC_ASSERT (sizeof   (PollFD)               == sizeof   (struct pollfd));
BIRNET_STATIC_ASSERT (offsetof (PollFD, fd)           == offsetof (struct pollfd, fd));
BIRNET_STATIC_ASSERT (sizeof (((PollFD*) 0)->fd)      == sizeof (((struct pollfd*) 0)->fd));
BIRNET_STATIC_ASSERT (offsetof (PollFD, events)       == offsetof (struct pollfd, events));
BIRNET_STATIC_ASSERT (sizeof (((PollFD*) 0)->events)  == sizeof (((struct pollfd*) 0)->events));
BIRNET_STATIC_ASSERT (offsetof (PollFD, revents)      == offsetof (struct pollfd, revents));
BIRNET_STATIC_ASSERT (sizeof (((PollFD*) 0)->revents) == sizeof (((struct pollfd*) 0)->revents));


enum {
  NEEDS_DISPATCH        = 0x1,
  IS_DESTROYED          = 0x2,  // FIXME: use (source->m_main_loop == this) instead
  _DUMMY = UINT_MAX
};

/* --- MainLoop --- */
MainLoop::~MainLoop  ()
{}

uint64
MainLoop::get_current_time_usecs ()
{
  GTimeVal current_time;
  g_get_current_time (&current_time);
  return current_time.tv_sec * 1000000ULL + current_time.tv_usec;
}

void
MainLoop::remove (uint   id)
{
  if (!try_remove (id))
    g_warning ("%s: failed to remove loop source: %u", G_STRFUNC, id);
}

bool
MainLoop::pending ()
{
  return iterate (false, false);
}

bool
MainLoop::iteration (bool may_block)
{
  return iterate (may_block, true);
}

/* --- MainLoopThread --- */
class MainLoopThread : public virtual Thread {
  MainLoop *m_loop;
  BIRNET_PRIVATE_CLASS_COPY (MainLoopThread);
public:
  MainLoopThread (MainLoop *mloop) :
    Thread ("MainLoopThread"),
    m_loop (ref_sink (mloop))
  {}
  ~MainLoopThread()
  {
    unref (m_loop);
  }
  virtual void
  run ()
  {
    while (!aborted())
      {
        if (m_loop->pending())
          m_loop->iteration (false);
        else
          m_loop->iteration (true);
      }
  }
};

/* --- RealMainLoop --- */
class RealMainLoop : public virtual MainLoop {
  typedef list<Source*>         SourceList;
  typedef map<int, SourceList>  SourceListMap;
  Mutex          m_mutex;
  uint           m_counter;             // FIXME: need to handle wrapping at some point
  SourceListMap  m_sources;
  vector<PollFD> m_pfds;
  int            m_wakeup_pipe[2];      // [0] = readable; [1] = writable
  Thread        *m_thread;
  bool           inactive;
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
        SourceList slist = (*it).second;
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
        SourceList slist = (*it).second;
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
    bool needs_destroy = !(source->m_loop_flags & IS_DESTROYED);
    source->m_loop_flags |= IS_DESTROYED;
    source->m_main_loop = NULL;
    m_mutex.unlock();
    if (needs_destroy)
      source->destroy();
    unref (source);
    m_mutex.lock();
  }
  virtual bool iterate (bool, bool);
public:
  RealMainLoop () :
    m_counter (1),
    m_thread (NULL),
    inactive (true),
    m_istate (WILL_CHECK)
  {
    m_wakeup_pipe[0] = -1;
    m_wakeup_pipe[1] = -1;
    AutoLocker locker (m_mutex);
  }
  ~RealMainLoop()
  {
    quit();
  }
  void
  wakeup_L (void)
  {
    if (m_wakeup_pipe[1] >= 0)
      {
        char w = 'w';
        int err;
        do
          err = write (m_wakeup_pipe[1], &w, 1);
        while (err < 0 && (errno == EINTR || errno == EAGAIN));
      }
  }
  virtual void
  wakeup (void)
  {
    AutoLocker locker (m_mutex);
    wakeup_L();
  }
  virtual bool
  running (void)
  {
    AutoLocker locker (m_mutex);
    return !inactive;
  }
  virtual uint
  add_source (Source   *source,
              int       priority)
  {
    AutoLocker locker (m_mutex);
    return_val_if_fail (source->m_main_loop == NULL, 0);
    ref_sink (source);
    source->m_priority = priority;
    source->m_main_loop = this;
    source->m_id = m_counter++;
    if (!source->m_id)
      error ("MainLoop::m_counter overflow, please report");
    list<Source*> &slist = m_sources[priority];
    slist.push_back (source);
    inactive = false;
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
    AutoLocker locker (m_mutex);
    Source *source = find_source (id);
    if (source)
      {
        real_remove_Lm (source);
        return true;
      }
    return false;
  }
  virtual bool
  start ()
  {
    AutoLocker locker (m_mutex);
    if (m_thread)
      return false;
    return_val_if_fail (m_wakeup_pipe[0] == -1 && m_wakeup_pipe[1] == -1, false);
    if (pipe (m_wakeup_pipe) < 0)
      return false;
    for (uint i = 0; i <= 1; i++)
      {
        long nflags = fcntl (m_wakeup_pipe[i], F_GETFL, 0);
        nflags |= O_NONBLOCK;
        int err;
        do
          err = fcntl (m_wakeup_pipe[i], F_SETFL, nflags);
        while (err < 0 && (errno == EINTR || errno == EAGAIN));
      }
    if (!m_thread)
      {
        m_thread = new MainLoopThread (this);
        ref_sink (m_thread);
        m_thread->start();
        return true;
      }
    return false;
  }
  virtual void
  quit (void)
  {
    AutoLocker locker (m_mutex);
    Source *source = find_first();
    while (source)
      {
        real_remove_Lm (source);
        source = find_first();
      }
    inactive = true;
    if (m_thread)
      {
        m_thread->queue_abort();
        wakeup_L();
        close (m_wakeup_pipe[1]);
        m_wakeup_pipe[1] = -1;
        if (&Thread::self() != m_thread)
          m_thread->wait_for_exit();
        close (m_wakeup_pipe[0]);
        m_wakeup_pipe[0] = -1;
        unref (m_thread);
        m_thread = NULL;
      }
  }
};

/* --- loop iteration function --- */
bool
RealMainLoop::iterate (bool may_block,
                       bool may_dispatch)
{
  AutoLocker locker (m_mutex);
  inactive = false;
  int64 timeout_usecs = INT64_MAX;
  bool must_dispatch = false;
  vector<PollFD> pfds;
  /* fetch source list to operate on */
  SourceList sources;
  for (SourceListMap::iterator it = m_sources.begin(); it != m_sources.end(); it++)
    {
      SourceList slist = (*it).second;
      for (SourceList::iterator lit = slist.begin(); lit != slist.end(); lit++)
        sources.push_back (ref (*lit));
    }
  /* prepare sources, up to NEEDS_DISPATCH priority */
  int max_priority = INT_MAX;
  uint64 current_time_usecs = MainLoop::get_current_time_usecs();
  for (SourceList::iterator lit = sources.begin(); lit != sources.end(); lit++)
    if (!((*lit)->m_loop_flags & IS_DESTROYED) &&
        max_priority >= (*lit)->m_priority)
      {
        Source &source = **lit;
        int64 timeout = -1;
        locker.unlock();
        bool need_dispatch = source.prepare (current_time_usecs, &timeout);
        locker.relock();
        if (need_dispatch)
          {
            max_priority = source.m_priority;
            source.m_loop_flags |= NEEDS_DISPATCH;
            must_dispatch = true;
          }
        else
          source.m_loop_flags &= ~NEEDS_DISPATCH;
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
  /* allow poll wakeups */
  PollFD wakeup = { m_wakeup_pipe[0], PollFD::IN, 0 };
  uint wakeup_idx = pfds.size();
  pfds.push_back (wakeup);
  /* poll file descriptors */
  int64 timeout_msecs = timeout_usecs / 1000;
  if (timeout_usecs > 0 && timeout_msecs <= 0)
    timeout_msecs = 1;
  if (must_dispatch || !may_block)
    timeout_msecs = 0;
  int presult;
  do
    {
      locker.unlock();
      presult = poll ((struct pollfd*) &pfds[0], pfds.size(), timeout_msecs > INT_MAX ? -1 : timeout_msecs);
      locker.relock();
    }
  while (presult < 0 && (errno == EAGAIN || errno == EINTR));
  if (presult < 0)
    warning ("failure during main loop poll: %s", strerror (errno));
  else if (pfds[wakeup_idx].revents)
    {
      /* flush wakeup pipe */
      char buffer[512]; // 512 <= posix pipe atomic read/write size
      read (m_wakeup_pipe[0], buffer, sizeof (buffer));
    }
  /* check polled sources */
  current_time_usecs = MainLoop::get_current_time_usecs();
  for (SourceList::iterator lit = sources.begin(); lit != sources.end(); lit++)
    if (!((*lit)->m_loop_flags & IS_DESTROYED) &&
        max_priority >= (*lit)->m_priority)
      {
        Source &source = **lit;
        uint npfds = source.n_pfds();
        for (uint i = 0; i < npfds; i++)
          {
            uint idx = source.m_pfds[i].idx;
            if (idx < pfds.size())
              source.m_pfds[i].pfd->revents = pfds[idx].revents;
          }
        if (!(source.m_loop_flags & NEEDS_DISPATCH))
          {
            Source &source = **lit;
            locker.unlock();
            bool need_dispatch = source.check (current_time_usecs);
            locker.relock();
            if (need_dispatch)
              {
                source.m_loop_flags |= NEEDS_DISPATCH;
                must_dispatch = true;
              }
          }
      }
  /* dispatch sources */
  if (may_dispatch && must_dispatch)
    for (SourceList::iterator lit = sources.begin(); lit != sources.end(); lit++)
      if (!((*lit)->m_loop_flags & IS_DESTROYED) &&
          max_priority >= (*lit)->m_priority)
        {
          Source &source = **lit;
          if (source.m_loop_flags & NEEDS_DISPATCH)
            {
              source.m_loop_flags &= ~NEEDS_DISPATCH;
              locker.unlock();
              bool keep_alive = source.dispatch ();
              locker.relock();
              if (!keep_alive && source.m_main_loop == this)
                real_remove_Lm (&source);
            }
        }
  /* clean up */
  locker.unlock();                              /* unlocked */
  for (SourceList::iterator lit = sources.begin(); lit != sources.end(); lit++)
    unref (*lit);                               /* unlocked */
  return must_dispatch && !may_dispatch;        /* unlocked */
}

/* --- MainLoop::Source --- */
MainLoop*
MainLoop::create ()
{
  return new RealMainLoop();
}

MainLoop::Source::Source () :
  m_main_loop (NULL),
  m_id (0),
  m_loop_flags (0),
  m_pfds (NULL)
{}

uint
MainLoop::Source::n_pfds ()
{
  uint i = 0;
  if (m_pfds)
    while (m_pfds[i].pfd)
      i++;
  return i;
}

void
MainLoop::Source::add_poll (PollFD *const pfd)
{
  const uint idx = n_pfds();
  uint npfds = idx + 1;
  m_pfds = (typeof (m_pfds)) realloc (m_pfds, sizeof (m_pfds[0]) * (npfds + 1));
  if (!m_pfds)
    error ("MainLoopSource: out of memory");
  m_pfds[npfds].idx = UINT_MAX;
  m_pfds[npfds].pfd = NULL;
  m_pfds[idx].idx = UINT_MAX;
  m_pfds[idx].pfd = pfd;
}

void
MainLoop::Source::remove_poll (PollFD *const pfd)
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
    warning ("MainLoopSource: unremovable PollFD: %p (fd=%d)", pfd, pfd->fd);
}

void
MainLoop::Source::destroy ()
{}

MainLoop::Source::~Source ()
{
  BIRNET_ASSERT (m_main_loop == NULL);
  if (m_pfds)
    free (m_pfds);
}

/* --- MainLoop::TimedSource --- */
MainLoop::TimedSource::TimedSource (Signals::Trampoline0<void> &vt,
                                    uint initial_interval_msecs,
                                    uint repeat_interval_msecs) :
  m_expiration_usecs (MainLoop::get_current_time_usecs() + 1000ULL * initial_interval_msecs),
  m_interval_msecs (repeat_interval_msecs), m_first_interval (true),
  m_oneshot (true), m_vtrampoline (ref_sink (&vt))
{}

MainLoop::TimedSource::TimedSource (Signals::Trampoline0<bool> &bt,
                                    uint initial_interval_msecs,
                                    uint repeat_interval_msecs) :
  m_expiration_usecs (MainLoop::get_current_time_usecs() + 1000ULL * initial_interval_msecs),
  m_interval_msecs (repeat_interval_msecs), m_first_interval (true),
  m_oneshot (false), m_btrampoline (ref_sink (&bt))
{}

bool
MainLoop::TimedSource::prepare (uint64 current_time_usecs,
                                int64   *timeout_usecs_p)
{
  if (current_time_usecs >= m_expiration_usecs)
    return true;                                            /* timeout expired */
  if (!m_first_interval)
    {
      uint64 interval = m_interval_msecs * 1000ULL;
      if (current_time_usecs + interval < m_expiration_usecs)
        m_expiration_usecs = current_time_usecs + interval; /* clock warped back in time */
    }
  *timeout_usecs_p = MIN (G_MAXINT, m_expiration_usecs - current_time_usecs);
  return 0 == *timeout_usecs_p;
}

bool
MainLoop::TimedSource::check (uint64 current_time_usecs)
{
  return current_time_usecs >= m_expiration_usecs;
}

bool
MainLoop::TimedSource::dispatch()
{
  bool repeat = false;
  m_first_interval = false;
  if (m_oneshot && m_vtrampoline->callable)
    (*m_vtrampoline) ();
  else if (!m_oneshot && m_btrampoline->callable)
    repeat = (*m_btrampoline) ();
  if (repeat)
    m_expiration_usecs = MainLoop::get_current_time_usecs() + 1000ULL * m_interval_msecs;
  return repeat;
}

MainLoop::TimedSource::~TimedSource ()
{
  if (m_oneshot)
    unref (m_vtrampoline);
  else
    unref (m_btrampoline);
}

/* --- MainLoop::PollFDSource --- */
MainLoop::PollFDSource::PollFDSource (Signals::Trampoline1<bool,PollFD&> &bt,
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

MainLoop::PollFDSource::PollFDSource (Signals::Trampoline1<void,PollFD&> &vt,
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
MainLoop::PollFDSource::construct (const String &mode)
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
MainLoop::PollFDSource::prepare (uint64 current_time_usecs,
                               int64 *timeout_usecs_p)
{
  m_pfd.revents = 0;
  return m_pfd.fd < 0;
}

bool
MainLoop::PollFDSource::check (uint64 current_time_usecs)
{
  return m_pfd.fd < 0 || m_pfd.revents != 0;
}

bool
MainLoop::PollFDSource::dispatch()
{
  bool keep_alive = false;
  if (m_pfd.fd >= 0 && (m_pfd.revents & PollFD::NVAL))
    ; // close down
  else if (m_pfd.fd >= 0 && !m_ignore_errors && (m_pfd.revents & PollFD::ERR))
    ; // close down
  else if (m_pfd.fd >= 0 && !m_ignore_hangup && (m_pfd.revents & PollFD::HUP))
    ; // close down
  else if (m_oneshot && m_vtrampoline->callable)
    (*m_vtrampoline) (m_pfd);
  else if (!m_oneshot && m_btrampoline->callable)
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
MainLoop::PollFDSource::destroy()
{
  /* close down */
  if (!m_never_close && m_pfd.fd >= 0)
    close (m_pfd.fd);
  m_pfd.fd = -1;
}

MainLoop::PollFDSource::~PollFDSource ()
{
  if (m_oneshot)
    unref (m_vtrampoline);
  else
    unref (m_btrampoline);
}

} // Rapicorn
