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
#include "servertests.hh"
#include <ui/uithread.hh>
#include <ui/testitems.hh>

namespace {
using namespace Rapicorn;

static void
run_main_loop_recursive (bool blocking_while_primary = true)
{
  MainLoop *main_loop = uithread_main_loop();
  assert_return (main_loop != NULL);
  ref (main_loop);
  if (!blocking_while_primary)
    main_loop->iterate_pending();
  else
    while (!main_loop->finishable())
      main_loop->iterate (true);
  unref (main_loop);
}

static void
test_factory ()
{
  TOK();
  ApplicationImpl &app = ApplicationImpl::the();

  /* find and load GUI definitions relative to argv[0] */
  String factory_xml = "factory.xml";
  app.auto_load ("RapicornTest",                        // namespace domain,
                 Path::vpath_find (factory_xml),        // GUI file name
                 program_file());
  TOK();
  ItemImpl *item;
  TestContainer *titem;
  WindowIface &testwin = *app.create_window ("RapicornTest:test-TestItemL2");
  testwin.show();
  run_main_loop_recursive (false);
  TOK();
  WindowImpl *window = &testwin.impl();
  item = window->interface<ItemImpl*> ("TestItemL2");
  TASSERT (item != NULL);
  titem = dynamic_cast<TestContainer*> (item);
  TASSERT (titem != NULL);
  if (0)
    {
      printout ("\n");
      printout ("TestContainer::accu: %s\n", titem->accu().c_str());
      printout ("TestContainer::accu_history: %s\n", titem->accu_history().c_str());
    }
  TASSERT (titem->accu_history() == "L0L1L2Instance");
  TOK();
  // test Item::name()
  TASSERT (item->name().empty() == false); // has factory default
  String factory_default = item->name();
  item->name ("FooBar_4356786453567");
  TASSERT (item->name() == "FooBar_4356786453567");
  item->name ("");
  TASSERT (item->name() != "FooBar_4356786453567");
  TASSERT (item->name().empty() == false); // back to factory default
  TASSERT (item->name() == factory_default);
  TOK();
  testwin.close();
  TOK();
}
REGISTER_UITHREAD_TEST ("Factory/Test Item Factory", test_factory);

static void
test_cxx_server_gui ()
{
  ApplicationImpl &app = ApplicationImpl::the(); // FIXME: use Application_SmartHandle once C++ bindings are ready
  WindowIface &window = *app.create_window ("Window");
  TOK();
  ItemImpl &titem = Factory::create_ui_item ("TestItem");
  TOK();
  window.impl().add (titem);
  TOK();
  /* close window (and exit main loop) after first expose */
  window.impl().enable_auto_close();
  TOK();
  /* verify and assert at least one TestItem rendering */
  uint old_seen_test = TestContainer::seen_test_items();
  TOK();
  /* show onscreen and handle events like expose */
  window.show();
  run_main_loop_recursive();
  TOK();
  /* assert TestItem rendering */
  uint seen_test = TestContainer::seen_test_items();
  TASSERT (seen_test > old_seen_test); // may fail due to missing exposes (locked screens) needs PNG etc. backends
}
REGISTER_UITHREAD_TEST ("TestItem/Test C++ Server Side GUI", test_cxx_server_gui);

static void
assertion_ok (const String &assertion)
{
  TINFO ("assertion_ok: %s", assertion.c_str());
}

static void
assertions_passed ()
{
  TOK();
}

static void
test_test_item ()
{
  ApplicationImpl &app = ApplicationImpl::the(); // FIXME: use Application_SmartHandle once C++ bindings are ready
  WindowIface &window_iface = *app.create_window ("RapicornTest:alignment-test");
  TOK();
  WindowImpl &window = window_iface.impl();
  TestContainer *titem = window.interface<TestContainer*>();
  TASSERT (titem != NULL);
  titem->sig_assertion_ok += slot (assertion_ok);
  titem->sig_assertions_passed += slot (assertions_passed);
  titem->fatal_asserts (ServerTests::server_test_item_fatal_asserts);
  TOK();
  run_main_loop_recursive (false);
  /* close window (and exit main loop) after first expose */
  window.enable_auto_close();
  /* verify and assert at least one TestItem rendering */
  uint old_seen_test = TestContainer::seen_test_items();
  window.show();
  run_main_loop_recursive();
  uint seen_test = TestContainer::seen_test_items();
  TASSERT (seen_test > old_seen_test);
  /* test item rendering also executed various assertions */
}
REGISTER_UITHREAD_TEST ("TestItem/Test GUI assertions (alignment-test)", test_test_item);

static void
ensure_ui_file()
{
  static bool initialized = false;
  if (once_enter (&initialized))
    {
      // first, load required ui files
      ApplicationImpl &app = ApplicationImpl::the();
      app.auto_load ("RapicornTest", Path::vpath_find ("testitems.xml"), program_file());
      once_leave (&initialized, true);
    }
}

static void
test_idl_test_item ()
{
  ensure_ui_file();
  ApplicationImpl &app = ApplicationImpl::the(); // FIXME: use Application_SmartHandle once C++ bindings are ready
  WindowIface &window_iface = *app.create_window ("RapicornTest:test-item-window");
  TOK();
  WindowImpl &window = window_iface.impl();
  IdlTestItemIface *titemp = window.interface<IdlTestItemIface*>();
  TASSERT (titemp != NULL);
  IdlTestItemIface &titem = *titemp;
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
}
REGISTER_UITHREAD_TEST ("TestItem/Test TestItem (test-item-window)", test_idl_test_item);

static void
test_complex_dialog ()
{
  ensure_ui_file();
  ApplicationImpl &app = ApplicationImpl::the(); // FIXME: use Application_SmartHandle once C++ bindings are ready
  WindowIface *windowp = app.query_window ("/#"); // invalid path
  TASSERT (windowp == NULL);

  windowp = app.query_window ("#complex-dialog"); // not yet existing window
  TASSERT (windowp == NULL);
  WindowIface &window = *app.create_window ("RapicornTest:complex-dialog");
  TOK();
  if (ServerTests::server_test_run_dialogs)
    {
      window.show();
      run_main_loop_recursive();
    }
  TOK();
  windowp = app.query_window ("#complex-dialog"); // now existing window
  TASSERT (windowp != NULL);

  ItemIface *item = window.query_selector_unique ("#complex-dialog");
  size_t count;
  TASSERT (item != NULL);
  item = window.query_selector_unique (".Item#complex-dialog");
  TASSERT (item != NULL);
  count = window.query_selector_all ("#complex-dialog .VBox .ScrollArea .Item").size();
  TASSERT (count > 0);
  count = window.query_selector_all (":root .VBox .Item").size();
  TASSERT (count > 0);
  item = window.query_selector (":root .Alignment");
  TASSERT (item != NULL);
  item = window.query_selector (":root .Alignment .VBox");
  TASSERT (item != NULL);
  item = window.query_selector_unique (":root .Alignment .VBox #scroll-text");
  TASSERT (item != NULL);
  item = window.query_selector_unique (":root .Frame");
  TASSERT (item == NULL); // not unique
  item = window.query_selector_unique (":root .Frame! .Arrow#special-arrow");
  TASSERT (item != NULL && dynamic_cast<Frame*> (item) != NULL);
  item = window.query_selector_unique (":root .Button .Label");
  TASSERT (item == NULL); // not unique
  item = window.query_selector_unique (":root .Button .Label[markup-text*='Ok']");
  TASSERT (item != NULL && dynamic_cast<Text::Editor::Client*> (item) != NULL);
  item = window.query_selector_unique (":root .Button! Label[markup-text*='Ok']");
  TASSERT (item != NULL && dynamic_cast<ButtonAreaImpl*> (item) != NULL && dynamic_cast<Text::Editor::Client*> (item) == NULL);
  item = window.query_selector_unique ("/#"); // invalid path
  TASSERT (item == NULL);
}
REGISTER_UITHREAD_TEST ("TestItem/Test Comples Dialog (complex-dialog)", test_complex_dialog);

} // Anon
