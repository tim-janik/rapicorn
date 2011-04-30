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

namespace Rapicorn {
namespace Test {

void
test_output (int kind, const char *format, ...)
{
  va_list args;
  va_start (args, format);
  String msg = string_vprintf (format, args);
  va_end (args);
  String sout, bar;
  switch (kind)
    {
    case 3: // test program title
      if (verbose())
        {
          bar = "### ** +--" + String (msg.size(), '-') + "--+ ** ###";
          msg = "### ** +  " + msg + "  + ** ###";
          sout = "\n" + bar + "\n" + msg + "\n" + bar + "\n";
        }
      else
        sout = "TEST: " + msg + "\n";
      break;
    case 4: // test title
      if (verbose())
        {
          msg = "## Testing: " + msg + "...";
          sout = "\n" + msg + "\n";
        }
      else
        sout = "  " + msg + ":" + String (70 - MIN (70, msg.size()), ' ');
      break;
    case 5: // test done
      if (!verbose())
        sout = "OK\n";
      break;
    case 1: // test message
      sout = msg;
      if (sout.size() && sout[sout.size()-1] != '\n')
        sout = sout + "\n";
      break;
    default: // 0 - regular msg
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

bool
perf (void)
{
  return init_settings().test_perf;
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
  rapicorn_init_core (argc, &argv, NULL, ivalues); // FIXME: argv
  unsigned int flags = g_log_set_always_fatal ((GLogLevelFlags) G_LOG_FATAL_MASK);
  g_log_set_always_fatal ((GLogLevelFlags) (flags | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL));
  Logging::override_config ("fatal-warnings");
  CPUInfo ci = cpu_info();
  if (init_settings().test_perf)
    g_printerr ("PERF: %s\n", g_get_prgname());
  else
    TTITLE (argv[0]);
}
} // Rapicorn
