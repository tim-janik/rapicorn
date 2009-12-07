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

/* PLIC TypePackage Parser */
#include "../PlicTypePackage.cc"
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
int
main (int   argc,
      char *argv[])
{
  vector<String> auxtests;
  /* parse args */
  for (uint i = 1; i < uint (argc); i++)
    {
      const char *str = NULL;
      if (strcmp (argv[i], "--") == 0)
        {
          argv[i] = NULL;
          break;
        }
      else if (parse_bool_option (argv, i, "--help"))
        {
          printf ("Usage: %s [options] TypePackage.out\n", argv[0]);
          printf ("Options:\n");
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
    error ("Usage: %s TypePackage.out\n", argv[0]);

  /* parse input file */
  int fd = open (argv[1], 0);
  if (fd < 0)
    error ("%s: open failed: %s", argv[1], strerror (errno));
  uint8 buffer[1024 * 1024];
  int l = read (fd, buffer, sizeof (buffer));
  if (l < 0)
    error ("%s: IO error: %s", argv[1], strerror (errno));
  close (fd);
  TypeRegistry tr;
  String err = tr.register_type_package (l, buffer);
  error_if (err != "", err);

  // check for intermediate \0 in type package data
  error_if (memchr (buffer, 0, l) != NULL, "embedded \\000 in type package");

  /* print types and match auxtests */
  vector<TypeNamespace> namespaces = tr.list_namespaces();
  for (uint i = 0; i < namespaces.size(); i++)
    {
      vector<TypeInfo> types = namespaces[i].list_types ();
      for (uint j = 0; j < types.size(); j++)
        {
          printf ("%c %s::%s;\n",
                  types[j].storage,
                  namespaces[i].fullname().c_str(),
                  types[j].name().c_str());
          uint ad = types[j].n_aux_strings();
          for (uint k = 0; k < ad; k++)
            {
              uint l = 0;
              const char *s = types[j].aux_string (k, &l);
              String astring = String (s, l);
              uint c = ' ';
              for (uint q = 0; q < auxtests.size(); q++)
                if (strstr (astring.c_str(), auxtests[q].c_str()))
                  {
                    auxtests.erase (auxtests.begin() + q);
                    c = '*';
                    break;
                  }
              printf ("  %c %s\n", c, astring.c_str());
            }
        }
    }

  /* fail for unmatched auxtests */
  if (auxtests.size())
    fprintf (stderr, "ERROR: unmatched aux-tests:\n");
  for (uint q = 0; q < auxtests.size(); q++)
    fprintf (stderr, "  %s\n", auxtests[q].c_str());
  return auxtests.size() != 0;
}
// g++ -Wall -Os ptptest.cc && ./a.out plic.out
