// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_DEBUG_TOOLS_HH__
#define __RAPICORN_DEBUG_TOOLS_HH__

#include <rcore/utilities.hh>
#include <rcore/strings.hh>

namespace Rapicorn {

/* --- test dump --- */
class TestStream {
  RAPICORN_CLASS_NON_COPYABLE (TestStream);
public:
  typedef enum { TEXT, NODE, VALUE, INTERN, INDENT, POPNODE, POPINDENT } Kind;
protected:
  /*Con*/       TestStream              ();
  virtual void  ddump                   (Kind kind, const String &name, const String &val) = 0;
public:
  virtual      ~TestStream              ();
  void          dump                    (const String &text) { ddump (TEXT, "", text); }
  template<typename Value>
  void          dump                    (const String &name, Value v) { ddump (VALUE, name, string_from_type (v)); }
  template<typename Value>
  void          dump_intern             (const String &name, Value v) { ddump (INTERN, name, string_from_type (v)); }
  void          push_node               (const String &name) { ddump (NODE, name, ""); }
  void          pop_node                ()     { ddump (POPNODE, "", ""); }
  void          push_indent             (void) { ddump (INDENT, "", ""); }
  void          pop_indent              (void) { ddump (POPINDENT, "", ""); }
  virtual void  filter_matched_nodes    (const String &matchpattern) = 0;
  virtual void  filter_unmatched_nodes  (const String &matchpattern) = 0;
  virtual String string                 () = 0;
  static
  TestStream*   create_test_stream      ();
};

} // Rapicorn

#endif /* __RAPICORN_DEBUG_TOOLS_HH__ */
/* vim:set ts=8 sts=2 sw=2: */
