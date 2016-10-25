// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_QUICKTIMER_HH__
#define __RAPICORN_QUICKTIMER_HH__

#include <rcore/cpuasm.hh>
#include <rcore/utilities.hh>

namespace Rapicorn {

class QuickTimer {
  enum Type { NONE, PROCESS, RDTSC, TIMEOFDAY };
  uint64        usecs_, start_, mark_;
  static uint64 volatile timer_pcounter;
  static uint64 const granularity = 1700; // rough granularity for expiration checks
  static Type   timer_type;
  static uint64 rdtsc_mask;
  static bool   toggle_timers (bool);
  static void   inc_pcounter  ();
  static void   unref_timers  ();
  static void   init_timers   ();
  static void   ref_timers    ();
  bool          time_elapsed  ();
  RAPICORN_CLASS_NON_COPYABLE (QuickTimer);
public:
  explicit      QuickTimer      (uint64 usecs);
  virtual      ~QuickTimer      ();
  void          start           ();
  void          start           (uint64 usecs);
  inline bool   expired         ();
};


// === Implementation Details ===
inline bool // inlined for performance
QuickTimer::expired()
{
  // fast path: QuickTimer::PROCESS and counter hasn't changed
  if (RAPICORN_LIKELY (mark_ == timer_pcounter))
    return false;
  // fallback to rdtsc polling
  if (RAPICORN_HAVE_X86_RDTSC && RAPICORN_LIKELY (timer_type == QuickTimer::RDTSC))
    {
      const uint64 rnow = RAPICORN_X86_RDTSC();
      // check: ABS (rnow - last) * 1000000 / rdtsc_timer_freq >= granularity
      if (RAPICORN_UNLIKELY ((rnow ^ mark_) & rdtsc_mask))
        {
          mark_ = rnow;
          return RAPICORN_UNLIKELY (time_elapsed());
        }
      return false;
    }
  // fallback to gettimeofday polling
  if (RAPICORN_LIKELY (timer_type == QuickTimer::TIMEOFDAY))
    {
      const uint64 tnow = timestamp_realtime();
      if (RAPICORN_UNLIKELY (tnow - mark_ >= granularity))
        {
          mark_ = tnow;
          return RAPICORN_UNLIKELY (time_elapsed());
        }
      return false;
    }
  // QuickTimer::PROCESS, counter changed
  mark_ = timer_pcounter; // update mark
  return RAPICORN_UNLIKELY (time_elapsed());
}

} // Rapicorn

#endif /* __RAPICORN_QUICKTIMER_HH__ */
