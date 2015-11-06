// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_THREADLIB_HH__
#define __RAPICORN_THREADLIB_HH__

#include <condition_variable>
#include <rcore/cpuasm.hh>

namespace Rapicorn {

namespace Lib { // Namespace for implementation internals

// == Once Scope ==
void once_list_enter  ();
bool once_list_bounce (volatile void *ptr);
bool once_list_leave  (volatile void *ptr);

class OnceScope {
  /*ctor*/       OnceScope (const OnceScope&) = delete;
  OnceScope&     operator= (const OnceScope&) = delete;
  volatile char *volatile flagp;
  bool           entered_once;
public:
  OnceScope (volatile char *volatile p) : flagp (p), entered_once (false) {}
  inline bool
  operator() ()
  {
    if (RAPICORN_LIKELY (*flagp != 0))
      return false;
    if (entered_once > 0)       // second or later invocation from for()
      {
        const bool is_first_initialization = __sync_bool_compare_and_swap (flagp, 0, 1);
        const bool found_and_removed = once_list_leave (flagp);
        if (!is_first_initialization || !found_and_removed)
          printerr ("__once: %s: assertion failed during leave: %d %d", __func__, is_first_initialization, found_and_removed);
      }
    entered_once = 1;           // mark first invocation
    once_list_enter();
    const bool initialized = atomic_load (flagp) != 0;
    const bool needs_init = once_list_bounce (initialized ? NULL : flagp);
    return needs_init;
  }
};

#define RAPICORN_ASECTION(bytes)    __attribute__ ((section (".data.aligned" #bytes), aligned (bytes)))
#define RAPICORN_DO_ONCE_COUNTER    ({ static volatile char RAPICORN_ASECTION (1) __rapicorn_oncebyte_ = 0; &__rapicorn_oncebyte_; })
#define RAPICORN_DO_ONCE   for (Rapicorn::Lib::OnceScope __rapicorn_oncescope_ (RAPICORN_DO_ONCE_COUNTER); __rapicorn_oncescope_(); )

} // Lib
} // Rapicorn


#endif // __RAPICORN_THREADLIB_HH__
