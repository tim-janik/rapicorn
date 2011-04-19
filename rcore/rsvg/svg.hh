/* Rapicorn
 * Copyright (C) 2011 Tim Janik
 * Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
 */
#ifndef __RAPICORN_SVG_HH__
#define __RAPICORN_SVG_HH__

#include <rcore/rapicornutils.hh>
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
protected:
  ElementImpl  *impl;
public:
  Element&      operator=       (const Element&);
  /*Copy*/      Element         (const Element&);
  /*Con*/       Element         ();
  /*Des*/      ~Element         ();
  bool          null            () const { return !impl; }
  bool          none            () const { return null(); }
  Info          info            ();
  Allocation    allocation      ();
  Allocation    allocation      (Allocation &_containee);
  Allocation    containee       ();
  Allocation    containee       (Allocation &_resized);
  bool          render          (cairo_surface_t *surface, const Allocation &area);
};

class Library {
public:
  static void           add_search_dir  (const String   &absdir);
  static void           add_library     (const String   &filename);
  static Element        lookup_element  (const String   &id);
};

} // Svg
} // Rapicorn

#endif /* __RAPICORN_SVG_HH__ */
