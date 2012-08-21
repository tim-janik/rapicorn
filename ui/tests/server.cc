/* Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html */
#include <rcore/testutils.hh>
#include <ui/uithread.hh>
using namespace Rapicorn;

static void
test_server_smart_handle()
{
  ApplicationImpl &app = ApplicationImpl::the(); // FIXME: use Application_SmartHandle once C++ bindings are ready
  ApplicationIface *ab = &app;
  Aida::FieldBuffer8 fb (4);
  fb.add_object (uint64 ((BaseObject*) ab));
  // FIXME: Aida::Coupler &c = *rope_thread_coupler();
  // c.reader.reset (fb);
  ApplicationImpl *am = dynamic_cast<ApplicationImpl*> (ab);
  assert (am == &app);
  ApplicationIface *ai = am;
  assert (ai == ab);
}
REGISTER_UITHREAD_TEST ("Server/Smart Handle", test_server_smart_handle);

static void
test_stock_resources()
{
  String s;
  s = Stock::stock_label ("broken-image");
  TASSERT (s.empty() == false);
  s = Stock::stock_string ("broken-image", "image");
  TASSERT (s.empty() == false);
  Blob b = Stock::stock_image ("broken-image");
  TASSERT (b && b.size() > 16);
  b = Stock::stock_image (" .no.. +such+ -image- ~hCZ75jv27j");
  TASSERT (!b && errno != 0);
}
REGISTER_UITHREAD_TEST ("Server/Stock Resources", test_stock_resources);
