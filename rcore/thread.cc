// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "thread.hh"
#include "strings.hh"
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <algorithm>
#include <sys/syscall.h>        // SYS_gettid
#include <list>

#define TDEBUG(...)     RAPICORN_KEY_DEBUG ("Threading", __VA_ARGS__)

#define fixalloc_aligned(sz, align)     ({ void *__m_ = NULL; if (posix_memalign (&__m_, (align), (sz)) != 0) __m_ = NULL; __m_; })

namespace Rapicorn {

static Mutex global_rwlock_real_init_mutex;

void
RWLock::real_init ()
{
  global_rwlock_real_init_mutex.lock();
  if (!initialized_)
    {
      pthread_rwlock_init (&rwlock_, NULL);
      initialized_ = true;
    }
  global_rwlock_real_init_mutex.unlock();
}

/// Debugging hook, returns if the Mutex is currently locked, might not work with all threading implementations.
bool
Mutex::debug_locked()
{
  const pthread_mutex_t unlocked_mutex1 = PTHREAD_MUTEX_INITIALIZER;
  const pthread_mutex_t unlocked_mutex2 = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
  return !(memcmp (&mutex_, &unlocked_mutex1, sizeof (mutex_)) == 0 ||
           memcmp (&mutex_, &unlocked_mutex2, sizeof (mutex_)) == 0);
}

static std::atomic<ThreadInfo*> list_head;
static size_t volatile          thread_counter = 0;
ThreadInfo __thread*            ThreadInfo::self_cached = NULL;
static Mutex                    thread_info_mutex;

ThreadInfo::~ThreadInfo ()
{
  assert ("~ThreadInfo must not be reached" == 0);
}

ThreadInfo::ThreadInfo () :
  hp { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  next (NULL), pth_thread_id (pthread_self())
{}

static pthread_key_t
thread_data_key (void (*destroy_specific) (void*))
{
  static pthread_key_t tdkey; // might be 0 after key_create
  static bool tdkey_initialized = false;
  if (UNLIKELY (!tdkey_initialized))
    {
      thread_info_mutex.lock();
      if (!tdkey_initialized)
        {
          if (pthread_key_create (&tdkey, destroy_specific) == 0)
            tdkey_initialized = true;
          else
            {
              perror ("pthread_key_create: failed to create ThreadInfo key");
              _exit (-127);
            }
        }
      thread_info_mutex.unlock();
      assert (tdkey_initialized);
    }
  return tdkey;
}

void
ThreadInfo::setup_specific()
{
  const pthread_t pttid = pthread_self();
  assert (pth_thread_id == pttid);
  pthread_setspecific (thread_data_key (&destroy_specific), NULL);
  assert (pth_thread_id == pttid); // reset_specific must not have been triggered here
  pthread_setspecific (thread_data_key (&destroy_specific), this);
  assert (pth_thread_id == pttid); // reset_specific must not have been triggered here
}

ThreadInfo*
ThreadInfo::create ()
{
  const pthread_t pttid = pthread_self();
  bool needs_listing = false;
  // find existing ThreadInfo
  ThreadInfo *tdata = static_cast<ThreadInfo*> (pthread_getspecific (thread_data_key (&destroy_specific)));
  if (tdata)
    {
      assert (tdata->pth_thread_id == pttid);
      return tdata;
    }
  // reuse existing ThreadInfo
  for (tdata = list_head; tdata; tdata = tdata->next)
    {
      pthread_t unset = 0;
      if (tdata->pth_thread_id.compare_exchange_strong (unset, pttid))
        break;
    }
  // allocate new ThreadInfo
  if (!tdata)
    {
      tdata = (ThreadInfo*) fixalloc_aligned (sizeof (ThreadInfo), RAPICORN_CACHE_LINE_ALIGNMENT);
      assert (tdata);
      assert (0 == (size_t (&tdata->hp[0]) & 0x3f)); // verify 64byte cache line alignment
      new (tdata) ThreadInfo();
      __sync_add_and_fetch (&thread_counter, +1);
      needs_listing = true;
    }
  // setup ThreadInfo with setspecific
  tdata->setup_specific();
  // enlist, now that setspecific knows about this
  if (needs_listing)
    {
      assert (tdata->next == NULL);
      ThreadInfo *next;
      do
        {
          next = list_head;
          tdata->next = next;
        }
      while (!list_head.compare_exchange_weak (next, tdata));
    }
  return tdata;
}

void
ThreadInfo::reset_specific ()
{
  data_list_.clear_like_destructor();
  for (size_t i = 0; i < ARRAY_SIZE (hp); i++)
    hp[i] = NULL;
  self_cached = NULL;
  pthread_t pttid = pthread_self();
  while (!pth_thread_id.compare_exchange_strong (pttid, 0))
    assert (pttid == pthread_self());
  data_list_.clear_like_destructor(); // should be empty
  // TESTED: printout ("resetted: %x (%p)\n", pttid, this);
}

void
ThreadInfo::destroy_specific (void *vdata)
{
  assert (vdata);
  const pthread_t pttid = pthread_self();
  ThreadInfo *tdata = NULL;
  for (tdata = list_head; tdata; tdata = tdata->next)
    if (tdata->pth_thread_id == pttid)
      break;
  assert (tdata && tdata == vdata);
  tdata->reset_specific();
  // if (rand() & 4) assert (NULL != &ThreadInfo::self());
}

std::vector<void*>
ThreadInfo::collect_hazards()
{
  std::vector<void*> ptrs;
  for (ThreadInfo *tdata = list_head; tdata; tdata = tdata->next)
    for (size_t i = 0; i < ARRAY_SIZE (tdata->hp); i++)
      {
        void *const cptr = tdata->hp[i];
        if (UNLIKELY (cptr != NULL))
          ptrs.push_back (cptr);
      }
  stable_sort (ptrs.begin(), ptrs.end());
  auto it = unique (ptrs.begin(), ptrs.end());
  ptrs.resize (it - ptrs.begin());
  return ptrs;
}

String
ThreadInfo::ident ()
{
  return string_format ("thread(%d/%d)", ThisThread::thread_pid(), ThisThread::process_pid());
}

String
ThreadInfo::name ()
{
  ScopedLock<Mutex> locker (thread_info_mutex);
  return name_.size() ? name_ : ident();
}

void
ThreadInfo::name (const String &newname)
{
  ScopedLock<Mutex> locker (thread_info_mutex);
  name_ = newname;
}

struct timespec
Cond::abstime (int64 usecs)
{
  struct timespec ts;
  if (usecs >= 0)
    {
      struct timeval now;
      gettimeofday (&now, NULL);
      const int64 secs = usecs / 1000000;
      int64 abs_sec = now.tv_sec + secs;
      usecs -= secs * 1000000;
      int64 abs_usec = now.tv_usec + usecs;
      if (abs_usec >= 1000000) // handle overflow from addition
        {
          abs_usec -= 1000000;
          abs_sec += 1;
        }
      ts.tv_sec = abs_sec;
      ts.tv_nsec = 1000 * abs_usec;
    }
  else
    {
      ts.tv_sec = ~time_t (0);
      ts.tv_nsec = 0;
    }
  return ts;
}

namespace ThisThread {

String
name()
{
  return ThreadInfo::self().name();
}

/** This function may be called before Rapicorn is initialized. */
int
online_cpus ()
{
  static int cpus = 0;
  if (!cpus)
    cpus = sysconf (_SC_NPROCESSORS_ONLN);
  return cpus;
}

/** This function may be called before Rapicorn is initialized. */
int
affinity ()
{
  pthread_t thread = pthread_self();
  cpu_set_t cpuset;
  if (pthread_getaffinity_np (thread, sizeof (cpu_set_t), &cpuset) == 0)
    errno = 0;
  else
    CPU_ZERO (&cpuset);
  int cpu = -1;
  for (uint j = 0; j < CPU_SETSIZE; j++)
    if (CPU_ISSET (j, &cpuset))
      {
        cpu = j;
        break;
      }
  TDEBUG ("%s: pthread_getaffinity_np: %s", name().c_str(),
          errno ? strerror() : string_format ("%d", cpu).c_str());
  return cpu;
}

/** This function may be called before Rapicorn is initialized. */
void
affinity (int cpu)
{
  pthread_t thread = pthread_self();
  cpu_set_t cpuset;
  if (cpu >= 0 && cpu < CPU_SETSIZE)
    {
      CPU_ZERO (&cpuset);
      CPU_SET (cpu, &cpuset);
      if (pthread_setaffinity_np (thread, sizeof (cpu_set_t), &cpuset) == 0)
        errno = 0;
      TDEBUG ("%s: pthread_setaffinity_np: %s", name().c_str(),
              errno ? strerror() : string_format ("%d", cpu).c_str());
    }
}

int
thread_pid ()
{
  int tid = -1;
#ifdef  __linux__       // SYS_gettid is present on linux >= 2.4.20
  tid = syscall (SYS_gettid);
#endif
  if (tid < 0)
    tid = process_pid();
  return tid;
}

int
process_pid ()
{
  return getpid();
}

} // ThisThread

} // Rapicorn
