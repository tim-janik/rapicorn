/* Rapicorn
 * Copyright (C) 2005 Tim Janik
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
#include "painter.hh"
#include <algorithm>

namespace Rapicorn {

/* auxillary structure to count dash state for stippled outlines */
struct Dasher {
  const vector<int> &dashes;
  int total;
  uint index, counter;
  Dasher (const vector<int> &d,
          int                offset) :
    dashes (d), total (0), index (0), counter (0)
  {
    for (uint i = 0; i < dashes.size(); i++)
      total += dashes[i];
    if (total)
      {
        offset %= total;
        if (offset < 0)
          offset += total;
        increment (offset);
      }
  }
  uint current_dash()
  {
    return index < dashes.size() ? dashes[index] : 0;
  }
  void increment1 ()
  {
    counter++;
    if (counter >= current_dash())
      {
        counter = 0;
        index++;
        index %= MAX (1, dashes.size());
      }
  }
  void increment (uint n)
  {
    if (n && total)
      {
        n %= total;
        while (n >= current_dash() - counter)
          {
            n -= current_dash() - counter;
            counter = 0;
            index++;
            index %= dashes.size();
          }
        while (n--)
          increment1();
      }
  }
  bool on()
  {
    return !(index & 1);
  }
  Dasher& operator++()
  {
    increment1();
    return *this;
  }
};

/* --- Painter --- */
Painter::Painter (Plane &plane) :
  m_plane (plane)
{}

Painter::~Painter ()
{}

void
Painter::draw_hline (int x0, int x1, int y, Color c, const vector<int> &dashes, int dash_offset)
{
  Dasher dd (dashes, dash_offset);
  int xmin = MAX (MIN (x0, x1), xstart());
  int xmax = MIN (MAX (x0, x1), xbound() - 1);
  if (y >= ystart() && y < ybound())
    {
      if (!dd.total)
        for (int x = xmin; x <= xmax; x++, ++dd)
          set (x, y, c);
      else if (x0 <= x1)
        {
          dd.increment (xmin - x0);
          for (int x = xmin; x <= xmax; x++, ++dd)
            if (dd.on())
              set (x, y, c);
        }
      else
        {
          dd.increment (x0 - xmax);
          for (int x = xmax; x >= xmin; x--, ++dd)
            if (dd.on())
              set (x, y, c);
        }
    }
}

void
Painter::draw_vline (int x, int y0, int y1, Color c, const vector<int> &dashes, int dash_offset)
{
  Dasher dd (dashes, dash_offset);
  int ymin = MAX (MIN (y0, y1), ystart());
  int ymax = MIN (MAX (y0, y1), ybound() - 1);
  if (x >= xstart() && x < xbound())
    {
      if (!dd.total)
        for (int y = ymin; y <= ymax; y++)
          set (x, y, c);
      else if (y0 <= y1)
        {
          dd.increment (ymin - y0);
          for (int y = ymin; y <= ymax; y++, ++dd)
            if (dd.on())
              set (x, y, c);
        }
      else
        {
          dd.increment (y0 - ymax);
          for (int y = ymax; y >= ymin; y--, ++dd)
            if (dd.on())
              set (x, y, c);
        }
    }
}

void
Painter::draw_shadow (int x, int y, int width, int height,
                      Color outer_upper_left, Color inner_upper_left,
                      Color inner_lower_right, Color outer_lower_right)
{
  if (width > 0 && height > 0)
    {
      /* outer lower-right */
      draw_hline (x, x + width - 1, y, outer_lower_right);
      draw_vline (x + width - 1, y, y + height - 1, outer_lower_right);
    }
  if (width > 1 && height > 1)
    {
      /* inner lower-right */
      draw_hline (x + 1, x + width - 2, y + 1, inner_lower_right);
      draw_vline (x + width - 2, y + 1, y + height - 2, inner_lower_right);
    }
  if (width > 0 && height > 0)
    {
      /* outer upper-left */
      draw_hline (x + width - 1, x, y + height - 1, outer_upper_left);
      draw_vline (x, y + height - 1, y, outer_upper_left);
    }
  if (width > 1 && height > 1)
    {
      /* inner upper-left */
      draw_hline (x + width - 2, x + 1, y + height - 2, inner_upper_left);
      draw_vline (x + 1, y + height - 2, y + 1, inner_upper_left);
    }
}

void
Painter::draw_border (int x, int y, int width, int height, Color border, const vector<int> &dashes, int dash_offset)
{
  if (width && height)
    {
      /* adjust the dash offset below by consumed length and by -1 for overlapping corners */
      draw_hline (x, x + width - 1, y, border, dashes, dash_offset);
      draw_vline (x + width - 1, y, y + height - 1, border, dashes, dash_offset + width - 1);
      draw_hline (x + width - 1, x, y + height - 1, border, dashes, dash_offset + width + height - 2);
      draw_vline (x, y + height - 1, y, border, dashes, dash_offset + width + height + width - 3);
    }
}

static inline uint32
quick_rand32 ()
{
  static uint32 accu = 2147483563;
  accu = 1664525 * accu + 1013904223;
  return accu;
}

void
Painter::draw_shaded_rect (int xc0, int yc0, Color color0, int xc1, int yc1, Color color1)
{
  draw_gradient_rect (MIN (xc0, xc1), MIN (yc0, yc1),
                      MAX (xc0, xc1) - MIN (xc0, xc1) + 1,
                      MAX (yc0, yc1) - MIN (yc0, yc1) + 1,
                      xc0, yc0, color0,
                      xc1, yc1, color1);
}

/* find u for (px,py) = (x1,y1) + u * (x2-x1,y2-y1)
 * <=> u = ( (px-x1)*(x2-x1) + (py-y1)*(y2-y1) ) / length(x2y2-x1y1)^2
 */
static inline double
perpendicular_point (double x1, double y1,
                     double x2, double y2,
                     double px, double py)
{
  /* gradient points distance and length^2 */
  double dx = x2 - x1, dy = y2 - y1;
  double length2 = dx * dx + dy * dy;
  if (length2 > 0)
    {
      /* start point distance */
      double jx = px - x1, jy = py - y1;
      /* fraction into line for perpendicular intersection */
      double u = (jx * dx + jy * dy) / length2;
      return u; /* u<0 => "before" line; u>1 => "after" line */
    }
  return -1; /* consider (px,py) "before" line of length 0 */
}

/* find scan line crossings for perpendiculars through line end points */
static inline bool
parallels_cut_point (double  x1,  double  y1, // line start
                     double  x2,  double  y2, // line end
                     double  py,              // scan line
                     double &xu0, double &xu1)
{
  /* gradient points distance and length^2 */
  double dx = x2 - x1, dy = y2 - y1;
  double length2 = dx * dx + dy * dy;
  if (dx > 0)
    {
      //        u = ((px - x1) * dx + (py - y1) * dy) / length2
      //  <=>  px = (u * length2 - (py - y1) * dy) / dx + x1
      xu0 = x1 + (y1 - py) * dy / dx;
      xu1 = x1 + (length2 + (y1 - py) * dy) / dx;
      return true;
    }
  return false; /* vertical gradient, perpendicular to (*,py) */
}

static inline Color
interpolate_colors (Color  color1,
                    Color  color2,
                    double weight)
{
  if (weight <= 0)
    return color1;
  if (weight >= 1)
    return color2;
  double a = 1 - weight, b = weight;
  Color A = color1.premultiplied(), B = color2.premultiplied();
  double Ca = A.alpha() * a, Cr = A.red() * a, Cg = A.green() * a, Cb = A.blue() * a;
  Ca += B.alpha() * b;
  Cr += B.red() * b;
  Cg += B.green() * b;
  Cb += B.blue() * b;
  return Color::from_premultiplied (Color (dtoi32 (Cr), dtoi32 (Cg), dtoi32 (Cb), dtoi32 (Ca)));
}

#define SWAP(a,b)       ({ __typeof (a) __tmp = b; b = a; a = __tmp; })

void
Painter::draw_gradient_rect (int64 recx,     int64 recy,
                             int64 recwidth, int64 recheight,
                             int64 c0x,      int64 c0y, Color color0,
                             int64 c1x,      int64 c1y, Color color1)
{
  const int64 x1 = MAX (xstart(), recx), x2 = MIN (xbound() - 1, x1 + recwidth - 1);
  const int64 y1 = MAX (ystart(), recy), y2 = MIN (ybound() - 1, y1 + recheight - 1);
  const double vd = c0x == c1x ? 0.2 : 0; // enforce non-vertical gradients
  /* ensure color0 point is left of color1 point */
  if (c0x > c1x)
    {
      SWAP (c0x, c1x);
      SWAP (c0y, c1y);
      SWAP (color0, color1);
    }
  /* for each scan line, we have 3 segments to render:
   *   ---before---/~~~gradient~~~/---after---
   *              xu0            xu1
   * unless we have a vertical gradient.
   */
  for (int64 yy = y1; yy <= y2; yy++)
    {
      double xu0, xu1;
      if (!parallels_cut_point (c0x - vd, c0y, c1x + vd, c1y, yy, xu0, xu1))
        {
          /* vertical gradient */
          double u = perpendicular_point (c0x, c0y, c1x, c1y, x1, yy);
          if (u <= 0)
            fill_scan_line (yy, x1, x2 - x1 + 1, color0);
          else if (u >= 1)
            fill_scan_line (yy, x1, x2 - x1 + 1, color1);
          else
            {
              /* this misses dithering but should never be triggered, since vd > 0 here */
              Color col = interpolate_colors (color0, color1, u);
              fill_scan_line (yy, x1, x2 - x1 + 1, col);
            }
          continue;
        }
      int64 xi0 = iround (xu0), xi1 = iround (xu1);
      if (xu0 > xu1)
        SWAP (xu0, xu1); // force: xu0 <= xu1
      if (x1 < xu0)
        fill_scan_line (yy, x1, MIN (xi0, x2 + 1) - x1, color0);   // [x1..xu0[
      if (x1 <= xu1)
        fill_line_gradient (yy, x1, x2, xi0, color0, xi1, color1); // [x1..x2] or [xu0..xu1]
      if (x2 > xu1)
        fill_scan_line (yy, xi1 + 1, x2 - xi1, color1);            // ]xu1..x2]
    }
}

void
Painter::fill_scan_line (int64 yy,
                         int64 x1, int64 width,
                         Color color0)
{
  Color pre0 = color0.premultiplied();
  int64 sx = MAX (xstart(), x1);
  int64 ex = MIN (xbound(), x1 + width);
  int64 len = MAX (ex, sx) - sx;
  uint32 *pixel = m_plane.poke_span (sx, yy, len);
  if (pixel)
    {
      uint32 col = pre0;
      memset4 (pixel, col, len);
    }
}

void
Painter::fill_line_gradient (int64 yy,
                             int64 lx1, int64 lx2,
                             int64 xc0, Color color0,
                             int64 xc1, Color color1)
{
  return_if_fail (xc0 <= xc1);
  return_if_fail (lx1 <= lx2);
  const int64 x1 = MAX (xstart(), MAX (lx1, xc0)), x2 = MIN (xbound() - 1, MIN (lx2, xc1));
  if (x2 < x1)
    return;
  /* extract alpha, red, green, blue */
  Color A = color0.premultiplied(), B = color1.premultiplied();
  double Aa = A.alpha(), Ar = A.red(), Ag = A.green(), Ab = A.blue(); // [0..255]
  double Ba = B.alpha(), Br = B.red(), Bg = B.green(), Bb = B.blue(); // [0..255]
  double Da = Ba - Aa,   Dr = Br - Ar, Dg = Bg - Ag,   Db = Bb - Ab;  // B - A color delta
  double Dx = xc1 - xc0; // color step per pixel
  // distribute color parts along pixels, shift left by 8 for dither arithmetic
  Da = Da / Dx * 0xff;
  Dr = Dr / Dx * 0xff;
  Dg = Dg / Dx * 0xff;
  Db = Db / Dx * 0xff;
  Aa *= 256;
  Ar *= 256;
  Ag *= 256;
  Ab *= 256;
  uint32 *pixel = m_plane.poke_span (x1, yy, x2 - x1 + 1);
  for (int64 ii = x1 - xc0; ii <= x2 - xc0; ii++)
    {
      /* interpolate colors (still shifted left by 8) */
      int32 Ia = dtoi32 (Aa + Da * ii), Ir = dtoi32 (Ar + Dr * ii), Ig = dtoi32 (Ag + Dg * ii), Ib = dtoi32 (Ab + Db * ii);
      /* dither */
      union { uint32 i32; uint8 b[4]; } rnd = { quick_rand32() };
      Ir += rnd.b[1];
      Ig += rnd.b[2];
      Ib += rnd.b[3];
      Ia += MAX (MAX (rnd.b[0], rnd.b[1]), MAX (rnd.b[2], rnd.b[3])); // preserve premul constraint
      /* shift right by 8, apply pixel */
      pixel[ii - x1 + xc0] = Color (Ir >> 8, Ig >> 8, Ib >> 8, Ia >> 8);
    }
}

void
Painter::draw_filled_rect (int x, int y, int width, int height, Color fill_color)
{
  Color pre = fill_color.premultiplied();
  for (int iy = y; iy < y + height; iy++)
    for (int ix = x; ix < x + width; ix++)
      set_premultiplied (ix, iy, pre);
}

void
Painter::draw_dir_arrow (double x, double y, double width, double height, Color c, DirType dir)
{
  double xhalf = width * 0.5, yhalf = height * 0.5;
  switch (dir)
    {
    case DIR_RIGHT:
      draw_trapezoid (y, x, x, y + yhalf, x, x + width, c);
      draw_trapezoid (y + height, x, x, y + yhalf, x, x + width, c);
      break;
    case DIR_UP:
      draw_trapezoid (y, x, x + width, y + height, x + xhalf, x + xhalf, c);
      break;
    case DIR_LEFT:
      draw_trapezoid (y, x + width, x + width, y + yhalf, x, x + width, c);
      draw_trapezoid (y + height, x + width, x + width, y + yhalf, x, x + width, c);
      break;
    case DIR_DOWN:
      draw_trapezoid (y + height, x, x + width, y, x + xhalf, x + xhalf, c);
      break;
    default:
      break;
    }
}

#define SQR(x) ((x) * (x))

void
Painter::draw_simple_line (double x0, double y0, double x1, double y1, double thickness, Color fill_color)
{
  /* calculate quadrangle coordinates from line by applying thickness */
  double thickness2 = thickness * 0.5;
  double dx = x1 - x0, dy = y1 - y0;
  double lx = thickness2 * dx / sqrt (dx * dx + dy * dy), ly = thickness2 * dy / sqrt (dx * dx + dy * dy);
  Point points[4];
  points[0].x = x1 + lx - ly;
  points[0].y = y1 + lx + ly;
  points[1].x = x1 + lx + ly;
  points[1].y = y1 - lx + ly;
  points[2].x = x0 - lx + ly;
  points[2].y = y0 - lx - ly;
  points[3].x = x0 - lx - ly;
  points[3].y = y0 + lx - ly;
  draw_quadrangle (points, fill_color);
}

struct LesserPointYX {
  inline bool
  operator() (const Point &p1,
              const Point &p2) const
  {
    if (p1.y != p2.y)
      return p1.y < p2.y;
    else
      return p1.x < p2.x;
  }
};

void
Painter::draw_quadrangle (const Point cpoints[4],
                          Color       fill_color)
{
  const bool print_diag = 0;
  Point p[4] = { cpoints[0], cpoints[1], cpoints[2], cpoints[3] };
  /* sort points by Y then X */
  std::stable_sort (&p[0], &p[4], LesserPointYX());
  if (print_diag)
    diag ("quadrangle: (%f, %f) (%f, %f) (%f, %f) (%f, %f)", p[0].x, p[0].y, p[1].x, p[1].y, p[2].x, p[2].y, p[3].x, p[3].y);
  /* divide rectangle into trapezoids, we need two auxillary
   * points for this, q and r:
   *                             p3
   *                            /\
   *                           /  \
   *                          /    \
   *                         /      \
   *                        /   A    \
   *                    p2 /__________\ r
   *                       |           \
   *                       |   B        \
   *                     q |_____________\ p1
   *                       |        __--
   *                       | C  __--
   *                       |__--
   *                       '
   *                      p0
   */
  Point r = p[2], q = p[1];
  if (p[2].y != p[0].y) /* need C and/or B */
    q.x = p[0].x + (p[2].x - p[0].x) * (p[1].y - p[0].y) / (p[2].y - p[0].y);
  if (p[1].y != p[3].y) /* need A and/or B */
    r.x = p[3].x + (p[1].x - p[3].x) * (p[2].y - p[3].y) / (p[1].y - p[3].y);
  /* A */
  if (print_diag)
    diag ("quadrangle: trapA: (%f, %f, %f) (%f, %f, %f)", p[3].y, p[3].x, p[3].x, r.y, p[2].x, r.x);
  draw_trapezoid (p[3].y, p[3].x, p[3].x, r.y, p[2].x, r.x, fill_color);
  /* B */
  if (print_diag)
    diag ("quadrangle: trapB: (%f, %f, %f) (%f, %f, %f)", r.y, p[2].x, r.x, q.y, q.x, p[1].x);
  draw_trapezoid (r.y, p[2].x, r.x, q.y, q.x, p[1].x, fill_color);
  /* C */
  if (print_diag)
    diag ("quadrangle: trapC: (%f, %f, %f) (%f, %f, %f)", q.y, q.x, p[1].x, p[0].y, p[0].x, p[0].x);
  draw_trapezoid (q.y, q.x, p[1].x, p[0].y, p[0].x, p[0].x, fill_color);
}

static inline double
left_trapezoid_area (const double by,
                     const double bx1,
                     const double bx2,
                     const double ty,
                     const double tx1,
                     const double tx2,
                     const double dy,
                     const double sx)
{
  /* split trapezoid, calculate area of left half */
  if (sx < tx1 && sx < bx1)
    return 0;
  else if (sx > tx2 && sx > bx2)
    return 0.5 * (bx2 - bx1 + tx2 - tx1) * dy;
  else if (sx > bx2)    /* right edge triangle split / |/ */
    {
      double ry = by + (sx - bx2) * dy / (tx2 - bx2);
      double right_area = 0.5 * (tx2 - sx) * (ty - ry);
      double total_area = 0.5 * (bx2 - bx1 + tx2 - tx1) * dy;
      return total_area - right_area;
    }
  else if (sx > tx2)    /* right edge triangle split \ |\ */
    {
      double ry = by + (bx2 - sx) * dy / (bx2 - tx2);
      double right_area = 0.5 * (bx2 - sx) * (ry - by);
      double total_area = 0.5 * (bx2 - bx1 + tx2 - tx1) * dy;
      return total_area - right_area;
    }
  else if (sx < bx1)    /* left edge triangle split \| \ */
    {
      double ry = by + (bx1 - sx) * dy / (bx1 - tx1);
      return 0.5 * (sx - tx1) * (ty - ry);
    }
  else if (sx < tx1)    /* left edge triangle split /| / */
    {
      double ry = by + (sx - bx1) * dy / (tx1 - bx1);
      return 0.5 * (sx - bx1) * (ry - by);
    }
  else                  /* split in parallel region */
    return 0.5 * (sx - tx1 + sx - bx1) * dy;
}

inline void
Painter::draw_trapezoid_run (const double by,
                             const double bx1,
                             const double bx2,
                             const double ty,
                             const double tx1,
                             const double tx2,
                             Color        fg_premul)
{
  /* ignore invalid/empty cases, often caused by rounding artefacts */
  if (ty <= by)
    return;
  // assert (bx2 >= bx1 && tx2 >= tx1); // can fail due to optimization dependent precision variations
  // diag ("trap: (%f, %f, %f) (%f, %f, %f)", by, bx1, bx2, ty, tx1, tx2);
  // assert (bx2 >= tx1 && bx1 <= tx2); // can fail due to optimization dependent precision variations
  const double dy = (ty - by);
  const double xs = CLAMP (MIN (tx1, bx1), xstart(), xbound());
  const double xe = CLAMP (MAX (tx2, bx2), xstart(), xbound());
  double last_left_area = left_trapezoid_area (by, bx1, bx2, ty, tx1, tx2, dy, xs);
  const int64 xsfloor = ifloor (xs), xeceil = iceil (xe);
  uint32 *d = m_plane.poke_span (xsfloor, ifloor (by), xeceil - xsfloor);
  for (double xi = xsfloor; xi < xeceil; xi += 1)
    {
      const double sx = MIN (xi + 1, xe);
      double left_area = left_trapezoid_area (by, bx1, bx2, ty, tx1, tx2, dy, sx);
      double area = left_area - last_left_area;
      int alpha = dtoi32 (area * 255);
      *d = Color (*d).add_premultiplied (fg_premul, alpha);
      last_left_area = left_area;
      d++;
    }
}

inline void
Painter::draw_trapezoid_row (double by,
                             double bx1,
                             double bx2,
                             double ty,
                             double tx1,
                             double tx2,
                             Color  fg_premul)
{
  /* divide trapezoids with too distant parallels into new
   * trapezoids for which bx2 >= tx1 && tx2 >= bx1):
   * _____            _____
   * \    \           \    \
   *  \    \           \    \
   *   \    \           \    \
   *    \    \     =>    \    \
   *  _ _\ _ _\ _ _       \____\ + _____
   *      \    \                   \    \
   *       \    \                   \    \
   *        \____\                   \____\
   */
  if (tx1 > bx2)        /* split right shear / / */
    {
      double last_by = by - 9e9;
      while (tx1 > bx2 && by > last_by)
        {
          const double newx1 = bx2;
          const double newy  = by  + (newx1 - bx1) * (ty - by)   / (tx1 - bx1);
          const double newx2 = bx2 + (newy - by)   * (tx2 - bx2) / (ty - by);
          /* draw split off trapezoid */
          draw_trapezoid_run (by, bx1, bx2, newy, newx1, newx2, fg_premul);
          /* restart with reminder */
          last_by = by;
          by = newy; bx1 = newx1; bx2 = newx2;
        }
      if (tx1 > bx2)    /* merge small gaps */
        bx2 = tx1 = 0.5 * (bx2 + tx1);
    }
  else if (bx1 > tx2)   /* split left shear \ \ */
    {
      double last_by = by - 9e9;
      while (bx1 > tx2 && by > last_by)
        {
          const double newx2 = bx1;
          const double newy  = by  + (bx2 - newx2) * (ty - by)   / (bx2 - tx2);
          const double newx1 = bx1 - (newy - by)   * (bx1 - tx1) / (ty - by);
          /* draw split off trapezoid */
          draw_trapezoid_run (by, bx1, bx2, newy, newx1, newx2, fg_premul);
          /* restart with reminder */
          last_by = by;
          by = newy; bx1 = newx1; bx2 = newx2;
        }
      if (bx1 > tx2)    /* merge small gaps */
        tx2 = bx1 = 0.5 * (tx2 + bx1);
    }
  /* well formed trapezoid */
  draw_trapezoid_run (by, bx1, bx2, ty, tx1, tx2, fg_premul);
}

void
Painter::draw_trapezoid (double c1y, double c1x1, double c1x2, double c2y, double c2x1, double c2x2, Color fill_color)
{
  if (c1y > c2y) /* swap */
    {
      const double sy = c1y, sx1 = c1x1, sx2 = c1x2;
      c1y = c2y; c1x1 = c2x1; c1x2 = c2x2;
      c2y = sy; c2x1 = sx1; c2x2 = sx2;
    }
  else if (c1y == c2y)
    return;
  Color fg_premul = fill_color.premultiplied();
  const double bot_y = c1y, bot_x1 = MIN (c1x1, c1x2), bot_x2 = MAX (c1x1, c1x2);
  const double top_y = c2y, top_x1 = MIN (c2x1, c2x2), top_x2 = MAX (c2x1, c2x2);
  /* render trapezoid with subpixel coordinates:
   *                                                        ########################################
   *                                                              #             #             #
   *                                                              #             #             #
   *    top_y __         top_x1 _______________ top_x2            #             #             #
   *                           /               \                  #             #             #
   *                          /                 \r                #             #             #
   *                      ---/-------------------\---       ########################################
   *                        /q                    \               #             #             #
   *                       /                       \              #             #             #
   *    bot_y __          /_________________________\             #             #             #
   *               bot_x1                             bot_x2      #             #             #
   *                                                              #             #             #
   *                                                        ########################################
   */
  const double dx1_from_height = (top_x1 - bot_x1) / (top_y - bot_y);
  const double dx2_from_height = (top_x2 - bot_x2) / (top_y - bot_y);
  const double iymax = MIN (ceil (top_y), ybound());
  double ynext;
  for (double iy = MAX (ystart(), floor (bot_y)); iy < iymax; iy = ynext)
    {
      ynext = ceil (iy + 1);
      const double by = MAX (bot_y, iy);
      const double bqx = bot_x1 + (by - bot_y) * dx1_from_height;
      const double brx = bot_x2 + (by - bot_y) * dx2_from_height;
      const double ty = MIN (top_y, ynext);
      const double tqx = bot_x1 + (ty - bot_y) * dx1_from_height;
      const double trx = bot_x2 + (ty - bot_y) * dx2_from_height;
      // preserve invariants despite double quntization on arg passing
      const double bx1 = MIN (bqx, brx);
      const double bx2 = MAX (brx, bqx);
      const double tx1 = MIN (tqx, trx);
      const double tx2 = MAX (trx, tqx);
      if (MIN (bx1, tx1) < xbound() && MAX (bx2, tx2) >= xstart())
        draw_trapezoid_row (by, bx1, bx2, ty, tx1, tx2, fg_premul);
    }
}

} // Rapicorn
