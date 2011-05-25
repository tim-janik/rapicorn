/* Rapicorn
 * Copyright (C) 2008 Tim Janik
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
#include "testutils.hh"

#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#define VERBOSE_TAG     100

namespace Rapicorn {
namespace Test {

static clockid_t bench_time_clockid = CLOCK_REALTIME;
static double    bench_time_resolution = 0.000001;      // 1µs resolution assumed for gettimeofday

Timer::Timer (double deadline_in_secs) :
  m_deadline (deadline_in_secs), m_test_duration (0), m_n_runs (0)
{
#if defined (_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 199309L
  static bool initialized = false;
  if (once_enter (&initialized))
    {
      struct timespec tp = { 0, 0 };
      // CLOCK_MONOTONIC_RAW cannot slew, but doesn't measure SI seconds accurately
      // CLOCK_MONOTONIC may slew, but attempts to accurately measure SI seconds
#ifdef CLOCK_MONOTONIC
      if (bench_time_clockid == CLOCK_REALTIME && clock_getres (CLOCK_MONOTONIC, &tp) >= 0)
        bench_time_resolution = tp.tv_sec + tp.tv_nsec / 1000000000.0, bench_time_clockid = CLOCK_MONOTONIC;
#endif
      // CLOCK_REALTIME may slew and jump
      if (bench_time_clockid == CLOCK_REALTIME && clock_getres (bench_time_clockid, &tp) >= 0)
        bench_time_resolution = tp.tv_sec + tp.tv_nsec / 1000000000.0, bench_time_clockid = CLOCK_REALTIME;
      once_leave (&initialized, true);
    }
#endif // _POSIX_C_SOURCE
}

Timer::~Timer ()
{}

double
Timer::bench_time ()
{
#if defined (_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 199309L
  struct timespec tp = { 0, 0 };
  if (ISLIKELY (clock_gettime (bench_time_clockid, &tp) >= 0))
    {
      double stamp = tp.tv_sec;
      stamp += tp.tv_nsec / 1000000000.0;
      return stamp;
    }
#endif // _POSIX_C_SOURCE
  uint64 ustamp = timestamp_now();
  return ustamp / 1000000.0;
}

int64
Timer::loops_needed () const
{
  if (m_samples.size() < 7)
    return m_n_runs + 1;        // force significant number of test runs
  const double deadline = MAX (m_deadline == 0.0 ? 0.005 : m_deadline, bench_time_resolution * 10000.0);
  if (m_test_duration > deadline * 0.5)
    return 0;                   // time for testing exceeded
  // we double the number of tests per run, to gain more accuracy: 0+1, 1+1, 3+1, 7+1, ...
  return m_n_runs + 1;
}

void
Timer::reset()
{
  m_samples.resize (0);
  m_test_duration = 0;
  m_n_runs = 0;
}

void
Timer::submit (double elapsed,
               int64  repetitions)
{
  m_test_duration += elapsed;
  m_n_runs += repetitions;
  if (elapsed >= bench_time_resolution * 100.0) // force error below 1%
    m_samples.push_back (elapsed / repetitions);
}

double
Timer::min_elapsed () const
{
  double m = DBL_MAX;
  for (size_t i = 0; i < m_samples.size(); i++)
    m = MIN (m, m_samples[i]);
  return m;
}

double
Timer::max_elapsed () const
{
  double m = 0;
  for (size_t i = 0; i < m_samples.size(); i++)
    m = MAX (m, m_samples[i]);
  return m;
}

static String
ensure_newline (const String &s)
{
  if (s.size() && s[s.size()-1] != '\n')
    return s + "\n";
  return s;
}

static __thread char *test_warning = NULL;
static __thread char *test_start = NULL;

void
test_output (int kind, const char *format, ...)
{
  va_list args;
  va_start (args, format);
  String msg = string_vprintf (format, args);
  va_end (args);
  String sout, bar;
  switch (verbose() ? VERBOSE_TAG + kind : kind)
    {
    case 1:                     // test message
    case 1 + VERBOSE_TAG:       // test message
      sout = ensure_newline (msg);
      break;
    case 2:                     // conditional test info
      break;
    case 2 + VERBOSE_TAG:       // conditional test info
      sout = ensure_newline (msg);
      break;
    case 3:                     // test program title
      sout = "START:   " + ensure_newline (msg);
      break;
    case 3 + VERBOSE_TAG:       // test program title
      bar = "### ** +--" + String (msg.size(), '-') + "--+ ** ###";
      msg = "### ** +  " + msg + "  + ** ###";
      sout = "\n" + bar + "\n" + msg + "\n" + bar + "\n";
      break;
    case 4:                     // test start
      sout = "  TEST   " + msg + ":" + String (63 - MIN (63, msg.size()), ' ');
      if (!test_start)
        {
          test_start = strdup (sout.c_str());
          sout = "";
        }
      break;
    case 4 + VERBOSE_TAG:       // test start
      sout = "\n## Testing: " + msg + "...\n";
      break;
    case 5:                     // test done
      if (test_start)
        {
          sout = test_start;
          free (test_start);
          test_start = NULL;
        }
      else
        sout = "";
      if (test_warning)
        {
          String w (test_warning);
          free (test_warning);
          test_warning = NULL;
          sout += "WARN\n" + ensure_newline (w);
        }
      else
        sout += "OK\n";
      break;
    case 5 + VERBOSE_TAG:       // test done
      break;
    case 6:                     // test warning
      {
        String w;
        if (test_warning)
          {
            w = test_warning;
            free (test_warning);
          }
        test_warning = strdup ((w + ensure_newline (msg)).c_str());
      }
      break;
    case 6 + VERBOSE_TAG:       // test warning
      sout = "WARNING: " + ensure_newline (msg);
      break;
    default:
    case 0 + VERBOSE_TAG:       // regular msg
    case 0:                     // regular msg
      sout = msg;
      break;
    }
  fflush (stdout);
  fputs (sout.c_str(), stderr);
  fflush (stderr);
}

static vector<void(*)(void*)> testfuncs;
static vector<void*>          testdatas;
static vector<String>         testnames;

void
add_internal (const String &testname,
              void        (*test_func) (void*),
              void         *data)
{
  testnames.push_back (testname);
  testfuncs.push_back ((void(*)(void*)) test_func);
  testdatas.push_back ((void*) data);
}

void
add (const String &testname,
     void        (*test_func) (void))
{
  add (testname, (void(*)(void*)) test_func, (void*) 0);
}

int
run (void)
{
  for (uint i = 0; i < testfuncs.size(); i++)
    {
      TSTART ("%s", testnames[i].c_str());
      testfuncs[i] (testdatas[i]);
      TDONE();
    }
  return 0;
}

bool
verbose (void)
{
  return init_settings().test_verbose;
}

bool
quick (void)
{
  return init_settings().test_quick;
}

bool
slow (void)
{
  return init_settings().test_slow;
}

bool
thorough (void)
{
  return init_settings().test_slow;
}

char
rand_bit (void)
{
  return 0 != (rand_int() & (1 << 15));
}

int32
rand_int (void)
{
  return g_random_int(); // g_test_rand_int();
}

int32
rand_int_range (int32 begin,
                int32 end)
{
  return g_random_int_range (begin, end); // g_test_rand_int_range()
}

double
test_rand_double (void)
{
  return g_random_double(); // g_test_rand_double()
}

double
test_rand_double_range (double range_start,
                        double range_end)
{
  return g_random_double_range (range_start, range_end); // g_test_rand_double_range()
}

} // Test
} // Rapicorn

namespace Rapicorn {
static bool verbose_init = false;

void
rapicorn_init_logtest (int *argc, char **argv)
{
  verbose_init = true;
  rapicorn_init_test (argc, argv);
}

void
rapicorn_init_test (int   *argc,
                    char **argv)
{
  /* check that NULL is defined to __null in C++ on 64bit */
  RAPICORN_ASSERT (sizeof (NULL) == sizeof (void*));
  /* normal initialization */
  RapicornInitValue ivalues[] = {
    { "stand-alone", "true" },
    { "test-verbose", verbose_init ? "true" : "false" },
    { "rapicorn-test-parse-args", "true" },
    { NULL }
  };
  rapicorn_init_core (argc, argv, NULL, ivalues); // FIXME: argv
  unsigned int flags = g_log_set_always_fatal ((GLogLevelFlags) G_LOG_FATAL_MASK);
  g_log_set_always_fatal ((GLogLevelFlags) (flags | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL));
  Logging::override_config ("fatal-warnings");
  CPUInfo ci = cpu_info();
  TTITLE ("%s", argv[0]);
}
} // Rapicorn
