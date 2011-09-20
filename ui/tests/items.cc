// This work is provided "as is"; see: http://rapicorn.org/LICENSE-AS-IS
#include <rcore/testutils.hh>
#include <ui/uithread.hh>

namespace { // Anon
using namespace Rapicorn;

static void
test_window()
{
  ApplicationImpl &app = ApplicationImpl::the(); // FIXME: use Application_SmartHandle once C++ bindings are ready

  // FIXME: this needs porting to use client API

  // create example item
  WindowIface &window = *app.create_window ("Window");
  TOK();
  ItemImpl &item = window.impl();
  TOK();
  WindowImpl &wimpl = item.interface<WindowImpl&>();
  TASSERT (&wimpl != NULL);
  TOK();
}
REGISTER_UITHREAD_TEST ("Items/Test Window creation", test_window);

} // Anon
