/* Rapicorn Test
 * Copyright (C) 2007 Tim Janik
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
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

namespace {
using namespace Rapicorn;

extern "C" int
main (int   argc,
      char *argv[])
{
  rapicorn_init_test (&argc, argv);

  /* initialize Rapicorn for X11 backend with application name */
  app.init_with_x11 (&argc, &argv, "FactoryTest");

  /* find and load GUI definitions relative to argv[0] */
  String factory_xml = "factory.xml";
  app.auto_load ("RapicornTest",                        // namespace domain,
                 Path::vpath_find (factory_xml),        // GUI file name
                 argv[0]);
  ItemImpl *item;
  TestContainer *titem;

  TSTART ("Factory Calls");
  Wind0wIface &testwin = *app.create_wind0w ("test-TestItemL2");
  testwin.show();
  while (RapicornTester::loops_pending()) /* complete showing */
    RapicornTester::loops_dispatch (false);
  TOK();
  WindowImpl *window = &testwin.impl();
  item = window->find_item ("TestItemL2");
  TASSERT (item != NULL);
  titem = dynamic_cast<TestContainer*> (item);
  TASSERT (titem != NULL);
  if (0)
    {
      printout ("\n");
      printout ("TestContainer::accu:%s\n", titem->accu().c_str());
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
  TDONE();

  return 0;
}

} // anon
