// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_BLIT_FUNCS_HH__
#define __RAPICORN_BLIT_FUNCS_HH__

#include <ui/events.hh>

#ifndef __RAPICORN_BUILD__
#error  This header file is for Rapicorn internal use only.
#endif

namespace Rapicorn {
namespace Blit {


#if __BYTE_ORDER == __LITTLE_ENDIAN
#define COLA(argb)              ({ union { uint32 w; uint8 b[4]; } u; u.w = (argb); u.b[3]; })
#define COLR(argb)              ({ union { uint32 w; uint8 b[4]; } u; u.w = (argb); u.b[2]; })
#define COLG(argb)              ({ union { uint32 w; uint8 b[4]; } u; u.w = (argb); u.b[1]; })
#define COLB(argb)              ({ union { uint32 w; uint8 b[4]; } u; u.w = (argb); u.b[0]; })
#else
#define COLA(argb)              ({ union { uint32 w; uint8 b[4]; } u; u.w = (argb); u.b[0]; })
#define COLR(argb)              ({ union { uint32 w; uint8 b[4]; } u; u.w = (argb); u.b[1]; })
#define COLG(argb)              ({ union { uint32 w; uint8 b[4]; } u; u.w = (argb); u.b[2]; })
#define COLB(argb)              ({ union { uint32 w; uint8 b[4]; } u; u.w = (argb); u.b[3]; })
#endif
#define COL_ARGB(a,r,g,b)       (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))

struct RenderTable {
  void  (*clear_fpu)            (void);
  void  (*combine_over)         (uint32       *dst,
                                 const uint32 *src,
                                 uint          span);
  void  (*gradient_line)        (uint32       *pixel,
                                 uint32       *bound,
                                 uint32        alpha1pre16,
                                 uint32        red1pre16,
                                 uint32        green1pre16,
                                 uint32        blue1pre16,
                                 uint32        alpha2pre16,
                                 uint32        red2pre16,
                                 uint32        green2pre16,
                                 uint32        blue2pre16);
};
extern RenderTable render;

void    render_optimize_mmx     (void);


} // Blit
} // Rapicorn

#endif  /* __RAPICORN_BLIT_FUNCS_HH__ */
