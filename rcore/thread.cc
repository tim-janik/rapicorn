// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "thread.hh"
#include <sys/time.h>
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
