/* Rapicorn
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
#include "primitives.hh"

namespace Rapicorn {

struct FPUCheck {
  FPUCheck()
  {
#if defined __i386__ && defined __GNUC__
    /* assert rounding mode round-to-nearest which ftoi() relies on */
    unsigned short int fpu_state;
    __asm__ ("fnstcw %0"
             : "=m" (*&fpu_state));
    bool rounding_mode_is_round_to_nearest = !(fpu_state & 0x0c00);
#else
    bool rounding_mode_is_round_to_nearest = true;
#endif
    assert (rounding_mode_is_round_to_nearest);
  }
};
static FPUCheck assert_rounding_mode;

void
Color::get_hsv (double *huep,           /* 0..360: 0=red, 120=green, 240=blue */
                double *saturationp,    /* 0..1 */
                double *valuep)         /* 0..1 */
{
  double r = red(), g = green(), b = blue();
  double value = MAX (MAX (r, g), b);
  double delta = value - MIN (MIN (r, g), b);
  double saturation = value == 0 ? 0 : delta / value;
  double hue = 0;
  if (saturation && huep)
    {
      if (r == value)
        {
          hue = 0 + 60 * (g - b) / delta;
          if (hue <= 0)
            hue += 360;
        }
      else if (g == value)
        hue = 120 + 60 * (b - r) / delta;
      else /* b == value */
        hue = 240 + 60 * (r - g) / delta;
    }
  if (huep)
    *huep = hue;
  if (saturationp)
    *saturationp = saturation;
  if (valuep)
    *valuep = value / 255;
}

void
Color::set_hsv (double hue,             /* 0..360: 0=red, 120=green, 240=blue */
                double saturation,      /* 0..1 */
                double value)           /* 0..1 */
{
  uint center = ifloor (hue / 60);
  double frac = hue / 60 - center;
  double v1s = value * (1 - saturation);
  double vsf = value * (1 - saturation * frac);
  double v1f = value * (1 - saturation * (1 - frac));
  switch (center)
    {
    case 6:
    case 0: /* red */
      red (iround (value * 0xff));
      green (iround (v1f * 0xff));
      blue (iround (v1s * 0xff));
      break;
    case 1: /* red + green */
      red (iround (vsf * 0xff));
      green (iround (value * 0xff));
      blue (iround (v1s * 0xff));
      break;
    case 2: /* green */
      red (iround (v1s * 0xff));
      green (iround (value * 0xff));
      blue (iround (v1f * 0xff));
      break;
    case 3: /* green + blue */
      red (iround (v1s * 0xff));
      green (iround (vsf * 0xff));
      blue (iround (value * 0xff));
      break;
    case 4: /* blue */
      red (iround (v1f * 0xff));
      green (iround (v1s * 0xff));
      blue (iround (value * 0xff));
      break;
    case 5: /* blue + red */
      red (iround (value * 0xff));
      green (iround (v1s * 0xff));
      blue (iround (vsf * 0xff));
      break;
    }
}

} // Rapicorn
