/* Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html */
#include "servertests.hh"

using namespace Rapicorn;

namespace ServerTests {

bool server_test_item_fatal_asserts = true;
bool server_test_run_dialogs = false;

} // ServerTests

extern "C" int
main (int   argc,
      char *argv[])
{
  init_test_app (String ("Rapicorn/") + RAPICORN__FILE__, &argc, argv);

  for (int i = 0; i < argc; i++)
    if (String (argv[i]) == "--non-fatal")
      ServerTests::server_test_item_fatal_asserts = false;
    else if (String (argv[i]) == "--run")
      ServerTests::server_test_run_dialogs = true;

  return Test::run();
}
