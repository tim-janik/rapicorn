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
test_cxx_server_gui ()
{
  ApplicationImpl &app = ApplicationImpl::the(); // FIXME: use Application_SmartHandle once C++ bindings are ready
  Wind0wIface &wind0w = *app.create_wind0w ("Window");
  TOK();
  ItemImpl &titem = Factory::create_item ("TestItem");
  TOK();
  wind0w.impl().add (titem);
  TOK();
  /* close wind0w (and exit main loop) after first expose */
  wind0w.impl().enable_auto_close();
  TOK();
  /* verify and assert at least one TestItem rendering */
  uint old_seen_test = TestContainer::seen_test_items();
  TOK();
  /* show onscreen and handle events like expose */
  wind0w.show();
  app.execute_loops();
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
  Wind0wIface &wind0w = *app.create_wind0w ("alignment-test");
  TOK();
  WindowImpl &window = wind0w.impl();
  TestContainer *titem = window.interface<TestContainer*>();
  TASSERT (titem != NULL);
  titem->sig_assertion_ok += slot (assertion_ok);
  titem->sig_assertions_passed += slot (assertions_passed);
  titem->fatal_asserts (ServerTests::server_test_item_fatal_asserts);
  TOK();
  while (EventLoop::iterate_loops (false, false)) // loops_pending?
    EventLoop::iterate_loops (false, true);       // loops_dispatch
  /* close wind0w (and exit main loop) after first expose */
  window.enable_auto_close();
  /* verify and assert at least one TestItem rendering */
  uint old_seen_test = TestContainer::seen_test_items();
  wind0w.show();
  app.execute_loops();
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
  Wind0wIface &wind0w = *app.create_wind0w ("test-item-wind0w");
  TOK();
  WindowImpl &window = wind0w.impl();
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
  wind0w.close();
}
REGISTER_UITHREAD_TEST ("TestItem/Test TestItem (test-item-wind0w)", test_idl_test_item);

template<class C> C*
interface (ItemIface *itemi)
{
  ItemImpl *item = dynamic_cast<ItemImpl*> (itemi);
  if (item)
    return item->interface<C*>();
  return NULL;
}

static void
test_complex_dialog ()
{
  ensure_ui_file();
  ApplicationImpl &app = ApplicationImpl::the(); // FIXME: use Application_SmartHandle once C++ bindings are ready
  ItemIface *item = app.unique_component ("/#"); // invalid path
  TASSERT (item == NULL);
  item = app.unique_component ("/#complex-dialog"); // non-existing wind0w
  TASSERT (item == NULL);
  Wind0wIface &wind0w = *app.create_wind0w ("complex-dialog");
  TOK();
  if (ServerTests::server_test_run_dialogs)
    {
      wind0w.show();
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
}
REGISTER_UITHREAD_TEST ("TestItem/Test Comples Dialog (complex-dialog)", test_complex_dialog);

} // Anon
