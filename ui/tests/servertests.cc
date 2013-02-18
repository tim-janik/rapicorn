/* Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html */
#include "servertests.hh"
#include <rapicorn.hh>

using namespace Rapicorn;

namespace ServerTests {

bool server_test_widget_fatal_asserts = true;
bool server_test_run_dialogs = false;

} // ServerTests

extern "C" int
main (int   argc,
      char *argv[])
{
  init_test_app (__SOURCE_COMPONENT__, &argc, argv);

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
