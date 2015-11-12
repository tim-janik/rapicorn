// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_UITHREAD_HH__
#define __RAPICORN_UITHREAD_HH__

#include <rapicorn-core.hh>
#include <ui/application.hh>
#include <ui/arrangement.hh>
#include <ui/binding.hh>
#include <ui/buttons.hh>
#include <ui/container.hh>
#include <ui/cmdlib.hh>
#include <ui/evaluator.hh>
#include <ui/events.hh>
#include <ui/factory.hh>
#include <ui/heritage.hh>
#include <ui/image.hh>
#include <ui/widget.hh>
#include <ui/layoutcontainers.hh>
#include <ui/listarea.hh>
#include <ui/models.hh>
#include <ui/object.hh>
#include <ui/paintcontainers.hh>
#include <ui/paintwidgets.hh>
#include <ui/painter.hh>
#include <ui/primitives.hh>
#include <ui/region.hh>
#include <ui/scrollwidgets.hh>
#include <ui/selector.hh>
#include <ui/selob.hh>
#include <ui/sinfex.hh>
#include <ui/sizegroup.hh>
#include <ui/stock.hh>
#include <ui/style.hh>
#include <ui/table.hh>
#include <ui/text-editor.hh>
// conditional: #include <ui/text-pango.hh>
#include <ui/utilities.hh>
#include <ui/displaywindow.hh>
#include <ui/window.hh>

namespace Rapicorn {

void                 uithread_test_trigger (void (*) ());
MainLoopP            uithread_main_loop    ();
bool                 uithread_is_current   ();

/// Register a standard test function for execution in the ui-thread.
#define REGISTER_UITHREAD_TEST(name, ...)     static const Rapicorn::Test::RegisterTest \
  RAPICORN_CPP_PASTE2 (__Rapicorn_RegisterTest__line, __LINE__) ('T', name, __VA_ARGS__)

/// Register a slow test function for execution in the ui-thread during slow unit testing.
#define REGISTER_UITHREAD_SLOWTEST(name, ...) static const Rapicorn::Test::RegisterTest \
  RAPICORN_CPP_PASTE2 (__Rapicorn_RegisterTest__line, __LINE__) ('S', name, __VA_ARGS__)

} // Rapicorn

#endif  // __RAPICORN_UITHREAD_HH__
