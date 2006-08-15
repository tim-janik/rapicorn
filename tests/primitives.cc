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
using Rapicorn::uint;

static void
color_test()
{
  printf ("%s: testing set_hsv(get_hsv(v)) == v...\n", __func__);
  for (uint r = 0; r < 256; r++)
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
}

extern "C" int
main (int   argc,
      char *argv[])
{
  birnet_init_test (&argc, &argv);
  color_test();
  return 0;
}

} // anon
