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

String
string_cunquote (const String &input) // FIXME
{
  uint i = 0;
  if (i < input.size() && (input[i] == '"' || input[i] == '\''))
    {
      const char qchar = input[i];
      i++;
      String out;
      bool be = false;
      while (i < input.size() && (input[i] != qchar || be))
        {
          if (!be && input[i] == '\\')
            be = true;
          else
            {
              if (be)
                switch (input[i])
                  {
                  case 'n':     out += '\n';            break;
                  case 'r':     out += '\r';            break;
                  case 't':     out += '\t';            break;
                  case 'b':     out += '\b';            break;
                  case 'f':     out += '\f';            break;
                  case 'v':     out += '\v';            break;
                  default:      out += input[i];        break;
                  }
              else
                out += input[i];
              be = false;
            }
          i++;
        }
      if (i < input.size() && input[i] == qchar)
        {
          i++;
          if (i < input.size())
            return input; // extraneous characters after string quotes
          return out;
        }
      else
        return input; // unclosed string quotes
    }
  else if (i == input.size())
    return input; // empty string arg: ""
  else
    return input; // missing string quotes
}

class Value {
  const bool   m_if;
  const double m_real;
  const String m_string;
public:
  bool        isreal   () const { return m_if; }
  bool        isstring () const { return !m_if; }
  double      real     () const { return m_if ? m_real : 0.0; }
  String      string   () const { return !m_if ? m_string : ""; }
  bool        asbool   () const { return (m_if && m_real) || (!m_if && m_string != ""); }
  explicit    Value    (double ldr) : m_if (1), m_real (ldr) {}
  explicit    Value    (const String &s) : m_if (0), m_real (0), m_string (string_cunquote (s)) {}
  String      tostring ()
  {
    String vstr = string();
    if (isreal())
      {
        char buffer[128];
        snprintf (buffer, 128, "%.15g", real());
        vstr = buffer;
      }
    return vstr;
  }
};

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
    while ((int) mark.cp & 3)
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
  uint  push_variable  (const char *name)   { uint ix = index(); put_uint (SINFEX_VARIABLE); put_string (name); return ix; }
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

class SinfexExpression {
  uint *start;
  Value eval_op (uint opx);
  inline int
  cmp_values (const Value &a, const Value &b)
  {
    if (RAPICORN_UNLIKELY (a.isstring()))
      {
        if (RAPICORN_UNLIKELY (!b.isstring()))
          return 1;
        return strcmp (a.string().c_str(), b.string().c_str());
      }
    else
      {
        if (RAPICORN_UNLIKELY (b.isstring()))
          return -1;
        return a.real() - b.real() < 0 ? -1 : a.real() - b.real() > 0 ? +1 : 0;
      }
  }
  RAPICORN_PRIVATE_CLASS_COPY (SinfexExpression);
public:
  SinfexExpression (const SinfexExpressionStack &estk) :
    start (NULL)
  {
    const uint *prog = estk.startmem();
    if (prog)
      {
        start = new uint[prog[0] / sizeof (uint)];
        memcpy (start, prog, prog[0]);
      }
  }
  ~SinfexExpression ()
  {
    if (start)
      {
        delete[] start;
        start = NULL;
      }
  }
  Value
  eval (void*)
  {
    if (!start || !start[1])
      return Value (0);
    else
      return eval_op (start[1]);
  }
};

Value
SinfexExpression::eval_op (uint opx)
{
  union {
    const uint   *up;
    const int    *ip;
    const char   *cp;
    const double *dp;
  } mark;
  mark.up = start + opx;
  assert (mark.up + 1 <= start + start[0]);
  switch (*mark.up++)
    {
      uint ui;
    case SINFEX_0:      return Value (0);
    case SINFEX_REAL:   return Value (*mark.dp);
    case SINFEX_STRING:
      ui = *mark.up++;
      return Value (String (mark.cp, ui));
    case SINFEX_VARIABLE:
      ui = *mark.up++;
      printout ("FIXME:VAR: %s\n", String (mark.cp, ui).c_str());
      return Value (0);
    case SINFEX_ENTITY_VARIABLE:
      {
        String v1, v2;
        ui = *mark.up++;
        v1 = String (mark.cp, ui);
        mark.up += (ui + 3) / 4;
        ui = *mark.up++;
        v2 = String (mark.cp, ui);
        printout ("VAR: %s.%s\n", v1.c_str(), v2.c_str());
      }
      return Value (0);
    case SINFEX_OR:
      {
        const Value &a = eval_op (*mark.up++), &b = eval_op (*mark.up++);
        return Value (a.asbool() ? a : b);
      }
    case SINFEX_AND:
      {
        const Value &a = eval_op (*mark.up++), &b = eval_op (*mark.up++);
        return Value (a.asbool() ? b : a);
      }
    case SINFEX_NOT:
      {
        const Value &a = eval_op (*mark.up++);
        return Value (!a.asbool());
      }
    case SINFEX_NEG:
      {
        const Value &a = eval_op (*mark.up++);
        if (a.isreal())
          return Value (-a.real());
        error ("incompatible value type for unary sign");
      }
    case SINFEX_POS:
      {
        const Value &a = eval_op (*mark.up++);
        if (a.isreal())
          return a;
        error ("incompatible value type for unary sign");
      }
    case SINFEX_ADD:
      {
        const Value &a = eval_op (*mark.up++), &b = eval_op (*mark.up++);
        if (a.isreal() && b.isreal())
          return Value (a.real() + b.real());
        else if (a.isstring() && b.isstring())
          return Value (a.string() + b.string());
        else
          error ("incompatible value types in '+'\n");
      }
    case SINFEX_SUB:
      {
        const Value &a = eval_op (*mark.up++), &b = eval_op (*mark.up++);
        if (a.isreal() && b.isreal())
          return Value (a.real() - b.real());
        else
          error ("incompatible value types in '-'\n");
      }
    case SINFEX_MUL:
      {
        const Value &a = eval_op (*mark.up++), &b = eval_op (*mark.up++);
        if (a.isreal() && b.isreal())
          return Value (a.real() * b.real());
        else
          error ("incompatible value types in '*'\n");
      }
    case SINFEX_DIV:
      {
        const Value &a = eval_op (*mark.up++), &b = eval_op (*mark.up++);
        if (a.isreal() && b.isreal())
          return Value (b.real() != 0.0 ? a.real() / b.real() : nanl (0));
        else
          error ("incompatible value types in '/'\n");
      }
    case SINFEX_POW:
      {
        const Value &a = eval_op (*mark.up++), &b = eval_op (*mark.up++);
        if (a.isreal() && b.isreal())
          return Value (pow (a.real(), b.real()));
        else
          error ("incompatible value types in '*'\n");
      }
    case SINFEX_EQ:
      {
        const Value &a = eval_op (*mark.up++), &b = eval_op (*mark.up++);
        return Value (cmp_values (a, b) == 0);
      }
    case SINFEX_NE:
      {
        const Value &a = eval_op (*mark.up++), &b = eval_op (*mark.up++);
        return Value (cmp_values (a, b) != 0);
      }
    case SINFEX_LT:
      {
        const Value &a = eval_op (*mark.up++), &b = eval_op (*mark.up++);
        return Value (cmp_values (a, b) < 0);
      }
    case SINFEX_GT:
      {
        const Value &a = eval_op (*mark.up++), &b = eval_op (*mark.up++);
        return Value (cmp_values (a, b) > 0);
      }
    case SINFEX_LE:
      {
        const Value &a = eval_op (*mark.up++), &b = eval_op (*mark.up++);
        return Value (cmp_values (a, b) <= 0);
      }
    case SINFEX_GE:
      {
        const Value &a = eval_op (*mark.up++), &b = eval_op (*mark.up++);
        return Value (cmp_values (a, b) >= 0);
      }
    case SINFEX_FUNCTION:
      {
        uint arg1 = *mark.up++;
        ui = *mark.up++;
        printout ("FIXME:FUNC: %s (->ARGS:%u)\n", String (mark.cp, ui).c_str(), arg1);
        return Value (0);
      }
    case SINFEX_ARG: ;
    }
  error ("Expression with invalid op code: (%p:%u:%u)", start, mark.up - 1 - start, *(mark.up-1));
}

} // Rapicorn

#endif  /* __RAPICORN_SINFEX_IMPL_HH__ */
