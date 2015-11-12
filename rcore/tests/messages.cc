// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
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
rapicorn_debug_fatal_syslog_0()
{
  // prevent assertion tests from uselessly spewing into syslog
  const char *v = getenv ("RAPICORN_DEBUG");
  String s = v ? v : "";
  s += ":fatal-syslog=0";
  setenv ("RAPICORN_DEBUG", s.c_str(), 1);
}

static void
test_logging (const char *logarg)
{
  errno = EINVAL;       // used by P* checks
  if (String ("--test-assert") == logarg)
    {
      rapicorn_debug_fatal_syslog_0();
      RAPICORN_ASSERT (0 == "test-assert");
    }
  if (String ("--test-unreached") == logarg)
    {
      rapicorn_debug_fatal_syslog_0();
      RAPICORN_ASSERT_UNREACHED();
    }
  if (String ("--test-fatal") == logarg)
    {
      rapicorn_debug_fatal_syslog_0();
      fatal ("execution has reached a fatal condition (\"test-fatal\")");
    }
  if (String ("--test-TASSERT") == logarg)
    TASSERT (0 == "test-TASSERT");
  if (String ("--test-assertion-hook") == logarg)
    {
      Test::set_assertion_hook ([] () { printerr ("assertion-hook triggered: magic=0x%x\n", 0xdecaff); });
      TASSERT (0 == "test-assertion-hook");
    }
  if (String ("--test-TCMP") == logarg)
    TCMP (0, ==, "validate failing TCMP");
  if (String ("--test-logging") == logarg)
    {
      debug_config_add ("fatal-warnings=0"); // cancel fatal-warnings, usually enforced for tests
      RAPICORN_KEY_DEBUG ("Test", "logging test selected, enabling various debugging messages");
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

  if (argc >= 2)
    test_logging (argv[1]);

  return Test::run();
}
