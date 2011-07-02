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
#include "../rcore/rsvg/svg.hh"       // FIXME

#include <string.h>
#include <stdlib.h>

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
  printout ("This tool will read the GUI description file listed on the command\n");
  printout ("line, look for a dialog named 'test-dialog' and show it.\n");
  printout ("\n");
  printout ("Options:\n");
  printout ("  --parse-test                  Parse GuiFile.xml and exit.\n");
  printout ("  -l <uilib>                    Load ''uilib'' upon startup.\n");
  printout ("  -x                            Enable auto-exit after first expose.\n");
  printout ("  --list                        List parsed definitions.\n");
  printout ("  --test-dump                   Dump test stream after first expose.\n");
  printout ("  --test-matched-node PATTERN   Filter nodes in test dumps.\n");
  printout ("  -h, --help                    Display this help and exit.\n");
  printout ("  -v, --version                 Display version and exit.\n");
}

static bool parse_test = false;
static bool auto_exit = false;
static bool list_definitions = false;

static bool test_dump = false;
static vector<String> test_dump_matched_nodes;

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
      else if (strcmp (argv[i], "-x") == 0)
        {
          auto_exit = true;
          argv[i] = NULL;
        }
      else if (strcmp (argv[i], "-l") == 0 ||
               strncmp ("-l=", argv[i], 2) == 0)
        {
          const char *v = NULL, *equal = argv[i] + 2;
          if (*equal == '=')
            v = equal + 1;
          else if (i + 1 < argc)
            {
              argv[i++] = NULL;
              v = argv[i];
            }
          if (v)
            Svg::Library::add_library (v);
          argv[i] = NULL;
        }
      else if (strcmp (argv[i], "--list") == 0)
        {
          list_definitions = true;
          argv[i] = NULL;
        }
      else if (strcmp (argv[i], "--test-dump") == 0)
        {
          test_dump = true;
          argv[i] = NULL;
        }
      else  if (strcmp ("--test-matched-node", argv[i]) == 0 ||
                strncmp ("--test-matched-node=", argv[i], 20) == 0)
        {
          char *v = NULL, *equal = argv[i] + 19;
          if (*equal == '=')
            v = equal + 1;
          else if (i + 1 < argc)
            {
              argv[i++] = NULL;
              v = argv[i];
            }
          if (v)
            test_dump_matched_nodes.push_back (v);
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

static void
window_test_dump (WindowImpl &window)
{
  if (!test_dump)
    return;
  TestStream *tstream = TestStream::create_test_stream();
  for (uint i = 0; i < test_dump_matched_nodes.size(); i++)
    tstream->filter_matched_nodes (test_dump_matched_nodes[i]);
  window.get_test_dump (*tstream);
  printout ("%s", tstream->string().c_str());
  delete tstream;
  test_dump = false;
}

extern "C" int
main (int   argc,
      char *argv[])
{
  /* initialize Rapicorn and its X11 (Gtk+) backend */
  app.init_with_x11 (&argc, &argv, "Rapidrun"); // acquires Rapicorn mutex
  parse_args (&argc, &argv);
  if (argc != 2)
    help_usage (true);

  /* find GUI definition file, relative to CWD */
  String filename = app.auto_path (argv[1], ".");

  /* load GUI definitions, fancy version of app.auto_load() */
  vector<String> definitions;
  int err = Factory::parse_file ("RapicornTest", filename, "", &definitions);
  if (err)
    fatal ("failed to load \"%s\": %s", filename.c_str(), string_from_errno (err).c_str());

  /* print definitions */
  String dialog;
  for (uint i = 0; i < definitions.size(); i++)
    {
      bool iswindow = Factory::item_definition_is_window (definitions[i]);
      if (list_definitions)
        printout ("%s%s\n", definitions[i].c_str(), iswindow ? " (window)" : "");
      if (dialog == "" && iswindow)
        dialog = definitions[i];
    }
  if (list_definitions)
    return 0;

  /* bail out without any dialogs */
  if (dialog == "")
    {
      printerr ("%s: no dialog definitions\n", filename.c_str());
      return 1;
    }

  /* create window item */
  Wind0wIface &wind0w = *app.create_wind0w (dialog);

  /* hook up test-dump handler */
  if (test_dump)
    wind0w.impl().sig_displayed += window_test_dump;

  /* hook up auto-exit handler */
  if (auto_exit)
    wind0w.impl().enable_auto_close();

  /* show wind0w and process events */
  wind0w.show();
  app.execute_loops();

  return 0;
}

} // anon
