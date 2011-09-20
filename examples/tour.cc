// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include <rapicorn.hh>

namespace {
using namespace Rapicorn;

static bool
custom_commands (Window           &window,
                 const String     &command,
                 const StringList &args)
{
  if (command == "testdump")
    {
      // tstream->filter_matched_nodes ("Button");
      String s = window.test_dump();
      printout ("%s", s.c_str());
    }
  else
    printout ("%s(): custom command: %s(%s) (Window: %s)\n", __func__,
              command.c_str(), string_join (",", args).c_str(), window.name().c_str());
  return true;
}

#include "../rcore/tests/testpixs.c" // alpha_rle alpha_raw rgb_rle rgb_raw

extern "C" int
main (int   argc,
      char *argv[])
{
  // initialize Rapicorn
  Application app = init_app (String ("Rapicorn/") + RAPICORN__FILE__, &argc, argv);

#if 0 // FIMXE: test builtin images
  ApplicationIface::pixstream ("testimage-alpha-rle", alpha_rle);
  ApplicationIface::pixstream ("testimage-alpha-raw", alpha_raw);
  ApplicationIface::pixstream ("testimage-rgb-rle", rgb_rle);
  ApplicationIface::pixstream ("testimage-rgb-raw", rgb_raw);
#endif

  // load GUI definition file, relative to argv[0]
  app.auto_load ("RapicornTest", "tour.xml", argv[0]);

  // create window, handle commands
  Window window = app.create_window ("tour-dialog");
  window.sig_commands() += custom_commands;

  // display window and enter main event loop
  window.show();
  return app.run_and_exit();
}

} // anon
