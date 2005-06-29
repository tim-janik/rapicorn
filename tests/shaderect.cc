/* Tests
 * Copyright (C) 2005 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <rapicorn/rapicorn.hh>

namespace {
using namespace Rapicorn;
using Rapicorn::uint;

static void
shade_rect_test (uint n)
{
  printf ("%s: testing draw_shaded_rect()...\n", __func__);
  Plane plane (0, 0, 1024, 1024);
  Painter painter (plane);
  for (uint i = 0; i <= n; i++)
    painter.draw_shaded_rect (0, 0, 0x80101010, 1024, 1024, 0x80202020);
}

extern "C" int
main (int   argc,
      char *argv[])
{
  printf ("TEST: %s:\n", basename (argv[0]));
  shade_rect_test (argc > 1 ? Birnet::string_to_uint (argv[1]) : 1);
  return 0;
}

} // anon
