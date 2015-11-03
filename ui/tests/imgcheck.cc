// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include <rapicorn.hh>
#include <rcore/testutils.hh>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../../configure.h"

#define PRGNAME "imgcheck"

namespace {
using namespace Rapicorn;

static double
compare_image_files (const String &image1_file,
                     const String &image2_file)
{
  Pixmap image1 (0, 0), image2 (0, 0);
  /* load images */
  if (!image1.load_png (image1_file))
    fatal ("failed to load \"%s\": %s", image1_file.c_str(), string_from_errno (errno).c_str());

  if (!image2.load_png (image2_file))
    fatal ("failed to load \"%s\": %s", image2_file.c_str(), string_from_errno (errno).c_str());

  /* check sizes */
  if (image1.width() != image2.width() || image1.height() != image2.height())
    return DBL_MAX;

  /* check equality */
  double avgerror = 0, maxerr = 0, npixels = 0, nerrors = 0;
  image1.compare (image2, 0, 0, -1, -1, 0, 0, &avgerror, &maxerr, &nerrors, &npixels);
  if (0) // debugging
    printerr ("image difference: avgerror=%f maxerr=%f nerrors=%g npixels=%g (%u * %u)\n",
              avgerror, maxerr, nerrors, npixels, image1.width(), image1.height());

  return maxerr;
}

static double similarity_threshold_perc = 6.0;

static void
help_usage (bool usage_error)
{
  String usage = String () + "Usage: " + PRGNAME + " [OPTIONS] <image1.png> <image2.png>";
  if (usage_error)
    {
      printerr ("%s\n", usage.c_str());
      printerr ("Try '%s --help' for more information.\n", PRGNAME);
      exit_app (1);
    }
  printout ("%s\n", usage.c_str());
  /*         12345678901234567890123456789012345678901234567890123456789012345678901234567890 */
  printout ("Compare image1.png and image2.png according to a similarity threshold.\n");
  printout ("\n");
  printout ("Options:\n");
  printout ("  -t <threshold_percent>        Threshold to consider images equal.\n");
  printout ("  -h, --help                    Display this help and exit.\n");
  printout ("  -v, --version                 Display version and exit.\n");
}

static void
parse_args (int    *argc_p,
            char ***argv_p)
{
  char **argv = *argv_p;
  uint argc = *argc_p;

  for (uint i = 1; i < argc; i++)
    {
      if (strcmp (argv[i], "-t") == 0 && i + 1 < argc)
        {
          similarity_threshold_perc = string_to_double (argv[i + 1]);
          argv[i++] = NULL;
          argv[i] = NULL;
        }
      else if (strcmp (argv[i], "--help") == 0 || strcmp (argv[i], "-h") == 0)
        {
          help_usage (false);
          exit_app (0);
        }
      else if (strcmp (argv[i], "--version") == 0 || strcmp (argv[i], "-v") == 0)
        {
          printout ("%s (Rapicorn utilities) %s\n", PRGNAME, RAPICORN_VERSION);
          printout ("Copyright (C) 2008 Tim Janik.\n");
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

} // Anon

int
main (int   argc,
      char *argv[])
{
  init_core_test (String ("Rapicorn/") + PRGNAME, &argc, argv);

  parse_args (&argc, &argv);
  if (argc < 3)
    {
      help_usage (false);
      exit_app (1);
    }

  double imgerror = compare_image_files (argv[1], argv[2]);

  if (fabs (imgerror) > similarity_threshold_perc / 100.0)
    fatal ("excessive image difference for \"%s\" - \"%s\": %f%% > %f%%",
           argv[1], argv[2], fabs (imgerror) * 100.0, similarity_threshold_perc);

  return 0;
}
