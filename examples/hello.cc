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
#include <rapicorn.hh>

namespace {
using namespace Rapicorn;

static bool
handle_commands (Wind0wIface      &wind0w,
                 const String     &command,
                 const StringList &args)
{
  if (command == "close")
    wind0w.close();
  else
    printout ("%s(): custom command: %s(%s)\n", __func__, command.c_str(), string_join (",", args).c_str());
  return true;
}

extern "C" int
main (int   argc,
      char *argv[])
{
  /* initialize Rapicorn for X11 backend with application name */
  Application_SmartHandle smApp = init_app ("HelloWorld", &argc, argv);
  ApplicationImpl &app = ApplicationImpl::the(); // FIXME: use Application_SmartHandle once C++ bindings are ready

  /* find and load GUI definitions relative to argv[0] */
  app.auto_load ("RapicornTest",        // namespace domain
                 "hello.xml",           // GUI file name
                 argv[0]);

  /* create main wind0w */
  Wind0wIface &wind0w = *app.create_wind0w ("main-wind0w");

  /* connect custom callback to handle UI commands */
  wind0w.sig_commands += handle_commands;

  /* display wind0w on screen */
  wind0w.show();

  /* run event loops while wind0ws are on screen */
  app.execute_loops();

  return 0;
}

} // anon
