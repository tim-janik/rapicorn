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
#include "main.hh"
#include "strings.hh"

#include <algorithm>
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#define VERBOSE_TAG     100

namespace Rapicorn {
namespace Test {

Timer::Timer (double deadline_in_secs) :
  m_deadline (deadline_in_secs), m_test_duration (0), m_n_runs (0)
{}

Timer::~Timer ()
{}

double
Timer::bench_time ()
{
  /* timestamp_benchmark() counts nano seconds since program start, so
   * it's not going to exceed the 52bit double mantissa too fast.
   */
  return timestamp_benchmark() / 1000000000.0;
}

int64
Timer::loops_needed () const
{
  if (m_samples.size() < 7)
    return m_n_runs + 1;        // force significant number of test runs
  double resolution = timestamp_resolution() / 1000000000.0;
  const double deadline = MAX (m_deadline == 0.0 ? 0.005 : m_deadline, resolution * 10000.0);
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
  double resolution = timestamp_resolution() / 1000000000.0;
  if (elapsed >= resolution * 100.0) // force error below 1%
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
      if (logging() && slow())
        msg += " (logging,slow)";
      else if (logging())
        msg += " (logging)";
      else if (slow())
        msg += " (slow)";
      sout = "START:   " + ensure_newline (msg);
      break;
    case 3 + VERBOSE_TAG:       // test program title
      bar = "### ** +--" + String (msg.size(), '-') + "--+ ** ###";
      msg = "### ** +  " + msg + "  + ** ###";
      sout = "\n" + bar + "\n" + msg + "\n" + bar + "\n\n";
      break;
    case 4: case 4 + VERBOSE_TAG:       // test start
      sout = "  TEST   " + msg + ":" + String (63 - MIN (63, msg.size()), ' ');
      if (!test_start)
        {
          test_start = strdup (sout.c_str());
          sout = "";
        }
      if (verbose())
        sout = "# Test:  " + msg + " ...\n";
      break;
    case 5: case 5 + VERBOSE_TAG:       // test done
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

struct TestEntry {
  void          (*func) (void*);
  void           *data;
  String          name;
  char            kind;
  TestEntry      *next;
};

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

static TestEntry *volatile test_entry_list = NULL;

void
RegisterTest::add_test (char kind, const String &testname, void (*test_func) (void*), void *data)
{
  TestEntry *te = new TestEntry;
  te->func = test_func;
  te->data = data;
  te->name = testname;
  te->kind = kind;
  do
    te->next = test_entry_list;
  while (!Atomic::value_cas (&test_entry_list, te->next, te));
}

static bool flag_test_verbose = false;
static bool flag_test_log = false;
static bool flag_test_slow = false;
static bool flag_test_ui = false;

bool
verbose (void)
{
  return flag_test_verbose;
}

bool
logging (void)
{
  return flag_test_log;
}

bool
slow (void)
{
  return flag_test_slow;
}

bool
ui_test (void)
{
  return flag_test_ui;
}

static RegisterTest::TestTrigger test_trigger = NULL;

static int
test_entry_cmp (const TestEntry *const &v1,
                const TestEntry *const &v2)
{
  return strverscmp (v1->name.c_str(), v2->name.c_str()) < 0;
}

static void
run_tests (void)
{
  vector<TestEntry*> entries;
  for (TestEntry *node = test_entry_list; node; node = node->next)
    entries.push_back (node);
  stable_sort (entries.begin(), entries.end(), test_entry_cmp);
  char ftype = logging() ? 'l' : (slow() ? 's' : 't');
  if (ui_test())
    ftype = toupper (ftype);
  for (size_t i = 0; i < entries.size(); i++)
    {
      const TestEntry *te = entries[i];
      if (te->kind == ftype)
        {
          TSTART ("%s", te->name.c_str());
          te->func (te->data);
          TDONE();
        }
    }
  for (uint i = 0; i < testfuncs.size(); i++)
    {
      TSTART ("%s", testnames[i].c_str());
      testfuncs[i] (testdatas[i]);
      TDONE();
    }
}

void
RegisterTest::test_set_trigger (TestTrigger func)
{
  test_trigger = func;
}

int
run (void)
{
  flag_test_ui = false;
  run_tests();
  flag_test_ui = true;
  if (test_trigger)
    test_trigger (run_tests);
  flag_test_ui = false;
  return 0;
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

void
init_core_test (const String       &app_ident,
                int                *argcp,
                char              **argv,
                const StringVector &args)
{
  // check that NULL is defined to __null in C++ on 64bit
  RAPICORN_ASSERT (sizeof (NULL) == sizeof (void*));
  // Rapicorn initialization
  const char *ivalues[] = { "autonomous=1", "parse-testargs=1" };
  StringVector targs = RAPICORN_STRING_VECTOR_FROM_ARRAY (ivalues);
  std::copy (args.begin(), args.end(), std::back_inserter (targs));
  init_core (app_ident, argcp, argv, targs);
  debug_configure ("fatal-criticals:fatal-warnings");
  const uint fatal_mask = g_log_set_always_fatal (GLogLevelFlags (G_LOG_FATAL_MASK));
  g_log_set_always_fatal (GLogLevelFlags (fatal_mask | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL));
  CPUInfo ci = cpu_info();
  Test::flag_test_log = (InitSettings::test_codes() & 0x2) || debug_check ("test-log", Test::flag_test_log);
  Test::flag_test_verbose = (InitSettings::test_codes() & 0x1) || debug_check ("test-verbose", Test::flag_test_verbose |
                                                                               Test::flag_test_log);
  Test::flag_test_slow = (InitSettings::test_codes() & 0x4) || debug_check ("test-slow", Test::flag_test_slow);
  TTITLE ("%s", Path::basename (argv[0]).c_str());
}

} // Rapicorn
