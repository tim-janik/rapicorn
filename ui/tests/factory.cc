// This work is provided "as is"; see: http://rapicorn.org/LICENSE-AS-IS
#include <rcore/testutils.hh>
#include <rapicorn.hh>
#include <ui/testitems.hh>

namespace { // Anon
using namespace Rapicorn;

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

static void
test_factory ()
{
  TOK();
  ApplicationImpl &app = ApplicationImpl::the(); // FIXME: use Application_SmartHandle once C++ bindings are ready

  /* find and load GUI definitions relative to argv[0] */
  String factory_xml = "factory.xml";
  app.auto_load ("RapicornTest",                        // namespace domain,
                 Path::vpath_find (factory_xml),        // GUI file name
                 program_file());
  TOK();
  ItemImpl *item;
  TestContainer *titem;
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
  TOK();
}
REGISTER_UITHREAD_TEST ("Factory/Test Item Factory", test_factory);

} // anon
