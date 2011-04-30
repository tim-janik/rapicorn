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
using namespace Rapicorn;

static void
force_pid0_for_testing ()
{
  const char *s = getenv ("RAPICORN");
  setenv ("RAPICORN", String (String (s ? s : "") + ":testpid0").c_str(), 1);
  /* set switch 'testpid0' to enforce deterministic ouput */
}

static void
test_messaging ()
{
  TASSERT (Msg::NONE    == Msg::lookup_type ("none"));
  TASSERT (Msg::ALWAYS  == Msg::lookup_type ("always"));
  TASSERT (Msg::ERROR   == Msg::lookup_type ("error"));
  TASSERT (Msg::WARNING == Msg::lookup_type ("warning"));
  TASSERT (Msg::SCRIPT  == Msg::lookup_type ("script"));
  TASSERT (Msg::INFO    == Msg::lookup_type ("info"));
  TASSERT (Msg::DIAG    == Msg::lookup_type ("diag"));
  TASSERT (Msg::DEBUG   == Msg::lookup_type ("debug"));
  TASSERT (Msg::check (Msg::NONE) == false);
  TASSERT (Msg::check (Msg::ALWAYS) == true);
  Msg::enable (Msg::NONE);
  Msg::disable (Msg::ALWAYS);
  TASSERT (Msg::check (Msg::NONE) == false);
  TASSERT (Msg::check (Msg::ALWAYS) == true);
  TASSERT (Msg::check (Msg::INFO) == true);
  Msg::disable (Msg::INFO);
  TASSERT (Msg::check (Msg::INFO) == false);
  Msg::enable (Msg::INFO);
  TASSERT (Msg::check (Msg::INFO) == true);
  Msg::display (Msg::WARNING,
                Msg::Title ("Warning Title"),
                Msg::Text1 ("Primary warning message."),
                Msg::Text2 ("Secondary warning message."),
                Msg::Text2 ("Continuation of secondary warning message."),
                Msg::Text3 ("Message details: 1, 2, 3."),
                Msg::Text3 ("And more message details: a, b, c."),
                Msg::Check ("Show this message again."));
}

int
main (int   argc,
      char *argv[])
{
  force_pid0_for_testing();
  rapicorn_init_logtest (&argc, argv);

  if (argc >= 2 && String ("--test-logging") == argv[1])
    {
      static Rapicorn::Logging testing_debug = Rapicorn::Logging ("testing");
      RAPICORN_DEBUG (testing_debug, "logging test selected via: --test-logging");
      pdiag ("diagnostics on the last errno assignment");
      Logging::override_config (""); // cancel fatal-warnings, usually enforced for tests
      warning ("the next logging message might abort the program");
      error ("we're approaching serious conditions that may lead to abort()");
      fatal ("at this point, program aborting is certain");
    }

  Test::add ("Message Types", test_messaging);

  return Test::run();
}

/* vim:set ts=8 sts=2 sw=2: */
