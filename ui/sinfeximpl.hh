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
#ifndef __RAPICORN_SINFEX_IMPL_HH__
#define __RAPICORN_SINFEX_IMPL_HH__

#include "sinfex.hh"
#include <math.h>

namespace Rapicorn {

typedef enum {
  SINFEX_0,
  SINFEX_REAL,
  SINFEX_STRING,
  SINFEX_VARIABLE,
  SINFEX_ENTITY_VARIABLE,
  SINFEX_OR,
  SINFEX_AND,
  SINFEX_NOT,
  SINFEX_NEG,
  SINFEX_POS,
  SINFEX_ADD,
  SINFEX_SUB,
  SINFEX_MUL,
  SINFEX_DIV,
  SINFEX_POW,
  SINFEX_EQ,
  SINFEX_NE,
  SINFEX_LT,
  SINFEX_GT,
  SINFEX_LE,
  SINFEX_GE,
  SINFEX_ARG,
  SINFEX_FUNCTION,
} SinfexOp;

class SinfexExpressionStack {
  uint *start;
  union {
    uint   *up;
    char   *cp;
    double *dp;
  } mark;
  void
  grow (uint free_bytes = 128)
  {
    uint size = start ? start[0] : 0;
    uint current = mark.cp - (char*) start;
    if (current + free_bytes > size)
      {
        const uint alignment = 256;
        size = (size + free_bytes + alignment - 1) / alignment;
        size *= alignment;
        start = (uint*) realloc (start, size);
        if (!start)
          error ("SinfexExpressionStack: out of memory (trying to allocate %u bytes)", size);
        mark.cp = current + (char*) start;
        start[0] = size;
      }
  }
  uint index      () const   { return mark.up - start; }
  void put_double (double d) { grow (sizeof (d)); *mark.dp++ = d; }
  void put_uint   (uint   u) { grow (sizeof (u)); *mark.up++ = u; }
  void
  put_string (String s)
  {
    uint l = s.size();
    grow (sizeof (l) + l);
    *mark.up++ = l;
    memcpy (mark.cp, &s[0], l);
    mark.cp += l;
    while (ptrdiff_t (mark.cp) & 3)
      *mark.cp++ = 0;
  }
public:
  SinfexExpressionStack () :
    start (NULL), mark ()
  {
    mark.up = start;
    grow (sizeof (*mark.up));
    mark.up++;          // byte_size
    put_uint (0);      // start offset
  }
  ~SinfexExpressionStack ()
  {
    if (start)
      free (start);
    mark.up = start = NULL;
  }
  uint* startmem    () const                { return start; }
  uint  push_or     (uint ex1, uint ex2)    { uint ix = index(); put_uint (SINFEX_OR);  put_uint (ex1); put_uint (ex2); return ix; }
  uint  push_and    (uint ex1, uint ex2)    { uint ix = index(); put_uint (SINFEX_AND); put_uint (ex1); put_uint (ex2); return ix; }
  uint  push_not    (uint ex1)              { uint ix = index(); put_uint (SINFEX_NOT); put_uint (ex1); return ix; }
  uint  push_neg    (uint ex1)              { uint ix = index(); put_uint (SINFEX_NEG); put_uint (ex1); return ix; }
  uint  push_pos    (uint ex1)              { uint ix = index(); put_uint (SINFEX_POS); put_uint (ex1); return ix; }
  uint  push_add    (uint ex1, uint ex2)    { uint ix = index(); put_uint (SINFEX_ADD); put_uint (ex1); put_uint (ex2); return ix; }
  uint  push_sub    (uint ex1, uint ex2)    { uint ix = index(); put_uint (SINFEX_SUB); put_uint (ex1); put_uint (ex2); return ix; }
  uint  push_mul    (uint ex1, uint ex2)    { uint ix = index(); put_uint (SINFEX_MUL); put_uint (ex1); put_uint (ex2); return ix; }
  uint  push_div    (uint ex1, uint ex2)    { uint ix = index(); put_uint (SINFEX_DIV); put_uint (ex1); put_uint (ex2); return ix; }
  uint  push_pow    (uint ex1, uint ex2)    { uint ix = index(); put_uint (SINFEX_POW); put_uint (ex1); put_uint (ex2); return ix; }
  uint  push_eq     (uint ex1, uint ex2)    { uint ix = index(); put_uint (SINFEX_EQ);  put_uint (ex1); put_uint (ex2); return ix; }
  uint  push_ne     (uint ex1, uint ex2)    { uint ix = index(); put_uint (SINFEX_NE);  put_uint (ex1); put_uint (ex2); return ix; }
  uint  push_lt     (uint ex1, uint ex2)    { uint ix = index(); put_uint (SINFEX_LT);  put_uint (ex1); put_uint (ex2); return ix; }
  uint  push_gt     (uint ex1, uint ex2)    { uint ix = index(); put_uint (SINFEX_GT);  put_uint (ex1); put_uint (ex2); return ix; }
  uint  push_le     (uint ex1, uint ex2)    { uint ix = index(); put_uint (SINFEX_LE);  put_uint (ex1); put_uint (ex2); return ix; }
  uint  push_ge     (uint ex1, uint ex2)    { uint ix = index(); put_uint (SINFEX_GE);  put_uint (ex1); put_uint (ex2); return ix; }
  uint  push_arg    (uint ex1, uint ex2)    { uint ix = index(); put_uint (SINFEX_ARG); put_uint (ex1); put_uint (ex2); return ix; }
  uint  push_double (double d)              { uint ix = index(); put_uint (SINFEX_REAL); put_double (d); return ix; }
  uint  push_string (const String &s)       { uint ix = index(); put_uint (SINFEX_STRING); put_string (s); return ix; }
  uint  push_variable  (const String &name) { uint ix = index(); put_uint (SINFEX_VARIABLE); put_string (name); return ix; }
  uint
  push_entity_variable (const String &entity,
                        const String &name)
  { uint ix = index();
    put_uint (SINFEX_ENTITY_VARIABLE);
    put_string (entity);
    put_string (name);
    return ix;
  }
  uint
  push_func (const String &func_name,
             uint          argx1)
  { uint ix = index();
    put_uint (SINFEX_FUNCTION);
    put_uint (argx1);
    put_string (func_name);
    return ix;
  }
  void
  set_start (uint ex1)
  {
    assert (start[1] == 0);
    start[1] = ex1;
  }
};

} // Rapicorn

#endif  /* __RAPICORN_SINFEX_IMPL_HH__ */
