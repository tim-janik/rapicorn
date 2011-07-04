/* Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html */
#ifndef __RAPICORN_SVG_HH__
#define __RAPICORN_SVG_HH__

#include <rcore/utilities.hh>
#include <cairo.h>
#include <math.h>

namespace Rapicorn {
namespace Svg {

struct Info {
  String id; double em, ex;
  Info ();
};
struct Allocation {
  double x, y, width, height;
  Allocation ();
  Allocation (double, double, double, double);
};

class ElementImpl;
class Element {
  typedef bool (Element::*_unspecified_bool_type) () const; // non-numeric operator bool() result
  static inline _unspecified_bool_type _unspecified_bool_true () { return &Element::is_null; }
protected:
  ElementImpl  *impl;
public:
  Element&      operator=       (const Element&);
  /*Copy*/      Element         (const Element&);
  /*Con*/       Element         ();
  /*Des*/      ~Element         ();
  Info          info            ();
  Allocation    allocation      ();
  Allocation    allocation      (Allocation &_containee);
  Allocation    containee       ();
  Allocation    containee       (Allocation &_resized);
  bool          render          (cairo_surface_t *surface, const Allocation &area);
  // none type and boolean evaluation
  static const
  Element&      none            ();
  bool          is_null         () const { return !impl; }
  inline operator _unspecified_bool_type () const { return is_null() ? NULL : _unspecified_bool_true(); }
};

class Library {
public:
  static void           add_search_dir  (const String   &absdir);
  static void           add_library     (const String   &filename);
  static Element        lookup_element  (const String   &id);
  static void           dump_tree       (const String   &id);
};

} // Svg
} // Rapicorn

#endif /* __RAPICORN_SVG_HH__ */
