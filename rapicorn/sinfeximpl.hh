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

struct Exon {
  struct Env;
  virtual      ~Exon () {}
  virtual Value eval (Env *env = NULL) const = 0;
};

class ExonReal : virtual public Exon {
  double m_d;
public:
  explicit      ExonReal (double d) : m_d (d) {}
  virtual Value eval     (Env *env) const { return Value (m_d); }
};

class ExonString : virtual public Exon {
  String m_s;
public:
  explicit      ExonString (const String &s) : m_s (s) {}
  String        string     () const { return m_s; }
  virtual Value eval       (Env *env) const { return Value (m_s); }
};

class ExonOr : virtual public Exon {
  Exon &m_a, &m_b;
public:
  explicit      ExonOr (Exon &a, Exon &b) : m_a (a), m_b (b) {}
  virtual Value
  eval (Env *env) const
  {
    Value a = m_a.eval(), b = m_b.eval();
    return Value (a.asbool() ? a : b);
  }
};

class ExonAnd : virtual public Exon {
  Exon &m_a, &m_b;
public:
  explicit      ExonAnd (Exon &a, Exon &b) : m_a (a), m_b (b) {}
  virtual Value
  eval (Env *env) const
  {
    Value a = m_a.eval(), b = m_b.eval();
    return Value (a.asbool() ? b : a);
  }
};

class ExonNot : virtual public Exon {
  Exon &m_a;
public:
  explicit      ExonNot (Exon &a) : m_a (a) {}
  virtual Value
  eval (Env *env) const
  {
    Value a = m_a.eval();
    return Value (!a.asbool());
  }
};

class ExonCmp : virtual public Exon {
  Exon &m_a, &m_b;
  int m_c;
public:
  explicit      ExonCmp (Exon &a, int c, Exon &b) : m_a (a), m_b (b), m_c (c) {}
  virtual Value
  eval (Env *env) const
  {
    Value a = m_a.eval(), b = m_b.eval();
    int tcmp = a.isstring() - b.isstring();
    int scmp = a.isstring() && b.isstring() ? strcmp (a.string().c_str(), b.string().c_str()) : 0;
    int rcmp = a.real() - b.real() < 0 ? -1 : a.real() - b.real() > 0 ? +1 : 0;
    int cmp = tcmp ? tcmp : scmp + rcmp;
    switch (m_c)
      {
      case -1:  return Value (cmp < 0);   // LT
      case +1:  return Value (cmp > 0);   // GT
      case -2:  return Value (cmp <= 0);  // LE
      case +2:  return Value (cmp >= 0);  // GE
      case  0:  return Value (cmp == 0);  // EQ
      default:  return Value (cmp != 0);  // NE
      }
  }
};

class ExonAdd : virtual public Exon {
  Exon &m_a, &m_b;
public:
  explicit      ExonAdd (Exon &a, Exon &b) : m_a (a), m_b (b) {}
  virtual Value
  eval (Env *env) const
  {
    Value a = m_a.eval(), b = m_b.eval();
    if (a.isreal() && b.isreal())
      return Value (a.real() + b.real());
    else if (a.isstring() && b.isstring())
      return Value (a.string() + b.string());
    else
      {
        fprintf (stderr, "invalid value types in '+'\n");
        exit (1);
      }
  }
};

class ExonSub : virtual public Exon {
  Exon &m_a, &m_b;
public:
  explicit      ExonSub (Exon &a, Exon &b) : m_a (a), m_b (b) {}
  virtual Value
  eval (Env *env) const
  {
    Value a = m_a.eval(), b = m_b.eval();
    if (a.isreal() && b.isreal())
      return Value (a.real() - b.real());
    else
      {
        fprintf (stderr, "invalid value types in '-'\n");
        exit (1);
      }
  }
};

class ExonSign : virtual public Exon {
  char m_sign;
  Exon &m_a;
public:
  explicit      ExonSign (char sign, Exon &a) : m_sign (sign), m_a (a) {}
  virtual Value
  eval (Env *env) const
  {
    Value a = m_a.eval();
    if (a.isreal())
      return Value (m_sign == '-' ? -a.real() : a.real());
    else
      {
        fprintf (stderr, "invalid value type in sign conversion ('%c')\n", m_sign);
        exit (1);
      }
  }
};

class ExonMul : virtual public Exon {
  Exon &m_a, &m_b;
public:
  explicit      ExonMul (Exon &a, Exon &b) : m_a (a), m_b (b) {}
  virtual Value
  eval (Env *env) const
  {
    Value a = m_a.eval(), b = m_b.eval();
    if (a.isreal() && b.isreal())
      return Value (a.real() * b.real());
    else
      {
        fprintf (stderr, "invalid value types in '*'\n");
        exit (1);
      }
  }
};

class ExonDiv : virtual public Exon {
  Exon &m_a, &m_b;
public:
  explicit      ExonDiv (Exon &a, Exon &b) : m_a (a), m_b (b) {}
  virtual Value
  eval (Env *env) const
  {
    Value a = m_a.eval(), b = m_b.eval();
    if (a.isreal() && b.isreal())
      return Value (b.real() != 0.0 ? a.real() / b.real() : nanl (0));
    else
      {
        fprintf (stderr, "invalid value types in '/'\n");
        exit (1);
      }
  }
};

class ExonPow : virtual public Exon {
  Exon &m_a, &m_b;
public:
  explicit      ExonPow (Exon &a, Exon &b) : m_a (a), m_b (b) {}
  virtual Value
  eval (Env *env) const
  {
    Value a = m_a.eval(), b = m_b.eval();
    if (a.isreal() && b.isreal())
      return Value (pow (a.real(), b.real()));
    else
      {
        fprintf (stderr, "invalid value types in '*'\n");
        exit (1);
      }
  }
};

class ExonArgs : virtual public Exon {
  vector<Exon*> m_args;
public:
  explicit      ExonArgs () {}
  void          add      (Exon &exon) { m_args.push_back (&exon); }
  const vector<Exon*>
  args()
  {
    return m_args;
  }
  virtual Value
  eval (Env *env) const
  {
    fprintf (stderr, "unevaluatable cons list\n");
    exit (1);
  }
};

class ExonFun : virtual public Exon {
  String    m_name;
  ExonArgs &m_args;
public:
  ExonFun (ExonString &name, ExonArgs &args) :
    m_name (name.string()), m_args (args)
  {
    delete &name;
  }
  virtual Value
  eval (Env *env) const
  {
    fprintf (stderr, "function call '%s'\n", m_name.c_str());
    const vector<Exon*> &args = m_args.args();
    for (uint i = 0; i < args.size(); i++)
      {
        Value v = args[i]->eval (NULL);
        fprintf (stderr, "  ARG: %s", v.tostring().c_str());
      }
    return Value (0);
  }
};

class ExonVar : virtual public Exon {
  String    m_entity;
  String    m_name;
public:
  explicit ExonVar (ExonString &entity, ExonString &name) :
    m_entity (entity.string()), m_name (name.string())
  {
    delete &entity;
    delete &name;
  }
  virtual Value
  eval (Env *env) const
  {
    fprintf (stderr, "FIXME: evaluate variable\n");
    return Value (0);
  }
};

} // Rapicorn

#endif  /* __RAPICORN_SINFEX_IMPL_HH__ */
