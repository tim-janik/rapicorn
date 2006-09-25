/* Rapicorn
 * Copyright (C) 2005 Tim Janik
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
  Point c0 (xc0, yc0);
  Color pre0 = color0.premultiplied(), pre1 = color1.premultiplied();
  /* extract alpha, red, green ,blue */
  float Aa = pre1.alpha(), Ar = pre1.red(), Ag = pre1.green(), Ab = pre1.blue();
  float Ba = pre0.alpha(), Br = pre0.red(), Bg = pre0.green(), Bb = pre0.blue();
  int x0 = MIN (xc0, xc1), x1 = MAX (xc0, xc1);
  int y0 = MIN (yc0, yc1), y1 = MAX (yc0, yc1);
  x0 = MAX (x0, xstart());
  x1 = MIN (x1, xbound() - 1);
  y0 = MAX (y0, ystart());
  y1 = MIN (y1, ybound() - 1);
  double max_dist = Point (xc0, yc0).dist (xc1, yc1);
  double wdist = 1 / MAX (1, max_dist);
  float Ca = (Aa - Ba) * wdist, Cr = (Ar - Br) * wdist, Cg = (Ag - Bg) * wdist, Cb = (Ab - Bb) * wdist;
  for (int y = y0; y <= y1; y++)
    {
      uint32 *d = m_plane.poke_span (x0, y, x1 - x0 + 1);
      for (int x = x0; x <= x1; x++)
        {
          double lucent_dist = c0.dist (x, y);
          /* A over B = colorA * alpha + colorB * (1 - alpha) */
          double Dr = Br + Cr * lucent_dist;
          double Dg = Bg + Cg * lucent_dist;
          double Db = Bb + Cb * lucent_dist;
          double Da = Ba + Ca * lucent_dist;
          /* dither */
          union { uint32 r; uint8 a[4]; } z = { quick_rand32() };
          uint32 dr = dtoi32 (Dr * 0xff) + z.a[3];
          uint32 dg = dtoi32 (Dg * 0xff) + z.a[2];
          uint32 db = dtoi32 (Db * 0xff) + z.a[1];
          uint32 da = dtoi32 (Da * 0xff) + z.a[0];
          /* apply */
          *d++ = Color (dr >> 8, dg >> 8, db >> 8, da >> 8);
        }
    }
  /* timings:
   * span+quick-rand+floatcolor:   	0m1.412s (63.8%)
   * span+quick-rand+dblcolor:   	0m1.464s
   * span+quick-rand:   		0m2.380s
   * poke-span:         		0m3.764s
   * original:            		0m3.904s (100%)
   * sqrt(float):       		0m4.140s
   * coarse2:           		0m4.390s
   * coarse3:           		0m4.460s
   * -ffast-math:       		0m4.418s
   * builtin_hypot:     		0m4.930s
   * hypot:             		0m4.948s
   */
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
  const double xsfloor = floor (xs), xeceil = ceil (xe);
  uint32 *d = m_plane.poke_span (xsfloor, by, xeceil - xsfloor);
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
