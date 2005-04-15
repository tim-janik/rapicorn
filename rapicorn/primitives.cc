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

Plane::Plane (int x, int y, uint width, uint height) :
  m_x (x), m_y (y), m_stride (width), m_height (height),
  m_pixel_buffer (NULL)
{
  m_pixel_buffer = new uint32[n_pixels()];
  memset (m_pixel_buffer, 0, 4 * n_pixels());
}

void
Plane::fill (Color c)
{
  uint32 argb = c.premultiplied();
  for (int y = 0; y < height(); y++)
    for (int x = 0; x < width(); x++)
      {
        uint32 *p = peek (x, y);
        *p = argb;
      }
}

Plane::~Plane()
{
  delete[] m_pixel_buffer;
}

bool
Plane::rgb_convert (uint cwidth, uint cheight, uint rowstride, uint8 *cpixels) const
{
  if (!m_pixel_buffer)
    return false;
  int h = MIN (cheight, (uint) height());
  int w = MIN (cwidth, (uint) width());
  for (int y = 0; y < h; y++)
    {
      const uint32 *pix = peek (0, y);
      uint8 *dst = cpixels + (h - 1 - y) * rowstride;
      for (const uint32 *b = pix + w; pix < b; pix++)
        {
          *dst++ = *pix >> 16;
          *dst++ = *pix >> 8;
          *dst++ = *pix;
        }
    }
  return true;
}

#define IMUL    Color::IMUL
#define IDIV    Color::IDIV
#define IDIV0   Color::IDIV0
#define IMULDIV Color::IMULDIV

template<int KIND> static inline void
pixel_combine (uint32 *D, const uint32 *B, const uint32 *A, uint span, uint8 blend_alpha)
{
  while (span--)
    {
      uint8 Da, Dr, Dg, Db;
      /* extract alpha, red, green ,blue */
      uint32 Aa = *A >> 24, Ar = (*A >> 16) & 0xff, Ag = (*A >> 8) & 0xff, Ab = *A & 0xff;
      uint32 Ba = *B >> 24, Br = (*B >> 16) & 0xff, Bg = (*B >> 8) & 0xff, Bb = *B & 0xff;
      /* A over B = colorA + colorB * (1 - alphaA) */
      if (KIND == COMBINE_OVER || KIND == COMBINE_NORMAL)
        {
          uint32 Ai = 255 - Aa;
          Dr = Ar + IMUL (Br, Ai);
          Dg = Ag + IMUL (Bg, Ai);
          Db = Ab + IMUL (Bb, Ai);
          Da = Aa + IMUL (Ba, Ai);
        }
      /* B over A = colorB + colorA * (1 - alphaB) */
      if (KIND == COMBINE_UNDER)
        {
          uint32 Bi = 255 - Ba;
          Dr = Br + IMUL (Ar, Bi);
          Dg = Bg + IMUL (Ag, Bi);
          Db = Bb + IMUL (Ab, Bi);
          Da = Ba + IMUL (Aa, Bi);
        }
      /* A add B = colorA + colorB */
      if (KIND == COMBINE_ADD)
        {
          Dr = MIN (Ar + Br, 255);
          Dg = MIN (Ag + Bg, 255);
          Db = MIN (Ab + Bb, 255);
          Da = MIN (Aa + Ba, 255);
        }
      /* A del B = colorB * (1 - alphaA) */
      if (KIND == COMBINE_DEL)
        {
          uint32 Ai = 255 - Aa;
          Dr = IMUL (Br, Ai);
          Dg = IMUL (Bg, Ai);
          Db = IMUL (Bb, Ai);
          Da = IMUL (Ba, Ai);
        }
      /* A atop B = colorA * alphaB + colorB * (1 - alphaA) */
      if (KIND == COMBINE_ATOP)
        {
          uint32 Ai = 255 - Aa;
          Dr = IMUL (Ar, Ba) + IMUL (Br, Ai);
          Dg = IMUL (Ag, Ba) + IMUL (Bg, Ai);
          Db = IMUL (Ab, Ba) + IMUL (Bb, Ai);
          Da = IMUL (Aa, Ba) + IMUL (Ba, Ai);
        }
      /* A xor B = colorA * (1 - alphaB) + colorB * (1 - alphaA) */
      if (KIND == COMBINE_XOR)
        {
          uint32 Ai = 255 - Aa, Bi = 255 - Ba;
          Dr = IMUL (Ar, Bi) + IMUL (Br, Ai);
          Dg = IMUL (Ag, Bi) + IMUL (Bg, Ai);
          Db = IMUL (Ab, Bi) + IMUL (Bb, Ai);
          Da = IMUL (Aa, Bi) + IMUL (Ba, Ai);
        }
      /* B * (1 - alpha) + A * alpha */
      if (KIND == COMBINE_BLEND)
        {
          uint32 ialpha = 255 - blend_alpha;
          Dr = IMUL (Br, ialpha) + Ar;
          Dg = IMUL (Bg, ialpha) + Ag;
          Db = IMUL (Bb, ialpha) + Ab;
          Da = IMUL (Ba, ialpha) + Aa;
        }
      /* (B.value = A.value) OVER B */
      if (KIND == COMBINE_VALUE)
        {
          uint8 Avalue = MAX (MAX (Ar, Ag), Ab); // HSV value
          uint8 Bvalue = MAX (MAX (Br, Bg), Bb); // HSV value
          Da = Aa;
          uint32 Di = 255 - Da;
          if (LIKELY (Bvalue))
            {
              Dr = IMUL (Br, Di) + IMULDIV (Br, Avalue, Bvalue);
              Dg = IMUL (Bg, Di) + IMULDIV (Bg, Avalue, Bvalue);
              Db = IMUL (Bb, Di) + IMULDIV (Bb, Avalue, Bvalue);
            }
          else
            {
              Dr = IMUL (Br, Di);
              Dg = IMUL (Bg, Di);
              Db = IMUL (Bb, Di);
            }
          Da = IMUL (Ba, Di) + Da;
        }
      /* assign */
      *D++ = (Da << 24) | (Dr << 16) | (Dg << 8) | Db;
      A++, B++;
    }
}

void
Plane::combine (const Plane &src, CombineType ct)
{
  Rect m (origin(), width(), height());
  Rect b (src.origin(), src.width(), src.height());
  b.intersect (m);
  if (b.is_empty())
    return;
  int xmin = iround (b.ll.x), xbound = iround (b.ur.x);
  int ymin = iround (b.ll.y), ybound = iround (b.ur.y);
  int xspan = xbound - xmin;
  for (int y = ymin; y < ybound; y++)
    {
      uint32 *d = peek (xmin - xstart(), y - ystart());
      const uint32 *s = src.peek (xmin - src.xstart(), y - src.ystart());
      switch (ct)
        {
        case COMBINE_NORMAL:  pixel_combine<COMBINE_NORMAL> (d, d, s, xspan, 0xff); break;
        case COMBINE_OVER:    pixel_combine<COMBINE_OVER>   (d, d, s, xspan, 0xff); break;
        case COMBINE_UNDER:   pixel_combine<COMBINE_UNDER>  (d, d, s, xspan, 0xff); break;
        case COMBINE_ADD:     pixel_combine<COMBINE_ADD>    (d, d, s, xspan, 0xff); break;
        case COMBINE_DEL:     pixel_combine<COMBINE_DEL>    (d, d, s, xspan, 0xff); break;
        case COMBINE_ATOP:    pixel_combine<COMBINE_ATOP>   (d, d, s, xspan, 0xff); break;
        case COMBINE_XOR:     pixel_combine<COMBINE_XOR>    (d, d, s, xspan, 0xff); break;
        case COMBINE_BLEND:   pixel_combine<COMBINE_BLEND>  (d, d, s, xspan, 0xff); break;
        case COMBINE_VALUE:   pixel_combine<COMBINE_VALUE>  (d, d, s, xspan, 0xff); break;
        }
    }
}

Affine
Affine::from_triangles (Point src_a, Point src_b, Point src_c,
                        Point dst_a, Point dst_b, Point dst_c)
{
  const double Ax = src_a.x, Ay = src_a.y;
  const double Bx = src_b.x, By = src_b.y;
  const double Cx = src_c.x, Cy = src_c.y;
  const double ax = dst_a.x, ay = dst_a.y;
  const double bx = dst_b.x, by = dst_b.y;
  const double cx = dst_c.x, cy = dst_c.y;
  
  /* solve the linear equation system:
   *   ax = Ax * matrix.xx + Ay * matrix.xy + matrix.xz;
   *   ay = Ax * matrix.yx + Ay * matrix.yy + matrix.yz;
   *   bx = Bx * matrix.xx + By * matrix.xy + matrix.xz;
   *   by = Bx * matrix.yx + By * matrix.yy + matrix.yz;
   *   cx = Cx * matrix.xx + Cy * matrix.xy + matrix.xz;
   *   cy = Cx * matrix.yx + Cy * matrix.yy + matrix.yz;
   * for matrix.*
   */
  double AxBy = Ax * By, AyBx = Ay * Bx;
  double AxCy = Ax * Cy, AyCx = Ay * Cx;
  double BxCy = Bx * Cy, ByCx = By * Cx;
  double divisor = AxBy - AyBx - AxCy + AyCx + BxCy - ByCx;
  
  if (fabs (divisor) < 1e-9)
    return false;
  
  Affine matrix;
  matrix.xx = By * ax - Ay * bx + Ay * cx - Cy * ax - By * cx + Cy * bx;
  matrix.yx = By * ay - Ay * by + Ay * cy - Cy * ay - By * cy + Cy * by;
  matrix.xy = Ax * bx - Bx * ax - Ax * cx + Cx * ax + Bx * cx - Cx * bx;
  matrix.yy = Ax * by - Bx * ay - Ax * cy + Cx * ay + Bx * cy - Cx * by;
  matrix.xz = AxBy * cx - AxCy * bx - AyBx * cx + AyCx * bx + BxCy * ax - ByCx * ax;
  matrix.yz = AxBy * cy - AxCy * by - AyBx * cy + AyCx * by + BxCy * ay - ByCx * ay;
  double rec_divisor = 1.0 / divisor;
  matrix.xx *= rec_divisor;
  matrix.xy *= rec_divisor;
  matrix.xz *= rec_divisor;
  matrix.yx *= rec_divisor;
  matrix.yy *= rec_divisor;
  matrix.yz *= rec_divisor;
  
  return true;
}

String
Affine::string() const
{
  char buffer[6 * 64 + 128];
  sprintf (buffer, "{ { %.17g, %.17g, %.17g }, { %.17g, %.17g, %.17g } }",
           xx, xy, xz, yx, yy, yz);
  return String (buffer);
}

} // Rapicorn
