// This work is provided "as is"; see: http://rapicorn.org/LICENSE-AS-IS
#include <rapicorn.hh>

namespace {
using namespace Rapicorn;

extern "C" int
main (int   argc,
      char *argv[])
{
  // initialize Rapicorn
  Application app = init_app (String ("Rapicorn/Examples/") + RAPICORN__FILE__, &argc, argv);

  // find and load GUI definitions relative to argv[0]
  app.auto_load ("RapicornExamples", "texttest.xml", argv[0]);

  // create and show main window
  Window window = app.create_window ("main-shell");
  window.show();

  // run event loops while windows are on screen
  return app.run_and_exit();
}

} // anon
