// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_PAINTER_HH__
#define __RAPICORN_PAINTER_HH__

#include <ui/primitives.hh>

namespace Rapicorn {

/// Cairo painting helper class.
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

// == Cairo Utilities ==
cairo_surface_t*        cairo_surface_from_pixmap       (Pixmap pixmap);

} // Rapicorn

#endif  /* __RAPICORN_PAINTER_HH__ */
