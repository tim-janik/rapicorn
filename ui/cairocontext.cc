// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "cairo-inlined/cairo.h"
#include "cairocontext.hh"

namespace Rapicorn {

/* --- Painter --- */
CairoContext::CairoContext() :
  cr (NULL)
{}
CairoContext::~CairoContext()
{
  set_cairo (NULL);
}

void
CairoContext::set_cairo (cairo_t *n_cr)
{
  if (n_cr)
    n_cr = cairo_reference (n_cr);
  if (cr)
    cairo_destroy (cr);
  cr = n_cr;
}

void
CairoContext::save ()
{
  cairo_save (cr);
}

void
CairoContext::restore ()
{
  cairo_restore (cr);
}

void
CairoContext::set_tolerance (double tolerance)
{
  cairo_set_tolerance (cr, tolerance);
}

static cairo_antialias_t
convert_antialias (CairoContext::Antialias antialias)
{
  switch (antialias)
    {
    default:
    case CairoContext::CAIRO_ANTIALIAS_DEFAULT:       return CAIRO_ANTIALIAS_DEFAULT;
    case CairoContext::CAIRO_ANTIALIAS_NONE:          return CAIRO_ANTIALIAS_NONE;
    case CairoContext::CAIRO_ANTIALIAS_GRAY:          return CAIRO_ANTIALIAS_GRAY;
    case CairoContext::CAIRO_ANTIALIAS_SUBPIXEL:      return CAIRO_ANTIALIAS_SUBPIXEL;
    }
}

void
CairoContext::set_antialias (Antialias antialias)
{
  cairo_set_antialias (cr, convert_antialias (antialias));
}

static cairo_fill_rule_t
convert_fill_rule (CairoContext::FillRule fill_rule)
{
  switch (fill_rule)
    {
    default:
    case CairoContext::CAIRO_FILL_RULE_WINDING:         return CAIRO_FILL_RULE_WINDING;
    case CairoContext::CAIRO_FILL_RULE_EVEN_ODD:        return CAIRO_FILL_RULE_EVEN_ODD;
    }
}

void
CairoContext::set_fill_rule (FillRule  fill_rule)
{
  cairo_set_fill_rule (cr, convert_fill_rule (fill_rule));
}

void
CairoContext::set_line_width (double    width)
{
  cairo_set_line_width (cr, width);
}

static cairo_line_cap_t
convert_line_cap (CairoContext::LineCap line_cap)
{
  switch (line_cap)
    {
    default:
    case CairoContext::CAIRO_LINE_CAP_BUTT:     return CAIRO_LINE_CAP_BUTT;
    case CairoContext::CAIRO_LINE_CAP_ROUND:    return CAIRO_LINE_CAP_ROUND;
    case CairoContext::CAIRO_LINE_CAP_SQUARE:   return CAIRO_LINE_CAP_SQUARE;
    }
}

void
CairoContext::set_line_cap (LineCap line_cap)
{
  cairo_set_line_cap (cr, convert_line_cap (line_cap));
}

static cairo_line_join_t
convert_line_join (CairoContext::LineJoin line_join)
{
  switch (line_join)
    {
    default:
    case CairoContext::CAIRO_LINE_JOIN_MITER:   return CAIRO_LINE_JOIN_MITER;
    case CairoContext::CAIRO_LINE_JOIN_ROUND:   return CAIRO_LINE_JOIN_ROUND;
    case CairoContext::CAIRO_LINE_JOIN_BEVEL:   return CAIRO_LINE_JOIN_BEVEL;
    }
}

void
CairoContext::set_line_join (LineJoin line_join)
{
  cairo_set_line_join (cr, convert_line_join (line_join));
}

void
CairoContext::set_miter_limit (double limit)
{
  cairo_set_miter_limit (cr, limit);
}

void
CairoContext::set_dash (double   *dashes,
                        int       num_dashes,
                        double    offset)
{
  cairo_set_dash (cr, dashes, num_dashes, offset);
}

void
CairoContext::set_source_color (Color c)
{
  cairo_set_source_rgba (cr, c.red() / 255., c.green() / 255., c.blue() / 255., c.alpha() / 255.);
}

void
CairoContext::translate (double x, double y)
{
  cairo_translate (cr, x, y);
}

void CairoContext::new_path ()
{
  cairo_new_path (cr);
}

void
CairoContext::move_to (double x,  double y)
{
  cairo_move_to (cr, x, y);
}

void
CairoContext::line_to (double x,  double y)
{
  cairo_line_to (cr, x, y);
}

void
CairoContext::rel_move_to (double x,  double y)
{
  cairo_rel_move_to (cr, x, y);
}

void
CairoContext::rel_line_to (double x,  double y)
{
  cairo_rel_line_to (cr, x, y);
}

void
CairoContext::rectangle (double x, double y, double width, double height)
{
  cairo_rectangle (cr, x, y, width, height);
}

void
CairoContext::curve_to (double x1, double y1,
                        double x2, double y2,
                        double x3, double y3)
{
  cairo_curve_to (cr, x1, y1, x2, y2, x3, y3);
}

void
CairoContext::arc (double xc, double yc, double radius,
                   double angle1, double angle2)
{
  cairo_arc (cr, xc, yc, radius, angle1, angle2);
}

void
CairoContext::arc_negative (double xc, double yc, double radius,
                            double angle1, double angle2)
{
  cairo_arc_negative (cr, xc, yc, radius, angle1, angle2);
}

void
CairoContext::close_path ()
{
  cairo_close_path (cr);
}

void
CairoContext::paint ()
{
  cairo_paint (cr);
}

void
CairoContext::stroke ()
{
  cairo_stroke (cr);
}

void
CairoContext::stroke_preserve ()
{
  cairo_stroke_preserve (cr);
}

void
CairoContext::fill ()
{
  cairo_fill (cr);
}

void
CairoContext::fill_preserve ()
{
  cairo_fill_preserve (cr);
}

CairoContext*
CairoContext::cairo_context_from_plane (Plane &plane)
{
  uint32 *buffer = plane.peek (0, 0);
  cairo_surface_t *cs = cairo_image_surface_create_for_data ((uint8*) buffer, CAIRO_FORMAT_ARGB32, plane.width(), plane.height(), plane.pixstride());
  cairo_t *cr = cairo_create (cs);
  cairo_surface_destroy (cs);
  cairo_translate (cr, -plane.xstart(), -plane.ystart());
  CairoContext *cc = new CairoContext;
  cc->set_cairo (cr);
  return cc;
}

CairoPainter::CairoPainter (Plane &plane) :
  Painter (plane),
  cc (*CairoContext::cairo_context_from_plane (plane_))
{}

CairoPainter::~CairoPainter()
{
  delete &cc;
}

void
CairoPainter::draw_arrow (double x, double y, double width, double height, Color c, double angle)
{
  double mx = x + width / 2, my = y + height / 2;
  double angle0 = (angle + 0) * PI / 180., angle1 = (angle + 120) * PI / 180., angle2 = (angle - 120) * PI / 180.;
  cc.set_source_color (c);
  /* east point */
  cc.move_to (mx + cos (angle0) * width / 2, my + sin (angle0) * height / 2);
  /* north-west point */
  cc.line_to (mx + cos (angle1) * width / 2, my + sin (angle1) * height / 2);
  /* south-west point */
  cc.line_to (mx + cos (angle2) * width / 2, my + sin (angle2) * height / 2);
  //cc.move_to (x,              y);
  //cc.line_to (x + width,      y);
  //cc.line_to (x + width / 2., y + height);
  cc.close_path();
  cc.fill();
}

void
CairoPainter::draw_dir_arrow (double x, double y, double width, double height, Color c, DirType dir)
{
  double xhalf = width / 2., yhalf = height / 2.;
  cc.set_source_color (c);
  switch (dir)
    {
    default:
    case DIR_RIGHT:
      cc.move_to (x + width, y + yhalf);
      cc.line_to (x,         y + height);
      cc.line_to (x,         y);
      break;
    case DIR_UP:
      cc.move_to (x,         y);
      cc.line_to (x + width, y);
      cc.line_to (x + xhalf, y + height);
      break;
    case DIR_LEFT:
      cc.move_to (x,         y + yhalf);
      cc.line_to (x + width, y);
      cc.line_to (x + width, y + height);
      break;
    case DIR_DOWN:
      cc.move_to (x,         y + height);
      cc.line_to (x + xhalf, y);
      cc.line_to (x + width, y + height);
      break;
    }
  cc.close_path();
  cc.fill();
}

void
CairoPainter::draw_dot (double x, double y, double width, double height, Color c1, Color c2, FrameType frame)
{
  cc.rectangle (x, y, width, height);
  cc.set_source_color (c1);
  cc.fill();
}

} // Rapicorn
