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

class Region {
  struct { int64 idummy[4]; void *pdummy; } m_region;
  inline void*       region_mem   ();
  inline const void* region_cmem  () const;
public: /* rectangles are represented at 64bit integer precision */
  typedef enum { OUTSIDE, INSIDE, PARTIAL } ContainedTyoe;
  explicit      Region            ();
  explicit      Region            (const Rect           &src);
  explicit      Region            (const Region         &src);
  Region&       operator=         (const Region         &src);
  void          clear             ();
  bool          is_empty          () const;
  bool          equal             (const Region         &other) const;
  void          swap              (Region               &other);
  Rect          extents           () const;
  bool          point_in          (const Point          &point) const;
  ContainedTyoe rect_in           (const Rect           &rect) const;
  void          list_rects        (std::vector<Rect>    &rects) const;
  void          union_rect        (const Rect           &rect);
  void          union_region      (const Region         &other);
  void          subtract_region   (const Region         &subtrahend);
  void          intersect_region  (const Region         &other);
  void          xor_region        (const Region         &other);
};

} // Rapicorn

#endif  /* __RAPICORN_REGION_HH__ */
