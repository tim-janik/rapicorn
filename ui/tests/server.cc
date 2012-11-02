/* Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html */
#include <rcore/testutils.hh>
#include <ui/uithread.hh>
using namespace Rapicorn;

static void
test_server_smart_handle (void)
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
