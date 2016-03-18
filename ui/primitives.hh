// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_PRIMITIVES_HH__
#define __RAPICORN_PRIMITIVES_HH__

#include <list>
#include <math.h>
#include <values.h> /* MAXDOUBLE, etc. */
#include <cairo.h>
#include <rapicorn-core.hh>
#include <ui/serverapi.hh> // for enums

namespace Rapicorn {

/* --- degree & radians --- */
#undef PI               /* some math packages define PI? */
static const double PI        = 3.141592653589793238462643383279502884197;
static const double LN_INV255 = -5.5412635451584261462455391880218; // ln(1/255)

inline double RAPICORN_CONST degree  (double radians) { return radians * (180. / PI); }
inline double RAPICORN_CONST radians (double degree)  { return degree * (PI / 180.); }

/* --- forward declarations --- */
struct FactoryContext; // see factory.cc

/* --- enums --- */
typedef enum {
  COMBINE_NORMAL,       /* A OVER B */
  COMBINE_OVER,         /* A + (B & ~A) */
  COMBINE_UNDER,        /* B + (A & ~B) */
  COMBINE_ADD,          /* A + B */
  COMBINE_DEL,          /* B - alphaA */
  COMBINE_ATOP,         /* (A + B) & B */
  COMBINE_XOR,          /* A ^ B */
  COMBINE_BLEND,        /* B * (1 - alpha) + A * alpha */
  COMBINE_VALUE,        /* B.value = A.value */
} CombineType;

/* --- Point --- */
class Point {
public:
  double x, y;
  Point (double ax,
         double ay) :
    x (ax),
    y (ay)
  {}
  explicit Point () :
    x (0),
    y (0)
  {}
  inline double
  dist2 (double px, double py) const
  {
    double dx = px - x;
    double dy = py - y;
    return dx * dx + dy * dy;
  }
  inline double
  dist (double px, double py) const
  {
    // avoid destructive underflow or overflow in (dx^2 + dy^2) ^ .5
    double dx = fabs (px - x);
    double dy = fabs (py - y);
    if (dx == 0.0)
      return dy;
    else if (dy == 0.0)
      return dx;
    else if (dx > dy)
      {
        const double r = dy / dx;
        return dx * sqrt (1.0 + r * r);
      }
    else // dy < dx
      {
        const double r = dx / dy;
        return dy * sqrt (1.0 + r * r);
      }
  }
  inline double dist2 (const Point &p = Point (0, 0)) const { return dist2 (p.x, p.y); }
  inline double dist  (const Point &p = Point (0, 0)) const { return dist (p.x, p.y); }
  Point  operator+  (const Point  &p) const { return Point (x + p.x, y + p.y); }
  Point  operator+  (double    delta) const { return Point (x + delta, y + delta); }
  Point  operator-  (const Point  &p) const { return Point (x - p.x, y - p.y); }
  Point  operator-  (double    delta) const { return Point (x - delta, y - delta); }
  Point& operator-  ()                      { x = -x; y = -y; return *this; }
  Point& operator+= (const Point  &p)       { return *this = *this + p; }
  Point& operator+= (double    delta)       { return *this = *this + delta; }
  Point& operator-= (const Point  &p)       { return *this = *this - p; }
  Point& operator-= (double    delta)       { return *this = *this - delta; }
  bool   operator== (const Point &p2) const { return x == p2.x && y == p2.y; }
  bool   operator!= (const Point &p2) const { return x != p2.x || y != p2.y; }
  bool   equals     (const Point &p2, double epsilon = 0.0) const;
};
inline Point
min (const Point &p1, const Point &p2)
{
  return Point (Rapicorn::min (p1.x, p2.x), Rapicorn::min (p1.y, p2.y));
}
using ::std::min;
inline Point
max (const Point &p1, const Point &p2)
{
  return Point (Rapicorn::max (p1.x, p2.x), Rapicorn::max (p1.y, p2.y));
}
using ::std::max;
inline Point
floor (const Point &s)
{
  using ::floor;
  return Point (floor (s.x), floor (s.y));
}
using ::floor;
inline Point
ceil (const Point &s)
{
  using ::ceil;
  return Point (ceil (s.x), ceil (s.y));
}
using ::ceil;
inline Point
round (const Point &s)
{
  using ::round;
  return Point (round (s.x), ::round (s.y));
}
using ::round;

/* --- Rect --- */
class IRect;
class Rect {
public:
  double x, y;
  double width;
  double height;
  explicit      Rect            ();
  explicit      Rect            (Point cp0, Point cp1);
  explicit      Rect            (Point p0, double cwidth, double cheight);
  explicit      Rect            (double cx, double cy, double cwidth, double cheight);
  inline        Rect            (const IRect &ir);
  Rect&         assign          (Point p0, Point p1);
  Rect&         assign          (Point p0, double cwidth, double cheight);
  Rect&         assign          (double cx, double cy, double cwidth, double cheight);
  double        upper_x         () const { return x + width; }
  double        upper_y         () const { return y + height; }
  Point         upper_left      () const { return Point (x, y + height); }
  Point         upper_right     () const { return Point (x + width, y + height); }
  Point         lower_right     () const { return Point (x + width, y); }
  Point         lower_left      () const { return Point (x, y); }
  Point         ul              () const { return upper_left(); }
  Point         ur              () const { return upper_right(); }
  Point         lr              () const { return lower_right(); }
  Point         ll              () const { return lower_left(); }
  double        diagonal        () const { return ll().dist (ur()); }
  double        area            () const { return width * height; }
  Point         ul_tangent      () const;
  Point         ur_tangent      () const;
  Point         lr_tangent      () const;
  Point         ll_tangent      () const;
  Point         center          () const { return Point (x + width * 0.5, y + height * 0.5); }
  Point         north           () const { return Point (x + width * 0.5, y); }
  Point         north_east      () const { return upper_right(); }
  Point         east            () const { return Point (x + width, y + height * 0.5); }
  Point         south_east      () const { return lower_right(); }
  Point         south           () const { return Point (x + width * 0.5, y); }
  Point         south_west      () const { return lower_left(); }
  Point         west            () const { return Point (x, y + height * 0.5); }
  Point         north_west      () const { return upper_left(); }
  bool          contains        (const Point &point) const      { return x <= point.x && y <= point.y && point.x < x + width && point.y < y + height; }
  bool          operator==      (const Rect  &other) const;
  bool          operator!=      (const Rect  &other) const      { return !operator== (other); }
  bool          equals          (const Rect  &other, double epsilon = 0.0) const;
  double        dist2           (const Point &p) const;
  double        dist            (const Point &p) const;
  Rect&         rect_union      (const Rect &r);
  Rect&         add             (const Point &p);
  Rect&         add_border      (double b);
  Rect&         intersect       (const Rect &r);
  bool          intersecting    (const Rect &r) const;
  Rect          intersection    (const Rect &r) const           { return Rect (*this).intersect (r); }
  bool          empty           () const;
  Point         anchor_point    (Anchor anchor);
  Rect&         translate       (double deltax,
                                 double delty);
  Rect&         operator+       (const Point &p);
  Rect&         operator-       (const Point &p);
  String        string          () const;
  static Rect   create_anchored (Anchor anchor,
                                 double     width,
                                 double     height);
};
struct IRect {
  int64 x, y, width, height;
  IRect (const Rect &r) :
    x (ifloor (r.x)), y (ifloor (r.y)),
    width (iceil (r.width)), height (iceil (r.height))
  {}
  IRect&
  operator= (const Rect &r)
  {
    x = ifloor (r.x);
    y = ifloor (r.y);
    width = iceil (r.width);
    height = iceil (r.height);
    return *this;
  }
};

/* --- Color --- */
class Color {
  uint32              argb_pixel;
  typedef uint32 (Color::*_unspecified_bool_type) () const; // non-numeric operator bool() result
  static inline _unspecified_bool_type _unspecified_bool_true () { return &Color::argb; }
public:
  static inline uint8 IMUL    (uint8 v, uint8 alpha)            { return (v * alpha * 0x0101 + 0x8080) >> 16; }
  static inline uint8 IDIV    (uint8 v, uint8 alpha)            { return (0xff * v + (alpha >> 1)) / alpha; }
  static inline uint8 IDIV0   (uint8 v, uint8 alpha)            { if (!alpha) return 0; return IDIV (v, alpha); }
  Color (uint32 c = 0) :
    argb_pixel (c)
  {}
  Color (uint8 red, uint8 green, uint8 blue, uint8 alpha = 0xff) :
    argb_pixel ((alpha << 24) | (red << 16) | (green << 8) | blue)
  {}
  static Color from_name          (const String &color_name);
  static Color from_premultiplied (uint32 pargb)
  {
    Color c = (pargb);
    if (c.alpha())
      {
        c.red (IDIV (c.red(), c.alpha()));
        c.green (IDIV (c.green(), c.alpha()));
        c.blue (IDIV (c.blue(), c.alpha()));
      }
    else
      {
        c.red (0);
        c.green (0);
        c.blue (0);
      }
    return c;
  }
  uint32        premultiplied() const
  {
    /* extract alpha, red, green ,blue */
    uint32 a = alpha(), r = red(), g = green(), b = blue();
    /* bring into premultiplied form */
    r = IMUL (r, a);
    g = IMUL (g, a);
    b = IMUL (b, a);
    return Color (r, g, b, a).argb();
  }
  void
  set (uint8 red, uint8 green, uint8 blue)
  {
    argb_pixel = (alpha() << 24) | (red << 16) | (green << 8) | blue;
  }
  void
  set (uint8 red, uint8 green, uint8 blue, uint8 alpha)
  {
    argb_pixel = (alpha << 24) | (red << 16) | (green << 8) | blue;
  }
  uint32        rgb    () const { return argb_pixel & 0x00ffffff; }
  uint32        argb   () const { return argb_pixel; }
  uint          red    () const { return (argb_pixel >> 16) & 0xff; }
  uint          green  () const { return (argb_pixel >> 8) & 0xff; }
  uint          blue   () const { return argb_pixel & 0xff; }
  uint          alpha  () const { return argb_pixel >> 24; }
  double        red1   () const { return red() / 256.; }
  double        green1 () const { return green() / 256.; }
  double        blue1  () const { return blue() / 256.; }
  double        alpha1 () const { return alpha() / 256.; }
  inline operator _unspecified_bool_type () const { return alpha() ? _unspecified_bool_true() : 0; }
  Color&        alpha  (uint8 v) { argb_pixel &= 0x00ffffff; argb_pixel |= v << 24; return *this; }
  Color&        red    (uint8 v) { argb_pixel &= 0xff00ffff; argb_pixel |= v << 16; return *this; }
  Color&        green  (uint8 v) { argb_pixel &= 0xffff00ff; argb_pixel |= v << 8; return *this; }
  Color&        blue   (uint8 v) { argb_pixel &= 0xffffff00; argb_pixel |= v << 0; return *this; }
  double        get_hsv_value () { return max (max (red(), green()), blue()) / 255.; }
  void          set_hsv_value (double v);
  void          get_hsv (double *huep,          /* 0..360: 0=red, 120=green, 240=blue */
                         double *saturationp,   /* 0..1 */
                         double *valuep);       /* 0..1 */
  void          set_hsv (double hue,            /* 0..360: 0=red, 120=green, 240=blue */
                         double saturation,     /* 0..1 */
                         double value);         /* 0..1 */
  void          tint    (double hsv_shift, double saturation_factor, double value_factor);
  uint8         channel (uint nth) const
  {
    switch (nth)
      {
      case 0: return alpha();
      case 1: return red();
      case 2: return green();
      case 3: return blue();
      }
    return 0;
  }
  void          channel (uint nth, uint8 v)
  {
    switch (nth)
      {
      case 0: alpha (v); break;
      case 1: red (v);   break;
      case 2: green (v); break;
      case 3: blue (v);  break;
      }
  }
  Color&        shade  (uint8 lucent)
  {
    uint32 a = argb_pixel >> 24;
    alpha ((a * lucent + a + lucent) >> 8);
    return *this;
  }
  Color&        combine (const Color c2, uint8 lucent)
  {
    /* extract alpha, red, green ,blue */
    uint32 Aa = c2.alpha(), Ar = c2.red(), Ag = c2.green(), Ab = c2.blue();
    uint32 Ba = alpha(), Br = red(), Bg = green(), Bb = blue();
    /* weight A layer */
    Aa = IMUL (Aa, lucent);
    /* bring into premultiplied form */
    Ar = IMUL (Ar, Aa);
    Ag = IMUL (Ag, Aa);
    Ab = IMUL (Ab, Aa);
    Br = IMUL (Br, Ba);
    Bg = IMUL (Bg, Ba);
    Bb = IMUL (Bb, Ba);
    /* A over B = colorA + colorB * (1 - alphaA) */
    uint32 Ai = 255 - Aa;
    uint8 Dr = Ar + IMUL (Br, Ai);
    uint8 Dg = Ag + IMUL (Bg, Ai);
    uint8 Db = Ab + IMUL (Bb, Ai);
    uint8 Da = Aa + IMUL (Ba, Ai);
    /* un-premultiply */
    if (Da)
      {
        Dr = IDIV (Dr, Da);
        Dg = IDIV (Dg, Da);
        Db = IDIV (Db, Da);
      }
    else
      Dr = Dg = Db = 0;
    /* assign */
    alpha (Da);
    red (Dr);
    green (Dg);
    blue (Db);
    return *this;
  }
  Color&        blend_premultiplied (const Color c2, uint8 lucent)
  {
    /* extract alpha, red, green ,blue */
    uint32 Aa = c2.alpha(), Ar = c2.red(), Ag = c2.green(), Ab = c2.blue();
    uint32 Ba = alpha(), Br = red(), Bg = green(), Bb = blue();
    /* A over B = colorA * alpha + colorB * (1 - alpha) */
    uint32 ilucent = 255 - lucent;
    uint8 Dr = IMUL (Ar, lucent) + IMUL (Br, ilucent);
    uint8 Dg = IMUL (Ag, lucent) + IMUL (Bg, ilucent);
    uint8 Db = IMUL (Ab, lucent) + IMUL (Bb, ilucent);
    uint8 Da = IMUL (Aa, lucent) + IMUL (Ba, ilucent);
    /* assign */
    alpha (Da);
    red (Dr);
    green (Dg);
    blue (Db);
    return *this;
  }
  Color&        add_premultiplied (const Color c2, uint8 lucent)
  {
    /* extract alpha, red, green ,blue */
    uint32 Aa = alpha(), Ar = red(), Ag = green(), Ab = blue();
    uint32 Ba = c2.alpha(), Br = c2.red(), Bg = c2.green(), Bb = c2.blue();
    /* A add B = colorA + colorB */
    uint32 Dr = Ar + IMUL (Br, lucent);
    uint32 Dg = Ag + IMUL (Bg, lucent);
    uint32 Db = Ab + IMUL (Bb, lucent);
    uint32 Da = Aa + IMUL (Ba, lucent);
    /* assign */
    alpha (MIN (Da, 255));
    red   (MIN (Dr, 255));
    green (MIN (Dg, 255));
    blue  (MIN (Db, 255));
    return *this;
  }
  String string() const;
};

/* --- Affine --- */
class Affine {
protected:
  /* ( xx yx )
   * ( xy yy )
   * ( xz yz )
   */
  double xx, xy, xz, yx, yy, yz;
public:
  Affine (double cxx = 1,
          double cxy = 0,
          double cxz = 0,
          double cyx = 0,
          double cyy = 1,
          double cyz = 0) :
    xx (cxx), xy (cxy), xz (cxz),
    yx (cyx), yy (cyy), yz (cyz)
  {}
  Affine&
  translate (double tx, double ty)
  {
    multiply (Affine (1, 0, tx, 0, 1, ty));
    return *this;
  }
  Affine&
  translate (Point p)
  {
    multiply (Affine (1, 0, p.x, 0, 1, p.y));
    return *this;
  }
  Affine&
  set_translation (double tx, double ty)
  {
    xz = tx;
    yz = ty;
    return *this;
  }
  Affine&
  hflip()
  {
    xx = -xx;
    yx = -yx;
    xz = -xz;
    return *this;
  }
  Affine&
  vflip()
  {
    xy = -xy;
    yy = -yy;
    yz = -yz;
    return *this;
  }
  Affine&
  rotate (double theta)
  {
    double s = sin (theta);
    double c = cos (theta);
    return multiply (Affine (c, -s, 0, s, c, 0));
  }
  Affine&
  rotate (double theta, Point anchor)
  {
    translate (anchor.x, anchor.y);
    rotate (theta);
    return translate (-anchor.x, -anchor.y);
  }
  Affine&
  scale (double sx, double sy)
  {
    xx *= sx;
    xy *= sy;
    yx *= sx;
    yy *= sy;
    return *this;
  }
  Affine&
  shear (double shearx, double sheary)
  {
    return multiply (Affine (1, shearx, 0, sheary, 1, 0));
  }
  Affine&
  shear (double theta)
  {
    return multiply (Affine (1, tan (theta), 0, 0, 1, 0));
  }
  Affine&
  multiply (const Affine &a2)
  {
    Affine dst; // dst * point = this * (a2 * point)
    dst.xx = a2.xx * xx + a2.yx * xy;
    dst.xy = a2.xy * xx + a2.yy * xy;
    dst.xz = a2.xz * xx + a2.yz * xy + xz;
    dst.yx = a2.xx * yx + a2.yx * yy;
    dst.yy = a2.xy * yx + a2.yy * yy;
    dst.yz = a2.xz * yx + a2.yz * yy + yz;
    return *this = dst;
  }
  Affine&
  multiply_swapped (const Affine &a2)
  {
    Affine dst; // dst * point = a2 * (this * point)
    dst.xx = xx * a2.xx + yx * a2.xy;
    dst.xy = xy * a2.xx + yy * a2.xy;
    dst.xz = xz * a2.xx + yz * a2.xy + a2.xz;
    dst.yx = xx * a2.yx + yx * a2.yy;
    dst.yy = xy * a2.yx + yy * a2.yy;
    dst.yz = xz * a2.yx + yz * a2.yy + a2.yz;
    return *this = dst;
  }
  Point
  point (const Point &s) const
  {
    Point d;
    d.x = xx * s.x + xy * s.y + xz;
    d.y = yx * s.x + yy * s.y + yz;
    return d;
  }
  Point point (double x, double y) const { return point (Point (x, y)); }
  double
  determinant() const
  {
    /* if this is != 0, the affine is invertible */
    return xx * yy - xy * yx;
  }
  double
  expansion() const
  {
    return sqrt (fabs (determinant()));
  }
  double
  hexpansion() const
  {
    Point p0 = point (Point (0, 0));
    Point p1 = point (Point (1, 0));
    return p1.dist (p0);
  }
  double
  vexpansion() const
  {
    Point p0 = point (Point (0, 0));
    Point p1 = point (Point (0, 1));
    return p1.dist (p0);
  }
  Affine&
  invert()
  {
    double rec_det = 1.0 / determinant();
    Affine dst (yy, -xy, 0, -yx, xx, 0);
    dst.xx *= rec_det;
    dst.xy *= rec_det;
    dst.yx *= rec_det;
    dst.yy *= rec_det;
    dst.xz = -(dst.xx * xz + dst.xy * yz);
    dst.yz = -(dst.yy * yz + dst.yx * xz);
    return *this = dst;
  }
  Point
  ipoint (const Point &s) const
  {
    double rec_det = 1.0 / determinant();
    Point d;
    d.x = yy * s.x - xy * s.y;
    d.x *= rec_det;
    d.x -= xz;
    d.y = xx * s.y - yx * s.x;
    d.y *= rec_det;
    d.y -= yz;
    return d;
  }
  Point ipoint (double x, double y) const { return ipoint (Point (x, y)); }
  Point
  operator* (const Point &p) const
  {
    return point (p);
  }
  Affine
  operator* (const Affine &a2) const
  {
    return Affine (*this).multiply (a2);
  }
  Affine&
  operator= (const Affine &a2)
  {
    xx = a2.xx;
    xy = a2.xy;
    xz = a2.xz;
    yx = a2.yx;
    yy = a2.yy;
    yz = a2.yz;
    return *this;
  }
  bool
  is_identity () const
  {
    return RAPICORN_ISLIKELY (xx == 1 && xy == 0 && xz == 0 && yx == 0 && yy == 1 && yz == 0);
  }
  Affine
  create_inverse () const
  {
    Affine inv (*this);
    return inv.invert();
  }
  bool  operator== (const Affine &oa) const { return xx == oa.xx && xy == oa.xy && xz == oa.xz && yx == oa.yx && yy == oa.yy && yz == oa.yz; }
  bool  operator!= (const Affine &oa) const { return !operator== (oa); }
  String        string() const;
  static Affine from_triangles (Point src_a, Point src_b, Point src_c,
                                Point dst_a, Point dst_b, Point dst_c);
  struct VectorReturn { double x, y, z; };
  VectorReturn x() const { VectorReturn v = { xx, xy, xz }; return v; }
  VectorReturn y() const { VectorReturn v = { yx, yy, yz }; return v; }
};
struct AffineIdentity : Affine {
  AffineIdentity() :
    Affine (1, 0, 0, 0, 1, 0)
  {}
};
struct AffineHFlip : Affine {
  AffineHFlip() :
    Affine (-1, 0, 0, 0, 1, 0)
  {}
};
struct AffineVFlip : Affine {
  AffineVFlip() :
    Affine (1, 0, 0, 0, -1, 0)
  {}
};
struct AffineTranslate : Affine {
  AffineTranslate (double tx, double ty)
  {
    xx = 1;
    xy = 0;
    xz = tx;
    yx = 0;
    yy = 1;
    yz = ty;
  }
  AffineTranslate (Point p)
  {
    xx = 1;
    xy = 0;
    xz = p.x;
    yx = 0;
    yy = 1;
    yz = p.y;
  }
};
struct AffineScale : Affine {
  AffineScale (double sx, double sy)
  {
    xx = sx;
    xy = 0;
    xz = 0;
    yx = 0;
    yy = sy;
    yz = 0;
  }
};
struct AffineRotate : Affine {
  AffineRotate (double theta)
  {
    double s = sin (theta);
    double c = cos (theta);
    xx = c;
    xy = -s;
    xz = 0;
    yx = s;
    yy = c;
    yz = 0;
  }
  AffineRotate (double theta, Point anchor)
  {
    rotate (theta, anchor);
  }    
};
struct AffineShear : Affine {
  AffineShear (double shearx, double sheary)
  {
    xx = 1;
    xy = shearx;
    xz = 0;
    yx = sheary;
    yy = 1;
    yz = 0;
  }
  AffineShear (double theta)
  {
    xx = 1;
    xy = tan (theta);
    xz = 0;
    yx = 0;
    yy = 1;
    yz = 0;
  }
};

/* --- implementations --- */
inline bool
Point::equals (const Point &p2,
               double       epsilon) const
{
  return fabs (x - p2.x) <= epsilon && fabs (y - p2.y) <= epsilon;
}

inline
Rect::Rect () :
  x (0), y (0), width (0), height (0)
{}

inline Rect&
Rect::assign (double cx, double cy, double cwidth, double cheight)
{
  x = cx;
  y = cy;
  width = MAX (cwidth, 0);
  height = MAX (cheight, 0);
  return *this;
}

inline
Rect::Rect (double cx, double cy, double cwidth, double cheight)
{
  assign (cx, cy, cwidth, cheight);
}

inline Rect&
Rect::assign (Point p0, double cwidth, double cheight)
{
  return assign (p0.x, p0.y, cwidth, cheight);
}

inline
Rect::Rect (Point p0, double cwidth, double cheight)
{
  assign (p0, cwidth, cheight);
}

inline Rect&
Rect::assign (Point p0, Point p1)
{
  Point mll (min (p0, p1));
  Point mur (max (p0, p1));
  x = p0.x;
  y = p0.y;
  width = mur.x - mll.x;
  height = mur.y - mll.y;
  return *this;
}

inline
Rect::Rect (Point cp0, Point cp1)
{
  assign (cp0, cp1);
}

inline
Rect::Rect (const IRect &ir) :
  x (ir.x), y (ir.y), width (ir.width), height (ir.height)
{}

inline bool
Rect::operator== (const Rect &other) const
{
  return other.x == x && other.y == y && other.width == width && other.height == height;
}

inline bool
Rect::equals (const Rect &other,
              double      epsilon) const
{
  if (empty() && other.empty())
    return true;
  return (fabs (other.x - x) <= epsilon &&
          fabs (other.y - y) <= epsilon &&
          fabs (other.width - width) <= epsilon &&
          fabs (other.height - height) <= epsilon);
}

inline Rect&
Rect::add_border (double b)
{
  x -= b;
  y -= b;
  width += 2 * b;
  height += 2 * b;
  return *this;
}

inline Rect&         
Rect::add (const Point &p)
{
  if (empty())
    assign (p, 0, 0);
  else
    assign (min (p, lower_left()), max (p, upper_right()));
  return *this;
}

inline Rect&
Rect::rect_union (const Rect &r)
{
  if (r.empty())
    return *this;
  if (empty())
    return *this = r;
  Point mll = min (lower_left(), r.lower_left());
  Point mur = max (upper_right(), r.upper_right());
  assign (mll, mur);
  return *this;
}

inline Rect&
Rect::intersect (const Rect &r)
{
  Point pll = max (lower_left(), r.lower_left());
  Point pur = min (upper_right(), r.upper_right());
  if (pll.x <= pur.x && pll.y <= pur.y)
    assign (pll, pur);
  else
    *this = Rect();
  return *this;
}

inline bool
Rect::intersecting (const Rect &r) const
{
  return (((r.x >= x && r.x < x + width) ||
           (x >= r.x && x < r.x + r.width)) &&
          ((r.y >= y && r.y < y + height) ||
           (y >= r.y && y < r.y + r.height)));
}

inline bool
Rect::empty () const
{
  return width * height == 0;
}

inline Rect&
Rect::translate (double deltax,
                 double delty)
{
  x += deltax;
  y += delty;
  return *this;
}

} // Rapicorn

#endif  /* __RAPICORN_PRIMITIVES_HH__ */
