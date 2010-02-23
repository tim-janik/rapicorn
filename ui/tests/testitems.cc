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
#include <core/rapicorntests.h>
#include <ui/rapicorn.hh>
#include <ui/testitems.hh>

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
test_cxx_gui ()
{
  TSTART ("C++GUI Test");
  Window window = Factory::create_window ("Root");
  TOK();
  Item &titem = Factory::create_item ("TestItem");
  TOK();
  window.root().add (titem);
  TOK();
  /* close window (and exit main loop) after first expose */
  window.root().enable_auto_close();
  TOK();
  /* verify and assert at least one TestItem rendering */
  uint old_seen_test = TestContainer::seen_test_items();
  TOK();
  /* show onscreen and handle events like expose */
  window.show();
  Application::execute_loops();
  TOK();
  /* assert TestItem rendering */
  uint seen_test = TestContainer::seen_test_items();
  TASSERT (seen_test > old_seen_test); // may fail due to missing exposes (locked screens) needs PNG etc. backends
  TDONE();
}

static void
test_test_item ()
{
  TSTART ("alignment-test");
  Window window = Factory::create_window ("alignment-test");
  TOK();
  Root &root = window.root();
  TestContainer *titem = root.interface<TestContainer*>();
  TASSERT (titem != NULL);
  titem->sig_assertion_ok += slot (assertion_ok);
  titem->sig_assertions_passed += slot (assertions_passed);
  titem->fatal_asserts (test_item_fatal_asserts);
  TOK();
  while (RapicornTester::loops_pending())
    RapicornTester::loops_dispatch (false);
  /* close window (and exit main loop) after first expose */
  root.enable_auto_close();
  /* verify and assert at least one TestItem rendering */
  uint old_seen_test = TestContainer::seen_test_items();
  window.show();
  Application::execute_loops();
  uint seen_test = TestContainer::seen_test_items();
  TASSERT (seen_test > old_seen_test);
  /* test item rendering also executed various assertions */
  TDONE();
  return;
}

extern "C" int
main (int   argc,
      char *argv[])
{
  rapicorn_init_test (&argc, &argv);

  for (int i = 0; i < argc; i++)
    if (String (argv[i]) == "--non-fatal")
      test_item_fatal_asserts = false;

  /* initialize rapicorn */
  Application::init_with_x11 (&argc, &argv, "TestItemsTest");
  
  /* parse GUI description */
  Application::auto_load ("testitems", Rapicorn::Path::join (SRCDIR, "testitems.xml"), argv[0]);

  /* create/run tests */
  test_cxx_gui ();
  test_test_item ();

  return 0;
}

} // Anon
