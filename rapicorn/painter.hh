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
#ifndef __RAPICORN_PAINTER_HH__
#define __RAPICORN_PAINTER_HH__

#include <rapicorn/primitives.hh>

namespace Rapicorn {

class Painter {
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
  void          draw_shaded_rect        (int xc0, int yc0, Color color0, int xc1, int yc1, Color color1);
  void          draw_filled_rect        (int x, int y, int width, int height, Color fill_color);
};

} // Rapicorn

#endif  /* __RAPICORN_PAINTER_HH__ */
