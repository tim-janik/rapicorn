/* Tests
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
//#define TEST_VERBOSE
#include <birnet/birnettests.h>
#include <rapicorn/rapicorn.hh>
#include <rapicorn/testitem.hh>

/* --- RapicornTester --- */
namespace Rapicorn {
struct RapicornTester {
  static bool
  loops_pending()
  {
    return EventLoop::iterate_loops (false, false);
  }
  static void
  loops_dispatch (bool may_block)
  {
    EventLoop::iterate_loops (may_block, true);
  }
  static void
  quit_loops ()
  {
    return EventLoop::quit_loops ();
  }
};
} // Rapicorn

/* --- tests --- */
namespace {
using namespace Rapicorn;

static bool test_item_fatal_asserts = true;

static void
assertion_ok (const String &assertion)
{
  TPRINT ("%s\n", assertion.c_str());
}

static void
assertions_passed ()
{
  TOK();
}

static void
run_and_test (const char *test_name)
{
  TSTART (test_name);
  Window window = Factory::create_window (test_name);
  TOK();
  AutoLocker locker (window);
  TOK();
  Root &root = window.root(); /* needs lock acquired */
  TOK();
  TestItem *titem = root.interface<TestItem*>();
  TASSERT (titem != NULL);
  titem->sig_assertion_ok += slot (assertion_ok);
  titem->sig_assertions_passed += slot (assertions_passed);
  titem->fatal_asserts (test_item_fatal_asserts);
  TOK();
  root.show();
  TOK();
  locker.unlock();
  TOK();
  while (RapicornTester::loops_pending())
    RapicornTester::loops_dispatch (false);
  TOK();
  printerr ("(auto-sleep)"); sleep (1); // FIXME: work around lack of show_now()
  TOK();
  while (RapicornTester::loops_pending())
    RapicornTester::loops_dispatch (false);
  TOK();
  uint seen_test = TestItem::seen_test_items();
  TASSERT (seen_test > 0);
  TDONE();
}

extern "C" int
main (int   argc,
      char *argv[])
{
  birnet_init_test (&argc, &argv);

  for (int i = 0; i < argc; i++)
    if (String (argv[i]) == "--non-fatal")
      test_item_fatal_asserts = false;

  /* initialize rapicorn */
  rapicorn_init_with_gtk_thread (&argc, &argv, NULL);
  AutoLocker ral (rapicorn_mutex);
  
  /* parse GUI description */
  Factory::must_parse_file ("testitems.xml", "TEST-ITEM", Path::dirname (argv[0]), Path::join (Path::dirname (argv[0]), ".."));

  /* create/run tests */
  run_and_test ("test-alignment");

  return 0;
}

} // Anon
