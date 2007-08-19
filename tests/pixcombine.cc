/* Tests
 * Copyright (C) 2005 Tim Janik
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
//#define TEST_VERBOSE
#include <rapicorn/birnettests.h>
#include <rapicorn/rapicorn.hh>

namespace {
using namespace Rapicorn;

static void
pixel_combine (uint n)
{
  printf ("%s: testing pixel_combine()...\n", __func__);
  Plane plane1 (0, 0, 1024, 1024);
  Plane plane2 (0, 0, 1024, 1024);
  plane1.fill (0x80808080);
  Painter painter (plane2);
  painter.draw_shaded_rect (0, 0, 0x80101010, 1024, 1024, 0x80202020);
  for (uint i = 0; i <= n; i++)
    plane1.combine (plane2, COMBINE_NORMAL, 0xff);
}

extern "C" int
main (int   argc,
      char *argv[])
{
  birnet_init_test (&argc, &argv);
  pixel_combine (argc > 1 ? Birnet::string_to_uint (argv[1]) : 1);
  return 0;
}

} // anon
