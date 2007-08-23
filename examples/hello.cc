/* Rapicorn Example
 * Copyright (C) 2007 Tim Janik
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */
#include <rapicorn/rapicorn.hh>

namespace {
using namespace Rapicorn;

static bool
handle_commands (Window       &window,
                 const String &command,
                 const String &args)
{
  if (command == "close")
    window.close();
  else
    printout ("%s(): custom command: %s(%s)\n", __func__, command.c_str(), args.c_str());
  return true;
}

extern "C" int
main (int   argc,
      char *argv[])
{
  /* initialize Rapicorn for X11 backend with application name */
  Application::init_with_x11 (&argc, &argv, "HelloWorld");

  /* find and load GUI definitions relative to argv[0] */
  Application::load_gui ("DummyTranslation",    // i18n_domain,
                         "hello.xml",           // GUI file name
                         argv[0]);

  /* create main window */
  Window window = Application::create_window ("main-window");

  /* connect custom callback to handle UI commands */
  window.commands += handle_commands;

  /* display window on screen */
  window.show();

  /* run event loops while windows are on screen */
  Application::execute_loops();

  return 0;
}

} // anon
