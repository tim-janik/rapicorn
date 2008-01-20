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
#ifdef __MMX__
#include <mmintrin.h>
#endif /* __MMX__ */

namespace Rapicorn {
namespace Blit {
#ifdef __MMX__

#define mmx_zero                        ((__m64) 0x0000000000000000ULL)
#define mmx_shiftl(v,bits)              (_mm_slli_si64 ((v), (bits)))   // v << bits
#define mmx_shiftr(v,bits)              (_mm_srli_si64 ((v), (bits)))   // v >> bits

static inline __m64
load_hi32lo32 (uint32_t a,
               uint32_t b)
{
  __m64 mhi = _mm_cvtsi32_si64 (a);     // 00 00 00 00 a3 a2 a1 a0
  __m64 mlo = _mm_cvtsi32_si64 (b);     // 00 00 00 00 b3 b2 b1 b0
  mhi = mmx_shiftl (mhi, 32);           // a3 a2 a1 a0 00 00 00 00
  return _mm_or_si64 (mhi, mlo);        // a3 a2 a1 a0 b3 b2 b1 b0
}


static inline __m64
packbyte_8x8 (__m64 a, // 4x16
              __m64 c) // 4x16
{
  // US = unsigned saturation, i.e. with (uint8) CLAMP (uint16), produce:
  // US (c7c6) US (c5c4) US (c3c2) US (c1c0) US (a7a6) US (a5a4) US (a3a2) US (a1a0)
  return _mm_packs_pu16 (a, c); // US: CCccCCcc AAaaAAaa
}

static inline uint32
store_00008888 (__m64 vv)               // vv = AA aa BB bb CC cc DD dd
{
  __m64 zz = _mm_setzero_si64();        // zz = 00 00 00 00 00 00 00 00
  __m64 ip = packbyte_8x8 (vv, zz);     // ip = US (ZZzzZZzz AaBbCcDd)
  return _mm_cvtsi64_si32 (ip);         // return 0x00000000ffffffff & ip
}

static inline uint32_t
color_agrb24 (__m64 ag24,                       // 00 aa .. .. 00 gg .. ..
              __m64 rb24)                       // 00 rr .. .. 00 bb .. ..
{
  const __m64 mmx_f0f0 = (__m64) 0xffff0000ffff0000ULL;
  ag24 = _mm_and_si64 (ag24, mmx_f0f0);         // 00 aa 00 00 00 gg 00 00
  rb24 = _mm_and_si64 (rb24, mmx_f0f0);         // 00 rr 00 00 00 bb 00 00
  rb24 = mmx_shiftr (rb24, 16);                 // 00 00 00 rr 00 00 00 bb
  __m64 mtmp = _mm_or_si64 (ag24, rb24);        // 00 aa 00 rr 00 gg 00 bb
  return store_00008888 (mtmp);                 // argb
}

static inline __m64
mmx_dither_rand (__m64 maccu)
{
  // 2^14-period mod16 generator: accu = (accu * 47989) & 0xffff;
  const __m64 mfact = (__m64) 0xbb75bb75bb75bb75ULL;
  return _mm_mullo_pi16 (maccu, mfact); // maccu_ * mfact_
}
static __m64 mmx_dither_accu = (__m64) 0xa9d1878556c9bcddULL; // 4095-steps apart seeds

static void
mmx_gradient_line (uint32 *pixel,
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

  const bool ditherp = 1;
  const uint32 roundoffs = ditherp ? 0 : 0x7fff;                // rounding offset when not dithering
  __m64 maccu = mmx_dither_accu;
  __m64 mdag = load_hi32lo32 (Da, Dg);                          // 00 aa .. .. 00 gg .. .. (mdag = alpha green)
  __m64 mdrb = load_hi32lo32 (Dr, Db);                          // 00 rr .. .. 00 bb .. .. (mdrb =   red  blue)
  __m64 mcag = load_hi32lo32 (Ca + roundoffs, Cg + roundoffs);  // 00 aa 7f ff 00 gg 7f ff (mcag = alpha green)
  __m64 mcrb = load_hi32lo32 (Cr + roundoffs, Cb + roundoffs);  // 00 rr 7f ff 00 bb 7f ff (mcrb =   red  blue)
  while (pixel < bound)
    {
      maccu = mmx_dither_rand (maccu);                          // rand3 rand2 rand1 rand0
      __m64 mvag = _mm_unpacklo_pi16 (maccu, mmx_zero);         // 00 00 rand1 00 00 rand0
      __m64 mvrb = _mm_unpackhi_pi16 (maccu, mmx_zero);         // 00 00 rand3 00 00 rand2
      mvag = _mm_add_pi32 (mcag, mvag);                         // mcag_ + dither
      mvrb = _mm_add_pi32 (mcrb, mvrb);                         // mcrb_ + dither

      if (ditherp)
        *pixel = color_agrb24 (mvag, mvrb);
      else
        *pixel = color_agrb24 (mcag, mcrb);

      mcag = _mm_add_pi32 (mcag, mdag);                         // mcag += mdag
      mcrb = _mm_add_pi32 (mcrb, mdrb);                         // mcrb += mdrb

      // printf ("pixel(%p): 0x%08x\n", pixel, *pixel);
      pixel++;
    }
  mmx_dither_accu = maccu;
  _mm_empty();  // cleanup FPU after MMX use
}

void
render_optimize_mmx (void)
{
  render.gradient_line = mmx_gradient_line;
}

#else  /* !__MMX__ */
void
render_optimize_mmx (void)
{}
#endif /* !__MMX__ */
} // Blit
} // Rapicorn
