/* Rapicorn
 * Copyright (C) 2007 Tim Janik
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
#ifndef __RAPICORN_DEBUG_TOOLS_HH__
#define __RAPICORN_DEBUG_TOOLS_HH__

#include <rapicorn-core/rapicornutils.hh>
#include <stdarg.h>

namespace Rapicorn {

class DebugChannel : public virtual ReferenceCountImpl {
  RAPICORN_PRIVATE_CLASS_COPY (DebugChannel);
protected:
  explicit              DebugChannel        ();
  virtual               ~DebugChannel       ();
public:
  virtual void          printf_valist       (const char *format,
                                             va_list     args) = 0;
  inline void           printf              (const char *format, ...) RAPICORN_PRINTF (2, 3);
  static DebugChannel*  new_from_file_async (const String &filename);
};

inline void
DebugChannel::printf (const char *format,
                      ...)
{
  va_list a;
  va_start (a, format);
  printf_valist (format, a);
  va_end (a);
}

/* test dump */
class TestStream {
public:
  typedef enum { TEXT, NODE, VALUE, INTERN, INDENT, POPNODE, POPINDENT } Kind;
  RAPICORN_PRIVATE_CLASS_COPY  (TestStream);
protected:
  /*Con*/       TestStream              ();
  virtual void  ddump                   (Kind kind, const String &name, const String &val) = 0;
public:
  void          dump                    (const String &text) { ddump (TEXT, "", text); }
  template<typename Value>
  void          dump                    (const String &name, Value v) { ddump (VALUE, name, string_from_type (v)); }
  template<typename Value>
  void          dump_intern             (const String &name, Value v) { ddump (INTERN, name, string_from_type (v)); }
  void          push_node               (const String &name) { ddump (NODE, name, ""); }
  void          pop_node                () { ddump (POPNODE, "", ""); }
  void          push_indent             (void) { ddump (INDENT, "", ""); }
  void          pop_indent              (void) { ddump (POPINDENT, "", ""); }
  virtual      ~TestStream              ();
  virtual String string                 () = 0;
  static
  TestStream*   create_test_stream      ();
};

} // Rapicorn

#endif /* __RAPICORN_DEBUG_TOOLS_HH__ */
/* vim:set ts=8 sts=2 sw=2: */
