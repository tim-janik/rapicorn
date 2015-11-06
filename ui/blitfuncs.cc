// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "blitfuncs.hh"

namespace Rapicorn {
namespace Blit {

static void
init_render_table (const StringVector &args)
{
  const bool have_mmx = strstr (cpu_info().c_str(), " MMX ");
  assert (have_mmx);
  if (have_mmx)
    {
      Blit::render_optimize_mmx();
      RAPICORN_STARTUP_DEBUG ("Using MMX functions for blitting");
    }
}
static InitHook _init_render_table ("ui-core/10 Optimize Render Table for SIMD Blitting", init_render_table);

static inline uint32
quick_rand32 ()
{
  static uint32 accu = 2147483563;
  accu = 1664525 * accu + 1013904223;
  return accu;
}

static void
alu_gradient_line (uint32 *pixel,
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
  uint32 Ca = alpha1pre16, Cr = red1pre16, Cg = green1pre16, Cb = blue1pre16;
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

static void
alu_combine_over (uint32 *dst, const uint32 *src, uint span)
{
  const uint32 *limit = dst + span;
  while (dst < limit)
    {
      /* extract alpha, red, green, blue */
      uint32 Aa = COLA (*src), Ar = COLR (*src), Ag = COLG (*src), Ab = COLB (*src);
      uint32 Ba = COLA (*dst), Br = COLR (*dst), Bg = COLG (*dst), Bb = COLB (*dst);
      /* A over B = colorA + colorB * (1 - alphaA) */
      uint32 Ai = 255 - Aa;
      uint8 Dr = Ar + Color::IMUL (Br, Ai);
      uint8 Dg = Ag + Color::IMUL (Bg, Ai);
      uint8 Db = Ab + Color::IMUL (Bb, Ai);
      uint8 Da = Aa + Color::IMUL (Ba, Ai);
      /* assign */
      *dst++ = COL_ARGB (Da, Dr, Dg, Db);
      src++;
    }
}

static void
nop_clear_fpu (void)
{
  /* clear render state for FPU use */
}

RenderTable render = {
  nop_clear_fpu,
  alu_combine_over,
  alu_gradient_line,
};

} // Blit
} // Rapicorn
