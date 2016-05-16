// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "region.hh"
#include "regionimpl.h"

namespace Rapicorn {

#define FIXED2DOUBLE    (1.0 / 256.)    // 0x.01p0
#define DOUBLE2FIXED    (256.)          // 0x100.p0


static inline int64
double2fixed (double d)
{
  return iround (d * DOUBLE2FIXED);
}

static inline double
fixed2double (int64 i)
{
  return FIXED2DOUBLE * i;
}

static inline RapicornRegionBox
rect2box (const DRect &src)
{
  RapicornRegionBox box = {
    double2fixed (src.x),
    double2fixed (src.y),
    double2fixed (src.upper_x()),
    double2fixed (src.upper_y())
  };
  return box;
}

inline void*
Region::region_mem ()
{
  return &region_;
}

inline const void*
Region::region_mem () const
{
  return &region_;
}
#define REGION(this)  ((RapicornRegion*) (this)->region_mem())

Region::Region ()
{
  _rapicorn_region_init (REGION (this), sizeof (region_));
}

Region::Region (const Region &src)
{
  _rapicorn_region_init (REGION (this), sizeof (region_));
  _rapicorn_region_copy (REGION (this), REGION (&src));
}

Region::Region (const DRect &src)
{
  _rapicorn_region_init (REGION (this), sizeof (region_));
  RapicornRegionBox box = rect2box (src);
  _rapicorn_region_union_rect (REGION (this), &box);
}

Region::Region (const Point          &rect_p1,
                const Point          &rect_p2)
{
  _rapicorn_region_init (REGION (this), sizeof (region_));
  RapicornRegionBox box = rect2box (DRect (rect_p1, rect_p2));
  _rapicorn_region_union_rect (REGION (this), &box);
}

Region::~Region ()
{
  _rapicorn_region_uninit (REGION (this));
}

Region&
Region::operator= (const Region &src)
{
  _rapicorn_region_copy (REGION (this), REGION (&src));
  return *this;
}

void
Region::clear ()
{
  _rapicorn_region_clear (REGION (this));
}

bool
Region::empty () const
{
  return _rapicorn_region_empty (REGION (this));
}

bool
Region::equal (const Region &other) const
{
  return _rapicorn_region_equal (REGION (this), REGION (&other));
}

int
Region::cmp (const Region &other) const
{
  return _rapicorn_region_cmp (REGION (this), REGION (&other));
}

void
Region::swap (Region &other)
{
  _rapicorn_region_swap (REGION (this), REGION (&other));
}

DRect
Region::extents () const
{
  RapicornRegionBox box;
  _rapicorn_region_extents (REGION (this), &box);
  return DRect (fixed2double (box.x1), fixed2double (box.y1),
                fixed2double (box.x2 - box.x1),
                fixed2double (box.y2 - box.y1));
}

bool
Region::contains (const Point &point) const
{
  RapicornRegionPoint p = { double2fixed (point.x), double2fixed (point.y) };
  return _rapicorn_region_point_in (REGION (this), &p);
}

Region::ContainedType
Region::contains (const DRect &rect) const
{
  RapicornRegionBox box = rect2box (rect);
  RAPICORN_STATIC_ASSERT (OUTSIDE == (int) RAPICORN_REGION_OUTSIDE);
  RAPICORN_STATIC_ASSERT (INSIDE  == (int) RAPICORN_REGION_INSIDE);
  RAPICORN_STATIC_ASSERT (PARTIAL == (int) RAPICORN_REGION_PARTIAL);
  return ContainedType (_rapicorn_region_rect_in (REGION (this), &box));
}

Region::ContainedType
Region::contains (const Region &other) const
{
  return ContainedType (_rapicorn_region_region_in (REGION (this), REGION (&other)));
}

void
Region::list_rects (std::vector<DRect> &rects) const
{
  rects.clear();
  uint n = _rapicorn_region_get_rects (REGION (this), 0, NULL);
  RapicornRegionBox boxes[n];
  uint k = _rapicorn_region_get_rects (REGION (this), n, boxes);
  assert (k == n);
  for (uint i = 0; i < n; i++)
    rects.push_back (DRect (fixed2double (boxes[i].x1), fixed2double (boxes[i].y1),
                            fixed2double (boxes[i].x2 - boxes[i].x1),
                            fixed2double (boxes[i].y2 - boxes[i].y1)));
}

uint
Region::count_rects () const
{
  return _rapicorn_region_get_rect_count (REGION (this));
}

void
Region::add (const DRect &rect)
{
  RapicornRegionBox box = rect2box (rect);
  _rapicorn_region_union_rect (REGION (this), &box);
}

void
Region::add (const Region &other)
{
  _rapicorn_region_union (REGION (this), REGION (&other));
}

void
Region::subtract (const Region &subtrahend)
{
  _rapicorn_region_subtract (REGION (this), REGION (&subtrahend));
}

void
Region::intersect (const Region &other)
{
  _rapicorn_region_intersect (REGION (this), REGION (&other));
}

void
Region::exor (const Region &other)
{
  _rapicorn_region_xor (REGION (this), REGION (&other));
}

void
Region::translate (double deltax, double deltay)
{
  std::vector<DRect> rects;
  list_rects (rects);
  clear();
  for (uint i = 0; i < rects.size(); i++)
    add (DRect (rects[i].x + deltax, rects[i].y + deltay, rects[i].width, rects[i].height));
}

void
Region::affine (const Affine &aff)
{
  // FIXME: optimize for aff.is_identity()
  std::vector<DRect> rects;
  list_rects (rects);
  clear();
  for (uint i = 0; i < rects.size(); i++)
    {
      /* to make sure to cover the new rectangle even for rotations,
       * we take the bounding box of all 4 corners transformed
       */
      Point p1 = aff.point (rects[i].upper_left());
      Point p2 = aff.point (rects[i].lower_left());
      Point p3 = aff.point (rects[i].upper_right());
      Point p4 = aff.point (rects[i].lower_right());
      add (DRect (min (min (p1, p2), min (p3, p4)), max (max (p1, p2), max (p3, p4))));
    }
}

bool
operator== (const Region &r1,
            const Region &r2)
{
  return r1.equal (r2);
}

bool
operator!= (const Region &r1,
            const Region &r2)
{
  return !r1.equal (r2);
}

bool
operator< (const Region &r1,
           const Region &r2)
{
  return r1.cmp (r2) < 0;
}

double
Region::epsilon () const
{
  return FIXED2DOUBLE;
}

String
Region::string()
{
  String s ("{ ");
  std::vector<DRect> rects;
  list_rects (rects);
  for (uint i = 0; i < rects.size(); i++)
    {
      if (i)
        s += ",\n  ";
      s += rects[i].string();
    }
  if (rects.size())
    s += "\n";
  s += "/*";
  DRect ext = extents();
  s += ext.string();
  s += "*/ }";
  return s;
}

} // Rapicorn
