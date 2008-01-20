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

void
render_optimize_mmx (void)
{
}

#else  /* !__MMX__ */
void
render_optimize_mmx (void)
{}
#endif /* !__MMX__ */
} // Blit
} // Rapicorn
