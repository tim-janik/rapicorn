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
#include <rcore/rapicorntests.h>
#include <rapicorn.hh>
#include <stdio.h>

namespace {
using namespace Rapicorn;

static void
plane_fill_test()
{
#if 0
  Plane plane1 (0, 0, 1024, 1024);
  plane1.fill (0x80808080);     // premul: 0x80404040
  TASSERT_CMP (0x80404040, ==, *plane1.poke_span (0, 0, 1));
  TASSERT_CMP (0x80404040, ==, *plane1.poke_span (1023, 0, 1));
  TASSERT_CMP (0x80404040, ==, *plane1.poke_span (0, 1023, 1));
  TASSERT_CMP (0x80404040, ==, *plane1.poke_span (1023, 1023, 1));
  TASSERT_CMP (0x80404040, ==, *plane1.poke_span (512, 512, 1));
#endif
}

static void
plane_gradient_test()
{
#if 0
  Plane plane1 (0, 0, 1024, 1024);
  Painter painter (plane1);
  painter.draw_gradient_rect (0, 0, 1024, 1024,
                              200, 300, Color (0x80406080),     // premul: 0x80203040
                              600, 700, Color (0xff202020));    // premul: 0xff202020
  TASSERT_CMPx (0x80203040, ==, *plane1.poke_span (0, 0, 1));
  TASSERT_CMPx (0xff202020, ==, *plane1.poke_span (1023, 1023, 1));
  TASSERT_CMPx (0x80203040, ==, *plane1.poke_span (0, 200 + 1, 1));
  TASSERT_CMPx (0xff202020, ==, *plane1.poke_span (1023, 700 - 1, 1));
  TASSERT_CMPx (0x80203040, <,  *plane1.poke_span (400, 500, 1));
  TASSERT_CMPx (0xff202020, >,  *plane1.poke_span (400, 500, 1));
#endif
}

static void
pixel_combine (uint n)
{
  printf ("%s: testing pixel_combine()...\n", __func__);
#if 0
  Plane plane1 (0, 0, 1024, 1024);
  Plane plane2 (0, 0, 1024, 1024);
  plane1.fill (0x80808080);
  Painter painter (plane2);
  painter.draw_shaded_rect (0, 0, 0x80101010, 1024, 1024, 0x80202020);
  for (uint i = 0; i <= n; i++)
    plane1.combine (plane2, COMBINE_NORMAL, 0xff);
#endif
}

extern "C" int
main (int   argc,
      char *argv[])
{
  rapicorn_init_test (&argc, &argv);
  pixel_combine (argc > 1 ? Rapicorn::string_to_uint (argv[1]) : 1);
  TRUN ("Fill",         plane_fill_test);
  TRUN ("Gradient",     plane_gradient_test);
  return 0;
}

} // anon
