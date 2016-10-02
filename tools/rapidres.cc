// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "rcore/cxxaux.hh"
#include "../configure.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <zlib.h>
#include <vector>
#include <string>
#include <sys/types.h>
#include <regex.h>
using namespace Rapicorn;

/* rapidres - small C source compression utility
 * Implemented without linking against librapicorn, as it may be used during the rapicorn build process.
 */

typedef std::string String;

static const char *main_argv0 = "rapidres";

static void zintern_error  (const char *format, ...) RAPICORN_PRINTF (1, 2);

static void
zintern_error (const char *format, ...)
{
  char buffer[4096];
  va_list args;
  va_start (args, format);
  vsnprintf (buffer, sizeof (buffer), format, args);
  va_end (args);
  buffer[sizeof (buffer) - 1] = 0;
  fprintf (stderr, "\n%s: ERROR: %s\n", main_argv0, buffer);
  _exit (1);
}

static String strip_prefix;

typedef struct {
  uint pos;
  bool pad;
} Config;
static Config config_init = { 0, 0 };

static inline void
print_uchar (Config *config,
	     uint8 d)
{
  if (config->pos > 70)
    {
      printf ("\"\n  \"");
      config->pos = 3;
      config->pad = false;
    }
  if (d == '\\')
    {
      printf ("\\\\");
      config->pos += 2;
    }
  else if (d == '\n')
    {
      printf ("\\n");
      config->pos += 2;
    }
  else if (d == '"')
    {
      printf ("\\\"");
      config->pos += 2;
    }
  else if (d < 32 || d > 126 || d == '?')
    {
      printf ("\\%o", d);
      config->pos += 1 + 1 + (d > 7) + (d > 63);
      config->pad = d < 64;
      return;
    }
  else if (config->pad && d >= '0' && d <= '9')
    {
      printf ("\"\"");
      printf ("%c", d);
      config->pos += 3;
    }
  else
    {
      printf ("%c", d);
      config->pos += 1;
    }
  config->pad = false;
}

static void
gen_zfile (const String &file)
{
  // create nice resource path
  String fname = file;
  // strip common path prefix
  if (!strip_prefix.empty())
    {
      regex_t rx;
      if (regcomp (&rx, strip_prefix.c_str(), REG_EXTENDED | REG_NEWLINE) == 0)
        {
          regmatch_t pmatch[1];
          if (regexec (&rx, fname.c_str(), ARRAY_SIZE (pmatch), pmatch, 0) == 0 &&
              pmatch[0].rm_so == 0 && size_t (pmatch[0].rm_eo) < fname.size())
            {
              fname = fname.substr (pmatch[0].rm_eo);
            }
          regfree (&rx);
        }
    }
  // create C identifier
  String ident = fname;
  // substitute [/.-] with _
  for (size_t i = 0; i < ident.size(); i++)
    if (strchr ("./-", ident[i]))
      ident[i] = '_';
  // check identifier chars
  for (size_t i = 0; i < ident.size(); i++)
    if ((ident[i] >= 'a' && ident[i] <= 'z') ||
        (ident[i] >= 'A' && ident[i] <= 'Z') ||
        (i && ident[i] >= '0' && ident[i] <= '9') ||
        ident[i] == '_')
      ; // fine
    else
      zintern_error ("file name contains non-symbol characters: %s", file.c_str());
  // file processing
  std::vector<uint8> vdata;
  uint i, dlen = 0, mlen = 0;
  uLongf clen;
  Config config;
  FILE *f = fopen (file.c_str(), "r");
  if (!f)
    zintern_error ("failed to open \"%s\": %s", file.c_str(), strerror (errno));
  do
    {
      if (mlen <= dlen + 1024)
	{
	  mlen += 8192;
          vdata.resize (mlen);
	}
      dlen += fread (&vdata[dlen], 1, mlen - dlen, f);
    }
  while (!ferror (f) && !feof (f));

  if (ferror (f))
    zintern_error ("failed to read from \"%s\": %s", file.c_str(), strerror (errno));

  std::vector<uint8> cdata;

  int result;
  const char *err;
  clen = dlen + dlen / 100 + 64;
  cdata.resize (clen);
  result = compress2 (&cdata[0], &clen, &vdata[0], dlen, Z_BEST_COMPRESSION);
  switch (result)
    {
    case Z_OK:
      err = NULL;
      break;
    case Z_MEM_ERROR:
      err = "out of memory";
      break;
    case Z_BUF_ERROR:
      err = "insufficient buffer size";
      break;
    default:
      err = "unknown error";
      break;
    }
  if (err)
    zintern_error ("while compressing \"%s\": %s", file.c_str(), err);

  printf ("/* rapidres file dump of %s */\n", file.c_str());

  /* two things to consider for using the compressed data:
   * 1) for the reader code, pack_size + 1 must be smaller than data_size to identify compressed data.
   * 2) using compressed data requires runtime unpacking overhead and extra dynamic memory allocation,
   *    so it should provide a *significant* benefit if it's used.
   */
  const bool compress_resource = clen <= 0.75 * dlen && clen + 1 < dlen;
  const size_t rlen = compress_resource ? clen : dlen;
  const uint8 *rdata = rlen == dlen ? &vdata[0] : &cdata[0];

  config = config_init;
  printf ("RAPICORN_RES_STATIC_DATA (%s) =\n  \"", ident.c_str());
  for (i = 0; i < rlen; i++)
    print_uchar (&config, rdata[i]);
  printf ("\"; // %zu + 1\n", rlen);

  config = config_init;
  printf ("RAPICORN_RES_STATIC_ENTRY (%s, \"", ident.c_str());
  for (i = 0; i < fname.size(); i++)
    print_uchar (&config, fname[i]);
  printf ("\", %u);\n", dlen);

  fclose (f);
}

static int
help (int exitcode)
{
  printf ("usage: rapidres [-h] [-v] [-s prefix] [files...]\n");
  if (exitcode != 0)
    exit (exitcode);
  printf ("  -h, --help    Print usage information\n");
  printf ("  -v, --version Print version and file paths\n");
  printf ("  -s PREFIX     Strip PREFIX from resource names\n");
  printf ("Generate compressed C source code for each file.\n");
  exit (0);
}

int
main (int argc, char *argv[])
{
  main_argv0 = argv[0];
  std::vector<String> arg_strings;

  for (int i = 1; i < argc; i++)
    {
      if (strcmp ("-h", argv[i]) == 0 || strcmp ("--help", argv[i]) == 0)
	{
	  return help (0);
	}
      else if (strcmp ("-v", argv[i]) == 0 || strcmp ("--version", argv[i]) == 0)
	{
          printf ("rapidrun (Rapicorn utilities) %s (Build ID: %s)\n", RAPICORN_VERSION, RapicornInternal::buildid());
          printf ("Copyright (C) 2007 Tim Janik.\n");
          printf ("This is free software and comes with ABSOLUTELY NO WARRANTY; see\n");
          printf ("the source for copying conditions. Sources, examples and contact\n");
          printf ("information are available at http://rapicorn.org/.\n");
          exit (0);
	}
      else if (strcmp ("-s", argv[i]) == 0 && i + 1 < argc)
        {
          strip_prefix = argv[i + 1];
          i++;
        }
      else
	arg_strings.push_back (argv[i]);
    }

  if (arg_strings.size() == 0)
    return help (1);

  for (size_t i = 0; i < arg_strings.size(); i++)
    {
      const String file = arg_strings[i + 0];
      gen_zfile (file);
    }

  return 0;
}
