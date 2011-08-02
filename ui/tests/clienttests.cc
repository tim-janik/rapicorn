// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include <rapicorn.hh>
#include <rcore/testutils.hh>

namespace { // Anon
using namespace Rapicorn;

static Application app;
static Wind0w
create_plain_window ()
{
  Wind0w wind0w = app.create_wind0w ("Window");
  TASSERT (wind0w._is_null() == false);
  return wind0w;
}

static void
test_item_casting()
{
  Wind0w wind0w = create_plain_window();
  Item item = wind0w;
  TASSERT (item._is_null() == false);
  Container c = Container::_cast (item);
  TASSERT (c._is_null() == false);
  TASSERT (Wind0w::_cast (c)._is_null() == false);
  wind0w.close();
  TASSERT (Item::_cast (app)._is_null() == true);
  TASSERT (Container::_cast (app)._is_null() == true);
  TASSERT (Wind0w::_cast (app)._is_null() == true);
}
REGISTER_TEST ("Client/Item Casting", test_item_casting);

} // Anon

extern "C" int
main (int   argc,
      char *argv[])
{
  app = init_test_app (String ("Rapicorn/") + RAPICORN__FILE__, &argc, argv);

  return Test::run();
}
