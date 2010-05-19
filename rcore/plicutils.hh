/* Plic
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
#ifndef __RAPICORN_PLICUTILS_HH__
#define __RAPICORN_PLICUTILS_HH__

#include <string>
#include <vector>

namespace Rapicorn {
namespace Plic {
typedef std::string String;
using std::vector;
typedef long long signed int   int64;
typedef long long unsigned int uint64;
typedef unsigned int uint;
typedef unsigned short int uint16;
typedef unsigned char uint8;

typedef enum {
  VOID = 0,
  INT, FLOAT, STRING, ENUM,
  RECORD, SEQUENCE, FUNC, INSTANCE
} FieldType;

class PlicRec;
class PlicSeq;

class MsgBuffer {
public:
};

union FieldUnion;

union FieldUnion {
  int64        vint64;
  double       vdouble;
  uint64       smem[(sizeof (String) + 7) / 8];         // String
  uint64       bmem[(sizeof (FieldUnion*) + 7) / 8];    // FieldBuffer
  uint8        bytes[8];                // FieldBuffer types
  struct { uint capacity, index; };     // FieldBuffer.buffermem[0]
};

class FieldBufferReader;

class FieldBuffer {
  FieldUnion        *buffermem;
  inline uint        offset () const { const uint offs = 1 + (n_types() + 7) / 8; return offs; }
  inline FieldType   type_at (uint n) const { return FieldType (buffermem[1 + n/8].bytes[n%8]); }
  inline void        set_type (FieldType ft) { buffermem[1 + nth()/8].bytes[nth()%8] = ft; }
  inline uint        n_types () const { return buffermem[0].capacity; }
  inline uint        nth () const     { return buffermem[0].index; }
  inline FieldUnion& getu () const    { return buffermem[offset() + nth()]; }
  inline FieldUnion& addu (FieldType ft) { set_type (ft); FieldUnion &u = getu(); buffermem[0].index++; return u; }
  inline void        check() { /* FIXME: n_types() shouldn't be exceeded */ }
  friend class FieldBufferReader;
  inline FieldUnion& uat (uint n) const { return n < n_types() ? buffermem[offset() + n] : *(FieldUnion*) NULL; }
public:
  inline int64 first_id () const { return buffermem && type_at (0) == INT ? uat (0).vint64 : 0; }
  inline void add_int64  (int64  vint64)  { FieldUnion &u = addu (INT); u.vint64 = vint64; check(); }
  inline void add_evalue (int64  vint64)  { FieldUnion &u = addu (ENUM); u.vint64 = vint64; check(); }
  inline void add_double (double vdouble) { FieldUnion &u = addu (FLOAT); u.vdouble = vdouble; check(); }
  inline void add_string (const String &s) { FieldUnion &u = addu (STRING); new (&u) String (s); check(); }
  inline void add_func   (const String &s) { FieldUnion &u = addu (FUNC); new (&u) String (s); check(); }
  inline void add_object (const String &s) { FieldUnion &u = addu (INSTANCE); new (&u) String (s); check(); }
  inline FieldBuffer& add_rec (uint nt) { FieldUnion &u = addu (RECORD); return *new (&u) FieldBuffer (nt); check(); }
  inline FieldBuffer& add_seq (uint nt) { FieldUnion &u = addu (SEQUENCE); return *new (&u) FieldBuffer (nt); check(); }
  inline FieldBuffer     (uint _ntypes) :
    buffermem (NULL)
  {
    if (_ntypes > 61440)
      ; // FIXME: _ntypes OOB
    // buffermem layout: [{n_types,nth}] [{type nibble} * n_types]... [field]...
    const uint _offs = 1 + (_ntypes + 7) / 8;
    buffermem = new FieldUnion[_offs + _ntypes];
    wmemset ((wchar_t*) buffermem, 0, sizeof (FieldUnion[_offs + _ntypes]) / sizeof (wchar_t));
    buffermem[0].capacity = _ntypes;
    buffermem[0].index = 0;
  }
  inline const uint8&
  buffer  (uint *l = NULL) const
  {
    if (l)
      *l = sizeof (FieldUnion[offset() + n_types()]);
    return *(uint8*) buffermem;
  }
  inline ~FieldBuffer()
  {
    while (nth() > 0)
      {
        buffermem[0].index--; // nth()--
        switch (type_at (nth()))
          {
          case INSTANCE:
          case STRING:
          case FUNC:    { FieldUnion &u = getu(); ((String*) &u)->~String(); }; break;
          case SEQUENCE:
          case RECORD:  { FieldUnion &u = getu(); ((FieldBuffer*) &u)->~FieldBuffer(); }; break;
          default: ;
          }
      }
    delete [] buffermem;
  }
};

class FieldBufferReader {
  const FieldBuffer &m_fb;
  uint               m_nth;
  inline FieldUnion& fb_getu () { return m_fb.uat (m_nth); }
  inline FieldUnion& fb_popu () { FieldUnion &u = m_fb.uat (m_nth++); check(); return u; }
  inline void        check() { /* FIXME: n_types() shouldn't be exceeded */ }
public:
  FieldBufferReader (const FieldBuffer &_fb) : m_fb (_fb), m_nth (0) {}
  inline uint               remaining  () { return n_types() - m_nth; }
  inline void               skip       () { m_nth++; check(); }
  inline uint               n_types    () { return m_fb.n_types(); }
  inline FieldType          get_type   () { return m_fb.type_at (m_nth); }
  inline int64              get_int64  () { FieldUnion &u = fb_getu(); return u.vint64; }
  inline int64              get_evalue () { FieldUnion &u = fb_getu(); return u.vint64; }
  inline double             get_double () { FieldUnion &u = fb_getu(); return u.vdouble; }
  inline const String&      get_string () { FieldUnion &u = fb_getu(); return *(String*) &u; }
  inline const String&      get_func   () { FieldUnion &u = fb_getu(); return *(String*) &u; }
  inline const String&      get_object () { FieldUnion &u = fb_getu(); return *(String*) &u; }
  inline const FieldBuffer& get_rec () { FieldUnion &u = fb_getu(); return *(FieldBuffer*) &u; }
  inline const FieldBuffer& get_seq () { FieldUnion &u = fb_getu(); return *(FieldBuffer*) &u; }
  inline int64              pop_int64  () { FieldUnion &u = fb_popu(); return u.vint64; }
  inline int64              pop_evalue () { FieldUnion &u = fb_popu(); return u.vint64; }
  inline double             pop_double () { FieldUnion &u = fb_popu(); return u.vdouble; }
  inline const String&      pop_string () { FieldUnion &u = fb_popu(); return *(String*) &u; }
  inline const String&      pop_func   () { FieldUnion &u = fb_popu(); return *(String*) &u; }
  inline const String&      pop_object () { FieldUnion &u = fb_popu(); return *(String*) &u; }
  inline const FieldBuffer& pop_rec () { FieldUnion &u = fb_popu(); return *(FieldBuffer*) &u; }
  inline const FieldBuffer& pop_seq () { FieldUnion &u = fb_popu(); return *(FieldBuffer*) &u; }
};

} // Plic
} // Rapicorn

#endif /* __RAPICORN_PLICUTILS_HH__ */
