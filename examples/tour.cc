// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include <rapicorn.hh>

namespace {
using namespace Rapicorn;

static bool
custom_commands (WindowH &window, const String &command, const StringSeq &args)
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

#include "../ui/tests/testpixs.c" // alpha_rle alpha_raw rgb_rle rgb_raw

extern "C" int
main (int   argc,
      char *argv[])
{
  // initialize Rapicorn
  ApplicationH app = init_app (__PRETTY_FILE__, &argc, argv);

#if 0 // FIMXE: test builtin images
  ApplicationIface::pixstream ("testimage-alpha-rle", alpha_rle);
  ApplicationIface::pixstream ("testimage-alpha-raw", alpha_raw);
  ApplicationIface::pixstream ("testimage-rgb-rle", rgb_rle);
  ApplicationIface::pixstream ("testimage-rgb-raw", rgb_raw);
#endif

  // load GUI definition file, relative to argv[0]
  app.auto_load ("tour.xml", argv[0]);

  // create window, handle commands
  WindowH window = app.create_window ("tour-dialog");
  window.sig_commands() += [&window] (const String &command, const StringSeq &args) -> bool { return custom_commands (window, command, args); };

  // display window and enter main event loop
  window.show();

  // automated unit testing
  if (argc > 1 && argv[1] == String ("--test-automation"))
    {
      bool b;
      printout ("# %s %s\n", argv[0], argv[1]);
      b = window.synthesize_enter();    // enter window to allow input events
      assert (b);
      WidgetH button;
      button = window.query_selector_unique ("#TDumpButton");
      assert (button);                  // ensure we have the test dump button
      window.synthesize_click (button, 1);
      while (app.iterate (false));      // process pending events and signals (calls into custom_commands)
      button = window.query_selector_unique ("#CloseButton");
      assert (button);                  // ensure we have the close button
      window.synthesize_click (button, 1);
      assert (window.closed() == true);
      window.synthesize_leave();        // click and leave
    }

  return app.run_and_exit();
}

} // anon
