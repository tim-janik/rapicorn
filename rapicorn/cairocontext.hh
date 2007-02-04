/* Rapicorn
 * Copyright (C) 2006 Tim Janik
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
#ifndef __RAPICORN_CAIRO_CONTEXT_HH__
#define __RAPICORN_CAIRO_CONTEXT_HH__

#include <rapicorn/painter.hh>
#include <rapicorn/primitives.hh>

namespace Rapicorn {

class CairoContext {
#ifndef CAIRO_H
  typedef struct _cairo cairo_t;
#endif
  void          set_cairo               (cairo_t *n_cr);
protected:
  cairo_t      *cr;
  explicit      CairoContext            ();
public:
  virtual      ~CairoContext            ();
  /* enums & constants */
  typedef enum {
    CAIRO_ANTIALIAS_NONE,
    CAIRO_ANTIALIAS_DEFAULT,
    CAIRO_ANTIALIAS_GRAY,
    CAIRO_ANTIALIAS_SUBPIXEL,
    NONE                        = CAIRO_ANTIALIAS_NONE,
    DEFAULT                     = CAIRO_ANTIALIAS_NONE,
    GRAY                        = CAIRO_ANTIALIAS_GRAY,
    SUBPIXEL                    = CAIRO_ANTIALIAS_SUBPIXEL,
  } Antialias;
  typedef enum {
    CAIRO_FILL_RULE_WINDING,
    CAIRO_FILL_RULE_EVEN_ODD,
    WINDING                     = CAIRO_FILL_RULE_WINDING,
    EVEN_ODD                    = CAIRO_FILL_RULE_EVEN_ODD,
  } FillRule;
  typedef enum {
    CAIRO_LINE_CAP_BUTT,
    CAIRO_LINE_CAP_ROUND,
    CAIRO_LINE_CAP_SQUARE,
    BUTT                        = CAIRO_LINE_CAP_BUTT,
    // ROUND                    = CAIRO_LINE_CAP_ROUND,
    SQUARE                      = CAIRO_LINE_CAP_SQUARE,
  } LineCap;
  typedef enum {
    CAIRO_LINE_JOIN_MITER,
    CAIRO_LINE_JOIN_ROUND,
    CAIRO_LINE_JOIN_BEVEL,
    MITER                       = CAIRO_LINE_JOIN_MITER,
    // ROUND                    = CAIRO_LINE_JOIN_ROUND,
    BEVEL                       = CAIRO_LINE_JOIN_BEVEL,
  } LineJoin;
  typedef enum { ROUND } _Round; /* conflicting enums */
  void          save                    ();
  void          restore                 ();
  void          set_tolerance           (double    tolerance);
  void          set_antialias           (Antialias antialias = CAIRO_ANTIALIAS_DEFAULT);
  void          set_fill_rule           (FillRule  fill_rule);
  void          set_line_width          (double    width);
  void          set_line_cap            (LineCap   line_cap);
  void          set_line_cap            (_Round    round)       { set_line_cap (CAIRO_LINE_CAP_ROUND); }
  void          set_line_join           (LineJoin  line_join);
  void          set_line_join           (_Round    round)       { set_line_join (CAIRO_LINE_JOIN_ROUND); }
  void          set_miter_limit         (double    limit);
  void          set_dash                (double   *dashes,
                                         int       num_dashes,
                                         double    offset);
  void          set_source_color        (Color     color);
  void          translate               (double x,  double y);
  void          new_path                ();
  void          move_to                 (double x,  double y);
  void          line_to                 (double x,  double y);
  void          rel_move_to             (double x,  double y);
  void          rel_line_to             (double x,  double y);
  void          rectangle               (double x, double y, double width, double height);
  void          curve_to                (double x1, double y1,
                                         double x2, double y2,
                                         double x3, double y3);
  void          arc                     (double xc, double yc, double radius,
                                         double angle1, double angle2);
  void          arc_negative            (double xc, double yc, double radius,
                                         double angle1, double angle2);
  void          close_path              ();
  void          paint                   ();
  void          stroke                  ();
  void          stroke_preserve         ();
  void          fill                    ();
  void          fill_preserve           ();
  static CairoContext*
  cairo_context_from_plane (Plane &plane);
};

class CairoPainter : virtual public Painter {
  CairoContext &cc;
public:
  explicit      CairoPainter            (Plane &plane);
  virtual       ~CairoPainter           ();
  void          draw_arrow              (double x, double y, double width, double height, Color c, double angle);
  void          draw_dir_arrow          (double x, double y, double width, double height, Color c, DirType dir);
  void          draw_dot                (double x, double y, double width, double height, Color c1, Color c2, FrameType frame);
};

} // Rapicorn

#endif  /* __RAPICORN_CAIRO_CONTEXT_HH__ */
