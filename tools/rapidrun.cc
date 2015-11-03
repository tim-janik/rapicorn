// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
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
      exit_app (1);
    }
  printout ("%s\n", usage);
  /*         12345678901234567890123456789012345678901234567890123456789012345678901234567890 */
  printout ("Show Rapicorn dialogs defined in GuiFile.xml.\n");
  printout ("This tool will read the GUI description file listed on the command\n");
  printout ("line, look for a dialog window and show it.\n");
  printout ("\n");
  printout ("Options:\n");
  printout ("  --parse-test                  Parse GuiFile.xml and exit.\n");
  printout ("  -x                            Enable auto-exit after first expose.\n");
  printout ("  --list                        List parsed definitions.\n");
  printout ("  --fatal-warnings              Turn criticals/warnings into fatal conditions.\n");
  printout ("  --snapshot pngname            Dump a snapshot to <pngname>.\n");
  printout ("  --test-dump                   Dump test stream after first expose.\n");
  printout ("  --test-matched-node PATTERN   Filter nodes in test dumps.\n");
  printout ("  -h, --help                    Display this help and exit.\n");
  printout ("  -v, --version                 Display version and exit.\n");
}

static bool parse_test = false;
static bool auto_exit = false;
static bool list_definitions = false;
static String dump_snapshot = "";
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
      else if (strcmp (argv[i], "--list") == 0)
        {
          list_definitions = true;
          argv[i] = NULL;
        }
      else  if (strcmp ("--snapshot", argv[i]) == 0 ||
                strncmp ("--snapshot=", argv[i], 11) == 0)
        {
          char *v = NULL, *equal = argv[i] + 10;
          if (*equal == '=')
            v = equal + 1;
          else if (i + 1 < argc)
            {
              argv[i++] = NULL;
              v = argv[i];
            }
          if (v)
            dump_snapshot = v;
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
          exit_app (0);
        }
      else if (strcmp (argv[i], "--version") == 0 || strcmp (argv[i], "-v") == 0)
        {
          printout ("rapidrun (Rapicorn utilities) %s (Build ID: %s)\n", rapicorn_version().c_str(), rapicorn_buildid().c_str());
          printout ("Copyright (C) 2007 Tim Janik.\n");
          printout ("This is free software and comes with ABSOLUTELY NO WARRANTY; see\n");
          printout ("the source for copying conditions. Sources, examples and contact\n");
          printout ("information are available at http://rapicorn.org/.\n");
          exit_app (0);
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
window_displayed (WindowH &window)
{
  if (!dump_snapshot.empty())
    {
      window.snapshot (dump_snapshot);
    }
  if (test_dump)
    {
#if 0 // FIXME
      for (uint i = 0; i < test_dump_matched_nodes.size(); i++)
        tstream->filter_matched_nodes (test_dump_matched_nodes[i]);
#endif
      String s = window.test_dump();
      printout ("%s", s.c_str());
      test_dump = false;
    }
  if (auto_exit)
    window.close();
}

static Blob
unit_test_res (const String &res_path)
{
  const int saved_errno = errno;
  const char *rres = getenv ("RAPIDRUN_RES");
  if (rres && strlen (rres) > 0)
    {
      if (Path::isabs (res_path))
        return Blob::load (res_path);
      StringVector dirs = string_split_any (rres, ":;");
      for (auto dir : dirs)
        {
          const String path = Path::join (dir, res_path);
          if (Path::check (path, "e"))
            return Blob::load (path);
        }
    }
  if (0)
    printerr ("rapidrun: missing resource: %s\n", res_path);
  errno = saved_errno;
  return Blob();
}

extern "C" int
main (int   argc,
      char *argv[])
{
  /* initialize Rapicorn and its backend (X11) */
  ApplicationH app = Rapicorn::init_app ("Rapidrun", &argc, argv); // acquires Rapicorn mutex

  struct TestRes : Res { using Res::utest_hook; };
  TestRes::utest_hook (unit_test_res);

  parse_args (&argc, &argv);
  if (argc != 2)
    help_usage (true);

  /* find GUI definition file, relative to CWD */
  String filename = app.auto_path (argv[1], ".");

  /* load GUI definitions, fancy version of app.auto_load() */
  StringSeq definitions = app.auto_load (filename, "");

  /* print definitions */
  String dialog;
  for (uint i = 0; i < definitions.size(); i++)
    {
      bool iswindow = app.factory_window (definitions[i]);
      if (list_definitions)
        printout ("%s%s\n", definitions[i].c_str(), iswindow ? " (window)" : "");
      if (dialog == "" && iswindow)
        dialog = definitions[i];
    }
  if (list_definitions)
    exit_app (0);

  /* bail out without any dialogs */
  if (dialog == "")
    {
      printerr ("%s: no dialog definitions\n", filename.c_str());
      return 1;
    }

  // create window, hook up post-display handler
  WindowH window = app.create_window (dialog);
  window.sig_displayed() += [&window]() { window_displayed (window); };

  /* show window and process events */
  window.show();

  return app.run_and_exit();
}

} // anon
