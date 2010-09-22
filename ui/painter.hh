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

#include <ui/primitives.hh>

namespace Rapicorn {

class CPainter {
protected:
  cairo_t *cr;
public:
  explicit      CPainter                (cairo_t *context);
  virtual      ~CPainter                ();
  void          draw_filled_rect        (int x, int y, int width, int height, Color fill_color);
  void          draw_shaded_rect        (int xc0, int yc0, Color color0, int xc1, int yc1, Color color1);
  void          draw_center_shade_rect  (int xc0, int yc0, Color color0, int xc1, int yc1, Color color1);
  void          draw_border             (int x, int y, int width, int height, Color border, const vector<double> &dashes = vector<double>(), double dash_offset = 0.5);
  void          draw_shadow             (int x, int y, int width, int height,
                                         Color outer_upper_left, Color inner_upper_left,
                                         Color inner_lower_right, Color outer_lower_right);
  void          draw_dir_arrow          (double x, double y, double width, double height, Color c, DirType dir);
};

} // Rapicorn

#endif  /* __RAPICORN_PAINTER_HH__ */
