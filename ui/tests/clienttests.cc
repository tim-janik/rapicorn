// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include <rapicorn.hh>
#include <rcore/testutils.hh>

namespace { // Anon
using namespace Rapicorn;

static Application app;
static Window
create_plain_window ()
{
  Window window = app.create_window ("Window");
  TASSERT (window._is_null() == false);
  return window;
}

static void
test_item_usage()
{
  Window window = create_plain_window();
  Item item = window;
  TASSERT (item._is_null() == false);
  Container c = Container::downcast (item);
  TASSERT (c._is_null() == false);
  TASSERT (Window::downcast (c)._is_null() == false);
  window.close();
  TASSERT (Item::downcast (app)._is_null() == true);
  TASSERT (Container::downcast (app)._is_null() == true);
  TASSERT (Window::downcast (app)._is_null() == true);
  const char *testname = "3e3b0d44-ded6-4fc5-a134-c16569c23d96";
  String name = item.name();
  TASSERT (name != testname);
  item.name (testname);
  name = item.name();
  TASSERT (name == testname);
  window.name ("Test-Name");
  TASSERT ("Test-Name" == item.name());
  TASSERT (c.component<Window> (".Window")._is_null() == false);
  TASSERT (c.component<Window> (":root")._is_null() == false);
  TASSERT (window.component<Container> (".Window")._is_null() == false);
  TASSERT (item.component<ButtonArea> (".Window")._is_null() == true);
  TASSERT (item.component<Item> (".Window")._is_null() == false);
  TASSERT (item.component<Item> ("FrobodoNotHere")._is_null() == true);
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
