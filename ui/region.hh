/* Rapicorn
 * Copyright (C) 2006 Tim Janik
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
#ifndef __RAPICORN_REGION_HH__
#define __RAPICORN_REGION_HH__

#include <ui/primitives.hh>

namespace Rapicorn {

/**
 * Region implements various common operations on areas which can be covered
 * by a list of rectangles. Region calculations use fixed point arithmetics,
 * so only a small number of fractional digits are supported (2 decimals,
 * fractions are represented by 8 bit), and rectangle sizes are also limited
 * (rectangle dimensions shouldn't exceed 3.6e16, i.e. 2^55).
 */
class Region {
  union {
    struct CRegion { int64 idummy[4]; void *pdummy; };
    CRegion          cstruct_mem;               // ensure C structure size and alignment
    char             chars[sizeof (CRegion)];   // char may_alias any type
  }                  m_region;                  // RAPICORN_MAY_ALIAS; ICE: GCC#30894
  inline void*       region_mem   ();
  inline const void* region_mem   () const;
public: /* rectangles are represented at 64bit integer precision */
  typedef enum { OUTSIDE = 0, INSIDE = 1, PARTIAL } ContainedType;
  explicit      Region            ();
  /*Con*/       Region            (const Region         &src);
  /*Con*/       Region            (const Rect           &src);
  /*Con*/       Region            (const Point          &rect_p1,
                                   const Point          &rect_p2);
  Region&       operator=         (const Region         &src);
  void          clear             ();
  bool          empty             () const;
  bool          equal             (const Region &other) const;
  int           cmp               (const Region &other) const;
  void          swap              (Region               &other);
  Rect          extents           () const;
  bool          contains          (const Point          &point) const;
  bool          contains          (double                x,
                                   double                y) const       { return contains (Point (x, y)); }
  ContainedType contains          (const Rect           &rect) const;
  ContainedType contains          (const Region         &other) const;
  void          list_rects        (std::vector<Rect>    &rects) const;
  uint          count_rects       () const;
  void          add               (const Rect           &rect);
  void          add               (const Region         &other);
  void          subtract          (const Region         &subtrahend);
  void          intersect         (const Region         &other);
  void          exor              (const Region         &other);
  void          translate         (double deltax, double deltay);
  void          affine            (const Affine         &aff);
  double        epsilon           () const;
  String        string            ();
  /*Des*/      ~Region            ();
};

bool operator== (const Region &r1,
                 const Region &r2);
bool operator!= (const Region &r1,
                 const Region &r2);
bool operator<  (const Region &r1,
                 const Region &r2);

} // Rapicorn

#endif  /* __RAPICORN_REGION_HH__ */
