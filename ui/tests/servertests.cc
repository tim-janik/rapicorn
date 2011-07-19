/* Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html */
#include <rcore/testutils.hh>
#include <rapicorn.hh>

using namespace Rapicorn;

extern "C" int
main (int   argc,
      char *argv[])
{
  init_test_app (String ("Rapicorn/") + RAPICORN__FILE__, &argc, argv);
  return Test::run();
}
