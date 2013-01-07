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
  TASSERT (window._is_null() == false);
  return window;
}

static void
test_item_usage()
{
  WindowH window = create_plain_window();
  ItemH item = window;
  TASSERT (item._is_null() == false);
  ContainerH c = ContainerH::down_cast (item);
  TASSERT (c._is_null() == false);
  TASSERT (WindowH::down_cast (c)._is_null() == false);
  window.close();
  TASSERT (ItemH::down_cast (app)._is_null() == true);
  TASSERT (ContainerH::down_cast (app)._is_null() == true);
  TASSERT (WindowH::down_cast (app)._is_null() == true);
  const char *testname = "3e3b0d44-ded6-4fc5-a134-c16569c23d96";
  String name = item.name();
  TASSERT (name != testname);
  item.name (testname);
  name = item.name();
  TASSERT (name == testname);
  window.name ("Test-Name");
  TASSERT ("Test-Name" == item.name());
  TASSERT (c.component<WindowH> (".Window")._is_null() == false);
  TASSERT (c.component<WindowH> (":root")._is_null() == false);
  TASSERT (window.component<ContainerH> (".Window")._is_null() == false);
  TASSERT (item.component<ButtonAreaH> (".Window")._is_null() == true);
  TASSERT (item.component<ItemH> (".Window")._is_null() == false);
  TASSERT (item.component<ItemH> ("FrobodoNotHere")._is_null() == true);
}
REGISTER_TEST ("Client/Basic Item Usage", test_item_usage);

} // Anon

extern "C" int
main (int   argc,
      char *argv[])
{
  app = init_test_app (__SOURCE_COMPONENT__, &argc, argv);

  return Test::run();
}
