/* Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html */
#include <rcore/testutils.hh>
#include <rapicorn.hh>
using namespace Rapicorn;

static Application * volatile smart_app_p = NULL;

static void
test_server_smart_handle (void)
{
  Application *ab = &app;
  Plic::FieldBuffer8 fb (4);
  fb.add_object (uint64 ((BaseObject*) ab));
  Plic::Coupler &c = *rope_thread_coupler();
  c.reader.reset (fb);
  Application_SmartHandle sh (c, c.reader);
  Application &shab = *sh;
  assert (ab == &shab);
  assert (ab == sh.operator->());
}
REGISTER_TEST ("Server/Smart Handle", test_server_smart_handle);
