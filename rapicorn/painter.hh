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
#ifndef __RAPICORN_PAINTER_HH__
#define __RAPICORN_PAINTER_HH__

#include <rapicorn/primitives.hh>

namespace Rapicorn {

class Painter {
  inline void   draw_trapezoid_run      (double by, double bx1, double bx2, double ty, double tx1, double tx2, Color fg_premul);
  inline void   draw_trapezoid_row      (double by, double bx1, double bx2, double ty, double tx1, double tx2, Color fg_premul);
protected:
  Plane &m_plane;
public:
  explicit      Painter                 (Plane &plane);
  virtual       ~Painter                ();
  int           xstart                  () const { return m_plane.xstart(); }
  int           ystart                  () const { return m_plane.ystart(); }
  int           width                   () const { return m_plane.width(); }
  int           height                  () const { return m_plane.height(); }
  int           xbound                  () const { return m_plane.xbound(); }
  int           ybound                  () const { return m_plane.ybound(); }
  void          set_premultiplied       (int x, int y, Color c) { m_plane.set_premultiplied (x, y, c); }
  void          set                     (int x, int y, Color c) { m_plane.set (x, y, c); }
  void          draw_hline              (int x0, int x1, int y, Color c, const vector<int> &dashes = vector<int>(), int dash_offset = 0);
  void          draw_vline              (int x, int y0, int y1, Color c, const vector<int> &dashes = vector<int>(), int dash_offset = 0);
  void          draw_shadow             (int x, int y, int width, int height,
                                         Color outer_upper_left, Color inner_upper_left,
                                         Color inner_lower_right, Color outer_lower_right);
  void          draw_border             (int x, int y, int width, int height, Color border, const vector<int> &dashes = vector<int>(), int dash_offset = 0);
  void          draw_gradient_rect      (int64 recx, int64 recy, int64 recwidth, int64 recheight,
                                         int64 c0x, int64 c0y, Color color0,
                                         int64 c1x, int64 c1y, Color color1);
  void          draw_shaded_rect        (int xc0, int yc0, Color color0, int xc1, int yc1, Color color1);
  void          draw_filled_rect        (int x, int y, int width, int height, Color fill_color);
  void          draw_dir_arrow          (double x, double y, double width, double height, Color c, DirType dir);
  void          draw_trapezoid          (double c1y, double c1x1, double c1x2, double c2y, double c2x1, double c2x2, Color fill_color);
  void          draw_quadrangle         (const Point points[4], Color fill_color);
  void          draw_simple_line        (double x0, double y0, double x1, double y1, double thickness, Color fill_color);
private:
  void          fill_scan_line          (int64 yy, int64 x1, int64 width, Color color0);
};

} // Rapicorn

#endif  /* __RAPICORN_PAINTER_HH__ */
