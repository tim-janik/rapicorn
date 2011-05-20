/* Rapicorn Examples
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
#include <rapicorn.hh>

namespace {
using namespace Rapicorn;

static void
drawable_draw (Display  &display,
               Drawable &drawable)
{
  cairo_t *cr = display.create_cairo();
  Rect area = drawable.allocation();
  const double lthickness = 2.25;
  cairo_set_source_rgba (cr, 0, 0, 0, 1);
  cairo_set_line_width (cr, lthickness);
  cairo_move_to (cr, area.x + 15, area.y + 15), cairo_line_to (cr, area.x + 35, area.y + 35);
  cairo_move_to (cr, area.x + 35, area.y + 35), cairo_line_to (cr, area.x + 50, area.y + 20);
  cairo_move_to (cr, area.x + 50, area.y + 20), cairo_line_to (cr, area.x + 75, area.y + 90);
  cairo_move_to (cr, area.x + 75, area.y + 90), cairo_line_to (cr, area.x + 230, area.y + 93);
  cairo_stroke (cr);
  cairo_set_line_width (cr, lthickness * 0.5);
  cairo_move_to (cr, area.x + 75, area.y + 120), cairo_line_to (cr, area.x + 230, area.y + 110);
}

extern "C" int
main (int   argc,
      char *argv[])
{
  /* initialize Rapicorn and its gtk backend */
  app.init_with_x11 (&argc, &argv, "Graphics");
  /* initialization acquired global Rapicorn mutex */

  /* load GUI definition file, relative to argv[0] */
  app.auto_load ("RapicornTest", "graphics.xml", argv[0]);

  /* create root item */
  Wind0w &wind0w = *app.create_wind0w ("graphics-dialog");

  /* hook up drawable test */
  Root &root = wind0w.root();
  Drawable &drawable = root.interface<Drawable&>();
  drawable.sig_draw += slot (&drawable_draw, drawable);

  wind0w.show();

  app.execute_loops();

  return 0;
}

} // anon
