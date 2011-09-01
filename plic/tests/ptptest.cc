/* ptptest.cc - PLIC Test Program
 * Copyright (C) 2008 Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this program; if not, see http://www.gnu.org/copyleft/.
 */

// include PLIC TypeMap Parser
#include "../../rcore/plicutils.hh"
#include "../../rcore/plicutils.cc"

#define error(...) do { fputs ("ERROR: ", stderr); fprintf (stderr, __VA_ARGS__); fputs ("\n", stderr); abort(); } while (0)

using namespace Plic;
using Plic::uint8;
using Plic::uint;
using Plic::vector;
using Plic::String;

#ifndef MAX
#define MIN(a,b)        ((a) <= (b) ? (a) : (b))
#define MAX(a,b)        ((a) >= (b) ? (a) : (b))
#endif

/* --- option parsing --- */
static bool
parse_str_option (char       **argv,
                  uint        &i,
                  const char  *arg,
                  const char **strp,
                  uint         argc)
{
  uint length = strlen (arg);
  if (strncmp (argv[i], arg, length) == 0)
    {
      const char *equal = argv[i] + length;
      if (*equal == '=')              /* -x=Arg */
        *strp = equal + 1;
      else if (*equal)                /* -xArg */
        *strp = equal;
      else if (i + 1 < argc)          /* -x Arg */
        {
          argv[i++] = NULL;
          *strp = argv[i];
        }
      argv[i] = NULL;
      if (*strp)
        return true;
    }
  return false;
}

static bool
parse_bool_option (char       **argv,
                   uint        &i,
                   const char  *arg)
{
  uint length = strlen (arg);
  if (strncmp (argv[i], arg, length) == 0)
    {
      argv[i] = NULL;
      return true;
    }
  return false;
}

/* --- test program --- */

static void
aux_check (std::string    type_map_file,
           vector<String> auxtests)
{
  // read type map
  TypeMap tp = TypeMap::load (type_map_file);
  if (tp.error_status())
    error ("%s: open failed: %s", type_map_file.c_str(), strerror (tp.error_status()));
  // print types and match auxtests
  for (size_t i = 0; i < tp.type_count(); i++)
    {
      const TypeCode tc = tp.type (i);
      printf ("%s %s;\n", tc.kind_name().c_str(), tc.name().c_str());
      for (size_t k = 0; k < tc.aux_count(); k++)
        {
          String aux = tc.aux_data (k);
          uint c = ' ';
          for (size_t q = 0; q < auxtests.size(); q++)
            if (strstr (aux.c_str(), auxtests[q].c_str()))
              {
                auxtests.erase (auxtests.begin() + q);
                c = '*';
                break;
              }
          printf ("  %c %s\n", c, aux.c_str());
        }
    }
  // fail for unmatched auxtests
  if (auxtests.size())
    {
      fprintf (stderr, "ERROR: unmatched aux-tests:\n");
      for (uint q = 0; q < auxtests.size(); q++)
        fprintf (stderr, "  %s\n", auxtests[q].c_str());
      abort();
    }
}

static void
list_types (std::string type_map_file)
{
  TypeMap tmap = TypeMap::load_local (type_map_file);
  if (tmap.error_status())
    error ("%s: open failed: %s", type_map_file.c_str(), strerror (tmap.error_status()));
  errno = 0;
  printf ("TYPES: %zu\n", tmap.type_count());
  for (uint i = 0; i < tmap.type_count(); i++)
    {
      const TypeCode tcode = tmap.type (i);
      printf ("%s\n", tcode.pretty ("  ").c_str());
      // verify type search
      const TypeCode other = tmap.lookup_local (tcode.name());
      if (other != tcode)
        {
          errno = ENXIO;
          perror (std::string ("searching type: " + tcode.name()).c_str()), abort();
        }
    }
  if (errno)
    perror ("checking type package"), abort();
}

static void
standard_tests ()
{
  TypeCode tcint = TypeMap::lookup ("int");
  assert (tcint.kind() == INT);
  TypeCode tcfloat = TypeMap::lookup ("float");
  assert (tcfloat.kind() == FLOAT);
  TypeCode tcstring = TypeMap::lookup ("string");
  assert (tcstring.kind() == STRING);
  TypeCode tcany = TypeMap::lookup ("any");
  assert (tcany.kind() == ANY);
  printf ("  TEST   Plic standard types                                             OK\n");
  exit (0);
}

int
main (int   argc,
      char *argv[])
{
  vector<String> auxtests;
  /* parse args */
  for (uint i = 1; i < uint (argc); i++)
    {
      const char *str = NULL;
      if (strcmp (argv[i], "--tests") == 0)
        standard_tests();
      else if (strcmp (argv[i], "--") == 0)
        {
          argv[i] = NULL;
          break;
        }
      else if (parse_bool_option (argv, i, "--help"))
        {
          printf ("Usage: %s [options] TypeMap.typ\n", argv[0]);
          printf ("Options:\n");
          printf ("  --tests       Check standard types\n");
          printf ("  --help        Print usage summary\n");
          printf ("  --aux-test=x  Find 'x' in auxillary type data\n");
          exit (0);
        }
      else if (parse_str_option (argv, i, "--aux-test", &str, argc))
        auxtests.push_back (str);
    }
  /* collapse parsed args */
  uint e = 1;
  for (uint i = 1; i < uint (argc); i++)
    if (argv[i])
      {
        argv[e++] = argv[i];
        if (i >= e)
          argv[i] = NULL;
      }
  argc = e;
  /* validate mandatory arg */
  if (argc < 2)
    error ("Usage: %s TypeMap.typ\n", argv[0]);

  if (auxtests.size())
    aux_check (argv[1], auxtests);
  else
    list_types (argv[1]);

  return 0;
}
// g++ -Wall -Os ptptest.cc && ./a.out typemap.typ
