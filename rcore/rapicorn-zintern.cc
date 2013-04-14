// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "cxxaux.hh"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <zlib.h>
#include <vector>
#include <string>
using namespace Rapicorn;

/* rapicorn-zintern - small C source compression utility
 * Implemented without linking against librapicorn, as it may be used during the rapicorn build process.
 */

typedef std::string String;

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
  fprintf (stderr, "\nERROR: %s\n", buffer);
  _exit (1);
}

static bool use_compression = false;
static bool use_base_name = false;
static bool break_at_newlines = false;
static bool as_resource = false;

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
      if (break_at_newlines && config->pos > 3)
        config->pos |= 1 << 31;
    }
  else if (d == '"')
    {
      printf ("\\\"");
      config->pos += 2;
    }
  else if (d < 33 || d > 126 || d == '?')
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

#define to_upper(c)     ((c) >='a' && (c) <='z' ? (c) - 'a' + 'A' : (c))
#define is_alnum(c)     (((c) >='A' && (c) <='Z') || ((c) >='a' && (c) <='z') || ((c) >='0' && (c) <='9'))
static String
to_cupper (const String &str)
{
  String s (str);
  for (uint i = 0; i < s.size(); i++)
    if (is_alnum (s[i]))
      s[i] = to_upper (s[i]);
    else
      s[i] = '_';
  return s;
}

static String
canonify (const String &string, const String &valid_chars, const String &substitute)
{
  const size_t l = string.size();
  const char *p = string.c_str();
  String d;
  for (size_t i = 0; i < l; i++)
    if (strchr (valid_chars.c_str(), p[i]))
      d += p[i];
    else
      d += substitute;
  return d;
}

static String
base_name (const String &fname)
{
  const char *name = fname.c_str();
  const char *b = strrchr (name, RAPICORN_DIR_SEPARATOR);
  return b ? b + 1 : name;
}

static void
gen_zfile (const char *name,
	   const char *file)
{
  std::vector<uint8> vdata;
  uint i, dlen = 0, mlen = 0;
  uLongf clen;
  String fname = use_base_name ? base_name (file) : file;
  Config config;
  FILE *f = fopen (file, "r");
  if (!f)
    zintern_error ("failed to open \"%s\": %s", file, strerror (errno));
  do
    {
      if (mlen <= dlen + 1024)
	{
	  mlen += 8192;
          vdata.resize (mlen);
	}
      dlen += fread (&vdata[dlen], 1, mlen - dlen, f);
    }
  while (!feof (f));

  if (ferror (f))
    zintern_error ("failed to read from \"%s\": %s", file, strerror (errno));

  std::vector<uint8> cdata;
  if (use_compression || as_resource)
    {
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
	zintern_error ("while compressing \"%s\": %s", file, err);
    }
  else
    {
      clen = dlen;
      cdata = vdata;
    }

  printf ("/* rapicorn-zintern file dump of %s */\n", file);

  if (as_resource)
    {
      if (name[0] == '/')
        zintern_error ("invalid absolute resource path (must be relative): %s", name);
      /* two things to consider for using the compressed data:
       * 1) for the reader code, pack_size + 1 must be smaller than data_size to identify compressed data.
       * 2) using compressed data requires runtime unpacking overhead and extra dynamic memory allocation,
       *    so it should provide a *significant* benefit if it's used.
       */
      const bool compress_resource = clen <= 0.75 * dlen && clen + 1 < dlen;
      const size_t rlen = compress_resource ? clen : dlen;
      const uint8 *rdata = rlen == dlen ? &vdata[0] : &cdata[0];
      String ident = canonify (name, "0123456789abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ", "_");

      config = config_init;
      printf ("RAPICORN_STATIC_RESOURCE_DATA (%s) =\n  \"", ident.c_str());
      for (i = 0; i < rlen; i++)
        print_uchar (&config, rdata[i]);
      printf ("\"; // %lu + 1\n", rlen);

      config = config_init;
      printf ("RAPICORN_STATIC_RESOURCE_ENTRY (%s, \"", ident.c_str());
      for (i = 0; i < strlen (name); i++)
        print_uchar (&config, name[i]);
      printf ("\", %u);\n", dlen);
    }
  else
    {
      config = config_init;
      printf ("#define %s_NAME \"", to_cupper (name).c_str());
      for (i = 0; i < fname.size(); i++)
        print_uchar (&config, fname[i]);
      printf ("\"\n");

      printf ("#define %s_SIZE (%u)\n", to_cupper (name).c_str(), dlen);

      config = config_init;
      printf ("static const unsigned char %s_DATA[%lu + 1] =\n", to_cupper (name).c_str(), clen);
      printf ("( \"");
      for (i = 0; i < clen; i++)
        print_uchar (&config, cdata[i]);
      printf ("\");\n");
    }

  fclose (f);
}

static int
help (int exitcode)
{
  printf ("usage: rapicorn-zintern [-h] [-b] [-z] [[name file]...]\n");
  if (exitcode != 0)
    exit (exitcode);
  printf ("  -h  Print usage information\n");
  printf ("  -b  Strip directories from file names\n");
  printf ("  -n  Break output lines after newlines raw data\n");
  printf ("  -r  Produce Rapicorn resource declarations\n");
  printf ("  -z  Compress data blocks with libz\n");
  printf ("Parse (name, file) pairs and generate C source\n");
  printf ("containing inlined data blocks of the files given.\n");
  exit (0);
}

int
main (int argc, char *argv[])
{
  std::vector<String> arg_strings;

  for (int i = 1; i < argc; i++)
    {
      if (strcmp ("-z", argv[i]) == 0)
	{
	  use_compression = true;
	}
      else if (strcmp ("-r", argv[i]) == 0)
	{
	  as_resource = true;
	}
      else if (strcmp ("-b", argv[i]) == 0)
	{
	  use_base_name = true;
	}
      else if (strcmp ("-n", argv[i]) == 0)
	{
	  break_at_newlines = true;
	}
      else if (strcmp ("-h", argv[i]) == 0)
	{
	  return help (0);
	}
      else
	arg_strings.push_back (argv[i]);
    }

  if (arg_strings.size() % 2)
    return help (1);

  for (size_t i = 0; i < arg_strings.size(); i += 2)
    {
      const char *name = arg_strings[i + 0].c_str();
      const char *file = arg_strings[i + 1].c_str();
      gen_zfile (name, file);
    }

  return 0;
}
