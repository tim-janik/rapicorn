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
#include <rcore/testutils.hh>
#include <rapicorn.hh>
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
  // TMSG ("%s\n", assertion.c_str());
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
  Window &window = *app.create_window ("Root");
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
  app.execute_loops();
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
  Window &window = *app.create_window ("alignment-test");
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
  app.execute_loops();
  uint seen_test = TestContainer::seen_test_items();
  TASSERT (seen_test > old_seen_test);
  /* test item rendering also executed various assertions */
  TDONE();
}

static void
idl_test_item_test ()
{
  TSTART ("idl-test-item");
  Window &window = *app.create_window ("test-item-window");
  TOK();
  Root &root = window.root();
  IdlTestItem *titemp = root.interface<IdlTestItem*>();
  TASSERT (titemp != NULL);
  IdlTestItem &titem = *titemp;
  titem.bool_prop (0); TASSERT (titem.bool_prop() == 0);
  titem.bool_prop (1); TASSERT (titem.bool_prop() == 1);
  titem.bool_prop (0); TASSERT (titem.bool_prop() == 0);
  titem.int_prop (765760); TASSERT (titem.int_prop() == 765760);
  titem.int_prop (-211232); TASSERT (titem.int_prop() == -211232);
  double f = 32.1231;
  titem.float_prop (f); TASSERT (abs (titem.float_prop() - f) < 1e-11); // assert double precision
  titem.string_prop (""); TASSERT (titem.string_prop() == "");
  titem.string_prop ("5768trzg"); TASSERT (titem.string_prop() == "5768trzg");
  titem.string_prop ("äöü"); TASSERT (titem.string_prop() == "äöü");
  titem.enum_prop (TEST_ENUM_VALUE1); TASSERT (titem.enum_prop() == TEST_ENUM_VALUE1);
  titem.enum_prop (TEST_ENUM_VALUE2); TASSERT (titem.enum_prop() == TEST_ENUM_VALUE2);
  titem.enum_prop (TEST_ENUM_VALUE3); TASSERT (titem.enum_prop() == TEST_ENUM_VALUE3);
  Requisition r (123, 765);
  titem.record_prop (r); TASSERT (titem.record_prop().width == r.width && titem.record_prop().height == r.height);
  StringList sl;
  sl.push_back ("one");
  sl.push_back ("2");
  sl.push_back ("THREE");
  titem.sequence_prop (sl); StringList sv = titem.sequence_prop();
  TASSERT (sv.size() == sl.size()); TASSERT (sv[2] == "THREE");
  titem.self_prop (NULL); TASSERT (titem.self_prop() == NULL);
  titem.self_prop (titemp); TASSERT (titem.self_prop() == titemp);
  window.close();
  TDONE();
}

static bool run_dialogs = false;

template<class C> C*
interface (It3m *it3m)
{
  Item *item = dynamic_cast<Item*> (it3m);
  if (item)
    return item->interface<C*>();
  return NULL;
}

static void
complex_dialog_test ()
{
  TSTART ("complex-dialog-test");
  It3m *item = app.unique_component ("/#"); // invalid path
  TASSERT (item == NULL);
  item = app.unique_component ("/#complex-dialog"); // non-existing window
  TASSERT (item == NULL);
  Window &window = *app.create_window ("complex-dialog");
  TOK();
  if (run_dialogs)
    {
      window.show();
      app.execute_loops();
    }
  TOK();
  item = app.unique_component ("/#complex-dialog");
  TASSERT (item != NULL);
  item = app.unique_component ("/Item#complex-dialog");
  TASSERT (item != NULL);
  item = app.unique_component ("/#complex-dialog/VBox/ScrollArea/Item");
  TASSERT (item != NULL);
  item = app.unique_component ("/#complex-dialog/VBox/Item");
  TASSERT (item == NULL); // not unique
  item = app.unique_component ("/#complex-dialog/Alignment");
  TASSERT (item != NULL);
  item = app.unique_component (" / #complex-dialog / Alignment / VBox ");
  TASSERT (item != NULL);
  item = app.unique_component ("/#complex-dialog/Alignment/VBox/#scroll-text");
  TASSERT (item != NULL);
  item = app.unique_component ("/#complex-dialog/Frame");
  TASSERT (item == NULL); // not unique
  item = app.unique_component ("/#complex-dialog/Frame[/Arrow#special-arrow]");
  TASSERT (item != NULL);
  item = app.unique_component ("/#complex-dialog/Button / Label");
  TASSERT (item == NULL); // not unique
  item = app.unique_component ("/#complex-dialog/Button / Label [ @markup-text =~ '\\bOk' ]");
  TASSERT (item != NULL);
  TASSERT (dynamic_cast<Text::Editor::Client*> (item) != NULL);
  item = app.unique_component ("/#complex-dialog/Button [ /Label [ @markup-text=~'\\bOk\\b' ] ]");
  TASSERT (item != NULL);
  TASSERT (dynamic_cast<ButtonArea*> (item) != NULL);
  TASSERT (dynamic_cast<Text::Editor::Client*> (item) == NULL);
  item = app.unique_component ("/#"); // invalid path
  TASSERT (item == NULL);
  TDONE();
}

extern "C" int
main (int   argc,
      char *argv[])
{
  rapicorn_init_test (&argc, argv);

  for (int i = 0; i < argc; i++)
    if (String (argv[i]) == "--non-fatal")
      test_item_fatal_asserts = false;
    else if (String (argv[i]) == "--run")
      run_dialogs = true;

  /* initialize rapicorn */
  app.init_with_x11 (&argc, &argv, "TestItemsTest");

  /* parse GUI description */
  app.auto_load ("RapicornTest", Rapicorn::Path::join (SRCDIR, "testitems.xml"), argv[0]);

  /* create/run tests */
  test_cxx_gui();
  test_test_item();
  idl_test_item_test();
  complex_dialog_test();

  return 0;
}

} // Anon
