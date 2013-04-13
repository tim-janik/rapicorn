/* Rapicorn
 * Copyright (C) 2006 Tim Janik
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
#include <rcore/testutils.hh>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
using namespace Rapicorn;

static void
bogus_func ()
{
  assert_return (0 == "assert-return-void");
}

static int
value_func (int)
{
  assert_return (0 == "assert-return+val", 0);
  return 1;
}

static void
test_logging (const char *logarg)
{
  errno = EINVAL;       // used by P* checks
  if (String ("--test-assert") == logarg)
    RAPICORN_ASSERT (0 == "test-assert");
  if (String ("--test-unreached") == logarg)
    RAPICORN_ASSERT_UNREACHED();
  if (String ("--test-fatal") == logarg)
    fatal ("execution has reached a fatal condition (\"test-fatal\")");
  if (String ("--test-TASSERT") == logarg)
    TASSERT (0 == "test-TASSERT");
  if (String ("--test-TCMP") == logarg)
    TCMP (0, ==, "validate failing TCMP");
  if (String ("--test-logging") == logarg)
    {
      debug_configure ("no-fatal-warnings"); // cancel fatal-warnings, usually enforced for tests
      RAPICORN_DEBUG ("logging test selected, will trigger various warnings (debugging message)");
      RAPICORN_CRITICAL_UNLESS (0 > 1);
      errno = EINVAL;
      RAPICORN_CRITICAL_UNLESS (errno == 0);
      critical ("execution has reached a critical condition (\"test-critical\")");
      bogus_func();
      value_func (0);
      exit (0);
    }
}

int
main (int   argc,
      char *argv[])
{
  init_core_test (__PRETTY_FILE__, &argc, argv);

  if (argc >= 2 || Test::logging())
    {
      // set testing switch 'testpid0' to enforce deterministic ouput
      debug_configure ("testpid0");
    }

  if (argc >= 2)
    test_logging (argv[1]);

  return Test::run();
}
