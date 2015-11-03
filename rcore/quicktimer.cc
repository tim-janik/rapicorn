// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "quicktimer.hh"
#include "memory.hh"
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

namespace Rapicorn {

QuickTimer::Type       QuickTimer::timer_type = QuickTimer::NONE;
uint64 volatile        QuickTimer::timer_pcounter = ~uint64 (0);   // changes every few milliseconds while busy
uint64                 QuickTimer::rdtsc_mask = 0;                   // fmsb (rdtsc ticks per second)
static timer_t         quick_timer_ptimer = 0;
static pthread_mutex_t quick_timer_mutex = PTHREAD_MUTEX_INITIALIZER;
static uint32          quick_timer_rcount = 0;

void
QuickTimer::inc_pcounter (void)
{
  __sync_fetch_and_add (&timer_pcounter, 1);
}

bool
QuickTimer::toggle_timers (bool onoff)
{
  struct itimerspec const ptimer = { { 0, granularity * 1000 }, { 0, granularity * 1000 } };
  struct itimerspec const disarm = { { 0, 0 }, { 0, 0 } };
  return timer_settime (quick_timer_ptimer, 0, onoff ? &ptimer : &disarm, NULL) == 0;
}

#ifdef DOXYGEN
/**
 * Perform a very fast check of a global variable for timer expiration.
 * Usually, this function is inlined and does not consume more time than a regular volatile variable access,
 * which means it's ideally suited to check for time period expiration from inside inner loops.
 * @returns Whether the time period that QuickTimer was setup for has expired.
 */
bool QuickTimer::expired();
#endif // DOXYGEN

/** @class QuickTimer
 * The QuickTimer class allows fast timer expiration checks from inner loops.
 * On Unix the implementation uses CLOCK_PROCESS_CPUTIME_ID or CLOCK_REALTIME in
 * a concurrent handler which updates a global volatile variable periodically.
 * This allows very fast expiration checks in worker threads, with negligible
 * overhead for most uses.
 */
void
QuickTimer::init_timers ()
{
  assert (timer_type == QuickTimer::NONE);
  // prefer QuickTimer::PROCESS
  struct sigevent sevent = { { 0 } };
  sevent.sigev_notify = SIGEV_THREAD;                   // invoke sigev_notify_function
  sevent.sigev_notify_function = (void(*)(sigval_t)) inc_pcounter;
  const bool flipper_threadfunc = RAPICORN_FLIPPER ("quick-timer-threadfunc", "Rapicorn::QuickTimer: Use a separate thread to update a timer variable.");
  const bool flipper_cputime    = RAPICORN_FLIPPER ("quick-timer-cputime", "Rapicorn::QuickTimer: Use CLOCK_PROCESS_CPUTIME_ID which measures consumed CPU time.");
  const bool flipper_realtime   = RAPICORN_FLIPPER ("quick-timer-realtime", "Rapicorn::QuickTimer: Use CLOCK_REALTIME, the system wide realtime clock.");
  const bool flipper_rdtsc      = RAPICORN_FLIPPER ("quick-timer-rdtsc", "Rapicorn::QuickTimer: Use the X86 architecture RDTSC assembler command as a timer.");
  if (flipper_threadfunc &&
      (
#ifdef    _POSIX_CPUTIME
       (flipper_cputime && timer_create (CLOCK_PROCESS_CPUTIME_ID, &sevent, &quick_timer_ptimer) == 0) ||
#endif
       (flipper_realtime && timer_create (CLOCK_REALTIME, &sevent, &quick_timer_ptimer) == 0)))
    {
      if (toggle_timers (true) && toggle_timers (false))
        {
          timer_type = QuickTimer::PROCESS;
          return;
        }
      timer_delete (quick_timer_ptimer);
      quick_timer_ptimer = 0;
    }
  // RDTSC is the next best choice
  if (flipper_rdtsc && !rdtsc_mask && 0 != RAPICORN_X86_RDTSC() && timer_pcounter != RAPICORN_X86_RDTSC())
    {
      uint64 freq = 666 * 1000000; // fallback to assume 666MHz ticks for rdtsc
      int fd = open ("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", O_RDONLY);
      if (fd >= 0)
        {
          char buf[128] = { 0, };
          ssize_t l;
          do
            l = read (fd, buf, sizeof (buf));
          while (l < 0 && errno == EINTR);
          close (fd);
          buf[l > 0 && l < (int) sizeof (buf) ? l : 0] = 0;
          size_t n = 0;
          if (sscanf (buf, "%zu", &n) == 1 && n)
            freq = n * 1000;
        }
      rdtsc_mask = (~uint64 (0)) << fmsb (freq * granularity * 1000 / 1000000000);
      timer_type = QuickTimer::RDTSC;
      return;
    }
  // fallback to gettimeofday
  timer_type = QuickTimer::TIMEOFDAY;
}

void
QuickTimer::ref_timers()
{
  pthread_mutex_lock (&quick_timer_mutex);
  if (quick_timer_rcount++ == 0)
    {
      if (timer_type == QuickTimer::NONE)
        init_timers();
      if (timer_type == QuickTimer::PROCESS)
        toggle_timers (true);
    }
  pthread_mutex_unlock (&quick_timer_mutex);
}

void
QuickTimer::unref_timers()
{
  pthread_mutex_lock (&quick_timer_mutex);
  assert (quick_timer_rcount > 0);
  if (--quick_timer_rcount == 0)
    {
      if (timer_type == QuickTimer::PROCESS)
        {
          assert (quick_timer_ptimer != 0);
          toggle_timers (false);
        }
    }
  pthread_mutex_unlock (&quick_timer_mutex);
}

bool
QuickTimer::time_elapsed ()
{
  uint64 s = timestamp_realtime(), d = MAX (start_, s) - MIN (start_, s);
  return d >= usecs_;
}

/**
 * Restart the QuickTimer, using the previously set expiration period.
 */
void
QuickTimer::start ()
{
  start_ = timestamp_realtime();
  mark_ = 0;
  expired(); // calibrate
}

/**
 * Restart the QuickTimer, setting it to expire after @a usecs micro seconds.
 */
void
QuickTimer::start (uint64 usecs)
{
  usecs_ = usecs;
  start();
}

QuickTimer::~QuickTimer ()
{
  unref_timers();
}

/**
 * Construct a QuickTimer object, suitable to expire after @a usecs micro seconds.
 */
QuickTimer::QuickTimer (uint64 usecs) :
  usecs_ (0), start_ (0), mark_ (0)
{
  ref_timers();
  start (usecs);
}

} // Rapicorn
