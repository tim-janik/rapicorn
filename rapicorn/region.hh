/* Rapicorn
 * Copyright (C) 2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __RAPICORN_REGION_HH__
#define __RAPICORN_REGION_HH__

#include <rapicorn/primitives.hh>

namespace Rapicorn {

/**
 * Region implements various common operations on areas which can be covered
 * by a list of rectangles. Region calculations use fixed point arithmetics,
 * so only a small number of fractional digits are supported (2 decimals,
 * fractions are represented by 8 bit), and rectangle sizes are also limited
 * (rectangle dimensions shouldn't exceed 3.6e16, i.e. 2^55).
 */
class Region {
  struct { int64 idummy[4]; void *pdummy; } m_region;
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
  void          add               (const Rect           &rect);
  void          add               (const Region         &other);
  void          subtract          (const Region         &subtrahend);
  void          intersect         (const Region         &other);
  void          exor              (const Region         &other);
  String        string            ();
};

bool operator== (const Region &r1,
                 const Region &r2);
bool operator!= (const Region &r1,
                 const Region &r2);
bool operator<  (const Region &r1,
                 const Region &r2);

} // Rapicorn

#endif  /* __RAPICORN_REGION_HH__ */
