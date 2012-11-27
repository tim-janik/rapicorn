// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_QUICKTIMER_HH__
#define __RAPICORN_QUICKTIMER_HH__

#include <rcore/utilities.hh>

namespace Rapicorn {

class QuickTimer {
  enum Type { NONE, PROCESS, RDTSC, TIMEOFDAY };
  uint64        m_usecs, m_start, m_mark;
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
  if (RAPICORN_LIKELY (m_mark == timer_pcounter))
    return false;
  // fallback to rdtsc polling
  if (RAPICORN_HAVE_X86_RDTSC && RAPICORN_LIKELY (timer_type == QuickTimer::RDTSC))
    {
      const uint64 rnow = RAPICORN_X86_RDTSC();
      // check: ABS (rnow - last) * 1000000 / rdtsc_timer_freq >= granularity
      if (RAPICORN_UNLIKELY ((rnow ^ m_mark) & rdtsc_mask))
        {
          m_mark = rnow;
          return RAPICORN_UNLIKELY (time_elapsed());
        }
      return false;
    }
  // fallback to gettimeofday polling
  if (RAPICORN_LIKELY (timer_type == QuickTimer::TIMEOFDAY))
    {
      const uint64 tnow = timestamp_realtime();
      if (RAPICORN_UNLIKELY (ABS (tnow - m_mark) >= granularity))
        {
          m_mark = tnow;
          return RAPICORN_UNLIKELY (time_elapsed());
        }
      return false;
    }
  // QuickTimer::PROCESS, counter changed
  m_mark = timer_pcounter; // update mark
  return RAPICORN_UNLIKELY (time_elapsed());
}

} // Rapicorn

#endif /* __RAPICORN_QUICKTIMER_HH__ */
