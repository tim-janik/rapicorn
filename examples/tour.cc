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
#include <rapicorn/rapicorn.hh>

namespace {
using namespace Rapicorn;

static bool
custom_commands (Window       &window,
                 const String &command,
                 const String &args)
{
  printout ("%s(): custom command: %s(%s) (window: %s)\n", __func__, command.c_str(), args.c_str(), window.root().name().c_str());
  return true;
}

static void
drawable_draw (Display  &display,
               Drawable &drawable)
{
  Plane &plane = display.create_plane();
  Painter painter (plane);
  Rect area = drawable.allocation();
  Color fg = 0xff000000;
  double lthickness = 2.25;
  painter.draw_simple_line (area.x + 15, area.y + 15, area.x + 35, area.y + 35, lthickness, fg);
  painter.draw_simple_line (area.x + 35, area.y + 35, area.x + 50, area.y + 20, lthickness, fg);
  painter.draw_simple_line (area.x + 50, area.y + 20, area.x + 75, area.y + 90, lthickness, fg);
  painter.draw_simple_line (area.x + 75, area.y + 90, area.x + 230, area.y + 93, lthickness, fg);
  painter.draw_simple_line (area.x + 75, area.y + 120, area.x + 230, area.y + 110, lthickness * 0.5, fg);
}

#include "../rapicorn-core/tests/testpixs.c" // alpha_rle alpha_raw rgb_rle rgb_raw

extern "C" int
main (int   argc,
      char *argv[])
{
  /* initialize Rapicorn and its gtk backend */
  Application::init_with_x11 (&argc, &argv, "TourTest");
  /* initialization acquired global Rapicorn mutex */

  /* register builtin images */
  Application::pixstream ("testimage-alpha-rle", alpha_rle);
  Application::pixstream ("testimage-alpha-raw", alpha_raw);
  Application::pixstream ("testimage-rgb-rle", rgb_rle);
  Application::pixstream ("testimage-rgb-raw", rgb_raw);

  /* load GUI definition file, relative to argv[0] */
  Application::auto_load ("RapicornTest", "tour.xml", argv[0]);

  /* create root item */
  Window window = Application::create_window ("tour-dialog");
  window.commands += custom_commands;

  /* hook up drawable test */
  Root &root = window.root();
  Drawable &drawable = root.interface<Drawable&>();
  drawable.sig_draw += slot (&drawable_draw, drawable);

  window.show();

  Application::execute_loops();

  return 0;
}

} // anon
