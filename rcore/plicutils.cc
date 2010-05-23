/* Plic Binding utilities
 * Copyright (C) 2010 Tim Janik
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
#include "plicutils.hh"
#include <assert.h>

/* === Auxillary macros === */
#define PLIC_CPP_PASTE2i(a,b)                   a ## b // indirection required to expand __LINE__ etc
#define PLIC_CPP_PASTE2(a,b)                    PLIC_CPP_PASTE2i (a,b)
#define PLIC_STATIC_ASSERT_NAMED(expr,asname)   typedef struct { char asname[(expr) ? 1 : -1]; } PLIC_CPP_PASTE2 (Plic_StaticAssertion_LINE, __LINE__)
#define PLIC_STATIC_ASSERT(expr)                PLIC_STATIC_ASSERT_NAMED (expr, compile_time_assertion_failed)
#define ALIGN4(sz,unit)                         (sizeof (unit) * ((sz + sizeof (unit) - 1) / sizeof (unit)))

namespace Plic {

FieldBuffer::FieldBuffer (uint _ntypes) :
  buffermem (NULL)
{
  PLIC_STATIC_ASSERT (sizeof (FieldBuffer) <= sizeof (FieldUnion));
  // buffermem layout: [{n_types,nth}] [{type nibble} * n_types]... [field]...
  const uint _offs = 1 + (_ntypes + 7) / 8;
  buffermem = new FieldUnion[_offs + _ntypes];
  wmemset ((wchar_t*) buffermem, 0, sizeof (FieldUnion[_offs]) / sizeof (wchar_t));
  buffermem[0].capacity = _ntypes;
  buffermem[0].index = 0;
}

FieldBuffer::FieldBuffer (uint        _ntypes,
                          FieldUnion *_bmem,
                          uint        _bmemlen) :
  buffermem (_bmem)
{
  const uint _offs = 1 + (_ntypes + 7) / 8;
  assert (_bmem && _bmemlen >= sizeof (FieldUnion[_offs + _ntypes]));
  wmemset ((wchar_t*) buffermem, 0, sizeof (FieldUnion[_offs]) / sizeof (wchar_t));
  buffermem[0].capacity = _ntypes;
  buffermem[0].index = 0;
}

FieldBuffer::~FieldBuffer()
{
  reset();
  if (buffermem)
    delete [] buffermem;
}

class OneChunkFieldBuffer : public FieldBuffer {
  virtual ~OneChunkFieldBuffer () { reset(); buffermem = NULL; }
  explicit OneChunkFieldBuffer (uint        _ntypes,
                                FieldUnion *_bmem,
                                uint        _bmemlen) :
    FieldBuffer (_ntypes, _bmem, _bmemlen)
  {}
public:
  static OneChunkFieldBuffer*
  _new (uint _ntypes)
  {
    const uint _offs = 1 + (_ntypes + 7) / 8;
    size_t bmemlen = sizeof (FieldUnion[_offs + _ntypes]);
    size_t objlen = ALIGN4 (sizeof (OneChunkFieldBuffer), int64);
    uint8 *omem = new uint8[objlen + bmemlen];
    FieldUnion *bmem = (FieldUnion*) (omem + objlen);
    return new (omem) OneChunkFieldBuffer (_ntypes, bmem, bmemlen);
  }
};

FieldBuffer*
FieldBuffer::_new (uint _ntypes)
{
  return OneChunkFieldBuffer::_new (_ntypes);
}

} // Plic
