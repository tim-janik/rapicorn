/* Rapicorn - Experimental UI Toolkit
 * Copyright (C) 2007 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include <rapicorn.hh>

namespace {
using namespace Rapicorn;

static void
help_usage (bool usage_error)
{
  const char *usage = "Usage: rapidrun [OPTIONS] <GuiFile.xml>";
  if (usage_error)
    {
      printerr ("%s\n", usage);
      printerr ("Try 'rapidrun --help' for more information.\n");
      exit (1);
    }
  printout ("%s\n", usage);
  /*         12345678901234567890123456789012345678901234567890123456789012345678901234567890 */
  printout ("Show Rapicorn dialogs defined in GuiFile.xml.\n");
  printout ("\n");
  printout ("Options:\n");
  printout ("  --parse-test                  Parse GuiFile.xml and exit.\n");
  printout ("  -h, --help                    Display this help and exit.\n");
  printout ("  -v, --version                 Display version and exit.\n");
}

static bool parse_test = false;

static void
parse_args (int    *argc_p,
            char ***argv_p)
{
  char **argv = *argv_p;
  uint argc = *argc_p;

  for (uint i = 1; i < argc; i++)
    {
      if (strcmp (argv[i], "--parse-test") == 0)
        {
          parse_test = true;
          argv[i] = NULL;
        }
      else if (strcmp (argv[i], "--help") == 0 || strcmp (argv[i], "-h") == 0)
        {
          help_usage (false);
          exit (0);
        }
      else if (strcmp (argv[i], "--version") == 0 || strcmp (argv[i], "-v") == 0)
        {
          printout ("rapidrun (Rapicorn utilities) %s\n", RAPICORN_VERSION);
          printout ("Copyright (C) 2007 Tim Janik.\n");
          printout ("This is free software and comes with ABSOLUTELY NO WARRANTY; see\n");
          printout ("the source for copying conditions. Sources, examples and contact\n");
          printout ("information are available at http://rapicorn.org/.\n");
          exit (0);
        }
    }

  uint e = 1;
  for (uint i = 1; i < argc; i++)
    if (argv[i])
      {
        argv[e++] = argv[i];
        if (i >= e)
          argv[i] = NULL;
      }
  *argc_p = e;
}

extern "C" int
main (int   argc,
      char *argv[])
{
  /* initialize Rapicorn and its X11 (Gtk+) backend */
  Application::init_with_x11 (&argc, &argv, "Rapidrun"); // acquires Rapicorn mutex
  parse_args (&argc, &argv);
  if (argc != 2)
    help_usage (true);

  /* load GUI definition file, relative to argv[0] */
  Application::load_gui ("RapicornTest", argv[1], argv[0]);

  /* create root item */
  Window window = Application::create_window ("test-dialog");

  /* show window and process events */
  window.show();
  Application::execute_loops();

  return 0;
}

} // anon
