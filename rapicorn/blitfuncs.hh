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
#ifndef __RAPICORN_BLIT_FUNCS_HH__
#define __RAPICORN_BLIT_FUNCS_HH__

#include <rapicorn/events.hh>

#ifndef RAPICORN_INTERNALS
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

void    render_gradient_line            (uint32 *pixel,
                                         uint32 *bound,
                                         uint32  col1,
                                         uint32  col2);


} // Blit
} // Rapicorn

#endif  /* __RAPICORN_BLIT_FUNCS_HH__ */
