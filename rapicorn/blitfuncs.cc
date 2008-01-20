/* Rapicorn
 * Copyright (C) 2008 Tim Janik
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
#include "blitfuncs.hh"

namespace Rapicorn {
namespace Blit {

static inline uint32
quick_rand32 ()
{
  static uint32 accu = 2147483563;
  accu = 1664525 * accu + 1013904223;
  return accu;
}

static void
render_gradient_line (uint32 *pixel,
                      uint32 *bound,
                      uint32  col1,
                      uint32  col2)
{
  uint32 Ca = COLA (col1) << 16, Cr = COLR (col1) << 16, Cg = COLG (col1) << 16, Cb = COLB (col1) << 16;
  int32 Da = (COLA (col2) << 16) - Ca, Dr = (COLR (col2) << 16) - Cr;
  int32 Dg = (COLG (col2) << 16) - Cg, Db = (COLB (col2) << 16) - Cb;
  int delta = bound - pixel - 1;
  if (delta)
    {
      Da /= delta;
      Dr /= delta;
      Dg /= delta;
      Db /= delta;
    }
  while (pixel < bound)
    {
      union { uint32 i32; uint8 b[4]; } rnd = { quick_rand32() };
      *pixel = COL_ARGB ((Ca + (rnd.b[3] * 0x0101)) >> 16,
                         (Cr + (rnd.b[2] * 0x0101)) >> 16,
                         (Cg + (rnd.b[1] * 0x0101)) >> 16,
                         (Cb + (rnd.b[0] * 0x0101)) >> 16);
      Ca += Da;
      Cr += Dr;
      Cg += Dg;
      Cb += Db;
      pixel++;
    }
}

RenderTable render = {
  render_gradient_line,
};

} // Blit
} // Rapicorn
