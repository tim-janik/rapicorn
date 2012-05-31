/* Rapicorn
 * Copyright (C) 2006 Tim Janik
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
#include "rapicornthread.hh"
#include <list>
#include <algorithm>
#include <sys/time.h>
#if _POSIX_SPIN_LOCKS >= 200112L
#define USE_POSIX_SPINLOCK
#endif

#define rapicorn_threads_initialized()    ISLIKELY ((void*) ThreadTable.mutex_lock != (void*) ThreadTable.mutex_unlock)

namespace Rapicorn {
extern RapicornThreadTable ThreadTable; /* private, provided by rapicornthreadimpl.cc */

/* --- Thread::ThreadWrapperInternal --- */
struct Thread::ThreadWrapperInternal : public Thread {
  ThreadWrapperInternal (RapicornThread *bthread) :
    Thread (bthread)
  {}
  virtual void
  run ()
  {}
  static Thread*
  thread_from_c (RapicornThread *bthread)
  {
    Thread::ThreadWrapperInternal *ithread = new ThreadWrapperInternal (bthread);
    if (!ithread->bthread)
      {
        /* someone else was faster */
        ithread->ref_sink();
        ithread->unref();
      }
    void *threadxx = ThreadTable.thread_getxx (bthread);
    RAPICORN_ASSERT (threadxx != NULL);
    return reinterpret_cast<Thread*> (threadxx);
  }
  static void
  thread_reset_c (Thread *thread)
  {
    RapicornThread *bthread = thread->bthread;
    RAPICORN_ASSERT (thread->bthread != NULL);
    thread->data_list.clear_like_destructor();
    thread->bthread = NULL;
    ThreadTable.thread_setxx (bthread, NULL);
  }
  static void
  run_static (void *thread_data)
  {
    Thread &self = *reinterpret_cast<Thread*> (thread_data);
    ref_sink (self);
    self.run();
    unref (self);
  }
};

/* --- ThreadWrapperInternal (public version of Thread::ThreadWrapperInternal --- */
struct ThreadDescendant : public Thread {
  typedef ThreadWrapperInternal PublicThreadWrapperInternal;
  ThreadDescendant (const String &name) : Thread (name) {}
};
typedef ThreadDescendant::PublicThreadWrapperInternal ThreadWrapperInternal;

/* --- Thread methods --- */
void
Thread::threadxx_wrap (RapicornThread *cthread)
{
  ThreadWrapperInternal::thread_from_c (cthread);
}

void
Thread::threadxx_delete (void *cxxthread)
{
  Thread *thread = reinterpret_cast<Thread*> (cxxthread);
  ThreadWrapperInternal::thread_reset_c (thread);
}

Thread::Thread (RapicornThread* thread) :
  bthread (NULL),
  last_cpu (-1)
{
  ThreadTable.thread_ref (thread);
  if (ThreadTable.thread_setxx (thread, this))
    {
      bthread = thread;
      ThreadTable.thread_ref_sink (thread);
      RAPICORN_ASSERT (ThreadTable.thread_getxx (thread) == this);
    }
  else
    ; /* invalid object state; this should be reaped by thread_from_c() */
  ThreadTable.thread_unref (thread);
}

int
Thread::online_cpus ()
{
  int cpus = -1;
#ifdef _SC_NPROCESSORS_ONLN
  cpus = sysconf (_SC_NPROCESSORS_ONLN);
#endif
  return cpus;
}

static int
cpu_affinity (int cpu)
{
#ifdef __USE_GNU
  pthread_t thread = pthread_self();
  cpu_set_t cpuset;
  if (cpu >= 0 && cpu < CPU_SETSIZE)
    {
      CPU_ZERO (&cpuset);
      CPU_SET (cpu, &cpuset);
      if (pthread_setaffinity_np (thread, sizeof (cpu_set_t), &cpuset) != 0)
        DEBUG ("pthread_setaffinity_np: %s", strerror());
    }
  if (pthread_getaffinity_np (thread, sizeof (cpu_set_t), &cpuset) != 0)
    DEBUG ("pthread_getaffinity_np: %s", strerror());
  for (int j = 0; j < CPU_SETSIZE; j++)
    if (CPU_ISSET (j, &cpuset))
      return j;
#endif
  return -1;
}

/**
 * @param cpu   CPU# to operate on, use -1 to leave unspecified.
 *
 * Set and get the current CPU affinity. This function may be called before Rapicorn is initialized.
 * @returns CPU affinity of the current thread.
 */
int
Thread::Self::affinity (int cpu)
{
  int real = cpu_affinity (cpu);
  if (cpu >= 0 && real >= 0)
    DEBUG ("Thread::Self: affinitiy: cpu#%u", 1 + real);
  return real;
}

int
Thread::affinity (int cpu)
{
  last_cpu = cpu_affinity (cpu);
  if (cpu >= 0 && last_cpu >= 0)
    DEBUG ("%s: affinitiy: cpu#%u", name().c_str(), 1 + last_cpu);
  return last_cpu;
}

int
Thread::last_affinity() const
{
  return last_cpu;
}

static RapicornThread*
bthread_create_for_thread (const String &name,
                           void         *threadxx)
{
  RapicornThread *bthread = ThreadTable.thread_new (name.c_str());
  bool success = ThreadTable.thread_setxx (bthread, threadxx);
  RAPICORN_ASSERT (success);
  ThreadTable.thread_ref_sink (bthread);
  return bthread;
}

Thread::Thread (const String &_name) :
  bthread (bthread_create_for_thread (_name, this))
{}

Thread::~Thread ()
{
  if (bthread)  /* can be NULL in thread_from_c() */
    {
      ThreadTable.thread_setxx (bthread, NULL);
      ThreadTable.thread_unref (bthread);
    }
}

void
Thread::start ()
{
  bool success = false;
  while (!success)
    {
      success = ThreadTable.thread_start (bthread, Thread::ThreadWrapperInternal::run_static, this);
      if (!success)
        Thread::Self::yield();
    }
}

int
Thread::pid () const
{
  return ThreadTable.thread_pid (bthread);
}

String
Thread::name () const
{
  return ThreadTable.thread_name (bthread);
}

void
Thread::queue_abort ()
{
  ThreadTable.thread_queue_abort (bthread);
}

void
Thread::abort ()
{
  ThreadTable.thread_abort (bthread);
}

bool
Thread::aborted ()
{
  return ThreadTable.thread_get_aborted (bthread);
}

bool
Thread::running ()
{
  return ThreadTable.thread_get_running (bthread);
}

void
Thread::wait_for_exit ()
{
  ThreadTable.thread_wait_for_exit (bthread);
}

Thread&
Thread::self ()
{
  Thread *thread = (Thread*) ThreadTable.thread_selfxx();
  return *thread;
}

String
Thread::Self::name ()
{
  return ThreadTable.thread_name (ThreadTable.thread_self());
}

void
Thread::Self::name (const String &name)
{
  ThreadTable.thread_set_name (name.c_str());
}

/**
 * @param max_useconds  maximum amount of micro seconds to sleep (-1 for infinite time)
 * @param returns       TRUE while the thread should continue execution
 *
 * This is a wrapper for rapicorn_thread_sleep().
 */
bool
Thread::Self::sleep (long max_useconds)
{
  return ThreadTable.thread_sleep (max_useconds);
}

bool
Thread::Self::aborted ()
{
  return ThreadTable.thread_aborted();
}

int
Thread::Self::pid ()
{
  return ThreadTable.thread_pid (ThreadTable.thread_self());
}

OwnedMutex&
Thread::Self::owned_mutex ()
{
  return self().m_omutex;
}

void
Thread::Self::yield ()
{
  sched_yield();
}

void
Thread::Self::exit (void *retval)
{
  ThreadTable.thread_exit (retval);
}

#define M_SPINPTR       ((pthread_spinlock_t*) &this->spinspace)

SpinLock::SpinLock ()
{
#ifdef USE_POSIX_SPINLOCK
  pthread_spin_init (M_SPINPTR, 0);
#else
  spinspace.fallback = new Mutex();
#endif
}

void
SpinLock::lock ()
{
#ifdef USE_POSIX_SPINLOCK
  pthread_spin_lock (M_SPINPTR);
#else
  spinspace.fallback->lock();
#endif
}

void
SpinLock::unlock ()
{
#ifdef USE_POSIX_SPINLOCK
  pthread_spin_unlock (M_SPINPTR);
#else
  spinspace.fallback->unlock();
#endif
}

bool
SpinLock::trylock ()
{
#ifdef USE_POSIX_SPINLOCK
  return pthread_spin_trylock (M_SPINPTR) == 0;
#else
  return spinspace.fallback->trylock();
#endif
}

SpinLock::~SpinLock ()
{
#ifdef USE_POSIX_SPINLOCK
  pthread_spin_destroy (M_SPINPTR);
#else
  delete spinspace.fallback;
#endif
  spinspace.fallback = NULL;
}

static const RapicornMutex zero_mutex = { 0, };

Mutex::Mutex () :
  mutex (zero_mutex)
{
  /* need NULL attribute here, which is the fast mutex in glibc
   * and cannot be chosen through pthread_mutexattr_settype()
   */
  pthread_mutex_init ((pthread_mutex_t*) &mutex, NULL);
}

void
Mutex::lock ()
{
  pthread_mutex_lock ((pthread_mutex_t*) &mutex);
}

void
Mutex::unlock ()
{
  pthread_mutex_unlock ((pthread_mutex_t*) &mutex);
}

bool
Mutex::trylock ()
{
  // TRUE indicates success
  return 0 == pthread_mutex_trylock ((pthread_mutex_t*) &mutex);
}

Mutex::~Mutex ()
{
  pthread_mutex_destroy ((pthread_mutex_t*) &mutex);
}

static const RapicornRecMutex zero_rec_mutex = { { 0, }, };

RecMutex::RecMutex () :
  rmutex (zero_rec_mutex)
{
  RAPICORN_STATIC_ASSERT (offsetof (RapicornRecMutex, mutex) == 0);
  pthread_mutexattr_t attr;
  pthread_mutexattr_init (&attr);
  pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init ((pthread_mutex_t*) &rmutex, &attr);
  pthread_mutexattr_destroy (&attr);
}

void
RecMutex::lock ()
{
  pthread_mutex_lock ((pthread_mutex_t*) &rmutex);
}

void
RecMutex::unlock ()
{
  pthread_mutex_unlock ((pthread_mutex_t*) &rmutex);
}

bool
RecMutex::trylock ()
{
  // TRUE indicates success
  return 0 == pthread_mutex_trylock ((pthread_mutex_t*) &rmutex);
}

RecMutex::~RecMutex ()
{
  pthread_mutex_destroy ((pthread_mutex_t*) &rmutex);
}

static const RapicornCond zero_cond = { 0, };

Cond::Cond () :
  cond (zero_cond)
{
  pthread_cond_init ((pthread_cond_t*) &cond, NULL);
}

void
Cond::signal ()
{
  pthread_cond_signal ((pthread_cond_t*) &cond);
}

void
Cond::broadcast ()
{
  pthread_cond_broadcast ((pthread_cond_t*) &cond);
}

void
Cond::wait (Mutex &m)
{
  pthread_cond_wait ((pthread_cond_t*) &cond, (pthread_mutex_t*) &m.mutex);
}

static bool
common_split_useconds (RapicornInt64   max_useconds,
                       RapicornUInt64 *abs_secs,
                       RapicornUInt64 *abs_usecs)
{
  if (max_useconds < 0)
    return false;
  struct timeval now;
  gettimeofday (&now, NULL);
  RapicornUInt64 secs = max_useconds / 1000000;
  RapicornUInt64 limit_sec = now.tv_sec + secs;
  max_useconds -= secs * 1000000;
  RapicornUInt64 limit_usec = now.tv_usec + max_useconds;
  if (limit_usec >= 1000000)
    {
      limit_usec -= 1000000;
      limit_sec += 1;
    }
  *abs_secs = limit_sec;
  *abs_usecs = limit_usec;
  return true;
}

void
Cond::wait_timed (Mutex &m, int64 max_usecs)
{
  RapicornUInt64 abs_secs, abs_usecs;
  if (common_split_useconds (max_usecs, &abs_secs, &abs_usecs))
    {
      struct timespec abstime;
      abstime.tv_sec = abs_secs;
      abstime.tv_nsec = abs_usecs * 1000;
      pthread_cond_timedwait ((pthread_cond_t*) &cond, (pthread_mutex_t*) &m.mutex, &abstime);
    }
  else
    pthread_cond_wait ((pthread_cond_t*) &cond, (pthread_mutex_t*) &m.mutex);
}

Cond::~Cond ()
{
  pthread_cond_destroy ((pthread_cond_t*) &cond);
}

OwnedMutex::OwnedMutex () :
  m_owner (NULL),
  m_count (0)
{}

OwnedMutex::~OwnedMutex()
{
  RAPICORN_ASSERT (m_owner == NULL);
  RAPICORN_ASSERT (m_count == 0);
}

struct OnceData {
  Mutex                      mutex;
  Cond                       cond;
  std::list<volatile void *> list;
};
static OnceData *static_once_data = NULL;

static inline OnceData&
atomic_once_data ()
{
  OnceData *od = Atomic::value_get (&static_once_data);
  if (LIKELY (od != NULL))
    return *od;
  od = new OnceData;
  if (!Atomic::value_cas (&static_once_data, (OnceData*) NULL, od))
    delete od;
  return *Atomic::value_get (&static_once_data);
}

void
once_list_enter()
{
  OnceData &once_data = atomic_once_data();
  once_data.mutex.lock();
}

bool
once_list_bounce (volatile void *ptr)
{
  OnceData &once_data = atomic_once_data();
  bool ptr_listed = false;
  if (ptr)
    {
      if (find (once_data.list.begin(), once_data.list.end(), ptr) == once_data.list.end())
        {
          ptr_listed = true;
          once_data.list.push_front (ptr);
        }
      else
        do
          once_data.cond.wait (once_data.mutex);
        while (find (once_data.list.begin(), once_data.list.end(), ptr) != once_data.list.end());
    }
  once_data.mutex.unlock();
  return ptr_listed;
}

bool
once_list_leave (volatile void *ptr)
{
  OnceData &once_data = atomic_once_data();
  once_data.mutex.lock();
  std::list<volatile void *>::iterator it = find (once_data.list.begin(), once_data.list.end(), ptr);
  bool found_removed = it != once_data.list.end();
  if (found_removed)
    once_data.list.erase (it);
  once_data.cond.broadcast();
  once_data.mutex.unlock();
  return found_removed;
}

} // Rapicorn
