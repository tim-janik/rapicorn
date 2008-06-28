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
#include <stdio.h>
#include <stdint.h>

/* PLIC TypePackage Parser */
#include "../PlicTypePackage.cc"

#ifndef MAX
#define MIN(a,b)        ((a) <= (b) ? (a) : (b))
#define MAX(a,b)        ((a) >= (b) ? (a) : (b))
#endif

/* --- test program --- */
int
main (int   argc,
      char *argv[])
{
  if (argc < 2)
    error ("Usage: %s <cstring>\n", argv[0]);
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
        }
    }
  // FIXME: check that no \0 is contained in the whole string
  return 0;
}
// g++ -Wall -Os ptptest.cc && ./a.out plic.out
