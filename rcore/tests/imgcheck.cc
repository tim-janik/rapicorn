/* Rapicorn
 * Copyright (C) 2008 Tim Janik
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
#include <rcore/testutils.hh>
#include <errno.h>
#include <stdlib.h>

#define PRGNAME "imgcheck"

namespace {
using namespace Rapicorn;

static double
compare_image_files (const String &image1_file,
                     const String &image2_file)
{
  /* load images */
  Pixmap *image1 = Pixmap::load_png (image1_file);
  if (!image1 || errno)
    error ("failed to load \"%s\": %s", image1_file.c_str(), string_from_errno (errno).c_str());
  ref_sink (image1);

  Pixmap *image2 = Pixmap::load_png (image2_file);
  if (!image2 || errno)
    error ("failed to load \"%s\": %s", image2_file.c_str(), string_from_errno (errno).c_str());
  ref_sink (image2);

  /* check sizes */
  if (image1->width() != image2->width() ||
      image1->height() != image2->height())
    return DBL_MAX;

  /* check equality */
  double avgerror = 0, npixels = 0;
  image1->compare (*image2, 0, 0, -1, -1, 0, 0, &avgerror, NULL, NULL, &npixels);

  return avgerror * npixels;
}

static double similarity_threshold = 1.0;

static void
help_usage (bool usage_error)
{
  String usage = String () + "Usage: " + PRGNAME + " [OPTIONS] <image1.png> <image2.png>";
  if (usage_error)
    {
      printerr ("%s\n", usage.c_str());
      printerr ("Try '%s --help' for more information.\n", PRGNAME);
      exit (1);
    }
  printout ("%s\n", usage.c_str());
  /*         12345678901234567890123456789012345678901234567890123456789012345678901234567890 */
  printout ("Compare image1.png and image2.png according to a similarity threshold.\n");
  printout ("\n");
  printout ("Options:\n");
  printout ("  -t <similarity-threshold>     Threshold to consider images equal.\n");
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
          similarity_threshold = string_to_double (argv[i + 1]);
          argv[i++] = NULL;
          argv[i] = NULL;
        }
      else if (strcmp (argv[i], "--help") == 0 || strcmp (argv[i], "-h") == 0)
        {
          help_usage (false);
          exit (0);
        }
      else if (strcmp (argv[i], "--version") == 0 || strcmp (argv[i], "-v") == 0)
        {
          printout ("%s (Rapicorn utilities) %s\n", PRGNAME, RAPICORN_VERSION);
          printout ("Copyright (C) 2008 Tim Janik.\n");
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

} // Anon

int
main (int   argc,
      char *argv[])
{
  rapicorn_init_core (&argc, &argv, PRGNAME);

  parse_args (&argc, &argv);
  if (argc < 3)
    {
      help_usage (false);
      exit (1);
    }

  double imgerror = compare_image_files (argv[1], argv[2]);

  if (fabs (imgerror) > similarity_threshold)
    error ("excessive image difference for \"%s\" - \"%s\": %f > %f",
           argv[1], argv[2], fabs (imgerror), similarity_threshold);

  return 0;
}
