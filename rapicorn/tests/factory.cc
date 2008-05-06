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
#include <rapicorn-core/rapicorntests.h>
#include <rapicorn/rapicorn.hh>
#include <rapicorn/testitems.hh>


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
  rapicorn_init_test (&argc, &argv);

  /* initialize Rapicorn for X11 backend with application name */
  Application::init_with_x11 (&argc, &argv, "FactoryTest");

  /* find and load GUI definitions relative to argv[0] */
  Application::auto_load ("DummyTranslation",   // i18n_domain,
                          "factory.xml",        // GUI file name
                          argv[0]);
  Root *root;
  Item *item;
  TestItem *titem;

  TSTART ("Factory Calls");
  Window testwin = Application::create_window ("test-TestItemL2");
  root = &testwin.root();
  testwin.show();
  while (RapicornTester::loops_pending()) /* complete showing */
    RapicornTester::loops_dispatch (false);
  TOK();
  item = root->find_item ("TestItemL2");
  TASSERT (item != NULL);
  titem = dynamic_cast<TestItem*> (item);
  TASSERT (titem != NULL);
  if (0)
    {
      printout ("\n");
      printout ("TestItem::accu:%s\n", titem->accu().c_str());
      printout ("TestItem::accu_history: %s\n", titem->accu_history().c_str());
    }
  TASSERT (titem->accu_history() == "L0L1L2Instance");
  TOK();
  testwin.close();
  TDONE();

  return 0;
}

} // anon
