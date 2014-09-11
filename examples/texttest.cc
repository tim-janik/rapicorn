// Licensed CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0
#include <rapicorn.hh>

namespace {
using namespace Rapicorn;

extern "C" int
main (int   argc,
      char *argv[])
{
  // initialize Rapicorn
  ApplicationH app = init_app (__PRETTY_FILE__, &argc, argv);

  // find and load GUI definitions relative to argv[0]
  app.auto_load ("RapicornExamples", "texttest.xml", argv[0]);

  // create and show main window
  WindowH window = app.create_window ("RapicornExamples:main-shell");
  window.show();

  // run event loops while windows are on screen
  return app.run_and_exit();
}

} // anon
