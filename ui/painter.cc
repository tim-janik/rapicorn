// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#include "painter.hh"
#include "blitfuncs.hh"
#include <algorithm>

namespace Rapicorn {

/* --- CPainter --- */
CPainter::CPainter (cairo_t *_context) :
  cr (_context)
{}

CPainter::~CPainter ()
{}

void
CPainter::draw_border (int x, int y, int width, int height, Color border, const vector<double> &dashes, double dash_offset)
{
  const double line_width = 1.0, l2 = line_width / 2.;
  cairo_set_line_width (cr, line_width);
  if (width && height)
    {
      Rect r (x + l2, y + l2, width - 1, height - 1);
      cairo_set_source_rgba (cr, border.red1(), border.green1(), border.blue1(), border.alpha1());
      cairo_rectangle (cr, r.x, r.y, r.width, r.height);
      cairo_set_dash (cr, dashes.data(), dashes.size(), dash_offset);
      cairo_stroke (cr);
    }
}

void
CPainter::draw_shadow (int x, int y, int width, int height,
                       Color outer_upper_left, Color inner_upper_left,
                       Color inner_lower_right, Color outer_lower_right)
{
  cairo_set_line_width (cr, 1.0);
  // outer upper left triangle
  if (width >= 1 && height >= 1)
    {
      cairo_set_source_rgba (cr, outer_upper_left.red1(), outer_upper_left.green1(),
                             outer_upper_left.blue1(), outer_upper_left.alpha1());
      cairo_move_to (cr, x + .5, y + height);
      cairo_rel_line_to (cr, 0, -(height - .5));
      cairo_rel_line_to (cr, +width - .5, 0);
      cairo_stroke (cr);
    }
  // outer lower right triangle
  if (width >= 2 && height >= 2)
    {
      cairo_set_source_rgba (cr, outer_lower_right.red1(), outer_lower_right.green1(),
                             outer_lower_right.blue1(), outer_lower_right.alpha1());
      cairo_move_to (cr, x + 1, y + height - .5);
      cairo_rel_line_to (cr, +width - 1.5, 0);
      cairo_rel_line_to (cr, 0, -(height - 1.5));
      cairo_stroke (cr);
    }
  // inner upper left triangle
  if (width >= 3 && height >= 3)
    {
      cairo_set_source_rgba (cr, inner_upper_left.red1(), inner_upper_left.green1(),
                             inner_upper_left.blue1(), inner_upper_left.alpha1());
      cairo_move_to (cr, x + 1.5, y + (height - 1));
      cairo_rel_line_to (cr, 0, -(height - 2.5));
      cairo_rel_line_to (cr, +width - 2.5, 0);
      cairo_stroke (cr);
    }
  // inner lower right triangle
  if (width >= 4 && height >= 4)
    {
      cairo_set_source_rgba (cr, inner_lower_right.red1(), inner_lower_right.green1(),
                             inner_lower_right.blue1(), inner_lower_right.alpha1());
      cairo_move_to (cr, x + 2, y + height - 1.5);
      cairo_rel_line_to (cr, +width - 3.5, 0);
      cairo_rel_line_to (cr, 0, -(height - 3.5));
      cairo_stroke (cr);
    }
}

void
CPainter::draw_filled_rect (int x, int y, int width, int height, Color fill)
{
  cairo_set_source_rgba (cr, fill.red1(), fill.green1(), fill.blue1(), fill.alpha1());
  cairo_rectangle (cr, x, y, width, height);
  cairo_fill (cr);
}

void
CPainter::draw_shaded_rect (int xc0, int yc0, Color color0, int xc1, int yc1, Color color1)
{
  cairo_pattern_t *gradient = cairo_pattern_create_linear (xc0, yc0, xc1, yc1);
  cairo_pattern_add_color_stop_rgba (gradient, 0,
                                     color0.red1(), color0.green1(),
                                     color0.blue1(), color0.alpha1());
  cairo_pattern_add_color_stop_rgba (gradient, 1,
                                     color1.red1(), color1.green1(),
                                     color1.blue1(), color1.alpha1());
  cairo_set_source (cr, gradient);
  cairo_pattern_destroy (gradient);
  cairo_rectangle (cr, MIN (xc0, xc1), MIN (yc0, yc1),
                   MAX (xc0, xc1) - MIN (xc0, xc1) + 1,
                   MAX (yc0, yc1) - MIN (yc0, yc1) + 1);
  cairo_fill (cr);
}

void
CPainter::draw_center_shade_rect (int xc0, int yc0, Color color0, int xc1, int yc1, Color color1)
{
  draw_shaded_rect (xc0, yc0, color0, xc1, yc1, color1);
}

void
CPainter::draw_dir_arrow (double x, double y, double width, double height, Color fill, DirType dir)
{
  double xhalf = width / 2., yhalf = height / 2.;
  switch (dir)
    {
    case DIR_RIGHT:
      cairo_move_to (cr, x + width, y + yhalf);
      cairo_rel_line_to (cr, -width, +yhalf);
      cairo_rel_line_to (cr, 0, -height);
      cairo_close_path (cr);
      break;
    case DIR_DOWN:
      cairo_move_to (cr, x, y);
      cairo_rel_line_to (cr, +width, 0);
      cairo_rel_line_to (cr, -xhalf, height);
      cairo_close_path (cr);
      break;
    case DIR_LEFT:
      cairo_move_to (cr, x, y + yhalf);
      cairo_rel_line_to (cr, +width, -yhalf);
      cairo_rel_line_to (cr, 0, +height);
      cairo_close_path (cr);
      break;
    case DIR_UP:
      cairo_move_to (cr, x + width, y + height);
      cairo_rel_line_to (cr, -width, 0);
      cairo_rel_line_to (cr, +xhalf, -height);
      cairo_close_path (cr);
      break;
    default: ;
    }
  cairo_set_source_rgba (cr, fill.red1(), fill.green1(), fill.blue1(), fill.alpha1());
  cairo_fill (cr);
}

} // Rapicorn
