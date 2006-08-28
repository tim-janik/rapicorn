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
#include "region.hh"
#include "regionimpl.h"

namespace Rapicorn {

static inline int64
i64round (double d)
{
  return (int64) round (d);
}

static inline RapicornRegionBox
rect2box (const Rect &src)
{
  RapicornRegionBox box = {
    i64round (src.ll.x),
    i64round (src.ll.y),
    i64round (src.ur.x),
    i64round (src.ur.y)
  };
  return box;
}

inline void*
Region::region_mem ()
{
  return &m_region;
}
#define REGION(this)  ((RapicornRegion*) (this)->region_mem())

inline const void*
Region::region_cmem () const
{
  return &m_region;
}
#define CREGION(this) ((const RapicornRegion*) (this)->region_cmem())

Region::Region ()
{
  _rapicorn_region_init (REGION (this), sizeof (m_region));
}

Region::Region (const Rect &src)
{
  _rapicorn_region_init (REGION (this), sizeof (m_region));
  RapicornRegionBox box = rect2box (src);
  _rapicorn_region_union_rect (REGION (this), &box);
}

Region::Region (const Region &src)
{
  _rapicorn_region_init (REGION (this), sizeof (m_region));
  _rapicorn_region_copy (REGION (this), CREGION (&src));
}

Region&
Region::operator= (const Region &src)
{
  _rapicorn_region_copy (REGION (this), CREGION (&src));
  return *this;
}

void
Region::clear ()
{
  _rapicorn_region_clear (REGION (this));
}

bool
Region::is_empty () const
{
  return _rapicorn_region_empty (CREGION (this));
}

bool
Region::equal (const Region &other) const
{
  return _rapicorn_region_equal (CREGION (this), CREGION (&other));
}

void
Region::swap (Region &other)
{
  _rapicorn_region_swap (REGION (this), REGION (&other));
}

Rect
Region::extents () const
{
  RapicornRegionBox box;
  _rapicorn_region_extents (CREGION (this), &box);
  return Rect (Point (box.x1, box.y1), Point (box.x2, box.y2));
}

bool
Region::point_in (const Point &point) const
{
  RapicornRegionPoint p = { i64round (point.x), i64round (point.y) };
  return _rapicorn_region_point_in (CREGION (this), &p);
}

Region::ContainedTyoe
Region::rect_in (const Rect &rect) const
{
  RapicornRegionBox box = rect2box (rect);
  BIRNET_STATIC_ASSERT (OUTSIDE == (int) RAPICORN_REGION_OUTSIDE);
  BIRNET_STATIC_ASSERT (INSIDE  == (int) RAPICORN_REGION_INSIDE);
  BIRNET_STATIC_ASSERT (PARTIAL == (int) RAPICORN_REGION_PARTIAL);
  return ContainedTyoe (_rapicorn_region_rect_in (CREGION (this), &box));
}

void
Region::list_rects (std::vector<Rect> &rects) const
{
  rects.clear();
  uint n = _rapicorn_region_get_rects (CREGION (this), 0, NULL);
  RapicornRegionBox boxes[n];
  uint k = _rapicorn_region_get_rects (CREGION (this), n, boxes);
  assert (k == n);
  for (uint i = 0; i < n; i++)
    rects.push_back (Rect (Point (boxes[i].x1, boxes[i].y1), Point (boxes[i].x2, boxes[i].y2)));
}

void
Region::union_rect (const Rect &rect)
{
  RapicornRegionBox box = rect2box (rect);
  _rapicorn_region_union_rect (REGION (this), &box);
}

void
Region::union_region (const Region &other)
{
  _rapicorn_region_union (REGION (this), CREGION (&other));
}

void
Region::subtract_region (const Region &subtrahend)
{
  _rapicorn_region_subtract (REGION (this), CREGION (&subtrahend));
}

void
Region::intersect_region (const Region &other)
{
  _rapicorn_region_intersect (REGION (this), CREGION (&other));
}

void
Region::xor_region (const Region &other)
{
  _rapicorn_region_xor (REGION (this), CREGION (&other));
}

} // Rapicorn
