// Licensed CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0
#include <rcore/testutils.hh>
#include <ui/uithread.hh>

namespace { // Anon
using namespace Rapicorn;

static void
test_window()
{
  ApplicationImpl &app = ApplicationImpl::the(); // FIXME: use Application_RemoteHandle once C++ bindings are ready

  // FIXME: this needs porting to use client API

  // create example widget
  WindowIface &window = *app.create_window ("Window");
  TOK();
  WidgetImpl &widget = window.impl();
  TOK();
  WindowImpl *wimpl = widget.interface<WindowImpl*>();
  TASSERT (wimpl != NULL);
  TOK();
}
REGISTER_UITHREAD_TEST ("Widgets/Test Window creation", test_window);

} // Anon
