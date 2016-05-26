// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include <rapicorn.hh>
#include <rcore/testutils.hh>

namespace { // Anon
using namespace Rapicorn;

static ApplicationH app;
static WindowH
create_plain_window ()
{
  WindowH window = app.create_window ("Window");
  TASSERT (window != NULL);
  return window;
}

struct Foo               { double f; };
struct ExtendedFoo : Foo { bool operator== (const ExtendedFoo &ef) const { return f == ef.f; } };

static void
test_widget_usage()
{
  WindowH window = create_plain_window();
  WidgetH widget = window;
  TASSERT (widget != NULL);
  ContainerH c = ContainerH::down_cast (widget);
  TASSERT (c != NULL);
  TASSERT (WindowH::down_cast (c) != NULL);
  window.close();
  TASSERT (WidgetH::down_cast (app) == NULL);
  TASSERT (ContainerH::down_cast (app) == NULL);
  TASSERT (WindowH::down_cast (app) == NULL);
  const char *testid = "3e3b0d44-ded6-4fc5-a134-c16569c23d96";
  String id = widget.id();
  TASSERT (id != testid);
  widget.id (testid);
  id = widget.id();
  TASSERT (id == testid);
  window.id ("Test-Name");
  TASSERT ("Test-Name" == widget.id());
  TASSERT (c.component<WindowH> (".Window") != NULL);
  TASSERT (c.component<WindowH> (":root") != NULL);
  TASSERT (window.component<ContainerH> (".Window") != NULL);
  TASSERT (widget.component<ButtonAreaH> (".Window") == NULL);
  TASSERT (widget.component<WidgetH> (".Window") != NULL);
  TASSERT (widget.component<WidgetH> ("FrobodoNotHere") == NULL);
  // user_data / any
  Any any1 (7);
  widget.set_user_data ("test-int", any1);
  Any any2 = widget.get_user_data ("test-int");
  TASSERT (any1 == any2);
  any2 = widget.get_user_data ("nothere");
  TASSERT (any1 != any2);
  widget.set_user_data ("test-float", Any (0.75));
  any2 = widget.get_user_data ("test-float");
  TASSERT (any2.get<double>() == 0.75);
  any2 = widget.get_user_data ("test-int");
  TASSERT (any2.get<int64>() == 7);
  widget.set_user_data ("test-string", Any ("Wocks"));
  any1 = widget.get_user_data ("test-string");
  TASSERT (any1 == Any ("Wocks"));
  any2.set (any1);
  TASSERT (any2.kind() == Aida::ANY);
  widget.set_user_data ("test-any", any2);
  any1 = widget.get_user_data ("test-any");
  TASSERT (any1.kind() == Aida::ANY);
  TASSERT (any1.as_any().to_string() == "Wocks");
  // INSTANCE as user_data
  any1.set (widget);
  TASSERT (widget.__aida_orbid__() == any1.get<WidgetH>().__aida_orbid__());
  widget.set_user_data ("test-user-widget", any1);
  any2 = widget.get_user_data ("test-user-widget");
  TASSERT (any1 == any2);
  WidgetH extracted_widget;
  extracted_widget = any2.get<WidgetH>();
  TASSERT (extracted_widget.__aida_orbid__() == any2.get<WidgetH>().__aida_orbid__());
  TASSERT (widget == extracted_widget);
  widget.set_user_data ("test-user-widget", Any()); // delete
  any2 = widget.get_user_data ("test-user-widget");
  TASSERT (any1 != any2);
  // LOCAL as user_data (not comparable)
  Foo f;
  f.f = -0.25;
  any1.set (f);
  widget.set_user_data ("test-foo", any1);
  any2 = widget.get_user_data ("test-foo");
  TASSERT (f.f == any2.get<Foo>().f);
  TASSERT (any1 != any2); // Foo is not comparable
  // LOCAL as user_data (comparable)
  ExtendedFoo e;
  e.f = 1.0;
  any1.set (e);
  widget.set_user_data ("test-efoo", any1);
  any2 = widget.get_user_data ("test-efoo");
  TASSERT (any1 == any2); // ExtendedFoo is comparable
  TASSERT (any1.get<ExtendedFoo>().f == 1.0);
  TASSERT (any2.get<ExtendedFoo>().f == 1.0);
  TASSERT (any2.get<Foo>().f == 0.0); // invalid type, so Foo() is returned
}
REGISTER_TEST ("Client/Basic Widget Usage", test_widget_usage);

} // Anon

extern "C" int
main (int   argc,
      char *argv[])
{
  app = init_test_app (__PRETTY_FILE__, &argc, argv);

  return Test::run();
}
