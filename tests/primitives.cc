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
//#define TEST_VERBOSE
#include <birnet/birnettests.h>
#include <rapicorn/rapicorn.hh>

namespace {
using namespace Rapicorn;

static void
color_test()
{
  /* assert that set_hsv(get_hsv(v))==v for all v */
  TSTART ("HSV-convert");
  for (uint r = 0; r < 256; r++)
    {
      for (uint g = 0; g < 256; g++)
        for (uint b = 0; b < 256; b++)
          {
            Color c (r, g, b, 0xff);
            double hue, saturation, value;
            c.get_hsv (&hue, &saturation, &value);
            if (r > g && r > b)
              BIRNET_ASSERT (hue < 60 || hue > 300);
            else if (g > r && g > b)
              BIRNET_ASSERT (hue > 60 || hue < 180);
            else if (b > g && b > r)
              BIRNET_ASSERT (hue > 180 || hue < 300);
            Color d (0xff75c3a9);
            d.set_hsv (hue, saturation, value);
            if (c.red()   != d.red() ||
                c.green() != d.green() ||
                c.blue()  != d.blue() ||
                c.alpha() != d.alpha())
              error ("color difference after hsv: %s != %s (hue=%f saturation=%f value=%f)\n",
                     c.string().c_str(), d.string().c_str(), hue, saturation, value);
          }
      if (r % 5 == 0)
        TICK();
    }
  TDONE();
}

static void
affine_test()
{
  TSTART ("affine");
  AffineIdentity id;
  Affine a;
  TASSERT (a == id);
  TASSERT (a.determinant() == 1);
  TASSERT (a.is_identity());
  AffineTranslate at (1, 1);
  TASSERT (at.point (3, 5) == Point (4, 6));
  TASSERT (at.ipoint (4, 6) == Point (3, 5));
  TDONE();
}

static void
math_test()
{
  TSTART ("dtoi32");
  TASSERT (_dtoi32_generic (0.0) == 0);
  TASSERT (_dtoi32_generic (+0.3) == +0);
  TASSERT (_dtoi32_generic (-0.3) == -0);
  TASSERT (_dtoi32_generic (+0.7) == +1);
  TASSERT (_dtoi32_generic (-0.7) == -1);
  TASSERT (_dtoi32_generic (+2147483646.3) == +2147483646);
  TASSERT (_dtoi32_generic (+2147483646.7) == +2147483647);
  TASSERT (_dtoi32_generic (-2147483646.3) == -2147483646);
  TASSERT (_dtoi32_generic (-2147483646.7) == -2147483647);
  TASSERT (_dtoi32_generic (-2147483647.3) == -2147483647);
  TASSERT (_dtoi32_generic (-2147483647.7) == -2147483648LL);
  TASSERT (dtoi32 (0.0) == 0);
  TASSERT (dtoi32 (+0.3) == +0);
  TASSERT (dtoi32 (-0.3) == -0);
  TASSERT (dtoi32 (+0.7) == +1);
  TASSERT (dtoi32 (-0.7) == -1);
  TASSERT (dtoi32 (+2147483646.3) == +2147483646);
  TASSERT (dtoi32 (+2147483646.7) == +2147483647);
  TASSERT (dtoi32 (-2147483646.3) == -2147483646);
  TASSERT (dtoi32 (-2147483646.7) == -2147483647);
  TASSERT (dtoi32 (-2147483647.3) == -2147483647);
  TASSERT (dtoi32 (-2147483647.7) == -2147483648LL);
  TDONE();
  TSTART ("dtoi64");
  TASSERT (_dtoi64_generic (0.0) == 0);
  TASSERT (_dtoi64_generic (+0.3) == +0);
  TASSERT (_dtoi64_generic (-0.3) == -0);
  TASSERT (_dtoi64_generic (+0.7) == +1);
  TASSERT (_dtoi64_generic (-0.7) == -1);
  TASSERT (_dtoi64_generic (+2147483646.3) == +2147483646);
  TASSERT (_dtoi64_generic (+2147483646.7) == +2147483647);
  TASSERT (_dtoi64_generic (-2147483646.3) == -2147483646);
  TASSERT (_dtoi64_generic (-2147483646.7) == -2147483647);
  TASSERT (_dtoi64_generic (-2147483647.3) == -2147483647);
  TASSERT (_dtoi64_generic (-2147483647.7) == -2147483648LL);
  TASSERT (_dtoi64_generic (+4294967297.3) == +4294967297LL);
  TASSERT (_dtoi64_generic (+4294967297.7) == +4294967298LL);
  TASSERT (_dtoi64_generic (-4294967297.3) == -4294967297LL);
  TASSERT (_dtoi64_generic (-4294967297.7) == -4294967298LL);
  TASSERT (_dtoi64_generic (+1125899906842624.3) == +1125899906842624LL);
  TASSERT (_dtoi64_generic (+1125899906842624.7) == +1125899906842625LL);
  TASSERT (_dtoi64_generic (-1125899906842624.3) == -1125899906842624LL);
  TASSERT (_dtoi64_generic (-1125899906842624.7) == -1125899906842625LL);
  TASSERT (dtoi64 (0.0) == 0);
  TASSERT (dtoi64 (+0.3) == +0);
  TASSERT (dtoi64 (-0.3) == -0);
  TASSERT (dtoi64 (+0.7) == +1);
  TASSERT (dtoi64 (-0.7) == -1);
  TASSERT (dtoi64 (+2147483646.3) == +2147483646);
  TASSERT (dtoi64 (+2147483646.7) == +2147483647);
  TASSERT (dtoi64 (-2147483646.3) == -2147483646);
  TASSERT (dtoi64 (-2147483646.7) == -2147483647);
  TASSERT (dtoi64 (-2147483647.3) == -2147483647);
  TASSERT (dtoi64 (-2147483647.7) == -2147483648LL);
  TASSERT (dtoi64 (+4294967297.3) == +4294967297LL);
  TASSERT (dtoi64 (+4294967297.7) == +4294967298LL);
  TASSERT (dtoi64 (-4294967297.3) == -4294967297LL);
  TASSERT (dtoi64 (-4294967297.7) == -4294967298LL);
  TASSERT (dtoi64 (+1125899906842624.3) == +1125899906842624LL);
  TASSERT (dtoi64 (+1125899906842624.7) == +1125899906842625LL);
  TASSERT (dtoi64 (-1125899906842624.3) == -1125899906842624LL);
  TASSERT (dtoi64 (-1125899906842624.7) == -1125899906842625LL);
  TDONE();
  TSTART ("iround");
  TASSERT (round (0.0) == 0.0);
  TASSERT (round (+0.3) == +0.0);
  TASSERT (round (-0.3) == -0.0);
  TASSERT (round (+0.7) == +1.0);
  TASSERT (round (-0.7) == -1.0);
  TASSERT (round (+4294967297.3) == +4294967297.0);
  TASSERT (round (+4294967297.7) == +4294967298.0);
  TASSERT (round (-4294967297.3) == -4294967297.0);
  TASSERT (round (-4294967297.7) == -4294967298.0);
  TASSERT (iround (0.0) == 0);
  TASSERT (iround (+0.3) == +0);
  TASSERT (iround (-0.3) == -0);
  TASSERT (iround (+0.7) == +1);
  TASSERT (iround (-0.7) == -1);
  TASSERT (iround (+4294967297.3) == +4294967297LL);
  TASSERT (iround (+4294967297.7) == +4294967298LL);
  TASSERT (iround (-4294967297.3) == -4294967297LL);
  TASSERT (iround (-4294967297.7) == -4294967298LL);
  TASSERT (iround (+1125899906842624.3) == +1125899906842624LL);
  TASSERT (iround (+1125899906842624.7) == +1125899906842625LL);
  TASSERT (iround (-1125899906842624.3) == -1125899906842624LL);
  TASSERT (iround (-1125899906842624.7) == -1125899906842625LL);
  TDONE();
  TSTART ("iceil");
  TASSERT (ceil (0.0) == 0.0);
  TASSERT (ceil (+0.3) == +1.0);
  TASSERT (ceil (-0.3) == -0.0);
  TASSERT (ceil (+0.7) == +1.0);
  TASSERT (ceil (-0.7) == -0.0);
  TASSERT (ceil (+4294967297.3) == +4294967298.0);
  TASSERT (ceil (+4294967297.7) == +4294967298.0);
  TASSERT (ceil (-4294967297.3) == -4294967297.0);
  TASSERT (ceil (-4294967297.7) == -4294967297.0);
  TASSERT (iceil (0.0) == 0);
  TASSERT (iceil (+0.3) == +1);
  TASSERT (iceil (-0.3) == -0);
  TASSERT (iceil (+0.7) == +1);
  TASSERT (iceil (-0.7) == -0);
  TASSERT (iceil (+4294967297.3) == +4294967298LL);
  TASSERT (iceil (+4294967297.7) == +4294967298LL);
  TASSERT (iceil (-4294967297.3) == -4294967297LL);
  TASSERT (iceil (-4294967297.7) == -4294967297LL);
  TASSERT (iceil (+1125899906842624.3) == +1125899906842625LL);
  TASSERT (iceil (+1125899906842624.7) == +1125899906842625LL);
  TASSERT (iceil (-1125899906842624.3) == -1125899906842624LL);
  TASSERT (iceil (-1125899906842624.7) == -1125899906842624LL);
  TDONE();
  TSTART ("ifloor");
  TASSERT (floor (0.0) == 0.0);
  TASSERT (floor (+0.3) == +0.0);
  TASSERT (floor (-0.3) == -1.0);
  TASSERT (floor (+0.7) == +0.0);
  TASSERT (floor (-0.7) == -1.0);
  TASSERT (floor (+4294967297.3) == +4294967297.0);
  TASSERT (floor (+4294967297.7) == +4294967297.0);
  TASSERT (floor (-4294967297.3) == -4294967298.0);
  TASSERT (floor (-4294967297.7) == -4294967298.0);
  TASSERT (ifloor (0.0) == 0);
  TASSERT (ifloor (+0.3) == +0);
  TASSERT (ifloor (-0.3) == -1);
  TASSERT (ifloor (+0.7) == +0);
  TASSERT (ifloor (-0.7) == -1);
  TASSERT (ifloor (+4294967297.3) == +4294967297LL);
  TASSERT (ifloor (+4294967297.7) == +4294967297LL);
  TASSERT (ifloor (-4294967297.3) == -4294967298LL);
  TASSERT (ifloor (-4294967297.7) == -4294967298LL);
  TASSERT (ifloor (+1125899906842624.3) == +1125899906842624LL);
  TASSERT (ifloor (+1125899906842624.7) == +1125899906842624LL);
  TASSERT (ifloor (-1125899906842624.3) == -1125899906842625LL);
  TASSERT (ifloor (-1125899906842624.7) == -1125899906842625LL);
  TDONE();
}

extern "C" int
main (int   argc,
      char *argv[])
{
  birnet_init_test (&argc, &argv);
  math_test();
  affine_test();
  color_test();
  return 0;
}

} // anon
