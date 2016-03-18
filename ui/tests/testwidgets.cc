// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "servertests.hh"
#include <ui/uithread.hh>
#include <ui/testwidgets.hh>

namespace {
using namespace Rapicorn;

static void
run_main_loop_recursive (bool blocking_while_primary = true)
{
  MainLoopP main_loop = uithread_main_loop();
  assert_return (main_loop != NULL);
  if (!blocking_while_primary)
    main_loop->iterate_pending();
  else
    while (!main_loop->finishable())
      main_loop->iterate (true);
}

static void
test_factory ()
{
  TOK();
  ApplicationImpl &app = ApplicationImpl::the();

  /* find and load GUI definitions relative to argv[0] */
  String factory_xml = "factory.xml";
  app.auto_load (Path::vpath_find (factory_xml),        // GUI file name
                 program_argv0());                      // binary to determine file search path
  TOK();
  WidgetImpl *widget;
  WindowIfaceP testwin = app.create_window ("test-TestWidgetL2");
  TASSERT (testwin);
  TASSERT (dynamic_cast<WindowImpl*> (testwin.get()));
  testwin->show();
  run_main_loop_recursive (false);
  TOK();
  WindowImpl *window = &testwin->impl();
  widget = window->interface<WidgetImpl*> ("TestWidgetL2");
  TASSERT (widget != NULL);
  TestContainerImpl *twidget = dynamic_cast<TestContainerImpl*> (widget);
  TASSERT (twidget != NULL);
  if (0)
    {
      printout ("\n");
      printout ("TestContainer::accu: %s\n", twidget->accu().c_str());
      printout ("TestContainer::accu_history: %s\n", twidget->accu_history().c_str());
    }
  TASSERT (twidget->accu_history() == "L0L1L2Instance");
  TOK();
  // test Widget::name()
  TASSERT (widget->name().empty() == false); // has factory default
  String factory_default = widget->name();
  widget->name ("FooBar_4356786453567");
  TASSERT (widget->name() == "FooBar_4356786453567");
  widget->name ("");
  TASSERT (widget->name() != "FooBar_4356786453567");
  TASSERT (widget->name().empty() == false); // back to factory default
  TASSERT (widget->name() == factory_default);
  TOK();
  testwin->close();
  TOK();
}
REGISTER_UITHREAD_TEST ("Factory/Test Widget Factory", test_factory);

static void
test_cxx_server_gui ()
{
  ApplicationImpl &app = ApplicationImpl::the(); // FIXME: use Application_RemoteHandle once C++ bindings are ready
  WindowIface &window = *app.create_window ("Window");
  TOK();
  WidgetImplP twidget = Factory::create_ui_widget ("RapicornTestWidget");
  TOK();
  window.impl().add (*twidget);
  TOK();
  /* close window (and exit main loop) after first expose */
  window.impl().sig_displayed() += [&window]() { window.close(); };
  TOK();
  /* verify and assert at least one RapicornTestWidget rendering */
  uint old_seen_test = TestContainerImpl::seen_test_widgets();
  TOK();
  /* show onscreen and handle events like expose */
  window.show();
  run_main_loop_recursive();
  TOK();
  /* assert RapicornTestWidget rendering */
  uint seen_test = TestContainerImpl::seen_test_widgets();
  TASSERT (seen_test > old_seen_test); // may fail due to missing exposes (locked screens) needs PNG etc. backends
}
REGISTER_UITHREAD_TEST ("TestWidget/Test C++ Server Side GUI", test_cxx_server_gui);

static void
assertion_ok (const String &assertion)
{
  TPASS ("assertion_ok: %s", assertion.c_str());
}

static void
assertions_passed ()
{
  TOK();
}

static void
test_test_widget ()
{
  ApplicationImpl &app = ApplicationImpl::the(); // FIXME: use Application_RemoteHandle once C++ bindings are ready
  WindowIface &window_iface = *app.create_window ("alignment-test");
  TOK();
  WindowImpl &window = window_iface.impl();
  TestContainerImpl *twidget = window.interface<TestContainerImpl*>();
  TASSERT (twidget != NULL);
  twidget->sig_assertion_ok() += assertion_ok;
  twidget->sig_assertions_passed() += assertions_passed;
  twidget->fatal_asserts (ServerTests::server_test_widget_fatal_asserts);
  TOK();
  run_main_loop_recursive (false);
  /* close window (and exit main loop) after first expose */
  window.impl().sig_displayed() += [&window]() { window.close(); };
  /* verify and assert at least one RapicornTestWidget rendering */
  uint old_seen_test = TestContainerImpl::seen_test_widgets();
  window.show();
  run_main_loop_recursive();
  uint seen_test = TestContainerImpl::seen_test_widgets();
  TASSERT (seen_test > old_seen_test);
  /* test widget rendering also executed various assertions */
}
REGISTER_UITHREAD_TEST ("TestWidget/Test GUI assertions (alignment-test)", test_test_widget);

static void
ensure_ui_file()
{
  do_once
    {
      // first, load required ui files
      ApplicationImpl &app = ApplicationImpl::the();
      app.auto_load (Path::vpath_find ("testwidgets.xml"), program_argv0());
    }
}

static void
test_idl_test_widget ()
{
  ensure_ui_file();
  ApplicationImpl &app = ApplicationImpl::the(); // FIXME: use Application_RemoteHandle once C++ bindings are ready
  WindowIface &window_iface = *app.create_window ("test-widget-window");
  TOK();
  WindowImpl &window = window_iface.impl();
  IdlTestWidgetIface *twidgetp = window.interface<IdlTestWidgetIface*>();
  TASSERT (twidgetp != NULL);
  IdlTestWidgetIface &twidget = *twidgetp;
  twidget.bool_prop (0); TASSERT (twidget.bool_prop() == 0);
  twidget.bool_prop (1); TASSERT (twidget.bool_prop() == 1);
  twidget.bool_prop (0); TASSERT (twidget.bool_prop() == 0);
  twidget.int_prop (765760); TASSERT (twidget.int_prop() == 765760);
  twidget.int_prop (-211232); TASSERT (twidget.int_prop() == -211232);
  double f = 32.1231;
  twidget.float_prop (f); TASSERT (abs (twidget.float_prop() - f) < 1e-11); // assert double precision
  twidget.string_prop (""); TASSERT (twidget.string_prop() == "");
  twidget.string_prop ("5768trzg"); TASSERT (twidget.string_prop() == "5768trzg");
  twidget.string_prop ("äöü"); TASSERT (twidget.string_prop() == "äöü");
  twidget.enum_prop (TestEnum::VALUE1); TASSERT (twidget.enum_prop() == TestEnum::VALUE1);
  twidget.enum_prop (TestEnum::VALUE2); TASSERT (twidget.enum_prop() == TestEnum::VALUE2);
  twidget.enum_prop (TestEnum::VALUE3); TASSERT (twidget.enum_prop() == TestEnum::VALUE3);
  Requisition r (123, 765);
  twidget.record_prop (r); TASSERT (twidget.record_prop().width == r.width && twidget.record_prop().height == r.height);
  StringSeq sl;
  sl.push_back ("one");
  sl.push_back ("2");
  sl.push_back ("THREE");
  twidget.sequence_prop (sl); StringSeq sv = twidget.sequence_prop();
  TASSERT (sv.size() == sl.size()); TASSERT (sv[2] == "THREE");
  twidget.self_prop (NULL);
  TASSERT (twidget.self_prop() == NULL);
  auto twp = shared_ptr_cast<IdlTestWidgetIface> (twidgetp);
  twidget.self_prop (twp.get());
  TASSERT (twidget.self_prop().get() == twidgetp);
  twp.reset(); // destroys self_prop weak pointer
  twidget.self_prop (NULL);
  TASSERT (twidget.self_prop() == NULL);
  window.close();
}
REGISTER_UITHREAD_TEST ("TestWidget/Test TestWidget (test-widget-window)", test_idl_test_widget);

static void
test_complex_dialog ()
{
  ensure_ui_file();
  ApplicationImpl &app = ApplicationImpl::the(); // FIXME: use Application_RemoteHandle once C++ bindings are ready
  WindowIfaceP windowp = app.query_window ("/#"); // invalid path
  TASSERT (windowp == NULL);

  windowp = app.query_window ("#complex-dialog"); // not yet existing window
  TASSERT (windowp == NULL);
  WindowIface &window = *app.create_window ("complex-dialog");
  TOK();
  if (ServerTests::server_test_run_dialogs)
    {
      window.show();
      run_main_loop_recursive();
    }
  TOK();
  windowp = app.query_window ("#complex-dialog"); // now existing window
  TASSERT (windowp != NULL);

  WidgetIfaceP widget = window.query_selector_unique ("#complex-dialog");
  size_t count;
  TASSERT (widget != NULL);
  widget = window.query_selector_unique (".Widget#complex-dialog");
  TASSERT (widget != NULL);
  count = window.query_selector_all ("#complex-dialog .VBox .ScrollArea .Widget").size();
  TASSERT (count > 0);
  count = window.query_selector_all (":root .VBox .Widget").size();
  TASSERT (count > 0);
  widget = window.query_selector (":root .Alignment");
  TASSERT (widget != NULL);
  widget = window.query_selector (":root .Alignment .VBox");
  TASSERT (widget != NULL);
  widget = window.query_selector_unique (":root .Alignment .VBox #scroll-text");
  TASSERT (widget != NULL);
  widget = window.query_selector_unique (":root .Frame");
  TASSERT (widget == NULL); // not unique
  widget = window.query_selector_unique (":root .Frame! .Arrow#special-arrow");
  TASSERT (widget != NULL && shared_ptr_cast<FrameIface> (widget) != NULL);
  widget = window.query_selector_unique (":root .Button .Label");
  TASSERT (widget == NULL); // not unique
  widget = window.query_selector_unique (":root .Button .Label[markup-text*='Ok']");
  TASSERT (widget != NULL && shared_ptr_cast<LabelImpl> (widget) != NULL);
  widget = window.query_selector_unique (":root .Button! Label[markup-text*='Ok']");
  TASSERT (widget != NULL && shared_ptr_cast<ButtonAreaImpl> (widget) != NULL && shared_ptr_cast<LabelImpl> (widget) == NULL);
  widget = window.query_selector_unique ("/#"); // invalid path
  TASSERT (widget == NULL);
}
REGISTER_UITHREAD_TEST ("TestWidget/Test Comples Dialog (complex-dialog)", test_complex_dialog);

} // Anon
