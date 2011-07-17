// This work is provided "as is"; see: http://rapicorn.org/LICENSE-AS-IS
#include <rcore/testutils.hh>
#include <rapicorn.hh>

namespace { // Anon
using namespace Rapicorn;

static void
test_window()
{
  ApplicationImpl &app = ApplicationImpl::the(); // FIXME: use Application_SmartHandle once C++ bindings are ready

  // FIXME: this needs porting to use client API

  // create example item
  Wind0wIface &wind0w = *app.create_wind0w ("Window");
  TOK();
  ItemImpl &item = wind0w.impl();
  TOK();
  WindowImpl &window = item.interface<WindowImpl&>();
  TASSERT (&window != NULL);
  TOK();
}
REGISTER_UITHREAD_TEST ("Items/Test Window creation", test_window);

} // Anon

extern "C" int
main (int   argc,
      char *argv[])
{
  init_test_app (String ("Rapicorn/") + RAPICORN__FILE__, &argc, argv);
  return Test::run();
}
