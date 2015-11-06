/* This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 */
#ifndef __RAPICORN_SERVERTESTS_HH__
#define __RAPICORN_SERVERTESTS_HH__

#include <ui/uithread.hh>
#include <rapicorn.hh> // needed for init_test_app()
#include <rcore/testutils.hh>

namespace ServerTests {

extern bool server_test_widget_fatal_asserts;
extern bool server_test_run_dialogs;
extern void sinfex_shell_wrapper (void);

} // ServerTestsServerTests

#endif  /* __RAPICORN_SERVERTESTS_HH__ */
