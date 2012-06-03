// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "thread.hh"
#include <sys/time.h>
#include <sys/types.h>
#include <algorithm>
#include <list>

namespace Rapicorn {

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
  if (pthread_getaffinity_np (thread, sizeof (cpu_set_t), &cpuset) != 0)
    {
      DEBUG ("pthread_getaffinity_np: %s", strerror());
      CPU_ZERO (&cpuset);
    }
  for (uint j = 0; j < CPU_SETSIZE; j++)
    if (CPU_ISSET (j, &cpuset))
      return j;
  return -1;
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
      if (pthread_setaffinity_np (thread, sizeof (cpu_set_t), &cpuset) != 0)
        DEBUG ("pthread_setaffinity_np: %s", strerror());
    }
}

int
thread_pid ()
{
  int tid = -1;
#if     defined (__linux__) && defined (__NR_gettid)    /* present on linux >= 2.4.20 */
  tid = syscall (__NR_gettid);
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

namespace Lib {

struct OnceData {
  Mutex                     mutex;
  Cond                      cond;
  std::list<void volatile*> list;
};

} // Lib

static Atomic<Lib::OnceData*> static_once_data = NULL;

namespace Lib {

static inline OnceData&
atomic_once_data ()
{
  OnceData *od = static_once_data.load();
  if (LIKELY (od != NULL))
    return *od;
  od = new OnceData;
  if (!static_once_data.cas (NULL, od))
    delete od;
  return *static_once_data.load();
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

} // Lib
} // Rapicorn
