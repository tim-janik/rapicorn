/* This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 */
#include "servertests.hh"

using namespace Rapicorn;

namespace ServerTests {

bool server_test_widget_fatal_asserts = true;
bool server_test_run_dialogs = false;

} // ServerTests

extern "C" int
main (int   argc,
      char *argv[])
{
  init_test_app (__PRETTY_FILE__, &argc, argv);

  for (int i = 0; i < argc; i++)
    if (String (argv[i]) == "--non-fatal")
      ServerTests::server_test_widget_fatal_asserts = false;
    else if (String (argv[i]) == "--run")
      ServerTests::server_test_run_dialogs = true;
    else if (String (argv[i]) == "--shell")
      {
        ServerTests::sinfex_shell_wrapper();
        return 0;
      }

  return Test::run();
}
