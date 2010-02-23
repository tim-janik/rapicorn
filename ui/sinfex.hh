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
#ifndef __RAPICORN_SINFEX_HH__
#define __RAPICORN_SINFEX_HH__

#include <ui/utilities.hh>

namespace Rapicorn {

class Sinfex : public virtual ReferenceCountImpl {
  RAPICORN_PRIVATE_CLASS_COPY (Sinfex);
protected:
  uint          *m_start;
  explicit       Sinfex ();
  virtual       ~Sinfex ();
public:
  static Sinfex* parse_string (const String &expression);
  class          Value;
  struct Scope {
    virtual Value resolve_variable        (const String        &entity,
                                           const String        &name) = 0;
    virtual Value call_function           (const String        &entity,
                                           const String        &name,
                                           const vector<Value> &args) = 0;
  };
  virtual Value  eval     (Scope &scope) = 0;
  /* Sinfex::Value implementation */
  class Value {
    String   m_string;
    double   m_real;
    bool     m_strflag;
    String   real2string () const;
    double   string2real () const;
  public:
    bool     isreal      () const { return !m_strflag; }
    bool     isstring    () const { return m_strflag; }
    double   real        () const { return !m_strflag ? m_real : string2real(); }
    String   string      () const { return m_strflag ? m_string : real2string(); }
    bool     asbool      () const { return (!m_strflag && m_real) || (m_strflag && m_string != ""); }
    explicit Value       (double        d) : m_real (d), m_strflag (0) {}
    explicit Value       (const String &s);
  };
};

} // Rapicorn

#endif  /* __RAPICORN_SINFEX_HH__ */
