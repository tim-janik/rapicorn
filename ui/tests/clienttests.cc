// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
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
  const char *testname = "3e3b0d44-ded6-4fc5-a134-c16569c23d96";
  String name = widget.name();
  TASSERT (name != testname);
  widget.name (testname);
  name = widget.name();
  TASSERT (name == testname);
  window.name ("Test-Name");
  TASSERT ("Test-Name" == widget.name());
  TASSERT (c.component<WindowH> (".Window") != NULL);
  TASSERT (c.component<WindowH> (":root") != NULL);
  TASSERT (window.component<ContainerH> (".Window") != NULL);
  TASSERT (widget.component<ButtonAreaH> (".Window") == NULL);
  TASSERT (widget.component<WidgetH> (".Window") != NULL);
  TASSERT (widget.component<WidgetH> ("FrobodoNotHere") == NULL);
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
