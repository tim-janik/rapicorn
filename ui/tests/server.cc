/* Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html */
#include <rcore/testutils.hh>
#include <rapicorn.hh>
using namespace Rapicorn;

static void
test_server_smart_handle (void)
{
  ApplicationImpl &app = ApplicationImpl::the(); // FIXME: use Application_SmartHandle once C++ bindings are ready
  ApplicationIface *ab = &app;
  Plic::FieldBuffer8 fb (4);
  fb.add_object (uint64 ((BaseObject*) ab));
  Plic::Coupler &c = *rope_thread_coupler();
  c.reader.reset (fb);
  Application_SmartHandle sh (c, c.reader);
  ApplicationIface &shab = *sh;
  assert (ab == &shab);
  assert (ab == sh.operator->());
}
REGISTER_TEST ("Server/Smart Handle", test_server_smart_handle);
