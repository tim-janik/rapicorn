// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
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
load_08080808 (uint32 vv)
{
  __m64 mi = _mm_cvtsi32_si64 (vv);             // 00 00 00 00 v3 v2 v1 v0
  return _mm_unpacklo_pi8 (mi, mmx_zero);       // 00 v3 00 v2 00 v1 00 v0
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
store_00008888 (__m64 vv)                       // vv = AA aa BB bb CC cc DD dd
{
  __m64 ip = packbyte_8x8 (vv, mmx_zero);       // ip = US (ZZzzZZzz AaBbCcDd)
  return _mm_cvtsi64_si32 (ip);                 // return 0x00000000ffffffff & ip
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
pixmul_4x16 (__m64 a,
             __m64 b)
{
  // scaled 16bit multiplication so that 255*255=255 for the 4x16bit vectors a and b
  __m64 mask = (__m64) 0x0080008000800080ULL;
  __m64 res;
  res = _mm_mullo_pi16 (a, b);          // 16bit: res_ = a_ * b_
  res = _mm_adds_pu16 (res, mask);      // 16bit: res_ = SATURATE (res_ + 0x80)
  __m64 tmp = _mm_srli_pi16 (res, 8);   // 16bit: tmp_ = res_ >> 8
  res = _mm_adds_pu16 (res, tmp);       // 16bit: res_ = SATURATE (res_ + tmp_)
  res = _mm_srli_pi16 (res, 8);         // 16bit: res_ = res_ >> 8
  return res;   // return (a_ * b_ + 0x80 + ((a_ * b_ + 0x80) >> 8)) >> 8
}

static inline __m64
expand_alpha_4x16 (__m64 pixel4)
{
  // expand alpha 4 times, the extracted alpha should be <= 255
  __m64 t1, t2;
  t1 = mmx_shiftr (pixel4, 48); // t1 = pixel4 >> 48
  t2 = mmx_shiftl (t1, 16);     // t2 = t1 << 16
  t1 = _mm_or_si64 (t1, t2);    // t1 |= t2
  t2 = mmx_shiftl (t1, 32);     // t2 = t1 << 32
  t1 = _mm_or_si64 (t1, t2);    // t1 |= t2
  return t1;    // AAaa = alpha16 (pixel4); return AA aa AA aa AA aa AA aa
}

static inline __m64
combine_over_4x16 (__m64 src,      // 00aa 00rr 00gg 00bb
                   __m64 srcalpha, // 00aa 00aa 00aa 00aa
                   __m64 dst)      // 00aa 00rr 00gg 00bb
{
  // src OVER dst = src + dest * (255 - alpha)
  __m64 m0f = (__m64) 0x00ff00ff00ff00ffULL;
  __m64 malpha = _mm_xor_si64 (srcalpha, m0f);  // malpha_ = 255 - srcalpha_
  __m64 apix = pixmul_4x16 (dst, malpha);       // apix_ = dst_ * malpha_
  return _mm_adds_pu8 (src, apix);              // return src_ + apix_
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
mmx_clear_fpu (void)
{
  _mm_empty();  // cleanup FPU after MMX use
}

static void
mmx_combine_over (uint32       *dst,
                  const uint32 *src,
                  uint32        length)
{
  uint32 *bound = dst + length;
  while (dst < bound)
    {
      uint32 s = *src++, a = s >> 24;
      if (a == 0xff)
        *dst = s;
      else if (a)
        {
          __m64 msrc = load_08080808 (s);
          __m64 mals = expand_alpha_4x16 (msrc);
          __m64 mdst = load_08080808 (*dst);
          __m64 mcmb = combine_over_4x16 (msrc, mals, mdst);
          *dst = store_00008888 (mcmb);
        }
      dst++;
    }
}

#define DITHER  1

static void
mmx_gradient_line (uint32 *pixel,
                   uint32 *bound,
                   uint32  alpha1pre16,
                   uint32  red1pre16,
                   uint32  green1pre16,
                   uint32  blue1pre16,
                   uint32  alpha2pre16,
                   uint32  red2pre16,
                   uint32  green2pre16,
                   uint32  blue2pre16)
{
  const uint32 Ca = alpha1pre16, Cr = red1pre16, Cg = green1pre16, Cb = blue1pre16;
  int32 Da = alpha2pre16 - Ca, Dr = red2pre16 - Cr;
  int32 Dg = green2pre16 - Cg, Db = blue2pre16 - Cb;
  int delta = bound - pixel - 1;
  if (delta)
    {
      Da /= delta;
      Dr /= delta;
      Dg /= delta;
      Db /= delta;
    }

  const uint32 roundoffs = DITHER ? 0 : 0x7fff;                 // rounding offset when not dithering
  __m64 maccu = mmx_dither_accu;
  __m64 mdag = load_hi32lo32 (Da, Dg);                          // 00 aa .. .. 00 gg .. .. (mdag = alpha green)
  __m64 mdrb = load_hi32lo32 (Dr, Db);                          // 00 rr .. .. 00 bb .. .. (mdrb =   red  blue)
  __m64 mcag = load_hi32lo32 (Ca + roundoffs, Cg + roundoffs);  // 00 aa 7f ff 00 gg 7f ff (mcag = alpha green)
  __m64 mcrb = load_hi32lo32 (Cr + roundoffs, Cb + roundoffs);  // 00 rr 7f ff 00 bb 7f ff (mcrb =   red  blue)
  while (pixel < bound)
    {
#if DITHER
      maccu = mmx_dither_rand (maccu);                          // rand3 rand2 rand1 rand0
      __m64 mvag = _mm_unpacklo_pi16 (maccu, mmx_zero);         // 00 00 rand1 00 00 rand0
      __m64 mvrb = _mm_unpackhi_pi16 (maccu, mmx_zero);         // 00 00 rand3 00 00 rand2
      mvag = _mm_add_pi32 (mcag, mvag);                         // mcag_ + dither
      mvrb = _mm_add_pi32 (mcrb, mvrb);                         // mcrb_ + dither
      *pixel = color_agrb24 (mvag, mvrb);
#else
      *pixel = color_agrb24 (mcag, mcrb);
#endif
      mcag = _mm_add_pi32 (mcag, mdag);                         // mcag += mdag
      mcrb = _mm_add_pi32 (mcrb, mdrb);                         // mcrb += mdrb

      // printf ("pixel(%p): 0x%08x\n", pixel, *pixel);
      pixel++;
    }
  mmx_dither_accu = maccu;
}

void
render_optimize_mmx (void)
{
  render.clear_fpu = mmx_clear_fpu;
  render.gradient_line = mmx_gradient_line;
  render.combine_over = mmx_combine_over;
}

#else  /* !__MMX__ */
void
render_optimize_mmx (void)
{}
#endif /* !__MMX__ */
} // Blit
} // Rapicorn
