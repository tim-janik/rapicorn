// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
//                                                      -*-mode:c++;-*-

#ifndef __RAPICORN_REGION_HH__
#define __RAPICORN_REGION_HH__

#include <ui/primitives.hh>

namespace Rapicorn {

/**
 * @brief The Region class manages areas defined through a list of rectangles.
 *
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
  }                  region_;                  // RAPICORN_MAY_ALIAS; ICE: GCC#30894
  inline void*       region_mem   ();
  inline const void* region_mem   () const;
public: /* rectangles are represented at 64bit integer precision */
  typedef enum { OUTSIDE = 0, INSIDE = 1, PARTIAL } ContainedType;
  explicit      Region            ();                           ///< Construct an empty Region.
  /*Con*/       Region            (const Region &region);       ///< Copy-construct a Region.
  /*Con*/       Region            (const Rect   &rectangle);    ///< Construct a Region covering @a rectangle.
  /*Con*/       Region            (const Point  &pt1,
                                   const Point  &pt2);          ///< Construct a Region covering a rectangle defined by two points.
  Region&       operator=         (const Region &region);       ///< Region assignment.
  void          clear             ();                           ///< Forces an empty Region.
  bool          empty             () const;                     ///< Returns whether the Region is empty.
  bool          equal             (const Region &other) const;  ///< Returns whether two Regions cover the same area.
  int           cmp               (const Region &other) const;  ///< Provides linear ordering for regions.
  void          swap              (Region              &other); ///< Swaps the contents of two Regions.
  Rect          extents           () const;                     ///< Provides rectangle extents of the region.
  bool          contains          (const Point  &point) const;  ///< Test if @a point is inside Region.
  bool          contains          (double x, double y) const    { return contains (Point (x, y)); } ///< Point test with coordinates.
  ContainedType contains          (const Rect   &rect) const;   ///< Returns if Region covers @a rect fully, partially or not at all.
  ContainedType contains          (const Region &other) const;  ///< Returns if Region covers @a other fully, partially or not at all.
  void          list_rects        (vector<Rect> &rects) const;  ///< Provides a list of rectangles that define Region.
  uint          count_rects       () const;                     ///< Provides the number of rectangles that cover Region.
  void          add               (const Rect   &rect);         ///< Adds a rectangle to the Region.
  void          add               (const Region &other);        ///< Causes Region to contain the union with @a other.
  void          subtract          (const Region &subtrahend);   ///< Removes @a subtrahend from Region.
  void          intersect         (const Region &other);        ///< Causes Region to contain the intersection with @a other.
  void          exor              (const Region &other);        ///< Causes Region to contain the XOR composition with @a other.
  void          translate         (double dx, double dy);       ///< Shifts Region by @a dx, @a dy.
  void          affine            (const Affine &affine);       ///< Transforms Region by @a affine.
  double        epsilon           () const;                     ///< Returns the precision (granularity) between fractional digits.
  String        string            ();                           ///< Describes Region in a string.
  /*dtor*/     ~Region            ();                           ///< Public destructor, Region is a simple copyable structure.
};

bool operator== (const Region &r1, const Region &r2);   ///< Compare two Region structures for equality.
bool operator!= (const Region &r1, const Region &r2);   ///< Compare two Region structures returns if these are unequal.
bool operator<  (const Region &r1, const Region &r2);   ///< Provides linear ordering for regions.

} // Rapicorn

#endif  /* __RAPICORN_REGION_HH__ */
